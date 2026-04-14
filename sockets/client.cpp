#include <iostream>
#include <string>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

const int PORT = 8080;
const int BUFFER_SIZE = 1024;

int main() {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1) {
        perror("socket");
        return 1;
    }
    std::cout << "[CLIENT] Сокет создан." << std::endl;

    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("inet_pton");
        close(sock);
        return 1;
    }

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(sock);
        return 1;
    }
    std::cout << "[CLIENT] Подключён к серверу на порту " << PORT << std::endl;

    std::string message;
    char buffer[BUFFER_SIZE];
    
    while (true) {
        std::cout << "Введите сообщение (или 'quit' для выхода): ";
        std::getline(std::cin, message);
        
        if (message == "quit") {
            break;
        }

        ssize_t bytes_sent = write(sock, message.c_str(), message.size());
        if (bytes_sent == -1) {
            perror("write");
            break;
        }

        std::memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_read = read(sock, buffer, BUFFER_SIZE - 1);
        if (bytes_read == -1) {
            perror("read");
            break;
        }
        if (bytes_read == 0) {
            std::cout << "[CLIENT] Сервер разорвал соединение." << std::endl;
            break;
        }

        std::cout << "[CLIENT] Ответ сервера: " << buffer << std::endl;
    }

    close(sock);
    std::cout << "[CLIENT] Соединение закрыто." << std::endl;
    return 0;
}
