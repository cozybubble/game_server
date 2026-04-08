#ifndef PLAYER_H
#define PLAYER_H

class Player{
public:
	Player(int id);

	int getId() const;

private:
	int id_;
};

#endif