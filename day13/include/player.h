#ifndef PLAYER_H
#define PLAYER_H

class Player{
public:
	Player(int id);

	int getId() const;

	void setRoomId(int room_id) { room_id_ = room_id; }
    int getRoomId() const { return room_id_; }

private:
	int id_;
	int room_id_ = -1;
};

#endif