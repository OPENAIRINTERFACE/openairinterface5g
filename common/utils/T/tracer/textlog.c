#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include "database.h"
#include "event.h"
#include "handler.h"
#include "logger/logger.h"
#include "view/view.h"
#include "gui/gui.h"
#include "utils.h"
#include "../T_defs.h"
#include "event_selector.h"

#define DEFAULT_REMOTE_PORT 2021

int get_connection(char *addr, int port)
{
  struct sockaddr_in a;
  socklen_t alen;
  int s, t;

  printf("waiting for connection on %s:%d\n", addr, port);

  s = socket(AF_INET, SOCK_STREAM, 0);
  if (s == -1) { perror("socket"); exit(1); }
  t = 1;
  if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &t, sizeof(int)))
    { perror("setsockopt"); exit(1); }

  a.sin_family = AF_INET;
  a.sin_port = htons(port);
  a.sin_addr.s_addr = inet_addr(addr);

  if (bind(s, (struct sockaddr *)&a, sizeof(a))) { perror("bind"); exit(1); }
  if (listen(s, 5)) { perror("bind"); exit(1); }
  alen = sizeof(a);
  t = accept(s, (struct sockaddr *)&a, &alen);
  if (t == -1) { perror("accept"); exit(1); }
  close(s);

  printf("connected\n");

  return t;
}

void usage(void)
{
  printf(
"options:\n"
"    -d <database file>        this option is mandatory\n"
"    -on <GROUP or ID>         turn log ON for given GROUP or ID\n"
"    -off <GROUP or ID>        turn log OFF for given GROUP or ID\n"
"    -ON                       turn all logs ON\n"
"    -OFF                      turn all logs OFF\n"
"                              note: you may pass several -on/-off/-ON/-OFF,\n"
"                                    they will be processed in order\n"
"                                    by default, all is off\n"
"    -p <port>                 use given port (default %d)\n"
"    -x                        GUI output\n"
"    -debug-gui                activate GUI debug logs\n"
"    -no-gui                   disable GUI entirely\n",
  DEFAULT_REMOTE_PORT
  );
  exit(1);
}

int fullread(int fd, void *_buf, int count)
{
  char *buf = _buf;
  int ret = 0;
  int l;
  while (count) {
    l = read(fd, buf, count);
    if (l <= 0) { printf("read socket problem\n"); abort(); }
    count -= l;
    buf += l;
    ret += l;
  }
  return ret;
}

event get_event(int s, char *v, void *d)
{
#ifdef T_SEND_TIME
  struct timespec t;
#endif
  int type;
  int32_t length;

  fullread(s, &length, 4);
#ifdef T_SEND_TIME
  fullread(s, &t, sizeof(struct timespec));
  length -= sizeof(struct timespec);
#endif
  fullread(s, &type, sizeof(int));
  length -= sizeof(int);
  fullread(s, v, length);

  return new_event(type, length, v, d);
}

static void *gui_thread(void *_g)
{
  gui *g = _g;
  gui_loop(g);
  return NULL;
}

int main(int n, char **v)
{
  extern int volatile gui_logd;
  char *database_filename = NULL;
  void *database;
  int port = DEFAULT_REMOTE_PORT;
  char **on_off_name;
  int *on_off_action;
  int on_off_n = 0;
  int *is_on;
  int number_of_events;
  int s;
  int i;
  char t;
  int l;
  event_handler *h;
  logger *textlog;
  gui *g;
  int gui_mode = 0;
  view *out;
  int gui_active = 1;

  on_off_name = malloc(n * sizeof(char *)); if (on_off_name == NULL) abort();
  on_off_action = malloc(n * sizeof(int)); if (on_off_action == NULL) abort();

  for (i = 1; i < n; i++) {
    if (!strcmp(v[i], "-h") || !strcmp(v[i], "--help")) usage();
    if (!strcmp(v[i], "-d"))
      { if (i > n-2) usage(); database_filename = v[++i]; continue; }
    if (!strcmp(v[i], "-p"))
      { if (i > n-2) usage(); port = atoi(v[++i]); continue; }
    if (!strcmp(v[i], "-on")) { if (i > n-2) usage();
      on_off_name[on_off_n]=v[++i]; on_off_action[on_off_n++]=1; continue; }
    if (!strcmp(v[i], "-off")) { if (i > n-2) usage();
      on_off_name[on_off_n]=v[++i]; on_off_action[on_off_n++]=0; continue; }
    if (!strcmp(v[i], "-ON"))
      { on_off_name[on_off_n]=NULL; on_off_action[on_off_n++]=1; continue; }
    if (!strcmp(v[i], "-OFF"))
      { on_off_name[on_off_n]=NULL; on_off_action[on_off_n++]=0; continue; }
    if (!strcmp(v[i], "-x")) { gui_mode = 1; continue; }
    if (!strcmp(v[i], "-debug-gui")) { gui_logd = 1; continue; }
    if (!strcmp(v[i], "-no-gui")) { gui_active = 0; continue; }
    usage();
  }

  if (gui_active == 0) gui_mode = 0;

  if (database_filename == NULL) {
    printf("ERROR: provide a database file (-d)\n");
    exit(1);
  }

  database = parse_database(database_filename);

  number_of_events = number_of_ids(database);
  is_on = calloc(number_of_events, sizeof(int));
  if (is_on == NULL) abort();

  h = new_handler(database);

  if (gui_active) {
    g = gui_init();
    new_thread(gui_thread, g);
  }

  if (gui_mode) {
    widget *w, *win;
//    w = new_textlist(g, 600, 20, 0);
    w = new_textlist(g, 800, 50, BACKGROUND_COLOR);
    win = new_toplevel_window(g, 800, 50*12, "textlog");
    widget_add_child(g, win, w, -1);
    out = new_view_textlist(1000, 10, g, w);
    //tout = new_view_textlist(7, 4, g, w);
  } else {
    out = new_view_stdout();
  }

  for (i = 0; i < number_of_events; i++) {
    char *name, *desc;
    database_get_generic_description(database, i, &name, &desc);
    textlog = new_textlog(h, database, name, desc);
//        "ENB_UL_CHANNEL_ESTIMATE",
//        "ev: {} eNB_id [eNB_ID] frame [frame] subframe [subframe]");
    logger_add_view(textlog, out);
    free(name);
    free(desc);
  }

  for (i = 0; i < on_off_n; i++)
    on_off(database, on_off_name[i], is_on, on_off_action[i]);

  s = get_connection("0.0.0.0", port);

  /* send the first message - activate selected traces */
  t = 0;
  if (write(s, &t, 1) != 1) abort();
  l = 0;
  for (i = 0; i < number_of_events; i++) if (is_on[i]) l++;
  if (write(s, &l, sizeof(int)) != sizeof(int)) abort();
  for (l = 0; l < number_of_events; l++)
    if (is_on[l])
      if (write(s, &l, sizeof(int)) != sizeof(int)) abort();

  if (gui_active)
    setup_event_selector(g, database, s, is_on);

  /* read messages */
  while (1) {
    char v[T_BUFFER_MAX];
    event e;
    e = get_event(s, v, database);
    handle_event(h, e);
  }

  return 0;
}
