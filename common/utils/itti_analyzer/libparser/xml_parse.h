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

#include "rc.h"
#include "types.h"

#ifndef XML_PARSE_H_
#define XML_PARSE_H_

extern void        *xml_raw_data;
extern uint32_t     xml_raw_data_size;

extern types_t     *root;

int xml_parse_file(const char *filename);

int xml_parse_buffer(char *xml_buffer, const int size);

int dissect_signal(buffer_t *buffer, ui_set_signal_text_cb_t ui_set_signal_text_cb,
                   gpointer user_data);

int dissect_signal_header(buffer_t *buffer, ui_set_signal_text_cb_t ui_set_signal_text_cb,
                          gpointer cb_user_data);

#endif  /* XML_PARSE_H_ */
