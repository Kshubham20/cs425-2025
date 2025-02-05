# README: Chat Server with Groups and Private Messaging

## Group members
- **Shubham Kumar** 200967
- **Ashutosh Dwivedi** 200214
- **Chinmay Hiran Pillai** 200298

## Overview
This project implements a **multi-threaded chat server** that supports:
- **User authentication** using `users.txt`
- **Private messages** between users
- **Broadcast messages** to all users
- **Group messaging** with support for group creation, joining, leaving, and sending messages
- **Multiple concurrent clients** using threads

The chat server is built using **C++** with **POSIX sockets and multithreading**.

---

## Installation & Compilation
### Prerequisites
Ensure you have:
- **g++ compiler** (supports C++20)
- **Make** installed

### Compilation
To compile both the client and server, run:
```bash
make
```
This will generate two executables:
- `server_grp` (server application)
- `client_grp` (client application)

To clean previous builds, run:
```bash
make clean
make
```

---

## Running the Chat Server
Start the server first:
```bash
./server_grp
```
Expected output:
```
Server started on port 12345
```

---

## Running the Client
Each client should be launched in a separate terminal:
```bash
./client_grp
```

Upon connecting, users will be prompted to enter their username and password. These must match the entries in `users.txt`. Example:
```
Connected to the server.
Enter username: alice
Enter password: password123
Welcome to the chat server!
```

If authentication fails:
```
Authentication failed.
```

---

## Commands Available
Once connected, clients can use the following commands:

### **1. Private Messaging**
Send a private message to a user:
```bash
/msg <username> <message>
```
Example:
```
/msg bob Hi Bob!
```

### **2. Broadcast Messaging**
Send a message to all users:
```bash
/broadcast <message>
```
Example:
```
/broadcast Hello everyone!
```

### **3. Group Management**
#### **Create a Group**
```bash
/create group <group_name>
```
Example:
```
/create group CS425
```
Output:
```
Group CS425 created.
```

#### **Join a Group**
```bash
/join group <group_name>
```
Example:
```
/join group CS425
```
Output:
```
You joined the group CS425.
```

#### **Leave a Group**
```bash
/leave group <group_name>
```
Example:
```
/leave group CS425
```
Output:
```
You left the group CS425.
```

#### **Send a Group Message**
```bash
/group msg <group_name> <message>
```
Example:
```
/group msg CS425 Hi, team!
```
Output (for all group members):
```
[Group CS425] alice: Hi, team!
```

### **4. Exit Chat**
```bash
/exit
```

---

## Code Explanation
### **1. Server (`server_grp.cpp`)**
- Loads `users.txt` for authentication.
- Uses **unordered maps** for:
  - `users`: storing username-password pairs.
  - `clients`: mapping connected sockets to usernames.
  - `groups`: mapping group names to sets of client sockets.
- Each client connection is handled in a separate **thread**.
- Uses **mutex locks** to manage concurrent access to shared resources.

### **2. Client (`client_grp.cpp`)**
- Connects to the server using a **TCP socket**.
- Handles authentication and allows users to send/receive messages.
- Runs a **separate thread** to listen for incoming messages from the server.

---

## Troubleshooting
### **Port Already in Use**
If you see an error like:
```
Error: Address already in use
```
Find and kill the process using port `12345`:
```bash
netstat -tulnp | grep 12345
kill -9 <PID>
```

### **Client Cannot Connect**
Ensure the server is running before launching the client.

### **Authentication Issues**
Check `users.txt` and ensure the correct username and password are entered.

---
## **Code general information**
### ** Design Specifications**
- Utilizes threading for connection of each user.
- Multiple threads insure simulataneous connection as required by a chat server.
- we have three major functions:
- 1. handle_client : which handles the requests of the client according to the requirements set by the user
  2. load_users: checks and verifies the users and password
  3. send_message: response for communication of actual messages from one user point to another.

### **Testing**
- Code worked correctly with operating normally upto 16 threads. After that our computer faced issues with terminal opernings.
- There is no limitation within the server for number of users, number of groups, size of message. However, this factors are greatly influenced by the computation power of the device on which the server is hosted.

### **Challenges**
- The major challenge in the assignment was to come up with the connection design for the socket connection.
- Lack of knowledge of server development by use of CPP also possessed a set back for us.
- Testing was one of the hardest things to do after the completion as we did not had a common cluster of devices hence everything was done on the same machine.

### **Contribution**
- All three of the members had equal contribution in the project.
- Chinmay Pillai: Design of the server functionalities and common architecture, creating of the tests and documentation of the code.
- Shubham Kumar, Ashutosh Dwivedi: Coding the server. Implementation of various functions and optimizing the connection time. Creation of repository and responsible for preparation of README file.

## Declaration
We team members declare that we did not indulge in any malpractices during the course of this assignment. All the code in this assignment has been written manually and was not copied from any website, other group member or Gen AI .
- Team members.

