// vim : et ts=4 sw=4
#include "iostream"
#include "map"
#include "message.h"
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <assert.h>
#include <string.h>
#include "thread"
#include <mutex>

#define PORT 7734
int num_thread = 256; // This is the maximum amount of client connections
		      // we are going to support
std::mutex sv_thread_lock;

using std::cout;
using std::endl;
using std::string;
using std::cin;
using std::map;

class ActivePeersRepository {
private:
    std::map<string, string> hostPortMap_;
public:
    typedef std::map<string, string>::iterator APIter;
    void add(string hostname, string port) {
        hostPortMap_[hostname] = port;
    }
    string lookup(string hostname) {
         return hostPortMap_[hostname];
    }
    void deleteHost(string hostname) {
	hostPortMap_.erase(hostname);
    }
};

class RFCIndexRepository {
public:
    typedef pair<string, string> HostTitle;
    typedef std::multimap<string, HostTitle>::iterator IndexIter;

    typedef pair<IndexIter, IndexIter> EqualRangeIter;

    std::multimap<string, HostTitle> field_value_map_;
    void add(string rfcNo, const HostTitle& hostport) {
        field_value_map_.insert(make_pair(rfcNo, hostport));
    }

    IndexIter lookup(string rfcNo) {
        return field_value_map_.find(rfcNo);
    }

    EqualRangeIter lookup_multi(string rfcNo) {
	return field_value_map_.equal_range(rfcNo);
    }

    void list() {
        istringstream ss;
        cout << "listing index" << endl;
        for (auto iter : field_value_map_) {
            cout << "Host " <<  iter.second.first << endl;
            cout << "Port " << iter.second.second << endl;
        }
        cout << "list ends " << endl;
    }
    void deleteHost(string hostname) {
	auto rfcIter = field_value_map_.begin();
	while (rfcIter != field_value_map_.end()) {
	    if (rfcIter->second.first == hostname) {
		rfcIter = field_value_map_.erase(rfcIter);
	    }
	    else
		rfcIter++;
	}
    }
};

class Server {
public:
    RFCIndexRepository rfcIndex;
    ActivePeersRepository activeIndex;

