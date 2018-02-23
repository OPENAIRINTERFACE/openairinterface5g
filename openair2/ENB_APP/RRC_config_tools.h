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

/*
  RRC_config_tools.h
  -------------------
  AUTHOR  : Francois TABURET
  COMPANY : NOKIA BellLabs France
  EMAIL   : francois.taburet@nokia-bell-labs.com
*/
#ifndef RRC_CONFIG_TOOLS_H_
#define RRC_CONFIG_TOOLS_H_

#define KHz (1000UL)
#define MHz (1000 * KHz)

typedef struct eutra_band_s {
  int16_t             band;
  uint32_t            ul_min;
  uint32_t            ul_max;
  uint32_t            dl_min;
  uint32_t            dl_max;
  lte_frame_type_t    frame_type;
} eutra_band_t;


extern int config_check_band_frequencies(int ind, int16_t band, uint32_t downlink_frequency, 
                                         int32_t uplink_frequency_offset, uint32_t  frame_type);

extern int config_check_assign_DLGap_NB(paramdef_t *param);
extern int config_check_assign_rach_NB(paramdef_t *param);
#endif
