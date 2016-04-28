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
printf("PAINT text_list %p xywh %d %d %d %d\n", _this, this->common.x, this->common.y, this->common.width, this->common.height);
  x_fill_rectangle(g->x, g->xwin, this->background_color,
      this->common.x, this->common.y,
      this->common.width, this->common.height);
  for (i = 0, j = this->starting_line;
       i < this->allocated_nlines && j < this->text_count; i++, j++)
    x_draw_string(g->x, g->xwin, FOREGROUND_COLOR,
        this->common.x,
        this->common.y + i * this->line_height + this->baseline,
        this->text[j]);
}

static void hints(gui *_gui, widget *_w, int *width, int *height)
{
  struct text_list_widget *w = _w;
  *width = w->wanted_width;
  *height = w->wanted_nlines * w->line_height;
printf("HINTS text_list wh %d %d\n", *width, *height);
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
printf("ALLOCATE text_list %p xywh %d %d %d %d nlines %d\n", this, x, y, width, height, this->allocated_nlines);
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

  gunlock(g);

  return w;
}

/*************************************************************************/
/*                             public functions                          */
/*************************************************************************/

void text_list_add(gui *_gui, widget *_this, const char *text, int position)
{
  struct gui *g = _gui;
  struct text_list_widget *this = _this;

  glock(g);

  if (position < 0) position = this->text_count;
  if (position > this->text_count) position = this->text_count;

  this->text_count++;
  this->text = realloc(this->text, this->text_count * sizeof(char *));
  if (this->text == NULL) OOM;

  memmove(this->text + position + 1, this->text + position,
          (this->text_count-1 - position) * sizeof(char *));

  this->text[position] = strdup(text); if (this->text[position] == NULL) OOM;

  send_event(g, DIRTY, this->common.id);

  gunlock(g);
}

void text_list_del(gui *_gui, widget *_this, int position)
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

  this->text_count--;
  this->text = realloc(this->text, this->text_count * sizeof(char *));
  if (this->text == NULL) OOM;

  send_event(g, DIRTY, this->common.id);

done:
  gunlock(g);
}
