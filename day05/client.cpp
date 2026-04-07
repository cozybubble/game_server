#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <iostream>

int main() {
    int fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in addr{};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(8888);
    inet_pton(AF_INET, "127.0.0.1", &addr.sin_addr);

    connect(fd, (sockaddr*)&addr, sizeof(addr));

    std::string msg = "login:1001";
    int len = msg.size();

    // 发送长度
    send(fd, &len, 4, 0);

    // 发送内容
    send(fd, msg.c_str(), len, 0);

    // 接收返回
    int recv_len;
    recv(fd, &recv_len, 4, 0);

    char buf[1024] = {0};
    recv(fd, buf, recv_len, 0);

    std::cout << "Server reply: " << buf << std::endl;

    close(fd);
}