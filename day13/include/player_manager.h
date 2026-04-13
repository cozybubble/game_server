#ifndef PLAYER_MANAGER_H
#define PLAYER_MANAGER_H

#include <unordered_map>
#include <memory>
#include "player.h"
#include "connection.h"
#include <shared_mutex>

class PlayerManager{
public:
	static PlayerManager& getInstance();

	std::shared_ptr<Player> getPlayer(int id);
	void addPlayer(int id);
	void removePlayer(int id);
	void sendToPlayer(int player_id,const game::Packet& packet);
	void bindPlayer2Conn(int player_id, const std::shared_ptr<Connection>& conn);
private:
	PlayerManager() = default;

	std::unordered_map<int, std::shared_ptr<Player>> players_;
	std::unordered_map<int, std::weak_ptr<Connection>> id2player_;
	mutable std::shared_mutex mutex_;
};

#endif