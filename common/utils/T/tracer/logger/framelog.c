#include "framelog.h"
#include "handler.h"
#include "database.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

struct framelog {
  char *event_name;
  void *database;
  unsigned long handler_id;
  int subframe_arg;
  int buffer_arg;
  /* list of views */
  view **v;
  int vsize;
  float *x;
  float *buffer;
  int blength;
};

static void _event(void *p, event e)
{
  struct framelog *l = p;
  int i;
  int subframe;
  void *buffer;
  int bsize;
  int nsamples;

  subframe = e.e[l->subframe_arg].i;
  buffer = e.e[l->buffer_arg].b;
  bsize = e.e[l->buffer_arg].bsize;

  nsamples = bsize / (2*sizeof(int16_t));

  if (l->blength != nsamples * 10) {
    l->blength = nsamples * 10;
    free(l->x);
    free(l->buffer);
    l->x = calloc(sizeof(float), l->blength);
    if (l->x == NULL) abort();
    l->buffer = calloc(sizeof(float), l->blength);
    if (l->buffer == NULL) abort();
    /* update 'x' */
    for (i = 0; i < l->blength; i++)
      l->x[i] = i;
    /* update 'length' of views */
    for (i = 0; i < l->vsize; i++)
      l->v[i]->set(l->v[i], "length", l->blength);
  }

  /* TODO: compute the LOGs in the plotter (too much useless computations) */
  for (i = 0; i < nsamples; i++) {
    int I = ((int16_t *)buffer)[i*2];
    int Q = ((int16_t *)buffer)[i*2+1];
    l->buffer[subframe * nsamples + i] = 10*log10(1.0+(float)(I*I+Q*Q));
  }

  if (subframe == 9)
    for (i = 0; i < l->vsize; i++)
      l->v[i]->append(l->v[i], l->x, l->buffer, l->blength);
}

framelog *new_framelog(event_handler *h, void *database,
    char *event_name, char *subframe_varname, char *buffer_varname)
{
  struct framelog *ret;
  int event_id;
  database_event_format f;
  int i;

  ret = calloc(1, sizeof(struct framelog)); if (ret == NULL) abort();

  ret->event_name = strdup(event_name); if (ret->event_name == NULL) abort();
  ret->database = database;

  event_id = event_id_from_name(database, event_name);

  ret->handler_id = register_handler_function(h, event_id, _event, ret);

  f = get_format(database, event_id);

  /* look for subframe and buffer args */
  ret->subframe_arg = -1;
  ret->buffer_arg = -1;
  for (i = 0; i < f.count; i++) {
    if (!strcmp(f.name[i], subframe_varname)) ret->subframe_arg = i;
    if (!strcmp(f.name[i], buffer_varname)) ret->buffer_arg = i;
  }
  if (ret->subframe_arg == -1) {
    printf("%s:%d: subframe argument '%s' not found in event '%s'\n",
        __FILE__, __LINE__, subframe_varname, event_name);
    abort();
  }
  if (ret->buffer_arg == -1) {
    printf("%s:%d: buffer argument '%s' not found in event '%s'\n",
        __FILE__, __LINE__, buffer_varname, event_name);
    abort();
  }
  if (strcmp(f.type[ret->subframe_arg], "int") != 0) {
    printf("%s:%d: argument '%s' has wrong type (should be 'int')\n",
        __FILE__, __LINE__, subframe_varname);
    abort();
  }
  if (strcmp(f.type[ret->buffer_arg], "buffer") != 0) {
    printf("%s:%d: argument '%s' has wrong type (should be 'buffer')\n",
        __FILE__, __LINE__, buffer_varname);
    abort();
  }

  return ret;
}

void framelog_add_view(framelog *_l, view *v)
{
  struct framelog *l = _l;
  l->vsize++;
  l->v = realloc(l->v, l->vsize * sizeof(view *)); if (l->v == NULL) abort();
  l->v[l->vsize-1] = v;
}
