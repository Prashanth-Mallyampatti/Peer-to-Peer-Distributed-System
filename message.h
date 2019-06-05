// vim: et ts=4 sw=4
#ifndef MESSAGE_H
#define MESSAGE_H

#include <iostream>
#include <sstream>
#include "string"
#include "vector"
#include <chrono>
#include <ctime>
#include "read.h"
#include <numeric>
#include <iterator>

using namespace std;

static const string TITLE = "Title";
static const string ADD = "ADD";
static const string LOOKUP = "LOOKUP";
static const string LIST = "LIST";
static const string RFC = "RFC";
static const string HOST = "Host";
static const string PORT_NUM = "Port";
static const string ALL = "ALL";
static const string OK = "200 OK";
static const string BAD = "400 Bad Request";
static const string NOT_FOUND = "404 Not Found";
static const string UNSUPPORTED = "505 P2P-CI Version Not Supported";
static const string VERSION = "P2P-CI/1.0";
static const string GET = "GET";
static const string OS = "OS";
static const string DATE = "DATE";
static const string LAST_MOD = "Last-Modified";
static const string LEN = "Content-Length";
static const string TYPE = "Content-Type";

bool is_number(const std::string& s) {
    return !s.empty() && s.find_first_not_of("0123456789") == 
        std::string::npos;
}

bool hasStr(const string& lhs, const string& rhs, bool strict = false) {
    if (strict) {
        return lhs.find(rhs) != std::string::npos;
    }
    else {
        return lhs.find(rhs) != std::string::npos or
            rhs.find(lhs) != std::string::npos;
    }
}

class BaseMessage {
public:
    virtual void format() = 0;
    virtual void pack(string& bytes) = 0;
    virtual void unpack(const string& bytes) = 0;
};

class PeerRequestMessage : public BaseMessage {
public:
    enum class METHOD {GET, POST};
    PeerRequestMessage() { }
    PeerRequestMessage(string hostname, string os,
                       PeerRequestMessage::METHOD method,
                       string rfc, string version) :
        hostname_(hostname), os_(os), method_(method),
        rfc_(rfc), version_(version) { }

    string hostname_;
    string os_;
    METHOD method_;
    string rfc_;
    string version_;

    void format() {
        cout << "***********In Peer Request Format**********" << endl;
        cout << "RFC: " << rfc_ << endl;
        cout << "Version: " << version_ << endl;
        cout << "Host: " << hostname_ << endl;
        cout << "OS: " << os_ << endl;
        cout << "*******************************************" << endl;
    }

    bool hasError() {
        if (hostname_.empty() or 
                !is_number(rfc_)) { 
            return true;
        }
        return false;
    }

    bool correctVersion() {
        if (version_ == VERSION)
            return true;
        else 
            return false;
    }

    void pack(string& packet) {
        packet += GET + " ";
        packet += RFC + " ";
        packet += rfc_ + " ";
        packet += version_ + "\n";

        packet += HOST + ":" + " " + hostname_  + "\n";
        packet += OS + ": " + os_;
    }

    void unpack(const string& packet) {
        std::stringstream ss(packet);
        std::istream_iterator<std::string> begin(ss);
        std::istream_iterator<std::string> end;
        std::vector<std::string> vstrings(begin, end);

        if (vstrings[0].find_first_of(GET) != std::string::npos) {
            method_ = METHOD::GET;
        }

        if (vstrings[1].find_first_of(RFC) != std::string::npos) {
            rfc_ = vstrings[2];
        }
        else
            return;

        version_ = vstrings[3];
        hostname_ = vstrings[5];

        for (int i = 7; i < vstrings.size(); i++)
            os_ += vstrings[i] + " ";
    }
};

class PeerResponseMessage : public BaseMessage {
public:
    enum class STATUS_CODE {OK, BAD_REQUEST, NOT_FOUND, VERSION_NOT_SUPPORTED};

