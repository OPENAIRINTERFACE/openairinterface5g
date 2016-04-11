#include "gui.h"
#include "gui_defs.h"
#include <stdio.h>
#include <stdlib.h>

static void repack(gui *g, widget *_this)
{
printf("REPACK container %p\n", _this);
  struct container_widget *this = _this;
  this->hints_are_valid = 0;
  return this->common.parent->repack(g, this->common.parent);
}

static void add_child(gui *g, widget *this, widget *child, int position)
{
printf("ADD_CHILD container\n");
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
        width, cheight);
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
        cwidth, this->hint_height);
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
  } else {
    w->common.allocate  = horizontal_allocate;
    w->common.hints     = horizontal_hints;
  }

  gunlock(g);

  return w;
}
