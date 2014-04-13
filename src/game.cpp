#include "game.h"
#include "display.h"
#include "shutdown.h"
#include "structures.h"
#include "items.h"
#include "units.h"
#include "log.h"
#include "defs.h"
#include "SFML_GlobalRenderWindow.hpp"

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
   PICK_UP,
   HELP_SCREEN
};

GameState game_state;

int selection, max_selection, menu_scroll, alt_selection, max_alt_selection; // Used in menus

unsigned long int ticks; // 'ticks' since game started

//////////////////////////////////////////////////////////////////////
// System Log
//////////////////////////////////////////////////////////////////////

const int system_log_width = 23;
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

void clearSystemLog()
{
   system_log.clear();
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
   if (new_l.unit != NULL) // Unit in the way
      return -2;

   if (new_l.ter <= IMPASSABLE_WALL) // Can't go there
      return -1;
   
   // Go ahead and move
   old_l.unit = NULL;
   new_l.unit = unit;
   unit->pos_x = x;
   unit->pos_y = y;
   return 0;
}

void addDirection( Direction dir, Vector2u &pos )
{
   if (dir == NORTH) {
      pos.y--;
   } else if (dir == SOUTH) {
      pos.y++;
   } else if (dir == WEST) {
      pos.x--;
   } else if (dir == EAST) {
      pos.x++;
   } else if (dir == NORTHEAST) {
      pos.y--;
      pos.x++;
   } else if (dir == NORTHWEST) {
      pos.y--;
      pos.x--;
   } else if (dir == SOUTHWEST) {
      pos.y++;
      pos.x--;
   } else if (dir == SOUTHEAST) {
      pos.y++;
      pos.x++;
   }
}

void keepInBounds( Vector2u &pos )
{
   if (pos.x < 0) pos.x = 0;
   if (pos.x >= current_level->x_dim) pos.x = current_level->x_dim - 1;

   if (pos.y < 0) pos.y = 0;
   if (pos.y >= current_level->y_dim) pos.y = current_level->y_dim - 1;
}

int moveUnit( Unit* unit, Direction dir )
{
   if (unit == NULL) return -1;

   Vector2u dest( unit->pos_x, unit->pos_y );
   addDirection( dir, dest );
   keepInBounds( dest );

   return putUnit( unit, dest.x, dest.y );
}

