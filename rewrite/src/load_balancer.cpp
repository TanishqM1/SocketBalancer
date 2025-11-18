#define _WINSOCK_DEPRECATED_NO_WARNINGS

#ifdef _WIN32
#include <winsock2.h>
#include <ws2tcpip.h>
#pragma comment(lib, "ws2_32.lib")
#endif

#include <iostream>
#include <vector>
#include <thread>
#include <atomic>
#include <string>

using namespace std;


// load balancer only serves as a middleman, so it only needs it's own ip + port (to serve as a
// seperate entity, to re-route data)
struct Backend{
    string ip;
    int port;
};

atomic<int> rrCounter{0};

void forwardLoop(SOCKET src, SOCKET dst){
    char buffer[4096];
    while (true){
        int n = recv(src, buffer, sizeof(buffer), 0);
        if (n <= 0) break;
        int sent = send(dst, buffer, n, 0);
        if (sent <= 0) break;
    }
    shutdown(dst, SD_SEND);
    shutdown(src, SD_RECEIVE);
}

// connect client to availible backend

void handleClient(SOCKET clientSock, const Backend& backend){
    SOCKET backendSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    sockaddr_in backendAddr{};
    backendAddr.sin_family = AF_INET;
    backendAddr.sin_port = htons(backend.port);
    inet_pton(AF_INET, backend.ip.c_str(), &backendAddr.sin_addr);

    // error
    if (connect(backendSock, (sockaddr*)&backendAddr, sizeof(backendAddr)) == SOCKET_ERROR) {
        cout << "\nLoad Balancer cannot reach backend! "
            << backend.ip << ":" << backend.port
            << " WSAError=" << WSAGetLastError();
        closesocket(clientSock);
        closesocket(backendSock);
        return;
}


    cout << "\n Client Connected to Load Balancer. " << backend.ip << ":" << backend.port;
    
    // back and forth data flow
    thread t1(forwardLoop, clientSock, backendSock);
    thread t2(forwardLoop, backendSock, clientSock);

    t1.join();
    t2.join();

    closesocket(clientSock);
    closesocket(backendSock);

    cout << "[LB] Connection closed\n";
}


// run load balancer

int main(){
    WSADATA wsa;
    WSAStartup(MAKEWORD(2,2), &wsa);

    vector<Backend> backends = {
        {"127.0.0.1", 5001},
        {"127.0.0.1", 5002}
    };

    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    int opt = 1;
    setsockopt(listenSock, SOL_SOCKET, SO_REUSEADDR, (char*)&opt, sizeof(opt));

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(1234);

    

    if (bind(listenSock, (sockaddr*)&addr, sizeof(addr)) < 0) {
    cout << "LB bind failed!\n";
        }

    listen(listenSock, SOMAXCONN);

    cout << "[LB] Load Balancer running on port 1234...\n";

    while (true){
        sockaddr_in ClientAddr{};
        int len = sizeof(ClientAddr);

        SOCKET clientSock = accept(listenSock, (sockaddr*)&ClientAddr, &len);
        if (clientSock == INVALID_SOCKET) continue;

        // backend selector (set to red-robin)
        int idx = rrCounter++ % backends.size();
        Backend backend = backends[idx];

        cout << "\nLoad Balancer: New Client routes to backend # " << idx << " " 
        << backend.ip << ":" << backend.port << "\n";
        
        thread(&handleClient, clientSock, backend).detach();
    }

    closesocket(listenSock);
    WSACleanup();
    return 0;
}