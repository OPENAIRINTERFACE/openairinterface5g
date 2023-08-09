#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include "database.h"
#include "event.h"
#include "handler.h"
#include "logger/logger.h"
#include "view/view.h"
#include "gui/gui.h"
#include "filter/filter.h"
#include "utils.h"
#include "event_selector.h"
#include "openair_logo.h"
#include "configuration.h"

/****************************************************************************/
/* conversion from rnti to "ue_id" (which does not really exists)           */
/* the idea is to assign an ue_id to an rnti as soon as we get an event     */
/* for the rnti                                                             */
/****************************************************************************/

int ue_id[65536];
int next_ue_id;

void reset_ue_ids(void)
{
  int i;
  printf("resetting known UEs\n");
  for (i = 0; i < 65536; i++) ue_id[i] = -1;
  ue_id[65535] = 0;
  ue_id[65534] = 1;     /* HACK: to be removed */
  ue_id[2]     = 2;     /* this supposes RA RNTI = 2, very openair specific */
  next_ue_id = 3;
}

int ue_id_from_rnti(void *_priv, int rnti)
{
  if (rnti < 0) rnti = 65534; /* HACK, to be removed */

  if (rnti < 0 || rnti > 65535) { printf("bad rnti %d\n", rnti); exit(1); }
  /* rnti not seen yet? give it a new ue_id */
  if (ue_id[rnti] == -1) {
    ue_id[rnti] = next_ue_id;
    next_ue_id++;
  }
  return ue_id[rnti];
}

/****************************************************************************/
/* end of rnti->ue_id conversion                                            */
/****************************************************************************/


typedef struct {
  view *phyview;
  view *macview;
  view *rlcview;
  view *pdcpview;
  view *rrcview;
  view *legacy;
  widget *current_ue_label;
  widget *current_ue_button;
  widget *prev_ue_button;
  widget *next_ue_button;
  widget *pusch_iq_ue_xy_plot;
  widget *ul_estimate_ue_xy_plot;
  widget *pucch1_energy_ue_xy_plot;
  widget *pucch_iq_ue_xy_plot;
  widget *dl_ul_harq_ue_label;
  widget *dl_mcs_xy_plot;
  widget *ul_mcs_xy_plot;
  logger *pusch_iq_ue_logger;
  logger *ul_estimate_ue_logger;
  logger *pucch1_energy_ue_threshold_logger;
  logger *pucch1_energy_ue_energy_logger;
  logger *pucch_iq_ue_logger;
  logger *dl_dci_logger[8];
  logger *dl_ack_logger[8];
  logger *dl_nack_logger[8];
  logger *ul_dci_logger[8];
  logger *ul_dci_retransmission_logger[8];
  logger *ul_ack_logger[8];
  logger *ul_nack_logger[8];
  logger *dl_mcs_logger;
  logger *ul_mcs_logger;
} enb_gui;

typedef struct {
  int socket;
  int *is_on;
  int nevents;
  pthread_mutex_t lock;
  enb_gui *e;
  int ue;                /* what UE is displayed in the UE specific views */
  void *database;
  int nb_rb;
} enb_data;

