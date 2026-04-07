#ifndef PLAYER_MANAGER_H
#define PLAYER_MANAGER_H

#include <unordered_map>
#include <memory>
#include "player.h"

class PlayerManager{
public:
	static PlayerManager& getInstance();

	std::shared_ptr<Player> getPlayer(int id);
	void addPlayer(int id);
	void removePlayer(int id);

private:
	PlayerManager() = default;

	std::unordered_map<int, std::shared_ptr<Player>> players_;
};

#endif