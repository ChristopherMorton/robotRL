#ifndef STRUCTURES_H__
#define STRUCTURES_H__

struct Unit;
struct Item;

enum Direction
{
   NORTH,
   NORTHEAST,
   EAST,
   SOUTHEAST,
   SOUTH,
   SOUTHWEST,
   WEST,
   NORTHWEST
};

enum Terrain {
   IMPASSABLE_WALL,
   // Everything after here is passable, if you're the right kind of robot
   CONVEYER_HOR,
   CONVEYER_VER,
   FLOOR,
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

   Location();
};

struct Level {
   int x_dim, y_dim;
   Location **map;
   Level* *exits; // Indexed array of exits (Level*)

   Level( int x, int y );
};

#endif
