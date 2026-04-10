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

    // 1. 发送 login
    std::string msg = "login:1002";
    int len = msg.size();
    send(fd, &len, 4, 0);
    send(fd, msg.c_str(), len, 0);

    // 接收 login 回复
    char buf[1024] = {0};
    int recv_len = recv(fd, buf, sizeof(buf) - 1, 0);
    if (recv_len > 0) {
        buf[recv_len] = '\0';
        std::cout << "Login reply: " << buf << std::endl;
    }

    // 2. 发送 match
    msg = "match";
    len = msg.size();
    send(fd, &len, 4, 0);
    send(fd, msg.c_str(), len, 0);

    // 接收 match 返回的长度
    recv(fd, &recv_len, 4, 0);
    
    // 清空 buf（关键步骤）
    memset(buf, 0, sizeof(buf));
    
    // 接收 match 返回的内容
    recv(fd, buf, recv_len, 0);
    buf[recv_len] = '\0';  // 添加字符串结束符
    std::cout << "Match reply: " << buf << std::endl;

	// 接收 match 返回的长度
    recv(fd, &recv_len, 4, 0);

	// 清空 buf（关键步骤）
    memset(buf, 0, sizeof(buf));
    
    // 接收 match 返回的内容
    recv(fd, buf, recv_len, 0);
    buf[recv_len] = '\0';  // 添加字符串结束符
    std::cout << "Match reply2: " << buf << std::endl;
    close(fd);
    return 0;
}