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
"    -debug-gui                activate GUI debug logs\n",
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

#ifdef T_SEND_TIME
  return new_event(t, type, length, v, d);
#else
  return new_event(type, length, v, d);
#endif
}

static void *gui_thread(void *_g)
{
  gui *g = _g;
  gui_loop(g);
  return NULL;
}

static void enb_main_gui(gui *g, event_handler *h, void *database)
{
  widget *main_window;
  widget *top_container;
  widget *line;
  widget *input_signal_plot;
  logger *input_signal_log;
  view *input_signal_view;
  widget *timeline_plot;
  logger *timelog;
  view *timeview;
  view *subview;
  int i;

  main_window = new_toplevel_window(g, 800, 600, "eNB tracer");
  top_container = new_container(g, VERTICAL);
  widget_add_child(g, main_window, top_container, -1);

  line = new_container(g, HORIZONTAL);
  widget_add_child(g, top_container, line, -1);
  input_signal_plot = new_xy_plot(g, 256, 55, "input signal", 20);
  widget_add_child(g, line, input_signal_plot, -1);
  xy_plot_set_range(g, input_signal_plot, 0, 7680*10, 20, 70);
  input_signal_log = new_framelog(h, database,
      "ENB_INPUT_SIGNAL", "subframe", "rxdata");
  /* a skip value of 10 means to process 1 frame over 10, that is
   * more or less 10 frames per second
   */
  framelog_set_skip(input_signal_log, 10);
  input_signal_view = new_view_xy(7680*10, 10,
      g, input_signal_plot, new_color(g, "#0c0c72"));
  logger_add_view(input_signal_log, input_signal_view);

  /* downlink UE DCIs */
  line = new_container(g, HORIZONTAL);
  widget_add_child(g, top_container, line, -1);
  timeline_plot = new_timeline(g, 512, 8, 5);
  widget_add_child(g, line, timeline_plot, -1);
  container_set_child_growable(g, line, timeline_plot, 1);
  widget_add_child(g, line, new_label(g,"DL/UL TICK/DCI/ACK/NACK "), 0);
  for (i = 0; i < 8; i++)
    timeline_set_subline_background_color(g, timeline_plot, i,
        new_color(g, i & 1 ? "#ddd" : "#eee"));
  timeview = new_view_time(3600, 10, g, timeline_plot);
  /* tick logging */
  timelog = new_timelog(h, database, "ENB_DL_TICK");
  subview = new_subview_time(timeview, 0, FOREGROUND_COLOR);
  logger_add_view(timelog, subview);
  /* DCI logging */
  timelog = new_timelog(h, database, "ENB_DLSCH_UE_DCI");
  subview = new_subview_time(timeview, 1, new_color(g, "#228"));
  logger_add_view(timelog, subview);

  /* uplink UE DCIs */
  timelog = new_timelog(h, database, "ENB_UL_TICK");
  subview = new_subview_time(timeview, 4, FOREGROUND_COLOR);
  logger_add_view(timelog, subview);
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
  gui *g;

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
    if (!strcmp(v[i], "-debug-gui")) { gui_logd = 1; continue; }
    usage();
  }

  if (database_filename == NULL) {
    printf("ERROR: provide a database file (-d)\n");
    exit(1);
  }

  database = parse_database(database_filename);

  number_of_events = number_of_ids(database);
  is_on = calloc(number_of_events, sizeof(int));
  if (is_on == NULL) abort();

  h = new_handler(database);

  g = gui_init();
  new_thread(gui_thread, g);

  enb_main_gui(g, h, database);

  on_off(database, "ENB_INPUT_SIGNAL", is_on, 1);
  on_off(database, "ENB_UL_TICK", is_on, 1);
  on_off(database, "ENB_DL_TICK", is_on, 1);
  on_off(database, "ENB_DLSCH_UE_DCI", is_on, 1);

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
