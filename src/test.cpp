// FOR TESTING PURPOSES

#include <iostream>

#include "beflux.h"

namespace dpx = Dodecaplex;
int main(void) {
  dpx::Beflux beflux;
  byte src[] = {
    '4','1','0','1','+','.','Q'
  };
  byte dst[16] = {};
  beflux.program()->read(src, 7);
  beflux.run();
  beflux.output()->write(dst, 16);
  for (auto &i: dst)
    std::cout << i << std::endl;
  return 0;
}
