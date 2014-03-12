#ifndef UNITS_H__
#define UNITS_H__

#include "items.h"

struct Unit
{
   unsigned int display_char;

   int pos_x, pos_y;

   int move_speed;
   int vision_range;

   Chassis *chassis;
   Item *inventory;

   Unit();
   Unit( unsigned int d_c );
   virtual ~Unit();

   void drawUnit( int x, int y );

   virtual int takeTurn() = 0;
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
};

struct Player : public Unit
{
   Player();
   virtual ~Player();

   virtual int takeTurn();
};

#endif
