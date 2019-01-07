#ifndef _FILTER_H_
#define _FILTER_H_

#include "event.h"

typedef void filter;

filter *filter_and(filter *a, filter *b);
filter *filter_eq(filter *a, filter *b);
filter *filter_int(int v);
filter *filter_evarg(void *database, char *event_name, char *varname);
filter *filter_evfun(void *database, int (*fun)(void *priv, int v),
    void *priv, filter *x);

int filter_eval(filter *f, event e);

void free_filter(filter *f);

#endif /* _FILTER_H_ */
