#ifndef ROOM_MANAGER_H
#define ROOM_MANAGER_H
#include <unordered_map>
#include <memory>
#include <mutex>
#include <vector>
#include "room.h"

class RoomManager {
public:
	static RoomManager& getInstance();

	int createRoom(const std::vector<int>& players);
	std::shared_ptr<Room> getRoom(int room_id);
	void removeRoom(int room_id);

	void leaveRoom(int player_id, int room_id);
    void sendRoomMsg(int room_id, const game::Packet& packet);

private:
	RoomManager() = default;

    int next_room_id_ = 1;
    std::unordered_map<int, std::shared_ptr<Room>> rooms_;
    std::mutex mutex_;
};

#endif