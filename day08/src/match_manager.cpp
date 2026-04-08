#include "match_manager.h"

MatchManager& MatchManager::getInstance() {
    static MatchManager instance;
    return instance;
}

std::vector<int> MatchManager::addPlayer(int player_id) {
    std::lock_guard<std::mutex> lock(mutex_);

    waiting_players_.push(player_id);

    if (waiting_players_.size() >= 2) {
        int p1 = waiting_players_.front(); waiting_players_.pop();
        int p2 = waiting_players_.front(); waiting_players_.pop();

        return {p1, p2};
    }

    return {};
}