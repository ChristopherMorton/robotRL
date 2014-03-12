#include "items.h"
#include "display.h"
#include "log.h"

#include <sstream>

using namespace sf;

void Item::drawItem( int x, int y ) {
   writeChar( display_char, Color::White, Color::Black, x, y );
}

/* Generic Items have only one action:
 * 0 - Drop
 */

int Item::drawActions()
{
   const int column = 33, header_column = 31, column_end = 57;
   int row = 12;

   num_actions = 1;

   writeString( "Actions:", Color::White, Color::Black, header_column, row );
   row++;
   writeString( "a) Drop", Color::White, Color::Black, column, row );
   colorInvert( column, row, column_end, row );

   return num_actions;
}

int Item::doAction( int selection )
{
   return 0;
}

void Item::drawDescription()
{
   writeString( getName(), Color::White, Color::Black, 32, 2 );
}

//////////////////////////////////////////////////////////////////////
// Chassis
//////////////////////////////////////////////////////////////////////

Item* Chassis::removeAll()
{
   Item *stack = getAllItems();

   arms = NULL;
   mounts = NULL;
   systems = NULL;

   return stack;
}

Item* Chassis::getAllItems()
{
   Item *stack = NULL, *stack_last = NULL;

   if (arms != NULL) {
      stack = stack_last = arms;

      while (stack_last->next != NULL) stack_last = stack_last->next;
   }

   if (mounts != NULL) {
      if (stack == NULL)
         stack = stack_last = mounts;
      else
         stack_last->next = mounts;

      while (stack_last->next != NULL) stack_last = stack_last->next;
   }

   if (systems != NULL) {
      if (stack == NULL)
         stack = stack_last = systems;
      else
         stack_last->next = systems;

      while (stack_last->next != NULL) stack_last = stack_last->next;
   }
   return stack;
}

Item* Chassis::getAllMelee()
{
   Item *alt_stack = NULL, *alt_stack_end = NULL;

   Item *next_item = NULL;

   // All melee weapons are in the arm slot
   if (arms != NULL) {
      next_item = arms;
      while (next_item != NULL) {
         if (next_item->weapon_type == MELEE_WEAPON) {
            if (alt_stack == NULL)
               alt_stack = alt_stack_end = next_item;
            else {
               alt_stack_end->alt_next = next_item;
               alt_stack_end = next_item;
            }
         }
         next_item = next_item->next;
      }
   }

   return alt_stack;
}

Item* Chassis::getAllRanged()
{
   Item *alt_stack = NULL, *alt_stack_end = NULL;

   Item *next_item = NULL;

   // All ranged weapons are in the arm or mount slot
   if (arms != NULL) {
      next_item = arms;
      while (next_item != NULL) {
         if (next_item->weapon_type == RANGED_WEAPON) {
            if (alt_stack == NULL)
               alt_stack = alt_stack_end = next_item;
            else {
               alt_stack_end->alt_next = next_item;
               alt_stack_end = next_item;
            }
         }
         next_item = next_item->next;
      }
   }
   if (mounts != NULL) {
      next_item = mounts;
      while (next_item != NULL) {
         if (next_item->weapon_type == RANGED_WEAPON) {
            if (alt_stack == NULL)
               alt_stack = alt_stack_end = next_item;
            else {
               alt_stack_end->alt_next = next_item;
               alt_stack_end = next_item;
            }
         }
         next_item = next_item->next;
      }
   }

   return alt_stack;
}

Item* Chassis::getAllTactical()
{
   Item *alt_stack = NULL, *alt_stack_end = NULL;

   Item *next_item = NULL;

   // Tactical weapons can be in any slot
   if (arms != NULL) {
      next_item = arms;
      while (next_item != NULL) {
         if (next_item->weapon_type == TACTICAL_WEAPON) {
            if (alt_stack == NULL)
               alt_stack = alt_stack_end = next_item;
            else {
               alt_stack_end->alt_next = next_item;
               alt_stack_end = next_item;
            }
         }
         next_item = next_item->next;
      }
   }
   if (mounts != NULL) {
      next_item = mounts;
      while (next_item != NULL) {
         if (next_item->weapon_type == TACTICAL_WEAPON) {
            if (alt_stack == NULL)
               alt_stack = alt_stack_end = next_item;
            else {
               alt_stack_end->alt_next = next_item;
               alt_stack_end = next_item;
            }
         }
         next_item = next_item->next;
      }
   }
   if (systems != NULL) {
      next_item = systems;
      while (next_item != NULL) {
         if (next_item->weapon_type == TACTICAL_WEAPON) {
            if (alt_stack == NULL)
               alt_stack = alt_stack_end = next_item;
            else {
               alt_stack_end->alt_next = next_item;
               alt_stack_end = next_item;
            }
         }
         next_item = next_item->next;
      }
   }

   return alt_stack;
}

