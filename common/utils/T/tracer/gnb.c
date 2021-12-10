#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include "database.h"
#include "event.h"
#include "handler.h"
#include "config.h"
#include "logger/logger.h"
#include "gui/gui.h"
#include "utils.h"
#include "openair_logo.h"

int ue_id[65536];
int next_ue_id;

typedef struct {
  widget *pucch_pusch_iq_plot;
  widget *ul_estimate_ue_xy_plot;
  widget *current_ue_label;
  widget *current_ue_button;
  widget *prev_ue_button;
  widget *next_ue_button;
  logger *pucch_pusch_iq_logger;
  logger *ul_estimate_ue_logger;
} gnb_gui;

typedef struct {
  int socket;
  int *is_on;
  int nevents;
  pthread_mutex_t lock;
  gnb_gui *e;
  int ue;                /* what UE is displayed in the UE specific views */
  void *database;
} gnb_data;

void is_on_changed(void *_d) {

  gnb_data *d = _d;
  char t;

  if (pthread_mutex_lock(&d->lock)) abort();

  if (d->socket == -1) goto no_connection;

  t = 1;
  if (socket_send(d->socket, &t, 1) == -1 ||
      socket_send(d->socket, &d->nevents, sizeof(int)) == -1 ||
      socket_send(d->socket, d->is_on, d->nevents * sizeof(int)) == -1)
    goto connection_dies;

no_connection:
  if (pthread_mutex_unlock(&d->lock)) abort();
  return;

connection_dies:
  close(d->socket);
  d->socket = -1;
  if (pthread_mutex_unlock(&d->lock)) abort();
}

void usage(void) {
  printf(
      "options:\n"
      "    -d   <database file>      this option is mandatory\n"
      "    -ip <host>                connect to given IP address (default %s)\n"
      "    -p  <port>                connect to given port (default %d)\n",
      DEFAULT_REMOTE_IP,
      DEFAULT_REMOTE_PORT
  );
  exit(1);
}

static void *gui_thread(void *_g) {
  gui *g = _g;
  gui_loop(g);
  return NULL;
}

static void set_current_ue(gui *g, gnb_data *e, int ue) {

  char s[256];

  sprintf(s, "[UE %d]  ", ue);
  label_set_text(g, e->e->current_ue_label, s);

  sprintf(s, "GNB_PHY_PUCCH_PUSCH_IQ [UE %d]", ue);
  xy_plot_set_title(g, e->e->pucch_pusch_iq_plot, s);

  sprintf(s, "UL estimated channel [UE %d]", ue);
  xy_plot_set_title(g, e->e->ul_estimate_ue_xy_plot, s);
}

void reset_ue_ids(void) {
  int i;
  printf("resetting known UEs\n");
  for (i = 0; i < 65536; i++) ue_id[i] = -1;
  ue_id[65535] = 0;
  ue_id[65534] = 1;     // HACK: to be removed
  ue_id[2]     = 2;     // this supposes RA RNTI = 2, very openair specific
  next_ue_id = 0;
}

static void click(void *private, gui *g, char *notification, widget *w, void *notification_data) {
  int *d = notification_data;
  int button = d[0];
  gnb_data *ed = private;
  gnb_gui *e = ed->e;
  int ue = ed->ue;
  int do_reset = 0;

  if (button != 1) return;
  if (w == e->prev_ue_button) { ue--; if (ue < 0) ue = 0; }
  if (w == e->next_ue_button) ue++;
  if (w == e->current_ue_button) do_reset = 1;

  if (pthread_mutex_lock(&ed->lock)) abort();
  if (do_reset) reset_ue_ids();
  if (ue != ed->ue) {
    set_current_ue(g, ed, ue);
    ed->ue = ue;
  }
  if (pthread_mutex_unlock(&ed->lock)) abort();
}

