#include "logger.h"
#include "logger_defs.h"
#include "event.h"
#include "database.h"
#include "handler.h"
#include "filter/filter.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

struct ttilog {
  struct logger common;
  void *database;
  int frame_arg;
  int subframe_arg;
  int data_arg;
  int convert_to_dB;
};

static void _event(void *p, event e)
{
  struct ttilog *l = p;
  int i;
  int frame;
  int subframe;
  float value;

  if (l->common.filter != NULL && filter_eval(l->common.filter, e) == 0)
    return;

  frame = e.e[l->frame_arg].i;
  subframe = e.e[l->subframe_arg].i;
  switch (e.e[l->data_arg].type) {
  case EVENT_INT: value = e.e[l->data_arg].i; break;
  case EVENT_ULONG: value = e.e[l->data_arg].ul; break;
  default: printf("%s:%d: unsupported type\n", __FILE__, __LINE__); abort();
  }

  if (l->convert_to_dB) value = 10 * log10(value);

  for (i = 0; i < l->common.vsize; i++)
    l->common.v[i]->append(l->common.v[i], frame, subframe, value);
}

logger *new_ttilog(event_handler *h, void *database,
    char *event_name, char *frame_varname, char *subframe_varname,
    char *data_varname, int convert_to_dB)
{
  struct ttilog *ret;
  int event_id;
  database_event_format f;
  int i;

  ret = calloc(1, sizeof(struct ttilog)); if (ret == NULL) abort();

  ret->common.event_name = strdup(event_name);
  if (ret->common.event_name == NULL) abort();
  ret->database = database;
  ret->convert_to_dB = convert_to_dB;

  event_id = event_id_from_name(database, event_name);

  ret->common.handler_id = register_handler_function(h,event_id,_event,ret);

  f = get_format(database, event_id);

  /* look for frame, subframe and data args */
  ret->frame_arg = -1;
  ret->subframe_arg = -1;
  ret->data_arg = -1;
  for (i = 0; i < f.count; i++) {
    if (!strcmp(f.name[i], frame_varname)) ret->frame_arg = i;
    if (!strcmp(f.name[i], subframe_varname)) ret->subframe_arg = i;
    if (!strcmp(f.name[i], data_varname)) ret->data_arg = i;
  }
  if (ret->frame_arg == -1) {
    printf("%s:%d: frame argument '%s' not found in event '%s'\n",
        __FILE__, __LINE__, frame_varname, event_name);
    abort();
  }
  if (ret->subframe_arg == -1) {
    printf("%s:%d: subframe argument '%s' not found in event '%s'\n",
        __FILE__, __LINE__, subframe_varname, event_name);
    abort();
  }
  if (ret->data_arg == -1) {
    printf("%s:%d: data argument '%s' not found in event '%s'\n",
        __FILE__, __LINE__, data_varname, event_name);
    abort();
  }
  if (strcmp(f.type[ret->frame_arg], "int") != 0) {
    printf("%s:%d: argument '%s' has wrong type (should be 'int')\n",
        __FILE__, __LINE__, frame_varname);
    abort();
  }
  if (strcmp(f.type[ret->subframe_arg], "int") != 0) {
    printf("%s:%d: argument '%s' has wrong type (should be 'int')\n",
        __FILE__, __LINE__, subframe_varname);
    abort();
  }
  if (strcmp(f.type[ret->data_arg], "int") != 0 &&
      strcmp(f.type[ret->data_arg], "float") != 0) {
    printf("%s:%d: argument '%s' has wrong type"
           " (should be 'int' or 'float')\n",
        __FILE__, __LINE__, data_varname);
    abort();
  }

  return ret;
}
