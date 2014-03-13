#ifndef MENU_H__
#define MENU_H__

#include <SFML/Window.hpp>

void displayMenu();
int sendKeyToMenu( sf::Keyboard::Key k, int mod=0 ); // Returns 1 when game begins

#endif
