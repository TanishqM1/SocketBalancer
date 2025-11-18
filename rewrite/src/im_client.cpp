#define _WINSOCK_DEPRECATED_NO_WARNINGS
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")

#include <iostream>
#include <thread>
#include <vector>
#include <string>
#include <sstream>
#include <mutex>
#include <atomic>
#include <chrono>
#include <random>

#include <filesystem>
namespace fs = std::filesystem;
using namespace std;

// protocol constant outline
static const string ONLINE_STATUS = "100 ONLINE";
static const string OFFLINE_STATUS = "101 OFFLINE";
static const string AWAY_STATUS = "102 AWAY";

static const string CODE_OK = "200 OK";
static const string CODE_INVALID = "201 INVALID";
static const string CODE_NO_SUCH = "202 NO SUCH USER";
static const string CODE_USER_EXISTS = "203 USER EXISTS";

struct BuddyStatusRecord
{
    string buddyId;
    string status;
    string ip;
    int port;

    bool isOnline() const
    {
        return status.find("100") != string::npos;
    }

    string toString() const{
        ostringstream oss;
        oss << buddyId << "\t" << status << "\t" << ip << "\t" << port;
        return oss.str();
    }
};

class IMClient {
    public:
        IMClient(const string& serverIP, int tcpPort, int udpPort):
        serverIP_(serverIP),
        tcpServerPort_(tcpPort),
        udpServerPort_(udpPort),
        userId_(""),
        status_(ONLINE_STATUS),
        pendingConnection_(INVALID_SOCKET),
        shutdown_(false)
        {
            tcpMessagePort_ = getRandomPort();
            udpThread_ = thread(&IMClient::udpPrescenceLoop, this);
            welcomeThread_ = thread(&IMClient::tcpWelcomeLoop, this);
        }

        ~IMClient(){
            shutdown_ = true;
            if (udpThread_.joinable()) udpThread_.join();
            if (welcomeThread_.joinable()) welcomeThread_.join();
        }

        void run(){
            while (!shutdown_){
                printMenu();

                string choice;
                getline(cin, choice);
                if (choice.empty()) continue;

                char c = toupper(choice[0]);
                switch (c) {
                case 'R': registerUser(); break;
                case 'L': loginUser(); break;
                case 'A': addBuddy(); break;
                case 'D': deleteBuddy(); break;
                case 'S': showBuddyStatus(); break;
                case 'M': messagBuddy(); break;
                case 'Y': acceptIncoming(); break;
                case 'N': rejectIncoming(); break;
                case 'X': shutdown_ = true; break;
                default:
                    std::cout << "Unknown command.\n";
                }
            }
            cout << "\nClient Shutting Down";
        }

    private:
        string serverIP_;
        int tcpServerPort_;
        int udpServerPort_;

        int tcpMessagePort_;
        string userId_;
        string status_;

        vector<BuddyStatusRecord> buddyList_;
        mutex buddyMutex_;

        SOCKET pendingConnection_;
        mutex pendingMutex_;

        atomic<bool> shutdown_;
        thread udpThread_;
        thread welcomeThread_;
        thread chatRecieveThread_;

        // Randomly pick a port
        int getRandomPort(){
            random_device rd;
            mt19937 rng(rd());
            uniform_int_distribution<int> dist(20000,65000);
            return dist(rng);
        }

        // console client
        void printMenu(){
            std::cout << "\n==== IM Client ====\n";
            std::cout << "R: Register\n";
            std::cout << "L: Login\n";
            std::cout << "A: Add buddy\n";
            std::cout << "D: Delete buddy\n";
            std::cout << "S: Show buddy status\n";
            std::cout << "M: Message buddy\n";
            std::cout << "Y: Accept incoming chat\n";
            std::cout << "N: Reject incoming chat\n";
            std::cout << "X: Exit\n";
            std::cout << "Enter choice: ";
        }


        // TCP Methods
        SOCKET connectTCP(){
            SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
            if (sock == INVALID_SOCKET) return INVALID_SOCKET;

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_port = htons(tcpServerPort_);
            inet_pton(AF_INET, serverIP_.c_str(), &addr.sin_addr);
            
            if (connect(sock, (sockaddr*)&addr, sizeof(addr))<0){
                closesocket(sock);
                return INVALID_SOCKET;
            }
            return sock;
        }

