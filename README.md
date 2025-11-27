# Instant Messaging System — TCP + UDP (Client/Server)

This project implements a simple instant-messaging system using both **TCP** and **UDP**.  
The server manages user registration, buddy lists, and online/offline presence.  
Clients communicate with the server for account operations and communicate **directly with each other** for real-time chat.

this project demonstrates practical socket programming:
- TCP request/response flow  
- UDP presence updates  
- Multi-threaded server + client  
- Peer-to-peer TCP messaging  
- **Load balancing** for TCP traffic distribution

---

## Features

### Client–Server Operations

| Operation | Syntax | Protocol | Response |
|----------|--------|----------|----------|
| Register user | `REG [userid]` | TCP | 200 / 201 / 202 / 203 |
| Add buddy | `ADD [userid] [buddyid]` | TCP | 200 / 201 / 202 |
| Delete buddy | `DEL [userid] [buddyid]` | TCP | 200 / 201 / 202 |
| Update status | `SET [userid] [status] [msgport]` | UDP | No response |
| Get buddy status | `GET [userid]` | UDP | Buddy list |

**Status Codes**
- `100 ONLINE` — user is online and accepting chat  
- `101 OFFLINE` — user is offline  

---

## Message Flow

### TCP (Client → Server)
Used for:
- Registration  
- Adding buddies  
- Deleting buddies  

Every TCP request opens a **new connection** and receives a single-line response (e.g., `200 OK`).

---

### UDP (Client ↔ Server)

Clients periodically send:
- `SET` — update their presence (`ONLINE`/`OFFLINE`) and message port  
- `GET` — request updated buddy statuses  

The server responds with lines such as:
```
alice 100 ONLINE 192.168.1.5 20001
bob 101 OFFLINE unknown 0
```

---

### TCP (Client ↔ Client)

Clients chat **peer-to-peer** over TCP.  
The server only helps clients discover **IP + port**, but does not forward messages.

Chat continues until the user types `q` to quit.

---

## Client Architecture

A typical client implementation uses multiple threads:

1. **Main Thread** — menu and user input  
2. **UDP Sender** — periodically broadcasts presence + GET requests  
3. **UDP Receiver** — receives buddy statuses  
4. **TCP Welcome Thread** — listens for incoming chat requests  
5. **TCP Chat Thread** — receives messages during chat  

Only one incoming or outgoing chat is allowed at a time.

> **Tip:** If running multiple clients on one machine, use different TCP message ports.

---

## Basic User Workflow

- Register or log in  
- Automatically set status to **online**  
- Add or remove buddies  
- Periodically receive buddy status updates  
- Initiate a chat with any online buddy  
- Chat in real time over TCP  
- Quit with `q`  

---

## Walkthrough Example

To verify correct program behavior, run **two clients** and perform the following steps:

### Client 1
1. Register: `yourname1`  
2. Attempt to add `yourname2` (should fail initially)  
3. After Client 2 registers, add `yourname2` (success)  
4. Wait for status update and verify buddy is online  
5. Initiate chat with `yourname2`  
6. Send: `Hello buddy!`  
7. Delete buddy  
8. Confirm buddy is removed  
9. Quit  

### Client 2
1. Register: `yourname2`  
2. Add `yourname1` (success)  
3. Wait for status update and verify online  
4. Respond to chat: `Nice to meet you buddy!`  
5. Quit chat (`q`)  
6. Attempt invalid delete  
7. Attempt duplicate add  
8. Quit  

---

## Load Balancer

### Overview

The load balancer distributes **TCP traffic** across multiple backend IM servers using a **round-robin** algorithm. This demonstrates how to scale the system horizontally and handle increased load.

### Architecture

```
[Client 1] ──┐
[Client 2] ──┼──> [Load Balancer :1234] ──┬──> [IM Server 1 :5001] ──> [Database]
[Client 3] ──┘                             └──> [IM Server 2 :5002] ──────^
              
              UDP Presence Traffic (bypasses load balancer)
              └──────────────────────────────────> [IM Server :1235]
```

### Key Points

