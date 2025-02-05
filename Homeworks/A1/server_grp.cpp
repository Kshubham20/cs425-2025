#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 12345
#define BUFFER_SIZE 1024

//Map to store all usernames and passwords in memory
std::unordered_map<std::string, std::string> users;
//Map to store all clients (sockets) and their usernames in memory
std::unordered_map<int, std::string> clients;
//Map to store all groups and their members (sockets) in memory
std::unordered_map<std::string, std::unordered_set<int>> groups;
std::mutex clients_mutex;


/**
 * @brief Loads the user data from users.txt into memory via the users unordered_map.
 *
 * The function reads the content of the "users.txt" file and populates the users unordered_map.
 * Each line in the file represents a user with a username and password separated by a colon.
 * The function splits each line and stores the username as the key and the password as the value in the users unordered_map.
 *
 * @return void
 */
void load_users() {
    // Open users.txt in read mode
    std::ifstream file("users.txt");
    std::string line;

    // Read each line from the file
    while (std::getline(file, line)) {

        // Find the position of the colon delimiter
        size_t delimiter = line.find(":");

        // If the delimiter is found, split the line into username and password
        if (delimiter != std::string::npos) {
            // The username is the substring from the beginning of the line to the delimiter. Obtain this substring
            std::string username = line.substr(0, delimiter);
            // The password is the substring from the delimiter to the end of the line. Obtain this substring
            std::string password = line.substr(delimiter + 1);

            // Add the username and password to the users unordered_map
            users[username] = password;
        }
    }
}


/**
 * @brief Sends a message to the specified client socket.
 *
 * This function uses the send() system call to send a message to the client identified by the given client_socket.
 * The message is sent as a null-terminated C-style string.
 *
 * @param client_socket The socket descriptor of the client to which the message will be sent.
 * @param message The message to be sent to the client.
 *
 * @return void
 */
void send_message(int client_socket, const std::string &message) {

    // Send the message to the client using the send() system call
    send(client_socket, message.c_str(), message.size(), 0);
}


/**
 * @brief Handles the communication with a single client.
 *
 * This function is responsible for authenticating the client, handling client commands, and broadcasting messages to other clients.
 * It reads messages from the client, processes them, and sends appropriate responses.
 *
 * @param client_socket The socket descriptor of the client to be handled.
 *
 * @return void
 */
void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    std::string username;

    // Prompt the user to enter the username
    send_message(client_socket, "Enter username: ");
    // Clear the buffer
    memset(buffer, 0, BUFFER_SIZE);
    // Receive the username from the client into the buffer and store it in the username variable
    recv(client_socket, buffer, BUFFER_SIZE, 0);
    username = buffer;

    // Prompt the user to enter the password
    send_message(client_socket, "Enter password: ");
    // Clear the buffer
    memset(buffer, 0, BUFFER_SIZE);
    // Receive the password from the client into the buffer and store it in the password variable
    recv(client_socket, buffer, BUFFER_SIZE, 0);
    std::string password = buffer;

    // Check if the username exists in the users unordered_map and the password matches the stored password
    // Else, fail the authentication and close the client socket
    if (users.find(username) == users.end() || users[username] != password) {
        send_message(client_socket, "Authentication failed.\n");
        close(client_socket);
        return;
    }

    {
        // Lock the clients_mutex
        std::lock_guard<std::mutex> lock(clients_mutex);
        // Add the cuurent client to the clients unordered_map
        clients[client_socket] = username;
    }

    // Send a welcome message
    send_message(client_socket, "Welcome to the chat server!\n");

    // Main loop to receive and process messages from the client
    while (true) {
        // Clear buffer
        memset(buffer, 0, BUFFER_SIZE);
        // Receive message from the client
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);

        // If the number of bytes received is less than or equal to 0, close the client and remove it from clients map
        if (bytes_received <= 0) {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.erase(client_socket);
            close(client_socket);
            return;
        }

        // Get message from buffer
        std::string message = buffer;

        // If exit command is received, close the client
        if (message == "/exit") {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.erase(client_socket);
            close(client_socket);
            return;
        }

        // Create an input string stream from the message and get the command
        std::istringstream iss(message);
        std::string command;
        iss >> command;

        // If broadcast command is received, broadcast the message to all clients
        if (command == "/broadcast") {
            std::string msg;
            std::getline(iss, msg);
            msg = "[Broadcast] " + username + ":" + msg + "\n";
            std::lock_guard<std::mutex> lock(clients_mutex);

            // Send the message to all clients
            for (auto &[sock, user] : clients) {
                send_message(sock, msg);
            }

        // If private message command is received, send the message to the specified user
        } else if (command == "/msg") {
            std::string target_user, msg;
            iss >> target_user;
            std::getline(iss, msg);
            std::lock_guard<std::mutex> lock(clients_mutex);

            //Check every user
            for (auto &[sock, user] : clients) {
                if (user == target_user) {
                    //Send the message to the target user
                    send_message(sock, "[Private] " + username + ":" + msg + "\n");
                    break;
                }
            }

        // If "/create group" command is received, create a group
        } else if (command == "/create" && iss >> command && command == "group") {
            std::string group_name;
            iss >> group_name;
            std::lock_guard<std::mutex> lock(clients_mutex);
            // Create a new group and add the client to the group
            groups[group_name].insert(client_socket);
            send_message(client_socket, "Group " + group_name + " created.\n");
        
        // If "/join group" command is received, join a group
        } else if (command == "/join" && iss >> command && command == "group") {
            std::string group_name;
            iss >> group_name;
            std::lock_guard<std::mutex> lock(clients_mutex);
            // Add the client to the group
            groups[group_name].insert(client_socket);
            send_message(client_socket, "You joined the group " + group_name + "\n");
        
        // If "/leave group" command is received, leave the group
        } else if (command == "/leave" && iss >> command && command == "group") {
            std::string group_name;
            iss >> group_name;
            std::lock_guard<std::mutex> lock(clients_mutex);
            // Remove the client from the group
            groups[group_name].erase(client_socket);
            send_message(client_socket, "You left the group " + group_name + "\n");
        
        // If the "/group msg" command is received, send a message to the group
        } else if (command == "/group" && iss >> command && command == "msg") {
            std::string group_name, msg;
            iss >> group_name;
            std::getline(iss, msg);
            std::lock_guard<std::mutex> lock(clients_mutex);

            // Send the message to all clients in the group
            for (int sock : groups[group_name]) {

                // Don't send the message to the client that sent it
                if (sock != client_socket)
                    send_message(sock, "[Group " + group_name + "] " + username + ":" + msg + "\n");
            }
        
        // If unexpected command is received, send an error message
        } else {
            send_message(client_socket, "Unknown command.\n");
        }
    }
}


int main() {
    // Load the user data from the users.txt file into memory
    load_users();

    // Create a server socket
    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr{};

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Bind and listen on the server socket
    bind(server_socket, (sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_socket, 10);

    std::cout << "Server started on port " << PORT << std::endl;

    // Main loop to accept incoming client connections
    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        // Accept incoming client
        int client_socket = accept(server_socket, (sockaddr *)&client_addr, &client_len);
        // Handle the client in a separate thread
        std::thread(handle_client, client_socket).detach();
    }

    // Close the server socket
    close(server_socket);
    return 0;
}


