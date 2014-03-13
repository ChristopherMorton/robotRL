#include "items.h"
#include "display.h"
#include "log.h"
#include "game.h"
#include "syslog.h"
#include "units.h"
#include "defs.h"

#include <sstream>

using namespace sf;

void Item::drawItem( int x, int y ) {
   writeChar( display_char, C_WHITE, C_BLACK, x, y );
}

/* Generic Items have only one action:
 * a)Drop
 */

int Item::drawActions()
{
   int row = actions_start_row;

   writeString( "Actions:", C_WHITE, C_BLACK, actions_header_column, row );
   row++;

   writeString( "a)", C_GRAY, C_BLACK, actions_header_column, row);
   writeString( "Drop", C_WHITE, C_BLACK, actions_column, row );

   return num_actions;
}

int Item::doAction( int selection )
{
   if (selection != 0) return -1;

   dropItem( this );
   return 0;
}

void Item::drawDescription()
{
   writeString( getName(), C_WHITE, C_BLACK, desc_col, 2 );
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
   int retval = 0;
   if (arm == NULL) {
      log("Chassis can't add arm, arm is NULL");
      retval = -2;
   }
   else if (arm->next != NULL) {
      log("Chassis can't add arm, arm has a next Item* attached");
      retval = -3;
   }
   else if (arm->type != ARM && arm->type != MOUNT) {
      log("Chassis can't add arm, as it is not an arm item");
      retval = -4;
   } else if (arms == NULL && num_arms > 0) {
      arms = arm;
      writeSystemLog( ">Equipped:" );
      writeSystemLog( arm->getName() );
      return 0;
   }
   else
   { 
      int count = 1;
      Item *cur = arms;
      while( count < num_arms ) {
         if (cur->next == NULL) { // Slot in here
            cur->next = arm;
            writeSystemLog( ">Equipped:" );
            writeSystemLog( arm->getName() );
            return 0;
         }
         cur = cur->next;
         count++;
      }

      // No space
      writeSystemLog( ">ERROR CANNOT EQUIP:" );
      writeSystemLog( arm->getName() );
      writeSystemLog( "ARM SLOTS AT CAPACITY" );
      retval = -1;
   }

   return retval;
}

int Chassis::addMount( Item* mount )
{
   int retval = 0;
   if (mount == NULL) {
      log("Chassis can't add mount, mount is NULL");
      retval = -2;
   }
   else if (mount->next != NULL) {
      log("Chassis can't add mount, mount has a next Item* attached");
      retval = -3;
   }
   else if (mount->type != MOUNT) {
      log("Chassis can't add mount, as it is not a mount item");
      retval = -4;
   } else if (mounts == NULL && num_mounts > 0) {
      mounts = mount;
      writeSystemLog( ">Equipped:" );
      writeSystemLog( mount->getName() );
      return 0;
   }
   else
   { 
      int count = 1;
      Item *cur = mounts;
      while( count < num_mounts ) {
         if (cur->next == NULL) { // Slot in here
            cur->next = mount;
            writeSystemLog( ">Equipped:" );
            writeSystemLog( mount->getName() );
            return 0;
         }
         cur = cur->next;
         count++;
      }

      // No space
      log("Chassis can't add mount, no more mount slots");
      retval = -1;
   }

   writeSystemLog( ">Unable to equip:" );
   writeSystemLog( mount->getName() );

   return retval;
}

