#include "structures.h"

Location::Location() {
   ter = FLOOR;
   unit = 0;
   items = 0;
}

Level::Level( int x, int y ) {
   x_dim = x;
   y_dim = y;
   map = new Location*[y_dim];
   for (int i = 0; i < y_dim; ++i) {
      map[i] = new Location[x_dim];
   }
   exits = 0;
}
