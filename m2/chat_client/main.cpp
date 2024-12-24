#include <iostream>
#include <thread>
#include <atomic>
#include <cstring>
#include <unistd.h>
#include <termios.h>
#include <mutex>
#include <arpa/inet.h>
#include <sys/select.h>

constexpr int BUFFER_SIZE = 1024;
std::atomic<bool> running(true);

std::string userInputBuffer;

void setRawMode(bool enable) {
    static struct termios oldt, newt;
    if (enable) {
        tcgetattr(STDIN_FILENO, &oldt);
        newt = oldt;
        newt.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
    } else {
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    }
}

void displayIncomingMessage(const std::string& message) {
    if (!running) {
        return;
    }

    if (!userInputBuffer.empty()) {
        std::cout << "\r" << std::string(userInputBuffer.size() + 5, ' ') << "\r";
    } else {
        std::cout << "\r" << std::flush;
    }
    
    std::cout << "[Message]: " << message << std::endl;
    
    if (!userInputBuffer.empty()) {
        std::cout << "> " << userInputBuffer << std::flush;
    } else {
        std::cout << "> " << std::flush;
    }
}

void receiveMessages(int socket_fd) {
    char buffer[BUFFER_SIZE];
    sockaddr_in sender_addr;
    socklen_t sender_len = sizeof(sender_addr);

    while (running) {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(socket_fd, &readfds);

        struct timeval tv;
        tv.tv_sec = 1;
        tv.tv_usec = 0;

        int select_result = select(socket_fd + 1, &readfds, nullptr, nullptr, &tv);

        if (select_result > 0) {
            ssize_t received_bytes = recvfrom(socket_fd, buffer, BUFFER_SIZE - 1, 0, 
                                              (sockaddr*)&sender_addr, &sender_len);
            if (received_bytes > 0) {
                buffer[received_bytes] = '\0';
                displayIncomingMessage(buffer);
            }
        } else if (select_result < 0) {
            perror("select failed");
            break;
        }

        if (!running) {
            break;
        }
    }
}

void sendMessage(int socket_fd, sockaddr_in& broadcast_addr, const std::string& username) {
    std::string message;
    char ch;

    std::string enterMessage = username + " join to the chat!";
    sendto(socket_fd, enterMessage.c_str(), enterMessage.size(), 0,
           (sockaddr*)&broadcast_addr, sizeof(broadcast_addr));

    setRawMode(true);
    while (running) {
        if (read(STDIN_FILENO, &ch, 1) == 1) {
            if (ch == '\n') {
                if (!userInputBuffer.empty()) {
                    if (userInputBuffer == "/quit") {
                        std::cout << "\n";
                        running = false;
                        break;
                    }
                    message = username + ": " + userInputBuffer;
                    userInputBuffer.clear();
                    sendto(socket_fd, message.c_str(), message.size(), 0,
                           (sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
                }
            } else if (ch == '\b' || ch == 127) {
                if (!userInputBuffer.empty()) {
                    userInputBuffer.pop_back();
                    std::cout << "\b \b" << std::flush;
                }
            } else {
                userInputBuffer.push_back(ch);
                std::cout << ch << std::flush;
            }
        }
    }
    setRawMode(false);

    std::string leaveMessage = username + " left the chat(";
    sendto(socket_fd, leaveMessage.c_str(), leaveMessage.size(), 0,
           (sockaddr*)&broadcast_addr, sizeof(broadcast_addr));
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <port>" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);

    std::string username;
    std::cout << "Enter your username: ";
    std::getline(std::cin, username);
    if (username.empty()) {
        std::cerr << "Username cannot be empty." << std::endl;
        return 1;
    }

    int socket_fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (socket_fd < 0) {
        perror("Socket creation failed");
        return 1;
    }

    int reuse_enable = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &reuse_enable, sizeof(reuse_enable)) < 0) {
        perror("Failed to set SO_REUSEADDR");
        close(socket_fd);
        return 1;
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEPORT, &reuse_enable, sizeof(reuse_enable)) < 0) {
        perror("Failed to set SO_REUSEPORT");
        close(socket_fd);
        return 1;
    }

    int broadcast_enable = 1;
    if (setsockopt(socket_fd, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) < 0) {
        perror("Failed to enable broadcast");
        close(socket_fd);
        return 1;
    }

    sockaddr_in local_addr{};
    local_addr.sin_family = AF_INET;
    local_addr.sin_addr.s_addr = INADDR_ANY;
    local_addr.sin_port = htons(port);

    if (bind(socket_fd, (sockaddr*)&local_addr, sizeof(local_addr)) < 0) {
        perror("Bind failed");
        close(socket_fd);
        return 1;
    }

    sockaddr_in broadcast_addr{};
    broadcast_addr.sin_family = AF_INET;
    broadcast_addr.sin_addr.s_addr = htonl(INADDR_BROADCAST);
    broadcast_addr.sin_port = htons(port);

    std::thread receiver(receiveMessages, socket_fd);
    std::thread sender(sendMessage, socket_fd, std::ref(broadcast_addr), username);

    sender.join();
    running = false;
    receiver.join();

    close(socket_fd);
    return 0;
}