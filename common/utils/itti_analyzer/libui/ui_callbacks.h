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


#ifndef UI_CALLBACKS_H_
#define UI_CALLBACKS_H_

#include <gtk/gtk.h>

gboolean ui_callback_on_open_messages(GtkWidget *widget,
                                      gpointer   data);

gboolean ui_callback_on_save_messages(GtkWidget *widget,
                                      gpointer   data);

gboolean ui_callback_on_filters_enabled(GtkToolButton *button,
                                        gpointer data);

gboolean ui_callback_on_open_filters(GtkWidget *widget,
                                     gpointer data);

gboolean ui_callback_on_save_filters(GtkWidget *widget,
                                     gpointer data);

gboolean ui_callback_on_enable_filters(GtkWidget *widget,
                                       gpointer data);

gboolean ui_callback_on_about(GtkWidget *widget,
                              gpointer   data);

gint ui_callback_check_string (const char *string,
                               const gint lenght,
                               const guint message_number);

gboolean ui_pipe_callback(gint source, gpointer user_data);

gboolean ui_callback_on_auto_reconnect(GtkWidget *widget,
                                       gpointer data);

void ui_callback_dialogbox_connect_destroy(void);

gboolean ui_callback_on_connect(GtkWidget *widget,
                                gpointer   data);

gboolean ui_callback_on_disconnect(GtkWidget *widget,
                                   gpointer   data);

gboolean ui_callback_on_tree_view_select(GtkWidget *widget,
                                         GdkEvent  *event,
                                         gpointer   data);

gboolean ui_callback_on_select_signal(GtkTreeSelection *selection,
                                      GtkTreeModel     *model,
                                      GtkTreePath      *path,
                                      gboolean          path_currently_selected,
                                      gpointer          userdata);

void ui_signal_add_to_list(gpointer data,
                           gpointer user_data);

gboolean ui_callback_on_menu_enable (GtkWidget *widget, gpointer data);

gboolean ui_callback_on_menu_color (GtkWidget *widget, gpointer data);

gboolean ui_callback_signal_go_to_first(GtkWidget *widget,
                                        gpointer   data);

gboolean ui_callback_signal_go_to(GtkWidget *widget,
                                  gpointer data);

gboolean ui_callback_signal_go_to_entry(GtkWidget *widget,
                                        gpointer   data);

gboolean ui_callback_signal_go_to_last(GtkWidget *widget,
                                       gpointer   data);

gboolean ui_callback_display_message_header(GtkWidget *widget,
                                            gpointer data);

gboolean ui_callback_display_brace(GtkWidget *widget,
                                   gpointer data);

gboolean ui_callback_signal_clear_list(GtkWidget *widget,
                                       gpointer   data);

gboolean ui_callback_on_menu_none(GtkWidget *widget,
                                  gpointer data);

gboolean ui_callback_on_menu_all(GtkWidget *widget,
                                 gpointer data);

gboolean ui_callback_on_menu_item_selected(GtkWidget *widget,
                                           gpointer data);

gboolean ui_callback_on_tree_column_header_click(GtkWidget *widget,
                                                 gpointer   data);
#endif /* UI_CALLBACKS_H_ */
