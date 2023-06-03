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
  RRC_config_tools.c
  -------------------
  AUTHOR  : Francois TABURET
  COMPANY : NOKIA BellLabs France
  EMAIL   : francois.taburet@nokia-bell-labs.com

*/

#include <string.h>
#include <inttypes.h>

#include "common/utils/LOG/log.h"
#include "assertions.h"
#include "intertask_interface.h"
#include "s1ap_eNB.h"
#include "sctp_eNB_task.h"
#include "LTE_SystemInformationBlockType2.h"
#include "common/config/config_userapi.h"
#include "RRC_config_tools.h"
#include "LTE_DL-GapConfig-NB-r13.h"
#include "LTE_NPRACH-Parameters-NB-r13.h"

static const eutra_band_t eutra_bands[] = {
  { 1, 1920    * MHz, 1980    * MHz, 2110    * MHz, 2170    * MHz, FDD},
  { 2, 1850    * MHz, 1910    * MHz, 1930    * MHz, 1990    * MHz, FDD},
  { 3, 1710    * MHz, 1785    * MHz, 1805    * MHz, 1880    * MHz, FDD},
  { 4, 1710    * MHz, 1755    * MHz, 2110    * MHz, 2155    * MHz, FDD},
  { 5,  824    * MHz,  849    * MHz,  869    * MHz,  894    * MHz, FDD},
  { 6,  830    * MHz,  840    * MHz,  875    * MHz,  885    * MHz, FDD},
  { 7, 2500    * MHz, 2570    * MHz, 2620    * MHz, 2690    * MHz, FDD},
  { 8,  880    * MHz,  915    * MHz,  925    * MHz,  960    * MHz, FDD},
  { 9, 1749900 * KHz, 1784900 * KHz, 1844900 * KHz, 1879900 * KHz, FDD},
  {10, 1710    * MHz, 1770    * MHz, 2110    * MHz, 2170    * MHz, FDD},
  {11, 1427900 * KHz, 1452900 * KHz, 1475900 * KHz, 1500900 * KHz, FDD},
  {12,  698    * MHz,  716    * MHz,  728    * MHz,  746    * MHz, FDD},
  {13,  777    * MHz,  787    * MHz,  746    * MHz,  756    * MHz, FDD},
  {14,  788    * MHz,  798    * MHz,  758    * MHz,  768    * MHz, FDD},

  {17,  704    * MHz,  716    * MHz,  734    * MHz,  746    * MHz, FDD},
  {20,  832    * MHz,  862    * MHz,  791    * MHz,  821    * MHz, FDD},
  {33, 1900    * MHz, 1920    * MHz, 1900    * MHz, 1920    * MHz, TDD},
  {33, 1900    * MHz, 1920    * MHz, 1900    * MHz, 1920    * MHz, TDD},
  {34, 2010    * MHz, 2025    * MHz, 2010    * MHz, 2025    * MHz, TDD},
  {35, 1850    * MHz, 1910    * MHz, 1850    * MHz, 1910    * MHz, TDD},
  {36, 1930    * MHz, 1990    * MHz, 1930    * MHz, 1990    * MHz, TDD},
  {37, 1910    * MHz, 1930    * MHz, 1910    * MHz, 1930    * MHz, TDD},
  {38, 2570    * MHz, 2620    * MHz, 2570    * MHz, 2630    * MHz, TDD},
  {39, 1880    * MHz, 1920    * MHz, 1880    * MHz, 1920    * MHz, TDD},
  {40, 2300    * MHz, 2400    * MHz, 2300    * MHz, 2400    * MHz, TDD},
  {41, 2496    * MHz, 2690    * MHz, 2496    * MHz, 2690    * MHz, TDD},
  {42, 3400    * MHz, 3600    * MHz, 3400    * MHz, 3600    * MHz, TDD},
  {43, 3600    * MHz, 3800    * MHz, 3600    * MHz, 3800    * MHz, TDD},
  {44, 703    * MHz, 803    * MHz, 703    * MHz, 803    * MHz, TDD},
  // ...
  {65,  1920    * MHz,  2010    * MHz,  2110    * MHz,  2200    * MHz, FDD},
  {66,  1710    * MHz,  1780    * MHz,  2110    * MHz,  2200    * MHz, FDD},
  {71,  663    * MHz,  698    * MHz,  617    * MHz,  652    * MHz, FDD},
  {72,  451    * MHz,  456    * MHz,  461    * MHz,  466    * MHz, FDD},
  {73,  450    * MHz,  455    * MHz,  460    * MHz,  465    * MHz, FDD},
};


int config_check_band_frequencies(    int ind,
                                      int16_t band,
                                      uint32_t downlink_frequency,
                                      int32_t uplink_frequency_offset,
                                      uint32_t  frame_type) {
  int errors = 0;

  if (band > 0) {
    int band_index;

    for (band_index = 0; band_index < sizeof (eutra_bands) / sizeof (eutra_bands[0]); band_index++) {
      if (band == eutra_bands[band_index].band) {
        uint32_t uplink_frequency = downlink_frequency + uplink_frequency_offset;
        AssertError (eutra_bands[band_index].dl_min < downlink_frequency, errors ++,
                     "enb %d downlink frequency %u too low (%u) for band %d!",
                     ind, downlink_frequency, eutra_bands[band_index].dl_min, band);
        AssertError (downlink_frequency < eutra_bands[band_index].dl_max, errors ++,
                     "enb %d downlink frequency %u too high (%u) for band %d!",
                     ind, downlink_frequency, eutra_bands[band_index].dl_max, band);
        AssertError (eutra_bands[band_index].ul_min < uplink_frequency, errors ++,
                     "enb %d uplink frequency %u too low (%u) for band %d!",
                     ind, uplink_frequency, eutra_bands[band_index].ul_min, band);
        AssertError (uplink_frequency < eutra_bands[band_index].ul_max, errors ++,
                     "enb %d uplink frequency %u too high (%u) for band %d!",
                     ind, uplink_frequency, eutra_bands[band_index].ul_max, band);
        AssertError (eutra_bands[band_index].frame_type == frame_type, errors ++,
                     "enb %d invalid frame type (%u/%u) for band %d!",
                     ind, eutra_bands[band_index].frame_type, frame_type, band);
      }
    }
  }

  return errors;
}

