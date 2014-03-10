#ifndef DISPLAY_H__
#define DISPLAY_H__

#include <string>
#include <SFML/Graphics.hpp>

void clearDisplay();

int writeChar( unsigned int c, sf::Color color, int x, int y );
int writeString( std::string s, sf::Color color, int x, int y );

void drawDisplay();

#endif