- **TCP Operations** (REG, ADD, DEL) go through the load balancer
- **UDP Presence** updates bypass the load balancer (sent directly to server)
- **P2P Chat** happens directly between clients (no load balancer involved)
- Round-robin distribution ensures even load across backend servers

### Running with CMAKE

1. Required: Windows + MSYS2 (ucrt64) + the following packages:
```
pacman -Syu
pacman -S mingw-w64-ucrt-x86_64-gcc
pacman -S mingw-w64-ucrt-x86_64-cmake
pacman -S mingw-w64-ucrt-x86_64-ninja
```
2. From project root:
```
cd src
mkdir build
cd build
cmake -G "Ninja" ..
cmake --build .
```

3. Run system:

IM Servers:
`
./im_server.exe 5001 1235 ../data
`
 and 
`
./im_server.exe 5002 1236 ../data
`

Load Balancer: `./load_balancer.exe`

Launch Client(s): `./im_client.exe`

### Running with Raw Compile

**Compile:**
```
cd {YourPath}NetworksProject/rewrite/src
g++ -std=gnu++17 -O2 im_server.cpp -lws2_32 -o im_server.exe
g++ -std=gnu++17 -O2 im_client.cpp -lws2_32 -o im_client.exe
g++ -std=gnu++17 -O2 load_balancer.cpp -lws2_32 -o load_balancer.exe
```


**Terminal 1 — Start Server 1:**
```bash
./im_server.exe 5001 1235 ../data
```

**Terminal 2 — Start Server 2 (optional):**
```bash
./im_server.exe 5002 1236 ../data
```

**Terminal 3 — Start Load Balancer:**
```bash
./load_balancer.exe
```
Output:
```
[LB] Load Balancer running on port 1234...
[LB] Forwarding to 2 backend(s)
[LB]   Backend 0: 127.0.0.1:5001
[LB]   Backend 1: 127.0.0.1:5002
```

**Terminal 4+ — Start Client(s):**
```bash
./im_client.exe
```

Clients connect to port 1234 (load balancer), which transparently forwards to available backends.

### Configuration

Edit `load_balancer.cpp` to add/remove backend servers:

```cpp
vector<Backend> backends = {
    {"127.0.0.1", 5001},  // Server 1
    {"127.0.0.1", 5002},  // Server 2
    {"127.0.0.1", 5003}   // Server 3 (add more as needed)
};
```

### Limitations & Considerations

**Data Synchronization:**
- Each server maintains its own user database and buddy lists
- Users registered on Server 1 won't automatically exist on Server 2
- Status updates are only visible to the server that received them

**Solutions:**
1. **Shared Data Directory** — All servers read/write to the same folder
   - Simple but risks file corruption
   - Good for testing/demonstration

2. **Sticky Sessions** — Route users to the same server based on userId hash
   - Ensures consistency per user
   - Better load distribution

3. **Shared Database** — Use PostgreSQL/MySQL with all servers
   - Production-ready solution
   - Requires significant refactoring

**UDP Presence:**
- Currently, clients must send UDP to a **single server port**
- For true multi-server UDP, implement UDP forwarding or server-to-server synchronization

### Load Balancer Features

- **Round-Robin Distribution** — Evenly distributes incoming TCP connections
- **Transparent Proxying** — Clients don't know which backend they're talking to
- **Bidirectional Forwarding** — Full duplex communication between client and server
- **Multi-threaded** — Each client connection handled in a separate thread
- **Socket Reuse** — `SO_REUSEADDR` allows quick restart after crashes

### Testing Load Distribution

Run the load balancer with 2+ backend servers and observe the console output:

```
[LB] New client -> backend #0
[LB] Client connected to backend: 127.0.0.1:5001
[LB] Connection closed

[LB] New client -> backend #1
[LB] Client connected to backend: 127.0.0.1:5002
[LB] Connection closed

[LB] New client -> backend #0
[LB] Client connected to backend: 127.0.0.1:5001
[LB] Connection closed
```

Each connection alternates between backends, demonstrating the round-robin algorithm in action.

---
