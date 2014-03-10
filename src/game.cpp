#include "game.h"
#include "display.h"
#include "shutdown.h"

#include <vector>

//////////////////////////////////////////////////////////////////////
// Definitions and Data Structures
//////////////////////////////////////////////////////////////////////

struct Unit;
struct Item;

// Levels

enum Terrain {
   FLOOR,
   IMPASSABLE_WALL,
   STAIRS_UP_1,
   STAIRS_UP_2,
   STAIRS_UP_3,
   STAIRS_UP_4,
   STAIRS_DOWN_1,
   STAIRS_DOWN_2,
   STAIRS_DOWN_3,
   STAIRS_DOWN_4
};

struct Location {
   Unit* unit;
   Item* items; // an item points the next item, in this case on the ground
   Terrain ter;

   Location() {
      ter = FLOOR;
      unit = NULL;
      items = NULL;
   }
};

struct Level {
   int x_dim, y_dim;
   Location **map;
   Level* *exits; // Indexed array of exits

   Level( int x, int y ) {
      x_dim = x;
      y_dim = y;
      map = new Location*[y_dim];
      for (int i = 0; i < y_dim; ++i) {
         map[i] = new Location[x_dim];
      }
      exits = NULL;
   }
};


//////////////////////////////////////////////////////////////////////
// Persistent Data
//////////////////////////////////////////////////////////////////////

std::vector<Level*> all_levels;
Level* current_level = NULL;

sf::Vector2u player_pos;
sf::Vector2u map_view_base; // The 0,0 of the view


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

   player_pos = sf::Vector2u( 50, 50 );
   map_view_base = sf::Vector2u( 10, 10 );
}

//////////////////////////////////////////////////////////////////////
// Main Interface
//////////////////////////////////////////////////////////////////////

int sendKeyToGame( sf::Keyboard::Key k )
{
   if (k == sf::Keyboard::Q)
      shutdown(1, 1);

   return 0;
}

int displayGame()
{
   if (current_level == NULL) return -1;

   for (int x = 0; x < 80; x++) {
      for (int y = 0; y < 30; y++) {

         int map_x = x + map_view_base.x, map_y = y + map_view_base.y;
         if (map_x < 0 || map_x >= current_level->x_dim ||
               map_y < 0 || map_y >= current_level->y_dim)
            continue;

         Location l = current_level->map[map_y][map_x];

         if (l.ter == FLOOR)
            writeChar( '.', sf::Color::White, sf::Color::Black, x, y );
         else if (l.ter == IMPASSABLE_WALL)
            writeChar( ' ', sf::Color::Black, sf::Color::White, x, y );

      }
   }

   drawDisplay();

   return 0;
}
