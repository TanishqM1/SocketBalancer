#define _WINSOCK_DEPRECATED_NO_WARNINGS

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h> // REQUIRED for InetNtop, INET_ADDRSTRLEN
#pragma comment(lib, "ws2_32.lib")
#endif

#include <filesystem>
#include <fstream>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <algorithm>

using namespace std;
namespace fs = std::filesystem;

// protocol constant outline
static const string ONLINE_STATUS = "100 ONLINE";
static const string OFFLINE_STATUS = "101 OFFLINE";
static const string AWAY_STATUS = "102 AWAY";

static const string CODE_OK = "200 OK";
static const string CODE_INVALID = "201 INVALID";
static const string CODE_NO_SUCH = "202 NO SUCH USER";
static const string CODE_USER_EXISTS = "203 USER EXISTS";

// User Status Record
struct StatusRecord
{
    string IPaddress; 
    int port = 0;
    string status;
};

// IM Server

class IMServer
{

public:
    IMServer(int tcpPort, int udpPort, const string& dataDir): tcpPort_(tcpPort), udpPort_(udpPort), dataDir_(dataDir), stop_(false){
        fs::create_directories(fs::path(dataDir_) / "users");
    }
    // run server function
    void run()
    {
        thread udpThread(&IMServer::udpLoop, this);
        tcpAcceptLoop();
        udpThread.join();
    }

private:
    // data structure to hold port(s)/connections, and connection lifecycle methods.
    int tcpPort_, udpPort_;
    string dataDir_;
    unordered_map<string, StatusRecord> userStatus_;
    mutex statusMutex_;

    bool stop_;

    // USER CONNECTION MANAGEMENT.
    string userFilePath(const string &userId)
    {
        return (fs::path(dataDir_) / "users" / (userId + ".txt")).string();
    }

    bool existingUser(const string &userId)
    {
        return fs::exists(userFilePath(userId));
    }

    bool registerUser(const string &userId)
    {
        if (existingUser(userId))
        {
            return false;
        }
        ofstream ofs(userFilePath(userId));
        return (bool)ofs;
    }

    vector<string> readBuddyList(const string &userId)
    {
        vector<string> results;
        ifstream ifs(userFilePath(userId));
        string line;
        while (getline(ifs, line))
        {
            if (!line.empty())
            {
                results.push_back(line);
            }
        }
        return results;
    }

    bool updateBuddyList(bool isAdd, const string &userId, const string &buddyId)
    {
        auto path = userFilePath(userId);
        auto buddies = readBuddyList(userId);
        bool changed = false;

        if (isAdd)
        {
            if (find(buddies.begin(), buddies.end(), buddyId) == buddies.end())
            {
                buddies.push_back(buddyId);
                changed = true;
            }
        }
        else
        {
            vector<string> filtered;
            for (auto &b : buddies)
            {
                if (b != buddyId)
                {
                    filtered.push_back(b);
                }
                else
                {
                    changed = true;
                }
            }
            buddies = filtered;
        }

        if (!changed)
            return true;

        ofstream ofs(path, ios::trunc);
        for (auto &b : buddies)
            ofs << b << "\n";
        return true;
    }

    void updateUserStatus(const string &userId, const StatusRecord &rec)
    {
        lock_guard<mutex> lock(statusMutex_);
        userStatus_[userId] = rec;
    }

    // auto lets the compiler deduce the type of a variable.
    // do not use it when types are NEEDED for readability / context, but rather for verbose types to be shortened.
    bool getUserStatus(const string &userId, StatusRecord &out)
    {
        lock_guard<mutex> lock(statusMutex_);
        auto it = userStatus_.find(userId);
        if (it == userStatus_.end())
            return false;
        out = it->second;
        return true;
    }

    // TCP methods

    void tcpAcceptLoop()
    {
        SOCKET serverFd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // create TCP socket

        // map to local address
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(tcpPort_);

        // bind & listen w/ socket
        bind(serverFd, (sockaddr *)&addr, sizeof(addr));
        listen(serverFd, 16);

        cout << "\nTCP listening on " << tcpPort_;

        // tcp connect loop while server is up.

        while (!stop_)
        {
            // to accept incoming clients
            sockaddr_in clientAddr{};
            int len = sizeof(clientAddr);
            SOCKET ClientFd = accept(serverFd, (sockaddr *)&clientAddr, &len);

            if (ClientFd == INVALID_SOCKET)
            {
                continue;
            }

            std::thread(&IMServer::handleTcpClient, this, ClientFd).detach();
        }
        closesocket(serverFd);
    }

    // read/write console writing from TCP socket.
    static bool readLine(SOCKET fd, string &out)
    {
        out.clear();
        char c;
        while (true)
        {
            int n = recv(fd, &c, 1, 0);
            if (n <= 0)
                return false;
            if (c == '\n')
                break;
            if (c != '\r')
                out.push_back(c);
        }
        return true;
    }