static void gnb_main_gui(gnb_gui *e, gui *g, event_handler *h, void *database, gnb_data *ed) {

  widget *main_window;
  widget *top_container;
  widget *line;
  widget *col;
  widget *logo;
  widget *w;
  widget *w2;
  logger *l;
  view *v;

  main_window = new_toplevel_window(g, 1050, 230, "gNB tracer");
  top_container = new_container(g, VERTICAL);
  widget_add_child(g, main_window, top_container, -1);

  line = new_container(g, HORIZONTAL);
  widget_add_child(g, top_container, line, -1);
  logo = new_image(g, openair_logo_png, openair_logo_png_len);

  // logo + prev/next UE buttons
  col = new_container(g, VERTICAL);
  widget_add_child(g, col, logo, -1);
  w = new_container(g, HORIZONTAL);
  widget_add_child(g, col, w, -1);
  w2 = new_label(g, "");
  widget_add_child(g, w, w2, -1);
  label_set_clickable(g, w2, 1);
  e->current_ue_button = w2;
  e->current_ue_label = w2;
  w2 = new_label(g, "  [prev UE]  ");
  widget_add_child(g, w, w2, -1);
  label_set_clickable(g, w2, 1);
  e->prev_ue_button = w2;
  w2 = new_label(g, "  [next UE]  ");
  widget_add_child(g, w, w2, -1);
  label_set_clickable(g, w2, 1);
  e->next_ue_button = w2;
  widget_add_child(g, line, col, -1);

  // PUCCH/PUSCH IQ data
  w = new_xy_plot(g, 200, 200, "", 10);
  e->pucch_pusch_iq_plot = w;
  widget_add_child(g, line, w, -1);
  xy_plot_set_range(g, w, -1000, 1000, -1000, 1000);
  l = new_iqlog_full(h, database, "GNB_PHY_PUCCH_PUSCH_IQ", "rxdataF");
  v = new_view_xy(300*12*14,10,g,w,new_color(g,"#000"),XY_FORCED_MODE);
  logger_add_view(l, v);
  e->pucch_pusch_iq_logger = l;

  // UL estimated channel
  w = new_xy_plot(g, 600, 200, "", 50);
  e->ul_estimate_ue_xy_plot = w;
  widget_add_child(g, line, w, -1);
  xy_plot_set_range(g, w, 0, 512*10, -10, 80);
  l = new_framelog(h, database, "ENB_PHY_UL_CHANNEL_ESTIMATE", "subframe", "chest_t");
  framelog_set_update_only_at_sf9(l, 0);
  v = new_view_xy(512*10, 10, g, w, new_color(g, "#0c0c72"), XY_LOOP_MODE);
  logger_add_view(l, v);
  e->ul_estimate_ue_logger = l;

  set_current_ue(g, ed, ed->ue);
  register_notifier(g, "click", e->current_ue_button, click, ed);
  register_notifier(g, "click", e->prev_ue_button, click, ed);
  register_notifier(g, "click", e->next_ue_button, click, ed);
}

int main(int n, char **v) {

  char *database_filename = NULL;
  void *database;
  char *ip = DEFAULT_REMOTE_IP;
  int port = DEFAULT_REMOTE_PORT;
  int *is_on;
  int number_of_events;
  int i;
  event_handler *h;
  gnb_data gnb_data;
  gui *g;
  gnb_gui eg;

  for (i = 1; i < n; i++) {
    if (!strcmp(v[i], "-h") || !strcmp(v[i], "--help")) usage();
    if (!strcmp(v[i], "-d")) {
      if (i > n-2) usage();
      database_filename = v[++i];
      continue;
    }
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

  on_off(database, "GNB_PHY_PUCCH_PUSCH_IQ", is_on, 1);
  on_off(database, "ENB_PHY_UL_CHANNEL_ESTIMATE", is_on, 1);

  gnb_data.ue = 0;
  gnb_data.e = &eg;
  gnb_data.database = database;
  gnb_data.socket = -1;
  gnb_data.is_on = is_on;
  gnb_data.nevents = number_of_events;
  if (pthread_mutex_init(&gnb_data.lock, NULL)) abort();

  g = gui_init();
  new_thread(gui_thread, g);

  gnb_main_gui(&eg, g, h, database, &gnb_data);

  OBUF ebuf = { osize: 0, omaxsize: 0, obuf: NULL };

restart:
  clear_remote_config();
  if (gnb_data.socket != -1) close(gnb_data.socket);
  gnb_data.socket = connect_to(ip, port);

  // send the first message - activate selected traces
  is_on_changed(&gnb_data);

  /* read messages */
  while (1) {
    event e;
    e = get_event(gnb_data.socket, &ebuf, database);
    if (e.type == -1) goto restart;
    if (pthread_mutex_lock(&gnb_data.lock)) abort();
    handle_event(h, e);
    if (pthread_mutex_unlock(&gnb_data.lock)) abort();
  }

  return 0;
}