Item* Chassis::getAllNonWeapon()
{
   Item *alt_stack = NULL, *alt_stack_end = NULL;

   Item *next_item = NULL;

   // Tactical weapons can be in any slot
   if (arms != NULL) {
      next_item = arms;
      while (next_item != NULL) {
         if (next_item->weapon_type >= USABLE_WEAPON) {
            if (alt_stack == NULL)
               alt_stack = alt_stack_end = next_item;
            else {
               alt_stack_end->alt_next = next_item;
               alt_stack_end = next_item;
            }
         }
         next_item = next_item->next;
      }
   }
   if (mounts != NULL) {
      next_item = mounts;
      while (next_item != NULL) {
         if (next_item->weapon_type >= USABLE_WEAPON) {
            if (alt_stack == NULL)
               alt_stack = alt_stack_end = next_item;
            else {
               alt_stack_end->alt_next = next_item;
               alt_stack_end = next_item;
            }
         }
         next_item = next_item->next;
      }
   }
   if (systems != NULL) {
      next_item = systems;
      while (next_item != NULL) {
         if (next_item->weapon_type >= USABLE_WEAPON) {
            if (alt_stack == NULL)
               alt_stack = alt_stack_end = next_item;
            else {
               alt_stack_end->alt_next = next_item;
               alt_stack_end = next_item;
            }
         }
         next_item = next_item->next;
      }
   }

   return alt_stack;
}

int Chassis::addArm( Item* arm )
{
   if (arm == NULL) {
      log("Chassis can't add arm, arm is NULL");
      return -2;
   }
   
   if (arm->next != NULL) {
      log("Chassis can't add arm, arm has a next Item* attached");
      return -3;
   }

   if (arm->type != ARM) {
      log("Chassis can't add arm, as it is not an arm item");
      return -4;
   }

   if (arms == NULL && num_arms > 0) {
      arms = arm;
      return 0;
   }

   int count = 1;
   Item *cur = arms;
   while( count < num_arms ) {
      if (cur->next == NULL) { // Slot in here
         cur->next = arm;
         return 0;
      }
      cur = cur->next;
      count++;
   }
   // No space
   log("Chassis can't add arm, no more arm slots");
   return -1;
}

int Chassis::addMount( Item* mount )
{
   if (mount == NULL) {
      log("Chassis can't add mount, mount is NULL");
      return -2;
   }
   
   if (mount->next != NULL) {
      log("Chassis can't add mount, mount has a next Item* attached");
      return -3;
   }

   if (mount->type != MOUNTED) {
      log("Chassis can't add mount, as it is not a mounted item");
      return -4;
   }

   if (mounts == NULL && num_mounts > 0) {
      mounts = mount;
      return 0;
   }

   int count = 1;
   Item *cur = mounts;
   while( count < num_mounts ) {
      if (cur->next == NULL) { // Slot in here
         cur->next = mount;
         return 0;
      }
      cur = cur->next;
      count++;
   }
   // No space
   log("Chassis can't add mount, no more mount slots");
   return -1;
}

int Chassis::addSystem( Item* system )
{
   if (system == NULL) {
      log("Chassis can't add system, system is NULL");
      return -2;
   }
   
   if (system->next != NULL) {
      log("Chassis can't add system, system has a next Item* attached");
      return -3;
   }

   if (system->type != SYSTEM) {
      log("Chassis can't add system, as it is not a system item");
      return -4;
   }


   if (systems == NULL && num_systems > 0) {
      systems = system;
      return 0;
   }

   int count = 1;
   Item *cur = systems;
   while( count < num_systems ) {
      if (cur->next == NULL) { // Slot in here
         cur->next = system;
         return 0;
      }
      cur = cur->next;
      count++;
   }
   // No space
   log("Chassis can't add system, no more system slots");
   return -1;
}

