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
  x_draw_string(g->x, g->xwin, DEFAULT_FONT, l->color,
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

  x_text_get_dimensions(g->x, DEFAULT_FONT, label,
      &w->width, &w->height, &w->baseline);

  w->common.paint = paint;
  w->common.hints = hints;

  gunlock(g);

  return w;
}

static void button(gui *gui, widget *_this, int x, int y,
    int key_modifiers, int button, int up)
{
  LOGD("BUTTON label %p xy %d %d button %d up %d\n", _this, x, y, button, up);

  if (up != 0) return;

  gui_notify(gui, "click", _this, &button);
}

/* we could use default_button, but it's in widget.c, so, well... */
static void no_button(gui *gui, widget *_this, int x, int y,
    int key_modifiers, int button, int up)
{
  /* do nothing */
}

/*************************************************************************/
/*                             public functions                          */
/*************************************************************************/

void label_set_clickable(gui *_g, widget *_this, int clickable)
{
  struct gui *g = _g;
  struct label_widget *this = _this;

  glock(g);

  if (clickable)
    this->common.button = button;
  else
    this->common.button = no_button;

  gunlock(g);
}

void label_set_text(gui *_g, widget *_this, char *text)
{
  struct gui *g = _g;
  struct label_widget *this = _this;

  glock(g);

  free(this->t);
  this->t = strdup(text); if (this->t == NULL) OOM;

  x_text_get_dimensions(g->x, DEFAULT_FONT, text,
      &this->width, &this->height, &this->baseline);

  send_event(g, REPACK, this->common.id);

  gunlock(g);
}
