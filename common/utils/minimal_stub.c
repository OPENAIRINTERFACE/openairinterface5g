#include <stdlib.h>

#ifndef T_TRACER
int T_stdout;
#endif

struct configmodule_interface_s;
struct configmodule_interface_s *uniqCfg = NULL;

void exit_function(const char *file, const char *function, const int line, const char *s, const int assert)
{
  if (assert) {
    abort();
  } else {
    exit(EXIT_SUCCESS);
  }
}