Item* Chassis::removeArm( int number )
{
   if ( number < 0 || number >= num_arms ) {
      log("Chassis can't remove arm, number is invalid");
      return NULL;
   }

   Item *cur, *to_remove;

   if ( number == 0 ) {
      to_remove = arms;
      if (arms != NULL)
         arms = arms->next;
      return to_remove;
   }

   int count = 1;
   cur = arms;
   while (cur != NULL) {
      to_remove = cur->next;
      if (count == number) {
         // Remove to_remove
         if (to_remove != NULL)
            cur->next = to_remove->next;

         return to_remove;
      }

      cur = cur->next;
      count++;
   }
   // No arm at this number
   log("Chassis can't remove arm, no arm at this number");
   return NULL;
}

Item* Chassis::removeMount( int number )
{
   if ( number < 0 || number >= num_mounts ) {
      log("Chassis can't remove mount, number is invalid");
      return NULL;
   }

   Item *cur, *to_remove;

   if ( number == 0 ) {
      to_remove = mounts;
      if (mounts != NULL)
         mounts = mounts->next;
      return to_remove;
   }

   int count = 1;
   cur = mounts;
   while (cur != NULL) {
      to_remove = cur->next;
      if (count == number) {
         // Remove to_remove
         if (to_remove != NULL)
            cur->next = to_remove->next;

         return to_remove;
      }

      cur = cur->next;
      count++;
   }
   // No mount at this number
   log("Chassis can't remove mount, no mount at this number");
   return NULL;
}

Item* Chassis::removeSystem( int number )
{
   if ( number < 0 || number >= num_systems ) {
      log("Chassis can't remove system, number is invalid");
      return NULL;
   }

   Item *cur, *to_remove;

   if ( number == 0 ) {
      to_remove = systems;
      if (systems != NULL)
         systems = systems->next;
      return to_remove;
   }

   int count = 1;
   cur = systems;
   while (cur != NULL) {
      to_remove = cur->next;
      if (count == number) {
         // Remove to_remove
         if (to_remove != NULL)
            cur->next = to_remove->next;

         return to_remove;
      }

      cur = cur->next;
      count++;
   }
   // No system at this number
   log("Chassis can't remove system, no system at this number");
   return NULL;
}

Item* Chassis::removeAny( int number )
{
   if (number < 0 || number >= getTotalSlots()) {
      log("Can't removeAny, number is invalid");
      return NULL;
   }

   if (number < num_arms)
      return removeArm( number );

   number -= num_arms;

   if (number < num_mounts)
      return removeMount( number );

   number -= num_mounts;

   return removeSystem( number );
}

int Chassis::getTotalSlots()
{
   return num_arms + num_mounts + num_systems;
}

void Chassis::listEquipment( int selection )
{
   const int column = 33, header_column = 31, column_end = 57;
   int row = 1, count = 0;
   Item *to_write;

   to_write = arms;
   writeString( "ARM SLOTS:", Color::White, Color::Black, header_column, row );
   ++row;
   for (int i = 0; i < num_arms; ++i) {
      if (to_write != NULL) {
         writeString( to_write->getName(), Color::White, Color::Black, column, row );
         to_write = to_write->next;
      }
      else {
         writeString( "<empty>", Color::White, Color::Black, column, row );
      }

      if (count == selection)
         colorInvert( column, row, column_end, row );
   
      ++count;
      ++row;
   }

   to_write = mounts;
   writeString( "MOUNT SLOTS:", Color::White, Color::Black, header_column, row );
   ++row;
   for (int i = 0; i < num_mounts; ++i) {
      if (to_write != NULL) {
         writeString( to_write->getName(), Color::White, Color::Black, column, row );
         to_write = to_write->next;
      }
      else
         writeString( "<empty>", Color::White, Color::Black, column, row );

      if (count == selection)
         colorInvert( column, row, column_end, row );

      ++count;
      ++row;
   }
      
   to_write = systems;
   writeString( "SYSTEM SLOTS:", Color::White, Color::Black, header_column, row );
   ++row;
   for (int i = 0; i < num_systems; ++i) {
      if (to_write != NULL) {
         writeString( to_write->getName(), Color::White, Color::Black, column, row );
         to_write = to_write->next;
      }
      else
         writeString( "<empty>", Color::White, Color::Black, column, row );

      if (count == selection)
         colorInvert( column, row, column_end, row );

      ++count;
      ++row;
   }
}

void Chassis::drawChassisStats( int row )
{
   std::stringstream dur_str;
   dur_str << "Durability:  " << durability << "/" << max_durability;

   writeString( dur_str.str(), Color::White, Color::Black, 34, row );
}

