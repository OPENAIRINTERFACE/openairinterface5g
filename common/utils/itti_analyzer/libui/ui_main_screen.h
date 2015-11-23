/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
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
