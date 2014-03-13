#include "game.h"
#include "display.h"
#include "shutdown.h"
#include "structures.h"
#include "items.h"
#include "units.h"
#include "log.h"

#include <vector>
#include <queue>
#include <stack>
#include <deque>
#include <sstream>
#include <cmath>

using namespace sf;

//////////////////////////////////////////////////////////////////////
// Persistent Data
//////////////////////////////////////////////////////////////////////

std::vector<Level*> all_levels;
Level* current_level = NULL;

Player* player;
Unit* current_unit;

Vector2u map_view_base; // The 0,0 of the view

bool waiting_for_input;

enum GameState
{
   ON_MAP,
   SELECTING_ABILITY,
   TARGETTING,
   TEXT_PAUSE,
   INVENTORY_SCREEN,
   INVENTORY_SELECT,
   EQUIP_SCREEN,
   EQUIP_INVENTORY,
   HELP_SCREEN
};

GameState game_state;

int selection, max_selection, menu_scroll, alt_selection, max_alt_selection; // Used in menus

unsigned long int ticks; // 'ticks' since game started

//////////////////////////////////////////////////////////////////////
// System Log
//////////////////////////////////////////////////////////////////////

const int system_log_width = 18;
const int system_log_memory = 100;
std::deque<std::string> system_log;
int system_log_scroll = 0;

void writeSystemLog( std::string text )
{
   if (text.size() > system_log_width)
      text.resize( system_log_width );
   system_log.push_front( text );
   system_log_scroll = 0;

   if (system_log.size() > system_log_memory)
      system_log.pop_back();
}

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

   return putUnit( unit, x_dest, y_dest );
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
// Player management
//////////////////////////////////////////////////////////////////////

// Inventory management

const int invPageSize = 25;

int getInventorySize()
{
   int size = 0;
   Item *i = player->inventory;
   while (i != NULL) {
      size++;
      i = i->next;
   }
   return size;
}
  
int addToInventory( Item *to_add )
{
   if (to_add == NULL) return -1;

   // Simple implementation
   to_add->next = player->inventory;
   player->inventory = to_add;
   return 0;
}

int removeFromInventory( Item *i )
{
   Item *prev = player->inventory;

   if (prev == NULL) return -1;

   if (prev == i) {
      player->inventory = prev->next;
      i->next = NULL;
      return 0;
   }

   while (prev != NULL) {
      if (prev->next == i) {
         prev->next = i->next; // removed
         i->next = NULL;
         return 0;
      }
      prev = prev->next;
   }

   return -2;
}

int dropItem( Item *i )
{
   Location &drop_point = current_level->map[player->pos_y][player->pos_x];
   i->next = drop_point.items;
   drop_point.items = i;

   writeSystemLog( ">Dropped:" );
   writeSystemLog( i->getName() );
   return 0;
}

int dropFromInventory( Item *i )
{
   if (removeFromInventory( i ) == 0) {
      dropItem( i );
      return 0;
   }

   log("Couldn't drop item, it was not in the inventory.");
   return -2;
}

int invSelectionDown()
{
   ++selection;
   if (selection == max_selection) {
      selection = 0;
      menu_scroll = 0;
   } else {
      if (selection - menu_scroll >= 24)
         ++menu_scroll;
   }
   return 0;
}

int invSelectionUp()
{
   --selection;
   if (selection == -1) {
      selection = max_selection - 1;
      menu_scroll = max_selection - 25;
   } else {
      if (selection - menu_scroll <= 1)
         --menu_scroll;
   }
   return 0;
}

int invSelectionPageDown()
{
   int old = selection;
   selection += invPageSize;
   if (selection >= max_selection)
      selection = max_selection - 1;

   menu_scroll += (selection - old);
   return 0;
}

int invSelectionPageUp()
{
   int old = selection;
   selection -= invPageSize;
   if (selection < 0)
      selection = 0;

   menu_scroll += (selection - old);
   return 0;
}

int drawInventory()
{
   const int column = 4, header_column = 2, column_end = 27;
   int row = 1;
   writeString( "Inventory:", Color::White, Color::Black, header_column, row );
   row++;

   Item* i = player->inventory;

   int count = 0; 
   while (count < menu_scroll && i != NULL) {
      count++;
      i = i->next;
   }

   while (i != NULL && row < 28) {
      writeString( i->getName(), Color::White, Color::Black, column, row );
      if (count == selection) {
         colorInvert( column, row, column_end, row );
         i->drawDescription();
         if (game_state == INVENTORY_SELECT) {
            i->drawActions();
            colorInvert( 33, 13+alt_selection, 57, 13+alt_selection );
         }
      }

      row++;
      count++;
      i = i->next;
   }
   return 0;
}

