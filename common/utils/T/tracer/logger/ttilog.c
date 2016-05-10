#include "ttilog.h"
#include "event.h"
#include "database.h"
#include "handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct ttilog {
  char *event_name;
  void *database;
  unsigned long handler_id;
  int frame_arg;
  int subframe_arg;
  int data_arg;
  /* list of views */
  view **v;
  int vsize;
};

static void _event(void *p, event e)
{
  struct ttilog *l = p;
  int i;
  int frame;
  int subframe;
  float value;

  frame = e.e[l->frame_arg].i;
  subframe = e.e[l->subframe_arg].i;
  switch (e.e[l->data_arg].type) {
  case EVENT_INT: value = e.e[l->data_arg].i; break;
  default: printf("%s:%d: unsupported type\n", __FILE__, __LINE__); abort();
  }

  for (i = 0; i < l->vsize; i++)
    l->v[i]->append(l->v[i], frame, subframe, value);
}

ttilog *new_ttilog(event_handler *h, void *database,
    char *event_name, char *frame_varname, char *subframe_varname,
    char *data_varname)
{
  struct ttilog *ret;
  int event_id;
  database_event_format f;
  int i;

  ret = calloc(1, sizeof(struct ttilog)); if (ret == NULL) abort();

  ret->event_name = strdup(event_name); if (ret->event_name == NULL) abort();
  ret->database = database;

  event_id = event_id_from_name(database, event_name);

  ret->handler_id = register_handler_function(h, event_id, _event, ret);

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

void ttilog_add_view(ttilog *_l, view *v)
{
  struct ttilog *l = _l;
  l->vsize++;
  l->v = realloc(l->v, l->vsize * sizeof(view *)); if (l->v == NULL) abort();
  l->v[l->vsize-1] = v;
}