int Chassis::addSystem( Item* system )
{
   int retval = 0;
   if (system == NULL) {
      log("Chassis can't add system, system is NULL");
      retval = -2;
   }
   else if (system->next != NULL) {
      log("Chassis can't add system, system has a next Item* attached");
      retval = -3;
   }
   else if (system->type != SYSTEM) {
      log("Chassis can't add system, as it is not a system item");
      retval = -4;
   } else if (systems == NULL && num_systems > 0) {
      systems = system;
      writeSystemLog( ">Equipped:" );
      writeSystemLog( system->getName() );
      return 0;
   }
   else
   { 
      int count = 1;
      Item *cur = systems;
      while( count < num_systems ) {
         if (cur->next == NULL) { // Slot in here
            cur->next = system;
            writeSystemLog( ">Equipped:" );
            writeSystemLog( system->getName() );
            return 0;
         }
         cur = cur->next;
         count++;
      }

      // No space
      log("Chassis can't add system, no more system slots");
      retval = -1;
   }

   writeSystemLog( ">Unable to equip:" );
   writeSystemLog( system->getName() );

   return retval;
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

ItemType Chassis::getSlot( int number )
{
   if (number < 0 || number >= getTotalSlots())
      return CHASSIS;

   if (number < num_arms)
      return ARM;

   number -= num_arms;

   if (number < num_mounts)
      return MOUNT;

   number -= num_mounts;

   return SYSTEM;
}

int Chassis::getTotalSlots()
{
   return num_arms + num_mounts + num_systems;
}

void Chassis::listEquipment( int selection )
{
   const int column = 31, header_column = 29, column_end = 50;
   int row = 1, count = 0;
   Item *to_write;

   to_write = arms;
   writeString( "ARM SLOTS:", C_WHITE, C_BLACK, header_column, row );
   ++row;
   for (int i = 0; i < num_arms; ++i) {
      if (to_write != NULL) {
         writeString( to_write->getName(), C_WHITE, C_BLACK, column, row );
         to_write = to_write->next;
      }
      else {
         writeString( "<empty>", C_WHITE, C_BLACK, column, row );
      }

      if (count == selection)
         colorInvert( column, row, column_end, row );
   
      ++count;
      ++row;
   }

   to_write = mounts;
   writeString( "MOUNT SLOTS:", C_WHITE, C_BLACK, header_column, row );
   ++row;
   for (int i = 0; i < num_mounts; ++i) {
      if (to_write != NULL) {
         writeString( to_write->getName(), C_WHITE, C_BLACK, column, row );
         to_write = to_write->next;
      }
      else
         writeString( "<empty>", C_WHITE, C_BLACK, column, row );

      if (count == selection)
         colorInvert( column, row, column_end, row );

      ++count;
      ++row;
   }
      
   to_write = systems;
   writeString( "SYSTEM SLOTS:", C_WHITE, C_BLACK, header_column, row );
   ++row;
   for (int i = 0; i < num_systems; ++i) {
      if (to_write != NULL) {
         writeString( to_write->getName(), C_WHITE, C_BLACK, column, row );
         to_write = to_write->next;
      }
      else
         writeString( "<empty>", C_WHITE, C_BLACK, column, row );

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

   writeString( dur_str.str(), C_WHITE, C_BLACK, 34, row );
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

   num_actions = 1;

   durability = max_durability = 100;
}

std::string BasicChassis::getName() {
   return "Basic Chassis";
}

void BasicChassis::drawDescription()
{
   writeString( getName(), C_WHITE, C_BLACK, desc_col, 2 );
   writeString( "A typical robotic frame,", C_WHITE, C_BLACK, desc_col, 4);
   writeString( "with plug-and-play ports", C_WHITE, C_BLACK, desc_col, 5);
   writeString( "for arms, system mods, and", C_WHITE, C_BLACK, desc_col, 6);
   writeString( "an extra heavy mount.", C_WHITE, C_BLACK, desc_col, 7);

   drawChassisStats( 9 );
}

void BasicChassis::drawEquipScreen( int selection )
{
   writeString("        Basic Chassis", C_WHITE, C_BLACK, 0, 2);
   writeString("          ----------", C_WHITE, C_BLACK, 0, 4);
   writeString("         |          |", C_WHITE, C_BLACK, 0, 5);
   writeString("         |  /\\  /\\  |", C_WHITE, C_BLACK, 0, 6);
   writeString("         |  \\/  \\/  |", C_WHITE, C_BLACK, 0, 7);
   writeString("         |          |", C_WHITE, C_BLACK, 0, 8);
   writeString("         |    ==    |  ++", C_WHITE, C_BLACK, 0, 9);
   writeString("          ----------   ++", C_WHITE, C_BLACK, 0, 10);
   writeString("              ||      //", C_WHITE, C_BLACK, 0, 11);
   writeString("      ------------------", C_WHITE, C_BLACK, 0, 12);
   writeString("      /                \\", C_WHITE, C_BLACK, 0, 13);
   writeString("     /  / \\        / \\  \\", C_WHITE, C_BLACK, 0, 14);
   writeString("    /  /   \\      /   \\  \\", C_WHITE, C_BLACK, 0, 15);
   writeString("    | |     \\    /     | |", C_WHITE, C_BLACK, 0, 16);
   writeString("    | |      \\--/      | |", C_WHITE, C_BLACK, 0, 17);
   writeString("    +++       ||       +++", C_WHITE, C_BLACK, 0, 18);
   writeString("            ------", C_WHITE, C_BLACK, 0, 19);
   writeString("           |      |", C_WHITE, C_BLACK, 0, 20);
   writeString("          / ------ \\", C_WHITE, C_BLACK, 0, 21);
   writeString("         //        \\\\", C_WHITE, C_BLACK, 0, 22);
   writeString("         ||        ||", C_WHITE, C_BLACK, 0, 23);
   writeString("         ||        ||", C_WHITE, C_BLACK, 0, 24);
   writeString("         ||        ||", C_WHITE, C_BLACK, 0, 25);
   writeString("       ----        ----", C_WHITE, C_BLACK, 0, 26);

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

   num_actions = 1;

   durability = max_durability = 120;
}

std::string QuadChassis::getName() {
   return "Quad Chassis";
}

void QuadChassis::drawDescription()
{
   writeString( getName(), C_WHITE, C_BLACK, desc_col, 2 );
   writeString( "A four-armed extension of", C_WHITE, C_BLACK, desc_col, 4);
   writeString( "the basic frame, built for", C_WHITE, C_BLACK, desc_col, 5);
   writeString( "mining and melee combat.", C_WHITE, C_BLACK, desc_col, 6);

   drawChassisStats( 8 );
}

void QuadChassis::drawEquipScreen( int selection )
{
   writeString("                              ", C_WHITE, C_BLACK, 0, 2);
   writeString("                              ", C_WHITE, C_BLACK, 0, 3);
   writeString("                              ", C_WHITE, C_BLACK, 0, 4);
   writeString("                              ", C_WHITE, C_BLACK, 0, 5);
   writeString("                              ", C_WHITE, C_BLACK, 0, 6);
   writeString("                              ", C_WHITE, C_BLACK, 0, 7);
   writeString("                              ", C_WHITE, C_BLACK, 0, 8);
   writeString("                              ", C_WHITE, C_BLACK, 0, 9);
   writeString("                              ", C_WHITE, C_BLACK, 0, 10);
   writeString("                              ", C_WHITE, C_BLACK, 0, 11);
   writeString("                              ", C_WHITE, C_BLACK, 0, 12);
   writeString("                              ", C_WHITE, C_BLACK, 0, 13);
   writeString("                              ", C_WHITE, C_BLACK, 0, 14);
   writeString("                              ", C_WHITE, C_BLACK, 0, 15);
   writeString("                              ", C_WHITE, C_BLACK, 0, 16);
   writeString("                              ", C_WHITE, C_BLACK, 0, 17);
   writeString("                              ", C_WHITE, C_BLACK, 0, 18);
   writeString("                              ", C_WHITE, C_BLACK, 0, 19);
   writeString("                              ", C_WHITE, C_BLACK, 0, 20);
   writeString("                              ", C_WHITE, C_BLACK, 0, 21);
   writeString("                              ", C_WHITE, C_BLACK, 0, 22);
   writeString("                              ", C_WHITE, C_BLACK, 0, 23);
   writeString("                              ", C_WHITE, C_BLACK, 0, 24);
   writeString("                              ", C_WHITE, C_BLACK, 0, 25);
   writeString("                              ", C_WHITE, C_BLACK, 0, 26);
   writeString("                              ", C_WHITE, C_BLACK, 0, 27);

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

   num_actions = 1;

   durability = max_durability = 300;
}

std::string DomeChassis::getName() {
   return "Dome Chassis";
}

void DomeChassis::drawDescription()
{
   writeString( getName(), C_WHITE, C_BLACK, desc_col, 2 );
   writeString( "A rolling frame designed for", C_WHITE, C_BLACK, desc_col, 4);
   writeString( "calculation and command.", C_WHITE, C_BLACK, desc_col, 5);
   writeString( "Tough and efficient.", C_WHITE, C_BLACK, desc_col, 6);

   drawChassisStats( 8 );
} 

void DomeChassis::drawEquipScreen( int selection )
{
   writeString("                              ", C_WHITE, C_BLACK, 0, 2);
   writeString("                              ", C_WHITE, C_BLACK, 0, 3);
   writeString("                              ", C_WHITE, C_BLACK, 0, 4);
   writeString("                              ", C_WHITE, C_BLACK, 0, 5);
   writeString("                              ", C_WHITE, C_BLACK, 0, 6);
   writeString("                              ", C_WHITE, C_BLACK, 0, 7);
   writeString("                              ", C_WHITE, C_BLACK, 0, 8);
   writeString("                              ", C_WHITE, C_BLACK, 0, 9);
   writeString("                              ", C_WHITE, C_BLACK, 0, 10);
   writeString("                              ", C_WHITE, C_BLACK, 0, 11);
   writeString("                              ", C_WHITE, C_BLACK, 0, 12);
   writeString("                              ", C_WHITE, C_BLACK, 0, 13);
   writeString("                              ", C_WHITE, C_BLACK, 0, 14);
   writeString("                              ", C_WHITE, C_BLACK, 0, 15);
   writeString("                              ", C_WHITE, C_BLACK, 0, 16);
   writeString("                              ", C_WHITE, C_BLACK, 0, 17);
   writeString("                              ", C_WHITE, C_BLACK, 0, 18);
   writeString("                              ", C_WHITE, C_BLACK, 0, 19);
   writeString("                              ", C_WHITE, C_BLACK, 0, 20);
   writeString("                              ", C_WHITE, C_BLACK, 0, 21);
   writeString("                              ", C_WHITE, C_BLACK, 0, 22);
   writeString("                              ", C_WHITE, C_BLACK, 0, 23);
   writeString("                              ", C_WHITE, C_BLACK, 0, 24);
   writeString("                              ", C_WHITE, C_BLACK, 0, 25);
   writeString("                              ", C_WHITE, C_BLACK, 0, 26);
   writeString("                              ", C_WHITE, C_BLACK, 0, 27);

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

   num_actions = 1;

   durability = max_durability = 180;
}

std::string CritterChassis::getName() {
   return "Critter Chassis";
}

void CritterChassis::drawDescription()
{
   writeString( getName(), C_WHITE, C_BLACK, desc_col, 2 );
   writeString( "A compact frame specializing", C_WHITE, C_BLACK, desc_col, 4);
   writeString( "in staying out of the way.", C_WHITE, C_BLACK, desc_col, 5);

   drawChassisStats( 7 );
} 

void CritterChassis::drawEquipScreen( int selection )
{
   writeString("                              ", C_WHITE, C_BLACK, 0, 2);
   writeString("                              ", C_WHITE, C_BLACK, 0, 3);
   writeString("                              ", C_WHITE, C_BLACK, 0, 4);
   writeString("                              ", C_WHITE, C_BLACK, 0, 5);
   writeString("                              ", C_WHITE, C_BLACK, 0, 6);
   writeString("                              ", C_WHITE, C_BLACK, 0, 7);
   writeString("                              ", C_WHITE, C_BLACK, 0, 8);
   writeString("                              ", C_WHITE, C_BLACK, 0, 9);
   writeString("                              ", C_WHITE, C_BLACK, 0, 10);
   writeString("                              ", C_WHITE, C_BLACK, 0, 11);
   writeString("                              ", C_WHITE, C_BLACK, 0, 12);
   writeString("                              ", C_WHITE, C_BLACK, 0, 13);
   writeString("                              ", C_WHITE, C_BLACK, 0, 14);
   writeString("                              ", C_WHITE, C_BLACK, 0, 15);
   writeString("                              ", C_WHITE, C_BLACK, 0, 16);
   writeString("                              ", C_WHITE, C_BLACK, 0, 17);
   writeString("                              ", C_WHITE, C_BLACK, 0, 18);
   writeString("                              ", C_WHITE, C_BLACK, 0, 19);
   writeString("                              ", C_WHITE, C_BLACK, 0, 20);
   writeString("                              ", C_WHITE, C_BLACK, 0, 21);
   writeString("                              ", C_WHITE, C_BLACK, 0, 22);
   writeString("                              ", C_WHITE, C_BLACK, 0, 23);
   writeString("                              ", C_WHITE, C_BLACK, 0, 24);
   writeString("                              ", C_WHITE, C_BLACK, 0, 25);
   writeString("                              ", C_WHITE, C_BLACK, 0, 26);
   writeString("                              ", C_WHITE, C_BLACK, 0, 27);

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

   num_actions = 1;

   durability = max_durability = 700;
}

std::string HeavyChassis::getName() {
   return "Heavy Chassis";
}

void HeavyChassis::drawDescription()
{
   writeString( getName(), C_WHITE, C_BLACK, desc_col, 2 );
   writeString( "A massive bipedal frame with", C_WHITE, C_BLACK, desc_col, 4);
   writeString( "multiple heavy launchers.", C_WHITE, C_BLACK, desc_col, 5);
   writeString( "Very powerful, but slow.", C_WHITE, C_BLACK, desc_col, 6);

   drawChassisStats( 9 );
}

void HeavyChassis::drawEquipScreen( int selection )
{
   writeString("                              ", C_WHITE, C_BLACK, 0, 2);
   writeString("                              ", C_WHITE, C_BLACK, 0, 3);
   writeString("                              ", C_WHITE, C_BLACK, 0, 4);
   writeString("                              ", C_WHITE, C_BLACK, 0, 5);
   writeString("                              ", C_WHITE, C_BLACK, 0, 6);
   writeString("                              ", C_WHITE, C_BLACK, 0, 7);
   writeString("                              ", C_WHITE, C_BLACK, 0, 8);
   writeString("                              ", C_WHITE, C_BLACK, 0, 9);
   writeString("                              ", C_WHITE, C_BLACK, 0, 10);
   writeString("                              ", C_WHITE, C_BLACK, 0, 11);
   writeString("                              ", C_WHITE, C_BLACK, 0, 12);
   writeString("                              ", C_WHITE, C_BLACK, 0, 13);
   writeString("                              ", C_WHITE, C_BLACK, 0, 14);
   writeString("                              ", C_WHITE, C_BLACK, 0, 15);
   writeString("                              ", C_WHITE, C_BLACK, 0, 16);
   writeString("                              ", C_WHITE, C_BLACK, 0, 17);
   writeString("                              ", C_WHITE, C_BLACK, 0, 18);
   writeString("                              ", C_WHITE, C_BLACK, 0, 19);
   writeString("                              ", C_WHITE, C_BLACK, 0, 20);
   writeString("                              ", C_WHITE, C_BLACK, 0, 21);
   writeString("                              ", C_WHITE, C_BLACK, 0, 22);
   writeString("                              ", C_WHITE, C_BLACK, 0, 23);
   writeString("                              ", C_WHITE, C_BLACK, 0, 24);
   writeString("                              ", C_WHITE, C_BLACK, 0, 25);
   writeString("                              ", C_WHITE, C_BLACK, 0, 26);
   writeString("                              ", C_WHITE, C_BLACK, 0, 27);

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

   num_arms = 6;
   num_mounts = 0;
   num_systems = 7;

   num_actions = 1;

   durability = max_durability = 420;
}

std::string OrbChassis::getName() {
   return "Orb Chassis";
}

void OrbChassis::drawDescription()
{
   writeString( getName(), C_WHITE, C_BLACK, desc_col, 2 );
   writeString( "A levitating orb with six", C_WHITE, C_BLACK, desc_col, 4);
   writeString( "heavy-duty arm sockets.", C_WHITE, C_BLACK, desc_col, 5);
   writeString( "Extremely mobile, but", C_WHITE, C_BLACK, desc_col, 6);
   writeString( "with high energy costs.", C_WHITE, C_BLACK, desc_col, 7);

   drawChassisStats( 9 );
}

void OrbChassis::drawEquipScreen( int selection )
{
   writeString("         Orb Chassis          ", C_WHITE, C_BLACK, 0, 2);
   writeString("                              ", C_WHITE, C_BLACK, 0, 3);
   writeString("                              ", C_WHITE, C_BLACK, 0, 4);
   writeString("                              ", C_WHITE, C_BLACK, 0, 5);
   writeString("                              ", C_WHITE, C_BLACK, 0, 6);
   writeString("       +++         +++        ", C_WHITE, C_BLACK, 0, 7);
   writeString("       \\  \\       /  /        ", C_WHITE, C_BLACK, 0, 8);
   writeString("        \\  \\     /  /         ", C_WHITE, C_BLACK, 0, 9);
   writeString("         \\  \\---/  /          ", C_WHITE, C_BLACK, 0, 10);
   writeString("          \\/     \\/           ", C_WHITE, C_BLACK, 0, 11);
   writeString("     +----|       |----+      ", C_WHITE, C_BLACK, 0, 12);
   writeString("     +    |  (O)  |    +      ", C_WHITE, C_BLACK, 0, 13);
   writeString("     +----|       |----+      ", C_WHITE, C_BLACK, 0, 14);
   writeString("          /\\     /\\           ", C_WHITE, C_BLACK, 0, 15);
   writeString("         /  /---\\  \\          ", C_WHITE, C_BLACK, 0, 16);
   writeString("        /  /     \\  \\         ", C_WHITE, C_BLACK, 0, 17);
   writeString("       /  /       \\  \\        ", C_WHITE, C_BLACK, 0, 18);
   writeString("       +++         +++        ", C_WHITE, C_BLACK, 0, 19);
   writeString("                              ", C_WHITE, C_BLACK, 0, 20);
   writeString("                              ", C_WHITE, C_BLACK, 0, 21);
   writeString("                              ", C_WHITE, C_BLACK, 0, 22);
   writeString("                              ", C_WHITE, C_BLACK, 0, 23);
   writeString("                              ", C_WHITE, C_BLACK, 0, 24);
   writeString("                              ", C_WHITE, C_BLACK, 0, 25);
   writeString("                              ", C_WHITE, C_BLACK, 0, 26);
   writeString("                              ", C_WHITE, C_BLACK, 0, 27);

   listEquipment( selection );
}

//////////////////////////////////////////////////////////////////////
// Arms
//////////////////////////////////////////////////////////////////////

int Arm::drawActions()
{
   int row = actions_start_row;

   writeString( "Actions:", C_WHITE, C_BLACK, actions_header_column, row );
   row++;

   writeString( "a)", C_GRAY, C_BLACK, actions_header_column, row);
   writeString( "Drop", C_WHITE, C_BLACK, actions_column, row );
   row++;
   writeString( "b)", C_GRAY, C_BLACK, actions_header_column, row);
   writeString( "Equip (Arm)", C_WHITE, C_BLACK, actions_column, row );

   return num_actions;
}


int Arm::doAction( int selection )
{
   if (selection == 0)
      dropItem( this );
   else if (selection == 1) {
      Chassis* ch = player->chassis;
      if (ch) {
         if (ch->addArm( this ) != 0)
            return -1;
      }
   }
   else
      return -1;

   return 0;
}

ClawArm::ClawArm()
{
   type = ARM;
   weapon_type = MELEE_WEAPON;
   alt_next = NULL;
   next = NULL;

   num_actions = 2;

   display_char = '(';
}

int ClawArm::meleeAttack( Unit *target )
{
   return 0;
}

std::string ClawArm::getName()
{
   return "VRX110 Manipulator";
} 

void ClawArm::drawDescription()
{
   writeString( getName(), C_WHITE, C_BLACK, desc_col, 2 );
   writeString( "A mechanical arm that ends", C_WHITE, C_BLACK, desc_col, 4);
   writeString( "in a gripping claw.", C_WHITE, C_BLACK, desc_col, 5);
}

HammerArm::HammerArm()
{
   type = ARM;
   weapon_type = MELEE_WEAPON;
   alt_next = NULL;
   next = NULL;

   num_actions = 2;

   display_char = '(';
}

int HammerArm::meleeAttack( Unit *target )
{
   return 0;
}

std::string HammerArm::getName()
{
   return "Series A7 Demolisher";
} 

void HammerArm::drawDescription()
{
   writeString( getName(), C_WHITE, C_BLACK, desc_col, 2 );
   writeString( "A massive pneumatic hammer.", C_WHITE, C_BLACK, desc_col, 4);
}

ShockArm::ShockArm()
{
   type = ARM;
   weapon_type = MELEE_WEAPON;
   alt_next = NULL;
   next = NULL;

   num_actions = 2;

   display_char = '(';
}

int ShockArm::meleeAttack( Unit *target )
{
   return 0;
}

std::string ShockArm::getName()
{
   return "VRX770 Heavy Taser";
} 

void ShockArm::drawDescription()
{
   writeString( getName(), C_WHITE, C_BLACK, desc_col, 2 );
   writeString( "Delivers 42,000,000V", C_WHITE, C_BLACK, desc_col, 4);
   writeString( "directly into the target.", C_WHITE, C_BLACK, desc_col, 5);
}

EnergyLance::EnergyLance()
{
   type = ARM;
   weapon_type = MELEE_WEAPON;
   alt_next = NULL;
   next = NULL;

   num_actions = 2;

   display_char = '(';
}

int EnergyLance::meleeAttack( Unit *target )
{
   return 0;
}

std::string EnergyLance::getName()
{
   return "Energy Lance Mk 1";
} 

void EnergyLance::drawDescription()
{
   writeString( getName(), C_WHITE, C_BLACK, desc_col, 2 );
   writeString( "Spears your target on a", C_WHITE, C_BLACK, desc_col, 4);
   writeString( "cone of destructive energy.", C_WHITE, C_BLACK, desc_col, 5);
}

//////////////////////////////////////////////////////////////////////
// Mounts
//////////////////////////////////////////////////////////////////////

int Mount::drawActions()
{
   int row = actions_start_row;

   writeString( "Actions:", C_WHITE, C_BLACK, actions_header_column, row );
   row++;

   writeString( "a)", C_GRAY, C_BLACK, actions_header_column, row);
   writeString( "Drop", C_WHITE, C_BLACK, actions_column, row );
   row++;
   writeString( "b)", C_GRAY, C_BLACK, actions_header_column, row);
   writeString( "Equip (Mount)", C_WHITE, C_BLACK, actions_column, row );
   row++;
   writeString( "c)", C_GRAY, C_BLACK, actions_header_column, row);
   writeString( "Equip (Arm)", C_WHITE, C_BLACK, actions_column, row );

   return num_actions;
}


int Mount::doAction( int selection )
{
   if (selection == 0)
      dropItem( this );
   else if (selection == 1) {
      Chassis* ch = player->chassis;
      if (ch) {
         if (ch->addMount( this ) != 0)
            return -1;
      }
   }
   else if (selection == 2) {
      Chassis* ch = player->chassis;
      if (ch) {
         if (ch->addArm( this ) != 0)
            return -1;
      }
   }
   else
      return -1;

   return 0;
}

Laser::Laser()
{
   type = MOUNT;
   weapon_type = RANGED_WEAPON;
   alt_next = NULL;
   next = NULL;

   num_actions = 3;

   display_char = '}';
}

int Laser::rangedAttack( Unit *target )
{
   return 0;
}

std::string Laser::getName()
{
   return "VRX24 Mining Laser";
} 

void Laser::drawDescription()
{
   writeString( getName(), C_WHITE, C_BLACK, desc_col, 2 );
   writeString( "Fires a beam of concentrated", C_WHITE, C_BLACK, desc_col, 4);
   writeString( "photons that can eat", C_WHITE, C_BLACK, desc_col, 5);
   writeString( "through any material.", C_WHITE, C_BLACK, desc_col, 6);
}
