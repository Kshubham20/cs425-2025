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

std::unordered_map<std::string, std::string> users;
std::unordered_map<int, std::string> clients;
std::unordered_map<std::string, std::unordered_set<int>> groups;
std::mutex clients_mutex;

void load_users() {
    std::ifstream file("users.txt");
    std::string line;
    while (std::getline(file, line)) {
        size_t delimiter = line.find(":");
        if (delimiter != std::string::npos) {
            std::string username = line.substr(0, delimiter);
            std::string password = line.substr(delimiter + 1);
            users[username] = password;
        }
    }
}

void send_message(int client_socket, const std::string &message) {
    send(client_socket, message.c_str(), message.size(), 0);
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    std::string username;

    send_message(client_socket, "Enter username: ");
    memset(buffer, 0, BUFFER_SIZE);
    recv(client_socket, buffer, BUFFER_SIZE, 0);
    username = buffer;

    send_message(client_socket, "Enter password: ");
    memset(buffer, 0, BUFFER_SIZE);
    recv(client_socket, buffer, BUFFER_SIZE, 0);
    std::string password = buffer;

    if (users.find(username) == users.end() || users[username] != password) {
        send_message(client_socket, "Authentication failed.\n");
        close(client_socket);
        return;
    }

    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients[client_socket] = username;
    }

    send_message(client_socket, "Welcome to the chat server!\n");

    while (true) {
        memset(buffer, 0, BUFFER_SIZE);
        int bytes_received = recv(client_socket, buffer, BUFFER_SIZE, 0);
        if (bytes_received <= 0) {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.erase(client_socket);
            close(client_socket);
            return;
        }

        std::string message = buffer;
        if (message == "/exit") {
            std::lock_guard<std::mutex> lock(clients_mutex);
            clients.erase(client_socket);
            close(client_socket);
            return;
        }

        std::istringstream iss(message);
        std::string command;
        iss >> command;

        if (command == "/broadcast") {
            std::string msg;
            std::getline(iss, msg);
            msg = "[Broadcast] " + username + ":" + msg + "\n";
            std::lock_guard<std::mutex> lock(clients_mutex);
            for (auto &[sock, user] : clients) {
                send_message(sock, msg);
            }
        } else if (command == "/msg") {
            std::string target_user, msg;
            iss >> target_user;
            std::getline(iss, msg);
            std::lock_guard<std::mutex> lock(clients_mutex);
            for (auto &[sock, user] : clients) {
                if (user == target_user) {
                    send_message(sock, "[Private] " + username + ":" + msg + "\n");
                    break;
                }
            }
        } else if (command == "/create" && iss >> command && command == "group") {
            std::string group_name;
            iss >> group_name;
            std::lock_guard<std::mutex> lock(clients_mutex);
            groups[group_name].insert(client_socket);
            send_message(client_socket, "Group " + group_name + " created.\n");
        } else if (command == "/join" && iss >> command && command == "group") {
            std::string group_name;
            iss >> group_name;
            std::lock_guard<std::mutex> lock(clients_mutex);
            groups[group_name].insert(client_socket);
            send_message(client_socket, "You joined the group " + group_name + "\n");
        } else if (command == "/leave" && iss >> command && command == "group") {
            std::string group_name;
            iss >> group_name;
            std::lock_guard<std::mutex> lock(clients_mutex);
            groups[group_name].erase(client_socket);
            send_message(client_socket, "You left the group " + group_name + "\n");
        } else if (command == "/group" && iss >> command && command == "msg") {
            std::string group_name, msg;
            iss >> group_name;
            std::getline(iss, msg);
            std::lock_guard<std::mutex> lock(clients_mutex);
            for (int sock : groups[group_name]) {
                if (sock != client_socket)
                    send_message(sock, "[Group " + group_name + "] " + username + ":" + msg + "\n");
            }
        } else {
            send_message(client_socket, "Unknown command.\n");
        }
    }
}

int main() {
    load_users();

    int server_socket = socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in server_addr{};

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    bind(server_socket, (sockaddr *)&server_addr, sizeof(server_addr));
    listen(server_socket, 10);

    std::cout << "Server started on port " << PORT << std::endl;

    while (true) {
        sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);
        int client_socket = accept(server_socket, (sockaddr *)&client_addr, &client_len);
        std::thread(handle_client, client_socket).detach();
    }

    close(server_socket);
    return 0;
}

