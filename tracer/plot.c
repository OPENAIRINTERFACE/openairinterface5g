#include "defs.h"
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>

typedef struct {
  Display *d;
  Window w;
  Pixmap p;
  int width;
  int height;
  float *buf;
  short *iqbuf;
  int count;
  int type;
  volatile int iq_count;  /* for ULSCH IQ data */
  int iq_insert_pos;
  pthread_mutex_t lock;
  float zoom;
  int timer_pipe[2];
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

      /* TODO: get white & black GCs at startup */
      {
        GC gc;
        XGCValues v;
        gc = DefaultGC(p->d, DefaultScreen(p->d));
        v.foreground = WhitePixel(p->d, DefaultScreen(p->d));
        XChangeGC(p->d, gc, GCForeground, &v);
        XFillRectangle(p->d, p->p, gc, 0, 0, p->width, p->height);
        v.foreground = BlackPixel(p->d, DefaultScreen(p->d));
        XChangeGC(p->d, gc, GCForeground, &v);
      }

      if (pthread_mutex_lock(&p->lock)) abort();

      if (p->type == PLOT_VS_TIME) {
        for (i = 0; i < p->count; i++)
          p->buf[i] = 10*log10(1.0+(float)(p->iqbuf[2*i]*p->iqbuf[2*i]+
                                           p->iqbuf[2*i+1]*p->iqbuf[2*i+1]));
        s = p->buf;
        for (i = 0; i < 512; i++) {
          v = 0;
          for (j = 0; j < p->count/512; j++, s++) v += *s;
          v /= p->count/512;
          XDrawLine(p->d, p->p, DefaultGC(p->d, DefaultScreen(p->d)),
                    i, 100, i, 100-v);
        }
      } else if (p->type == PLOT_IQ_POINTS) {
        XPoint pts[p->iq_count];
        int count = p->iq_count;
        for (i = 0; i < count; i++) {
          pts[i].x = p->iqbuf[2*i]*p->zoom/20+50;
          pts[i].y = -p->iqbuf[2*i+1]*p->zoom/20+50;
        }
        XDrawPoints(p->d, p->p, DefaultGC(p->d, DefaultScreen(p->d)),
                    pts, count, CoordModeOrigin);
      }

      if (pthread_mutex_unlock(&p->lock)) abort();
    }

    if (redraw) {
      redraw = 0;
      XCopyArea(p->d, p->p, p->w, DefaultGC(p->d, DefaultScreen(p->d)),
                0, 0, p->width, p->height, 0, 0);
    }

    FD_ZERO(&rset);
    FD_SET(p->timer_pipe[0], &rset);
    FD_SET(xfd, &rset);
    if (select(maxfd+1, &rset, NULL, NULL, NULL) == -1) abort();
    if (FD_ISSET(p->timer_pipe[0], &rset)) {
      char b[512];
      read(p->timer_pipe[0], b, 512);
      replot = 1;
    }
  }

  return NULL;
}

static void new_thread(void *(*f)(void *), void *data)
{
  pthread_t t;
  pthread_attr_t att;

  if (pthread_attr_init(&att))
    { fprintf(stderr, "pthread_attr_init err\n"); exit(1); }
  if (pthread_attr_setdetachstate(&att, PTHREAD_CREATE_DETACHED))
    { fprintf(stderr, "pthread_attr_setdetachstate err\n"); exit(1); }
  if (pthread_attr_setstacksize(&att, 10000000))
    { fprintf(stderr, "pthread_attr_setstacksize err\n"); exit(1); }
  if (pthread_create(&t, &att, f, data))
    { fprintf(stderr, "pthread_create err\n"); exit(1); }
  if (pthread_attr_destroy(&att))
    { fprintf(stderr, "pthread_attr_destroy err\n"); exit(1); }
}

void *make_plot(int width, int height, int count, char *title, int type)
{
  plot *p;
  Display *d;
  Window w;
  Pixmap pm;

  d = XOpenDisplay(0); if (d == NULL) abort();
  w = XCreateSimpleWindow(d, DefaultRootWindow(d), 0, 0, width, height,
        0, WhitePixel(d, DefaultScreen(d)), WhitePixel(d, DefaultScreen(d)));
  XSelectInput(d, w, ExposureMask | ButtonPressMask);
  XMapWindow(d, w);

  XStoreName(d, w, title);

  pm = XCreatePixmap(d, w, width, height, DefaultDepth(d, DefaultScreen(d)));

  p = malloc(sizeof(*p)); if (p == NULL) abort();
  p->width = width;
  p->height = height;
  if (type == PLOT_VS_TIME) {
    p->buf = malloc(sizeof(float) * count); if (p->buf == NULL) abort();
    p->iqbuf = malloc(sizeof(short) * count * 2); if(p->iqbuf==NULL)abort();
  } else {
    p->buf = NULL;
    p->iqbuf = malloc(sizeof(short) * count * 2); if(p->iqbuf==NULL)abort();
  }
  p->count = count;

  p->d = d;
  p->w = w;
  p->p = pm;
  p->type = type;

  p->iq_count = 0;
  p->iq_insert_pos = 0;

  p->zoom = 1;

  pthread_mutex_init(&p->lock, NULL);

  if (pipe(p->timer_pipe)) abort();

  new_thread(plot_thread, p);
  new_thread(timer_thread, p);

  return p;
}

void plot_set(void *_plot, float *data, int len, int pos)
{
  plot *p = _plot;
  if (pthread_mutex_lock(&p->lock)) abort();
  memcpy(p->buf + pos, data, len * sizeof(float));
  if (pthread_mutex_unlock(&p->lock)) abort();
}

void iq_plot_set(void *_plot, short *data, int count, int pos)
{
  plot *p = _plot;
  if (pthread_mutex_lock(&p->lock)) abort();
  memcpy(p->iqbuf + pos * 2, data, count * 2 * sizeof(short));
  if (pthread_mutex_unlock(&p->lock)) abort();
}

void iq_plot_set_sized(void *_plot, short *data, int count)
{
  plot *p = _plot;
  if (pthread_mutex_lock(&p->lock)) abort();
  memcpy(p->iqbuf, data, count * 2 * sizeof(short));
  p->iq_count = count;
  if (pthread_mutex_unlock(&p->lock)) abort();
}

void iq_plot_add_point_loop(void *_plot, short i, short q)
{
  plot *p = _plot;
  if (pthread_mutex_lock(&p->lock)) abort();
  p->iqbuf[p->iq_insert_pos*2] = i;
  p->iqbuf[p->iq_insert_pos*2+1] = q;
  if (p->iq_count != p->count) p->iq_count++;
  p->iq_insert_pos++;
  if (p->iq_insert_pos == p->count) p->iq_insert_pos = 0;
  if (pthread_mutex_unlock(&p->lock)) abort();
}
