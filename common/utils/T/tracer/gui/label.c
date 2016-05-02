#include "gui.h"
#include "gui_defs.h"
#include "x.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void paint(gui *_gui, widget *_w)
{
  struct gui *g = _gui;
  struct label_widget *l = _w;
  LOGD("PAINT label '%s'\n", l->t);
  x_draw_string(g->x, g->xwin, l->color,
      l->common.x, l->common.y + l->baseline, l->t);
}

static void hints(gui *_gui, widget *_w, int *width, int *height)
{
  struct label_widget *l = _w;
  LOGD("HINTS label '%s'\n", l->t);
  *width = l->width;
  *height = l->height;
}

widget *new_label(gui *_gui, const char *label)
{
  struct gui *g = _gui;
  struct label_widget *w;

  glock(g);

  w = new_widget(g, LABEL, sizeof(struct label_widget));

  w->t = strdup(label);
  if (w->t == NULL) OOM;
  w->color = FOREGROUND_COLOR;

  x_text_get_dimensions(g->x, label, &w->width, &w->height, &w->baseline);

  w->common.paint = paint;
  w->common.hints = hints;

  gunlock(g);

  return w;
}
