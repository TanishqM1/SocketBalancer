[![Review Assignment Due Date](https://classroom.github.com/assets/deadline-readme-button-22041afd0340ce965d47ae6ef1cefeee28c7c493a6346c4f15d667ab976d596c.svg)](https://classroom.github.com/a/cYO2PKDy)
[![Open in Visual Studio Code](https://classroom.github.com/assets/open-in-vscode-2e0aaae1b6195c2367325f4f02e2d04e9abb55f0b24a779b69b11b9e10269abc.svg)](https://classroom.github.com/online_ide?assignment_repo_id=18407654&assignment_repo_type=AssignmentRepo)
# Assignment 5: TCP and UDP Programming with Java

In this assignment, we will perform TCP and UDP programming with Java. Although the assignment sample code is in Java, you are welcome to implement your code in Python) but recommend using Java.  You will need to specify this when submitting your assignment.  The code has been tested with Java 18 but was written and tested as far back as Java 8.

**Expect to spend approximately 6 hours on this assignment.**

## Programming an Instant Message (IM) Client (40 marks)
We will implement an instant messaging (IM) client that communicates with an IM server created by the instructor. In the process, we will practice sending and receiving messages with both TCP and UDP. It is important that you follow the communication protocol defined below in order for correct operation of your client.

### Operations

Our instant messager program can register a user, add and delete buddies for a user, indicate when we are online/offline, and check when our buddies are online. If a buddy is online, we can directly exchange messages back and forth with them until we close the connection. All operations are specified in ASCII text with a 3-level operation code. Some operation messages are sent using TCP and others using UDP. Some messages have a response while others go unacknowledged.

| **Operation	Syntax**	| **Protocol**	| **Response**|
| --- | --- | --- |
|register user	| REG [userid]	| TCP | Yes - 200, 201, 202, 203 |
| add buddy	 | ADD [userid] [buddyid]	| TCP | Yes - 200, 201, 202 |
| delete buddy | 	DEL [userid] [buddyid] | TCP | Yes - 200, 201, 202 |
| update status |	SET [userid] [status] [msgport] | UDP | No |
| get buddy status | GET [userid]	| UDP	| Yes |

Update status indicates that the user is online/offline (codes follow). **[msgport]** is the TCP port that the client is accepting connections on from other users.

| Status Code and Description | Description |
| --- | --- |
| 100 ONLINE | User is online able to accept buddy connections. |
| 101 OFFLINE |	User is offline and not able to accept connections. |

A UDP get buddy status request is responded to with a UDP message in return. This message has the format:
```
[buddyId] [buddyStatus] [buddyIP] [buddyPort]
[buddyId] [buddyStatus] [buddyIP] [buddyPort]
...
[buddyId] [buddyStatus] [buddyIP] [buddyPort]
```
### Messages and Ports
Messages sent over TCP to the server are communicated on port 1234 and are acknowledged with a response message. Each message sent over TCP is done on a new connection (no persistent-connections). Messages sent over UDP are communicated on port 1235.  Both UDP and TCP is used when communicating between the client and the server.   

Communciations between clients is done directly between the clients using TCP.  This connection will use the port `TCPMessagePort`.  The server will faciliate the sharing of port numbers, but will not be involved in client-to-client communications.  WHen you are developing locally with two clients, you will need to make sure to use different values for `TCPMessagePort`.  Alternately, if you look at the JavaDoc for [`ServerSocket`](https://docs.oracle.com/javase/8/docs/api/java/net/ServerSocket.html#ServerSocket-int-) you can configure it to use select an available ephemeral port.  

### Response Messages
Response messages are in ASCII text with a response code and a response message (similar to HTTP).

| Response Code and Description	| Description |
| --- | --- |
| 200 OK	| Previous request OK and confirmed. |
| 201 INVALID	| Request syntax was invalid/incorrect. |
| 202 NO SUCH USER |	No such user id exists in the database. |
| 203 USER EXISTS | User with that id currently exists. |

### Server Operation
The server code and protocol is designed so that you can run the server and any number of clients on the same machine using TCP port 1234 and UDP port 1235.  I recommend you do your testing on your own local machine.  The server code is in the Server folder in the starter code.  

The server has two main threads. The first thread accepts TCP connections processing user registrations and buddy list updates (add/delete). When a request is encountered, a new thread is started to answer the request. A request may require a query or an update of the user "database" (which is just simple text files for portability). A second thread monitors UDP traffic and does not require a separate thread per request. Requests for the UDP thread are either to set a user status or get status of buddies. The server responds to a get status request with a special buddy status message as described above.

### Client Operation
The client will be text-based. When a client program is started, the client has access to a menu of options. The Client [skeleton code with an initial menu is available](src/IMClient/IMClient.java)

Menu commands require only a single letter. The menu options parallel the server request messages accept there are three additional options. The login option sets the client to use an existing user id without registering it. Note no validation must be performed on either the client or server-side to determine if this is a valid id. Another option asks for the buddy status to be displayed. The other initiates a connection to a buddy to communicate with them. Once in message mode, all text typed is directed to the buddy. Typing "q" will quit messaging mode and close the connection. Note that we will only accept one connection at a time from a buddy.

There are different ways to implement the client program. One way is to use five threads:

1. **Main thread** - accepts user input from menu and calls required function
2. **UDP send thread** - periodically sends (every 10 seconds) current user status and GET request for buddy status
3. **UDP receive thread** - receives response from IMServer from GET request
4. **Note:** You can have a single UDP thread that does both send and receive. However, the method to receive a packet on a UDP socket (e.g. `clientSocket.receive(receivePacket);`) is blocking. You must set a timeout value otherwise if a UDP packet did get lost, the method would block and you would no longer send out status updates. You can use either approach. A timeout is probably easier.
5. **TCP welcome thread** - listens to welcome port provided for communication from other users
6. **TCP communication thread** - receives messages from buddy currently communicating with. (Sending is performed using main thread.)
There are potentially other ways for configuring the client. Handling all the threads, specifically their conflict over standard in and out, is probably harder than the UDP/TCP communication issues. The sample code provided has the menu (main thread) setup, stub procedures for all menu options, and defines the class used for the TCP welcome thread. You can use whatever of this code is useful to you.

**Note: If you are using two clients on the same machine, you must change the TCP message port to be different for them.**

## Overview List of Client Actions
* On registration, client should change its status to online.
* On login, client should change its status to online.

## Getting Started Suggestions

* Clone the client code skeleton and verify you can compile it on your machine.
* Work in an incremental fashion, starting with the communications between your server and client.  You will need to run the server and then separately run the client that will them communicate with the server.  The server will report to the console the operations it is doing.  **Do not modify the server code.**
* The easiest thing to get working is the TCP requests to the server for register, add, and delete. Do those first. Use the sample code provided for [simple TCP client](src/tcp_client_server/TCPClient.java) and [simple TCP server](src/tcp_client_server/TCPServer.java).  This will allow for a single string (upto carriage return) and then will close.  [simple TCP client v2](src/tcp_client_server/TCPClient2.java) and [simple TCP server v2](src/tcp_client_server/TCPServer2.java) will allow multiple lines to be entered and then close the connection when a special character sequence is entered.  This version will still only allow for a single connection to the server.  To allow for multiple connections to a single server, you must utilize threading, where each connection will be handed off to a new thread.  The [threaded TCP server](src/tcp_client_server/TCPServerThread.java) can be used with the version 2 tcp client to allow multiple inbound concurrent connections.  Take the time to explore how this code works before proceeding. 
* Next create the UDP send thread that sends a status update and a get request every 10 seconds. See [UDP client](src/udp_client_server/UDPClient.java) and [UDP Server](src/udp_client_server/UDPServer.java) for sample UDP code.  A [client](src/udp_client_server/UDPClient2.java) and [server](src/udp_client_server/UDPServer2.java) that supports multiple lines of input is also available for review. 
* Create the UDP receive thread to get responses from server for buddy status. UDP server sample code will help.
* Creating the direct connection to transfer TCP messages back and forth is the most cumbersome mostly due to issues of standard in/out. I recommend doing that last.  

# Walkthrough

A walkthrough demonstrating client features is available. You must perform and capture this walkthrough and submit with your code.. You will need two client windows. Capture the output from both of them: [client #1 output](exampleOutput1.txt) and [client #2 output](exampleOutput2.txt) and commit them back to repository as `output1.txt` and `output2.txt`.

* Open up client window #1. In that window perform these operations:
  1. Register user with your first name + "1". E.g. "scott1".
  2. Try to add buddy with your first name + "2". E.g. "scott2". Error message should be generated and displayed.
  3. Wait until perform steps #1-#3 in second client window.
  4. Try to add buddy with your first name + "2". E.g. "scott2". Success message.
  5. Wait for a period of time. Check buddy status which should indicate that "scott2" is online.
  6. Initiate a connection to buddy "scott2".
  7. Type in message: "Hello buddy!". Go to step #5 for client window #2.
  8. Try to delete buddy with name "scott2". Success message.
  9. Wait for a period of time. Check status to see if "scott2" is no longer in buddy list.
  10. Quit client. Done.

* Open up client window #2. In that window perform these operations:
  1. Register user with your first name + "2". E.g. "scott2".
  2. Try to add buddy with your first name + "1". E.g. "scott1". Success message.
  3. Wait for a period of time. Check status of buddies to see that "scott1" is online.
  4. Perform step #4 in client window #1.
  5. Type in message: "Nice to meet you buddy!".
  6. Close connection to buddy (by entering 'q').
  7. Try to delete buddy with name "testXY". Error message.
  8. Try to add buddy with your first name + "1". E.g. "scott1". Error message.
  9. Quit client. Go to client window #1 step #8.

**Note that your code may be tested in other situations, but this exact walkthrough must be captured electronically and submitted with your assignment (using the users you created).**

# Submission Instructions and Marking

## Submission Instructions
All of your code and two walkthrough capture sessions should be submitted with GitHub Classroom.  Your submission is due before the end of day on Sunday, March 2nd, 2025.  You will have additional time to complete this assignment but there will be a new assignment in the following weeks  **Do not wait to start on this assignment as it will take some time to complete.   The due date for this assignment will not be extended.**. 

## Marking (40 marks total)
1. **+10** TCP commands: register user, add/delete buddy
2. **+10** UDP sender: send own status and request buddy list
3. **+10** UDP receive: get buddy list updates and show to user
4. **+10** TCP messaging: initiate, communicate, and terminate with a buddy
