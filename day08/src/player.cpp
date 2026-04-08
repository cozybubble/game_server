#include "player.h"

Player::Player(int id) : id_(id) {}

int Player::getId() const {
    return id_;
}