#ifndef MATCH_MANAGER_H
#define MATCH_MANAGER_H

#include <queue>
#include <mutex>
#include <vector>

class MatchManager {
public:
    static MatchManager& getInstance();

    // 玩家请求匹配
    std::vector<int> addPlayer(int player_id);

private:
    MatchManager() = default;

    std::queue<int> waiting_players_;
    std::mutex mutex_;
};

#endif