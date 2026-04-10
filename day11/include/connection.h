#ifndef CONNECTION_H
#define CONNECTION_H

#include <string>
#include <memory>
#include<mutex>
#include "packet.pb.h"

class Connection : public std::enable_shared_from_this<Connection> {
	public:
	Connection(int fd);
	void process();
	void sendPacket(const game::Packet& packet);

private:
	int fd_;
	std::string buffer_;
	int player_id_ = -1;
	std::mutex send_mutex_;

	void handleRead();
    void handlePacket(const game::Packet& packet);

	void onLoginReq(const std::string& payload);
    void onMatchReq(const std::string& payload);
    void onRoomChatReq(const std::string& payload);
    void onLeaveRoomReq(const std::string& payload);

    void sendError(int code, const std::string& msg);
};

#endif