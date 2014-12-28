// FOR TESTING PURPOSES

#include <iostream>

#include "beflux.h"

namespace dpx = Dodecaplex;
int main(void) {
  dpx::Beflux beflux;

  byte src[] = ">4101+.v        Q.+1024<";
  byte dst[16] = {};
  beflux.program()->read((byte*)src, 16);

  beflux.run();

  beflux.output()->write(dst, 16);

  for (auto &i: dst)
    std::cout << i;
  std::cout << std::endl;

  beflux.program()->write(dst, 16);

  for (auto &i: dst)
    std::cout << i;
  std::cout << std::endl;

  return 0;
}
