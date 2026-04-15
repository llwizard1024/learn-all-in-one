#include <iostream>
#include <memory>
#include <string>
#include <unordered_map>
#include <cstring>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <errno.h>

const int PORT = 8080;
const int MAX_EVENTS = 64;
const int BUFFER_SIZE = 1024;

enum ClientState {
    STATE_NICKNAME,
    STATE_CHAT
};

class Client {
public:
    int fd;
    ClientState state;
    std::string nickname;
    std::string in_buffer;
    std::string out_buffer;
    bool want_write;

    Client(int _fd)
        : fd(_fd)
        , state(STATE_NICKNAME)
        , want_write(false)
    {}

    ~Client() { if (fd != -1) close(fd); }

    Client(const Client&) = delete;
    Client& operator=(const Client&) = delete;

    void queue_message(const std::string& msg, int epoll_fd) {
        out_buffer += msg;
        if (!want_write && !out_buffer.empty()) {
            want_write = true;
            struct epoll_event ev;
            ev.events = EPOLLIN | EPOLLOUT | EPOLLET;
            ev.data.ptr = this;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1) {
                perror("epoll_ctl MOD (add EPOLLOUT)");
            }
        }
    }

    bool handle_write(int epoll_fd) {
        if (out_buffer.empty()) {
            if (want_write) {
                want_write = false;
                struct epoll_event ev;
                ev.events = EPOLLIN | EPOLLET;
                ev.data.ptr = this;
                if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1) {
                    perror("epoll_ctl MOD (remove EPOLLOUT)");
                }
            }
            return true;
        }

        ssize_t written = write(fd, out_buffer.c_str(), out_buffer.size());
        if (written == -1) {
            if (errno != EAGAIN && errno != EWOULDBLOCK) {
                return false;
            }
            return false;
        }

        out_buffer.erase(0, written);

        if (out_buffer.empty() && want_write) {
            want_write = false;
            struct epoll_event ev;
            ev.events = EPOLLIN | EPOLLET;
            ev.data.ptr = this;
            if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1) {
                perror("epoll_ctl MOD (remove EPOLLOUT)");
            }
        }
        return true;
    }
};

std::unordered_map<int, std::unique_ptr<Client>> clients;

bool set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags == -1) return false;
    return fcntl(fd, F_SETFL, flags | O_NONBLOCK) != -1;
}

void send_disconnect_notification(const std::string nickname, int epoll_fd) {
    for (auto& pair : clients) {
        Client* cl = pair.second.get();
        if (cl->state == STATE_CHAT) {
            cl->queue_message(nickname + " покинул чат\n", epoll_fd);
        }
    }
}

void disconnect_client(Client* client, int epoll_fd) {
    if (!client) return;
    std::cout << "[SERVER] Клиент отключён: fd=" << client->fd << std::endl;

    if (!client->nickname.empty()) {
        send_disconnect_notification(client->nickname, epoll_fd);
    }

    epoll_ctl(epoll_fd, EPOLL_CTL_DEL, client->fd, nullptr);
    close(client->fd);
    clients.erase(client->fd);
}

void broadcast(const std::string& msg, Client* sender, int epoll_fd) {
    for (auto& pair : clients) {
        Client* cl = pair.second.get();
        if (cl->state == STATE_CHAT) {
            cl->queue_message(msg, epoll_fd);
        }
    }
}

