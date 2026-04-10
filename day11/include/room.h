#ifndef ROOM_H
#define ROOM_H

#include<vector>
#include<string>
#include "packet.pb.h"

class Room{
public:
	void broadcast(const game::Packet& packet);
	Room(int room_id, const std::vector<int>& players);
	
	int getId() const;
    const std::vector<int>& getPlayers() const;

    void addPlayer(int player_id);
    void removePlayer(int player_id);
    bool empty() const;
	
private:
	int room_id_;
	std::vector<int> players_;
};

#endif