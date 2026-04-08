#include "player_manager.h"
#include "connection.h"
PlayerManager& PlayerManager::getInstance() {
    static PlayerManager instance;
    return instance;
}

std::shared_ptr<Player> PlayerManager::getPlayer(int id) {
    auto it = players_.find(id);
    if (it != players_.end()) {
        return it->second;
    }
    return nullptr;
}

void PlayerManager::addPlayer(int id) {
    players_[id] = std::make_shared<Player>(id);
}

void PlayerManager::removePlayer(int id) {
    players_.erase(id);
    id2player_.erase(id);
}

void PlayerManager::sendToPlayer(int player_id, std::string msg){
	auto conn = id2player_[player_id];
    if (conn) {
        conn->sendMessage(msg);
    }
}

void PlayerManager::bindPlayer2Conn(int player_id, Connection *conn){
	id2player_[player_id] = conn;
}
