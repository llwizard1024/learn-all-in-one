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
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }
    std::cout << "[SERVER] Сокет создан. Дескриптор: " << server_fd << std::endl;

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt");
        close(server_fd);
        return 1;
    }

    struct sockaddr_in address;
    std::memset(&address, 0, sizeof(address));
    address.sin_family = AF_INET;
    address.sin_port = htons(PORT);
    address.sin_addr.s_addr = INADDR_ANY;

    if (bind(server_fd, (struct sockaddr*)&address, sizeof(address)) == -1) {
        perror("bind");
        close(server_fd);
        return 1;
    }
    std::cout << "[SERVER] Привязан к порту " << PORT << std::endl;

    if (listen(server_fd, 5) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }
    std::cout << "[SERVER] Ожидание подключений..." << std::endl;

    struct sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
new_client:
    int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &client_addr_len);
    if (client_fd == -1) {
        perror("accept");
        close(server_fd);
        return 1;
    }

    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    std::cout << "[SERVER] Подключился клиент: " << client_ip
              << ":" << ntohs(client_addr.sin_port) << std::endl;

    char buffer[BUFFER_SIZE];
    while (true) {
        std::memset(buffer, 0, BUFFER_SIZE);
        
        ssize_t bytes_read = read(client_fd, buffer, BUFFER_SIZE - 1);
        if (bytes_read == -1) {
            perror("read");
            break;
        }
        if (bytes_read == 0) {
            std::cout << "[SERVER] Клиент закрыл соединение." << std::endl;
            goto new_client;
            break;
        }

        std::cout << "[SERVER] Получено: " << buffer << std::endl;

        std::string response = "Echo: " + std::string(buffer);

        ssize_t bytes_written = write(client_fd, response.c_str(), response.size());
        if (bytes_written == -1) {
            perror("write");
            break;
        }
    }

    close(client_fd);
    close(server_fd);
    std::cout << "[SERVER] Завершение работы." << std::endl;
    return 0;
}
