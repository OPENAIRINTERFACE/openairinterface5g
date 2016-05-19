#include "gui.h"
#include "gui_defs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX(a, b) ((a)>(b)?(a):(b))

static void repack(gui *g, widget *_this)
{
  LOGD("REPACK container %p\n", _this);
  struct container_widget *this = _this;
  this->hints_are_valid = 0;
  return this->common.parent->repack(g, this->common.parent);
}

static void add_child(gui *g, widget *_this, widget *child, int position)
{
  LOGD("ADD_CHILD container\n");
  struct container_widget *this = _this;

  this->hints_are_valid = 0;
  widget_add_child_internal(g, this, child, position);

  /* initially not growable */
  this->growable = realloc(this->growable, (this->nchildren+1)*sizeof(int));
  if (this->growable == NULL) abort();

  if (position == -1) position = this->nchildren;

  memmove(this->growable + position+1, this->growable + position,
          (this->nchildren - position) * sizeof(int));

  this->growable[position] = 0;

  this->nchildren++;
}

static void del_child(gui *g, widget *_this, widget *child)
{
  LOGD("DEL_CHILD container\n");
  struct container_widget *this = _this;
  int position = widget_get_child_position(g, _this, child);

  this->hints_are_valid = 0;
  widget_del_child_internal(g, this, child);

  memmove(this->growable + position, this->growable + position+1,
          (this->nchildren - position - 1) * sizeof(int));

  this->growable = realloc(this->growable, (this->nchildren-1)*sizeof(int));
  if (this->nchildren != 1 && this->growable == NULL) abort();

  this->nchildren--;
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
  LOGD("ALLOCATE container vertical %p\n", _this);
  int cy = 0;
  int cwidth, cheight;
  struct gui *g = _gui;
  struct container_widget *this = _this;
  struct widget_list *l;
  int over_pixels = 0;
  int i;

  if (this->hints_are_valid == 1) goto hints_ok;

  compute_vertical_hints(g, this);

hints_ok:

  this->common.x = x;
  this->common.y = y;
  this->common.width = width;
  this->common.height = height;

  /* TODO: some pixels won't be allocated, take care of it? */
  if (height > this->hint_height) {
    int ngrowable = 0;
    for (i = 0; i < this->nchildren; i++) if (this->growable[i]) ngrowable++;
    if (ngrowable)
      over_pixels = (height - this->hint_height) / ngrowable;
  }

  /* allocate */
  l = this->common.children;
  i = 0;
  while (l) {
    int allocated_height;
    l->item->hints(g, l->item, &cwidth, &cheight);
    allocated_height = cheight + (this->growable[i] ? over_pixels : 0);
    l->item->allocate(g, l->item, this->common.x, this->common.y + cy,
        MAX(width, cwidth), allocated_height);
    cy += allocated_height;
    l = l->next;
    i++;
  }

//  if (cy != this->hint_height) ERR("reachable?\n");
}

static void horizontal_allocate(gui *_gui, widget *_this,
    int x, int y, int width, int height)
{
  LOGD("ALLOCATE container horizontal %p\n", _this);
  int cx = 0;
  int cwidth, cheight;
  struct gui *g = _gui;
  struct container_widget *this = _this;
  struct widget_list *l;
  int over_pixels = 0;
  int i;

  if (this->hints_are_valid == 1) goto hints_ok;

  compute_horizontal_hints(g, this);

hints_ok:

  this->common.x = x;
  this->common.y = y;
  this->common.width = width;
  this->common.height = height;

  /* TODO: some pixels won't be allocated, take care of it? */
  if (width > this->hint_width) {
    int ngrowable = 0;
    for (i = 0; i < this->nchildren; i++) if (this->growable[i]) ngrowable++;
    if (ngrowable)
      over_pixels = (width - this->hint_width) / ngrowable;
  }

  /* allocate */
  l = this->common.children;
  i = 0;
  while (l) {
    int allocated_width;
    l->item->hints(g, l->item, &cwidth, &cheight);
    allocated_width = cwidth + (this->growable[i] ? over_pixels : 0);
    l->item->allocate(g, l->item, this->common.x + cx, this->common.y,
        allocated_width, MAX(height, cheight)/* this->hint_height */);
    cx += allocated_width;
    l = l->next;
    i++;
  }

//  if (cx != this->hint_width) ERR("reachable?\n");
}

static void vertical_hints(gui *_gui, widget *_w, int *width, int *height)
{
  LOGD("HINTS container vertical %p\n", _w);
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
  LOGD("HINTS container horizontal %p\n", _w);
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
    int key_modifiers, int button, int up)
{
  LOGD("BUTTON container horizontal %p xy %d %d button %d up %d\n", _this, x, y, button, up);
  struct gui *g = _g;
  struct container_widget *this = _this;
  struct widget_list *l;

  l = this->common.children;
  while (l) {
    if (l->item->x <= x && x < l->item->x + l->item->width) {
      l->item->button(g, l->item, x, y, key_modifiers, button, up);
      break;
    }
    l = l->next;
  }
}

static void vertical_button(gui *_g, widget *_this, int x, int y,
    int key_modifiers, int button, int up)
{
  LOGD("BUTTON container vertical %p xy %d %d button %d up %d\n", _this, x, y, button, up);
  struct gui *g = _g;
  struct container_widget *this = _this;
  struct widget_list *l;

  l = this->common.children;
  while (l) {
    if (l->item->y <= y && y < l->item->y + l->item->height) {
      l->item->button(g, l->item, x, y, key_modifiers, button, up);
      break;
    }
    l = l->next;
  }
}

static void paint(gui *_gui, widget *_this)
{
  LOGD("PAINT container\n");
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
  w->common.del_child = del_child;
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

/*************************************************************************/
/*                             public functions                          */
/*************************************************************************/

void container_set_child_growable(gui *_gui, widget *_this,
    widget *child, int growable)
{
  gui *g = _gui;
  struct container_widget *this = _this;
  struct widget_list *lcur;
  int i;

  glock(g);

  lcur = this->common.children;
  i = 0;
  while (lcur) {
    if (lcur->item == child) break;
    lcur = lcur->next;
    i++;
  }
  if (lcur == NULL) ERR("%s:%d: child not found\n", __FILE__, __LINE__);

  this->growable[i] = growable;

  send_event(g, REPACK, this->common.id);

  gunlock(g);
}
