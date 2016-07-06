#include "logger.h"
#include "logger_defs.h"
#include "handler.h"
#include "database.h"
#include "filter/filter.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

struct framelog {
  struct logger common;
  void *database;
  int subframe_arg;
  int buffer_arg;
  float *x;
  float *buffer;
  int blength;
  int skip_delay;       /* one frame over 'skip_delay' is truly processed
                         * 0 to disable (process all data)
                         */
  int skip_current;     /* internal data for the skip mechanism */
  int skip_on;          /* internal data for the skip mechanism */
  int update_only_at_sf9;
};

static void _event(void *p, event e)
{
  struct framelog *l = p;
  int i;
  int subframe;
  void *buffer;
  int bsize;
  int nsamples;

  if (l->common.filter != NULL && filter_eval(l->common.filter, e) == 0)
    return;

  subframe = e.e[l->subframe_arg].i;
  buffer = e.e[l->buffer_arg].b;
  bsize = e.e[l->buffer_arg].bsize;

  if (l->skip_delay != 0) {
    if (subframe == 0) {
      l->skip_current++;
      if (l->skip_current >= l->skip_delay) {
        l->skip_on = 0;
        l->skip_current = 0;
      } else
        l->skip_on = 1;
    }
  }
  if (l->skip_on) return;

  nsamples = bsize / (2*sizeof(int16_t));

  if (l->blength != nsamples * 10) {
    l->blength = nsamples * 10;
    free(l->x);
    free(l->buffer);
    l->x = calloc(l->blength, sizeof(float));
    if (l->x == NULL) abort();
    l->buffer = calloc(l->blength, sizeof(float));
    if (l->buffer == NULL) abort();
    /* update 'x' */
    for (i = 0; i < l->blength; i++)
      l->x[i] = i;
    /* update 'length' of views */
    for (i = 0; i < l->common.vsize; i++)
      l->common.v[i]->set(l->common.v[i], "length", l->blength);
  }

  /* TODO: compute the LOGs in the plotter (too much useless computations) */
  for (i = 0; i < nsamples; i++) {
    int I = ((int16_t *)buffer)[i*2];
    int Q = ((int16_t *)buffer)[i*2+1];
    l->buffer[subframe * nsamples + i] = 10*log10(1.0+(float)(I*I+Q*Q));
  }

  if (l->update_only_at_sf9 == 0 || subframe == 9)
    for (i = 0; i < l->common.vsize; i++)
      l->common.v[i]->append(l->common.v[i], l->x, l->buffer, l->blength);
}

logger *new_framelog(event_handler *h, void *database,
    char *event_name, char *subframe_varname, char *buffer_varname)
{
  struct framelog *ret;
  int event_id;
  database_event_format f;
  int i;

  ret = calloc(1, sizeof(struct framelog)); if (ret == NULL) abort();

  ret->update_only_at_sf9 = 1;

  ret->common.event_name = strdup(event_name);
  if (ret->common.event_name == NULL) abort();
  ret->database = database;

  event_id = event_id_from_name(database, event_name);

  ret->common.handler_id = register_handler_function(h,event_id,_event,ret);

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

/****************************************************************************/
/*                             public functions                             */
/****************************************************************************/

void framelog_set_skip(logger *_this, int skip_delay)
{
  struct framelog *l = _this;
  /* TODO: protect with a lock? */
  l->skip_delay = skip_delay;
  l->skip_current = 0;
  l->skip_on = 0;
}

void framelog_set_update_only_at_sf9(logger *_this, int update_only_at_sf9)
{
  struct framelog *l = _this;
  l->update_only_at_sf9 = update_only_at_sf9;
}
