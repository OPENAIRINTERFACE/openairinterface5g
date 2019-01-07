#include "logger.h"
#include "logger_defs.h"
#include "handler.h"
#include "database.h"
#include "filter/filter.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

struct iqlog {
  struct logger common;
  void *database;
  int nb_rb_arg;
  int N_RB_UL_arg;
  int symbols_per_tti_arg;
  int buffer_arg;
  float *i;
  float *q;
  int max_length;
};

static void _event(void *p, event e)
{
  struct iqlog *l = p;
  int i, j;
  void *buffer;
  int bsize;
  int nb_rb;
  int N_RB_UL;
  int symbols_per_tti;
  int max_nsamples;
  float *idst, *qdst;
  int count;

  if (l->common.filter != NULL && filter_eval(l->common.filter, e) == 0)
    return;

  nb_rb = e.e[l->nb_rb_arg].i;
  N_RB_UL = e.e[l->N_RB_UL_arg].i;
  symbols_per_tti = e.e[l->symbols_per_tti_arg].i;

  buffer = e.e[l->buffer_arg].b;
  bsize = e.e[l->buffer_arg].bsize;

  if (bsize != N_RB_UL * symbols_per_tti * 12 * 4) {
    printf("%s:%d:%s: bad buffer size\n", __FILE__, __LINE__, __FUNCTION__);
    abort();
  }

  max_nsamples = bsize / 4;

  if (max_nsamples > l->max_length) {
    l->i = realloc(l->i, max_nsamples * sizeof(float));
    if (l->i == NULL) abort();
    l->q = realloc(l->q, max_nsamples * sizeof(float));
    if (l->q == NULL) abort();
    l->max_length = max_nsamples;
  }

  idst = l->i;
  qdst = l->q;
  count = 0;
  for (i = 0; i < symbols_per_tti; i++)
    for (j = 0; j < 12 * nb_rb; j++) {
      *idst = ((int16_t *)buffer)[(i*N_RB_UL*12 + j) * 2];
      *qdst = ((int16_t *)buffer)[(i*N_RB_UL*12 + j) * 2 + 1];
      idst++;
      qdst++;
      count++;
    }

  for (i = 0; i < l->common.vsize; i++)
    l->common.v[i]->append(l->common.v[i], l->i, l->q, count);
}

logger *new_iqlog(event_handler *h, void *database,
    char *event_name, char *nb_rb, char *N_RB_UL, char *symbols_per_tti,
    char *buffer_varname)
{
  struct iqlog *ret;
  int event_id;
  database_event_format f;
  int i;

  ret = calloc(1, sizeof(struct iqlog)); if (ret == NULL) abort();

  ret->common.event_name = strdup(event_name);
  if (ret->common.event_name == NULL) abort();
  ret->database = database;

  event_id = event_id_from_name(database, event_name);

  ret->common.handler_id = register_handler_function(h,event_id,_event,ret);

  f = get_format(database, event_id);

  /* look for args */
  ret->nb_rb_arg = -1;
  ret->N_RB_UL_arg = -1;
  ret->symbols_per_tti_arg = -1;
  ret->buffer_arg = -1;
  for (i = 0; i < f.count; i++) {
    if (!strcmp(f.name[i], nb_rb)) ret->nb_rb_arg = i;
    if (!strcmp(f.name[i], N_RB_UL)) ret->N_RB_UL_arg = i;
    if (!strcmp(f.name[i], symbols_per_tti)) ret->symbols_per_tti_arg = i;
    if (!strcmp(f.name[i], buffer_varname)) ret->buffer_arg = i;
  }
  if (ret->nb_rb_arg == -1) {
    printf("%s:%d: argument '%s' not found in event '%s'\n",
        __FILE__, __LINE__, nb_rb, event_name);
    abort();
  }
  if (ret->N_RB_UL_arg == -1) {
    printf("%s:%d: argument '%s' not found in event '%s'\n",
        __FILE__, __LINE__, N_RB_UL, event_name);
    abort();
  }
  if (ret->symbols_per_tti_arg == -1) {
    printf("%s:%d: argument '%s' not found in event '%s'\n",
        __FILE__, __LINE__, symbols_per_tti, event_name);
    abort();
  }
  if (ret->buffer_arg == -1) {
    printf("%s:%d: buffer argument '%s' not found in event '%s'\n",
        __FILE__, __LINE__, buffer_varname, event_name);
    abort();
  }
  if (strcmp(f.type[ret->nb_rb_arg], "int") != 0) {
    printf("%s:%d: argument '%s' has wrong type (should be 'int')\n",
        __FILE__, __LINE__, nb_rb);
    abort();
  }
  if (strcmp(f.type[ret->N_RB_UL_arg], "int") != 0) {
    printf("%s:%d: argument '%s' has wrong type (should be 'int')\n",
        __FILE__, __LINE__, N_RB_UL);
    abort();
  }
  if (strcmp(f.type[ret->symbols_per_tti_arg], "int") != 0) {
    printf("%s:%d: argument '%s' has wrong type (should be 'int')\n",
        __FILE__, __LINE__, symbols_per_tti);
    abort();
  }
  if (strcmp(f.type[ret->buffer_arg], "buffer") != 0) {
    printf("%s:%d: argument '%s' has wrong type (should be 'buffer')\n",
        __FILE__, __LINE__, buffer_varname);
    abort();
  }

  return ret;
}
