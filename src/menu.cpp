#include "menu.h"
#include "display.h"
//#include "game.h"
#include "shutdown.h"

#include <SFML/Graphics.hpp>

int which_menu = 1;
int menu_selection = 1;
int menu_max = 2;

sf::Color MenuFG = sf::Color::White;
sf::Color MenuBG = sf::Color::Black;

void displayMenu()
{
   writeString("New Game", MenuFG, MenuBG, 36, 14); 
   writeString("  Quit", MenuFG, MenuBG, 36, 15);

   int inv_row = 14;
   if (menu_selection == 2)
      inv_row = 15;

   invert( 33, inv_row, 46, inv_row );
   
   drawDisplay();
}

int passKeyToMenu( sf::Keyboard::Key k )
{
   if (k == sf::Keyboard::Up) {
      menu_selection--;
      if (menu_selection == 0)
         menu_selection = menu_max;
   }
   if (k == sf::Keyboard::Down) {
      menu_selection++;
      if (menu_selection > menu_max)
         menu_selection = 1;
   }

   if (k == sf::Keyboard::Return || k == sf::Keyboard::Space) {
      if (which_menu == 1) {
         if (menu_selection == 1) {
            // Start a new game
            return 1;
         } else if (menu_selection == 2) {
            shutdown(1, 1);
         }
      }
   }

   return 0;
}
