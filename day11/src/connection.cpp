#include "connection.h"
#include "player_manager.h"
#include "match_manager.h"
#include "room_manager.h"
#include "packet.pb.h"
#include "message.pb.h"
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
	        	auto player = PlayerManager::getInstance().getPlayer(player_id_);
    if (player && player->getRoomId() != -1) {
        RoomManager::getInstance().leaveRoom(player_id_, player->getRoomId());
    }
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

        // 现在：先把包体解析成 Packet
        std::string body = buffer_.substr(4, len);

        game::Packet packet;
        if (!packet.ParseFromString(body)) {
            sendError(1001, "invalid packet");
            buffer_ = buffer_.substr(4 + len);
            continue;
        }

        handlePacket(packet);

        // remove processed data from buffer_
        buffer_ = buffer_.substr(4+len);
	}
}

void Connection::sendPacket(const game::Packet& packet) {
    std::lock_guard<std::mutex> lock(send_mutex_);

    std::string body;
    if (!packet.SerializeToString(&body)) {
        std::cerr << "serialize packet failed" << std::endl;
        return;
    }

    int len = static_cast<int>(body.size());
    ::send(fd_, &len, 4, 0);
    ::send(fd_, body.data(), len, 0);
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
