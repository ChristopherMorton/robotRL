#include "game.h"
#include "display.h"
#include "shutdown.h"
#include "structures.h"
#include "items.h"
#include "units.h"

#include <vector>
#include <queue>
#include <stack>
#include <sstream>

//////////////////////////////////////////////////////////////////////
// Persistent Data
//////////////////////////////////////////////////////////////////////

std::vector<Level*> all_levels;
Level* current_level = NULL;

Player* player;
Unit* current_unit;

sf::Vector2u map_view_base; // The 0,0 of the view

bool waiting_for_input;

unsigned long int ticks; // 'ticks' since game started

//////////////////////////////////////////////////////////////////////
// Time
//////////////////////////////////////////////////////////////////////

struct UnitTime
{
   unsigned long tick; 
   Unit* unit;

   UnitTime( unsigned long t, Unit* u ) { tick = t; unit = u;}
};

struct TimeQueueComparator
{
   bool operator() (const UnitTime &lhs, const UnitTime &rhs) {
      return lhs.tick > rhs.tick;
   }
};

typedef std::priority_queue<UnitTime,std::vector<UnitTime>,TimeQueueComparator> TimeQueue;

TimeQueue time_queue;

// Using the TimeQueue

int addUnitToQueue( Unit* unit, unsigned long ticks_from_now )
{
   if (unit == NULL || ticks_from_now < 0)
      return -1;

   time_queue.push( UnitTime( ticks_from_now + ticks, unit ) );
   return 0;
}

Unit *getNextUnit()
{
   UnitTime ut = time_queue.top();
   time_queue.pop();
   ticks = ut.tick;
   return (current_unit = ut.unit);
}

void clearCurrentUnit()
{
   current_unit = NULL;
   waiting_for_input = false;
}

//////////////////////////////////////////////////////////////////////
// Moving stuff around
//////////////////////////////////////////////////////////////////////

int putUnit( Unit* unit, int x, int y )
{
   Location &old_l = current_level->map[unit->pos_y][unit->pos_x];
   Location &new_l = current_level->map[y][x];
   if (new_l.unit == NULL && new_l.ter > IMPASSABLE_WALL) {
      // Go ahead and move
      old_l.unit = NULL;
      new_l.unit = unit;
      unit->pos_x = x;
      unit->pos_y = y;
      return 0;
   } else
      return -1;
}

int moveUnit( Unit* unit, Direction dir )
{
   if (unit == NULL) return -1;

   int x_dest = unit->pos_x, y_dest = unit->pos_y;

   // Find destination
   if (dir == NORTH) {
      y_dest--;
   } else if (dir == SOUTH) {
      y_dest++;
   } else if (dir == WEST) {
      x_dest--;
   } else if (dir == EAST) {
      x_dest++;
   } else if (dir == NORTHEAST) {
      y_dest--;
      x_dest++;
   } else if (dir == NORTHWEST) {
      y_dest--;
      x_dest--;
   } else if (dir == SOUTHWEST) {
      y_dest++;
      x_dest--;
   } else if (dir == SOUTHEAST) {
      y_dest++;
      x_dest++;
   }

   putUnit( unit, x_dest, y_dest );

   addUnitToQueue( unit, unit->move_speed );
   clearCurrentUnit();

   return 0;

}

//////////////////////////////////////////////////////////////////////
// Player control
//////////////////////////////////////////////////////////////////////



//////////////////////////////////////////////////////////////////////
// Game Generation/Loading
//////////////////////////////////////////////////////////////////////

void newGame()
{
}

void loadGame()
{
}

void testLevel()
{
   Level* tl = new Level( 100, 100 );
   current_level = tl;
   all_levels.push_back(tl);

   map_view_base = sf::Vector2u( 10, 10 );

   for (int i = 30; i <= 40; ++i) {
      tl->map[30][i].ter = IMPASSABLE_WALL;
   }

   player = new Player();
   putUnit( player, 25, 25 );
   current_unit = player;

   Unit* rr = new RandomRobo();
   putUnit( rr, 30, 20 );
   addUnitToQueue( rr, 500 );
}

//////////////////////////////////////////////////////////////////////
// Main Interface
//////////////////////////////////////////////////////////////////////

std::stack<sf::Keyboard::Key> input_stack;


#define PLAYERINPUT(key,action) if(k==key){if(current_unit!=player){input_stack.push(k);return 1;}else{action;}}

int sendKeyToGame( sf::Keyboard::Key k )
{
   if (k == sf::Keyboard::Q)
      shutdown(1, 1);

   // PLAYERINPUT macro will buffer the input while it isn't the player's turn

   // Movement
   PLAYERINPUT(sf::Keyboard::Numpad1,moveUnit(player,SOUTHWEST))
   else
   PLAYERINPUT(sf::Keyboard::Numpad2,moveUnit(player,SOUTH))
   else
   PLAYERINPUT(sf::Keyboard::Numpad3,moveUnit(player,SOUTHEAST))
   else
   PLAYERINPUT(sf::Keyboard::Numpad4,moveUnit(player,WEST))
   else
   PLAYERINPUT(sf::Keyboard::Numpad6,moveUnit(player,EAST))
   else
   PLAYERINPUT(sf::Keyboard::Numpad7,moveUnit(player,NORTHWEST))
   else
   PLAYERINPUT(sf::Keyboard::Numpad8,moveUnit(player,NORTH))
   else
   PLAYERINPUT(sf::Keyboard::Numpad9,moveUnit(player,NORTHEAST))

   return 0;
}

int playGame()
{
   if (waiting_for_input) { // wait for input
      while (!input_stack.empty() && waiting_for_input) {
         sf::Keyboard::Key next_input = input_stack.top();
         input_stack.pop();
         sendKeyToGame( next_input );
      }
   }
   else
   {
      if (current_unit == NULL)
         current_unit = getNextUnit();

      if (current_unit == player) {
         waiting_for_input = true;
         return 0;
      }

      // TODO: AI goes here
      current_unit->takeTurn();
   }
   return 0;
}

int displayGame()
{
   if (current_level == NULL) return -1;

   for (int x = 0; x < 80; x++) {
      for (int y = 0; y < 28; y++) {

         int map_x = x + map_view_base.x, map_y = y + map_view_base.y;
         if (map_x < 0 || map_x >= current_level->x_dim ||
               map_y < 0 || map_y >= current_level->y_dim)
            continue;

         Location l = current_level->map[map_y][map_x];

         if (l.unit != NULL) {
            l.unit->drawUnit(x, y);
         } else if (l.items != NULL) {
            l.items->drawItem(x, y);
         } else {

         if (l.ter == FLOOR)
            writeChar( '.', sf::Color::White, sf::Color::Black, x, y );
         else if (l.ter == IMPASSABLE_WALL)
            writeChar( ' ', sf::Color::Black, sf::Color::White, x, y );
         else if (l.ter >= STAIRS_UP_1 && l.ter <= STAIRS_UP_4)
            writeChar( '<', sf::Color::White, sf::Color::Black, x, y );
         else if (l.ter >= STAIRS_DOWN_1 && l.ter <= STAIRS_DOWN_4)
            writeChar( '>', sf::Color::White, sf::Color::Black, x, y );


         }
      }
   }

   std::stringstream tick_string;
   tick_string << "Ticks: " << ticks;
   writeString( tick_string.str(), sf::Color::White, sf::Color::Black, 5, 29 );

   drawDisplay();

   return 0;
}
