#include "view.h"
#include "../utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>

struct scrolltti {
  view common;
  gui *g;
  widget *w;
  widget *throughput_textarea;
  int plot;
  float refresh_rate;
  pthread_mutex_t lock;
  unsigned long data[1000];
  unsigned long total;  /* sum data[0..999] to have smoother value printed */
  float xout[1000];
  float yout[1000];
  int insert_point;
};

/* this array is used to get Y range 1000, 2000, 5000, 10000, ... */
static int tolog[11] = { -1, 1, 2, 5, 5, 5, 10, 10, 10, 10, 10 };

static void *scrolltti_thread(void *_this)
{
  struct scrolltti *this = _this;
  int i;
  int p;
  float max, mlog;
  char o[64];

  while (1) {
    if (pthread_mutex_lock(&this->lock)) abort();
    /* TODO: optimize */
    p = this->insert_point;
    max = 0;
    for (i = 0; i < 1000; i++) {
      this->xout[i] = i;
      this->yout[i] = this->data[p];
      if (this->data[p] > max) max = this->data[p];
      p = (p + 1) % 1000;
    }
    bps(o, this->total/1000., "b/s");
    textarea_set_text(this->g, this->throughput_textarea, o);
    /* for Y range we want 1000, 2000, 5000, 10000, 20000, 50000, etc. */
    if (max < 1000) max = 1000;
    mlog = pow(10, floor(log10(max)));
    max = tolog[(int)ceil(max/mlog)] * mlog;
    xy_plot_set_range(this->g, this->w, 0, 1000, 0, max);
    xy_plot_set_points(this->g, this->w, this->plot,
        1000, this->xout, this->yout);
    if (pthread_mutex_unlock(&this->lock)) abort();
    sleepms(1000/this->refresh_rate);
  }

  return 0;
}

static void clear(view *this)
{
  /* TODO */
}

static void append(view *_this, int frame, int subframe, double value)
{
  struct scrolltti *this = (struct scrolltti *)_this;

  if (pthread_mutex_lock(&this->lock)) abort();
  this->total -= this->data[this->insert_point];
  this->data[this->insert_point] = value;
  this->total += this->data[this->insert_point];
  this->insert_point = (this->insert_point + 1) % 1000;
  if (pthread_mutex_unlock(&this->lock)) abort();
}

view *new_view_scrolltti(float refresh_rate, gui *g, widget *w, int color,
    widget *throughput_textarea)
{
  struct scrolltti *ret = calloc(1, sizeof(struct scrolltti));
  if (ret == NULL) abort();

  ret->common.clear = clear;
  ret->common.append = (void (*)(view *, ...))append;

  ret->refresh_rate = refresh_rate;
  ret->g = g;
  ret->w = w;
  ret->throughput_textarea = throughput_textarea;
  ret->plot = xy_plot_new_plot(g, w, color);

  if (pthread_mutex_init(&ret->lock, NULL)) abort();

  new_thread(scrolltti_thread, ret);

  return (view *)ret;
}
