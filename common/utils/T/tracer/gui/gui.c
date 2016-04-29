#include "gui.h"
#include "gui_defs.h"
#include "x.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

void glock(gui *_gui)
{
  struct gui *g = _gui;
  if (pthread_mutex_lock(g->lock)) ERR("mutex error\n");
  xlock(g->x);
}

void gunlock(gui *_gui)
{
  struct gui *g = _gui;
  xunlock(g->x);
  if (pthread_mutex_unlock(g->lock)) ERR("mutex error\n");
}

int new_color(gui *_gui, char *color)
{
  struct gui *g = _gui;
  return x_new_color(g->x, color);
}