        bool sendLine(SOCKET s, const string& msg){
            string data = msg + "\n";
            return send(s, data.c_str(), (int)data.size(), 0) == data.size();
        }

        bool readLine(SOCKET s, string& out){
            out.clear();
            char c;
            while (true) {
                int n = recv(s, &c, 1, 0);
                if (n <= 0) return false;
                if (c == '\n') break;
                if (c != '\r') out.push_back(c);
            }
            return true;
        }

        void registerUser(){
            cout << "\nEnter new user id: ";
            string id;
            getline(cin, id);

            SOCKET sock = connectTCP();
            if (sock == INVALID_SOCKET){
                cout << "\n Connection Failed";
                return ;
            }

            sendLine(sock, "REG " + id);

            string response;
            if (readLine(sock, response)) {
                cout << "\nServer: " << response;
            }

            if (response.find("200") != string::npos){
                userId_ = id;
                cout << "\nLogged in as: " << userId_;
            }

            closesocket(sock);
        }

        void loginUser() {
            cout << "\nEnter existing user id: ";
            string id;
            getline(cin, id);
            userId_ = id;
            cout << "\nLogged in as: " << userId_;
        }

        void addBuddy(){
            if (userId_.empty()){
                cout << "\nYou must be logged in first!";
                return;
            }

            cout << "\nEnter buddy id: ";
            string buddy;
            getline(cin, buddy);

            SOCKET sock = connectTCP();
            if (sock == INVALID_SOCKET){
                cout << "\nConnection Failed";
                return;
            }

            sendLine(sock, "ADD " + userId_ + " " + buddy);
            string response;
            if (readLine(sock, response)){
                cout << "\nServer: " << response;
            }
            closesocket(sock);
        }

        void deleteBuddy(){
            if (userId_.empty()){
                cout << "\nYou must be logged in first!";
                return;
            }

            std::cout << "Enter buddy id: ";
            std::string buddy;
            std::getline(std::cin, buddy);

            SOCKET sock = connectTCP();
            if (sock == INVALID_SOCKET){
                cout << "\nConnection Failed";
                return;
            }

            sendLine(sock, "DEL " + userId_ + " " + buddy);
            string response;
            if (readLine(sock,response)){
                cout << "\nServer: " << response;
            }
            closesocket(sock);
        }

        // extra method for status
        void showBuddyStatus(){
            lock_guard<mutex> lock(buddyMutex_);
            if (buddyList_.empty()){
                cout << "\nNo Buddies.";
                return;
            }

            cout << "\nBuddy List: ";
            for (auto& b: buddyList_){
                cout << "\n" << b.toString();
            }
        }

        // UDP methods

