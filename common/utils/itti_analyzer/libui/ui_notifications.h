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

#ifndef UI_NOTIFICATIONS_H_
#define UI_NOTIFICATIONS_H_

typedef void (*message_write_callback_t)  (const gpointer buffer, const gchar *signal_name);

int ui_disable_connect_button(void);

int ui_enable_connect_button(void);

int ui_messages_read(char *filename);

int ui_messages_open_file_chooser(void);

int ui_messages_save_file_chooser(gboolean filtered);

int ui_filters_open_file_chooser(void);

int ui_filters_save_file_chooser(void);

void ui_progressbar_window_destroy(void);

int ui_progress_bar_set_fraction(double fraction);

int ui_progress_bar_terminate(void);

#endif /* UI_NOTIFICATIONS_H_ */