    PeerResponseMessage() {}

    PeerResponseMessage(string os, vector<string> last_mod,
                        vector<string> length, vector<string> type,
                        vector<vector<string>> fc,
                        STATUS_CODE method) :
        os_(os), last_mod_(last_mod), length_(length), type_(type),
        file_content(fc), status_code_(method) {}

    string os_;
    vector<string> last_mod_;
    vector<string> length_;
    vector<string> type_;
    vector<vector<string>> file_content;
    STATUS_CODE status_code_;
    string date_;
    string version_;

    void format() {
        cout << "******In peer respose format******" << endl;
        cout << version_ << " " << OK << endl;
        cout << DATE << ": " << date_ << endl;
        cout << OS << ": " << os_ << endl;
        cout << LAST_MOD << ": " << (last_mod_.back()) << endl;
        cout << LEN << ": " << (length_.back()) << endl;
        cout << TYPE << ": " << (type_.back()) << endl;

        for (auto iter : (file_content.back()))
            cout << iter << endl;
        cout << "**********************************" << endl;
    }

    void pack(string& packet) {
        packet += VERSION + " ";

        string errorCode;
        if (status_code_ == PeerResponseMessage::STATUS_CODE::BAD_REQUEST)
            errorCode = BAD;
        if (status_code_ == PeerResponseMessage::STATUS_CODE::NOT_FOUND)
            errorCode = NOT_FOUND;
        if (status_code_ == PeerResponseMessage::STATUS_CODE::VERSION_NOT_SUPPORTED)
            errorCode = UNSUPPORTED; 

        packet += errorCode + " \n";
        packet += DATE + ": ";
        auto end = std::chrono::system_clock::now();
        std::time_t end_time = std::chrono::system_clock::to_time_t(end);
        packet += std::ctime(&end_time);

        packet += OS + ": " + os_ + "\n";

        for (int i = 0; i < length_.size(); i++) {

            packet += LAST_MOD + ":" +  last_mod_[i] + "\n";
            packet += LEN + ":" + length_[i] + "\n";
            packet += TYPE + ":" + type_[i] + "\n";
            vector<string> fc = file_content[i];

            for (auto iter : fc) {
                packet += iter + "\n";
            }
        }
    }

    void unpack(const string& packet) {
        std::stringstream ss(packet);
        char line_char[4096];

        ss.getline(line_char, 1024, '\n');
        string linestring(line_char);

        version_ = linestring.substr(0, 10);

        ss.getline(line_char, 1024, '\n');
        date_ = string(&line_char[5]);

        ss.getline(line_char, 1024, '\n');
        os_ = string(&line_char[4]);

        ss.getline(line_char, 1024, '\n');
        last_mod_.push_back(string(&line_char[LAST_MOD.size() + 1]));

        ss.getline(line_char, 1024, '\n');
        length_.push_back(string(&line_char[LEN.size() + 1]));

        ss.getline(line_char, 1024, '\n');
        type_.push_back(string(&line_char[TYPE.size() + 1]));


        // store the file
        vector<string> rfc_content;
        while(ss.getline(line_char, 1024))
            rfc_content.push_back(string(line_char));

        file_content.push_back(rfc_content);

    }
};

class ServerRequestMessage : public BaseMessage {
public:
    ServerRequestMessage() {}
    enum class METHOD {ADD, LOOKUP, LIST};
    enum class LIST_METHOD {ALL};

    string hostname_;
    string port_;
    string title_;
    // rfc_ should be of type integer
    string rfc_;
    string version_;
    METHOD method_;

    ServerRequestMessage(const ServerRequestMessage& sm) {
        hostname_ = sm.hostname_;
        port_ = sm.port_;
        title_ = sm.title_;
        rfc_ = sm.rfc_;
        version_ = sm.version_;
        method_ = sm.method_;
    }

