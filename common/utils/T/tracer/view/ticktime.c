#include "view.h"
#include "../utils.h"
#include "logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

/* TODO: some code is identical/almost identical to time.c, merge/factorize */

/****************************************************************************/
/*                               tick timeview                              */
/****************************************************************************/

struct plot {
  struct timespec *tick;
  int ticksize;
  int tickmaxsize;
  int tickstart;
  int line;
  int color;
};

struct ticktime {
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
  struct timespec tick_latest_time;
  int tick_latest_frame;
  int tick_latest_subframe;
  void *clock_ticktime;      /* data for the clock tick, see below */
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

static int interval_empty(struct ticktime *this, int sub,
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

static void *ticktime_thread(void *_this)
{
  struct ticktime *this = _this;
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
        int x[3] = {i==0?i:i-1, i, i==width-1?i:i+1};
        timeline_add_points_silent(this->g, this->w, p->line, p->color, x, 3);
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
  struct ticktime *this = private;
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
  struct ticktime *this = private;
  int *d = notification_data;
  int button = *d;

  if (button == 3) this->autoscroll = 0;
  if (button == 1) this->autoscroll = 1;
}

view *new_view_ticktime(float refresh_rate, gui *g, widget *w)
{
  struct ticktime *ret = calloc(1, sizeof(struct ticktime));
  if (ret == NULL) abort();

  ret->refresh_rate = refresh_rate;
  ret->g = g;
  ret->w = w;

  ret->p = NULL;
  ret->psize = 0;

  ret->autoscroll = 1;

  ret->tick_latest_time.tv_sec = 1;

  /* default pixel length: 10ms */
  ret->pixel_length = 10 * 1000000;

  register_notifier(g, "scrollup", w, scroll, ret);
  register_notifier(g, "scrolldown", w, scroll, ret);
  register_notifier(g, "click", w, click, ret);

  if (pthread_mutex_init(&ret->lock, NULL)) abort();

  new_thread(ticktime_thread, ret);

  return (view *)ret;
}

/****************************************************************************/
/*                          subticktimeview                                 */
/****************************************************************************/

struct subticktime {
  view common;
  struct ticktime *parent;
  int line;
  int color;
  int subview;
};

static void append(view *_this, struct timespec t, int frame, int subframe)
{
  struct subticktime *this = (struct subticktime *)_this;
  struct ticktime    *ticktime = this->parent;
  struct plot        *p = &ticktime->p[this->subview];
  int                i;
  struct timespec    swap;
  int64_t            diff;

  if (pthread_mutex_lock(&ticktime->lock)) abort();

  /* get time with respect to latest known tick time */
  diff = (frame*10 + subframe) -
      (ticktime->tick_latest_frame*10 + ticktime->tick_latest_subframe);
  if (diff > 1024*10/2) diff -= 1024*10;
  else if (diff < -1024*10/2) diff += 1024*10;
  if (diff < 0)
    t = time_sub(ticktime->tick_latest_time, nano_to_time(-diff * 1000000));
  else
    t = time_add(ticktime->tick_latest_time, nano_to_time(diff * 1000000));

  if (p->ticksize < p->tickmaxsize) {
    p->tick[p->ticksize] = t;
    p->ticksize++;
  } else {
    p->tick[p->tickstart] = t;
    p->tickstart = (p->tickstart + 1) % p->ticksize;
  }

  /* due to adjustment of the time, array may not be ordered anymore */
  for (i = p->ticksize-2; i >= 0; i--) {
    int prev = (p->tickstart + i) % p->ticksize;
    int cur = (prev + 1) % p->ticksize;
    if (time_cmp(p->tick[prev], p->tick[cur]) <= 0) break;
    swap = p->tick[prev];
    p->tick[prev] = p->tick[cur];
    p->tick[cur] = swap;
  }

  if (time_cmp(ticktime->latest_time, t) < 0)
    ticktime->latest_time = t;

  if (pthread_mutex_unlock(&ticktime->lock)) abort();
}

view *new_subview_ticktime(view *_time, int line, int color, int size)
{
  struct ticktime *ticktime = (struct ticktime *)_time;
  struct subticktime *ret = calloc(1, sizeof(struct subticktime));
  if (ret == NULL) abort();

  ret->common.append = (void (*)(view *, ...))append;

  if (pthread_mutex_lock(&ticktime->lock)) abort();

  ret->parent = ticktime;
  ret->line = line;
  ret->color = color;
  ret->subview = ticktime->psize;

  ticktime->p = realloc(ticktime->p,
      (ticktime->psize + 1) * sizeof(struct plot));
  if (ticktime->p == NULL) abort();
  ticktime->p[ticktime->psize].tick = calloc(size, sizeof(struct timespec));
  if (ticktime->p[ticktime->psize].tick == NULL) abort();
  ticktime->p[ticktime->psize].ticksize = 0;
  ticktime->p[ticktime->psize].tickmaxsize = size;
  ticktime->p[ticktime->psize].tickstart = 0;
  ticktime->p[ticktime->psize].line = line;
  ticktime->p[ticktime->psize].color = color;

  ticktime->psize++;

  if (pthread_mutex_unlock(&ticktime->lock)) abort();

  return (view *)ret;
}

/****************************************************************************/
/*                               clock tick                                 */
/****************************************************************************/

struct clock_ticktime {
  view common;
  struct ticktime *parent;
};

static void clock_append(view *_this, struct timespec t,
    int frame, int subframe)
{
  struct clock_ticktime *this = (struct clock_ticktime *)_this;
  struct ticktime *tt = this->parent;
  int64_t diff;

  if (subframe == 10) { subframe = 0; frame = (frame + 1) % 1024; }

  if (pthread_mutex_lock(&tt->lock)) abort();

  /* get time relative to latest known tick time */
  /* In normal conditions diff is 1 but if the user pauses reception of events
   * it may be anything. Let's take only positive values.
   */
  diff = (frame*10 + subframe) -
      (tt->tick_latest_frame*10 + tt->tick_latest_subframe);
  if (diff < 0) diff += 1024*10;
  tt->tick_latest_time = time_add(tt->tick_latest_time,
      nano_to_time(diff * 1000000));
  tt->tick_latest_frame = frame;
  tt->tick_latest_subframe = subframe;

  if (time_cmp(tt->latest_time, tt->tick_latest_time) < 0)
    tt->latest_time = tt->tick_latest_time;

  if (pthread_mutex_unlock(&tt->lock)) abort();
}

void ticktime_set_tick(view *_ticktime, void *logger)
{
  struct ticktime *ticktime = (struct ticktime *)_ticktime;
  struct clock_ticktime *n;

  if (pthread_mutex_lock(&ticktime->lock)) abort();

  free(ticktime->clock_ticktime);
  n = ticktime->clock_ticktime = calloc(1, sizeof(struct clock_ticktime));
  if (n == NULL) abort();

  n->common.append = (void (*)(view *, ...))clock_append;
  n->parent = ticktime;

  logger_add_view(logger, (view *)n);

  if (pthread_mutex_unlock(&ticktime->lock)) abort();
}
