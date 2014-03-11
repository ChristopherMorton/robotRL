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

struct RandomRobo : public Unit
{
   RandomRobo();
   virtual ~RandomRobo();

   virtual int takeTurn();
};

struct AI : public Unit
{
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