//////////////////////////////////////////////////////////////////////
// Specific Chassises
//////////////////////////////////////////////////////////////////////


BasicChassis::BasicChassis()
{
   type = CHASSIS;
   weapon_type = NOT_A_WEAPON;
   alt_next = NULL;
   next = NULL;

   display_char = '&';
   arms = NULL;
   mounts = NULL;
   systems = NULL;

   num_arms = 2;
   num_mounts = 1;
   num_systems = 3;

   durability = max_durability = 100;
}

std::string BasicChassis::getName() {
   return "Basic Chassis";
}

void BasicChassis::drawDescription()
{
   writeString( getName(), Color::White, Color::Black, 32, 2 );
   writeString( "A typical robotic frame,", Color::White, Color::Black, 32, 4);
   writeString( "with plug-and-play ports", Color::White, Color::Black, 32, 5);
   writeString( "for arms, system mods and", Color::White, Color::Black, 32, 6);
   writeString( "one heavy mount.", Color::White, Color::Black, 32, 7);

   drawChassisStats( 9 );
}

void BasicChassis::drawEquipScreen( int selection )
{
   writeString("        Basic Chassis", Color::White, Color::Black, 0, 2);
   writeString("          ----------", Color::White, Color::Black, 0, 4);
   writeString("         |          |", Color::White, Color::Black, 0, 5);
   writeString("         |  /\\  /\\  |", Color::White, Color::Black, 0, 6);
   writeString("         |  \\/  \\/  |", Color::White, Color::Black, 0, 7);
   writeString("         |          |", Color::White, Color::Black, 0, 8);
   writeString("         |    ==    |  ++", Color::White, Color::Black, 0, 9);
   writeString("          ----------   ++", Color::White, Color::Black, 0, 10);
   writeString("              ||      //", Color::White, Color::Black, 0, 11);
   writeString("      ------------------", Color::White, Color::Black, 0, 12);
   writeString("      /                \\", Color::White, Color::Black, 0, 13);
   writeString("     /  / \\        / \\  \\", Color::White, Color::Black, 0, 14);
   writeString("    /  /   \\      /   \\  \\", Color::White, Color::Black, 0, 15);
   writeString("    | |     \\    /     | |", Color::White, Color::Black, 0, 16);
   writeString("    | |      \\--/      | |", Color::White, Color::Black, 0, 17);
   writeString("    +++       ||       +++", Color::White, Color::Black, 0, 18);
   writeString("            ------", Color::White, Color::Black, 0, 19);
   writeString("           |      |", Color::White, Color::Black, 0, 20);
   writeString("          / ------ \\", Color::White, Color::Black, 0, 21);
   writeString("         //        \\\\", Color::White, Color::Black, 0, 22);
   writeString("         ||        ||", Color::White, Color::Black, 0, 23);
   writeString("         ||        ||", Color::White, Color::Black, 0, 24);
   writeString("         ||        ||", Color::White, Color::Black, 0, 25);
   writeString("       ----        ----", Color::White, Color::Black, 0, 26);

   listEquipment( selection );
}

QuadChassis::QuadChassis()
{
   type = CHASSIS;
   weapon_type = NOT_A_WEAPON;
   alt_next = NULL;
   next = NULL;

   display_char = '&';
   arms = NULL;
   mounts = NULL;
   systems = NULL;

   num_arms = 4;
   num_mounts = 1;
   num_systems = 3;

   durability = max_durability = 120;
}

std::string QuadChassis::getName() {
   return "Quad Chassis";
}

void QuadChassis::drawDescription()
{
   writeString( getName(), Color::White, Color::Black, 32, 2 );
   writeString( "A four-armed extension of the basic", Color::White, Color::Black, 32, 4);
   writeString( "frame, used for heavy lifting,", Color::White, Color::Black, 32, 5);
   writeString( "mining, and melee combat.", Color::White, Color::Black, 32, 6);

   drawChassisStats( 8 );
}

