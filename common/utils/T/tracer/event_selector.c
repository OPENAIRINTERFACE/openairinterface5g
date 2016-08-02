#include "event_selector.h"
#include "gui/gui.h"
#include "database.h"
#include "utils.h"
#include <stdlib.h>
#include <string.h>

struct event_selector {
  int *is_on;
  int *is_on_paused;    /* when pausing, is_on is set to all 0, this one
                         * is used to copy back data when un-pausing */
  int red;
  int green;
  gui *g;
  widget *events;
  widget *groups;
  void *database;
  int nevents;
  int ngroups;
  int paused;
  /* those three widgets used to pause/unpause reception of events */
  widget *parent_widget;
  widget *normal_widget;
  widget *pause_widget;
  void (*change_callback)(void *change_callback_data);
  void *change_callback_data;
};

static void scroll(void *private, gui *g,
    char *notification, widget *w, void *notification_data)
{
  int visible_lines;
  int start_line;
  int number_of_lines;
  int new_line;
  int inc;
  int *d = notification_data;
  int key_modifiers = *d;

  textlist_state(g, w, &visible_lines, &start_line, &number_of_lines);
  inc = 10;
  if (inc > visible_lines - 2) inc = visible_lines - 2;
  if (inc < 1) inc = 1;
  if (key_modifiers & KEY_CONTROL) inc = 1;
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

  /* notification_data depends on the kind of widget */
  if (w == this->pause_widget) {
    line = 0;
    button = d[0];
  } else {
    line = d[0];
    button = d[1];
  }

  /* middle-button toggles - redo with SPACE when keyboard is processed */
  if (button == 2) {
    if (this->paused == 0) {
      widget_del_child(g, this->parent_widget, this->normal_widget);
      widget_add_child(g, this->parent_widget, this->pause_widget, 0);
      container_set_child_growable(g, this->parent_widget,
          this->pause_widget, 1);
      /* pause */
      memcpy(this->is_on_paused, this->is_on, this->nevents * sizeof(int));
      memset(this->is_on, 0, this->nevents * sizeof(int));
      this->change_callback(this->change_callback_data);
    } else {
      widget_del_child(g, this->parent_widget, this->pause_widget);
      widget_add_child(g, this->parent_widget, this->normal_widget, 0);
      container_set_child_growable(g, this->parent_widget,
          this->normal_widget, 1);
      /* un-pause */
      memcpy(this->is_on, this->is_on_paused, this->nevents * sizeof(int));
      this->change_callback(this->change_callback_data);
    }
    this->paused = 1 - this->paused;
    return;
  }

  if (w == this->pause_widget) return;

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

  this->change_callback(this->change_callback_data);
}

event_selector *setup_event_selector(gui *g, void *database, int *is_on,
    void (*change_callback)(void *), void *change_callback_data)
{
  struct event_selector *ret;
  widget *win;
  widget *win_container;
  widget *main_container;
  widget *container;
  widget *left, *right;
  widget *events, *groups;
  widget *pause_container;
  char **ids;
  char **gps;
  int n;
  int i;
  int red, green;

  ret = calloc(1, sizeof(struct event_selector)); if (ret == NULL) abort();

  red = new_color(g, "#c93535");
  green = new_color(g, "#2f9e2a");

  win = new_toplevel_window(g, 470, 300, "event selector");
  win_container = new_container(g, VERTICAL);
  widget_add_child(g, win, win_container, -1);

  main_container = new_container(g, VERTICAL);
  widget_add_child(g, win_container, main_container, -1);
  container_set_child_growable(g, win_container, main_container, 1);

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

  pause_container = new_positioner(g);
  widget_add_child(g, pause_container,
      new_label(g,
          "events' reception paused - click middle button to resume"), -1);
  label_set_clickable(g, pause_container, 1);

  n = database_get_ids(database, &ids);
  for (i = 0; i < n; i++) {
    textlist_add(g, events, ids[i], -1,
        is_on[database_pos_to_id(database, i)] ? green : red);
  }
  free(ids);

  ret->nevents = n;

  ret->is_on_paused = calloc(n, sizeof(int));
  if (ret->is_on_paused == NULL) abort();

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
  ret->change_callback = change_callback;
  ret->change_callback_data = change_callback_data;

  ret->parent_widget = win_container;
  ret->normal_widget = main_container;
  ret->pause_widget = pause_container;

  register_notifier(g, "scrollup", events, scroll, ret);
  register_notifier(g, "scrolldown", events, scroll, ret);
  register_notifier(g, "click", events, click, ret);

  register_notifier(g, "scrollup", groups, scroll, ret);
  register_notifier(g, "scrolldown", groups, scroll, ret);
  register_notifier(g, "click", groups, click, ret);

  register_notifier(g, "click", pause_container, click, ret);

  return ret;
}
