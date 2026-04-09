#include "room_manager.h"
#include "player_manager.h"

RoomManager& RoomManager::getInstance() {
    static RoomManager instance;
    return instance;
}

int RoomManager::createRoom(const std::vector<int>& players) {
	std::lock_guard<std::mutex> lock(mutex_);

	int room_id = next_room_id_++;
	auto room = std::make_shared<Room>(room_id, players);
	rooms_[room_id] = room;

	for (int player_id : players) {
        auto player = PlayerManager::getInstance().getPlayer(player_id);
        if (player) {
            player->setRoomId(room_id);
        }
    }

    return room_id;
}

std::shared_ptr<Room> RoomManager::getRoom(int room_id) {
    auto it = rooms_.find(room_id);
    if (it != rooms_.end()) {
        return it->second;
    }
    return nullptr;
}

void RoomManager::removeRoom(int room_id) {
    rooms_.erase(room_id);
}

void RoomManager::leaveRoom(int player_id, int room_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    auto it = rooms_.find(room_id);
    if (it == rooms_.end()) return;

    auto room = it->second;
    room->removePlayer(player_id);

    auto player = PlayerManager::getInstance().getPlayer(player_id);
    if (player) {
        player->setRoomId(-1);
    }

    if (room->empty()) {
        rooms_.erase(room_id);
    }
}

void RoomManager::sendRoomMsg(int room_id, const std::string& msg) {
    auto room = getRoom(room_id);
    if (room) {
        room->broadcast(msg);
    }
}