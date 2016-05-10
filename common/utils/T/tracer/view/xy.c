#include "view.h"
#include "../utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include <string.h>

struct xy {
  view common;
  gui *g;
  widget *w;
  int plot;
  float refresh_rate;
  pthread_mutex_t lock;
  int length;
  float *x;
  float *y;
  int insert_point;
};

static void *xy_thread(void *_this)
{
  struct xy *this = _this;

  while (1) {
    if (pthread_mutex_lock(&this->lock)) abort();
    xy_plot_set_points(this->g, this->w, this->plot,
        this->length, this->x, this->y);
    if (pthread_mutex_unlock(&this->lock)) abort();
    sleepms(1000/this->refresh_rate);
  }

  return 0;
}

static void clear(view *this)
{
  /* TODO */
}

static void append(view *_this, float *x, float *y, int length)
{
  struct xy *this = (struct xy *)_this;
  int i;
  int ip;

  if (pthread_mutex_lock(&this->lock)) abort();

  ip = this->insert_point;

  /* TODO: optimize the copy */
  for (i = 0; i < length; i++) {
    this->x[ip] = x[i];
    this->y[ip] = y[i];
    ip++; if (ip == this->length) ip = 0;
  }

  this->insert_point = ip;

  if (pthread_mutex_unlock(&this->lock)) abort();
}

static void set(view *_this, char *name, ...)
{
  struct xy *this = (struct xy *)_this;
  va_list ap;

  if (!strcmp(name, "length")) {
    if (pthread_mutex_lock(&this->lock)) abort();

    va_start(ap, name);

    free(this->x);
    free(this->y);
    this->length = va_arg(ap, int);
    this->x = calloc(this->length, sizeof(float)); if (this->x==NULL)abort();
    this->y = calloc(this->length, sizeof(float)); if (this->y==NULL)abort();
    this->insert_point = 0;

    va_end(ap);

    if (pthread_mutex_unlock(&this->lock)) abort();
    return;
  }

  printf("%s:%d: unkown setting '%s'\n", __FILE__, __LINE__, name);
  abort();
}

view *new_view_xy(int length, float refresh_rate, gui *g, widget *w,
    int color)
{
  struct xy *ret = calloc(1, sizeof(struct xy));
  if (ret == NULL) abort();

  ret->common.clear = clear;
  ret->common.append = (void (*)(view *, ...))append;
  ret->common.set = set;

  ret->refresh_rate = refresh_rate;
  ret->g = g;
  ret->w = w;
  ret->plot = xy_plot_new_plot(g, w, color);

  ret->length = length;
  ret->x = calloc(length, sizeof(float)); if (ret->x == NULL) abort();
  ret->y = calloc(length, sizeof(float)); if (ret->y == NULL) abort();
  ret->insert_point = 0;

  if (pthread_mutex_init(&ret->lock, NULL)) abort();

  new_thread(xy_thread, ret);

  return (view *)ret;
}
