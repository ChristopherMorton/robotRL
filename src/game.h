#ifndef GAME_H__
#define GAME_H__

/* The game.cpp file is the home for everything involved in actual gameplay,
 * including level/enemy/stuff generation, persistence of everything,
 * and the entirety of the control scheme and menus once in-game.
 *
 * The app.cpp really just calls directly into playGame().
 */

#include <SFML/Window.hpp>
#include "structures.h"

void newGame();
void loadGame();

void testLevel();

int sendKeyToGame( sf::Keyboard::Key k, int mod=0 );

int playGame();
int displayGame();

// Interface to data

extern Level* current_level;

struct Player;
extern Player* player;

extern unsigned long int ticks; // 'ticks' since game started

int dropItem( Item *i );
int dropFromInventory( Item *i );

int moveUnit( Unit*, Direction );
int destroyUnit( Unit* target );

#endif
