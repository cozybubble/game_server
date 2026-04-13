#ifndef CONNECTION_H
#define CONNECTION_H

#include <string>
#include <memory>
#include<mutex>
#include "packet.pb.h"
#include "reactor.h"


class Connection : public std::enable_shared_from_this<Connection> {
public:
	Connection(int fd, Reactor& reactor);
    ~Connection();

    // ✅ 由 Reactor 回调调用
    void handleEvents(uint32_t events);

    void sendPacket(const game::Packet& packet);

    int getPlayerId() const { return player_id_; }

private:
	int fd_;
	Reactor& reactor_;       // ✅ 引用 Reactor（用于动态修改事件）
	std::string buffer_;
	int player_id_ = -1;
	std::mutex send_mutex_;
    bool closed_ = false;    // 连接是否已关闭

    void handleClose();      // 处理连接关闭
	void handleRead();
	void handleWrite();      // 写入数据
    void handlePacket(const game::Packet& packet);
	void tryParsePackets();

	void onLoginReq(const std::string& payload);
    void onMatchReq(const std::string& payload);
    void onRoomChatReq(const std::string& payload);
    void onLeaveRoomReq(const std::string& payload);

    void sendError(int code, const std::string& msg);
};

#endif