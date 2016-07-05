#include "view.h"
#include "../utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

/****************************************************************************/
/*                              timeview                                    */
/****************************************************************************/

struct plot {
  struct timespec *tick;
  int ticksize;
  int tickmaxsize;
  int tickstart;
  int line;
  int color;
};

struct time {
  view common;
  gui *g;
  widget *w;
  float refresh_rate;
  pthread_mutex_t lock;
  struct plot *p;
  int psize;
  double pixel_length;        /* unit: nanosecond (maximum 1 hour/pixel) */
  struct timespec latest_time;
  struct timespec start_time;
  int autoscroll;
};

/* TODO: put that function somewhere else (utils.c) */
static struct timespec time_add(struct timespec a, struct timespec b)
{
  struct timespec ret;
  ret.tv_sec = a.tv_sec + b.tv_sec;
  ret.tv_nsec = a.tv_nsec + b.tv_nsec;
  if (ret.tv_nsec > 1000000000) {
    ret.tv_sec++;
    ret.tv_nsec -= 1000000000;
  }
  return ret;
}

/* TODO: put that function somewhere else (utils.c) */
static struct timespec time_sub(struct timespec a, struct timespec b)
{
  struct timespec ret;
  if (a.tv_nsec < b.tv_nsec) {
    ret.tv_nsec = (int64_t)a.tv_nsec - (int64_t)b.tv_nsec + 1000000000;
    ret.tv_sec = a.tv_sec - b.tv_sec - 1;
  } else {
    ret.tv_nsec = a.tv_nsec - b.tv_nsec;
    ret.tv_sec = a.tv_sec - b.tv_sec;
  }
  return ret;
}

/* TODO: put that function somewhere else (utils.c) */
static struct timespec nano_to_time(int64_t n)
{
  struct timespec ret;
  ret.tv_sec = n / 1000000000;
  ret.tv_nsec = n % 1000000000;
  return ret;
}

/* TODO: put that function somewhere else (utils.c) */
static int time_cmp(struct timespec a, struct timespec b)
{
  if (a.tv_sec < b.tv_sec) return -1;
  if (a.tv_sec > b.tv_sec) return 1;
  if (a.tv_nsec < b.tv_nsec) return -1;
  if (a.tv_nsec > b.tv_nsec) return 1;
  return 0;
}

static int interval_empty(struct time *this, int sub,
    struct timespec start, struct timespec end)
{
  int a, b, mid;
  int i;

  if (this->p[sub].ticksize == 0) return 1;

  /* look for a tick larger than start and smaller than end */
  a = 0;
  b = this->p[sub].ticksize - 1;
  while (b >= a) {
    mid = (a+b) / 2;
    i = (this->p[sub].tickstart + mid) % this->p[sub].ticksize;
    if (time_cmp(this->p[sub].tick[i], start) < 0) a = mid + 1;
    else if (time_cmp(this->p[sub].tick[i], end) > 0) b = mid - 1;
    else return 0;
  }
  return 1;
}

static void *time_thread(void *_this)
{
  struct time *this = _this;
  int width;
  int l;
  int i;
  struct timespec tstart;
  struct timespec tnext;
  struct plot *p;
  int64_t pixel_length;

  while (1) {
    if (pthread_mutex_lock(&this->lock)) abort();

    timeline_get_width(this->g, this->w, &width);
    timeline_clear_silent(this->g, this->w);

    /* TODO: optimize? */

    /* use rounded pixel_length */
    pixel_length = this->pixel_length;

    if (this->autoscroll) {
      tnext = time_add(this->latest_time,
          (struct timespec){tv_sec:0,tv_nsec:1});
      tstart = time_sub(tnext, nano_to_time(pixel_length * width));
      this->start_time = tstart;
    } else {
      tstart = this->start_time;
      tnext = time_add(tstart, nano_to_time(pixel_length * width));
    }

    for (l = 0; l < this->psize; l++) {
      for (i = 0; i < width; i++) {
        struct timespec tick_start, tick_end;
        tick_start = time_add(tstart, nano_to_time(pixel_length * i));
        tick_end = time_add(tick_start, nano_to_time(pixel_length-1));
        if (interval_empty(this, l, tick_start, tick_end))
          continue;
        p = &this->p[l];
        /* TODO: only one call */
        timeline_add_points_silent(this->g, this->w, p->line, p->color, &i, 1);
      }
    }

    widget_dirty(this->g, this->w);

    if (pthread_mutex_unlock(&this->lock)) abort();
    sleepms(1000 / this->refresh_rate);
  }

  return 0;
}