Item *invSelectedItem()
{
   Item *i = player->inventory;
   int count = 0;
   while (i != NULL) {
      if (count == selection)
         return i;

      count++;
      i = i->next;
   }
   return NULL;
}

Item *inv_selected_prev = NULL;

Item *invRemoveSelected()
{
   Item *i = player->inventory;
   int count = 0;
   if (selection == 0 && i != NULL) {
      player->inventory = i->next;
      i->next = NULL;
      return i;
   }
   
   inv_selected_prev = i;
   count++;
   i = i->next;

   while (i != NULL) {
      if (count == selection) {
         inv_selected_prev->next = i->next;
         i->next = NULL;
         return i;
      }

      inv_selected_prev = i;
      count++;
      i = i->next;
   }
   return NULL; 
}

void invReplaceSelected( Item *i )
{
   if (inv_selected_prev == NULL) {
      Item *i = player->inventory;
      int count = 0;
      while (i != NULL) {
         if (count == selection - 1) {
            inv_selected_prev = i;
            break;
         }

         count++;
         i = i->next;
      }
   }

   i->next = inv_selected_prev->next;
   inv_selected_prev->next = i;

   inv_selected_prev = NULL;
}

// Limited Inventory = sorted/filtered

Item *limited_inventory = NULL;

void buildLimitedInventory( ItemType type ) {
   Item *latest = player->inventory, *l_i_end = NULL;
   bool match = false;
   int count = 0;

   limited_inventory = NULL;

   while (latest != NULL) {
      match = false;
      switch (type) {
         case CHASSIS:
            if (latest->type == CHASSIS) match = true;
            break;
         case ARM:
            if (latest->type == ARM) match = true;
         case MOUNT:
            if (latest->type == MOUNT) match = true;
            break;
         case SYSTEM:
            if (latest->type == SYSTEM) match = true;
            break;
         case MISSILE:
            if (latest->type == MISSILE) match = true;
            break;
         case GRENADE:
            if (latest->type == GRENADE) match = true;
            break;
         case MINE:
            if (latest->type == MINE) match = true;
            break;
         case TURRET:
            if (latest->type == TURRET) match = true;
            break;
         case DEVICE:
            if (latest->type == DEVICE) match = true;
            break;
         case CODE:
            if (latest->type == CODE) match = true;
            break;
         case REMAINS:
            if (latest->type == REMAINS) match = true;
            break;
      }
      if (match) {
         if (limited_inventory == NULL)
            limited_inventory = latest;
         else
            l_i_end->alt_next = latest;

         l_i_end = latest;
         count++;
      }
      latest = latest->next;
   }

   max_selection = count;
   selection = 0;
}

Item *limitedInvSelectedItem()
{
   Item *i = limited_inventory;
   int count = 0;
   while (i != NULL) {
      if (count == selection)
         return i;

      count++;
      i = i->alt_next;
   }
   return NULL;
}

void drawLimitedInventory()
{
   const int column = 4, header_column = 2, column_end = 27;
   int row = 1;
   writeString( "Limited Inventory:", Color::White, Color::Black, header_column, row );
   row++;

   Item* i = limited_inventory;

   int count = 0; 
   while (count < menu_scroll && i != NULL) {
      count++;
      i = i->alt_next;
   }

   while (i != NULL && row < 28) {
      writeString( i->getName(), Color::White, Color::Black, column, row );
      if (count == selection) {
         colorInvert( column, row, column_end, row );
      }

      row++;
      count++;
      i = i->alt_next;
   }
}

// Interacting with the environment

void examineLocation()
{ 
   Location &player_loc = current_level->map[player->pos_y][player->pos_x];

   if (player_loc.items != NULL) {
      Item *i = player_loc.items;
      writeSystemLog( ">Items here:" );
      while( i != NULL ) {
         writeSystemLog( i->getName() );
         i = i->next;
      }
   }
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
      //int size = x_dim * y_dim;



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

   map_view_base = Vector2u( 10, 10 );

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

   player->chassis = new OrbChassis();
   player->chassis->addArm( new ClawArm() );
   player->chassis->addArm( new ClawArm() );

   for (int s = 0; s < 20; ++s)
      addToInventory( new ClawArm() );

   for (int r = 0; r < 20; ++r)
      addToInventory( new BasicChassis() );

   Unit* rr = new AI();
   putUnit( rr, 27, 28 );
   addUnitToQueue( rr, 500 );

   game_state = ON_MAP;
}

