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
} vcd_data;

void is_on_changed(void *_d)
{
  vcd_data *d = _d;
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
"    -ip <host>                connect to given IP address (default %s)\n"
"    -p <port>                 connect to given port (default %d)\n"
"    -debug-gui                activate GUI debug logs\n",
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

static void vcd_main_gui(gui *g, event_handler *h, void *database)
{
  int i, j;
  int n;
  int nb_functions = 0;
  char **ids;
  widget *win;
  widget *container;
  widget *w;
  view *timeview;
  view *subview;
  logger *timelog;

  /* get number of vcd functions - look for all events VCD_FUNCTION_xxx */
  n = database_get_ids(database, &ids);
  for (i = 0; i < n; i++) {
    if (strncmp(ids[i], "VCD_FUNCTION_", 13) != 0) continue;
    nb_functions++;
  }

  win = new_toplevel_window(g, 1000, 5 * nb_functions, "VCD tracer");
  container = new_container(g, VERTICAL);
  widget_add_child(g, win, container, -1);

  w = new_timeline(g, 1000, nb_functions, 5);
  widget_add_child(g, container, w, -1);
  for (i = 0; i < nb_functions; i++)
    timeline_set_subline_background_color(g, w, i,
        new_color(g, i & 1 ? "#ddd" : "#eee"));
  timeview = new_view_time(3600, 10, g, w);
  i = 0;
  for (j = 0; j < n; j++) {
    if (strncmp(ids[j], "VCD_FUNCTION_", 13) != 0) continue;
    timelog = new_timelog(h, database, ids[j]);
    subview = new_subview_time(timeview, i, FOREGROUND_COLOR, 3600*1000);
    logger_add_view(timelog, subview);
    i++;
  }

  free(ids);
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
  gui *g;
  vcd_data vcd_data;

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
    if (!strcmp(v[i], "-debug-gui")) { gui_logd = 1; continue; }
    usage();
  }

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

  g = gui_init();
  new_thread(gui_thread, g);

  vcd_main_gui(g, h, database);

  on_off(database, "VCD_FUNCTION", is_on, 1);

  for (i = 0; i < on_off_n; i++)
    on_off(database, on_off_name[i], is_on, on_off_action[i]);

  vcd_data.socket = -1;
  vcd_data.is_on = is_on;
  vcd_data.nevents = number_of_events;
  if (pthread_mutex_init(&vcd_data.lock, NULL)) abort();
  setup_event_selector(g, database, is_on, is_on_changed, &vcd_data);

  vcd_data.socket = connect_to(ip, port);

  /* send the first message - activate selected traces */
  is_on_changed(&vcd_data);

  /* read messages */
  while (1) {
    char v[T_BUFFER_MAX];
    event e;
    e = get_event(vcd_data.socket, v, database);
    if (e.type == -1) abort();
    handle_event(h, e);
  }

  return 0;
}