    ServerRequestMessage(string hostname, string port, 
        string title, string rfc, string version,
        METHOD method) : hostname_(hostname), port_(port), title_(title),
                         rfc_(rfc), version_(version), method_(method) {
    }

    void format() {
        cout << "**************************************" << endl;
        cout << "Hostname: " << hostname_ << endl;
        cout << "port: " << port_ << endl;
        cout << "title: " << title_ << endl;
        cout << "rfc: " << rfc_ << endl;
        cout << "version: " << version_ << endl;
        cout << "***************************************" << endl;
    }

    bool hasError() {
        if (method_ == ServerRequestMessage::METHOD::ADD or
                method_ == ServerRequestMessage::METHOD::LOOKUP) {
            if (hostname_.empty() or 
                    !is_number(port_) or 
                    !is_number(rfc_) or 
                    title_.empty())
                return true;
        }
        else if (method_ == ServerRequestMessage::METHOD::LIST) {
           if (hostname_.empty() or
                !is_number(port_))
                return true;
        }
        return false;
    }

    bool correctVersion() {
        if (version_ == VERSION)
            return true;
        else 
            return false;
    }

    void pack(string& packet) {
        switch (method_) {
        case METHOD::ADD:
            packet += "ADD";
            packet += " RFC ";
            packet += rfc_;
            packet += " ";
            packet += version_ + " \n";

            packet += "Host: ";
            packet += hostname_ + "\n";

            packet += "Port: ";
            packet += port_ + "\n";

            packet += "Title: ";
            packet += title_ + "\n";
            break;

        case METHOD::LOOKUP:
            packet += "LOOKUP";
            packet += " RFC ";
            packet += rfc_;
            packet += " ";
            packet += version_ + "\n";

            packet += "Host: ";
            packet += hostname_ + "\n";

            packet += "Port: ";
            packet += port_ + "\n";

            packet += "Title: ";
            packet += title_ + "\n";
            break;
        case METHOD::LIST:
            packet += "LIST";
            packet += " " + ALL + " ";
            packet += version_ + "\n";

            packet += "Host: ";
            packet += hostname_ + "\n";

            packet += "Port: ";
            packet += port_ + "\n";
            break;
        }
    }

    void unpack(const string& packet) {
        istringstream ss(packet);
        string msg_word;

        ss >> msg_word;

        if (msg_word == ADD) {
            method_ = METHOD::ADD;
        }
        else if (msg_word == LIST) {
            method_ = METHOD::LIST;
        }
        else if (msg_word == LOOKUP) {
            method_ = METHOD::LOOKUP;
        }

        switch (method_) {
        case METHOD::ADD:

            ss >> msg_word;

            if (msg_word == RFC) {
                ss >> msg_word;
                rfc_ = msg_word;
            }
            else
                return;

            ss >> msg_word;
            version_ = msg_word;

            ss >> msg_word;
            if (HOST.find_first_of(msg_word) != std::string::npos) {
                ss >> msg_word;
                hostname_ = msg_word;
            }
            else
                return;

            ss >> msg_word;
            if (PORT_NUM.find_first_of(msg_word) != std::string::npos) {
                ss >> msg_word;
                port_ = msg_word;
            }
            else
                return;

            ss >> msg_word;
            if (TITLE.find_first_of(msg_word) != std::string::npos) {
                while (ss >> msg_word)
                    title_ += msg_word + " ";
            }
            else
                return;
        break;
        case METHOD::LOOKUP:
            ss >> msg_word;

            if (msg_word == RFC) {
                ss >> msg_word;
                rfc_ = msg_word;
            }
            else
                return;

            ss >> msg_word;
            version_ = msg_word;

            ss >> msg_word;
            if (HOST.find_first_of(msg_word) != std::string::npos) {
                ss >> msg_word;
                hostname_ = msg_word;
            }
            else
                return;

            ss >> msg_word;
            if (PORT_NUM.find_first_of(msg_word) != std::string::npos) {
                ss >> msg_word;
                port_ = msg_word;
            }
            else
                return;

            ss >> msg_word;
            if (TITLE.find_first_of(msg_word) != std::string::npos) {
                while (ss >> msg_word)
                    title_ += msg_word + " ";
            }
        break;
        case METHOD::LIST:
            ss >> msg_word;

            if (msg_word == ALL) {
                ss >> msg_word;
                version_ = msg_word;

                ss >> msg_word;

                if (HOST.find_first_of(msg_word) != std::string::npos) {
                    ss >> msg_word;
                    hostname_ = msg_word;
                }
                else 
                    return;
                ss >> msg_word;
                if (PORT_NUM.find_first_of(msg_word) != std::string::npos) {
                    ss >> msg_word;
                    port_ = msg_word;
                }

            }
        break;
        }
        // cout << "**unpackign ends**" << endl;
    }
};