void QuadChassis::drawEquipScreen( int selection )
{
   writeString("                              ", Color::White, Color::Black, 0, 2);
   writeString("                              ", Color::White, Color::Black, 0, 3);
   writeString("                              ", Color::White, Color::Black, 0, 4);
   writeString("                              ", Color::White, Color::Black, 0, 5);
   writeString("                              ", Color::White, Color::Black, 0, 6);
   writeString("                              ", Color::White, Color::Black, 0, 7);
   writeString("                              ", Color::White, Color::Black, 0, 8);
   writeString("                              ", Color::White, Color::Black, 0, 9);
   writeString("                              ", Color::White, Color::Black, 0, 10);
   writeString("                              ", Color::White, Color::Black, 0, 11);
   writeString("                              ", Color::White, Color::Black, 0, 12);
   writeString("                              ", Color::White, Color::Black, 0, 13);
   writeString("                              ", Color::White, Color::Black, 0, 14);
   writeString("                              ", Color::White, Color::Black, 0, 15);
   writeString("                              ", Color::White, Color::Black, 0, 16);
   writeString("                              ", Color::White, Color::Black, 0, 17);
   writeString("                              ", Color::White, Color::Black, 0, 18);
   writeString("                              ", Color::White, Color::Black, 0, 19);
   writeString("                              ", Color::White, Color::Black, 0, 20);
   writeString("                              ", Color::White, Color::Black, 0, 21);
   writeString("                              ", Color::White, Color::Black, 0, 22);
   writeString("                              ", Color::White, Color::Black, 0, 23);
   writeString("                              ", Color::White, Color::Black, 0, 24);
   writeString("                              ", Color::White, Color::Black, 0, 25);
   writeString("                              ", Color::White, Color::Black, 0, 26);
   writeString("                              ", Color::White, Color::Black, 0, 27);

   listEquipment( selection );

}

DomeChassis::DomeChassis()
{
   type = CHASSIS;
   weapon_type = NOT_A_WEAPON;
   alt_next = NULL;
   next = NULL;

   display_char = '&';
   arms = NULL;
   mounts = NULL;
   systems = NULL;

   num_arms = 2;
   num_mounts = 2;
   num_systems = 6;

   durability = max_durability = 300;
}

std::string DomeChassis::getName() {
   return "Dome Chassis";
}

void DomeChassis::drawDescription()
{
   writeString( getName(), Color::White, Color::Black, 32, 2 );
   writeString( "A rolling frame designed for", Color::White, Color::Black, 32, 4);
   writeString( "high-level computing and command.", Color::White, Color::Black, 32, 5);
   writeString( "Tough and efficient.", Color::White, Color::Black, 32, 6);

   drawChassisStats( 8 );
} 

void DomeChassis::drawEquipScreen( int selection )
{
   writeString("                              ", Color::White, Color::Black, 0, 2);
   writeString("                              ", Color::White, Color::Black, 0, 3);
   writeString("                              ", Color::White, Color::Black, 0, 4);
   writeString("                              ", Color::White, Color::Black, 0, 5);
   writeString("                              ", Color::White, Color::Black, 0, 6);
   writeString("                              ", Color::White, Color::Black, 0, 7);
   writeString("                              ", Color::White, Color::Black, 0, 8);
   writeString("                              ", Color::White, Color::Black, 0, 9);
   writeString("                              ", Color::White, Color::Black, 0, 10);
   writeString("                              ", Color::White, Color::Black, 0, 11);
   writeString("                              ", Color::White, Color::Black, 0, 12);
   writeString("                              ", Color::White, Color::Black, 0, 13);
   writeString("                              ", Color::White, Color::Black, 0, 14);
   writeString("                              ", Color::White, Color::Black, 0, 15);
   writeString("                              ", Color::White, Color::Black, 0, 16);
   writeString("                              ", Color::White, Color::Black, 0, 17);
   writeString("                              ", Color::White, Color::Black, 0, 18);
   writeString("                              ", Color::White, Color::Black, 0, 19);
   writeString("                              ", Color::White, Color::Black, 0, 20);
   writeString("                              ", Color::White, Color::Black, 0, 21);
   writeString("                              ", Color::White, Color::Black, 0, 22);
   writeString("                              ", Color::White, Color::Black, 0, 23);
   writeString("                              ", Color::White, Color::Black, 0, 24);
   writeString("                              ", Color::White, Color::Black, 0, 25);
   writeString("                              ", Color::White, Color::Black, 0, 26);
   writeString("                              ", Color::White, Color::Black, 0, 27);

   listEquipment( selection );

}

CritterChassis::CritterChassis()
{
   type = CHASSIS;
   weapon_type = NOT_A_WEAPON;
   alt_next = NULL;
   next = NULL;

   display_char = '&';
   arms = NULL;
   mounts = NULL;
   systems = NULL;

   num_arms = 0;
   num_mounts = 2;
   num_systems = 3;

   durability = max_durability = 180;
}

