#include "logger.h"
#include "logger_defs.h"
#include "event.h"
#include "database.h"
#include "handler.h"
#include "filter/filter.h"
#include <stdlib.h>
#include <string.h>

struct timelog {
  struct logger common;
};

static void _event(void *p, event e)
{
  struct timelog *l = p;
  int i;

  if (l->common.filter != NULL && filter_eval(l->common.filter, e) == 0)
    return;

  for (i = 0; i < l->common.vsize; i++)
    l->common.v[i]->append(l->common.v[i], e.sending_time);
}

logger *new_timelog(event_handler *h, void *database, char *event_name)
{
  struct timelog *ret;
  int event_id;

  ret = calloc(1, sizeof(struct timelog)); if (ret == NULL) abort();

  ret->common.event_name = strdup(event_name);
  if (ret->common.event_name == NULL) abort();

  event_id = event_id_from_name(database, event_name);

  ret->common.handler_id = register_handler_function(h,event_id,_event,ret);

  return ret;
}
