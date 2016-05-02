#include "gui.h"
#include "gui_defs.h"
#include "x.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

static void default_clear(gui *gui, widget *_this);
static void default_repack(gui *gui, widget *_this);
static void default_allocate(
    gui *gui, widget *_this, int x, int y, int width, int height);
static void default_add_child(
    gui *_gui, widget *_this, widget *child, int position);
static void default_hints(gui *g, widget *this, int *width, int *height);
static void default_button(gui *gui, widget *_this, int x, int y, int button,
    int up);

static void toplevel_list_append(struct gui *g, struct widget *e)
{
  struct widget_list *new;

  new = calloc(1, sizeof(struct widget_list));
  if (new == NULL) OOM;

  new->item = e;

  if (g->toplevel == NULL) {
    g->toplevel = new;
    new->last = new;
    return;
  }

  g->toplevel->last->next = new;
  g->toplevel->last = new;
}

widget *new_widget(struct gui *g, enum widget_type type, int size)
{
  struct widget *ret;

  //glock(g);

  ret = calloc(1, size);
  if (ret == NULL) OOM;

  ret->clear     = default_clear;
  ret->repack    = default_repack;
  ret->add_child = default_add_child;
  ret->allocate  = default_allocate;
  ret->hints     = default_hints;
  ret->button    = default_button;
  /* there is no default paint, on purpose */

  ret->type      = type;
  ret->id        = g->next_id;
  g->next_id++;
  ret->width     = 0;
  ret->height    = 0;

  /* add toplevel windows to g->toplevel */
  if (type == TOPLEVEL_WINDOW)
    toplevel_list_append(g, ret);

  //gunlock(g);

  return ret;
}

/*************************************************************************/
/*                          internal functions                           */
/*************************************************************************/

void widget_add_child_internal(
    gui *_gui, widget *parent, widget *child, int position)
{
  struct widget *p = parent;
  struct widget *c = child;
  struct widget_list *new;
  struct widget_list *prev, *cur;
  int i;

  new = calloc(1, sizeof(struct widget_list));
  if (new == NULL) OOM;

  new->item = c;
  c->parent = p;

  prev = NULL;
  cur = p->children;

  for (i = 0; position < 0 || i < position; i++) {
    if (cur == NULL) break;
    prev = cur;
    cur = cur->next;
  }

  /* TODO: warn/err if i != position+1? */

  if (prev == NULL) {
    /* new is at head */
    new->next = p->children;
    if (p->children != NULL) new->last = p->children->last;
    else                     new->last = new;
    p->children = new;
    goto repack;
  }

  if (cur == NULL) {
    /* new is at tail */
    prev->next = new;
    p->children->last = new;
    goto repack;
  }

  /* new is between two existing items */
  prev->next = new;
  new->next = cur;

repack:
  send_event(_gui, REPACK, p->id);
}

/*************************************************************************/
/*                           default functions                           */
/*************************************************************************/

static void default_clear(gui *_gui, widget *_this)
{
  struct gui *g = _gui;
  struct widget *this = _this;
  x_fill_rectangle(g->x, g->xwin, BACKGROUND_COLOR,
      this->x, this->y, this->width, this->height);
}

static void default_repack(gui *gui, widget *_this)
{
  struct widget *this = _this;
  return this->parent->repack(gui, this->parent);
}

static void default_add_child(
    gui *_gui, widget *_this, widget *child, int position) 
{
  struct widget *this = _this;
  WARN("cannot add child to widget %s\n", widget_name(this->type));
}

static void default_allocate(
    gui *gui, widget *_this, int x, int y, int width, int height)
{
  struct widget *this = _this;
  this->x = x;
  this->y = y;
  this->width = width;
  this->height = height;
}

static void default_hints(gui *g, widget *this, int *width, int *height)
{
  *width = 1;
  *height = 1;
}

static void default_button(gui *gui, widget *_this, int x, int y, int button,
    int up)
{
  /* nothing */
}

/*************************************************************************/
/*                             utils functions                           */
/*************************************************************************/

void widget_add_child(gui *_gui, widget *parent, widget *child, int position)
{
  struct widget *this = parent;
  glock(_gui);
  this->add_child(_gui, parent, child, position);
  gunlock(_gui);
}

void widget_dirty(gui *_gui, widget *_this)
{
  struct gui *g = _gui;
  struct widget *this = _this;
  glock(g);
  send_event(g, DIRTY, this->id);
  gunlock(g);
}

static const char *names[] = {
  "TOPLEVEL_WINDOW", "CONTAINER", "TEXT_LIST", "XY_PLOT", "BUTTON", "LABEL"
};
const char *widget_name(enum widget_type type)
{
  switch (type) {
  default: break;
  case TOPLEVEL_WINDOW:
  case CONTAINER:
  case TEXT_LIST:
  case XY_PLOT:
  case BUTTON:
  case LABEL:
    return names[type];
  }
  return "UNKNOWN (error)";
}

/*************************************************************************/
/*                             find a widget                             */
/*************************************************************************/

/* TODO: optimize traversal and also use a cache */
struct widget *_find_widget(struct widget *c, int id)
{
  struct widget_list *l;
  struct widget *ret;
  if (c == NULL) return NULL;
  if (c->id == id) return c;
  l = c->children;
  while (l) {
    ret = _find_widget(l->item, id);
    if (ret != NULL) return ret;
    l = l->next;
  }
  return NULL;
}

struct widget *find_widget(struct gui *g, int id)
{
  struct widget_list *l;
  struct widget *ret;

  l = g->toplevel;

  while (l) {
    ret = _find_widget(l->item, id);
    if (ret != NULL) return ret;
    l = l->next;
  }

  return NULL;
}
