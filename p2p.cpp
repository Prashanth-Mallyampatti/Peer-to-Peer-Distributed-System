// vim: et ts=4 sw=4
#include "p2p.h"
#include "message.h"

int main(int argc, char* argv[]) {
    int choice;
    cout << "0 for Bootstrap Server 1 for Client" << endl;
    cin >> choice;
    switch (choice) {
    case 0: {
        Server server;
        server.create_server();
        break;
    }
    case 1: {
        cout << "Enter Bootstrap Server ip:" << endl;
        string server_ip;
        cin >> server_ip;

        int server_port;
        cout << "Enter Bootstrap server port:" << endl;
        cin >> server_port; 

        int peer_port;
        cout << "Enter client port (peer server): " << endl;
        cin >> peer_port;

        string client_ip;
        cout << "Enter my ip: " << endl;
        cin >> client_ip;

        Client client(server_ip, server_port);
        client.create_client();

        P2Server p2server(peer_port);
        p2server.create_server();
        cout << "created peer server " << endl;
        cout << "----------------------------------------" << endl;
        cout << "Waiting for commands. Add EOCEOCEOC as a delimiter between " 
                "each command for correct functioning:" << endl;

        string choice;
        while (true) {
            cin >> choice;
            if (choice == ADD) {
                // Change this if you want to use the code. Can cause buffer
                // overflow
                char buff[1024];
                string stat;

                string cmd(ADD);

                while (true) {
                    string c_wd;
                    cin.getline(buff, 1024);
                    c_wd = string(buff);
                    if (c_wd == "EOCEOCEOC") {
                        break;
                    }
                    cmd += c_wd + "\n";
                }

                cout << " received cmd:\n ";

                ServerRequestMessage srv_req;
                srv_req.unpack(cmd);
                //req2.format();

                /*
                ServerRequestMessage srv_req(client_ip,
                                             std::to_string(peer_port), 
                                             "SOME RFC",
                                             "128",
                                             VERSION,
                                             ServerRequestMessage::METHOD::ADD);
                */

                string msg;
                srv_req.pack(msg);
                client.send_msg(msg);
                string recv_msg = client.get_msg();

                ServerResponseMessage resp_msg;
                resp_msg.unpack(recv_msg);
                // resp_msg.format();

                cout << endl << endl;
                cout << recv_msg << endl;
            }
            else if (choice == GET) {
                char buff[1024];
                string stat;

                string cmd(GET);

                while (true) {
                    string c_wd;
                    cin.getline(buff, 1024);
                    c_wd = string(buff);
                    if (c_wd == "EOCEOCEOC") {
                        break;
                    }
                    cmd += c_wd + "\n";
                }
                PeerRequestMessage prms;
                prms.unpack(cmd);

                string msg;
                prms.pack(msg);
            
                string p2ServerPort;
                cout << "Enter the upload port of the peer server: " << endl;

                cin >> p2ServerPort;
                Client client2(prms.hostname_, atoi(p2ServerPort.c_str()));
                client2.create_client();
                client2.send_msg(msg);

                string p2resp;
                p2resp = client2.get_msg();
                cout << "----------------------------------------" << endl << endl;
                cout << "Got response from peer server" << endl << endl;
                cout << p2resp;
                cout << endl;

                PeerResponseMessage peerResp;
                peerResp.unpack(p2resp);
                
                string file_content;

                for (int i = 0; i < peerResp.length_.size(); i++) {
                    vector<string> fc = peerResp.file_content[i];

                    for (auto iter : fc) {
                        file_content += iter + "\n";
                    }
                }
                // Write this rfc 
                if (!file_content.empty()) {
                    cout << "----------------------------------------" << endl;
                    cout << "Wrote rfc to RFC/ directory" << endl;
                    string rfc_fname = "RFC/" + prms.rfc_ + ".txt";
                    FileHandler::writeStr(file_content, rfc_fname); 
                }
            }
            else if (choice == LOOKUP) {
                // Change this if you want to use the code. Can cause buffer
                // overflow
                char buff[1024];
                string stat;

                string cmd(LOOKUP);

                while (true) {
                    string c_wd;
                    cin.getline(buff, 1024);
                    c_wd = string(buff);
                    if (c_wd == "EOCEOCEOC") {
                        break;
                    }
                    cmd += c_wd + "\n";
                }


                ServerRequestMessage srv_req;
                srv_req.unpack(cmd);

                /*
                ServerRequestMessage srv_req("localhost",
                                             "7793", "SOME RFC",
                                             "128",
                                             "1.0",
                                             ServerRequestMessage::METHOD::LOOKUP);
                */

                cout << "----------------------------------------" << endl;
                cout << "Sending lookup request to Bootstrap Server" << endl;
                ServerResponseMessage resp_msg;
                string msg;
                srv_req.pack(msg);
                client.send_msg(msg);
                string recv_msg = client.get_msg();
                cout << "Got a message from Bootstrap Server " << endl << endl;
                resp_msg.unpack(recv_msg);
                //resp_msg.format();
                cout << recv_msg << endl;
                cout << "----------------------------------------" << endl;

                if (resp_msg.hostname_.empty())
                    continue;
            }
            else if (choice == LIST) {

                char buff[1024];
                string stat;

                string cmd(LIST);

                while (true) {
                    string c_wd;
                    cin.getline(buff, 1024);
                    c_wd = string(buff);
                    if (c_wd == "EOCEOCEOC") {
                        break;
                    }
                    cmd += c_wd + "\n";
                }
                
                ServerRequestMessage srv_req;
                srv_req.unpack(cmd);

                cout << "----------------------------------------" << endl;
                cout << "Sending list request to Bootstrap Servrer" << endl;
                string msg;
                srv_req.pack(msg);
                client.send_msg(msg);

                ServerResponseMessage resp_msg;
                string recv_msg = client.get_msg();
                cout << "----------------------------------------" << endl;
                cout << "Got a reply from  Bootstrap Server " << endl << endl;
                resp_msg.unpack(recv_msg);
                //resp_msg.format();
                cout << recv_msg << endl;
                cout << "----------------------------------------" << endl;
            }
            else
                break;

        }
    }
    }
    return 0;
}
