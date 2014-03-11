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
}

RandomRobo::~RandomRobo() { }

int RandomRobo::takeTurn() {
   int x = rand() % 8;
   moveUnit( this, (Direction) x );
   return 1000;
}

AI::AI() {
   display_char = 'z';
   chassis = NULL;
   inventory = NULL;
   pos_x = 0;
   pos_y = 0;
   move_speed = 1000;
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
}

Player::~Player() { }

int Player::takeTurn() {
   return -1; // I am the player
}
