#include "gui.h"
#include "gui_defs.h"
#include "x.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void paint(gui *_gui, widget *_this)
{
  struct gui *g = _gui;
  struct text_list_widget *this = _this;
  int i, j;
  LOGD("PAINT text_list %p xywh %d %d %d %d starting line %d allocated nlines %d text_count %d\n", _this, this->common.x, this->common.y, this->common.width, this->common.height, this->starting_line, this->allocated_nlines, this->text_count);
  x_fill_rectangle(g->x, g->xwin, this->background_color,
      this->common.x, this->common.y,
      this->common.width, this->common.height);
  for (i = 0, j = this->starting_line;
       i < this->allocated_nlines && j < this->text_count; i++, j++)
    x_draw_clipped_string(g->x, g->xwin, this->color[j],
        this->common.x,
        this->common.y + i * this->line_height + this->baseline,
        this->text[j],
        this->common.x, this->common.y,
        this->common.width, this->common.height);
}

static void hints(gui *_gui, widget *_w, int *width, int *height)
{
  struct text_list_widget *w = _w;
  *width = w->wanted_width;
  *height = w->wanted_nlines * w->line_height;
  LOGD("HINTS text_list wh %d %d\n", *width, *height);
}

static void allocate(
    gui *gui, widget *_this, int x, int y, int width, int height)
{
  struct text_list_widget *this = _this;
  this->common.x = x;
  this->common.y = y;
  this->common.width = width;
  this->common.height = height;
  this->allocated_nlines = height / this->line_height;
  LOGD("ALLOCATE text_list %p xywh %d %d %d %d nlines %d\n", this, x, y, width, height, this->allocated_nlines);
}

static void button(gui *_g, widget *_this, int x, int y, int button, int up)
{
  struct gui *g = _g;
  struct text_list_widget *this = _this;
  LOGD("BUTTON text_list %p xy %d %d button %d up %d\n", _this, x, y, button, up);
  /* scroll up */
  if (button == 4 && up == 0) {
    gui_notify(g, "scrollup", _this, NULL);
  }
  /* scroll down */
  if (button == 5 && up == 0) {
    gui_notify(g, "scrolldown", _this, NULL);
  }
  /* button 1/2/3 click */
  if (button >= 1 && button <= 3 && up == 0) {
    int line = this->starting_line + y / this->line_height;
    if (line >= 0 && line < this->text_count)
      gui_notify(g, "click", _this, (int[2]){ line, button });
  }
}

widget *new_text_list(gui *_gui, int width, int nlines, int bgcol)
{
  struct gui *g = _gui;
  struct text_list_widget *w;
  int dummy;

  glock(g);

  w = new_widget(g, TEXT_LIST, sizeof(struct text_list_widget));

  w->wanted_nlines = nlines;
  x_text_get_dimensions(g->x, ".", &dummy, &w->line_height, &w->baseline);
  w->background_color = bgcol;
  w->wanted_width = width;

  w->common.paint = paint;
  w->common.hints = hints;
  w->common.allocate = allocate;

  w->common.button = button;

  gunlock(g);

  return w;
}

/*************************************************************************/
/*                             public functions                          */
/*************************************************************************/

static void _text_list_add(gui *_gui, widget *_this, const char *text,
    int position, int color, int silent)
{
  struct gui *g = _gui;
  struct text_list_widget *this = _this;

  glock(g);

  if (position < 0) position = this->text_count;
  if (position > this->text_count) position = this->text_count;

  this->text_count++;
  this->text = realloc(this->text, this->text_count * sizeof(char *));
  if (this->text == NULL) OOM;
  this->color = realloc(this->color, this->text_count * sizeof(int));
  if (this->color == NULL) OOM;

  memmove(this->text + position + 1, this->text + position,
          (this->text_count-1 - position) * sizeof(char *));
  memmove(this->color + position + 1, this->color + position,
          (this->text_count-1 - position) * sizeof(int));

  this->text[position] = strdup(text); if (this->text[position] == NULL) OOM;
  this->color[position] = color;

  if (!silent) send_event(g, DIRTY, this->common.id);

  gunlock(g);
}

static void _text_list_del(gui *_gui, widget *_this, int position, int silent)
{
  struct gui *g = _gui;
  struct text_list_widget *this = _this;

  glock(g);

  /* TODO: useful check? */
  if (this->text_count == 0) goto done;

  if (position < 0) position = this->text_count;
  if (position > this->text_count-1) position = this->text_count-1;

  free(this->text[position]);

  memmove(this->text + position, this->text + position + 1,
          (this->text_count-1 - position) * sizeof(char *));
  memmove(this->color + position, this->color + position + 1,
          (this->text_count-1 - position) * sizeof(int));

  this->text_count--;
  this->text = realloc(this->text, this->text_count * sizeof(char *));
  if (this->text == NULL) OOM;
  this->color = realloc(this->color, this->text_count * sizeof(int));
  if (this->color == NULL) OOM;

  if (!silent) send_event(g, DIRTY, this->common.id);

done:
  gunlock(g);
}

void text_list_add(gui *gui, widget *this, const char *text, int position,
    int color)
{
  _text_list_add(gui, this, text, position, color, 0);
}

void text_list_del(gui *gui, widget *this, int position)
{
  _text_list_del(gui, this, position, 0);
}

void text_list_add_silent(gui *gui, widget *this, const char *text,
    int position, int color)
{
  _text_list_add(gui, this, text, position, color, 1);
}

void text_list_del_silent(gui *gui, widget *this, int position)
{
  _text_list_del(gui, this, position, 1);
}

void text_list_state(gui *_gui, widget *_this,
    int *visible_lines, int *start_line, int *number_of_lines)
{
  struct gui *g = _gui;
  struct text_list_widget *this = _this;

  glock(g);

  *visible_lines   = this->allocated_nlines;
  *start_line      = this->starting_line;
  *number_of_lines = this->text_count;

  gunlock(g);
}

void text_list_set_start_line(gui *_gui, widget *_this, int line)
{
  struct gui *g = _gui;
  struct text_list_widget *this = _this;

  glock(g);

  this->starting_line = line;

  send_event(g, DIRTY, this->common.id);

  gunlock(g);
}

void text_list_get_line(gui *_gui, widget *_this, int line,
    char **text, int *color)
{
  struct gui *g = _gui;
  struct text_list_widget *this = _this;

  glock(g);

  if (line < 0 || line >= this->text_count) {
    *text = NULL;
    *color = -1;
  } else {
    *text = this->text[line];
    *color = this->color[line];
  }

  gunlock(g);
}

void text_list_set_color(gui *_gui, widget *_this, int line, int color)
{
  struct gui *g = _gui;
  struct text_list_widget *this = _this;

  glock(g);

  if (line >= 0 && line < this->text_count) {
    this->color[line] = color;

    send_event(g, DIRTY, this->common.id);
  }

  gunlock(g);
}
