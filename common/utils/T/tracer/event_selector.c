#include "event_selector.h"
#include "gui/gui.h"
#include "database.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

struct event_selector {
  int *is_on;
  int red;
  int green;
  gui *g;
  widget *events;
  widget *groups;
  void *database;
  int nevents;
  int ngroups;
  int socket;
};

static void scroll(void *private, gui *g,
    char *notification, widget *w, void *notification_data)
{
  int visible_lines;
  int start_line;
  int number_of_lines;
  int new_line;
  int inc;

  textlist_state(g, w, &visible_lines, &start_line, &number_of_lines);
  inc = 10;
  if (inc > visible_lines - 2) inc = visible_lines - 2;
  if (inc < 1) inc = 1;
  if (!strcmp(notification, "scrollup")) inc = -inc;

  new_line = start_line + inc;
  if (new_line > number_of_lines - visible_lines)
    new_line = number_of_lines - visible_lines;
  if (new_line < 0) new_line = 0;

  textlist_set_start_line(g, w, new_line);
}

static void click(void *private, gui *g,
    char *notification, widget *w, void *notification_data)
{
  int *d = notification_data;
  struct event_selector *this = private;
  int set_on;
  int line = d[0];
  int button = d[1];
  char *text;
  int color;
  int i;
  char t;

  if (button != 1 && button != 3) return;

  if (button == 1) set_on = 1; else set_on = 0;

  if (w == this->events)
    textlist_get_line(this->g, this->events, line, &text, &color);
  else
    textlist_get_line(this->g, this->groups, line, &text, &color);

  on_off(this->database, text, this->is_on, set_on);

  for (i = 0; i < this->nevents; i++)
    textlist_set_color(this->g, this->events, i,
        this->is_on[database_pos_to_id(this->database, i)] ?
            this->green : this->red);

  for (i = 0; i < this->ngroups; i++)
    textlist_set_color(this->g, this->groups, i, FOREGROUND_COLOR);
  if (w == this->groups)
    textlist_set_color(this->g, this->groups, line,
        set_on ? this->green : this->red);

  t = 1;
  socket_send(this->socket, &t, 1);
  socket_send(this->socket, &this->nevents, sizeof(int));
  socket_send(this->socket, this->is_on, this->nevents * sizeof(int));
}

event_selector *setup_event_selector(gui *g, void *database, int socket,
    int *is_on)
{
  struct event_selector *ret;
  widget *win;
  widget *main_container;
  widget *container;
  widget *left, *right;
  widget *events, *groups;
  char **ids;
  char **gps;
  int n;
  int i;
  int red, green;

  ret = calloc(1, sizeof(struct event_selector)); if (ret == NULL) abort();

  red = new_color(g, "#c93535");
  green = new_color(g, "#2f9e2a");

  win = new_toplevel_window(g, 470, 300, "event selector");
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

  events = new_textlist(g, 235, 10, new_color(g, "#b3c1e1"));
  groups = new_textlist(g, 235, 10, new_color(g, "#edd6cb"));

  widget_add_child(g, left, events, -1);
  widget_add_child(g, right, groups, -1);
  container_set_child_growable(g, left, events, 1);
  container_set_child_growable(g, right, groups, 1);

  n = database_get_ids(database, &ids);
  for (i = 0; i < n; i++) {
    textlist_add(g, events, ids[i], -1,
        is_on[database_pos_to_id(database, i)] ? green : red);
  }
  free(ids);

  ret->nevents = n;

  n = database_get_groups(database, &gps);
  for (i = 0; i < n; i++) {
    textlist_add(g, groups, gps[i], -1, FOREGROUND_COLOR);
  }
  free(gps);

  ret->ngroups = n;

  ret->g = g;
  ret->is_on = is_on;
  ret->red = red;
  ret->green = green;
  ret->events = events;
  ret->groups = groups;
  ret->database = database;
  ret->socket = socket;

  register_notifier(g, "scrollup", events, scroll, ret);
  register_notifier(g, "scrolldown", events, scroll, ret);
  register_notifier(g, "click", events, click, ret);

  register_notifier(g, "scrollup", groups, scroll, ret);
  register_notifier(g, "scrolldown", groups, scroll, ret);
  register_notifier(g, "click", groups, click, ret);

  return ret;
}
