#include "defs.h"
#include <X11/Xlib.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <unistd.h>

typedef struct {
  Display *d;
  Window w;
  Pixmap p;
  int width;
  int height;
  float *buf;
  short *iqbuf;
  int bufsize;
} plot;

static void *plot_thread(void *_p)
{
  float v;
  float *s;
  int i, j;
  plot *p = _p;
  while (1) {
    while (XPending(p->d)) {
      XEvent e;
      XNextEvent(p->d, &e);
    }

    {
      /* TODO: get white & black GCs at startup */
      GC gc;
      XGCValues v;
      gc = DefaultGC(p->d, DefaultScreen(p->d));
      v.foreground = WhitePixel(p->d, DefaultScreen(p->d));
      XChangeGC(p->d, gc, GCForeground, &v);
      XFillRectangle(p->d, p->p, gc, 0, 0, p->width, p->height);
      v.foreground = BlackPixel(p->d, DefaultScreen(p->d));
      XChangeGC(p->d, gc, GCForeground, &v);
    }

  {
    int i;
    for (i = 0; i < p->bufsize/2; i++)
      p->buf[i] = 10*log10(1.0+(float)(p->iqbuf[2*i]*p->iqbuf[2*i]+
                                       p->iqbuf[2*i+1]*p->iqbuf[2*i+1]));
  }
    s = p->buf;
    for (i = 0; i < 512; i++) {
      v = 0;
      for (j = 0; j < p->bufsize/2/512; j++, s++) v += *s;
      v /= p->bufsize/2/512;
      XDrawLine(p->d, p->p, DefaultGC(p->d, DefaultScreen(p->d)), i, 100, i, 100-v);
    }

    XCopyArea(p->d, p->p, p->w, DefaultGC(p->d, DefaultScreen(p->d)),
              0, 0, p->width, p->height, 0, 0);
    usleep(100*1000);
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
  if (pthread_create(&t, &att, f, data))
    { fprintf(stderr, "pthread_create err\n"); exit(1); }
  if (pthread_attr_destroy(&att))
    { fprintf(stderr, "pthread_attr_destroy err\n"); exit(1); }
}

void *make_plot(int width, int height, int bufsize, char *title)
{
  plot *p;
  Display *d;
  Window w;
  Pixmap pm;

  d = XOpenDisplay(0); if (d == NULL) abort();
  w = XCreateSimpleWindow(d, DefaultRootWindow(d), 0, 0, width, height,
        0, WhitePixel(d, DefaultScreen(d)), WhitePixel(d, DefaultScreen(d)));
  XSelectInput(d, w, ExposureMask);
  XMapWindow(d, w);

  XStoreName(d, w, title);

  pm = XCreatePixmap(d, w, width, height, DefaultDepth(d, DefaultScreen(d)));

  p = malloc(sizeof(*p)); if (p == NULL) abort();
  p->width = width;
  p->height = height;
  p->buf = malloc(sizeof(float) * bufsize); if (p->buf == NULL) abort();
  p->iqbuf = malloc(sizeof(short) * 2 * bufsize); if (p->buf == NULL) abort();
  p->bufsize = bufsize;

  p->d = d;
  p->w = w;
  p->p = pm;

  new_thread(plot_thread, p);

  return p;
}

void plot_set(void *_plot, float *data, int len, int pos)
{
  plot *p = _plot;
  memcpy(p->buf + pos, data, len * sizeof(float));
}

void iq_plot_set(void *_plot, short *data, int len, int pos)
{
  plot *p = _plot;
  memcpy(p->iqbuf + pos * 2, data, len * 2 * sizeof(short));
}
