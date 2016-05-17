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
  long *nano;
  int nanosize;
  int nanomaxsize;
  int line;
  int color;
};

struct tick {
  struct plot *p;              /* one plot per subview,
                                * so size is 'subcount'
                                * (see in struct time below)
                                */
};

struct time {
  view common;
  gui *g;
  widget *w;
  float refresh_rate;
  pthread_mutex_t lock;
  struct tick *t;              /* t is a circular list
                                * a tick lasts one second
                                */
  int tsize;                   /* size of t */
  int tstart;                  /* starting index in t */
  time_t tstart_time;          /* timestamp (in seconds) of starting in t */
  int subcount;                /* number of subviews */
  struct timespec latest_time; /* time of latest received tick */
  double pixel_length;        /* unit: nanosecond (maximum 1 hour/pixel) */
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

static void *time_thread(void *_this)
{
  struct time *this = _this;
  int width;
  int l;
  int i;
  int t;
  struct timespec tstart;
  struct timespec tnext;
  struct plot *p;
  int64_t pixel_length;

  while (1) {
    if (pthread_mutex_lock(&this->lock)) abort();

    timeline_get_width(this->g, this->w, &width);
    timeline_clear_silent(this->g, this->w);

    /* TODO: optimize/cleanup */

    /* use rounded pixel_length */
    pixel_length = this->pixel_length;

    tnext = time_add(this->latest_time,(struct timespec){tv_sec:0,tv_nsec:1});
    tstart = time_sub(tnext, nano_to_time(pixel_length * width));

    for (l = 0; l < this->subcount; l++) {
      for (i = 0; i < width; i++) {
        struct timespec tick_start, tick_end;
        tick_start = time_add(tstart, nano_to_time(pixel_length * i));
        tick_end = time_add(tick_start, nano_to_time(pixel_length-1));
        /* look for a nano between tick_start and tick_end */
        /* TODO: optimize */
        for (t = 0; t < this->tsize; t++) {
          int n;
          time_t current_second = this->tstart_time + t;
          time_t next_second = current_second + 1;
          struct timespec current_time =
              (struct timespec){tv_sec:current_second,tv_nsec:0};
          struct timespec next_time =
              (struct timespec){tv_sec:next_second,tv_nsec:0};
          if (time_cmp(tick_end, current_time) < 0) continue;
          if (time_cmp(tick_start, next_time) >= 0) continue;
          p = &this->t[(this->tstart + t) % this->tsize].p[l];
          for (n = 0; n < p->nanosize; n++) {
            struct timespec nano =
                (struct timespec){tv_sec:current_second,tv_nsec:p->nano[n]};
            if (time_cmp(tick_start, nano) <= 0 &&
                time_cmp(nano, tick_end) <= 0)
              goto gotit;
          }
        }
        continue;
gotit:
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
  double mul = 1.2;
  double pixel_length;
  int64_t old_px_len_rounded;

  if (pthread_mutex_lock(&this->lock)) abort();

  old_px_len_rounded = this->pixel_length;

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

  if (pthread_mutex_unlock(&this->lock)) abort();
}

view *new_view_time(int number_of_seconds, float refresh_rate,
    gui *g, widget *w)
{
  struct time *ret = calloc(1, sizeof(struct time));
  if (ret == NULL) abort();

  ret->refresh_rate = refresh_rate;
  ret->g = g;
  ret->w = w;

  ret->t = calloc(number_of_seconds, sizeof(struct tick));
  if (ret->t == NULL) abort();
  ret->tsize = number_of_seconds;
  ret->tstart = 0;
  ret->tstart_time = 0;
  ret->subcount = 0;
  ret->pixel_length = 10 * 1000000;   /* 10ms */

  register_notifier(g, "scrollup", w, scroll, ret);
  register_notifier(g, "scrolldown", w, scroll, ret);

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
  time_t         start_time, end_time;
  int            i, l;
  int            tpos;
  struct plot    *p;

  if (pthread_mutex_lock(&time->lock)) abort();

  start_time = time->tstart_time;
  end_time   = time->tstart_time + time->tsize - 1;

  /* useless test? */
  if (t.tv_sec < start_time) abort();

  /* tick out of current window? if yes, move window */
  /* if too far, free all */
  if (t.tv_sec > end_time && t.tv_sec - end_time > time->tsize) {
    for (l = 0; l < time->tsize; l++)
      for (i = 0; i < time->subcount; i++) {
        free(time->t[l].p[i].nano);
        time->t[l].p[i].nano = NULL;
        time->t[l].p[i].nanosize = 0;
        time->t[l].p[i].nanomaxsize = 0;
      }
    time->tstart = 0;
    time->tstart_time = t.tv_sec - (time->tsize-1);
    start_time = time->tstart_time;
    end_time   = time->tstart_time + time->tsize - 1;
  }
  while (t.tv_sec > end_time) {
    for (i = 0; i < time->subcount; i++) {
      free(time->t[time->tstart].p[i].nano);
      time->t[time->tstart].p[i].nano = NULL;
      time->t[time->tstart].p[i].nanosize = 0;
      time->t[time->tstart].p[i].nanomaxsize = 0;
    }
    time->tstart = (time->tstart+1) % time->tsize;
    time->tstart_time++;
    start_time++;
    end_time++;
  }

  tpos = (time->tstart + (t.tv_sec - time->tstart_time)) % time->tsize;
  p = &time->t[tpos].p[this->subview];

  /* can we get a new event with <= time than last in list? */
  if (p->nanosize != 0 && t.tv_nsec <= p->nano[p->nanosize-1])
    { printf("%s:%d: possible?\n", __FILE__, __LINE__);  abort(); }

  if (p->nanosize == p->nanomaxsize) {
    p->nanomaxsize += 4096;
    p->nano = realloc(p->nano, p->nanomaxsize * sizeof(long));
    if (p->nano == NULL) abort();
  }

  p->nano[p->nanosize] = t.tv_nsec;
  p->nanosize++;

  if (time->latest_time.tv_sec < t.tv_sec ||
      (time->latest_time.tv_sec == t.tv_sec &&
       time->latest_time.tv_nsec < t.tv_nsec))
    time->latest_time = t;

  if (pthread_mutex_unlock(&time->lock)) abort();
}

view *new_subview_time(view *_time, int line, int color)
{
  int i;
  struct time *time = (struct time *)_time;
  struct subtime *ret = calloc(1, sizeof(struct subtime));
  if (ret == NULL) abort();

  ret->common.append = (void (*)(view *, ...))append;

  if (pthread_mutex_lock(&time->lock)) abort();

  ret->parent = time;
  ret->line = line;
  ret->color = color;
  ret->subview = time->subcount;

  for (i = 0; i < time->tsize; i++) {
    time->t[i].p = realloc(time->t[i].p,
        (time->subcount + 1) * sizeof(struct plot));
    if (time->t[i].p == NULL) abort();
    time->t[i].p[time->subcount].nano = NULL;
    time->t[i].p[time->subcount].nanosize = 0;
    time->t[i].p[time->subcount].nanomaxsize = 0;
    time->t[i].p[time->subcount].line = line;
    time->t[i].p[time->subcount].color = color;
  }

  time->subcount++;

  if (pthread_mutex_unlock(&time->lock)) abort();

  return (view *)ret;
}