class ServerResponseMessage : public BaseMessage {
public:
    enum class STATUS_CODE {OK, BAD_REQUEST, NOT_FOUND, VERSION_NOT_SUPPORTED};

    ServerResponseMessage() { }
    ServerResponseMessage(vector<string> rfc, vector<string> title,
                          vector<string> hostname, vector<string> port) :
        rfc_(rfc), title_(title), hostname_(hostname), port_(port) { }

    vector<string> rfc_;
    vector<string> title_;
    vector<string> hostname_;
    vector<string> port_;
    string version_;
    STATUS_CODE status_;

    void format() {
        cout << "********IN FORMAT***********" << endl;
        for (int i = 0; i < rfc_.size(); i++) {
            cout << RFC << " " << rfc_[i] << endl;
            cout << HOST << " " << hostname_[i] << endl;
            cout << PORT_NUM << " " << port_[i] << endl;
            cout << TITLE << " " << title_[i] << endl;
            cout << "****************************" << endl;
        }
    }

    void pack(string& packet) {
        packet += VERSION + " ";

        switch (status_) {
        case STATUS_CODE::OK:
            packet += OK + "\n";
            for (int i = 0; i < rfc_.size(); i++) {
                packet += RFC + " " + rfc_[i] + " " +
                    title_[i]  + " " + hostname_[i] + " " +
                    " " + port_[i] + "\n";
            }
            break;
        case STATUS_CODE::BAD_REQUEST:
            packet += BAD + "\n";
            break;
        case STATUS_CODE::NOT_FOUND:
            packet += NOT_FOUND + "\n";
            break;
        case STATUS_CODE::VERSION_NOT_SUPPORTED:
            packet += UNSUPPORTED + "\n";
            break;
        }
    }
    void unpack(const string& packet) {
        std::stringstream ss(packet);
        // First line has the status
        char line_char[1024];
        ss.getline(line_char, 1024, '\n');
        string line(line_char);

        std::stringstream linestream(line);
        std::istream_iterator<std::string> begin(linestream);
        std::istream_iterator<std::string> end;
        std::vector<std::string> vstrings(begin, end);

        version_ = vstrings[0];

        string status_code = vstrings[1];
        if (status_code.find("200") != std::string::npos) {
            status_ = STATUS_CODE::OK;

            char line_char[1024];
            while(ss.getline(line_char, 1024, '\n')) {
                string line(line_char);
                std::stringstream linestream(line);
                std::istream_iterator<std::string> begin(linestream);
                std::istream_iterator<std::string> end;
                std::vector<std::string> vstrings(begin, end);

                rfc_.push_back( vstrings[1]);
                hostname_.push_back(vstrings[vstrings.size() - 2]);
                port_.push_back(vstrings[vstrings.size() - 1]);

                string title;
                for (int i = 2; i < vstrings.size() - 2; i++) {
                    title += vstrings[i] + " ";
                }
                title_.push_back(title);
            }
        }
    }
};

#endif // MESSAGE_H
