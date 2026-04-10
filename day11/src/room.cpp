#include "room.h"
#include "player_manager.h"
#include <algorithm>

Room::Room(int room_id, const  std::vector<int>& players)
	: room_id_(room_id), players_(players){}

int Room::getId() const {
    return room_id_;
}

const std::vector<int>& Room::getPlayers() const {
    return players_;
}

void Room::addPlayer(int player_id) {
    players_.push_back(player_id);
}

void Room::removePlayer(int player_id) {
    players_.erase(
        std::remove(players_.begin(), players_.end(), player_id),
        players_.end()
    );
}

bool Room::empty() const {
    return players_.empty();
}

void Room::broadcast(const game::Packet& packet) {
    for (int player_id : players_) {
        PlayerManager::getInstance().sendToPlayer(player_id, packet);
    }
}