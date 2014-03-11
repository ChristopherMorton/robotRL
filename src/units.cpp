#include "game.h"
#include "units.h"
#include "display.h"
#include "structures.h"

#include <cstdlib>

#include <SFML/Graphics.hpp>

Unit::Unit() {
   display_char = '_';
   chassis = NULL;
   inventory = NULL;
   pos_x = 0;
   pos_y = 0;
   move_speed = 1000;
   vision_range = 5;
}

Unit::Unit( unsigned int d_c ) {
   display_char = d_c;
   chassis = NULL;
   inventory = NULL;
}

Unit::~Unit() { }

void Unit::drawUnit( int x, int y ) {
   writeChar( display_char, sf::Color::White, sf::Color::Black, x, y );
}

RandomRobo::RandomRobo() {
   display_char = '!';
   chassis = NULL;
   inventory = NULL;
   pos_x = 0;
   pos_y = 0;
   move_speed = 1000;
   vision_range = 5;
}

RandomRobo::~RandomRobo() { }

int RandomRobo::takeTurn() {
   int x = rand() % 8;
   moveUnit( this, (Direction) x );
   return move_speed;
}

AI::AI() {
   display_char = 'z';
   chassis = NULL;
   inventory = NULL;
   pos_x = 0;
   pos_y = 0;
   move_speed = 1000;
   vision_range = 5;
}

AI::~AI() { }

int AI::takeTurn() {
   // TODO...

   return 100;
};

Player::Player() {
   display_char = '@';
   chassis = NULL;
   inventory = NULL;
   pos_x = 0;
   pos_y = 0;
   move_speed = 700;
   vision_range = 5;
}

Player::~Player() { }

int Player::takeTurn() {
   return -1; // I am the player
}
