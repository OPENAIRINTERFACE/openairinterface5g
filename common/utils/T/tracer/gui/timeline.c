#include "gui.h"
#include "gui_defs.h"
#include "x.h"
#include <stdio.h>
#include <stdlib.h>

static void paint(gui *_gui, widget *_this)
{
  struct gui *g = _gui;
  struct timeline_widget *this = _this;
  int i;
  int j;

  for (i = 0; i < this->n; i++) {
    x_fill_rectangle(g->x, g->xwin, this->s[i].background,
        this->common.x, this->common.y + i * this->subline_height,
        this->common.width, this->subline_height);
    for (j = 0; j < this->s[i].width; j++)
      if (this->s[i].color[j] != -1)
        x_draw_line(g->x, g->xwin, this->s[i].color[j],
            this->common.x + j, this->common.y + i * this->subline_height,
            this->common.x + j, this->common.y + this->subline_height -1
                + i * this->subline_height);
  }

  LOGD("PAINT timeline xywh %d %d %d %d\n", this->common.x, this->common.y, this->common.width, this->common.height);
}

static void hints(gui *_gui, widget *_w, int *width, int *height)
{
  struct timeline_widget *w = _w;
  *width = w->wanted_width;
  *height = w->n * w->subline_height;
  LOGD("HINTS timeline wh %d %d\n", *width, *height);
}

static void allocate(gui *_gui, widget *_this,
    int x, int y, int width, int height)
{
  struct timeline_widget *this = _this;
  int i;
  int j;
  this->common.x = x;
  this->common.y = y;
  this->common.width = width;
  this->common.height = height;
  LOGD("ALLOCATE timeline %p xywh %d %d %d %d\n", this, x, y, width, height);
  for (i = 0; i < this->n; i++) {
    this->s[i].width = width;
    this->s[i].color = realloc(this->s[i].color, width * sizeof(int));
    if (this->s[i].color == NULL) abort();
    for (j = 0; j < width; j++) this->s[i].color[j] = -1;
  }
  gui_notify(_gui, "resize", _this, &width);
}

static void button(gui *_g, widget *_this, int x, int y,
    int key_modifiers, int button, int up)
{
  struct gui *g = _g;
  struct timeline_widget *w = _this;
  int d[3];
  LOGD("BUTTON timeline %p xy %d %d button %d up %d\n", _this, x, y, button, up);
  /* scroll up */
  if (button == 4 && up == 0) {
    d[0] = x - w->common.x;
    d[1] = y - w->common.y;
    d[2] = key_modifiers;
    gui_notify(g, "scrollup", _this, d);
  }
  /* scroll down */
  if (button == 5 && up == 0) {
    d[0] = x - w->common.x;
    d[1] = y - w->common.y;
    d[2] = key_modifiers;
    gui_notify(g, "scrolldown", _this, d);
  }
  /* button 1/2/3 */
  if ((button == 1 || button == 2 || button == 3) && up == 0) {
    gui_notify(g, "click", _this, &button);
  }
}

/*************************************************************************/
/*                           creation function                           */
/*************************************************************************/

widget *new_timeline(gui *_gui, int width, int number_of_sublines,
    int subline_height)
{
  struct gui *g = _gui;
  struct timeline_widget *w;
  int i;
  int j;

  glock(g);

  w = new_widget(g, TIMELINE, sizeof(struct timeline_widget));

  w->wanted_width = width;
  w->n = number_of_sublines;
  w->s = calloc(w->n, sizeof(struct timeline_subline)); if (w->s == NULL) OOM;
  w->subline_height = subline_height;

  /* initialize colors */
  for (i = 0; i < w->n; i++) {
    w->s[i].width = width;
    w->s[i].color = calloc(width, sizeof(int));
    if (w->s[i].color == NULL) abort();
    for (j = 0; j < width; j++) w->s[i].color[j] = -1;
    w->s[i].background = BACKGROUND_COLOR;
  }

  w->common.paint = paint;
  w->common.hints = hints;
  w->common.allocate = allocate;
  w->common.button = button;

  gunlock(g);

  return w;
}

/*************************************************************************/
/*                           public functions                            */
/*************************************************************************/

static void _timeline_clear(gui *_gui, widget *_this, int silent)
{
  struct gui *g = _gui;
  struct timeline_widget *this = _this;
  int i;
  int j;

  glock(g);

  for (i = 0; i < this->n; i++)
    for (j = 0; j < this->s[i].width; j++)
      this->s[i].color[j] = -1;

  if (silent == 0)
    send_event(g, DIRTY, this->common.id);

  gunlock(g);
}

void timeline_clear(gui *_gui, widget *_this)
{
  _timeline_clear(_gui, _this, 0);
}

void timeline_clear_silent(gui *_gui, widget *_this)
{
  _timeline_clear(_gui, _this, 1);
}

static void _timeline_add_points(gui *_gui, widget *_this, int subline,
    int color, int *x, int len, int silent)
{
  struct gui *g = _gui;
  struct timeline_widget *this = _this;
  int i;

  glock(g);

  for (i = 0; i < len; i++) {
    if (x[i] >= this->s[subline].width) { WARN("out of bounds\n"); continue; }
    this->s[subline].color[x[i]] = color;
  }

  if (silent == 0)
    send_event(g, DIRTY, this->common.id);

  gunlock(g);
}

void timeline_add_points(gui *_gui, widget *_this, int subline, int color,
    int *x, int len)
{
  _timeline_add_points(_gui, _this, subline, color, x, len, 0);
}

void timeline_add_points_silent(gui *_gui, widget *_this, int subline,
    int color, int *x, int len)
{
  _timeline_add_points(_gui, _this, subline, color, x, len, 1);
}

void timeline_set_subline_background_color(gui *_gui, widget *_this,
    int subline, int color)
{
  struct gui *g = _gui;
  struct timeline_widget *this = _this;

  glock(g);

  this->s[subline].background = color;

  send_event(g, DIRTY, this->common.id);

  gunlock(g);
}

void timeline_get_width(gui *_gui, widget *_this, int *width)
{
  struct gui *g = _gui;
  struct timeline_widget *this = _this;

  glock(g);

  *width = this->common.width == 0 ? this->wanted_width : this->common.width;

  gunlock(g);
}
