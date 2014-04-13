#ifndef ITEMS_H__
#define ITEMS_H__

#include <string>

struct Unit;

enum ItemType {
   // Equipment
   CHASSIS,
   ARM,
   MOUNT,
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

// Flag values

#define MELEE_PIERCING 0x4
#define MELEE_DISABLING 0x8
#define MELEE_TARGET_CHASSIS 0x40

#define RANGED_DISABLING 0x8
#define RANGED_TARGET_CHASSIS 0x40

#define TARGET_CHASSIS 0x1

struct Item {
   ItemType type;
   WeaponType weapon_type;
   unsigned int display_char;

   int durability, max_durability;
   int armor;
   int rearm_time;
   bool targetted;
   
   Item *next, *alt_next;

   void initBasics();

   void drawItem( int x, int y );

   virtual std::string getName() = 0;
   std::string getNamePadded( int num=1, char pad=' ' );

   virtual void drawDescription();

   int num_actions;
   virtual int drawActions();
   virtual int doAction( int selection );

   virtual int meleeAttack( Unit *target );
   int genericMeleeAttack( Unit *target, int base_dmg, int dmg_variation, int flags=0 );

   virtual int rangedAttack( Unit *target );
   int genericRangedAttack( Unit *target, int base_dmg, int dmg_variation, int flags );

   virtual ~Item();
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

   void findAndRemoveItem( Item *to_remove );

   ItemType getSlot( int number );

   void listEquipment( int selection );
   int getTotalSlots();

   void drawChassisStats( int row );

   Item *selectRandomItem( int flags = 0 );

   virtual ~Chassis();
};

struct BasicChassis : public Chassis
{
   BasicChassis();
   virtual void drawEquipScreen( int selection );
   virtual std::string getName();

   virtual void drawDescription();

   virtual ~BasicChassis();
};

struct QuadChassis : public Chassis
{
   QuadChassis();
   virtual void drawEquipScreen( int selection );
   virtual std::string getName();

   virtual void drawDescription();

   virtual ~QuadChassis();
};

struct DomeChassis : public Chassis
{
   DomeChassis();
   virtual void drawEquipScreen( int selection );
   virtual std::string getName();

   virtual void drawDescription();

   virtual ~DomeChassis();
};

struct CritterChassis : public Chassis
{
   CritterChassis();
   virtual void drawEquipScreen( int selection );
   virtual std::string getName();

   virtual void drawDescription();

   virtual ~CritterChassis();
};

struct HeavyChassis : public Chassis
{
   HeavyChassis();
   virtual void drawEquipScreen( int selection );
   virtual std::string getName();

   virtual void drawDescription();

   virtual ~HeavyChassis();
};

struct OrbChassis : public Chassis
{
   OrbChassis();
   virtual void drawEquipScreen( int selection );
   virtual std::string getName();

   virtual void drawDescription();

   virtual ~OrbChassis();
};

// Arms

struct Arm : public Item
{
   virtual int drawActions();
   virtual int doAction( int selection );

   virtual ~Arm();
};

struct ClawArm : public Arm
{
   ClawArm();
   virtual int meleeAttack( Unit *target );
   virtual std::string getName();

   virtual void drawDescription();

   virtual ~ClawArm();
};

struct HammerArm : public Arm
{
   HammerArm();
   virtual int meleeAttack( Unit *target );
   virtual std::string getName();

   virtual void drawDescription();

   virtual ~HammerArm();
};

struct ShockArm : public Arm
{
   ShockArm();
   virtual int meleeAttack( Unit *target );
   virtual std::string getName();

   virtual void drawDescription();

   virtual ~ShockArm();
};

struct EnergyLance : public Arm
{
   EnergyLance();
   virtual int meleeAttack( Unit *target );
   virtual std::string getName();

   virtual void drawDescription();

   virtual ~EnergyLance();
};

// Mounted items

struct Mount : public Item
{
   virtual int drawActions();
   virtual int doAction( int selection );

   virtual ~Mount();
};

struct Laser : public Mount
{
   Laser();
   virtual int rangedAttack( Unit *target );
   virtual std::string getName();

   virtual void drawDescription();

   virtual ~Laser();
};

#endif
