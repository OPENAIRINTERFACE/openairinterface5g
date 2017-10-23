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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "common_types.h"

#include "enum_string.h"

enum_to_string_t rat_to_string[NUMBER_OF_RAT_TYPE] = {
  { RAT_WLAN,     "WLAN" },
  { RAT_VIRTUAL,  "VIRUTAL" },
  { RAT_UTRAN,    "UTRAN" },
  { RAT_GERAN,    "GERAN" },
  { RAT_GAN,      "GAN" },
  { RAT_HSPA_EVOLUTION, "HSPA_EVOLUTION" },
  { RAT_EUTRAN,         "E-UTRAN" },
  { RAT_CDMA2000_1X,    "CDMA2000_1X" },
  { RAT_HRPD,           "HRPD" },
  { RAT_UMB,            "UMB" },
  { RAT_EHRPD,          "EHRPD" },
};

enum_to_string_t network_access_mode_to_string[NAM_MAX] = {
  { NAM_PACKET_AND_CIRCUIT, "PACKET AND CIRCUIT" },
  { NAM_RESERVED,           "RESERVED" },
  { NAM_ONLY_PACKET,        "ONLY PACKET" },
};

enum_to_string_t all_apn_conf_ind_to_string[ALL_APN_MAX] = {
  { ALL_APN_CONFIGURATIONS_INCLUDED, "ALL APN CONFIGURATIONS INCLUDED" },
  { MODIFIED_ADDED_APN_CONFIGURATIONS_INCLUDED, "MODIFIED ADDED APN CONFIGURATIONS INCLUDED" },
};

enum_to_string_t pdn_type_to_string[IP_MAX] = {
  { IPv4, "IPv4" },
  { IPv6, "IPv6" },
  { IPv4_AND_v6, "IPv4 and IPv6" },
  { IPv4_OR_v6,  "IPv4 or IPv6" },
};

static int
compare_values(const void *m1, const void *m2)
{
  enum_to_string_t *mi1 = (enum_to_string_t *) m1;
  enum_to_string_t *mi2 = (enum_to_string_t *) m2;
  return (mi1->enum_value - mi2->enum_value);
}

char *enum_to_string(int enum_val, enum_to_string_t *string_table, int nb_element)
{
  enum_to_string_t *res;
  enum_to_string_t temp;

  temp.enum_value = enum_val;
  res = bsearch(&temp, string_table, nb_element,
                sizeof(enum_to_string_t), compare_values);

  if (res == NULL) {
    return "UNKNOWN";
  }

  return res->enum_value_name;
}
