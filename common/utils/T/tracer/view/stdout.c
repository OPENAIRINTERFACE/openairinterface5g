#include "view.h"
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>

struct stdout {
  view common;
  pthread_mutex_t lock;
};

static void clear(view *this)
{
  /* do nothing */
}

static void append(view *_this, char *s)
{
  struct stdout *this = (struct stdout *)_this;
  if (pthread_mutex_lock(&this->lock)) abort();
  printf("%s\n", s);
  if (pthread_mutex_unlock(&this->lock)) abort();
}

view *new_view_stdout(void)
{
  struct stdout *ret = calloc(1, sizeof(struct stdout));
  if (ret == NULL) abort();

  ret->common.clear = clear;
  ret->common.append = (void (*)(view *, ...))append;

  if (pthread_mutex_init(&ret->lock, NULL)) abort();

  return (view *)ret;
}
