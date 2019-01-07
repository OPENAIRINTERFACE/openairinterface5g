#include "logger.h"
#include "logger_defs.h"
#include "filter/filter.h"
#include <stdlib.h>

void logger_add_view(logger *_l, view *v)
{
  struct logger *l = _l;
  l->vsize++;
  l->v = realloc(l->v, l->vsize * sizeof(view *)); if (l->v == NULL) abort();
  l->v[l->vsize-1] = v;
}

void logger_set_filter(logger *_l, void *filter)
{
  struct logger *l = _l;
  free_filter(l->filter);
  l->filter = filter;
}
