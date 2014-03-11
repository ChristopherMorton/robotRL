#include "items.h"
#include "display.h"

Item::Item (unsigned int d_c) {
   display_char = d_c;
   next = NULL;
}

void Item::drawItem( int x, int y ) {
   writeChar( display_char, sf::Color::White, sf::Color::Black, x, y );
}
