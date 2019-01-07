#include "gui.h"
#include "gui_defs.h"
#include "x.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void paint(gui *_gui, widget *_w)
{
  struct gui *g = _gui;
  struct textarea_widget *t = _w;
  LOGD("PAINT textarea '%s'\n", t->t);
  x_fill_rectangle(g->x, g->xwin, BACKGROUND_COLOR,
      t->common.x, t->common.y,
      t->common.width, t->common.height);
  x_draw_clipped_string(g->x, g->xwin, DEFAULT_FONT, t->color,
      t->common.x + t->common.width - t->text_width,
      t->common.y + t->baseline, t->t,
      t->common.x, t->common.y,
      t->common.width, t->common.height);
}

static void hints(gui *_gui, widget *_w, int *width, int *height)
{
  struct textarea_widget *t = _w;
  LOGD("HINTS textarea '%s'\n", t->t);
  *width = t->wanted_width;
  *height = t->wanted_height;
}

widget *new_textarea(gui *_gui, int width, int height, int maxsize)
{
  struct gui *g = _gui;
  struct textarea_widget *w;
  int _;

  glock(g);

  w = new_widget(g, TEXTAREA, sizeof(struct textarea_widget));

  w->t = calloc(maxsize, 1);
  if (w->t == NULL) OOM;
  w->tmaxsize = maxsize;
  w->wanted_width = width;
  w->wanted_height = height;
  w->color = FOREGROUND_COLOR;
  w->text_width = 0;

  x_text_get_dimensions(g->x, DEFAULT_FONT, "jlM",
      &_, &_, &w->baseline);

  w->common.paint = paint;
  w->common.hints = hints;

  gunlock(g);

  return w;
}

/*************************************************************************/
/*                             public functions                          */
/*************************************************************************/

void textarea_set_text(gui *_g, widget *_this, char *text)
{
  struct gui *g = _g;
  struct textarea_widget *this = _this;
  int _;
  int len = strlen(text);
  if (len >= this->tmaxsize) {
    fprintf(stderr, "ERROR: string '%s' too big for textarea\n", text);
    return;
  }

  glock(g);

  strcpy(this->t, text);
  x_text_get_dimensions(g->x, DEFAULT_FONT, text,
      &this->text_width, &_, &this->baseline);
  send_event(g, DIRTY, this->common.id);

  gunlock(g);
}
