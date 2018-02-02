/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#ifndef UI_MAIN_SCREEN_H_
#define UI_MAIN_SCREEN_H_

#include <gtk/gtk.h>

#include "ui_signal_dissect_view.h"

typedef struct {
    GtkWidget *window;
    GtkWidget *ip_entry;
    char *ip_entry_init;
    GtkWidget *port_entry;
    char *port_entry_init;

    GtkWidget      *progressbar_window;
    GtkWidget      *progressbar;
    GtkWidget      *messages_list;
    ui_text_view_t *text_view;

    /* Buttons */
    GtkToolItem *filters_enabled;
    GtkToolItem *open_filters_file;
    GtkToolItem *refresh_filters_file;
    GtkToolItem *save_filters_file;

    GtkToolItem *open_replay_file;
    GtkToolItem *refresh_replay_file;
    GtkToolItem *stop_loading;
    GtkToolItem *save_replay_file;
    GtkToolItem *save_replay_file_filtered;

    GtkToolItem *auto_reconnect;
    GtkToolItem *connect;
    GtkToolItem *disconnect;

    /* Signal list buttons */
    /* Clear signals button */
    GtkWidget *signals_go_to_entry;
    GtkToolItem *signals_go_to_last_button;
    GtkToolItem *signals_go_to_first_button;
    gboolean display_message_header;
    gboolean display_brace;

    GtkTreeSelection *selection;
    gboolean follow_last;

    /* Nb of messages received */
    guint nb_message_received;

    GLogLevelFlags log_flags;
    char *dissect_file_name;
    char *filters_file_name;
    char *messages_file_name;

    GtkWidget *menu_filter_messages;
    GtkWidget *menu_filter_origin_tasks;
    GtkWidget *menu_filter_destination_tasks;
    GtkWidget *menu_filter_instances;

    int pipe_fd[2];
} ui_main_data_t;

extern ui_main_data_t ui_main_data;

void ui_gtk_parse_arg(int argc, char *argv[]);

void ui_set_title(const char *fmt, ...);

void ui_main_window_destroy (void);

int ui_gtk_initialize(int argc, char *argv[]);

void ui_gtk_flush_events(void);

#endif /* UI_MAIN_SCREEN_H_ */
