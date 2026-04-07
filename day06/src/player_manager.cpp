#include "player_manager.h"

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
}