void is_on_changed(void *_d)
{
  enb_data *d = _d;
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

static void *gui_thread(void *_g)
{
  gui *g = _g;
  gui_loop(g);
  return NULL;
}

static filter *ticktime_filter(void *database, char *event, int i, int ue)
{
  /* filter is "harq_pid == i && UE_id == 0 && eNB_id == 0" */
  /* we get the UE_id from the rnti */
  return
    filter_and(
      filter_eq(filter_evarg(database, event, "harq_pid"), filter_int(i)),
      filter_and(
        filter_eq(
          filter_evfun(database, ue_id_from_rnti, NULL,
            filter_evarg(database, event, "rnti")),
          filter_int(ue)),
        filter_eq(filter_evarg(database, event, "eNB_ID"), filter_int(0))));
}

static void set_current_ue(gui *g, enb_data *e, int ue)
{
  int i;
  char s[256];

  sprintf(s, "[UE %d]  ", ue);
  label_set_text(g, e->e->current_ue_label, s);
  sprintf(s, "PUSCH IQ [UE %d]", ue);
  xy_plot_set_title(g, e->e->pusch_iq_ue_xy_plot, s);
  sprintf(s, "UL estimated channel [UE %d]", ue);
  xy_plot_set_title(g, e->e->ul_estimate_ue_xy_plot, s);
  sprintf(s, "PUCCH1 energy (SR) [UE %d]", ue);
  xy_plot_set_title(g, e->e->pucch1_energy_ue_xy_plot, s);
  sprintf(s, "PUCCH IQ [UE %d]", ue);
  xy_plot_set_title(g, e->e->pucch_iq_ue_xy_plot, s);
  sprintf(s, "DL/UL HARQ (x8) [UE %d]", ue);
  label_set_text(g, e->e->dl_ul_harq_ue_label, s);
  sprintf(s, "DL MCS [UE %d]", ue);
  xy_plot_set_title(g, e->e->dl_mcs_xy_plot, s);
  sprintf(s, "UL MCS [UE %d]", ue);
  xy_plot_set_title(g, e->e->ul_mcs_xy_plot, s);

  logger_set_filter(e->e->pusch_iq_ue_logger,
      filter_eq(
        filter_evfun(e->database, ue_id_from_rnti, NULL,
          filter_evarg(e->database, "ENB_PHY_PUSCH_IQ", "rnti")),
        filter_int(ue)));
  logger_set_filter(e->e->ul_estimate_ue_logger,
      filter_eq(
        filter_evfun(e->database, ue_id_from_rnti, NULL,
          filter_evarg(e->database, "ENB_PHY_UL_CHANNEL_ESTIMATE", "rnti")),
        filter_int(ue)));
  logger_set_filter(e->e->pucch1_energy_ue_threshold_logger,
      filter_eq(
        filter_evfun(e->database, ue_id_from_rnti, NULL,
          filter_evarg(e->database, "ENB_PHY_PUCCH_1_ENERGY", "rnti")),
        filter_int(ue)));
  logger_set_filter(e->e->pucch1_energy_ue_energy_logger,
      filter_eq(
        filter_evfun(e->database, ue_id_from_rnti, NULL,
          filter_evarg(e->database, "ENB_PHY_PUCCH_1_ENERGY", "rnti")),
        filter_int(ue)));
  logger_set_filter(e->e->pucch_iq_ue_logger,
      filter_eq(
        filter_evfun(e->database, ue_id_from_rnti, NULL,
          filter_evarg(e->database, "ENB_PHY_PUCCH_1AB_IQ", "rnti")),
        filter_int(ue)));
  for (i = 0; i < 8; i++) {
    logger_set_filter(e->e->dl_dci_logger[i],
        ticktime_filter(e->database, "ENB_PHY_DLSCH_UE_DCI", i, ue));
    logger_set_filter(e->e->dl_ack_logger[i],
        ticktime_filter(e->database, "ENB_PHY_DLSCH_UE_ACK", i, ue));
    logger_set_filter(e->e->dl_nack_logger[i],
        ticktime_filter(e->database, "ENB_PHY_DLSCH_UE_NACK", i, ue));
    logger_set_filter(e->e->ul_dci_logger[i],
        ticktime_filter(e->database, "ENB_PHY_ULSCH_UE_DCI", i, ue));
    logger_set_filter(e->e->ul_dci_retransmission_logger[i],
        ticktime_filter(e->database,
            "ENB_PHY_ULSCH_UE_NO_DCI_RETRANSMISSION", i, ue));
    logger_set_filter(e->e->ul_ack_logger[i],
        ticktime_filter(e->database, "ENB_PHY_ULSCH_UE_ACK", i, ue));
    logger_set_filter(e->e->ul_nack_logger[i],
        ticktime_filter(e->database, "ENB_PHY_ULSCH_UE_NACK", i, ue));
  }
  logger_set_filter(e->e->dl_mcs_logger,
      filter_eq(
        filter_evfun(e->database, ue_id_from_rnti, NULL,
          filter_evarg(e->database, "ENB_PHY_DLSCH_UE_DCI", "rnti")),
        filter_int(ue)));
  logger_set_filter(e->e->ul_mcs_logger,
      filter_eq(
        filter_evfun(e->database, ue_id_from_rnti, NULL,
          filter_evarg(e->database, "ENB_PHY_ULSCH_UE_DCI", "rnti")),
        filter_int(ue)));
}

static void click(void *private, gui *g,
    char *notification, widget *w, void *notification_data)
{
  int *d = notification_data;
  int button = d[0];
  enb_data *ed = private;
  enb_gui *e = ed->e;
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

static void enb_main_gui(enb_gui *e, gui *g, event_handler *h, void *database,
    enb_data *ed)
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
  widget *w, *w2;
  view *v;
  logger *l;

  main_window = new_toplevel_window(g, 1200, 900, "eNB tracer");
  top_container = new_container(g, VERTICAL);
  widget_add_child(g, main_window, top_container, -1);

  line = new_container(g, HORIZONTAL);
  widget_add_child(g, top_container, line, -1);
  logo = new_image(g, openair_logo_png, openair_logo_png_len);

  /* logo + prev/next UE buttons */
  col = new_container(g, VERTICAL);
  widget_add_child(g, col, logo, -1);
  w = new_container(g, HORIZONTAL);
  widget_add_child(g, col, w, -1);
  /* TODO: use button widget, not label widget */
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

  input_signal_plot = new_xy_plot(g, 256, 55, "input signal", 20);
  widget_add_child(g, line, input_signal_plot, -1);
  xy_plot_set_range(g, input_signal_plot, 0, 7680*10 * ed->nb_rb/25, 20, 70);
  input_signal_log = new_framelog(h, database,
      "ENB_PHY_INPUT_SIGNAL", "subframe", "rxdata");
  /* a skip value of 10 means to process 1 frame over 10, that is
   * more or less 10 frames per second
   */
  framelog_set_skip(input_signal_log, 10);
  input_signal_view = new_view_xy(7680*10 * ed->nb_rb/25, 10,
      g, input_signal_plot, new_color(g, "#0c0c72"), XY_LOOP_MODE);
  logger_add_view(input_signal_log, input_signal_view);

  /* UE x PUSCH IQ data */
  w = new_xy_plot(g, 55, 55, "", 50);
  e->pusch_iq_ue_xy_plot = w;
  widget_add_child(g, line, w, -1);
  xy_plot_set_range(g, w, -1000, 1000, -1000, 1000);
  l = new_iqlog(h, database, "ENB_PHY_PUSCH_IQ", "nb_rb",
      "N_RB_UL", "symbols_per_tti", "pusch_comp");
  v = new_view_xy(100*12*14,10,g,w,new_color(g,"#000"),XY_FORCED_MODE);
  logger_add_view(l, v);
  e->pusch_iq_ue_logger = l;

  /* UE x estimated UL channel */
  w = new_xy_plot(g, 280, 55, "", 50);
  e->ul_estimate_ue_xy_plot = w;
  widget_add_child(g, line, w, -1);
  xy_plot_set_range(g, w, 0, 512*10, -10, 80);
  l = new_framelog(h, database,
      "ENB_PHY_UL_CHANNEL_ESTIMATE", "subframe", "chest_t");
  //framelog_set_skip(input_signal_log, 10);
  framelog_set_update_only_at_sf9(l, 0);
  v = new_view_xy(512*10, 10, g, w, new_color(g, "#0c0c72"), XY_LOOP_MODE);
  logger_add_view(l, v);
  e->ul_estimate_ue_logger = l;

  /* UE x PUCCH energy */
  w = new_xy_plot(g, 128, 55, "", 50);
  e->pucch1_energy_ue_xy_plot = w;
  widget_add_child(g, line, w, -1);
  xy_plot_set_range(g, w, 0, 1024*10, -10, 80);
  l = new_ttilog(h, database,
      "ENB_PHY_PUCCH_1_ENERGY", "frame", "subframe", "threshold", 0);
  v = new_view_tti(10, g, w, new_color(g, "#ff0000"));
  logger_add_view(l, v);
  e->pucch1_energy_ue_threshold_logger = l;
  l = new_ttilog(h, database,
      "ENB_PHY_PUCCH_1_ENERGY", "frame", "subframe", "energy", 1);
  v = new_view_tti(10, g, w, new_color(g, "#0c0c72"));
  logger_add_view(l, v);
  e->pucch1_energy_ue_energy_logger = l;

  /* UE x PUCCH IQ data */
  w = new_xy_plot(g, 55, 55, "", 50);
  e->pucch_iq_ue_xy_plot = w;
  widget_add_child(g, line, w, -1);
  xy_plot_set_range(g, w, -2000, 2000, -2000, 2000);
  l = new_iqdotlog(h, database, "ENB_PHY_PUCCH_1AB_IQ", "I", "Q");
  v = new_view_xy(500, 10, g, w, new_color(g,"#000"), XY_LOOP_MODE);
  logger_add_view(l, v);
  e->pucch_iq_ue_logger = l;

  /* UE x DL mcs */
  line = new_container(g, HORIZONTAL);
  widget_add_child(g, top_container, line, -1);
  w = new_xy_plot(g, 128, 55, "", 20);
  xy_plot_set_range(g, w, 0, 1024*10, -2, 30);
  e->dl_mcs_xy_plot = w;
  widget_add_child(g, line, w, -1);
  l = new_ticked_ttilog(h, database, "ENB_PHY_DL_TICK", "frame", "subframe",
      "ENB_PHY_DLSCH_UE_DCI", "mcs", 0, -1);
  v = new_view_tti(10, g, w, new_color(g, "#0c0c72"));
  logger_add_view(l, v);
  e->dl_mcs_logger = l;

  /* UE x UL mcs */
  w = new_xy_plot(g, 128, 55, "", 20);
  xy_plot_set_range(g, w, 0, 1024*10, -2, 30);
  e->ul_mcs_xy_plot = w;
  widget_add_child(g, line, w, -1);
  l = new_ticked_ttilog(h, database, "ENB_PHY_DL_TICK", "frame", "subframe",
      "ENB_PHY_ULSCH_UE_DCI", "mcs", 0, -1);
  v = new_view_tti(10, g, w, new_color(g, "#0c0c72"));
  logger_add_view(l, v);
  e->ul_mcs_logger = l;

  /* UE x DL PHY (truly: DCI) throughput */
  col = new_container(g, VERTICAL);
  widget_add_child(g, line, col, -1);
  w = new_xy_plot(g, 70, 10, "DL PHY [0]", 35);
  w2 = new_textarea(g, 70, 11, 64);
  xy_plot_set_range(g, w, 0, 1000, 0, 100000);
  xy_plot_set_tick_type(g, w, XY_PLOT_SCROLL_TICK);
  widget_add_child(g, col, w2, -1);
  widget_add_child(g, col, w, -1);
  container_set_child_growable(g, col, w, 1);
  l = new_throughputlog(h, database, "ENB_PHY_DL_TICK", "frame", "subframe",
      "ENB_PHY_DLSCH_UE_DCI", "TBS");
  v = new_view_scrolltti(10, g, w, new_color(g, "#0c0c72"), w2);
  logger_add_view(l, v);

  /* UE x UL PHY (truly: DCI) throughput */
  col = new_container(g, VERTICAL);
  widget_add_child(g, line, col, -1);
  w = new_xy_plot(g, 70, 10, "UL PHY [0]", 35);
  w2 = new_textarea(g, 70, 11, 64);
  xy_plot_set_range(g, w, 0, 1000, 0, 100000);
  xy_plot_set_tick_type(g, w, XY_PLOT_SCROLL_TICK);
  widget_add_child(g, col, w2, -1);
  widget_add_child(g, col, w, -1);
  container_set_child_growable(g, col, w, 1);
  l = new_throughputlog(h, database, "ENB_PHY_DL_TICK", "frame", "subframe",
      "ENB_PHY_ULSCH_UE_DCI", "TBS");
  v = new_view_scrolltti(10, g, w, new_color(g, "#0c0c72"), w2);
  logger_add_view(l, v);

  /* downlink/uplink UE DCIs */
  widget_add_child(g, top_container,
      new_label(g,"DL/UL TICK/DCI/ACK/NACK [all UEs]"), -1);
  line = new_container(g, HORIZONTAL);
  widget_add_child(g, top_container, line, -1);
  timeline_plot = new_timeline(g, 512, 8, 5);
  widget_add_child(g, line, timeline_plot, -1);
  container_set_child_growable(g, line, timeline_plot, 1);
  for (i = 0; i < 8; i++)
    timeline_set_subline_background_color(g, timeline_plot, i,
        new_color(g, i==0 || i==4 ? "#aaf" : "#eee"));
  timeview = new_view_time(3600, 10, g, timeline_plot);
  /* DL tick logging */
  timelog = new_timelog(h, database, "ENB_PHY_DL_TICK");
  subview = new_subview_time(timeview, 0, new_color(g, "#77c"), 3600*1000);
  logger_add_view(timelog, subview);
  /* DL DCI logging */
  timelog = new_timelog(h, database, "ENB_PHY_DLSCH_UE_DCI");
  subview = new_subview_time(timeview, 1, new_color(g, "#228"), 3600*1000);
  logger_add_view(timelog, subview);
  /* DL ACK */
  timelog = new_timelog(h, database, "ENB_PHY_DLSCH_UE_ACK");
  subview = new_subview_time(timeview, 2, new_color(g, "#282"), 3600*1000);
  logger_add_view(timelog, subview);
  /* DL NACK */
  timelog = new_timelog(h, database, "ENB_PHY_DLSCH_UE_NACK");
  subview = new_subview_time(timeview, 3, new_color(g, "#f22"), 3600*1000);
  logger_add_view(timelog, subview);

  /* UL tick logging */
  timelog = new_timelog(h, database, "ENB_PHY_UL_TICK");
  subview = new_subview_time(timeview, 4, new_color(g, "#77c"), 3600*1000);
  logger_add_view(timelog, subview);
  /* UL DCI logging */
  timelog = new_timelog(h, database, "ENB_PHY_ULSCH_UE_DCI");
  subview = new_subview_time(timeview, 5, new_color(g, "#228"), 3600*1000);
  logger_add_view(timelog, subview);
  /* UL retransmission without DCI logging */
  timelog = new_timelog(h,database,"ENB_PHY_ULSCH_UE_NO_DCI_RETRANSMISSION");
  subview = new_subview_time(timeview, 5, new_color(g, "#f22"), 3600*1000);
  logger_add_view(timelog, subview);
  /* UL ACK */
  timelog = new_timelog(h, database, "ENB_PHY_ULSCH_UE_ACK");
  subview = new_subview_time(timeview, 6, new_color(g, "#282"), 3600*1000);
  logger_add_view(timelog, subview);
  /* UL NACK */
  timelog = new_timelog(h, database, "ENB_PHY_ULSCH_UE_NACK");
  subview = new_subview_time(timeview, 7, new_color(g, "#f22"), 3600*1000);
  logger_add_view(timelog, subview);

  /* harq processes' ticktime view */
  e->dl_ul_harq_ue_label = new_label(g, "");
  widget_add_child(g, top_container, e->dl_ul_harq_ue_label, -1);
  line = new_container(g, HORIZONTAL);
  widget_add_child(g, top_container, line, -1);
  timeline_plot = new_timeline(g, 512, 2*8+2, 3);
  widget_add_child(g, line, timeline_plot, -1);
  container_set_child_growable(g, line, timeline_plot, 1);
  for (i = 0; i < 2*8+2; i++)
    timeline_set_subline_background_color(g, timeline_plot, i,
        new_color(g, i==0 || i==9 ? "#ddd" : (i%9)&1 ? "#e6e6e6" : "#eee"));
  timeview = new_view_ticktime(10, g, timeline_plot);
  ticktime_set_tick(timeview,
      new_ticklog(h, database, "ENB_MASTER_TICK", "frame", "subframe"));
  /* tick */
  timelog = new_ticklog(h, database, "ENB_MASTER_TICK", "frame", "subframe");
  /* tick on DL view */
  subview = new_subview_ticktime(timeview, 0, new_color(g,"#bbb"), 3600*1000);
  logger_add_view(timelog, subview);
  /* tick on UL view */
  subview = new_subview_ticktime(timeview, 9, new_color(g,"#bbb"), 3600*1000);
  logger_add_view(timelog, subview);
  /* DL DCI */
  for (i = 0; i < 8; i++) {
    timelog = new_ticklog(h, database, "ENB_PHY_DLSCH_UE_DCI",
        "frame", "subframe");
    subview = new_subview_ticktime(timeview, i+1,
        new_color(g,"#55f"), 3600*1000);
    logger_add_view(timelog, subview);
    e->dl_dci_logger[i] = timelog;
  }
  /* DL ACK */
  for (i = 0; i < 8; i++) {
    timelog = new_ticklog(h, database, "ENB_PHY_DLSCH_UE_ACK",
        "frame", "subframe");
    subview = new_subview_ticktime(timeview, i+1,
        new_color(g,"#282"), 3600*1000);
    logger_add_view(timelog, subview);
    e->dl_ack_logger[i] = timelog;
  }
  /* DL NACK */
  for (i = 0; i < 8; i++) {
    timelog = new_ticklog(h, database, "ENB_PHY_DLSCH_UE_NACK",
        "frame", "subframe");
    subview = new_subview_ticktime(timeview, i+1,
        new_color(g,"#f22"), 3600*1000);
    logger_add_view(timelog, subview);
    e->dl_nack_logger[i] = timelog;
  }
  /* UL DCI/retransmission without DCI */
  for (i = 0; i < 8; i++) {
    /* first transmission */
    timelog = new_ticklog(h, database, "ENB_PHY_ULSCH_UE_DCI",
        "frame", "subframe");
    subview = new_subview_ticktime(timeview, i+9+1,
        new_color(g,"#55f"), 3600*1000);
    logger_add_view(timelog, subview);
    e->ul_dci_logger[i] = timelog;
    /* retransmission */
    timelog = new_ticklog(h, database,
        "ENB_PHY_ULSCH_UE_NO_DCI_RETRANSMISSION", "frame", "subframe");
    subview = new_subview_ticktime(timeview, i+9+1,
        new_color(g,"#99f"), 3600*1000);
    logger_add_view(timelog, subview);
    e->ul_dci_retransmission_logger[i] = timelog;
  }
  /* UL ACK */
  for (i = 0; i < 8; i++) {
    timelog = new_ticklog(h, database, "ENB_PHY_ULSCH_UE_ACK",
        "frame", "subframe");
    subview = new_subview_ticktime(timeview, i+9+1,
        new_color(g,"#282"), 3600*1000);
    logger_add_view(timelog, subview);
    e->ul_ack_logger[i] = timelog;
  }
  /* UL NACK */
  for (i = 0; i < 8; i++) {
    timelog = new_ticklog(h, database, "ENB_PHY_ULSCH_UE_NACK",
        "frame", "subframe");
    subview = new_subview_ticktime(timeview, i+9+1,
        new_color(g,"#f22"), 3600*1000);
    logger_add_view(timelog, subview);
    e->ul_nack_logger[i] = timelog;
  }

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
  e->phyview = textview;

  /* mac */
  col = new_container(g, VERTICAL);
  widget_add_child(g, line, col, -1);
  container_set_child_growable(g, line, col, 1);
  widget_add_child(g, col, new_label(g, "MAC"), -1);
  text = new_textlist(g, 100, 10, new_color(g, "#adf"));
  widget_add_child(g, col, text, -1);
  container_set_child_growable(g, col, text, 1);
  textview = new_view_textlist(10000, 10, g, text);
  e->macview = textview;

  line = new_container(g, HORIZONTAL);
  widget_add_child(g, top_container, line, -1);
  container_set_child_growable(g, top_container, line, 1);

  /* rlc */
  col = new_container(g, VERTICAL);
  widget_add_child(g, line, col, -1);
  container_set_child_growable(g, line, col, 1);
  widget_add_child(g, col, new_label(g, "RLC"), -1);
  text = new_textlist(g, 100, 10, new_color(g, "#aff"));
  widget_add_child(g, col, text, -1);
  container_set_child_growable(g, col, text, 1);
  textview = new_view_textlist(10000, 10, g, text);
  e->rlcview = textview;

  /* pdcp */
  col = new_container(g, VERTICAL);
  widget_add_child(g, line, col, -1);
  container_set_child_growable(g, line, col, 1);
  widget_add_child(g, col, new_label(g, "PDCP"), -1);
  text = new_textlist(g, 100, 10, new_color(g, "#ed9"));
  widget_add_child(g, col, text, -1);
  container_set_child_growable(g, col, text, 1);
  textview = new_view_textlist(10000, 10, g, text);
  e->pdcpview = textview;

  line = new_container(g, HORIZONTAL);
  widget_add_child(g, top_container, line, -1);
  container_set_child_growable(g, top_container, line, 1);

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

  /* legacy logs (LOG_I, LOG_D, ...) */
  widget_add_child(g, top_container, new_label(g, "LEGACY"), -1);
  text = new_textlist(g, 100, 10, new_color(g, "#eeb"));
  widget_add_child(g, top_container, text, -1);
  container_set_child_growable(g, top_container, text, 1);
  e->legacy = new_view_textlist(10000, 10, g, text);

  set_current_ue(g, ed, ed->ue);
  register_notifier(g, "click", e->current_ue_button, click, ed);
  register_notifier(g, "click", e->prev_ue_button, click, ed);
  register_notifier(g, "click", e->next_ue_button, click, ed);
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

void usage(void)
{
  printf(
"options:\n"
"    -d   <database file>      this option is mandatory\n"
"    -rb  <RBs>                setup for this number of RBs (default 25)\n"
"    -on  <GROUP or ID>        turn log ON for given GROUP or ID\n"
"    -off <GROUP or ID>        turn log OFF for given GROUP or ID\n"
"    -ON                       turn all logs ON\n"
"    -OFF                      turn all logs OFF\n"
"                              note: you may pass several -on/-off/-ON/-OFF,\n"
"                                    they will be processed in order\n"
"                                    by default, all is off\n"
"    -ip <host>                connect to given IP address (default %s)\n"
"    -p  <port>                connect to given port (default %d)\n"
"    -debug-gui                activate GUI debug logs\n",
  DEFAULT_REMOTE_IP,
  DEFAULT_REMOTE_PORT
  );
  exit(1);
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
  enb_gui eg;
  enb_data enb_data;

  reset_ue_ids();

  enb_data.nb_rb = 25;

  /* write on a socket fails if the other end is closed and we get SIGPIPE */
  if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) abort();

  on_off_name = malloc(n * sizeof(char *)); if (on_off_name == NULL) abort();
  on_off_action = malloc(n * sizeof(int)); if (on_off_action == NULL) abort();

  for (i = 1; i < n; i++) {
    if (!strcmp(v[i], "-h") || !strcmp(v[i], "--help")) usage();
    if (!strcmp(v[i], "-d"))
      { if (i > n-2) usage(); database_filename = v[++i]; continue; }
    if (!strcmp(v[i], "-rb"))
      { if (i > n-2) usage(); enb_data.nb_rb = atoi(v[++i]); continue; }
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

  switch (enb_data.nb_rb) {
  case 25: case 50: case 100: break;
  default: printf("ERROR, bad value for -rb (%d)\n", enb_data.nb_rb); exit(1);
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

  enb_data.ue = 3;
  enb_data.e = &eg;
  enb_data.database = database;

  enb_main_gui(&eg, g, h, database, &enb_data);

  for (i = 0; i < number_of_events; i++) {
    logger *textlog;
    char *name, *desc;
    database_get_generic_description(database, i, &name, &desc);
    if (!strncmp(name, "LEGACY_", 7)) {
      textlog = new_textlog(h, database, name, desc);
      logger_add_view(textlog, eg.legacy);
    }
    free(name);
    free(desc);
  }

  on_off(database, "ENB_PHY_INPUT_SIGNAL", is_on, 1);
  on_off(database, "ENB_PHY_UL_CHANNEL_ESTIMATE", is_on, 1);
  on_off(database, "ENB_PHY_DL_TICK", is_on, 1);
  on_off(database, "ENB_PHY_DLSCH_UE_DCI", is_on, 1);
  on_off(database, "ENB_PHY_DLSCH_UE_ACK", is_on, 1);
  on_off(database, "ENB_PHY_DLSCH_UE_NACK", is_on, 1);
  on_off(database, "ENB_PHY_UL_TICK", is_on, 1);
  on_off(database, "ENB_PHY_ULSCH_UE_DCI", is_on, 1);
  on_off(database, "ENB_PHY_ULSCH_UE_NO_DCI_RETRANSMISSION", is_on, 1);
  on_off(database, "ENB_PHY_ULSCH_UE_ACK", is_on, 1);
  on_off(database, "ENB_PHY_ULSCH_UE_NACK", is_on, 1);
  on_off(database, "ENB_MASTER_TICK", is_on, 1);
  on_off(database, "ENB_PHY_PUSCH_IQ", is_on, 1);
  on_off(database, "ENB_PHY_PUCCH_1_ENERGY", is_on, 1);
  on_off(database, "ENB_PHY_PUCCH_1AB_IQ", is_on, 1);

  on_off(database, "LEGACY_GROUP_INFO", is_on, 1);
  on_off(database, "LEGACY_GROUP_ERROR", is_on, 1);
  on_off(database, "LEGACY_GROUP_WARNING", is_on, 1);

  view_add_log(eg.phyview, "ENB_PHY_DLSCH_UE_DCI", h, database, is_on);
  view_add_log(eg.phyview, "ENB_PHY_DLSCH_UE_ACK", h, database, is_on);
  view_add_log(eg.phyview, "ENB_PHY_DLSCH_UE_NACK", h, database, is_on);
  view_add_log(eg.phyview, "ENB_PHY_ULSCH_UE_DCI", h, database, is_on);
  view_add_log(eg.phyview, "ENB_PHY_ULSCH_UE_NO_DCI_RETRANSMISSION",
      h, database, is_on);
  view_add_log(eg.phyview, "ENB_PHY_ULSCH_UE_ACK", h, database, is_on);
  view_add_log(eg.phyview, "ENB_PHY_ULSCH_UE_NACK", h, database, is_on);

  view_add_log(eg.macview, "ENB_MAC_UE_DL_SDU", h, database, is_on);
  view_add_log(eg.macview, "ENB_MAC_UE_UL_SCHEDULE", h, database, is_on);
  view_add_log(eg.macview, "ENB_MAC_UE_UL_SCHEDULE_RETRANSMISSION",
      h, database, is_on);
  view_add_log(eg.macview, "ENB_MAC_UE_UL_PDU", h, database, is_on);
  view_add_log(eg.macview, "ENB_MAC_UE_UL_PDU_WITH_DATA", h, database, is_on);
  view_add_log(eg.macview, "ENB_MAC_UE_UL_SDU", h, database, is_on);
  view_add_log(eg.macview, "ENB_MAC_UE_UL_SDU_WITH_DATA", h, database, is_on);
  view_add_log(eg.macview, "ENB_MAC_UE_UL_CE", h, database, is_on);

  view_add_log(eg.rlcview, "ENB_RLC_DL", h, database, is_on);
  view_add_log(eg.rlcview, "ENB_RLC_UL", h, database, is_on);
  view_add_log(eg.rlcview, "ENB_RLC_MAC_DL", h, database, is_on);
  view_add_log(eg.rlcview, "ENB_RLC_MAC_UL", h, database, is_on);

  view_add_log(eg.pdcpview, "ENB_PDCP_UL", h, database, is_on);
  view_add_log(eg.pdcpview, "ENB_PDCP_DL", h, database, is_on);

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
  view_add_log(eg.rrcview, "ENB_RRC_SECURITY_MODE_COMPLETE",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_SECURITY_MODE_FAILURE",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_UE_CAPABILITY_INFORMATION",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_CONNECTION_REQUEST",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_CONNECTION_REESTABLISHMENT_REQUEST",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_CONNECTION_REESTABLISHMENT_COMPLETE",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_UL_HANDOVER_PREPARATION_TRANSFER",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_UL_INFORMATION_TRANSFER",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_COUNTER_CHECK_RESPONSE",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_UE_INFORMATION_RESPONSE_R9",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_PROXIMITY_INDICATION_R9",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_RECONFIGURATION_COMPLETE_R10",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_MBMS_COUNTING_RESPONSE_R10",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_INTER_FREQ_RSTD_MEASUREMENT_INDICATION",
      h, database, is_on);
  view_add_log(eg.rrcview, "ENB_RRC_UNKNOW_MESSAGE",
      h, database, is_on);

  /* deactivate those two by default, they are a bit heavy */
  on_off(database, "ENB_MAC_UE_UL_SDU_WITH_DATA", is_on, 0);
  on_off(database, "ENB_MAC_UE_UL_PDU_WITH_DATA", is_on, 0);

  for (i = 0; i < on_off_n; i++)
    on_off(database, on_off_name[i], is_on, on_off_action[i]);

  enb_data.socket = -1;
  enb_data.is_on = is_on;
  enb_data.nevents = number_of_events;
  if (pthread_mutex_init(&enb_data.lock, NULL)) abort();
  setup_event_selector(g, database, is_on, is_on_changed, &enb_data);

  OBUF ebuf = { osize: 0, omaxsize: 0, obuf: NULL };

restart:
  clear_remote_config();
  if (enb_data.socket != -1) close(enb_data.socket);
  enb_data.socket = connect_to(ip, port);

  /* send the first message - activate selected traces */
  is_on_changed(&enb_data);

  /* read messages */
  while (1) {
    event e;
    e = get_event(enb_data.socket, &ebuf, database);
    if (e.type == -1) goto restart;
    if (pthread_mutex_lock(&enb_data.lock)) abort();
    handle_event(h, e);
    if (pthread_mutex_unlock(&enb_data.lock)) abort();
  }

  return 0;
}