void handle_client_read(Client* client, int epoll_fd) {
    char buffer[BUFFER_SIZE];
    while (true) {
        ssize_t n = read(client->fd, buffer, sizeof(buffer) - 1);
        if (n == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                perror("read");
                disconnect_client(client, epoll_fd);
                return;
            }
        }
        if (n == 0) {
            disconnect_client(client, epoll_fd);
            return;
        }

        client->in_buffer.append(buffer, n);

        size_t pos;
        while ((pos = client->in_buffer.find('\n')) != std::string::npos) {
            std::string line = client->in_buffer.substr(0, pos);
            client->in_buffer.erase(0, pos + 1);

            if (!line.empty() && line.back() == '\r') {
                line.pop_back();
            }

            if (line.empty()) continue;

            if (client->state == STATE_NICKNAME) {
                client->nickname = line;
                client->state = STATE_CHAT;
                std::cout << "[SERVER] Клиент fd=" << client->fd
                          << " установил ник: " << client->nickname << std::endl;

                std::string welcome = "Добро пожаловать в чат, " + client->nickname + "!\n";
                client->queue_message(welcome, epoll_fd);

                std::string join_msg = "[SERVER]: " + client->nickname + " присоединился к чату.\n";
                broadcast(join_msg, client, epoll_fd);
            } else {
                std::cout << "[SERVER] Сообщение от " << client->nickname
                          << ": " << line << std::endl;

                std::string chat_msg = "[" + client->nickname + "]: " + line + "\n";
                broadcast(chat_msg, client, epoll_fd);
            }
        }
    }
}

void accept_new_client(int server_fd, int epoll_fd) {
    while (true) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_fd = accept(server_fd, (struct sockaddr*)&client_addr, &addr_len);

        if (client_fd == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                break;
            } else {
                perror("accept");
                break;
            }
        }

        if (!set_nonblocking(client_fd)) {
            perror("set_nonblocking client_fd");
            close(client_fd);
            continue;
        }

        clients[client_fd] = std::make_unique<Client>(client_fd);
        Client* raw_ptr = clients[client_fd].get();

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, INET_ADDRSTRLEN);
        std::cout << "[SERVER] Новое подключение: " << client_ip
                  << ":" << ntohs(client_addr.sin_port) << " (fd=" << client_fd << ")" << std::endl;

        struct epoll_event ev;
        ev.events = EPOLLIN | EPOLLET;
        ev.data.ptr = raw_ptr;
        if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, client_fd, &ev) == -1) {
            perror("epoll_ctl ADD client");
            clients.erase(client_fd);
        } else {
            std::string prompt = "Введите ваш никнейм:\n";
            raw_ptr->queue_message(prompt, epoll_fd);
        }
    }
}

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("socket");
        return 1;
    }

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

    if (listen(server_fd, SOMAXCONN) == -1) {
        perror("listen");
        close(server_fd);
        return 1;
    }

    if (!set_nonblocking(server_fd)) {
        perror("set_nonblocking server_fd");
        close(server_fd);
        return 1;
    }

    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) {
        perror("epoll_create1");
        close(server_fd);
        return 1;
    }

    struct epoll_event ev;
    ev.events = EPOLLIN | EPOLLET;
    ev.data.fd = server_fd;
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, server_fd, &ev) == -1) {
        perror("epoll_ctl ADD server_fd");
        close(server_fd);
        close(epoll_fd);
        return 1;
    }

    std::cout << "[SERVER] Чат-сервер запущен на порту " << PORT << std::endl;
    std::cout << "[SERVER] Ожидание подключений..." << std::endl;

    struct epoll_event events[MAX_EVENTS];

    while (true) {
        int nfds = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (nfds == -1) {
            perror("epoll_wait");
            break;
        }

        for (int i = 0; i < nfds; ++i) {
            if (events[i].data.fd == server_fd) {
                accept_new_client(server_fd, epoll_fd);
                continue;
            }

            Client* client = static_cast<Client*>(events[i].data.ptr);
            if (!client) continue;

            if (events[i].events & (EPOLLERR | EPOLLHUP)) {
                disconnect_client(client, epoll_fd);
                continue;
            }

            if (events[i].events & EPOLLIN) {
                handle_client_read(client, epoll_fd);
            }

            if (events[i].events & EPOLLOUT) {
                if (!client->handle_write(epoll_fd)) {
                    disconnect_client(client, epoll_fd);
                }
            }
        }
    }

    close(server_fd);
    close(epoll_fd);

    for (auto& pair : clients) {
        close(pair.second->fd);
    }
    clients.clear();

    return 0;
}