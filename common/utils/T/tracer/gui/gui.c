#include "gui.h"
#include "gui_defs.h"
#include "x.h"
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

static volatile int locked = 0;

void __cyg_profile_func_enter (void *func,  void *caller)
{
  if (locked == 0) abort();
  printf("E %p %p %lu\n", func, caller, time(NULL));
}

void __cyg_profile_func_exit (void *func, void *caller)
{
  if (locked == 0) abort();
  printf("X %p %p %lu\n", func, caller, time(NULL));
}

void glock(gui *_gui)
{
  struct gui *g = _gui;
  if (pthread_mutex_lock(g->lock)) ERR("mutex error\n");
  locked = 1;
}

void gunlock(gui *_gui)
{
  struct gui *g = _gui;
  locked = 0;
  if (pthread_mutex_unlock(g->lock)) ERR("mutex error\n");
}

int new_color(gui *_gui, char *color)
{
  struct gui *g = _gui;
  int ret;

  glock(g);

  ret = x_new_color(g->x, color);

  gunlock(g);

  return ret;
}
