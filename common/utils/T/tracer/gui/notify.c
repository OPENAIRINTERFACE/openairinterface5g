#include "gui.h"
#include "gui_defs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

unsigned long register_notifier(gui *_g, char *notification, widget *w,
    notifier handler, void *private)
{
  struct gui *g = _g;
  unsigned long ret;

  glock(g);

  if (g->next_notifier_id == 2UL * 1024 * 1024 * 1024)
    ERR("%s:%d: report a bug\n", __FILE__, __LINE__);

  g->notifiers = realloc(g->notifiers,
      (g->notifiers_count+1) * sizeof(struct notifier));
  if (g->notifiers == NULL) abort();

  ret = g->next_notifier_id;

  g->notifiers[g->notifiers_count].handler = handler;
  g->notifiers[g->notifiers_count].id = g->next_notifier_id;
  g->next_notifier_id++;
  g->notifiers[g->notifiers_count].notification = strdup(notification);
  if (g->notifiers[g->notifiers_count].notification == NULL) abort();
  g->notifiers[g->notifiers_count].w = w;
  g->notifiers[g->notifiers_count].private = private;
  /* initialize done to 1 so as not to call the handler if it's created
   * by the call of another one that is in progress
   */
  g->notifiers[g->notifiers_count].done = 1;

  g->notifiers_count++;

  gunlock(g);

  return ret;
}

void unregister_notifier(gui *_g, unsigned long notifier_id)
{
  struct gui *g = _g;
  int i;

  glock(g);

  for (i = 0; i < g->notifiers_count; i++)
    if (g->notifiers[i].id == notifier_id) break;

  if (i == g->notifiers_count)
    ERR("%s:%d: notifier_id %ld not found\n", __FILE__,__LINE__,notifier_id);

  free(g->notifiers[i].notification);

  memmove(g->notifiers + i, g->notifiers + i + 1,
      (g->notifiers_count-1 - i) * sizeof(struct notifier));

  g->notifiers_count--;
  g->notifiers = realloc(g->notifiers,
      g->notifiers_count * sizeof(struct notifier));
  if (g->notifiers == NULL) abort();

  gunlock(g);
}

/* called with lock ON */
void gui_notify(struct gui *g, char *notification, widget *w,
    void *notification_data)
{
  void *private;
  notifier handler;
  int i;

  /* this function is not re-entrant, for the moment keep as is
   * and if the need is there, we'll make a new thread to handle
   * notifications (or something)
   * for now let's crash in case of recursive call
   */
  static int inside = 0;
  if (inside) ERR("%s:%d: BUG! contact the authors\n", __FILE__, __LINE__);
  inside = 1;

  /* clear all handlers */
  /* TODO: speedup */
  for (i = 0; i < g->notifiers_count; i++) g->notifiers[i].done = 0;

  /* calling the handler may modify the list of notifiers, we
   * need to be careful here
   */
loop:
  for (i = 0; i < g->notifiers_count; i++) {
    if (g->notifiers[i].done == 1 ||
        g->notifiers[i].w != w    ||
        strcmp(g->notifiers[i].notification, notification) != 0)
      continue;
    break;
  }
  if (i == g->notifiers_count) goto done;

  g->notifiers[i].done = 1;

  handler = g->notifiers[i].handler;
  private = g->notifiers[i].private;

  gunlock(g);
  handler(private, g, notification, w, notification_data);
  glock(g);

  goto loop;

done:
  inside = 0;
}
