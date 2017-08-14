#include "gui.h"
#include "gui_defs.h"
#include <stdio.h>

static void paint(gui *_gui, widget *_w)
{
  /* nothing */
}

static void hints(gui *_gui, widget *_w, int *width, int *height)
{
  struct space_widget *w = _w;
  LOGD("HINTS space %p\n", w);
  *width = w->wanted_width;
  *height = w->wanted_height;
}

widget *new_space(gui *_gui, int width, int height)
{
  struct gui *g = _gui;
  struct space_widget *w;

  glock(g);

  w = new_widget(g, SPACE, sizeof(struct space_widget));

  w->wanted_width = width;
  w->wanted_height = height;

  w->common.paint = paint;
  w->common.hints = hints;

  gunlock(g);

  return w;
}
