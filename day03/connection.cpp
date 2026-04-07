#include "connection.h"

#include <iostream>
#include <cstring>

// ✅ 网络相关
#include <sys/socket.h>   // send / recv
#include <unistd.h>       // close

Connection::Connection(int fd) : fd_(fd) {}

void Connection::process() {
    char buffer[1024];

    while (true) {
        memset(buffer, 0, sizeof(buffer));

        int n = recv(fd_, buffer, sizeof(buffer), 0);

        if (n <= 0) {
            std::cout << "Client disconnected: " << fd_ << std::endl;
            close(fd_);
            break;
        }

        std::cout << "Recv from " << fd_ << ": " << buffer << std::endl;

        handleWrite(buffer, n);
    }
}

void Connection::handleWrite(const char* data, int len) {
    send(fd_, data, len, 0);
}