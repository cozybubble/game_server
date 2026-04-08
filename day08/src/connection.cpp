#include "connection.h"
#include "player_manager.h"
#include "match_manager.h"
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
	        if (player_id_ != -1) {
		        PlayerManager::getInstance().removePlayer(player_id_);
		        std::cout << "Player " << player_id_ << " removed" << std::endl;
		    }
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

void Connection::sendMessage(const std::string& msg){
	int len = msg.size();
    ::send(fd_, &len, 4, 0);
    ::send(fd_, msg.data(), len, 0);
}

void Connection::handleMessage(const std::string& msg){	
	std::cout << "Message: " << msg << std::endl;
	if(msg.rfind("login:", 0) == 0){
		int id = atoi(msg.substr(6).c_str());
		if (player_id_ != -1) {
		    std::string reply = "already_login";
		    int len = reply.size();
		    send(fd_, &len, 4, 0);
		    send(fd_, reply.data(), len, 0);
		    return;
		}
		player_id_ = id;

		// 加入全局管理
    		PlayerManager::getInstance().addPlayer(id);
	        std::cout << "Player " << id << " logged in" << std::endl;
		PlayerManager::getInstance().bindPlayer2Conn(id, this); 
	        std::string reply = "login_ok";
      
    		int len = reply.size();

	    send(fd_, &len, 4, 0);
	    send(fd_, reply.data(), len, 0);
	}else if(msg == "match"){
		if (player_id_ == -1) {
        std::string reply = "please_login";
        int len = reply.size();
        send(fd_, &len, 4, 0);
        send(fd_, reply.data(), len, 0);
        return;
    }
    auto result = MatchManager::getInstance().addPlayer(player_id_);

    if (result.empty()) {
        std::string reply = "waiting";
        int len = reply.size();
        send(fd_, &len, 4, 0);
        send(fd_, reply.data(), len, 0);
    } else {
        std::string reply = "match_success:" +
            std::to_string(result[0]) + "," +
            std::to_string(result[1]);

        	PlayerManager::getInstance().sendToPlayer(result[0], reply);
		PlayerManager::getInstance().sendToPlayer(result[1], reply);
        
    }
	}
	
}

