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

struct ticked_ttilog {
  struct logger common;
  void *database;
  int tick_frame_arg;
  int tick_subframe_arg;
  int data_frame_arg;
  int data_subframe_arg;
  int data_arg;
  int convert_to_dB;
  int last_tick_frame;
  int last_tick_subframe;
  float last_value;
  float empty_value;
  char *tick_event_name;
  unsigned long tick_handler_id;
  /* tick filter - NULL is no filter set */
  void *tick_filter;
};

static void _event(void *p, event e)
{
  struct ticked_ttilog *l = p;
  int frame;
  int subframe;
  float value;

  if (l->common.filter != NULL && filter_eval(l->common.filter, e) == 0)
    return;

  frame = e.e[l->data_frame_arg].i;
  subframe = e.e[l->data_subframe_arg].i;

  if (frame != l->last_tick_frame || subframe != l->last_tick_subframe) {
    printf("WARNING: %s:%d: data comes without previous tick!\n",
           __FILE__, __LINE__);
    return;
  }

  switch (e.e[l->data_arg].type) {
  case EVENT_INT: value = e.e[l->data_arg].i; break;
  case EVENT_ULONG: value = e.e[l->data_arg].ul; break;
  default: printf("%s:%d: unsupported type\n", __FILE__, __LINE__); abort();
  }

  if (l->convert_to_dB) value = 10 * log10(value);

  l->last_value = value;
}

static void _tick_event(void *p, event e)
{
  struct ticked_ttilog *l = p;
  int i;
  int frame;
  int subframe;

  if (l->tick_filter != NULL && filter_eval(l->tick_filter, e) == 0)
    return;

  frame = e.e[l->tick_frame_arg].i;
  subframe = e.e[l->tick_subframe_arg].i;

  for (i = 0; i < l->common.vsize; i++)
    l->common.v[i]->append(l->common.v[i], frame, subframe, l->last_value);

  l->last_value = l->empty_value;
  l->last_tick_frame = frame;
  l->last_tick_subframe = subframe;
}

logger *new_ticked_ttilog(event_handler *h, void *database,
    char *tick_event_name, char *frame_varname, char *subframe_varname,
    char *event_name, char *data_varname,
    int convert_to_dB, float empty_value)
{
  struct ticked_ttilog *ret;
  int event_id;
  database_event_format f;
  int i;

  ret = calloc(1, sizeof(struct ticked_ttilog)); if (ret == NULL) abort();

  ret->common.event_name = strdup(event_name);
  if (ret->common.event_name == NULL) abort();
  ret->database = database;
  ret->convert_to_dB = convert_to_dB;

  ret->tick_event_name = strdup(tick_event_name);
  if (ret->tick_event_name == NULL) abort();
  ret->empty_value = empty_value;
  ret->last_value = empty_value;

  /* tick event */
  event_id = event_id_from_name(database, tick_event_name);

  ret->tick_handler_id=register_handler_function(h,event_id,_tick_event,ret);

  f = get_format(database, event_id);

  /* look for frame and subframe */
  ret->tick_frame_arg = -1;
  ret->tick_subframe_arg = -1;
  for (i = 0; i < f.count; i++) {
    if (!strcmp(f.name[i], frame_varname)) ret->tick_frame_arg = i;
    if (!strcmp(f.name[i], subframe_varname)) ret->tick_subframe_arg = i;
  }
  if (ret->tick_frame_arg == -1) {
    printf("%s:%d: frame argument '%s' not found in event '%s'\n",
        __FILE__, __LINE__, frame_varname, event_name);
    abort();
  }
  if (ret->tick_subframe_arg == -1) {
    printf("%s:%d: subframe argument '%s' not found in event '%s'\n",
        __FILE__, __LINE__, subframe_varname, event_name);
    abort();
  }
  if (strcmp(f.type[ret->tick_frame_arg], "int") != 0) {
    printf("%s:%d: argument '%s' has wrong type (should be 'int')\n",
        __FILE__, __LINE__, frame_varname);
    abort();
  }
  if (strcmp(f.type[ret->tick_subframe_arg], "int") != 0) {
    printf("%s:%d: argument '%s' has wrong type (should be 'int')\n",
        __FILE__, __LINE__, subframe_varname);
    abort();
  }

  /* data event */
  event_id = event_id_from_name(database, event_name);

  ret->common.handler_id = register_handler_function(h,event_id,_event,ret);

  f = get_format(database, event_id);

  /* look for frame, subframe and data args */
  ret->data_frame_arg = -1;
  ret->data_subframe_arg = -1;
  ret->data_arg = -1;
  for (i = 0; i < f.count; i++) {
    if (!strcmp(f.name[i], frame_varname)) ret->data_frame_arg = i;
    if (!strcmp(f.name[i], subframe_varname)) ret->data_subframe_arg = i;
    if (!strcmp(f.name[i], data_varname)) ret->data_arg = i;
  }
  if (ret->data_frame_arg == -1) {
    printf("%s:%d: frame argument '%s' not found in event '%s'\n",
        __FILE__, __LINE__, frame_varname, event_name);
    abort();
  }
  if (ret->data_subframe_arg == -1) {
    printf("%s:%d: subframe argument '%s' not found in event '%s'\n",
        __FILE__, __LINE__, subframe_varname, event_name);
    abort();
  }
  if (ret->data_arg == -1) {
    printf("%s:%d: data argument '%s' not found in event '%s'\n",
        __FILE__, __LINE__, data_varname, event_name);
    abort();
  }
  if (strcmp(f.type[ret->data_frame_arg], "int") != 0) {
    printf("%s:%d: argument '%s' has wrong type (should be 'int')\n",
        __FILE__, __LINE__, frame_varname);
    abort();
  }
  if (strcmp(f.type[ret->data_subframe_arg], "int") != 0) {
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

void ticked_ttilog_set_tick_filter(logger *_l, void *filter)
{
  struct ticked_ttilog *l = (struct ticked_ttilog *)_l;
  free(l->tick_filter);
  l->tick_filter = filter;
}