std::string CritterChassis::getName() {
   return "Critter Chassis";
}

void CritterChassis::drawDescription()
{
   writeString( getName(), Color::White, Color::Black, 32, 2 );
   writeString( "A compact frame designed for", Color::White, Color::Black, 32, 4);
   writeString( "mobility and unobtrusiveness.", Color::White, Color::Black, 32, 5);

   drawChassisStats( 7 );
} 

void CritterChassis::drawEquipScreen( int selection )
{
   writeString("                              ", Color::White, Color::Black, 0, 2);
   writeString("                              ", Color::White, Color::Black, 0, 3);
   writeString("                              ", Color::White, Color::Black, 0, 4);
   writeString("                              ", Color::White, Color::Black, 0, 5);
   writeString("                              ", Color::White, Color::Black, 0, 6);
   writeString("                              ", Color::White, Color::Black, 0, 7);
   writeString("                              ", Color::White, Color::Black, 0, 8);
   writeString("                              ", Color::White, Color::Black, 0, 9);
   writeString("                              ", Color::White, Color::Black, 0, 10);
   writeString("                              ", Color::White, Color::Black, 0, 11);
   writeString("                              ", Color::White, Color::Black, 0, 12);
   writeString("                              ", Color::White, Color::Black, 0, 13);
   writeString("                              ", Color::White, Color::Black, 0, 14);
   writeString("                              ", Color::White, Color::Black, 0, 15);
   writeString("                              ", Color::White, Color::Black, 0, 16);
   writeString("                              ", Color::White, Color::Black, 0, 17);
   writeString("                              ", Color::White, Color::Black, 0, 18);
   writeString("                              ", Color::White, Color::Black, 0, 19);
   writeString("                              ", Color::White, Color::Black, 0, 20);
   writeString("                              ", Color::White, Color::Black, 0, 21);
   writeString("                              ", Color::White, Color::Black, 0, 22);
   writeString("                              ", Color::White, Color::Black, 0, 23);
   writeString("                              ", Color::White, Color::Black, 0, 24);
   writeString("                              ", Color::White, Color::Black, 0, 25);
   writeString("                              ", Color::White, Color::Black, 0, 26);
   writeString("                              ", Color::White, Color::Black, 0, 27);

   listEquipment( selection );

}

HeavyChassis::HeavyChassis()
{
   type = CHASSIS;
   weapon_type = NOT_A_WEAPON;
   alt_next = NULL;
   next = NULL;

   display_char = '&';
   arms = NULL;
   mounts = NULL;
   systems = NULL;

   num_arms = 4;
   num_mounts = 6;
   num_systems = 4;

   durability = max_durability = 700;
}

std::string HeavyChassis::getName() {
   return "Heavy Chassis";
}

void HeavyChassis::drawDescription()
{
   writeString( getName(), Color::White, Color::Black, 32, 2 );
   writeString( "A massive bipedal frame with", Color::White, Color::Black, 32, 4);
   writeString( "multiple heavy launchers", Color::White, Color::Black, 32, 5);
   writeString( "protruding from its shoulders.", Color::White, Color::Black, 32, 6);
   writeString( "Very powerful, but slow to move.", Color::White, Color::Black, 32, 7);

   drawChassisStats( 9 );
}

void HeavyChassis::drawEquipScreen( int selection )
{
   writeString("                              ", Color::White, Color::Black, 0, 2);
   writeString("                              ", Color::White, Color::Black, 0, 3);
   writeString("                              ", Color::White, Color::Black, 0, 4);
   writeString("                              ", Color::White, Color::Black, 0, 5);
   writeString("                              ", Color::White, Color::Black, 0, 6);
   writeString("                              ", Color::White, Color::Black, 0, 7);
   writeString("                              ", Color::White, Color::Black, 0, 8);
   writeString("                              ", Color::White, Color::Black, 0, 9);
   writeString("                              ", Color::White, Color::Black, 0, 10);
   writeString("                              ", Color::White, Color::Black, 0, 11);
   writeString("                              ", Color::White, Color::Black, 0, 12);
   writeString("                              ", Color::White, Color::Black, 0, 13);
   writeString("                              ", Color::White, Color::Black, 0, 14);
   writeString("                              ", Color::White, Color::Black, 0, 15);
   writeString("                              ", Color::White, Color::Black, 0, 16);
   writeString("                              ", Color::White, Color::Black, 0, 17);
   writeString("                              ", Color::White, Color::Black, 0, 18);
   writeString("                              ", Color::White, Color::Black, 0, 19);
   writeString("                              ", Color::White, Color::Black, 0, 20);
   writeString("                              ", Color::White, Color::Black, 0, 21);
   writeString("                              ", Color::White, Color::Black, 0, 22);
   writeString("                              ", Color::White, Color::Black, 0, 23);
   writeString("                              ", Color::White, Color::Black, 0, 24);
   writeString("                              ", Color::White, Color::Black, 0, 25);
   writeString("                              ", Color::White, Color::Black, 0, 26);
   writeString("                              ", Color::White, Color::Black, 0, 27);

   listEquipment( selection );

}

