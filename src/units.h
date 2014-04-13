#ifndef UNITS_H__
#define UNITS_H__

#include "items.h"

struct Unit
{
   unsigned int display_char;

   bool alive;

   int pos_x, pos_y;

   int move_speed;
   int vision_range;

   Chassis *chassis;
   Item *inventory;

   Unit();
   Unit( unsigned int d_c );
   virtual ~Unit();

   void drawUnit( int x, int y );

   int meleeAttack( Unit *target );
   //int rangedAttack( Unit *target );

   int destroyEquipment( Item *to_destroy );

   virtual int takeTurn() = 0;
   virtual std::string getName() = 0;
   std::string getNamePadded( int num=1, char pad=' ' );
};

enum AIBehavior {
   IDLE,
   WANDER,
   PATROL,
   ATTACK_ENEMIES,
   RUN_FROM_ENEMIES
};

struct AI : public Unit
{
   AIBehavior behavior;
   bool onmyteam;
   int aggro;

   AI();
   virtual ~AI();

   virtual int takeTurn();
   virtual std::string getName();
};

struct Player : public Unit
{
   std::string personal_name;

   Player();
   virtual ~Player();

   virtual int takeTurn();
   virtual std::string getName();
};

#endif