static void scroll(void *private, gui *g,
    char *notification, widget *w, void *notification_data)
{
  struct time *this = private;
  int *d = notification_data;
  int x = d[0];
  int key_modifiers = d[2];
  double mul = 1.2;
  double pixel_length;
  int64_t old_px_len_rounded;
  struct timespec t;
  int scroll_px;
  int width;

  if (pthread_mutex_lock(&this->lock)) abort();

  old_px_len_rounded = this->pixel_length;

  /* scroll if control+wheel, zoom otherwise */

  if (key_modifiers & KEY_CONTROL) {
    timeline_get_width(this->g, this->w, &width);
    if (width < 2) width = 2;
    scroll_px = 100;
    if (scroll_px > width - 1) scroll_px = width - 1;
    if (!strcmp(notification, "scrolldown"))
      this->start_time = time_add(this->start_time,
          nano_to_time(scroll_px * old_px_len_rounded));
    else
      this->start_time = time_sub(this->start_time,
          nano_to_time(scroll_px * old_px_len_rounded));
    goto end;
  }

  if (!strcmp(notification, "scrollup")) mul = 1 / mul;

again:
  pixel_length = this->pixel_length * mul;
  if (pixel_length < 1) pixel_length = 1;
  if (pixel_length > (double)3600 * 1000000000)
    pixel_length = (double)3600 * 1000000000;

  this->pixel_length = pixel_length;

  /* due to rounding, we may need to zoom by more than 1.2 with
   * very close lookup, otherwise the user zooming command won't
   * be visible (say length is 2.7, zoom in, new length is 2.25,
   * and rounding is 2, same value, no change, no feedback to user => bad)
   * TODO: make all this cleaner
   */
  if (pixel_length != 1 && pixel_length != (double)3600 * 1000000000 &&
      (int64_t)pixel_length == old_px_len_rounded)
    goto again;

  t = time_add(this->start_time, nano_to_time(x * old_px_len_rounded));
  this->start_time = time_sub(t, nano_to_time(x * (int64_t)pixel_length));

end:
  if (pthread_mutex_unlock(&this->lock)) abort();
}

static void click(void *private, gui *g,
    char *notification, widget *w, void *notification_data)
{
  struct time *this = private;
  int *d = notification_data;
  int button = *d;

  if (button == 3) this->autoscroll = 0;
  if (button == 1) this->autoscroll = 1;
}

view *new_view_time(int number_of_seconds, float refresh_rate,
    gui *g, widget *w)
{
  struct time *ret = calloc(1, sizeof(struct time));
  if (ret == NULL) abort();

  ret->refresh_rate = refresh_rate;
  ret->g = g;
  ret->w = w;

  ret->p = NULL;
  ret->psize = 0;

  ret->autoscroll = 1;

  /* default pixel length: 10ms */
  ret->pixel_length = 10 * 1000000;

  register_notifier(g, "scrollup", w, scroll, ret);
  register_notifier(g, "scrolldown", w, scroll, ret);
  register_notifier(g, "click", w, click, ret);

  if (pthread_mutex_init(&ret->lock, NULL)) abort();

  new_thread(time_thread, ret);

  return (view *)ret;
}

/****************************************************************************/
/*                            subtimeview                                   */
/****************************************************************************/

struct subtime {
  view common;
  struct time *parent;
  int line;
  int color;
  int subview;
};

static void append(view *_this, struct timespec t)
{
  struct subtime *this = (struct subtime *)_this;
  struct time    *time = this->parent;
  struct plot    *p = &time->p[this->subview];

  if (pthread_mutex_lock(&time->lock)) abort();

  if (p->ticksize < p->tickmaxsize) {
    p->tick[p->ticksize] = t;
    p->ticksize++;
  } else {
    p->tick[p->tickstart] = t;
    p->tickstart = (p->tickstart + 1) % p->ticksize;
  }

  if (time_cmp(time->latest_time, t) < 0)
    time->latest_time = t;

  if (pthread_mutex_unlock(&time->lock)) abort();
}

view *new_subview_time(view *_time, int line, int color, int size)
{
  struct time *time = (struct time *)_time;
  struct subtime *ret = calloc(1, sizeof(struct subtime));
  if (ret == NULL) abort();

  ret->common.append = (void (*)(view *, ...))append;

  if (pthread_mutex_lock(&time->lock)) abort();

  ret->parent = time;
  ret->line = line;
  ret->color = color;
  ret->subview = time->psize;

  time->p = realloc(time->p,
      (time->psize + 1) * sizeof(struct plot));
  if (time->p == NULL) abort();
  time->p[time->psize].tick = calloc(size, sizeof(struct timespec));
  if (time->p[time->psize].tick == NULL) abort();
  time->p[time->psize].ticksize = 0;
  time->p[time->psize].tickmaxsize = size;
  time->p[time->psize].tickstart = 0;
  time->p[time->psize].line = line;
  time->p[time->psize].color = color;

  time->psize++;

  if (pthread_mutex_unlock(&time->lock)) abort();

  return (view *)ret;
}