int destroyUnit( Unit* target )
{
   std::stringstream txt;
   txt << "!" << target->getName() << " destroyed!";
   writeSystemLog( txt.str() );

   Location &l = current_level->map[target->pos_y][target->pos_x];

   Item *drops = target->inventory;
   Item *it = drops;
   if (it != NULL) {
      while (it->next != NULL)
         it = it->next;

      it->next = l.items;
      l.items = drops;
   }

   l.unit = NULL;
   target->alive = false;

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
   writeSystemLog( i->getNamePadded() );
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

int selectionDown()
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

int selectionUp()
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

int selectionPageDown()
{
   int old = selection;
   selection += invPageSize;
   if (selection >= max_selection)
      selection = max_selection - 1;

   menu_scroll += (selection - old);
   return 0;
}

int selectionPageUp()
{
   int old = selection;
   selection -= invPageSize;
   if (selection < 0)
      selection = 0;

   menu_scroll += (selection - old);
   if (menu_scroll < 0) menu_scroll = 0;

   return 0;
}

int drawItemStack( Item *first, std::string header, bool alt=false, bool describe=true )
{
   const int column = 4, header_column = 2, column_end = 23;
   int row = 1;
   writeString( header, C_WHITE, C_BLACK, header_column, row );
   row++;

   Item* i = first;

   int count = 0; 
   while (count < menu_scroll && i != NULL) {
      count++;
      if (alt)
         i = i->alt_next;
      else
         i = i->next;
   }

   char alpha = 'a';
   while (i != NULL && row < 28) {
      writeChar( alpha, C_GRAY, C_BLACK, 2, row );
      writeChar( ')', C_GRAY, C_BLACK, 3, row );
      alpha++;

      writeString( i->getName(), C_WHITE, C_BLACK, column, row );
      if (count == selection) {
         colorInvert( column, row, column_end, row );
         if (describe) {
            i->drawDescription();
            if (game_state == INVENTORY_SELECT) {
               i->drawActions();
               colorInvert( actions_column, 13+alt_selection, actions_column+19, 13+alt_selection );
            }
         }
      }

      row++;
      count++;
      if (alt)
         i = i->alt_next;
      else
         i = i->next;
   }
   return 0;
}

int drawInventory()
{
   return drawItemStack( player->inventory, "Inventory:" );
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
   if (selection == 0) {
      i->next = player->inventory;
      player->inventory = i;
      return;
   }
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

   if (l_i_end)
      l_i_end->alt_next = NULL;

   max_selection = count;
   selection = 0;
}

Item *invGetAllUsable()
{
   Item *first = NULL, *last = NULL, *it = player->inventory;

   while (it != NULL) {
      if (it->weapon_type == USABLE_WEAPON) {
         if (first != NULL)
            first = it;
         else {
            last->alt_next = it;
            last = it;
         }
         it->alt_next = NULL;
      }
      it = it->next;
   }
   return first;
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

int drawLimitedInventory()
{
   return drawItemStack( limited_inventory, "Limited Inventory:", true, false );
}

// Another set of Item manipulations, for a generic stack

Item *item_stack = NULL, *stack_selected_prev = NULL;

Item* stackRemoveSelected()
{
   Item *i = item_stack;
   int count = 0;
   if (selection == 0 && i != NULL) {
      item_stack = i->next;
      i->next = NULL;
      return i;
   }
   
   stack_selected_prev = i;
   count++;
   i = i->next;

   while (i != NULL) {
      if (count == selection) {
         stack_selected_prev->next = i->next;
         i->next = NULL;
         return i;
      }

      stack_selected_prev = i;
      count++;
      i = i->next;
   }
   return NULL;
}

void stackReplaceSelected( Item *i )
{
   if (selection == 0) {
      i->next = item_stack;
      item_stack = i;
      return;
   }
   if (stack_selected_prev == NULL) {
      Item *i = item_stack;
      int count = 0;
      while (i != NULL) {
         if (count == selection - 1) {
            stack_selected_prev = i;
            break;
         }

         count++;
         i = i->next;
      }
   }

   i->next = stack_selected_prev->next;
   stack_selected_prev->next = i;

   stack_selected_prev = NULL;
}

// Interacting with the environment

void examineLocation()
{ 
   Location &player_loc = current_level->map[player->pos_y][player->pos_x];

   if (player_loc.items != NULL) {
      Item *i = player_loc.items;
      writeSystemLog( ">Items here:" );
      while( i != NULL ) {
         writeSystemLog( i->getNamePadded() );
         i = i->next;
      }
   }
}

// Movement

int movePlayer( Direction dir )
{
   int result = moveUnit( player, dir );
   if (result == 0) {
      examineLocation();
      return player->move_speed;
   } 
   else if (result == -1) { // Can't move there
      return 0;
   }
   else if (result == -2) { // Unit in the way
      // Melee attack enemy unit
      Vector2u t_loc ( player->pos_x, player->pos_y );
      addDirection( dir, t_loc );
      Unit *target = current_level->map[t_loc.y][t_loc.x].unit;

      if (target == NULL) // Impossible?
         return 0;

      return player->meleeAttack( target );
   }

   return -1;
}

// Targetting

Vector2u reticle;

int analyzeTarget( bool print=true )
{
   Unit *target = current_level->map[reticle.y][reticle.x].unit;

   if (target == NULL || target == player)
      return 0;

   if (print) {
      std::stringstream ss;
      ss << " ::" << target->getName();
      writeSystemLog( ss.str() );
   }
   return 1;
}

int targetMoveSelector( Direction dir )
{
   addDirection( dir, reticle );
   keepInBounds( reticle );
   analyzeTarget();
   return 0;
}

int targetSelectNext()
{
   int x_start = reticle.x, y_start = reticle.y;

   while( reticle.y < current_level->y_dim) {
      while( reticle.x < current_level->x_dim) {
         reticle.x++;
         if ((current_level->vision_map[reticle.y][reticle.x] & MAP_VISIBLE) &&
             (current_level->map[reticle.y][reticle.x].unit != NULL)) {
            if (analyzeTarget())
               return 0;
         }
      }
      reticle.x = 0;
      reticle.y++;
   }

   // Try again from the beginning
   reticle.x = 0;
   reticle.y = 0;

   while( reticle.y < current_level->y_dim) {
      while( reticle.x < current_level->x_dim) {
         reticle.x++;
         if ((current_level->vision_map[reticle.y][reticle.x] & MAP_VISIBLE) &&
             (current_level->map[reticle.y][reticle.x].unit != NULL)) {
            if (analyzeTarget())
               return 0;
         }
      }
      reticle.x = 0;
      reticle.y++;
   }

   reticle.x = x_start;
   reticle.y = y_start;
   return -1;
}

int targetSelectPrevious()
{
   int x_start = reticle.x, y_start = reticle.y;

   while( reticle.y >= 0 ) {
      while( reticle.x >= 0 ) {
         reticle.x--;
         if ((current_level->vision_map[reticle.y][reticle.x] & MAP_VISIBLE) &&
             (current_level->map[reticle.y][reticle.x].unit != NULL)) {
            if (analyzeTarget())
               return 0;
         }
      }
      reticle.x = current_level->x_dim - 1;
      reticle.y--;
   }

   // Try again from the beginning
   reticle.x = current_level->x_dim - 1;
   reticle.y = current_level->y_dim - 1;

   while( reticle.y >= 0 ) {
      while( reticle.x >= 0 ) {
         reticle.x--;
         if ((current_level->vision_map[reticle.y][reticle.x] & MAP_VISIBLE) &&
             (current_level->map[reticle.y][reticle.x].unit != NULL)) {
            if (analyzeTarget())
               return 0;
         }
      }
      reticle.x = current_level->x_dim - 1;
      reticle.y--;
   }

   reticle.x = x_start;
   reticle.y = y_start;
   return -1;
}


// Using tactical things

/* Things you could be selecting for:
 * - Ranged attack (F)
 * - System usage (S)
 * - Usable item (U)
 */

Item *usable_stack = NULL;
int usable_count = 0;
WeaponType usable_type;

void initTacticalStack()
{
   max_selection = 0;
   selection = 0;

   Item *item = usable_stack;
   if (usable_stack == NULL)
      writeSystemLog( "Usable Stack is empty" );

   while( item != NULL) {
      max_selection++;
      item = item->alt_next;
   }
   std::stringstream ss;
   ss << "Max selection: " << max_selection;
   writeSystemLog( ss.str() );
}

int initTacticalSelection( WeaponType type )
{
   usable_type = type;
   if (type == RANGED_WEAPON) {
      usable_stack = player->chassis->getAllRanged();

   } else if (type == TACTICAL_WEAPON) {
      usable_stack = player->chassis->getAllTactical();

   } else if (type == USABLE_WEAPON) {
      usable_stack = invGetAllUsable();

   }

   if (usable_stack == NULL)
      return -1;

   initTacticalStack();
   return 0;
}

int use()
{
   Item *use_item = usable_stack;
   for (int i = 0; i < selection && use_item; ++i)
      use_item = use_item->alt_next;

   if (use_item == NULL)
      return -1;
   
   if (use_item->targetted == true)
      return 1; // Go to targetting

   writeSystemLog( "TODO: Using item" );

   return 0;
}

int fire()
{
   Item *use_item = usable_stack;
   for (int i = 0; i < selection && use_item; ++i)
      use_item = use_item->alt_next;

   if (use_item == NULL)
      return -1;

   Unit *target = current_level->map[reticle.y][reticle.x].unit;
   if (use_item->weapon_type == RANGED_WEAPON) {
      use_item->rangedAttack( target );
      return 0;
   }
   if (use_item->weapon_type == TACTICAL_WEAPON) {
      //use_item->tactical( target );
      return 0;
   }

   return 0;
}

int drawUsableInventory()
{
   if (usable_type == RANGED_WEAPON)
      return drawItemStack( usable_stack, "Ranged Weaponry:", true );
   if (usable_type == TACTICAL_WEAPON)
      return drawItemStack( usable_stack, "Tactical Weaponry:", true );
   if (usable_type == USABLE_WEAPON)
      return drawItemStack( usable_stack, "Usable Items:", true );

   return -1;
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

   player->chassis = new BasicChassis();
   player->chassis->addArm( new ClawArm() );
   player->chassis->addArm( new Laser() );

   for (int s = 0; s < 20; ++s)
      addToInventory( new ClawArm() );

   for (int r = 0; r < 20; ++r)
      addToInventory( new HammerArm() );

   addToInventory( new Laser() );

   Unit* rr = new AI();
   putUnit( rr, 27, 28 );
   addUnitToQueue( rr, 500 );
   rr->inventory = new EnergyLance();

   game_state = ON_MAP;
   clearSystemLog();
}

//////////////////////////////////////////////////////////////////////
// Main Interface
//////////////////////////////////////////////////////////////////////

struct KeyInput {
   Keyboard::Key k;
   int mod;

   KeyInput( Keyboard::Key kk ) {k = kk; mod = 0;}
   KeyInput( Keyboard::Key kk, int m ) {k = kk; mod = m;}
};

std::stack<KeyInput> input_stack;

bool alphaSelect( Keyboard::Key k, int scroll_offset, bool alt )
{
   int new_select = -1, max_select = max_selection;
   if (alt)
      max_select = max_alt_selection;

   max_select += scroll_offset;

   if (k == Keyboard::A && max_select >= 1) {
      new_select = 0; } else
   if (k == Keyboard::B && max_select >= 2) {
      new_select = 1; } else
   if (k == Keyboard::C && max_select >= 3) {
      new_select = 2; } else
   if (k == Keyboard::D && max_select >= 4) {
      new_select = 3; } else
   if (k == Keyboard::E && max_select >= 5) {
      new_select = 4; } else
   if (k == Keyboard::F && max_select >= 6) {
      new_select = 5; } else
   if (k == Keyboard::G && max_select >= 7) {
      new_select = 6; } else
   if (k == Keyboard::H && max_select >= 8) {
      new_select = 7; } else
   if (k == Keyboard::I && max_select >= 9) {
      new_select = 8; } else
   if (k == Keyboard::J && max_select >= 10) {
      new_select = 9; } else
   if (k == Keyboard::K && max_select >= 11) {
      new_select = 10; } else
   if (k == Keyboard::L && max_select >= 12) {
      new_select = 11; } else
   if (k == Keyboard::M && max_select >= 13) {
      new_select = 12; } else
   if (k == Keyboard::N && max_select >= 14) {
      new_select = 13; } else
   if (k == Keyboard::O && max_select >= 15) {
      new_select = 14; } else
   if (k == Keyboard::P && max_select >= 16) {
      new_select = 15; } else
   if (k == Keyboard::Q && max_select >= 17) {
      new_select = 16; } else
   if (k == Keyboard::R && max_select >= 18) {
      new_select = 17; } else
   if (k == Keyboard::S && max_select >= 19) {
      new_select = 18; } else
   if (k == Keyboard::T && max_select >= 20) {
      new_select = 19; } else
   if (k == Keyboard::U && max_select >= 21) {
      new_select = 20; } else
   if (k == Keyboard::V && max_select >= 22) {
      new_select = 21; } else
   if (k == Keyboard::W && max_select >= 23) {
      new_select = 22; } else
   if (k == Keyboard::X && max_select >= 24) {
      new_select = 23; } else
   if (k == Keyboard::Y && max_select >= 25) {
      new_select = 24; } else
   if (k == Keyboard::Z && max_select >= 26) {
      new_select = 25; }

   if (new_select != -1) {
      new_select += scroll_offset;

      if (alt)
         alt_selection = new_select;
      else
         selection = new_select;

      return true;
   }
   else
      return false;
}

int sendKeyToGame( Keyboard::Key k, int mod )
{
   if (k == Keyboard::Q && (mod & MOD_SHIFT))
      shutdown(1, 1);

   if (!waiting_for_input) {
      input_stack.push(KeyInput(k, mod));
      return 1;
   }

   if (game_state == TEXT_PAUSE) {
      if (k == Keyboard::Space || k == Keyboard::Return) {
         // Continue
      }
      return 2;
   }
   
   if (game_state == SELECTING_ABILITY) {
      if (k == Keyboard::Escape || k == Keyboard::BackSpace) {
         game_state = ON_MAP;
      } else
      if (k == Keyboard::Numpad2 || k == Keyboard::Down) {
         selectionDown(); } else
      if (k == Keyboard::Numpad8 || k == Keyboard::Up) {
         selectionUp(); } else
      if (k == Keyboard::PageDown) {
         selectionPageDown(); } else
      if (k == Keyboard::PageUp) {
         selectionPageUp(); } else
      if (k == Keyboard::Space || k == Keyboard::Return) {
         if (use() == 1) {
            game_state = TARGETTING;
            targetSelectNext();
         } else {
            game_state = ON_MAP;
         }
      }

      return 0;
   }

   if (game_state == TARGETTING) {
      if (k == Keyboard::Escape || k == Keyboard::BackSpace) {
         game_state = SELECTING_ABILITY;
      } else
      if (k == Keyboard::Numpad1){
         targetMoveSelector( SOUTHWEST ); } else
      if (k == Keyboard::Numpad2 || k == Keyboard::Down){
         targetMoveSelector( SOUTH ); } else
      if (k == Keyboard::Numpad3){
         targetMoveSelector( SOUTHEAST ); } else
      if (k == Keyboard::Numpad4 || k == Keyboard::Left){
         targetMoveSelector( WEST ); } else
      if (k == Keyboard::Numpad6 || k == Keyboard::Right){
         targetMoveSelector( EAST ); } else
      if (k == Keyboard::Numpad7){
         targetMoveSelector( NORTHWEST ); } else
      if (k == Keyboard::Numpad8 || k == Keyboard::Up){
         targetMoveSelector( NORTH ); } else
      if (k == Keyboard::Numpad9){
         targetMoveSelector( NORTHEAST ); } else
      if (k == Keyboard::N || k == Keyboard::PageDown) {
         targetSelectNext(); } else
      if (k == Keyboard::P || k == Keyboard::PageUp) {
         targetSelectPrevious(); } else
      if (k == Keyboard::Space || k == Keyboard::Return) {
         if (analyzeTarget(false)) {
            fire();
            game_state = ON_MAP;
         } else {
            writeSystemLog( ">ERROR: No target selected" );
         }
      }

      return 0;
   }
   
   if (game_state == PICK_UP) {
      if (k == Keyboard::Escape || k == Keyboard::BackSpace) {
         current_level->map[player->pos_y][player->pos_x].items = item_stack;
         game_state = ON_MAP;
      } else
      if (k == Keyboard::Numpad2 || k == Keyboard::Down) {
         selectionDown(); } else
      if (k == Keyboard::Numpad8 || k == Keyboard::Up) {
         selectionUp(); } else
      if (k == Keyboard::PageDown) {
         selectionPageDown(); } else
      if (k == Keyboard::PageUp) {
         selectionPageUp(); } else
      if (k == Keyboard::Space || k == Keyboard::Return) {
         Item *selected = stackRemoveSelected();
         if (selected != NULL) {
            selected->next = NULL;
            addToInventory( selected );
            max_selection--;
            if (selection == max_selection) selection--;
            writeSystemLog( selected->getNamePadded() );
         }
         if (item_stack == NULL) {
            current_level->map[player->pos_y][player->pos_x].items = item_stack;
            game_state = ON_MAP;
         }
      }

      return 0;
   }
   
   if (game_state == INVENTORY_SCREEN) {
      if (alphaSelect( k, menu_scroll, false ))
         return 0;

      if (k == Keyboard::Escape || k == Keyboard::BackSpace) {
         game_state = ON_MAP; } else
      if (k == Keyboard::Numpad2 || k == Keyboard::Down) {
         selectionDown(); } else
      if (k == Keyboard::Numpad8 || k == Keyboard::Up) {
         selectionUp(); } else
      if (k == Keyboard::PageDown) {
         selectionPageDown(); } else
      if (k == Keyboard::PageUp) {
         selectionPageUp(); } else
      if (k == Keyboard::Space || k == Keyboard::Return) {
         Item *selected = invSelectedItem();
         game_state = INVENTORY_SELECT;
         alt_selection = 0;
         max_alt_selection = selected->num_actions;
      }

      return 0;
   }

   if (game_state == INVENTORY_SELECT) {
      if (alphaSelect( k, 0, true ))
         return 0;

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
            menu_scroll = 0;

         } else {
            addToInventory( retval );
            writeSystemLog( ">Unequipped:" );
            writeSystemLog( retval->getNamePadded() );
         }
      }

      return 0;
   }

   if (game_state == EQUIP_INVENTORY) {
      if (alphaSelect( k, menu_scroll, false ))
         return 0;

      if (k == Keyboard::Escape || k == Keyboard::BackSpace) {
         game_state = EQUIP_SCREEN; } else
      if (k == Keyboard::Numpad2 || k == Keyboard::Down) {
         selectionDown(); } else
      if (k == Keyboard::Numpad8 || k == Keyboard::Up) {
         selectionUp(); } else
      if (k == Keyboard::PageDown) {
         selectionPageDown(); } else
      if (k == Keyboard::PageUp) {
         selectionPageUp(); } else
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

      if (k == Keyboard::Comma) {
         Location &player_loc = current_level->map[player->pos_y][player->pos_x];
         if (player_loc.items == NULL)
            return 0;

         writeSystemLog( ">Picked up:" );

         if (player_loc.items->next == NULL) { // Pick up the one item
            addToInventory( player_loc.items );
            writeSystemLog( player_loc.items->getNamePadded() );
            player_loc.items = NULL;
            return 0;
         }

         // Otherwise
         selection = 0;
         max_selection = 0;
         Item *i = item_stack = player_loc.items;
         stack_selected_prev = NULL;
         while (i != NULL) { i = i->next; ++max_selection; }
         game_state = PICK_UP;
         return 0;
      }

      // Items

      if (k == Keyboard::F) {
         if (initTacticalSelection( RANGED_WEAPON ) == -1) {
            writeSystemLog( ">ERROR: NO RANGED" );
            writeSystemLog( " WEAPONRY EQUIPPED" );
         }
         else
            game_state = SELECTING_ABILITY;
         return 0;
      }

      // Movement
      if (k == Keyboard::Numpad1){
         speed = movePlayer( SOUTHWEST ); } else
      if (k == Keyboard::Numpad2 || k == Keyboard::Down){
         speed = movePlayer( SOUTH ); } else
      if (k == Keyboard::Numpad3){
         speed = movePlayer( SOUTHEAST ); } else
      if (k == Keyboard::Numpad4 || k == Keyboard::Left){
         speed = movePlayer( WEST ); } else
      if (k == Keyboard::Numpad6 || k == Keyboard::Right){
         speed = movePlayer( EAST ); } else
      if (k == Keyboard::Numpad7){
         speed = movePlayer( NORTHWEST ); } else
      if (k == Keyboard::Numpad8 || k == Keyboard::Up){
         speed = movePlayer( NORTH ); } else
      if (k == Keyboard::Numpad9){
         speed = movePlayer( NORTHEAST ); }

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
         KeyInput next_input = input_stack.top();
         input_stack.pop();
         sendKeyToGame( next_input.k, next_input.mod );
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
      if (speed == -1) { // Unit is dead
         delete current_unit;
         clearCurrentUnit();
         return 0;
      }
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
   const int start_col = 55, end_col = 79;
   int y;
   for (y = 1; y < 27; ++y) {
      writeChar( '|', C_WHITE, C_BLACK, start_col, y );
      writeChar( '|', C_WHITE, C_BLACK, end_col, y );
   }
   writeString( "+-----------------------+", C_WHITE, C_BLACK, start_col, 0 );
   writeString( "System Log", C_WHITE, C_BLACK, start_col+1, 1 );
   writeString( "+-----------------------+", C_WHITE, C_BLACK, start_col, 2 );
   writeString( "+-----------------------+", C_WHITE, C_BLACK, start_col, 27 );

   // Log contents
   std::deque<std::string>::iterator log_it = system_log.begin(), log_end = system_log.end();
   for (int i = 0; i < system_log_scroll; ++i) {
      if (log_it == log_end)
         return;

      ++log_it;
   }

   for (y = 26; y > 2; --y) {
      if (log_it == log_end)
         break;

      writeString( *log_it, C_WHITE, C_BLACK, start_col+1, y );
      ++log_it;
   }
}

// HUD/BottomBar
   
void drawBottomBar()
{
   std::stringstream tick_string;
   tick_string << "Ticks: " << ticks;
   writeString( tick_string.str(), C_WHITE, C_BLACK, 5, 29 );
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
   else if (game_state == SELECTING_ABILITY) 
   {
      drawUsableInventory();
   } 
   else
   {
      int x_start = 0;

      if (game_state == PICK_UP) {
         x_start = 25;
         drawItemStack( item_stack, "Pick up:", false, false );
      }

      doFOV();

      for (int x = x_start; x < 55; x++) {
         for (int y = 0; y < 28; y++) {

            int map_x = x + map_view_base.x, map_y = y + map_view_base.y;
            if (map_x < 0 || map_x >= current_level->x_dim ||
                  map_y < 0 || map_y >= current_level->y_dim)
               continue;

            int visibility = current_level->vision_map[map_y][map_x];
            if (visibility == 0)
               continue;

            Location &l = current_level->map[map_y][map_x];

            if (l.unit != NULL && visibility & MAP_VISIBLE) {
               l.unit->drawUnit(x, y);
            } else if (l.items != NULL) {
               l.items->drawItem(x, y);
            } else {
               if (l.ter == FLOOR)
                  writeChar( '.', C_WHITE, C_BLACK, x, y );
               else if (l.ter == IMPASSABLE_WALL)
                  writeChar( ' ', C_BLACK, C_WHITE, x, y );
               else if (l.ter >= STAIRS_UP_1 && l.ter <= STAIRS_UP_4)
                  writeChar( '<', C_WHITE, C_BLACK, x, y );
               else if (l.ter >= STAIRS_DOWN_1 && l.ter <= STAIRS_DOWN_4)
                  writeChar( '>', C_WHITE, C_BLACK, x, y );
            }

            if (visibility & MAP_SEEN && !(visibility & MAP_VISIBLE))
               dim( x, y, x, y );
         }
      }

      if (game_state == TARGETTING) {
         int x = reticle.x - map_view_base.x,
             y = reticle.y - map_view_base.y;
         colorInvert( x, y, x, y );
      }
   }

   drawSystemLog();
   drawBottomBar();

   drawDisplay();

   return 0;
}