        void udpPrescenceLoop() {
            SOCKET udpSock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

            sockaddr_in serverAddr{};
            serverAddr.sin_family = AF_INET;
            serverAddr.sin_port = htons(udpServerPort_);
            inet_pton(AF_INET, serverIP_.c_str(), &serverAddr.sin_addr);

            char buffer[4096];

            while (!shutdown_){
                if (!userId_.empty()){
                    // try set
                    ostringstream setMsg;
                    setMsg << "SET " << userId_ << " " << status_.substr(0,3)
                    << " " << status_.substr(4) << " " << tcpMessagePort_;
                    
                    sendto(udpSock, setMsg.str().c_str(), (int)setMsg.str().size(), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
                    // try get
                    string getMsg = "GET " + userId_;
                    sendto(udpSock, getMsg.c_str(), (int)getMsg.size(), 0, (sockaddr*)&serverAddr, sizeof(serverAddr));
                    
                    // response
                    sockaddr_in from{};
                    int fromLen = sizeof(from);
                    int n = recvfrom(udpSock, buffer, 4095, 0, (sockaddr*)&from, &fromLen);

                    if (n>0){
                        buffer[n] = '\0';
                        parseBuddyStatus(buffer);
                    }
                }
                this_thread::sleep_for(chrono::milliseconds(800));
            }
            closesocket(udpSock);
        }

        void parseBuddyStatus(const string& msg){
            vector<BuddyStatusRecord> list;
            istringstream iss(msg);
            string line;

            while (getline(iss, line)){
                istringstream ls(line);
                BuddyStatusRecord rec;

                string statusCode, statusMsg;
                ls >> rec.buddyId >> statusCode >> statusMsg >> rec.ip >> rec.port;

                rec.status = statusCode + " " + statusMsg;

                list.push_back(rec);
            }

            lock_guard<mutex> lock(buddyMutex_);
            buddyList_ = list;
        }


        // display incoming chat(s)
        void tcpWelcomeLoop(){
            SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            addr.sin_addr.s_addr = INADDR_ANY;
            addr.sin_port = htons(tcpMessagePort_);
            
            bind(listenSock, (sockaddr*)&addr, sizeof(addr));
            listen(listenSock, 4);
        
            cout << "\nChat listening on port " << tcpMessagePort_;
            
            while (!shutdown_){
                sockaddr_in clientAddr{};
                int len = sizeof(clientAddr);

                SOCKET sock = accept(listenSock, (sockaddr*)&clientAddr, &len);
                if (sock == INVALID_SOCKET) continue;

                lock_guard<mutex> lock(pendingMutex_);
                if (pendingConnection_ != INVALID_SOCKET){
                    sendLine(sock, "REJECT");
                    closesocket(sock);
                }
                else{
                    pendingConnection_ = sock;
                    cout << "\nIncoming chat request. Accept? (Y/N)";
                }
            }
            closesocket(listenSock);
        }   
    
        // Accept or Reject Requests

        void acceptIncoming(){
            lock_guard<mutex> lock(pendingMutex_);
            if (pendingConnection_ == INVALID_SOCKET){
                cout << "\nNo Pending Connection";
                return;
            }
            
            sendLine(pendingConnection_, "ACCEPT");
            startChat(pendingConnection_);
            pendingConnection_ = INVALID_SOCKET;
        }

        void rejectIncoming(){
            lock_guard<mutex> lock(pendingMutex_);
            if (pendingConnection_ == INVALID_SOCKET){
                cout << "\nNo Pending Connection";
                return;
            }

            sendLine(pendingConnection_, "REJECT");
            closesocket(pendingConnection_);
            pendingConnection_ = INVALID_SOCKET;
        }

        // chat messages
        void messagBuddy(){
            if (userId_.empty()){
                cout << "\nMust Login";
                return;
            }
            
            // if the connection is still pending, we must ask.
            {
                lock_guard<mutex> lock(pendingMutex_);
                if (pendingConnection_ != INVALID_SOCKET){
                    cout << "\nAccept Incmoing Chat First: (Y/N)";
                    return;
                }
            }

            cout << "\nEnter Buddy id: ";
            string buddy;
            getline(cin, buddy);

            BuddyStatusRecord rec;
            {
                lock_guard<mutex> lock(buddyMutex_);
                bool found = false;
                for (auto& b: buddyList_){
                    if (b.buddyId == buddy){
                        rec = b;
                        found = true;
                        break;
                    }
                }
                if (!found){
                    cout <<  "\nBuddy not found";
                    return;
                }
            }

            if (!rec.isOnline()){
                cout << "\nBuddy is offline";
                return;
            }

            SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

            sockaddr_in addr{};
            addr.sin_family = AF_INET;
            inet_pton(AF_INET, rec.ip.c_str(), &addr.sin_addr);
            addr.sin_port = htons(rec.port);

            if (connect(s, (sockaddr*)&addr, sizeof(addr)) < 0){
                cout << "\nUnable to connect to buddy";
                closesocket(s);
                return;
            }

            string line;
            if (!readLine(s, line) || line == "REJECT"){
                cout << "\nBuddy Rejected Chat";
                closesocket(s);
                return;
            }

            if (line == "ACCEPT"){
                startChat(s);
            }else{
                cout << "Unexpected Response: " << line << "\n";
                closesocket(s);
            }
        }

        void startChat(SOCKET s) {
            std::cout << "Chat started. Type 'q' to quit.\n";

            // Launch receive thread
            chatRecieveThread_ = std::thread([this, s]() {
                char c;
                std::string msg;
                while (true) {
                    if (!readLine(s, msg)) break;
                    std::cout << "\nB: " << msg << "\n";
                }
                std::cout << "Chat ended.\n";
            });

            while (true) {
                std::string line;
                std::getline(std::cin, line);
                if (line == "q") break;
                sendLine(s, line);
            }

            closesocket(s);
            if (chatRecieveThread_.joinable()) chatRecieveThread_.join();
        }
    };


    int main(){
        WSADATA wsa;
        WSAStartup(MAKEWORD(2,2), &wsa);
        
        IMClient client("127.0.0.1", 1234, 1235);
        client.run();

        WSACleanup();
        return 0;
    }