    void server_thread(int new_sock) {
	    string hostname;
	    while (true) {
		//cout << "Im in server thread and trying to read from"
		  //  "new client sock" << endl;
                char buffer[4096] = {0};

		int  vals = read(new_sock, buffer, 4096);
		string inp_msg = string(buffer);
		if (string(buffer).empty()) {
		    cout << "Disconnecting from a client..."  << hostname << endl;
		    rfcIndex.deleteHost(hostname);
		    activeIndex.deleteHost(hostname);
		    return;
		}

                ServerRequestMessage svReq;
                svReq.unpack(inp_msg);

		if (svReq.hasError()) {
		    ServerResponseMessage svResponse;
		    svResponse.status_ = 
			    ServerResponseMessage::STATUS_CODE::BAD_REQUEST;
		    string msg;
		    svResponse.pack(msg);
		    send(new_sock, msg.c_str(), msg.length(), 0);
		    continue;
		}

		if (!svReq.correctVersion()) {
		    ServerResponseMessage svResponse;
		    svResponse.status_ = 
			    ServerResponseMessage::STATUS_CODE::VERSION_NOT_SUPPORTED;
		    string msg;
		    svResponse.pack(msg);
		    send(new_sock, msg.c_str(), msg.length(), 0);
		    continue;
		}
		cout << "-----------------------------------------" << endl;
		cout << "Got request: " << endl << inp_msg << endl;

                ServerRequestMessage::METHOD method = svReq.method_;

                if (method == ServerRequestMessage::METHOD::ADD) {
                    ServerResponseMessage svResponse;
                    RFCIndexRepository::HostTitle hostTitle =
                        make_pair(svReq.hostname_, svReq.title_);
                    rfcIndex.add(svReq.rfc_, hostTitle);
                    activeIndex.add(svReq.hostname_, svReq.port_);

                    svResponse.rfc_.push_back(svReq.rfc_);
                    svResponse.title_.push_back(svReq.title_);
                    svResponse.hostname_.push_back(svReq.hostname_);
                    svResponse.port_.push_back(svReq.port_);
                    svResponse.status_ = ServerResponseMessage::STATUS_CODE::OK;
		    
                    hostname = svReq.hostname_;
                    string msg;
                    svResponse.pack(msg);
                    send(new_sock, msg.c_str(), msg.length(), 0);
                }
                else if (method == ServerRequestMessage::METHOD::LIST) {
		    ServerResponseMessage svResponse;
		    svResponse.status_ = ServerResponseMessage::STATUS_CODE::OK;
		    for (auto iter : rfcIndex.field_value_map_) {
			string host = iter.second.first; // host
			string title = iter.second.second; // title
			string port = activeIndex.lookup(host); // port

			svResponse.port_.push_back(port);
			svResponse.hostname_.push_back(host);
			svResponse.title_.push_back(title);
			svResponse.rfc_.push_back(iter.first);
		    }
		    string msg;
		    svResponse.pack(msg);
		    send(new_sock, msg.c_str(), msg.length(), 0);
                }
                else if (method == ServerRequestMessage::METHOD::LOOKUP) {
                    ServerResponseMessage svResponse;

		    auto iterPair = rfcIndex.lookup_multi(svReq.rfc_);
		    auto iterBegin = iterPair.first;
		    auto iterEnd = iterPair.second;

		    // This is just to see if atleast 1 element exists
                    auto iter = rfcIndex.lookup(svReq.rfc_);

		    string msg;
		    if (iter != rfcIndex.field_value_map_.end()) {
			svResponse.status_ = ServerResponseMessage::STATUS_CODE::OK;

			while (iterBegin != iterEnd) {
			    svResponse.rfc_.push_back(iterBegin->first);
			    svResponse.hostname_.push_back(iterBegin->second.first);
			    svResponse.title_.push_back(iterBegin->second.second);
			    svResponse.port_.push_back(activeIndex.lookup(
				    iterBegin->second.first));
			    ++iterBegin;
			}
		    }
		    else
			svResponse.status_ = 
				ServerResponseMessage::STATUS_CODE::NOT_FOUND;
		    svResponse.pack(msg);

                    send(new_sock, msg.c_str(), msg.length(), 0);
                }
		else {
		    return;
		}
            }
    }


    void create_server() {
	RFCIndexRepository rfcIndex;
	ActivePeersRepository activeIndex;
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR |
                   SO_REUSEPORT, &opt,
                   sizeof(opt));

        struct sockaddr_in address;
        int addrlen = sizeof(address);
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        address.sin_port = htons(PORT);

        int bnd = ::bind(sockfd, (struct sockaddr*) &address,
                       sizeof(address));
        if (bnd < 0) {
            cout << "Could not bind" << endl;
            assert(0);
        }

        int ls = listen(sockfd, 128);
        if (ls < 0) {
            cout << "Could not listen" << endl;
            assert(0);
        }
        while (true) {
            int new_sock;
            new_sock = accept(sockfd, (struct sockaddr*) &address,
                              (socklen_t*) &addrlen);

	    std::thread t (&Server::server_thread, this, new_sock);
	    t.detach();
        }
    }
};


class P2Server {
private:
    P2Server() { }
    int peerPort_;
public:
    P2Server(int peerPort) : peerPort_(peerPort) { }
    
    void create_server() {
	cout << "Starting upload server on port: " << peerPort_ << endl;

	std::thread t(&P2Server::create_server_thread, this);
	t.detach();
    }
    void create_server_thread() {
        int sockfd = socket(AF_INET, SOCK_STREAM, 0);
        int opt = 1;
        setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR |
                   SO_REUSEPORT, &opt,
                   sizeof(opt));

        struct sockaddr_in address;
        int addrlen = sizeof(address);
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = htonl(INADDR_ANY);
        address.sin_port = htons(peerPort_);

        int bnd = ::bind(sockfd, (struct sockaddr*) &address,
                       sizeof(address));
        if (bnd < 0) {
            cout << "Could not bind" << endl;
            assert(0);
        }

        int ls = listen(sockfd, 128);
        if (ls < 0) {
            cout << "Could not listen" << endl;
            assert(0);
        }

