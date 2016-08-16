#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <signal.h>
#include "database.h"
#include "event.h"
#include "handler.h"
#include "logger/logger.h"
#include "view/view.h"
#include "gui/gui.h"
#include "utils.h"
#include "../T_defs.h"
#include "event_selector.h"
#include "config.h"

typedef struct {
  int socket;
  int *is_on;
  int nevents;
  pthread_mutex_t lock;
} textlog_data;

void is_on_changed(void *_d)
{
  textlog_data *d = _d;
  char t;

  if (pthread_mutex_lock(&d->lock)) abort();

  if (d->socket == -1) goto no_connection;

  t = 1;
  if (socket_send(d->socket, &t, 1) == -1 ||
      socket_send(d->socket, &d->nevents, sizeof(int)) == -1 ||
      socket_send(d->socket, d->is_on, d->nevents * sizeof(int)) == -1)
    abort();

no_connection:
  if (pthread_mutex_unlock(&d->lock)) abort();
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
"    -full                     also dump buffers' content\n"
"    -ip <host>                connect to given IP address (default %s)\n"
"    -p <port>                 connect to given port (default %d)\n"
"    -x                        GUI output\n"
"    -debug-gui                activate GUI debug logs\n"
"    -no-gui                   disable GUI entirely\n",
  DEFAULT_REMOTE_IP,
  DEFAULT_REMOTE_PORT
  );
  exit(1);
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
  char *ip = DEFAULT_REMOTE_IP;
  int port = DEFAULT_REMOTE_PORT;
  char **on_off_name;
  int *on_off_action;
  int on_off_n = 0;
  int *is_on;
  int number_of_events;
  int i;
  event_handler *h;
  logger *textlog;
  gui *g;
  int gui_mode = 0;
  view *out;
  int gui_active = 1;
  textlog_data textlog_data;
  int full = 0;

  /* write on a socket fails if the other end is closed and we get SIGPIPE */
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) abort();

  on_off_name = malloc(n * sizeof(char *)); if (on_off_name == NULL) abort();
  on_off_action = malloc(n * sizeof(int)); if (on_off_action == NULL) abort();

  for (i = 1; i < n; i++) {
    if (!strcmp(v[i], "-h") || !strcmp(v[i], "--help")) usage();
    if (!strcmp(v[i], "-d"))
      { if (i > n-2) usage(); database_filename = v[++i]; continue; }
    if (!strcmp(v[i], "-ip")) { if (i > n-2) usage(); ip = v[++i]; continue; }
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
    if (!strcmp(v[i], "-full")) { full = 1; continue; }
    usage();
  }

  if (gui_active == 0) gui_mode = 0;

  if (database_filename == NULL) {
    printf("ERROR: provide a database file (-d)\n");
    exit(1);
  }

  database = parse_database(database_filename);

  load_config_file(database_filename);

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
//        "ENB_PHY_UL_CHANNEL_ESTIMATE",
//        "ev: {} eNB_id [eNB_ID] frame [frame] subframe [subframe]");
    logger_add_view(textlog, out);
    if (full) textlog_dump_buffer(textlog, 1);
    free(name);
    free(desc);
  }

  for (i = 0; i < on_off_n; i++)
    on_off(database, on_off_name[i], is_on, on_off_action[i]);

  textlog_data.socket = -1;
  textlog_data.is_on = is_on;
  textlog_data.nevents = number_of_events;
  if (pthread_mutex_init(&textlog_data.lock, NULL)) abort();
  if (gui_active)
    setup_event_selector(g, database, is_on, is_on_changed, &textlog_data);

  textlog_data.socket = connect_to(ip, port);

  /* send the first message - activate selected traces */
  is_on_changed(&textlog_data);

  /* read messages */
  while (1) {
    char v[T_BUFFER_MAX];
    event e;
    e = get_event(textlog_data.socket, v, database);
    if (e.type == -1) abort();
    handle_event(h, e);
  }

  return 0;
}
