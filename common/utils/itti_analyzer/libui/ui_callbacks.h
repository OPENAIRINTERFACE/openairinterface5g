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
