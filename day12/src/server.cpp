#include "server.h"
#include "connection.h"
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <fcntl.h>

Server::Server(int port) : port_(port), listen_fd_(-1) {}

Server::~Server() {
    if (listen_fd_ != -1) {
        close(listen_fd_);
    }
}

void Server::initSocket() {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_fd_ == -1) {
        std::cerr << "[Server] socket failed" << std::endl;
        exit(1);
    }

    // ✅ 设置 SO_REUSEADDR — 允许端口复用（重启服务时不报 Address already in use）
    int opt = 1;
    setsockopt(listen_fd_, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    
    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(listen_fd_, (sockaddr*)&addr, sizeof(addr));
    listen(listen_fd_, 128);

    std::cout << "Server started on port " << port_ << std::endl;
}

void Server::start() {
    initSocket();

	// ✅ 将监听套接字加入 Reactor
    reactor_.addFd(listen_fd_, EPOLLIN,
        [this](uint32_t events) { this->handleAccept(events); }
    );

    // 启动事件循环（单线程处理所有事情）
    reactor_.loop();
}

void Server::handleAccept(uint32_t events) {
	if (events & (EPOLLERR | EPOLLHUP)) {
        std::cerr << "[Server] listen_fd error!" << std::endl;
        stop();
        return;
    }

       // 接受所有待处理的连接（可能有多个）
	while(true){
		sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        
        int client_fd = accept(listen_fd_, (sockaddr*)&client_addr, &addr_len);
        if (client_fd == -1) {
            if (errno == EAGAIN || EWOULDBLOCK) {
                // ✅ 非阻塞模式下没有更多连接了，正常退出
                break;
            }
            std::cerr << "[Server] accept error: " << strerror(errno) << std::endl;
            break;
        }

        std::cout << "[Server] new connection: fd=" << client_fd 
                  << ", ip=" << inet_ntoa(client_addr.sin_addr)
                  << ", port=" << ntohs(client_addr.sin_port) << std::endl;

        // ✅ 创建 Connection 并注册到 Reactor
        auto conn = std::make_shared<Connection>(client_fd, reactor_);

        // 将 client_fd 注册到 epoll，关注可读事件
        reactor_.addFd(client_fd, EPOLLIN | EPOLLET,
            [conn](uint32_t events) mutable {
                conn->handleEvents(events);  // ← Connection 处理自己的事件
            }
        );
	}
}

void Server::stop() {
    reactor_.stop();
}

