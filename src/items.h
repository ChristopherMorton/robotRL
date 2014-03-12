#ifndef ITEMS_H__
#define ITEMS_H__

#include <string>

struct Unit;

enum ItemType {
   // Equipment
   CHASSIS,
   ARM,
   MOUNTED,
   SYSTEM,
   // Usable
   MISSILE,
   GRENADE,
   MINE,
   TURRET,
   DEVICE,
   CODE,
   REMAINS
};

enum WeaponType {
   MELEE_WEAPON,
   RANGED_WEAPON,
   TACTICAL_WEAPON,
   USABLE_WEAPON,
   NOT_A_WEAPON
};

struct Item {
   ItemType type;
   WeaponType weapon_type;
   unsigned int display_char;

   int durability, max_durability;
   
   Item *next, *alt_next;

   void drawItem( int x, int y );

   virtual std::string getName() = 0;

   virtual void drawDescription();

   int num_actions;
   virtual int drawActions();
   virtual int doAction( int selection );
};

struct MeleeWeapon
{
   virtual int meleeAttack( Unit *target ) = 0;
};

struct RangedWeapon
{
   virtual int rangedAttack( Unit *target ) = 0;
};

// Chasses

enum ChassisType {
   BASIC, // 2 arm
   QUAD, // 4 arm
   DOME, // 2 arm rolly thing - e.g. Dalek
   CRITTER, // 0 arms - small and fast e.g. mouse bot
   HEAVY, // 4 arm many mounts - large and slow
   ORB // 2 arms - levitating mount platform
};

struct Chassis : public Item
{
   int num_arms, num_mounts, num_systems;
   Item *arms, *mounts, *systems;

   ChassisType c_type;

   int durability, max_durability;

   virtual std::string getName() = 0;
   virtual void drawEquipScreen( int selection ) = 0;

   Item* removeAll();

   Item* getAllItems();
   Item* getAllMelee();
   Item* getAllRanged();
   Item* getAllTactical();
   Item* getAllNonWeapon();

   int addArm( Item* arm );
   int addMount( Item* mount );
   int addSystem( Item* system );
   Item* removeArm( int number );
   Item* removeMount( int number );
   Item* removeSystem( int number );
   Item* removeAny( int number );

   void listEquipment( int selection );
   int getTotalSlots();

   void drawChassisStats( int row );
};

struct BasicChassis : public Chassis
{
   BasicChassis();
   virtual void drawEquipScreen( int selection );
   virtual std::string getName();

   virtual void drawDescription();
};

struct QuadChassis : public Chassis
{
   QuadChassis();
   virtual void drawEquipScreen( int selection );
   virtual std::string getName();

   virtual void drawDescription();
};

struct DomeChassis : public Chassis
{
   DomeChassis();
   virtual void drawEquipScreen( int selection );
   virtual std::string getName();

   virtual void drawDescription();
};

struct CritterChassis : public Chassis
{
   CritterChassis();
   virtual void drawEquipScreen( int selection );
   virtual std::string getName();

   virtual void drawDescription();
};

struct HeavyChassis : public Chassis
{
   HeavyChassis();
   virtual void drawEquipScreen( int selection );
   virtual std::string getName();

   virtual void drawDescription();
};

struct OrbChassis : public Chassis
{
   OrbChassis();
   virtual void drawEquipScreen( int selection );
   virtual std::string getName();

   virtual void drawDescription();
};

// Arms

struct ClawArm : public Item, public MeleeWeapon
{
   ClawArm();
   virtual int meleeAttack( Unit *target );
   virtual std::string getName();

   virtual void drawDescription();
};

#endif
