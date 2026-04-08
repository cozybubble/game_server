#include "player_manager.h"
#include "connection.h"
#include <shared_mutex>
PlayerManager& PlayerManager::getInstance() {
    static PlayerManager instance;
    return instance;
}

std::shared_ptr<Player> PlayerManager::getPlayer(int id) {
	std::shared_lock<std::shared_mutex> lock(mutex_); // 加锁
    auto it = players_.find(id);
    if (it != players_.end()) {
        return it->second;
    }
    return nullptr;
}

void PlayerManager::addPlayer(int id) {
	std::unique_lock<std::shared_mutex> lock(mutex_); // 加锁
    players_[id] = std::make_shared<Player>(id);
}

void PlayerManager::removePlayer(int id) {
	std::unique_lock<std::shared_mutex> lock(mutex_); // 加锁
    players_.erase(id);
    id2player_.erase(id);
}

void PlayerManager::sendToPlayer(int player_id, std::string msg){
	std::shared_ptr<Connection> conn;
	{
		std::shared_lock<std::shared_mutex> lock(mutex_); // 加锁
		auto it = id2player_.find(player_id);
        if (it != id2player_.end()) {
            conn = it->second.lock();
            if (!conn) {
                id2player_.erase(it);
            }
        }
	}
	
    if (conn) {
        conn->sendMessage(msg);
    }
}

void PlayerManager::bindPlayer2Conn(int player_id, const std::shared_ptr<Connection>& conn){
	std::unique_lock<std::shared_mutex> lock(mutex_); // 加锁
	id2player_[player_id] = conn;
}