        while (true) {
            int new_sock;
            new_sock = accept(sockfd, (struct sockaddr*) &address,
                              (socklen_t*) &addrlen);

            char buffer[1024] = {0};
            int vals = read(new_sock, buffer, 1024);
	    string inp_msg(buffer);
            PeerRequestMessage prms;
            prms.unpack(inp_msg);

            string os("MAC OS");
            vector<string> last_mod;
            vector<string> length;
            vector<string> type;
            vector<vector<string>> file_content;

	    if (prms.hasError()) {
		cout << "This is bad request " << endl;
		PeerResponseMessage pResp  = PeerResponseMessage(os,
		    last_mod, length, type, file_content,
		    PeerResponseMessage::STATUS_CODE::BAD_REQUEST);
		string resp_msg;
		pResp.pack(resp_msg);
		send(new_sock, resp_msg.c_str(), resp_msg.length(), 0);
		continue;
	    }

	    if (!prms.correctVersion()) {
		PeerResponseMessage pResp  = PeerResponseMessage(os,
		    last_mod, length, type, file_content,
		    PeerResponseMessage::STATUS_CODE::VERSION_NOT_SUPPORTED);
		string resp_msg;
		pResp.pack(resp_msg);
		send(new_sock, resp_msg.c_str(), resp_msg.length(), 0);
		continue;
	    }


            vector<string> rfc_fnames;
            FileHandler::read_directory(rfc_fnames);

	    bool file_found = false;
            for (auto rfc_fname : rfc_fnames) {
                // add check for which rfc you want
                if (!hasStr(rfc_fname, prms.rfc_))
                    continue;

		file_found = true;
                string modTime;
                vector<string> vectStr;
                int size;
                FileHandler::getStrArray(vectStr, rfc_fname, size, modTime);
                string fc;
                last_mod.push_back(modTime);
                length.push_back(std::to_string(size));
                type.push_back(string("text/text"));
                file_content.push_back(vectStr);
            }

	    PeerResponseMessage pRespMsg;

	    if (file_found) {
		pRespMsg = PeerResponseMessage(os, last_mod, length, type,
			file_content,
			PeerResponseMessage::STATUS_CODE::OK);
	    }
	    else
		pRespMsg  = 
		    PeerResponseMessage(os, last_mod, length, type, file_content, 
		    PeerResponseMessage::STATUS_CODE::NOT_FOUND);

            string resp_msg;
            pRespMsg.pack(resp_msg);
            send(new_sock, resp_msg.c_str(),
                 resp_msg.length(), 0);
        }
    }
};

class Client {
public:
    Client(string server_ip, int port) :
        server_ip_(server_ip), client_sock_fd_(-1) {
        serv_addr_.sin_family = AF_INET;
        serv_addr_.sin_addr.s_addr = inet_addr(server_ip_.c_str());
        serv_addr_.sin_port = htons(port);
    }

    Client() : server_ip_("127.0.0.1"), client_sock_fd_(-1) {
        memset(&serv_addr_, '0', sizeof(serv_addr_));
        serv_addr_.sin_family = AF_INET;
        // Convert ip from string to binary
        serv_addr_.sin_addr.s_addr = inet_addr(server_ip_.c_str());
        serv_addr_.sin_port = htons(5760);
    }

    void send_msg(string message) {
        send(client_sock_fd_, message.c_str(), message.length(), 0);
    }

    string get_msg() {
        char buffer[4096] = {0};
        int vals = read(client_sock_fd_, buffer, 4096);
        if (vals == -1) {
            cout << "Error" << endl;
        }
        return string(buffer);
    }

    void create_client() {
        client_sock_fd_ = socket(AF_INET, SOCK_STREAM, 0);
        int conn = connect(client_sock_fd_, (struct sockaddr*)  &serv_addr_,
                sizeof(serv_addr_));
        if (conn < 0) {
            cout << "Connection failed" << endl;
            //assert(0);
        }
    }

    int client_sock_fd_;
    int sock_fd_;
    struct sockaddr_in serv_addr_;
    string server_ip_;
};
