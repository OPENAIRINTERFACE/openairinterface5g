#include "view.h"
#include "../utils.h"
#include "gui/gui.h"
#include <stdlib.h>
#include <pthread.h>
#include <string.h>

struct textlist {
  view common;
  gui *g;
  widget *w;
  int maxsize;
  int cursize;
  float refresh_rate;
  int autoscroll;
  pthread_mutex_t lock;
  list * volatile to_append;
};

static void _append(struct textlist *this, char *s, int *deleted)
{
  if (this->cursize == this->maxsize) {
    textlist_del_silent(this->g, this->w, 0);
    this->cursize--;
    (*deleted)++;
  }
  textlist_add_silent(this->g, this->w, s, -1, FOREGROUND_COLOR);
  this->cursize++;
}

static void *textlist_thread(void *_this)
{
  struct textlist *this = _this;
  int dirty;
  int deleted;
  int visible_lines, start_line, number_of_lines;

  while (1) {
    if (pthread_mutex_lock(&this->lock)) abort();
    dirty = this->to_append == NULL ? 0 : 1;
    deleted = 0;
    while (this->to_append != NULL) {
      char *s = this->to_append->data;
      this->to_append = list_remove_head(this->to_append);
      _append(this, s, &deleted);
      free(s);
    }
    if (dirty) {
      textlist_state(this->g, this->w, &visible_lines, &start_line,
          &number_of_lines);
      if (this->autoscroll)
        start_line = number_of_lines - visible_lines;
      else
        start_line -= deleted;
      if (start_line < 0) start_line = 0;
      textlist_set_start_line(this->g, this->w, start_line);
      /* this call is not necessary, but if things change in textlist... */
      widget_dirty(this->g, this->w);
    }
    if (pthread_mutex_unlock(&this->lock)) abort();
    sleepms(1000/this->refresh_rate);
  }

  return 0;
}

static void clear(view *this)
{
  /* TODO */
}

static void append(view *_this, char *s)
{
  struct textlist *this = (struct textlist *)_this;
  char *dup;

  if (pthread_mutex_lock(&this->lock)) abort();
  dup = strdup(s); if (dup == NULL) abort();
  this->to_append = list_append(this->to_append, dup);
  if (pthread_mutex_unlock(&this->lock)) abort();
}

static void scroll(void *private, gui *g,
    char *notification, widget *w, void *notification_data)
{
  struct textlist *this = private;
  int visible_lines;
  int start_line;
  int number_of_lines;
  int new_line;
  int inc;
  int *d = notification_data;
  int key_modifiers = *d;

  if (pthread_mutex_lock(&this->lock)) abort();

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

  if (new_line + visible_lines < number_of_lines)
    this->autoscroll = 0;
  else
    this->autoscroll = 1;

  if (pthread_mutex_unlock(&this->lock)) abort();
}

static void click(void *private, gui *g,
    char *notification, widget *w, void *notification_data)
{
  struct textlist *this = private;
  int *d = notification_data;
  int button = d[1];

  if (pthread_mutex_lock(&this->lock)) abort();

  if (button == 1) this->autoscroll = 1;
  if (button == 3) this->autoscroll = 0;

  if (this->autoscroll) {
    int visible_lines, start_line, number_of_lines;
    textlist_state(this->g, this->w, &visible_lines, &start_line,
        &number_of_lines);
    start_line = number_of_lines - visible_lines;
    if (start_line < 0) start_line = 0;
    textlist_set_start_line(this->g, this->w, start_line);
    /* this call is not necessary, but if things change in textlist... */
    widget_dirty(this->g, this->w);
  }

  if (pthread_mutex_unlock(&this->lock)) abort();
}

view *new_view_textlist(int maxsize, float refresh_rate, gui *g, widget *w)
{
  struct textlist *ret = calloc(1, sizeof(struct textlist));
  if (ret == NULL) abort();

  ret->common.clear = clear;
  ret->common.append = (void (*)(view *, ...))append;

  ret->cursize = 0;
  ret->maxsize = maxsize;
  ret->refresh_rate = refresh_rate;
  ret->g = g;
  ret->w = w;
  ret->autoscroll = 1;

  if (pthread_mutex_init(&ret->lock, NULL)) abort();

  register_notifier(g, "scrollup", w, scroll, ret);
  register_notifier(g, "scrolldown", w, scroll, ret);
  register_notifier(g, "click", w, click, ret);

  new_thread(textlist_thread, ret);

  return (view *)ret;
}
