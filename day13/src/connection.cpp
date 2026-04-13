#include "connection.h"
#include "player_manager.h"
#include "match_manager.h"
#include "room_manager.h"
#include "packet.pb.h"
#include "message.pb.h"
#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <unistd.h>

Connection::Connection(int fd, Reactor& reactor) 
    : fd_(fd), reactor_(reactor) {}

Connection::~Connection() {
    if (!closed_) {
        close(fd_);
    }
}

void Connection::handleEvents(uint32_t events) {
	std::cout << "[Connection] fd=" << fd_ 
              << " events: " << events << std::endl;
              
	if (events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
        // 错误 / 对方关闭连接
        handleClose();
        return;
    }

    if (events & EPOLLIN) {
        handleRead();
    }

    if (events & EPOLLOUT) {
        handleWrite();
    }
}

void Connection::handleClose() {
    if (closed_) return;
    closed_ = true;

    std::cout << "[Connection] fd=" << fd_ << " closed" << std::endl;

    // 清理玩家数据
    if (player_id_ != -1) {
        auto player = PlayerManager::getInstance().getPlayer(player_id_);
        if (player && player->getRoomId() != -1) {
            RoomManager::getInstance().leaveRoom(player_id_, player->getRoomId());
        }
        PlayerManager::getInstance().removePlayer(player_id_);
        std::cout << "Player " << player_id_ << " removed" << std::endl;
    }

    // 从 Reactor 中移除
    reactor_.removeFd(fd_);
    close(fd_);
}

void Connection::handleRead(){
	char buf[4096];
	while(true){
		int n = recv(fd_, buf, sizeof(buf), 0);
		if (n > 0) {
            // 收到数据
            buffer_.append(buf, n);
            std::cout << "[Connection] fd=" << fd_ 
                      << " read " << n << " bytes, total buffer=" 
                      << buffer_.size() << std::endl;
            
            // 尝试解析完整包
            tryParsePackets();
        } else if (n == 0) {
            // 对端关闭连接
            std::cout << "[Connection] fd=" << fd_ << " peer closed" << std::endl;
            handleClose();
            return;
        } else {
            // n < 0
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // ✅ 正常情况：暂时没有更多数据了
                break;
            }
            // 真正的错误
            std::cerr << "[Connection] recv error: fd=" << fd_ 
                      << ", " << strerror(errno) << std::endl;
            handleClose();
            return;
        }
	}
}

void Connection::tryParsePackets() {
    while (true) {
        // 至少要有 4 字节长度头
        if (buffer_.size() < 4) return;

        // 读取长度（小端序）
        int len = 0;
        memcpy(&len, buffer_.data(), 4);

        // 检查是否收到完整的包
        if (buffer_.size() < 4 + len) return;

        // 解析包体
        std::string body = buffer_.substr(4, len);
        game::Packet packet;
        if (!packet.ParseFromString(body)) {
            sendError(1001, "invalid packet");
            buffer_ = buffer_.substr(4 + len);
            continue;
        }

        // 处理包
        handlePacket(packet);

        // 从缓冲区移除已处理的数据
        buffer_ = buffer_.substr(4 + len);
    }
}



void Connection::sendPacket(const game::Packet& packet) {
    if (closed_) return;

    std::lock_guard<std::mutex> lock(send_mutex_);

    std::string body;
    if (!packet.SerializeToString(&body)) {
        std::cerr << "[Connection] serialize failed, fd=" << fd_ << std::endl;
        return;
    }

    int len = static_cast<int>(body.size());
    
    // ✅ 注意：即使使用 epoll，send 在大多数情况下也是可以直接发的
    // 因为 TCP 有发送缓冲区，只要没满就能写
    ssize_t ret = send(fd_, &len, 4, MSG_NOSIGNAL);  // SIGPIPE 保护
    if (ret <= 0 && errno != EAGAIN) {
        handleClose();
        return;
    }

    ret = send(fd_, body.data(), len, MSG_NOSIGNAL);
    if (ret <= 0 && errno != EAGAIN) {
        handleClose();
    }
}


