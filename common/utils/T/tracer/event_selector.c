#include "event_selector.h"
#include "gui/gui.h"

void setup_event_selector(gui *g, void *database, int socket, int *is_on)
{
  widget *win;
  widget *main_container;
  widget *container;
  widget *left, *right;
  widget *events, *groups;

  win = new_toplevel_window(g, 610, 800, "event selector");
  main_container = new_container(g, VERTICAL);
  widget_add_child(g, win, main_container, -1);

  container = new_container(g, HORIZONTAL);
  widget_add_child(g, main_container, container, -1);
  container_set_child_growable(g, main_container, container, 1);
  widget_add_child(g, main_container,
      new_label(g, "mouse scroll to scroll - "
                   "left click to activate - "
                   "right click to deactivate"), -1);

  left = new_container(g, VERTICAL);
  right = new_container(g, VERTICAL);
  widget_add_child(g, container, left, -1);
  widget_add_child(g, container, right, -1);
  container_set_child_growable(g, container, left, 1);
  container_set_child_growable(g, container, right, 1);

  widget_add_child(g, left, new_label(g, "Events"), -1);
  widget_add_child(g, right, new_label(g, "Groups"), -1);

  events = new_text_list(g, 300, 10, new_color(g, "#ccccff"));
  groups = new_text_list(g, 300, 10, new_color(g, "#ccffee"));

  widget_add_child(g, left, events, -1);
  widget_add_child(g, right, groups, -1);
  container_set_child_growable(g, left, events, 1);
  container_set_child_growable(g, right, groups, 1);
}