OrbChassis::OrbChassis()
{
   type = CHASSIS;
   weapon_type = NOT_A_WEAPON;
   alt_next = NULL;
   next = NULL;

   display_char = '&';
   arms = NULL;
   mounts = NULL;
   systems = NULL;

   num_arms = 2;
   num_mounts = 4;
   num_systems = 7;

   durability = max_durability = 420;
}

std::string OrbChassis::getName() {
   return "Orb Chassis";
}

void OrbChassis::drawDescription()
{
   writeString( getName(), Color::White, Color::Black, 32, 2 );
   writeString( "A levitating orb with dual", Color::White, Color::Black, 32, 4);
   writeString( "arm sockets and several heavy", Color::White, Color::Black, 32, 5);
   writeString( "mounts. Mobile and powerful,", Color::White, Color::Black, 32, 6);
   writeString( "but with high energy costs.", Color::White, Color::Black, 32, 7);

   drawChassisStats( 9 );
}

void OrbChassis::drawEquipScreen( int selection )
{
   writeString("                              ", Color::White, Color::Black, 0, 2);
   writeString("                              ", Color::White, Color::Black, 0, 3);
   writeString("                              ", Color::White, Color::Black, 0, 4);
   writeString("                              ", Color::White, Color::Black, 0, 5);
   writeString("                              ", Color::White, Color::Black, 0, 6);
   writeString("                              ", Color::White, Color::Black, 0, 7);
   writeString("                              ", Color::White, Color::Black, 0, 8);
   writeString("                              ", Color::White, Color::Black, 0, 9);
   writeString("                              ", Color::White, Color::Black, 0, 10);
   writeString("                              ", Color::White, Color::Black, 0, 11);
   writeString("                              ", Color::White, Color::Black, 0, 12);
   writeString("                              ", Color::White, Color::Black, 0, 13);
   writeString("                              ", Color::White, Color::Black, 0, 14);
   writeString("                              ", Color::White, Color::Black, 0, 15);
   writeString("                              ", Color::White, Color::Black, 0, 16);
   writeString("                              ", Color::White, Color::Black, 0, 17);
   writeString("                              ", Color::White, Color::Black, 0, 18);
   writeString("                              ", Color::White, Color::Black, 0, 19);
   writeString("                              ", Color::White, Color::Black, 0, 20);
   writeString("                              ", Color::White, Color::Black, 0, 21);
   writeString("                              ", Color::White, Color::Black, 0, 22);
   writeString("                              ", Color::White, Color::Black, 0, 23);
   writeString("                              ", Color::White, Color::Black, 0, 24);
   writeString("                              ", Color::White, Color::Black, 0, 25);
   writeString("                              ", Color::White, Color::Black, 0, 26);
   writeString("                              ", Color::White, Color::Black, 0, 27);

   listEquipment( selection );
}

//////////////////////////////////////////////////////////////////////
// Arms
//////////////////////////////////////////////////////////////////////


ClawArm::ClawArm()
{
   type = ARM;
   weapon_type = MELEE_WEAPON;
   alt_next = NULL;
   next = NULL;

   display_char = '(';
}

int ClawArm::meleeAttack( Unit *target )
{
   return 0;
}

std::string ClawArm::getName()
{
   return "Claw Arm Mark I";
} 

void ClawArm::drawDescription()
{
   writeString( getName(), Color::White, Color::Black, 32, 2 );
   writeString( "A mechanical arm that ends", Color::White, Color::Black, 32, 4);
   writeString( "in a gripping claw.", Color::White, Color::Black, 32, 5);
}
