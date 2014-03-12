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
#include <cmath>

//////////////////////////////////////////////////////////////////////
// Persistent Data
//////////////////////////////////////////////////////////////////////

std::vector<Level*> all_levels;
Level* current_level = NULL;

Player* player;
Unit* current_unit;

sf::Vector2u map_view_base; // The 0,0 of the view

bool waiting_for_input;

enum GameState
{
   ON_MAP,
   SELECTING_ABILITY,
   TARGETTING,
   TEXT_PAUSE,
   INVENTORY_SCREEN,
   EQUIP_SCREEN,
   EQUIP_INVENTORY,
   HELP_SCREEN
};

GameState game_state;

int selection, max_selection; // Used in menus

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

   return 0;

}

//////////////////////////////////////////////////////////////////////
// LOS
//////////////////////////////////////////////////////////////////////

typedef unsigned int uint;

static int multipliers[4][8] = {
    {1, 0, 0, -1, -1, 0, 0, 1},
    {0, 1, -1, 0, 0, -1, 1, 0},
    {0, 1, 1, 0, 0, -1, -1, 0},
    {1, 0, 0, 1, -1, 0, 0, -1}
};

void blankVision()
{
   int x_dim = current_level->x_dim, y_dim = current_level->y_dim;
   for (int i = 0; i < x_dim; ++i) {
      for (int j = 0; j < y_dim; ++j) {
         current_level->vision_map[j][i] &= ~MAP_VISIBLE;
      }
   }
}

void castLight(uint x, uint y, uint radius, uint row,
        float start_slope, float end_slope, uint xx, uint xy, uint yx,
        uint yy) {

    if (start_slope < end_slope) {
        return;
    }

    float next_start_slope = start_slope;
    for (uint i = row; i <= radius; i++) {
        bool blocked = false;
        for (int dx = -i, dy = -i; dx <= 0; dx++) {
            float l_slope = (dx - 0.5) / (dy + 0.5);
            float r_slope = (dx + 0.5) / (dy - 0.5);
            if (start_slope < r_slope) {
                continue;
            } else if (end_slope > l_slope) {
                break;
            }

            int sax = dx * xx + dy * xy;
            int say = dx * yx + dy * yy;
            if ((sax < 0 && (uint)std::abs(sax) > x) ||
                    (say < 0 && (uint)std::abs(say) > y)) {
                continue;
            }
            uint ax = x + sax;
            uint ay = y + say;
            if (ax >= current_level->x_dim || ay >= current_level->y_dim) {
                continue;
            }

            uint radius2 = radius * radius;
            if ((uint)(dx * dx + dy * dy) < radius2) {
               current_level->vision_map[ay][ax] |= MAP_VISIBLE | MAP_SEEN;
            }

            if (blocked) {
                if (current_level->map[ay][ax].ter <= IMPASSABLE_WALL) {
                    next_start_slope = r_slope;
                    continue;
                } else {
                    blocked = false;
                    start_slope = next_start_slope;
                }
            } else if (current_level->map[ay][ax].ter <= IMPASSABLE_WALL) {
                blocked = true;
                next_start_slope = r_slope;
                castLight(x, y, radius, i + 1, start_slope, l_slope, xx,
                        xy, yx, yy);
            }
        }
        if (blocked) {
            break;
        }
    }
}

void visionSource(uint x, uint y, uint radius) {
    for (uint i = 0; i < 8; i++) {
        castLight(x, y, radius, 1, 1.0, 0.0, multipliers[0][i],
                multipliers[1][i], multipliers[2][i], multipliers[3][i]);
    }
}

void doFOV()
{
   blankVision();
   visionSource( player->pos_x, player->pos_y, player->vision_range );
   current_level->vision_map[player->pos_y][player->pos_x] |= MAP_VISIBLE | MAP_SEEN;
}

//////////////////////////////////////////////////////////////////////
// Player control
//////////////////////////////////////////////////////////////////////
  
int addToInventory( Item *to_add )
{
   if (to_add == NULL) return -1;

   // Simple implementation
   to_add->next = player->inventory;
   player->inventory = to_add;
   return 0;
}

//////////////////////////////////////////////////////////////////////
// Level Gen
//////////////////////////////////////////////////////////////////////

enum LevelStyle {
   FACTORY_LEVEL,
   MINE_LEVEL,
   MILITARY_LEVEL
};

Level *generateRandomLevel( LevelStyle style, int difficulty, int x_dim, int y_dim, int up_stairs, int down_stairs, Level *preceding, bool entering_from_above )
{
   Level *newLevel = new Level( x_dim, y_dim );

   if (style == FACTORY_LEVEL) {
      int size = x_dim * y_dim;



   }

   return newLevel;
}

//////////////////////////////////////////////////////////////////////
// Game Creation/Loading
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

   for (int i = 20; i <= 30; ++i) {
      tl->map[18][i].ter = IMPASSABLE_WALL;
      tl->map[20][i].ter = IMPASSABLE_WALL;
      tl->map[i][20].ter = IMPASSABLE_WALL;
      tl->map[30][i].ter = IMPASSABLE_WALL;
      tl->map[i][30].ter = IMPASSABLE_WALL;
   }
   tl->map[20][26].ter = FLOOR;

   blankVision();

   player = new Player();
   putUnit( player, 25, 25 );
   current_unit = player;

   player->chassis = new BasicChassis();
   player->chassis->addArm( new ClawArm() );
   player->chassis->addArm( new ClawArm() );

   Unit* rr = new AI();
   putUnit( rr, 27, 28 );
   addUnitToQueue( rr, 500 );

   game_state = ON_MAP;
}

