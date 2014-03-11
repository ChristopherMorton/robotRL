#ifndef ITEMS_H__
#define ITEMS_H__

enum ItemType {
   // Equipment
   CHASSIS,
   ARM,
   LEGS,
   MOUNTED,
   SYSTEM,
   BATTERY,
   // Usable
   MISSILE,
   GRENADE,
   MINE,
   TURRET,
   DEVICE,
   CODE,
   REMAINS
};

struct Item {
   ItemType type;
   unsigned int display_char;
   
   Item *next;

   Item (unsigned int d_c);

   void drawItem( int x, int y );
};

enum ChassisType {
   BASIC, // 2 arm 2 leg
   QUAD, // 4 arm 2 leg
   DOME, // 2 arm 0 legs - e.g. Dalek
   CRITTER, // 0 arms 0 legs - small and fast e.g. mouse bot
   HEAVY // 2 arm 2 leg many mounts - large and slow
};

struct Chassis : public Item
{
   ChassisType c_type;

   int num_arms;
   Item *arms;

   int num_legs;
   Item *legs;

   int num_mounts;
   Item *mounts;

   int num_systems;
   Item *systems;

   Item *battery;
};

#endif
