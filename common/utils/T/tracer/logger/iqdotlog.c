#include "logger.h"
#include "logger_defs.h"
#include "handler.h"
#include "database.h"
#include "filter/filter.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

struct iqdotlog {
  struct logger common;
  void *database;
  int i_arg;
  int q_arg;
};

static void _event(void *p, event e)
{
  struct iqdotlog *l = p;
  int i;
  float I, Q;

  if (l->common.filter != NULL && filter_eval(l->common.filter, e) == 0)
    return;

  I = e.e[l->i_arg].i;
  Q = e.e[l->q_arg].i;

  for (i = 0; i < l->common.vsize; i++)
    l->common.v[i]->append(l->common.v[i], &I, &Q, 1);
}

logger *new_iqdotlog(event_handler *h, void *database,
    char *event_name, char *I, char *Q)
{
  struct iqdotlog *ret;
  int event_id;
  database_event_format f;
  int i;

  ret = calloc(1, sizeof(struct iqdotlog)); if (ret == NULL) abort();

  ret->common.event_name = strdup(event_name);
  if (ret->common.event_name == NULL) abort();
  ret->database = database;

  event_id = event_id_from_name(database, event_name);

  ret->common.handler_id = register_handler_function(h,event_id,_event,ret);

  f = get_format(database, event_id);

  /* look for args */
  ret->i_arg = -1;
  ret->q_arg = -1;
  for (i = 0; i < f.count; i++) {
    if (!strcmp(f.name[i], I)) ret->i_arg = i;
    if (!strcmp(f.name[i], Q)) ret->q_arg = i;
  }
  if (ret->i_arg == -1) {
    printf("%s:%d: argument '%s' not found in event '%s'\n",
        __FILE__, __LINE__, I, event_name);
    abort();
  }
  if (ret->q_arg == -1) {
    printf("%s:%d: argument '%s' not found in event '%s'\n",
        __FILE__, __LINE__, Q, event_name);
    abort();
  }
  if (strcmp(f.type[ret->i_arg], "int") != 0) {
    printf("%s:%d: argument '%s' has wrong type (should be 'int')\n",
        __FILE__, __LINE__, I);
    abort();
  }
  if (strcmp(f.type[ret->q_arg], "int") != 0) {
    printf("%s:%d: argument '%s' has wrong type (should be 'int')\n",
        __FILE__, __LINE__, Q);
    abort();
  }

  return ret;
}