    static bool sendLine(SOCKET fd, const string &line)
    {
        string buf = line + "\n";
        int sent = send(fd, buf.c_str(), (int)buf.size(), 0);
        return sent == buf.size();
    }

    void handleTcpClient(SOCKET fd)
    {
        string req;
        if (!readLine(fd, req))
        {
            closesocket(fd);
            return;
        }

        istringstream iss(req);
        string cmd, userId, buddyId;
        iss >> cmd >> userId >> buddyId;

        if (cmd == "REG")
        {
            if (userId.empty())
                sendLine(fd, CODE_INVALID);
            else if (existingUser(userId))
                sendLine(fd, CODE_USER_EXISTS);
            else if (registerUser(userId))
                sendLine(fd, CODE_OK);
            else
                sendLine(fd, CODE_INVALID);
        }
        else if (cmd == "ADD" || cmd == "DEL")
        {
            bool isAdd = (cmd == "ADD");
            if (userId.empty() || buddyId.empty())
                sendLine(fd, CODE_INVALID);
            else if (!existingUser(userId) || !existingUser(buddyId))
                sendLine(fd, CODE_NO_SUCH);
            else if (userId == buddyId)
                sendLine(fd, CODE_INVALID);
            else if (updateBuddyList(isAdd, userId, buddyId))
                sendLine(fd, CODE_OK);
            else
                sendLine(fd, CODE_INVALID);
        }
        else
        {
            sendLine(fd, CODE_INVALID);
        }

        closesocket(fd);
    }

    // UDP methods

    void udpLoop()
    {
        SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP); // create UDP socket

        // map to local address
        sockaddr_in addr{};
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(udpPort_);

        bind(sock, (sockaddr *)&addr, sizeof(addr));

        char buf[2048];

        while (!stop_)
        {
            sockaddr_in clientAddr{};
            int len = sizeof(clientAddr);
            int n = recvfrom(sock, buf, 2047, 0, (sockaddr *)&clientAddr, &len);
            if (n <= 0)
                continue;

            buf[n] = '\0';
            string request(buf);

            istringstream iss(request);
            string cmd;
            iss >> cmd;

            if (cmd == "SET")
            {
                string userId, statusCode, statusMsg;
                int msgPort;
                iss >> userId >> statusCode >> statusMsg >> msgPort;

                if (!userId.empty() && msgPort > 0)
                {
                    string status = statusCode + " " + statusMsg;
                    if (status == ONLINE_STATUS || status == OFFLINE_STATUS || status == AWAY_STATUS)
                    {
                        char ipbuf[INET_ADDRSTRLEN];
                        InetNtopA(AF_INET, &clientAddr.sin_addr, ipbuf, INET_ADDRSTRLEN);

                        StatusRecord rec;
                        rec.IPaddress = ipbuf;
                        rec.port = msgPort;
                        rec.status = status;

                        updateUserStatus(userId, rec);
                    }
                }
            }
            else if (cmd == "GET")
            {
                string userId;
                iss >> userId;

                if (!existingUser(userId))
                    continue;

                auto buddies = readBuddyList(userId);

                ostringstream reply;
                for (size_t i = 0; i < buddies.size(); i++)
                {
                    StatusRecord rec;
                    if (getUserStatus(buddies[i], rec))
                    {
                        reply << buddies[i] << " "
                              << rec.status << " "
                              << rec.IPaddress << " "
                              << rec.port;
                    }
                    else
                    {
                        reply << buddies[i] << " "
                              << OFFLINE_STATUS << " unknown unknown";
                    }
                    if (i + 1 < buddies.size())
                        reply << "\n";
                }
                string out = reply.str();
                sendto(sock, out.c_str(), (int)out.size(), 0, (sockaddr *)&clientAddr, len);
            }
        }

        closesocket(sock);
    }
};

int main(int argc, char* argv[])
{
    WSADATA wsa;
    WSAStartup(MAKEWORD(2, 2), &wsa);

    // Default values
    int tcpPort = 5001;
    int udpPort = 1235;
    string dataDir = "data";

    // Parse command line: im_server.exe <tcp_port> <udp_port> <data_dir>
    if (argc >= 2) tcpPort = atoi(argv[1]);
    if (argc >= 3) udpPort = atoi(argv[2]);
    if (argc >= 4) dataDir = argv[3];

    cout << "[Server] Starting with TCP=" << tcpPort 
         << ", UDP=" << udpPort 
         << ", DataDir=" << dataDir << "\n";

    IMServer server(tcpPort, udpPort, dataDir);
    server.run();

    WSACleanup();
    return 0;
}
