#include "defs.h"
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <sys/select.h>
#include <stdarg.h>

typedef struct {
  float *buf;
  short *iqbuf;
  int count;
  int type;
  volatile int iq_count;  /* for ULSCH IQ data */
  int iq_insert_pos;
  GC g;
} data;

typedef struct {
  Display *d;
  Window w;
  Pixmap px;
  GC bg;
  int width;
  int height;
  pthread_mutex_t lock;
  float zoom;
  int timer_pipe[2];
  data *p;             /* list of plots */
  int nplots;
} plot;

static void *timer_thread(void *_p)
{
  plot *p = _p;
  char c;

  while (1) {
    /* more or less 10Hz */
    usleep(100*1000);
    c = 1;
    if (write(p->timer_pipe[1], &c, 1) != 1) abort();
  }

  return NULL;
}

static void *plot_thread(void *_p)
{
  float v;
  float *s;
  int i, j;
  plot *p = _p;
  int redraw = 0;
  int replot = 0;
  fd_set rset;
  int xfd = ConnectionNumber(p->d);
  int maxfd = xfd > p->timer_pipe[0] ? xfd : p->timer_pipe[0];
  int pp;

  while (1) {
    while (XPending(p->d)) {
      XEvent e;
      XNextEvent(p->d, &e);
      switch (e.type) {
      case ButtonPress:
        /* button 4: zoom out */
        if (e.xbutton.button == 4) { p->zoom = p->zoom * 1.25; replot = 1; }
        /* button 5: zoom in */
        if (e.xbutton.button == 5) { p->zoom = p->zoom * 0.8; replot = 1; }
        printf("zoom: %f\n", p->zoom);
        break;
      case Expose: redraw = 1; break;
      }
    }

    if (replot == 1) {
      replot = 0;
      redraw = 1;

      if (pthread_mutex_lock(&p->lock)) abort();

      XFillRectangle(p->d, p->px, p->bg, 0, 0, p->width, p->height);

      for (pp = 0; pp < p->nplots; pp++) {
        if (p->p[pp].type == PLOT_MINMAX) {
          s = p->p[pp].buf;
          for (i = 0; i < 512; i++) {
            int min = *s;
            int max = *s;
            for (j = 0; j < p->p[pp].count/512; j++, s++) {
              if (*s < min) min = *s;
              if (*s > max) max = *s;
            }
            XDrawLine(p->d, p->px, p->p[pp].g, i, 100-min, i, 100-max);
          }
        } else if (p->p[pp].type == PLOT_VS_TIME) {
          for (i = 0; i < p->p[pp].count; i++)
            p->p[pp].buf[i] =
                10*log10(1.0+(float)(p->p[pp].iqbuf[2*i]*p->p[pp].iqbuf[2*i]+
                p->p[pp].iqbuf[2*i+1]*p->p[pp].iqbuf[2*i+1]));
          s = p->p[pp].buf;
          for (i = 0; i < 512; i++) {
            v = 0;
            for (j = 0; j < p->p[pp].count/512; j++, s++) v += *s;
            v /= p->p[pp].count/512;
            XDrawLine(p->d, p->px, p->p[pp].g, i, 100, i, 100-v);
          }
        } else if (p->p[pp].type == PLOT_IQ_POINTS) {
          XPoint pts[p->p[pp].iq_count];
          int count = p->p[pp].iq_count;
          for (i = 0; i < count; i++) {
            pts[i].x = p->p[pp].iqbuf[2*i]*p->zoom/20+50;
            pts[i].y = -p->p[pp].iqbuf[2*i+1]*p->zoom/20+50;
          }
          XDrawPoints(p->d, p->px, p->p[pp].g, pts, count, CoordModeOrigin);
        }
      }

      if (pthread_mutex_unlock(&p->lock)) abort();
    }

    if (redraw) {
      redraw = 0;
      XCopyArea(p->d, p->px, p->w, DefaultGC(p->d, DefaultScreen(p->d)),
                0, 0, p->width, p->height, 0, 0);
    }

    XFlush(p->d);

    FD_ZERO(&rset);
    FD_SET(p->timer_pipe[0], &rset);
    FD_SET(xfd, &rset);
    if (select(maxfd+1, &rset, NULL, NULL, NULL) == -1) abort();
    if (FD_ISSET(p->timer_pipe[0], &rset)) {
      char b[512];
      if (read(p->timer_pipe[0], b, 512) <= 0) abort();
      replot = 1;
    }
  }

  return NULL;
}