void Connection::handleWrite() {
    // TODO: 如果有发送缓冲区未完成的数据，在这里继续发送
    // 当前简单实现中 send 是同步完成的，这个方法暂时留空
    // 后续可以优化为异步写（先写到应用层缓冲区，再等待 EPOLLOUT 发送）
}

void Connection::handlePacket(const game::Packet& packet) {
    switch (packet.cmd()) {
        case game::CMD_LOGIN_REQ:
            onLoginReq(packet.payload());
            break;
        case game::CMD_MATCH_REQ:
            onMatchReq(packet.payload());
            break;
        case game::CMD_ROOM_CHAT_REQ:
            onRoomChatReq(packet.payload());
            break;
        case game::CMD_LEAVE_ROOM_REQ:
            onLeaveRoomReq(packet.payload());
            break;
        default:
            sendError(1002, "unknown cmd");
            break;
    }
}

void Connection::onLoginReq(const std::string& payload) {
    game::LoginReq req;
    if (!req.ParseFromString(payload)) {
        sendError(2001, "bad login req");
        return;
    }

    game::LoginRsp rsp;
    rsp.set_player_id(req.player_id());

    if (player_id_ != -1) {
        rsp.set_success(false);
        rsp.set_message("already_login");
    } else {
        player_id_ = req.player_id();
        PlayerManager::getInstance().addPlayer(player_id_);
        PlayerManager::getInstance().bindPlayer2Conn(player_id_, shared_from_this());
        std::cout << "Player " << player_id_ << " logged in" << std::endl;

        rsp.set_success(true);
        rsp.set_message("login_ok");
    }

    game::Packet packet;
    packet.set_cmd(game::CMD_LOGIN_RSP);
    rsp.SerializeToString(packet.mutable_payload());
    sendPacket(packet);
}

void Connection::onMatchReq(const std::string& payload) {
    game::MatchReq req;
    if (!req.ParseFromString(payload)) {
        sendError(2002, "bad match req");
        return;
    }

    game::MatchRsp rsp;

    if (player_id_ == -1) {
        rsp.set_success(false);
        rsp.set_message("please_login");
    } else {
        auto result = MatchManager::getInstance().addPlayer(player_id_);
        if (result.empty()) {
            rsp.set_success(true);
            rsp.set_message("waiting");
        } else {
            int room_id = RoomManager::getInstance().createRoom(result);
            rsp.set_success(true);
            rsp.set_message("matched");
            rsp.set_room_id(room_id);

            auto room = RoomManager::getInstance().getRoom(room_id);
            if (room) {
                game::RoomEnterNtf ntf;
                ntf.set_room_id(room_id);
                for (int pid : room->getPlayers()) {
                    ntf.add_player_ids(pid);
                }

                game::Packet notify_packet;
                notify_packet.set_cmd(game::CMD_ROOM_ENTER_NTF);
                ntf.SerializeToString(notify_packet.mutable_payload());

                // 这里要求你把 Room::broadcast 改成支持 Packet
                room->broadcast(notify_packet);
            }
        }
    }

    game::Packet packet;
    packet.set_cmd(game::CMD_MATCH_RSP);
    rsp.SerializeToString(packet.mutable_payload());
    sendPacket(packet);
}

void Connection::sendError(int code, const std::string& msg) {
    game::ErrorRsp rsp;
    rsp.set_code(code);
    rsp.set_message(msg);

    game::Packet packet;
    packet.set_cmd(game::CMD_ERROR_RSP);
    rsp.SerializeToString(packet.mutable_payload());

    sendPacket(packet);
}

void Connection::onRoomChatReq(const std::string& payload) {
    game::RoomChatReq req;
    if (!req.ParseFromString(payload)) {
        sendError(2003, "bad room chat req");
        return;
    }

    std::cout << "room chat req: " << req.content() << std::endl;
}

void Connection::onLeaveRoomReq(const std::string& payload) {
    game::LeaveRoomReq req;
    if (!req.ParseFromString(payload)) {
        sendError(2004, "bad leave room req");
        return;
    }

    std::cout << "leave room req" << std::endl;
}
