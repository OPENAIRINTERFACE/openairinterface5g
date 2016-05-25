#include <stdio.h>
#include <stdlib.h>
#include <string.h>
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
#include "openair_logo.h"
#include "config.h"

typedef struct {
  view *rrcview;
} enb_gui;

#define DEFAULT_REMOTE_PORT 2021

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

static void *gui_thread(void *_g)
{
  gui *g = _g;
  gui_loop(g);
  return NULL;
}

static void enb_main_gui(enb_gui *e, gui *g, event_handler *h, void *database)
{
  widget *main_window;
  widget *top_container;
  widget *line, *col;
  widget *logo;
  widget *input_signal_plot;
  logger *input_signal_log;
  view *input_signal_view;
  widget *timeline_plot;
  logger *timelog;
  view *timeview;
  view *subview;
  widget *text;
  view *textview;
  int i;

  main_window = new_toplevel_window(g, 800, 600, "eNB tracer");
  top_container = new_container(g, VERTICAL);
  widget_add_child(g, main_window, top_container, -1);

  line = new_container(g, HORIZONTAL);
  widget_add_child(g, top_container, line, -1);
  logo = new_image(g, openair_logo_png, openair_logo_png_len);
  widget_add_child(g, line, logo, -1);
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
  widget_add_child(g, top_container,
      new_label(g,"DL/UL TICK/DCI/ACK/NACK "), -1);
  line = new_container(g, HORIZONTAL);
  widget_add_child(g, top_container, line, -1);
  timeline_plot = new_timeline(g, 512, 8, 5);
  widget_add_child(g, line, timeline_plot, -1);
  container_set_child_growable(g, line, timeline_plot, 1);
  for (i = 0; i < 8; i++)
    timeline_set_subline_background_color(g, timeline_plot, i,
        new_color(g, i==0 || i==4 ? "#aaf" : i & 1 ? "#ddd" : "#eee"));
  timeview = new_view_time(3600, 10, g, timeline_plot);
  /* tick logging */
  timelog = new_timelog(h, database, "ENB_DL_TICK");
  subview = new_subview_time(timeview, 0, FOREGROUND_COLOR, 3600*1000);
  logger_add_view(timelog, subview);
  /* DCI logging */
  timelog = new_timelog(h, database, "ENB_DLSCH_UE_DCI");
  subview = new_subview_time(timeview, 1, new_color(g, "#228"), 3600*1000);
  logger_add_view(timelog, subview);
  /* ACK */
  timelog = new_timelog(h, database, "ENB_DLSCH_UE_ACK");
  subview = new_subview_time(timeview, 2, new_color(g, "#282"), 3600*1000);
  logger_add_view(timelog, subview);
  /* NACK */
  timelog = new_timelog(h, database, "ENB_DLSCH_UE_NACK");
  subview = new_subview_time(timeview, 3, new_color(g, "#f22"), 3600*1000);
  logger_add_view(timelog, subview);

  /* uplink UE DCIs */
  timelog = new_timelog(h, database, "ENB_UL_TICK");
  subview = new_subview_time(timeview, 4, FOREGROUND_COLOR, 3600*1000);
  logger_add_view(timelog, subview);

  /* phy/mac/rlc/pdcp/rrc textlog */
  line = new_container(g, HORIZONTAL);
  widget_add_child(g, top_container, line, -1);
  container_set_child_growable(g, top_container, line, 1);

  /* phy */
  col = new_container(g, VERTICAL);
  widget_add_child(g, line, col, -1);
  container_set_child_growable(g, line, col, 1);
  widget_add_child(g, col, new_label(g, "PHY"), -1);
  text = new_textlist(g, 100, 10, new_color(g, "#afa"));
  widget_add_child(g, col, text, -1);
  container_set_child_growable(g, col, text, 1);
  textview = new_view_textlist(10000, 10, g, text);

  /* mac */
  col = new_container(g, VERTICAL);
  widget_add_child(g, line, col, -1);
  container_set_child_growable(g, line, col, 1);
  widget_add_child(g, col, new_label(g, "MAC"), -1);
  text = new_textlist(g, 100, 10, new_color(g, "#adf"));
  widget_add_child(g, col, text, -1);
  container_set_child_growable(g, col, text, 1);
  textview = new_view_textlist(10000, 10, g, text);

  /* rlc */
  col = new_container(g, VERTICAL);
  widget_add_child(g, line, col, -1);
  container_set_child_growable(g, line, col, 1);
  widget_add_child(g, col, new_label(g, "RLC"), -1);
  text = new_textlist(g, 100, 10, new_color(g, "#aff"));
  widget_add_child(g, col, text, -1);
  container_set_child_growable(g, col, text, 1);
  textview = new_view_textlist(10000, 10, g, text);

  /* pdcp */
  col = new_container(g, VERTICAL);
  widget_add_child(g, line, col, -1);
  container_set_child_growable(g, line, col, 1);
  widget_add_child(g, col, new_label(g, "PDCP"), -1);
  text = new_textlist(g, 100, 10, new_color(g, "#ed9"));
  widget_add_child(g, col, text, -1);
  container_set_child_growable(g, col, text, 1);
  textview = new_view_textlist(10000, 10, g, text);

  /* rrc */
  col = new_container(g, VERTICAL);
  widget_add_child(g, line, col, -1);
  container_set_child_growable(g, line, col, 1);
  widget_add_child(g, col, new_label(g, "RRC"), -1);
  text = new_textlist(g, 100, 10, new_color(g, "#fdb"));
  widget_add_child(g, col, text, -1);
  container_set_child_growable(g, col, text, 1);
  textview = new_view_textlist(10000, 10, g, text);
  e->rrcview = textview;
}

void view_add_log(view *v, char *log, event_handler *h, void *database,
    int *is_on)
{
  logger *textlog;
  char *name, *desc;

  database_get_generic_description(database,
      event_id_from_name(database, log), &name, &desc);
  textlog = new_textlog(h, database, name, desc);
  logger_add_view(textlog, v);
  free(name);
  free(desc);

  on_off(database, log, is_on, 1);
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
  enb_gui eg;

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

  store_config_file(database_filename);

  number_of_events = number_of_ids(database);
  is_on = calloc(number_of_events, sizeof(int));
  if (is_on == NULL) abort();

  h = new_handler(database);

  g = gui_init();
  new_thread(gui_thread, g);

  enb_main_gui(&eg, g, h, database);

  on_off(database, "ENB_INPUT_SIGNAL", is_on, 1);
  on_off(database, "ENB_UL_TICK", is_on, 1);
  on_off(database, "ENB_DL_TICK", is_on, 1);
  on_off(database, "ENB_DLSCH_UE_DCI", is_on, 1);
  on_off(database, "ENB_DLSCH_UE_ACK", is_on, 1);
  on_off(database, "ENB_DLSCH_UE_NACK", is_on, 1);

  view_add_log(eg.rrcview, "ENB_RRC_CONNECTION_SETUP_COMPLETE",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_SECURITY_MODE_COMMAND",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_UE_CAPABILITY_ENQUIRY",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_CONNECTION_REJECT",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_CONNECTION_REESTABLISHMENT_REJECT",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_CONNECTION_RELEASE",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_CONNECTION_RECONFIGURATION",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_MEASUREMENT_REPORT",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_HANDOVER_PREPARATION_INFORMATION",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_CONNECTION_RECONFIGURATION_COMPLETE",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_CONNECTION_SETUP",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_UL_CCCH_DATA_IN",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_UL_DCCH_DATA_IN",
      h, database, is_on);

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
