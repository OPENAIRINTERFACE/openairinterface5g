#include "gui.h"
#include "gui_defs.h"
#include <stdio.h>
#include <stdlib.h>

#define MAX(a, b) ((a)>(b)?(a):(b))

static void repack(gui *g, widget *_this)
{
printf("REPACK container %p\n", _this);
  struct container_widget *this = _this;
  this->hints_are_valid = 0;
  return this->common.parent->repack(g, this->common.parent);
}

static void add_child(gui *g, widget *_this, widget *child, int position)
{
printf("ADD_CHILD container\n");
  struct container_widget *this = _this;
  this->hints_are_valid = 0;
  widget_add_child_internal(g, this, child, position);
}

static void compute_vertical_hints(struct gui *g,
    struct container_widget *this)
{
  struct widget_list *l;
  int cwidth, cheight;
  int allocated_width = 0, allocated_height = 0;

  /* get largest width */
  l = this->common.children;
  while (l) {
    l->item->hints(g, l->item, &cwidth, &cheight);
    if (cwidth > allocated_width) allocated_width = cwidth;
    allocated_height += cheight;
    l = l->next;
  }
  this->hint_width = allocated_width;
  this->hint_height = allocated_height;
  this->hints_are_valid = 1;
}

static void compute_horizontal_hints(struct gui *g,
    struct container_widget *this)
{
  struct widget_list *l;
  int cwidth, cheight;
  int allocated_width = 0, allocated_height = 0;

  /* get largest height */
  l = this->common.children;
  while (l) {
    l->item->hints(g, l->item, &cwidth, &cheight);
    if (cheight > allocated_height) allocated_height = cheight;
    allocated_width += cwidth;
    l = l->next;
  }
  this->hint_width = allocated_width;
  this->hint_height = allocated_height;
  this->hints_are_valid = 1;
}

static void vertical_allocate(gui *_gui, widget *_this,
    int x, int y, int width, int height)
{
printf("ALLOCATE container vertical %p\n", _this);
  int cy = 0;
  int cwidth, cheight;
  struct gui *g = _gui;
  struct container_widget *this = _this;
  struct widget_list *l;

  if (this->hints_are_valid == 1) goto hints_ok;

  compute_vertical_hints(g, this);

hints_ok:

  this->common.x = x;
  this->common.y = y;
  this->common.width = width;
  this->common.height = height;

  /* allocate */
  l = this->common.children;
  while (l) {
    l->item->hints(g, l->item, &cwidth, &cheight);
    l->item->allocate(g, l->item, this->common.x, this->common.y + cy,
        //this->hint_width, cheight);
        MAX(width, cwidth), cheight);
    cy += cheight;
    l = l->next;
  }

  if (cy != this->hint_height) ERR("reachable?\n");
}

static void horizontal_allocate(gui *_gui, widget *_this,
    int x, int y, int width, int height)
{
printf("ALLOCATE container horizontal %p\n", _this);
  int cx = 0;
  int cwidth, cheight;
  struct gui *g = _gui;
  struct container_widget *this = _this;
  struct widget_list *l;

  if (this->hints_are_valid == 1) goto hints_ok;

  compute_horizontal_hints(g, this);

hints_ok:

  this->common.x = x;
  this->common.y = y;
  this->common.width = width;
  this->common.height = height;

  /* allocate */
  l = this->common.children;
  while (l) {
    l->item->hints(g, l->item, &cwidth, &cheight);
    l->item->allocate(g, l->item, this->common.x + cx, this->common.y,
        cwidth, MAX(height, cheight)/* this->hint_height */);
    cx += cwidth;
    l = l->next;
  }

  if (cx != this->hint_width) ERR("reachable?\n");
}

static void vertical_hints(gui *_gui, widget *_w, int *width, int *height)
{
printf("HINTS container vertical %p\n", _w);
  struct gui *g = _gui;
  struct container_widget *this = _w;

  if (this->hints_are_valid) {
    *width = this->hint_width;
    *height = this->hint_height;
    return;
  }

  compute_vertical_hints(g, this);

  *width = this->hint_width;
  *height = this->hint_height;
}

static void horizontal_hints(gui *_gui, widget *_w, int *width, int *height)
{
printf("HINTS container horizontal %p\n", _w);
  struct gui *g = _gui;
  struct container_widget *this = _w;

  if (this->hints_are_valid) {
    *width = this->hint_width;
    *height = this->hint_height;
    return;
  }

  compute_horizontal_hints(g, this);

  *width = this->hint_width;
  *height = this->hint_height;
}

static void horizontal_button(gui *_g, widget *_this, int x, int y,
    int button, int up)
{
printf("BUTTON container horizontal %p xy %d %d button %d up %d\n", _this, x, y, button, up);
  struct gui *g = _g;
  struct container_widget *this = _this;
  struct widget_list *l;

  l = this->common.children;
  while (l) {
    if (l->item->x <= x && x < l->item->x + l->item->width) {
      l->item->button(g, l->item, x - l->item->x, y, button, up);
      break;
    }
    l = l->next;
  }
}

static void vertical_button(gui *_g, widget *_this, int x, int y,
    int button, int up)
{
printf("BUTTON container vertical %p xy %d %d button %d up %d\n", _this, x, y, button, up);
  struct gui *g = _g;
  struct container_widget *this = _this;
  struct widget_list *l;

  l = this->common.children;
  while (l) {
    if (l->item->y <= y && y < l->item->y + l->item->height) {
      l->item->button(g, l->item, x, y - l->item->y, button, up);
      break;
    }
    l = l->next;
  }
}

static void paint(gui *_gui, widget *_this)
{
printf("PAINT container\n");
  struct gui *g = _gui;
  struct widget *this = _this;
  struct widget_list *l;

  l = this->children;
  while (l) {
    l->item->paint(g, l->item);
    l = l->next;
  }
}

widget *new_container(gui *_gui, int vertical)
{
  struct gui *g = _gui;
  struct container_widget *w;

  glock(g);

  w = new_widget(g, CONTAINER, sizeof(struct container_widget));

  w->vertical = vertical;
  w->hints_are_valid = 0;

  w->common.paint     = paint;
  w->common.add_child = add_child;
  w->common.repack    = repack;

  if (vertical) {
    w->common.allocate  = vertical_allocate;
    w->common.hints     = vertical_hints;
    w->common.button    = vertical_button;
  } else {
    w->common.allocate  = horizontal_allocate;
    w->common.hints     = horizontal_hints;
    w->common.button    = horizontal_button;
  }

  gunlock(g);

  return w;
}