//////////////////////////////////////////////////////////////////////
// Main Interface
//////////////////////////////////////////////////////////////////////

std::stack<Keyboard::Key> input_stack;

int sendKeyToGame( Keyboard::Key k )
{
   if (k == Keyboard::Q)
      shutdown(1, 1);

   if (!waiting_for_input) {
      input_stack.push(k);
      return 1;
   }

   if (game_state == TEXT_PAUSE) {
      if (k == Keyboard::Space || k == Keyboard::Return) {
         // Continue
      }
      return 2;
   }
   
   if (game_state == SELECTING_ABILITY) {

   }
   
   if (game_state == INVENTORY_SCREEN) {
      if (k == Keyboard::I || k == Keyboard::Escape || k == Keyboard::BackSpace) {
         game_state = ON_MAP; } else
      if (k == Keyboard::Numpad2 || k == Keyboard::Down) {
         invSelectionDown(); } else
      if (k == Keyboard::Numpad8 || k == Keyboard::Up) {
         invSelectionUp(); } else
      if (k == Keyboard::PageDown) {
         invSelectionPageDown(); } else
      if (k == Keyboard::PageUp) {
         invSelectionPageUp(); } else
      if (k == Keyboard::Space || k == Keyboard::Return) {
         Item *selected = invSelectedItem();
         game_state = INVENTORY_SELECT;
         alt_selection = 0;
         max_alt_selection = selected->num_actions;
      }

      return 0;
   }

   if (game_state == INVENTORY_SELECT) {
      if (k == Keyboard::Escape || k == Keyboard::BackSpace) {
         game_state = INVENTORY_SCREEN; } else
      if (k == Keyboard::Numpad2 || k == Keyboard::Down) {
         if (++alt_selection == max_alt_selection) alt_selection = 0; } else
      if (k == Keyboard::Numpad8 || k == Keyboard::Up) {
         if (--alt_selection == -1) alt_selection = max_alt_selection - 1; } else
      if (k == Keyboard::Space || k == Keyboard::Return) {
         // Do that Item action
         Item *selected = invRemoveSelected();
         // Most actions require the item to be removed first
         if(selected != NULL) {
            int result = selected->doAction( alt_selection );
            if (result != 0) // Failure
               invReplaceSelected( selected );
         }
         game_state = ON_MAP;
      }


      return 0;
   }

   if (game_state == EQUIP_SCREEN) {
      if (k == Keyboard::E || k == Keyboard::Escape || k == Keyboard::BackSpace) {
         game_state = ON_MAP; } else 
      if (k == Keyboard::Numpad2 || k == Keyboard::Down) {
         if (++alt_selection == max_alt_selection) alt_selection = 0; } else
      if (k == Keyboard::Numpad8 || k == Keyboard::Up) {
         if (--alt_selection == -1) alt_selection = max_alt_selection - 1; } else
      if (k == Keyboard::Space || k == Keyboard::Return) {
         Item *retval = player->chassis->removeAny( alt_selection );
         if (retval == NULL) { // Selected <empty>
            game_state = EQUIP_INVENTORY;
            ItemType slot = player->chassis->getSlot( alt_selection );
            buildLimitedInventory( slot );

         } else {
            addToInventory( retval );
            writeSystemLog( ">Unequipped:" );
            writeSystemLog( retval->getName() );
         }
      }

      return 0;
   }

   if (game_state == EQUIP_INVENTORY) {
      if (k == Keyboard::Escape || k == Keyboard::BackSpace) {
         game_state = EQUIP_SCREEN; } else
      if (k == Keyboard::Numpad2 || k == Keyboard::Down) {
         invSelectionDown(); } else
      if (k == Keyboard::Numpad8 || k == Keyboard::Up) {
         invSelectionUp(); } else
      if (k == Keyboard::PageDown) {
         invSelectionPageDown(); } else
      if (k == Keyboard::PageUp) {
         invSelectionPageUp(); } else
      if (k == Keyboard::Space || k == Keyboard::Return) {
         Item *selected = limitedInvSelectedItem();
         if (selected != NULL) {
            ItemType slot = player->chassis->getSlot( alt_selection );
            removeFromInventory( selected );
            int result;
            if (slot == ARM) result = player->chassis->addArm( selected );
            else if (slot == MOUNT) result = player->chassis->addMount( selected );
            else if (slot == SYSTEM) result = player->chassis->addSystem( selected );

            if (result != 0)
               addToInventory( selected );
         }

         game_state = EQUIP_SCREEN;
      }


      return 0;
   }

   if (game_state == ON_MAP) {
      int speed = 0;

      if (k == Keyboard::E) {
         game_state = EQUIP_SCREEN;
         alt_selection = 0;
         max_alt_selection = player->chassis->getTotalSlots();
         return 0;
      }

      if (k == Keyboard::I) {
         game_state = INVENTORY_SCREEN;
         selection = 0;
         max_selection = getInventorySize();
         menu_scroll = 0;
         return 0;
      }

      // Movement
      if (k == Keyboard::Numpad1){
         int result = moveUnit(player,SOUTHWEST);
         if (result == 0) {
            speed=player->move_speed;
            examineLocation();
         } } else
      if (k == Keyboard::Numpad2){
         int result = moveUnit(player,SOUTH);
         if (result == 0) {
            speed=player->move_speed;
            examineLocation();
         } } else
      if (k == Keyboard::Numpad3){
         int result = moveUnit(player,SOUTHEAST);
         if (result == 0) {
            speed=player->move_speed;
            examineLocation();
         } } else
      if (k == Keyboard::Numpad4){
         int result = moveUnit(player,WEST);
         if (result == 0) {
            speed=player->move_speed;
            examineLocation();
         } } else
      if (k == Keyboard::Numpad6){
         int result = moveUnit(player,EAST);
         if (result == 0) {
            speed=player->move_speed;
            examineLocation();
         } } else
      if (k == Keyboard::Numpad7){
         int result = moveUnit(player,NORTHWEST);
         if (result == 0) {
            speed=player->move_speed;
            examineLocation();
         } } else
      if (k == Keyboard::Numpad8){
         int result = moveUnit(player,NORTH);
         if (result == 0) {
            speed=player->move_speed;
            examineLocation();
         } } else
      if (k == Keyboard::Numpad9){
         int result = moveUnit(player,NORTHEAST);
         if (result == 0) {
            speed=player->move_speed;
            examineLocation();
         } }

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
         Keyboard::Key next_input = input_stack.top();
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

//////////////////////////////////////////////////////////////////////
// Visuals
//////////////////////////////////////////////////////////////////////

// System log

void drawSystemLog()
{
   int y;
   for (y = 1; y < 27; ++y) {
      writeChar( '|', Color::White, Color::Black, 60, y );
      writeChar( '|', Color::White, Color::Black, 79, y );
   }
   writeString( "+------------------+", Color::White, Color::Black, 60, 0 );
   writeString( "System Log", Color::White, Color::Black, 61, 1 );
   writeString( "+------------------+", Color::White, Color::Black, 60, 2 );
   writeString( "+------------------+", Color::White, Color::Black, 60, 27 );

   // Log contents
   std::deque<std::string>::iterator log_it = system_log.begin(), log_end = system_log.end();
   for (int i = 0; i < system_log_scroll; ++i) {
      if (log_it == log_end)
         return;

      ++log_it;
   }

   for (y = 26; y > 2; --y) {
      if (log_it == log_end)
         return;

      writeString( *log_it, Color::White, Color::Black, 61, y );
      ++log_it;
   }
}

// HUD/BottomBar
   
void drawBottomBar()
{
   std::stringstream tick_string;
   tick_string << "Ticks: " << ticks;
   writeString( tick_string.str(), Color::White, Color::Black, 5, 29 );
}

// The rest

int displayGame()
{
   if (current_level == NULL) return -1;

   if (game_state == EQUIP_SCREEN) {
      Chassis *ch = player->chassis;
      if (ch) {
         ch->drawEquipScreen( alt_selection );
      }
   }
   else if (game_state == EQUIP_INVENTORY) {
      Chassis *ch = player->chassis;
      if (ch) {
         ch->listEquipment( alt_selection );
         drawLimitedInventory();
      }
   }
   else if (game_state == INVENTORY_SCREEN || game_state == INVENTORY_SELECT)
   {
      drawInventory();
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
                  writeChar( '.', Color::White, Color::Black, x, y );
               else if (l.ter == IMPASSABLE_WALL)
                  writeChar( ' ', Color::Black, Color::White, x, y );
               else if (l.ter >= STAIRS_UP_1 && l.ter <= STAIRS_UP_4)
                  writeChar( '<', Color::White, Color::Black, x, y );
               else if (l.ter >= STAIRS_DOWN_1 && l.ter <= STAIRS_DOWN_4)
                  writeChar( '>', Color::White, Color::Black, x, y );
            }

            if (visibility & MAP_SEEN && !(visibility & MAP_VISIBLE))
               dim( x, y, x, y );
         }
      }
   }

   drawSystemLog();
   drawBottomBar();

   drawDisplay();

   return 0;
}
