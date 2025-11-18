# Instant Messaging System — TCP + UDP (Client/Server)

This project implements a simple instant-messaging system using both **TCP** and **UDP**.  
The server manages user registration, buddy lists, and online/offline presence.  
Clients communicate with the server for account operations and communicate **directly with each other** for real-time chat.

Originally designed as a networking assignment, this project demonstrates practical socket programming:
- TCP request/response flow  
- UDP presence updates  
- Multi-threaded server + client  
- Peer-to-peer TCP messaging  

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

## Submission Notes (Original Context)

For the original academic version of this project:
- Submit your code and two walkthrough logs  
- Name them `output1.txt` and `output2.txt`  
- Due: March 2nd, 2025  

---

If you want, I can also generate:
✅ A super clean version specifically for **your C++ implementation**  
✅ A version tailored for **public GitHub projects** 
