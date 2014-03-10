#ifndef DISPLAY_H__
#define DISPLAY_H__

#include <string>
#include <SFML/Graphics.hpp>

void clearDisplay();

int writeChar( unsigned int c, sf::Color fg, sf::Color bg, int x, int y );
int writeString( std::string s, sf::Color fg, sf::Color bg, int x, int y );
int invert( int x_base, int y_base, int x_end, int y_end );

void drawDisplay();

#endif
