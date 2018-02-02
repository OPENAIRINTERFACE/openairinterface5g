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

#ifndef ENUM_STRING_H_
#define ENUM_STRING_H_

typedef struct {
  int   enum_value;
  char *enum_value_name;
} enum_to_string_t;

extern enum_to_string_t network_access_mode_to_string[NAM_MAX];
extern enum_to_string_t rat_to_string[NUMBER_OF_RAT_TYPE];
extern enum_to_string_t pdn_type_to_string[IP_MAX];

char *enum_to_string(int enum_val, enum_to_string_t *string_table, int nb_element);
#define ACCESS_MODE_TO_STRING(vAL)                          \
    enum_to_string((int)vAL, network_access_mode_to_string, \
    sizeof(network_access_mode_to_string) / sizeof(enum_to_string_t))
#define PDN_TYPE_TO_STRING(vAL)                             \
    enum_to_string((int)vAL, pdn_type_to_string,            \
    sizeof(pdn_type_to_string) / sizeof(enum_to_string_t))

#endif /* ENUM_STRING_H_ */
