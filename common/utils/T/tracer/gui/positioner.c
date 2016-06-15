#include "gui.h"
#include "gui_defs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void add_child(gui *g, widget *_this, widget *child, int position)
{
  LOGD("ADD_CHILD positioner\n");
  struct positioner_widget *this = _this;
  widget_add_child_internal(g, this, child, position);
}

static void del_child(gui *g, widget *_this, widget *child)
{
  LOGD("DEL_CHILD positioner\n");
  struct positioner_widget *this = _this;
  widget_del_child_internal(g, this, child);
}

static void allocate(
    gui *_g, widget *_this, int x, int y, int width, int height)
{
  LOGD("ALLOCATE positioner %p\n", _this);
  struct gui *g = _g;
  struct positioner_widget *this = _this;
  struct widget_list *l = this->common.children;
  int cwidth, cheight;

  this->common.x = x;
  this->common.y = y;
  this->common.width = width;
  this->common.height = height;

  if (l != NULL) {
    l->item->hints(g, l->item, &cwidth, &cheight);
    l->item->allocate(g, l->item, x+(width-cwidth)/2, y+(height-cheight)/2,
        cwidth, cheight);
  }
}

static void hints(gui *_gui, widget *_w, int *width, int *height)
{
  LOGD("HINTS positioner %p\n", _w);
  struct gui *g = _gui;
  struct positioner_widget *this = _w;
  struct widget_list *l = this->common.children;
  if (l != NULL)
    l->item->hints(g, l->item, width, height);
  else { *width = *height = 1; }
}

static void button(gui *_g, widget *_this, int x, int y,
    int key_modifiers, int button, int up)
{
  LOGD("BUTTON positioner %p xy %d %d button %d up %d\n", _this, x, y, button, up);
  struct gui *g = _g;
  struct positioner_widget *this = _this;
  struct widget_list *l = this->common.children;
  if (l != NULL)
    l->item->button(g, l->item, x, y, key_modifiers, button, up);
}

static void paint(gui *_gui, widget *_this)
{
  LOGD("PAINT positioner\n");
  struct gui *g = _gui;
  struct widget *this = _this;
  struct widget_list *l = this->children;
  if (l != NULL)
    l->item->paint(g, l->item);
}

widget *new_positioner(gui *_gui)
{
  struct gui *g = _gui;
  struct positioner_widget *w;

  glock(g);

  w = new_widget(g, POSITIONER, sizeof(struct positioner_widget));

  w->common.paint     = paint;
  w->common.add_child = add_child;
  w->common.del_child = del_child;
  w->common.allocate  = allocate;
  w->common.hints     = hints;
  w->common.button    = button;

  gunlock(g);

  return w;
}
