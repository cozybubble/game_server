#include "server.h"
#include "connection.h"
#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <thread>

Server::Server(int port) : port_(port) {}

void Server::initSocket() {
    listen_fd_ = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port_);
    addr.sin_addr.s_addr = INADDR_ANY;

    bind(listen_fd_, (sockaddr*)&addr, sizeof(addr));
    listen(listen_fd_, 5);

    std::cout << "Server started on port " << port_ << std::endl;
}

void Server::start() {
    initSocket();

    while (true) {
        int client_fd = accept(listen_fd_, nullptr, nullptr);
        std::cout << "New client connected: " << client_fd << std::endl;

        //std::thread t(&Server::handleClient, this, client_fd);
        std::thread t([client_fd](){
        		auto conn = std::make_shared<Connection>(client_fd);
        		conn->process();
        });
        t.detach();
    }
}