void *make_plot(int width, int height, char *title, int nplots, ...)
{
  plot *p;
  Display *d;
  Window w;
  Pixmap pm;
  int i;
  va_list ap;
  XGCValues gcv;

  p = malloc(sizeof(*p)); if (p == NULL) abort();

  d = XOpenDisplay(0); if (d == NULL) abort();
  w = XCreateSimpleWindow(d, DefaultRootWindow(d), 0, 0, width, height,
        0, WhitePixel(d, DefaultScreen(d)), WhitePixel(d, DefaultScreen(d)));
  XSelectInput(d, w, ExposureMask | ButtonPressMask);
  XMapWindow(d, w);

  {
    XSetWindowAttributes att;
    att.backing_store = Always;
    XChangeWindowAttributes(d, w, CWBackingStore, &att);
  }

  XStoreName(d, w, title);

  p->bg = XCreateGC(d, w, 0, NULL);
  XCopyGC(d, DefaultGC(d, DefaultScreen(d)), -1L, p->bg);
  gcv.foreground = WhitePixel(d, DefaultScreen(d));
  XChangeGC(d, p->bg, GCForeground, &gcv);

  pm = XCreatePixmap(d, w, width, height, DefaultDepth(d, DefaultScreen(d)));

  p->width = width;
  p->height = height;
  p->p = malloc(nplots * sizeof(data)); if (p->p == NULL) abort();

  va_start(ap, nplots);
  for (i = 0; i < nplots; i++) {
    int count;
    int type;
    char *color;
    XColor rcol, scol;

    count = va_arg(ap, int);
    type = va_arg(ap, int);
    color = va_arg(ap, char *);

    p->p[i].g = XCreateGC(d, w, 0, NULL);
    XCopyGC(d, DefaultGC(d, DefaultScreen(d)), -1L, p->p[i].g);
    if (XAllocNamedColor(d, DefaultColormap(d, DefaultScreen(d)),
                         color, &scol, &rcol)) {
      gcv.foreground = scol.pixel;
      XChangeGC(d, p->p[i].g, GCForeground, &gcv);
    } else {
      printf("could not allocate color '%s'\n", color);
      abort();
    }

    if (type == PLOT_VS_TIME) {
      p->p[i].buf = malloc(sizeof(float) * count);
      if (p->p[i].buf == NULL) abort();
      p->p[i].iqbuf = malloc(sizeof(short) * count * 2);
      if(p->p[i].iqbuf==NULL)abort();
    } else if (type == PLOT_MINMAX) {
      p->p[i].buf = malloc(sizeof(float) * count);
      if (p->p[i].buf == NULL) abort();
      p->p[i].iqbuf = NULL;
    } else {
      p->p[i].buf = NULL;
      p->p[i].iqbuf = malloc(sizeof(short) * count * 2);
      if(p->p[i].iqbuf==NULL)abort();
    }
    p->p[i].count = count;
    p->p[i].type = type;
    p->p[i].iq_count = 0;
    p->p[i].iq_insert_pos = 0;
  }
  va_end(ap);

  p->d = d;
  p->w = w;
  p->px = pm;

  p->zoom = 1;
  p->nplots = nplots;

  pthread_mutex_init(&p->lock, NULL);

  if (pipe(p->timer_pipe)) abort();

  new_thread(plot_thread, p);
  new_thread(timer_thread, p);

  return p;
}

void plot_set(void *_plot, float *data, int len, int pos, int pp)
{
  plot *p = _plot;
  if (pthread_mutex_lock(&p->lock)) abort();
  memcpy(p->p[pp].buf + pos, data, len * sizeof(float));
  if (pthread_mutex_unlock(&p->lock)) abort();
}

void iq_plot_set(void *_plot, short *data, int count, int pos, int pp)
{
  plot *p = _plot;
  if (pthread_mutex_lock(&p->lock)) abort();
  memcpy(p->p[pp].iqbuf + pos * 2, data, count * 2 * sizeof(short));
  if (pthread_mutex_unlock(&p->lock)) abort();
}

void iq_plot_set_sized(void *_plot, short *data, int count, int pp)
{
  plot *p = _plot;
  if (pthread_mutex_lock(&p->lock)) abort();
  memcpy(p->p[pp].iqbuf, data, count * 2 * sizeof(short));
  p->p[pp].iq_count = count;
  if (pthread_mutex_unlock(&p->lock)) abort();
}

void iq_plot_add_iq_point_loop(void *_plot, short i, short q, int pp)
{
  plot *p = _plot;
  if (pthread_mutex_lock(&p->lock)) abort();
  p->p[pp].iqbuf[p->p[pp].iq_insert_pos*2] = i;
  p->p[pp].iqbuf[p->p[pp].iq_insert_pos*2+1] = q;
  if (p->p[pp].iq_count != p->p[pp].count) p->p[pp].iq_count++;
  p->p[pp].iq_insert_pos++;
  if (p->p[pp].iq_insert_pos == p->p[pp].count) p->p[pp].iq_insert_pos = 0;
  if (pthread_mutex_unlock(&p->lock)) abort();
}

void iq_plot_add_energy_point_loop(void *_plot, int e, int pp)
{
  plot *p = _plot;
  if (pthread_mutex_lock(&p->lock)) abort();
  p->p[pp].buf[p->p[pp].iq_insert_pos] = e;
  if (p->p[pp].iq_count != p->p[pp].count) p->p[pp].iq_count++;
  p->p[pp].iq_insert_pos++;
  if (p->p[pp].iq_insert_pos == p->p[pp].count) p->p[pp].iq_insert_pos = 0;
  if (pthread_mutex_unlock(&p->lock)) abort();
}
