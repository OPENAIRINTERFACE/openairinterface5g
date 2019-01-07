#include "gui.h"
#include "gui_defs.h"
#include "x.h"
#include <sys/select.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

void gui_loop(gui *_gui)
{
  struct gui *g = _gui;
  int xfd;
  int eventfd;
  int maxfd;
  fd_set rd;

  /* lock not necessary but there for consistency (when instrumenting x.c
   * we need the gui to be locked when calling any function in x.c)
   */
  glock(g);
  xfd = x_connection_fd(g->x);
  gunlock(g);
  eventfd = g->event_pipe[0];

  if (eventfd > xfd) maxfd = eventfd;
  else               maxfd = xfd;

  while (1) {
    glock(g);
    x_flush(g->x);
    gunlock(g);
    FD_ZERO(&rd);
    FD_SET(xfd, &rd);
    FD_SET(eventfd, &rd);
    if (select(maxfd+1, &rd, NULL, NULL, NULL) == -1)
      ERR("select: %s\n", strerror(errno));

    glock(g);

    if (FD_ISSET(xfd, &rd))
      x_events(g);

    if (FD_ISSET(eventfd, &rd)) {
      char c[256];
      if (read(eventfd, c, 256)); /* for no gcc warnings */
    }

    gui_events(g);

    if (g->repainted) {
      struct widget_list *cur;
      g->repainted = 0;
      cur = g->toplevel;
      while (cur) {
        struct toplevel_window_widget *w =
            (struct toplevel_window_widget *)cur->item;
        x_draw(g->x, w->x);
        cur = cur->next;
      }
    }

    gunlock(g);
  }
}