//////////////////////////////////////////////////////////////////////
// Main Interface
//////////////////////////////////////////////////////////////////////

std::stack<sf::Keyboard::Key> input_stack;

int sendKeyToGame( sf::Keyboard::Key k )
{
   if (k == sf::Keyboard::Q)
      shutdown(1, 1);

   if (!waiting_for_input) {
      input_stack.push(k);
      return 1;
   }

   if (game_state == TEXT_PAUSE) {
      if (k == sf::Keyboard::Space || k == sf::Keyboard::Return) {
         // Continue
      }
      return 2;
   }
   
   if (game_state == SELECTING_ABILITY) {

   }

   if (game_state == EQUIP_SCREEN) {
      if (k == sf::Keyboard::E || k == sf::Keyboard::Escape || k == sf::Keyboard::BackSpace) {
         game_state = ON_MAP; } else 
      if (k == sf::Keyboard::Numpad2 || k == sf::Keyboard::Down) {
         if (++selection == max_selection) selection = 0; } else
      if (k == sf::Keyboard::Numpad8 || k == sf::Keyboard::Up) {
         if (--selection == -1) selection = max_selection - 1; } else
      if (k == sf::Keyboard::Space || k == sf::Keyboard::Return) {
         Item *retval = player->chassis->removeAny( selection );
         if (retval == NULL) // Selected <empty>
            game_state = EQUIP_INVENTORY;
         else
            addToInventory( retval );
      }

      return 0;
   }

   if (game_state == ON_MAP) {
      int speed = 0;

      if (k == sf::Keyboard::E) {
         game_state = EQUIP_SCREEN;
         selection = 0;
         max_selection = player->chassis->getTotalSlots();
         return 0;
      }

      // Movement
      if (k == sf::Keyboard::Numpad1){
         moveUnit(player,SOUTHWEST);speed=player->move_speed;} else
      if (k == sf::Keyboard::Numpad2){
         moveUnit(player,SOUTH);speed=player->move_speed;} else
      if (k == sf::Keyboard::Numpad3){
         moveUnit(player,SOUTHEAST);speed=player->move_speed;} else
      if (k == sf::Keyboard::Numpad4){
         moveUnit(player,WEST);speed=player->move_speed;} else
      if (k == sf::Keyboard::Numpad6){
         moveUnit(player,EAST);speed=player->move_speed;} else
      if (k == sf::Keyboard::Numpad7){
         moveUnit(player,NORTHWEST);speed=player->move_speed;} else
      if (k == sf::Keyboard::Numpad8){
         moveUnit(player,NORTH);speed=player->move_speed;} else
      if (k == sf::Keyboard::Numpad9){
         moveUnit(player,NORTHEAST);speed=player->move_speed;}

      addUnitToQueue( player, speed );
      clearCurrentUnit();
      return 0;
   }


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
      int speed = current_unit->takeTurn();
      addUnitToQueue( current_unit, speed );
      clearCurrentUnit();

   }
   return 0;
}

void drawSidebar()
{
   int y;
   for (y = 1; y < 27; ++y) {
      writeChar( '|', sf::Color::White, sf::Color::Black, 60, y );
      writeChar( '|', sf::Color::White, sf::Color::Black, 79, y );
   }
   writeString( "+------------------+", sf::Color::White, sf::Color::Black, 60, 0 );
   writeString( "System Log", sf::Color::White, sf::Color::Black, 61, 1 );
   writeString( "+------------------+", sf::Color::White, sf::Color::Black, 60, 2 );
   writeString( "+------------------+", sf::Color::White, sf::Color::Black, 60, 27 );
}
   
void drawBottomBar()
{
   std::stringstream tick_string;
   tick_string << "Ticks: " << ticks;
   writeString( tick_string.str(), sf::Color::White, sf::Color::Black, 5, 29 );
}

int displayGame()
{
   if (current_level == NULL) return -1;

   if (game_state == EQUIP_SCREEN) {
      Chassis *ch = player->chassis;
      if (ch) {
         ch->drawEquipScreen( selection );
      }
   }
   else
   {

      doFOV();

      for (int x = 0; x < 60; x++) {
         for (int y = 0; y < 28; y++) {

            int map_x = x + map_view_base.x, map_y = y + map_view_base.y;
            if (map_x < 0 || map_x >= current_level->x_dim ||
                  map_y < 0 || map_y >= current_level->y_dim)
               continue;

            int visibility = current_level->vision_map[map_y][map_x];
            if (visibility == 0)
               continue;

            Location l = current_level->map[map_y][map_x];

            if (l.unit != NULL && visibility & MAP_VISIBLE) {
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

            if (visibility & MAP_SEEN && !(visibility & MAP_VISIBLE))
               dim( x, y, x, y );
         }
      }
   }

   drawSidebar();
   drawBottomBar();

   drawDisplay();

   return 0;
}
