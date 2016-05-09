#include "defs.h"
#include "gui/gui.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

static struct {
  gui *g;
  widget *input_signal;      /* CC_id 0 antenna 0 */
  volatile int input_signal_length;  /* unit: byte */
  void *input_signal_iq;
  pthread_mutex_t input_signal_lock;
} eNB_data;

static void *gui_thread(void *g)
{
  gui_loop(g);
  exit(0);
}

static void *input_signal_plotter(void *_)
{
  short *iqbuf;
  float *x;
  float *y;
  int i;
  int length = eNB_data.input_signal_length / 4;

  x = calloc(1, sizeof(float) * eNB_data.input_signal_length / 4);
  y = calloc(1, sizeof(float) * eNB_data.input_signal_length / 4);
  if (x == NULL || y == NULL) abort();

  while (1) {
    usleep(100 * 1000);

    if (pthread_mutex_lock(&eNB_data.input_signal_lock)) abort();

    if (length * 4 != eNB_data.input_signal_length) {
      free(x);
      free(y);
      x = calloc(1, sizeof(float) * eNB_data.input_signal_length / 4);
      y = calloc(1, sizeof(float) * eNB_data.input_signal_length / 4);
      if (x == NULL || y == NULL) abort();
      length = eNB_data.input_signal_length / 4;
    }

    iqbuf = eNB_data.input_signal_iq;

    for (i = 0; i < length; i++) {
      x[i] = i;
      y[i] = 10*log10(1.0+(float)(iqbuf[2*i]*iqbuf[2*i]+
                                  iqbuf[2*i+1]*iqbuf[2*i+1]));
    }

    xy_plot_set_points(eNB_data.g, eNB_data.input_signal, 0,
        length, x, y);

    if (pthread_mutex_unlock(&eNB_data.input_signal_lock)) abort();
  }
}

void t_gui_start(void)
{
  gui *g = gui_init();

  widget *win = new_toplevel_window(g, 550, 140, "input signal");
  widget *plot = new_xy_plot(g, 512, 100, "eNB 0 input signal", 20);
  widget_add_child(g, win, plot, -1);
  xy_plot_set_range(g, plot, 0, 76800, 30, 70);
  xy_plot_new_plot(g, plot, FOREGROUND_COLOR);

  eNB_data.input_signal = plot;
  eNB_data.input_signal_length = 76800 * 4;
  eNB_data.input_signal_iq = calloc(1, 76800 * 4);
  if (eNB_data.input_signal_iq == NULL) abort();
  pthread_mutex_init(&eNB_data.input_signal_lock, NULL);

  eNB_data.g = g;

  new_thread(gui_thread, g);
  new_thread(input_signal_plotter, NULL);
}

void t_gui_set_input_signal(int eNB, int frame, int subframe, int antenna,
    int size, void *buf)
{
  if (pthread_mutex_lock(&eNB_data.input_signal_lock)) abort();

  if (eNB_data.input_signal_length != size * 10) {
    free(eNB_data.input_signal_iq);
    eNB_data.input_signal_length = size * 10;
    eNB_data.input_signal_iq = calloc(1, eNB_data.input_signal_length);
    if (eNB_data.input_signal_iq == NULL) abort();
  }

  memcpy((char *)eNB_data.input_signal_iq + subframe * size, buf, size);

  if (pthread_mutex_unlock(&eNB_data.input_signal_lock)) abort();
}
