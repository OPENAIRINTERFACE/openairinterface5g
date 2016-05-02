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

static void _append(struct textlist *this, char *s)
{
  if (this->cursize == this->maxsize) {
    text_list_del_silent(this->g, this->w, 0);
    this->cursize--;
  }
  text_list_add_silent(this->g, this->w, s, -1, FOREGROUND_COLOR);
  this->cursize++;
}

static void *textlist_thread(void *_this)
{
  struct textlist *this = _this;
  int dirty;

  while (1) {
    if (pthread_mutex_lock(&this->lock)) abort();
    dirty = this->to_append == NULL ? 0 : 1;
    while (this->to_append != NULL) {
      char *s = this->to_append->data;
      this->to_append = list_remove_head(this->to_append);
      _append(this, s);
      free(s);
    }
    if (pthread_mutex_unlock(&this->lock)) abort();
    if (dirty) text_list_dirty(this->g, this->w);
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

view *new_textlist(int maxsize, float refresh_rate, gui *g, widget *w)
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

  new_thread(textlist_thread, ret);

  return (view *)ret;
}
