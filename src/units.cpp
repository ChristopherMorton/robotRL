#include "game.h"
#include "units.h"
#include "display.h"
#include "structures.h"
#include "items.h"
#include "syslog.h"

#include <cstdlib>
#include <sstream>

#include <SFML/Graphics.hpp>

Unit::Unit() {
   display_char = '_';
   alive = true;
   chassis = NULL;
   inventory = NULL;
   pos_x = 0;
   pos_y = 0;
   move_speed = 1000;
   vision_range = 5;
}

Unit::Unit( unsigned int d_c ) {
   display_char = d_c;
   alive = true;
   chassis = NULL;
   inventory = NULL;
}

Unit::~Unit() { }

void Unit::drawUnit( int x, int y ) {
   writeChar( display_char, sf::Color::White, sf::Color::Black, x, y );
}

int Unit::meleeAttack( Unit *target ) {
   Item *melee_stack = chassis->getAllMelee();

   if (melee_stack == NULL)
      return -1;

   std::stringstream txt1;
   txt1 << ">I attack " << target->getName();
   writeSystemLog( txt1.str() );

   while (melee_stack != NULL) {
      int result = melee_stack->meleeAttack( target );
      if (result == 1) // Target destroyed
         break;

      melee_stack = melee_stack->alt_next;
   }
   return 1000;
}

int Unit::destroyEquipment( Item *to_destroy )
{
   // TODO: Chance to add tech to inventory
   chassis->findAndRemoveItem( to_destroy );
   delete(to_destroy);

   return 0;
}

std::string Unit::getNamePadded( int num, char pad )
{
   std::string padding( num, pad );
   return padding + getName();
}

AI::AI() {
   display_char = 'z';
   alive = true;
   chassis = new BasicChassis();
   inventory = NULL;
   pos_x = 0;
   pos_y = 0;
   move_speed = 1000;
   vision_range = 5;

   behavior = WANDER;
   onmyteam = false;
   aggro = 0;
}

AI::~AI() { }

std::string AI::getName()
{
   return "AI Robot";
}

int AI::takeTurn() {
   if (!alive)
      return -1;

   if (behavior == WANDER) {
      int x = rand() % 8;
      moveUnit( this, (Direction) x );
   }

   return 1000;
};

Player::Player() {
   display_char = '@';
   alive = true;
   chassis = NULL;
   inventory = NULL;
   pos_x = 0;
   pos_y = 0;
   move_speed = 700;
   vision_range = 8;

   personal_name = "Robot Jones";
}

Player::~Player() { }

int Player::takeTurn() {
   return 0; // I am the player
}

std::string Player::getName()
{
   return personal_name;
}
