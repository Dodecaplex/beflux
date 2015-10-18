#include "beflux.h"

int main(int argc, char **argv) {
  int status = 0;
  if (argc == 1) {
    fprintf(stderr, ":: LIBBEFLUX_TEST ::\nUsage: libbeflux_test [program.bfx]\n");
  }
  else {
    beflux *b = bfx_new();
    bfx_load(b, 0, argv[1]);
    status = bfx_run(b);
    bfx_del(b);
  }
  return status;
}
