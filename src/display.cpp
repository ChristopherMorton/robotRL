#include "display.h"

char **display_array;

void init()
{
   display_array = new char*[30];
   for (int i = 0; i < 30; ++i)
      display_array[i] = new char[80];
}

void clearDisplay()
{
   for (int i = 0; i < 30; ++i)
      for (int j = 0; j < 80; ++j)
         display_array[i][j] = '.';
}

int writeChar( unsigned int c, sf::Color color, int x, int y );
int writeString( std::string s, sf::Color color, int x, int y );

void drawDisplay()
{
   sf::Text c;
   c.setFont

}
