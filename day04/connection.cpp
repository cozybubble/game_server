#include "connection.h"

#include <iostream>
#include <cstring>

// ✅ 网络相关
#include <sys/socket.h>   // send / recv
#include <unistd.h>       // close

Connection::Connection(int fd) : fd_(fd) {}

void Connection::process() {
    char buf[1024];

    while (true) {
        int n = recv(fd_, buf, sizeof(buf), 0);

        if (n <= 0) {
            std::cout << "Client disconnected: " << fd_ << std::endl;
            close(fd_);
            break;
        }

        buffer_.append(buf, n);

        handleRead();
    }
}

void Connection::handleRead(){
	while(true){
		// 至少要有4字节长度
        if (buffer_.size() < 4) return;

        // read length
        int len = 0;
        memcpy(&len, buffer_.data(), 4);

        // check if complete
        if(buffer_.size() < 4 + len) return;

        // take out msg
        std::string msg = buffer_.substr(4, len);

        handleMessage(msg);

        // remove processed data from buffer_
        buffer_ = buffer_.substr(4+len);
	}
}

void Connection::handleMessage(const std::string& msg){
	std::cout << "Message: " << msg << std::endl;

	// echo 回去（也要带长度）
    int len = msg.size();

    send(fd_, &len, 4, 0);
    send(fd_, msg.data(), len, 0);
}

