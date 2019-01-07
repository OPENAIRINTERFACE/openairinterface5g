#include "gui.h"
#include "gui_defs.h"
#include "x.h"
#include <stdio.h>
#include <stdlib.h>

/**********************************************************************/
/*                         callback functions                         */
/**********************************************************************/

static void repack(gui *g, widget *_this)
{
  LOGD("REPACK toplevel_window\n");
  struct toplevel_window_widget *this = _this;
  if (this->common.children == NULL) ERR("toplevel window has no child\n");
  if (this->common.children->next != NULL)
    ERR("toplevel window has too much children\n");
  this->common.children->item->allocate(g, this->common.children->item,
      0 /* x */, 0 /* y */, this->common.width, this->common.height);
  send_event(g, DIRTY, this->common.id);
}

static void add_child(gui *_gui, widget *_this, widget *child, int position)
{
  LOGD("ADD_CHILD toplevel_window\n");
  struct widget *this = _this;
  if (this->children != NULL) {
    WARN("toplevel window already has a child\n");
    return;
  }
  widget_add_child_internal(_gui, _this, child, 0); /* this does the REPACK */
}

/* called when the underlying window is resized by the user or the system */
static void allocate(
    gui *_gui, widget *_this, int x, int y, int width, int height)
{
  LOGD("ALLOCATE toplevel_window\n");
  struct toplevel_window_widget *this = _this;
  this->common.width = width;
  this->common.height = height;
//  repack(_gui, _this);
  send_event(_gui, REPACK, this->common.id);
}

static void paint(gui *_gui, widget *_this)
{
  struct gui *g = _gui;
  struct toplevel_window_widget *this = _this;
  LOGD("PAINT toplevel_window (%d %d)\n", this->common.width, this->common.height);
  x_fill_rectangle(g->x, this->x, BACKGROUND_COLOR,
      0, 0, this->common.width, this->common.height);
  g->xwin = this->x;
  this->common.children->item->paint(_gui, this->common.children->item);
  g->xwin = NULL;    /* TODO: remove? it's just in case */
}

static void button(gui *_g, widget *_this, int x, int y,
    int key_modifiers, int button, int up)
{
  struct gui *g = _g;
  struct toplevel_window_widget *this = _this;
  g->xwin = this->x;
  this->common.children->item->button(_g, this->common.children->item,
      x, y, key_modifiers, button, up);
  g->xwin = NULL;    /* TODO: remove? it's just in case */
}

/**********************************************************************/
/*                              creation                              */
/**********************************************************************/

widget *new_toplevel_window(gui *_gui, int width, int height, char *title)
{
  struct gui *g = _gui;
  struct toplevel_window_widget *w;

  glock(g);

  w = new_widget(g, TOPLEVEL_WINDOW, sizeof(struct toplevel_window_widget));

  w->common.width  = width;
  w->common.height = height;

  w->x = x_create_window(g->x, width, height, title);

  w->common.repack    = repack;
  w->common.add_child = add_child;
  w->common.allocate  = allocate;
  w->common.paint     = paint;

  w->common.button    = button;

  gunlock(g);

  return w;
}
