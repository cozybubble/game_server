#ifndef PLAYER_MANAGER_H
#define PLAYER_MANAGER_H

#include <unordered_map>
#include <memory>
#include "player.h"
#include "connection.h"

class PlayerManager{
public:
	static PlayerManager& getInstance();

	std::shared_ptr<Player> getPlayer(int id);
	void addPlayer(int id);
	void removePlayer(int id);
	void sendToPlayer(int player_id,std::string msg);
	void bindPlayer2Conn(int player_id, Connection *conn);
private:
	PlayerManager() = default;

	std::unordered_map<int, std::shared_ptr<Player>> players_;
	std::unordered_map<int, Connection*> id2player_;
};

#endif