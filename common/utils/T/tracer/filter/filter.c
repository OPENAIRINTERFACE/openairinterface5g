#include "filter.h"
#include "event.h"
#include "database.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

struct filter {
  union {
    struct { struct filter *a, *b; } op2;
    int v;
    struct { int event_type; int arg_index; } evarg;
    struct { int (*fun)(void *priv, int v); void *priv;
             struct filter *x; } evfun;
  } v;

  int (*eval)(struct filter *this, event e);
  void (*free)(struct filter *this);
};

/****************************************************************************/
/*                     evaluation functions                                 */
/****************************************************************************/

int eval_and(struct filter *f, event e)
{
  if (f->v.op2.a->eval(f->v.op2.a, e) == 0) return 0;
  return f->v.op2.b->eval(f->v.op2.b, e);
}

int eval_eq(struct filter *f, event e)
{
  int a = f->v.op2.a->eval(f->v.op2.a, e);
  int b = f->v.op2.b->eval(f->v.op2.b, e);
  return a == b;
}

int eval_int(struct filter *f, event e)
{
  return f->v.v;
}

int eval_evarg(struct filter *f, event e)
{
  if (e.type != f->v.evarg.event_type) {
    printf("%s:%d:%s: bad event type\n", __FILE__, __LINE__, __FUNCTION__);
    abort();
  }
  if (e.e[f->v.evarg.arg_index].type != EVENT_INT) {
    printf("%s:%d:%s: bad event argtype; has to be 'int'\n",
        __FILE__, __LINE__, __FUNCTION__);
    abort();
  }
  return e.e[f->v.evarg.arg_index].i;
}

int eval_evfun(struct filter *f, event e)
{
  return f->v.evfun.fun(f->v.evfun.priv, f->v.evfun.x->eval(f->v.evfun.x, e));
}

/****************************************************************************/
/*                     free memory functions                                */
/****************************************************************************/

void free_op2(struct filter *f)
{
  free_filter(f->v.op2.a);
  free_filter(f->v.op2.b);
  free(f);
}

void free_evfun(struct filter *f)
{
  free_filter(f->v.evfun.x);
  free(f);
}

void free_noop(struct filter *f)
{
  free(f);
}

/****************************************************************************/
/*                     filter construction/destruction functions            */
/****************************************************************************/

filter *filter_and(filter *a, filter *b)
{
  struct filter *ret = calloc(1, sizeof(struct filter));
  if (ret == NULL) abort();
  ret->eval = eval_and;
  ret->free = free_op2;
  ret->v.op2.a = a;
  ret->v.op2.b = b;
  return ret;
}

filter *filter_eq(filter *a, filter *b)
{
  struct filter *ret = calloc(1, sizeof(struct filter));
  if (ret == NULL) abort();
  ret->eval = eval_eq;
  ret->free = free_op2;
  ret->v.op2.a = a;
  ret->v.op2.b = b;
  return ret;
}

filter *filter_int(int v)
{
  struct filter *ret = calloc(1, sizeof(struct filter));
  if (ret == NULL) abort();
  ret->eval = eval_int;
  ret->free = free_noop;
  ret->v.v = v;
  return ret;
}

filter *filter_evarg(void *database, char *event_name, char *varname)
{
  struct filter *ret;
  int event_id;
  database_event_format f;
  int i;

  ret = calloc(1, sizeof(struct filter)); if (ret == NULL) abort();

  event_id = event_id_from_name(database, event_name);
  f = get_format(database, event_id);

  ret->eval = eval_evarg;
  ret->free = free_noop;
  ret->v.evarg.event_type = event_id;
  ret->v.evarg.arg_index = -1;

  for (i = 0; i < f.count; i++) {
    if (strcmp(f.name[i], varname) != 0) continue;
    ret->v.evarg.arg_index = i;
    break;
  }
  if (ret->v.evarg.arg_index == -1) {
    printf("%s:%d:%s: event '%s' has no argument '%s'\n",
        __FILE__, __LINE__, __FUNCTION__, event_name, varname);
    abort();
  }

  return ret;
}

filter *filter_evfun(void *database, int (*fun)(void *priv, int v),
    void *priv, filter *x)
{
  struct filter *ret = calloc(1, sizeof(struct filter));
  if (ret == NULL) abort();
  ret->eval = eval_evfun;
  ret->free = free_evfun;
  ret->v.evfun.fun  = fun;
  ret->v.evfun.priv = priv;
  ret->v.evfun.x    = x;
  return ret;
}

void free_filter(filter *_f)
{
  struct filter *f;
  if (_f == NULL) return;
  f = _f;
  f->free(f);
}

/****************************************************************************/
/*                     eval function                                        */
/****************************************************************************/

int filter_eval(filter *_f, event e)
{
  struct filter *f = _f;
  return f->eval(f, e);
}
