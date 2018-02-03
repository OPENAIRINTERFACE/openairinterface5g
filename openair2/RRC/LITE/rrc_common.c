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

/*! \file rrc_common.c
 * \brief rrc common procedures for eNB and UE
 * \author Navid Nikaein and Raymond Knopp
 * \date 2011 - 2014
 * \version 1.0
 * \company Eurecom
 * \email:  navid.nikaein@eurecom.fr and raymond.knopp@eurecom.fr
 */

#include "defs.h"
#include "extern.h"
#include "LAYER2/MAC/extern.h"
#include "COMMON/openair_defs.h"
#include "COMMON/platform_types.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "LAYER2/RLC/rlc.h"
#include "COMMON/mac_rrc_primitives.h"
#include "UTIL/LOG/log.h"
#include "asn1_msg.h"
#include "pdcp.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "rrc_eNB_UE_context.h"
#include "common/ran_context.h"

#ifdef LOCALIZATION
#include <sys/time.h>
#endif

#define DEBUG_RRC 1
extern RAN_CONTEXT_t RC;
extern UE_MAC_INST *UE_mac_inst;

extern mui_t rrc_eNB_mui;

//-----------------------------------------------------------------------------
int
rrc_init_global_param(
  void
)
//-----------------------------------------------------------------------------
{

  //  Rrc_xface = (RRC_XFACE*)malloc16(sizeof(RRC_XFACE));

  //  Rrc_xface->openair_rrc_top_init = openair_rrc_top_init;
  //  Rrc_xface->openair_rrc_eNB_init = openair_rrc_eNB_init;
  //  Rrc_xface->openair_rrc_UE_init  = openair_rrc_ue_init;
  //  Rrc_xface->mac_rrc_data_ind     = mac_rrc_data_ind;
  //Rrc_xface->mac_rrc_data_req     = mac_rrc_data_req;
  // Rrc_xface->rrc_data_indP        = (void *)rlcrrc_data_ind;
  //  Rrc_xface->rrc_rx_tx            = rrc_rx_tx;
  //  Rrc_xface->mac_rrc_meas_ind     = mac_rrc_meas_ind;
  //  Rrc_xface->get_rrc_status       = get_rrc_status;

  //Rrc_xface->rrc_get_status = ...

  //  Mac_rlc_xface->mac_out_of_sync_ind=mac_out_of_sync_ind;

#ifndef NO_RRM
  //  Rrc_xface->fn_rrc=fn_rrc;
#endif
  //  LOG_D(RRC, "[RRC]INIT_GLOBAL_PARAM: Mac_rlc_xface %p, rrc_rlc_register %p,rlcrrc_data_ind%p\n",Mac_rlc_xface,Mac_rlc_xface->rrc_rlc_register_rrc,rlcrrc_data_ind);
  /*
   if((Mac_rlc_xface==NULL) || (Mac_rlc_xface->rrc_rlc_register_rrc==NULL) ||
   (rlcrrc_data_ind==NULL)) {
   LOG_E(RRC,"Data structured is not initialized \n");
   return -1;
   }
   */
  rrc_rlc_register_rrc (rrc_data_ind, NULL); //register with rlc

  DCCH_LCHAN_DESC.transport_block_size = 4;
  DCCH_LCHAN_DESC.max_transport_blocks = 16;
  DCCH_LCHAN_DESC.Delay_class = 1;
  DTCH_DL_LCHAN_DESC.transport_block_size = 52;
  DTCH_DL_LCHAN_DESC.max_transport_blocks = 20;
  DTCH_DL_LCHAN_DESC.Delay_class = 1;
  DTCH_UL_LCHAN_DESC.transport_block_size = 52;
  DTCH_UL_LCHAN_DESC.max_transport_blocks = 20;
  DTCH_UL_LCHAN_DESC.Delay_class = 1;

  Rlc_info_um.rlc_mode = RLC_MODE_UM;
  Rlc_info_um.rlc.rlc_um_info.timer_reordering = 5;
  Rlc_info_um.rlc.rlc_um_info.sn_field_length = 10;
  Rlc_info_um.rlc.rlc_um_info.is_mXch = 0;
  //Rlc_info_um.rlc.rlc_um_info.sdu_discard_mode=16;

  Rlc_info_am_config.rlc_mode = RLC_MODE_AM;
  Rlc_info_am_config.rlc.rlc_am_info.max_retx_threshold = 50;
  Rlc_info_am_config.rlc.rlc_am_info.poll_pdu = 8;
  Rlc_info_am_config.rlc.rlc_am_info.poll_byte = 1000;
  Rlc_info_am_config.rlc.rlc_am_info.t_poll_retransmit = 15;
  Rlc_info_am_config.rlc.rlc_am_info.t_reordering = 50;
  Rlc_info_am_config.rlc.rlc_am_info.t_status_prohibit = 10;

  return 0;
}

//-----------------------------------------------------------------------------
void
rrc_config_buffer(
  SRB_INFO* Srb_info,
  uint8_t Lchan_type,
  uint8_t Role
)
//-----------------------------------------------------------------------------
{

  Srb_info->Rx_buffer.payload_size = 0;
  Srb_info->Tx_buffer.payload_size = 0;
}


//-----------------------------------------------------------------------------
long
binary_search_int(
  int elements[],
  long numElem,
  int value
)
//-----------------------------------------------------------------------------
{
  long first, last, middle, search = -1;
  first = 0;
  last = numElem-1;
  middle = (first+last)/2;

  if(value < elements[0]) {
    return first;
  }

  if(value > elements[last]) {
    return last;
  }

  while (first <= last) {
    if (elements[middle] < value) {
      first = middle+1;
    } else if (elements[middle] == value) {
      search = middle+1;
      break;
    } else {
      last = middle -1;
    }

    middle = (first+last)/2;
  }

  if (first > last) {
    LOG_E(RRC,"Error in binary search!");
  }

  return search;
}

/* This is a binary search routine which operates on an array of floating
   point numbers and returns the index of the range the value lies in
   Used for RSRP and RSRQ measurement mapping. Can potentially be used for other things
*/
//-----------------------------------------------------------------------------
long
binary_search_float(
  float elements[],
  long numElem,
  float value
)
//-----------------------------------------------------------------------------
{
  long first, last, middle;
  first = 0;
  last = numElem-1;
  middle = (first+last)/2;

  if(value <= elements[0]) {
    return first;
  }

  if(value >= elements[last]) {
    return last;
  }

  while (last - first > 1) {
    if (elements[middle] > value) {
      last = middle;
    } else {
      first = middle;
    }

    middle = (first+last)/2;
  }

  if (first < 0 || first >= numElem) {
    LOG_E(RRC,"\n Error in binary search float!");
  }

  return first;
}

typedef enum { BAND_TYPE_FDD, BAND_TYPE_TDD } band_type;

typedef struct {
  band_type t;
  int band;
  unsigned long ul_minfreq, ul_maxfreq;
  unsigned long dl_minfreq, dl_maxfreq;
} band_freq;

static band_freq bands[] = {
  { BAND_TYPE_FDD, 1, 1920000000UL, 1980000000UL, 2110000000UL, 2170000000UL },
  { BAND_TYPE_FDD, 2, 1850000000UL, 1910000000UL, 1930000000UL, 1990000000UL },
  { BAND_TYPE_FDD, 3, 1710000000UL, 1785000000UL, 1805000000UL, 1880000000UL },
  { BAND_TYPE_FDD, 4, 1710000000UL, 1755000000UL, 2110000000UL, 2155000000UL },
  { BAND_TYPE_FDD, 5, 824000000UL, 849000000UL, 869000000UL, 894000000UL },
    /* to remove? */{ BAND_TYPE_FDD, 6, 830000000UL, 840000000UL, 875000000UL, 885000000UL },
  { BAND_TYPE_FDD, 7, 2500000000UL, 2570000000UL, 2620000000UL, 2690000000UL },
  { BAND_TYPE_FDD, 8, 880000000UL, 915000000UL, 925000000UL, 960000000UL },
  { BAND_TYPE_FDD, 9, 1749900000UL, 1784900000UL, 1844900000UL, 1879900000UL },
  { BAND_TYPE_FDD, 10, 1710000000UL, 1770000000UL, 2110000000UL, 2170000000UL },
  { BAND_TYPE_FDD, 11, 1427900000UL, 1447900000UL, 1475900000UL, 1495900000UL },
  { BAND_TYPE_FDD, 12, 699000000UL, 716000000UL, 729000000UL, 746000000UL },
  { BAND_TYPE_FDD, 13, 777000000UL, 787000000UL, 746000000UL, 756000000UL },
  { BAND_TYPE_FDD, 14, 788000000UL, 798000000UL, 758000000UL, 768000000UL },
  { BAND_TYPE_FDD, 17, 704000000UL, 716000000UL, 734000000UL, 746000000UL },
  { BAND_TYPE_FDD, 18, 815000000UL, 830000000UL, 860000000UL, 875000000UL },
  { BAND_TYPE_FDD, 19, 830000000UL, 845000000UL, 875000000UL, 890000000UL },
  { BAND_TYPE_FDD, 20, 832000000UL, 862000000UL, 791000000UL, 821000000UL },
  { BAND_TYPE_FDD, 21, 1447900000UL, 1462900000UL, 1495900000UL, 1510900000UL },
  { BAND_TYPE_FDD, 22, 3410000000UL, 3490000000UL, 3510000000UL, 3590000000UL },
  { BAND_TYPE_FDD, 23, 2000000000UL, 2020000000UL, 2180000000UL, 2200000000UL },
  { BAND_TYPE_FDD, 24, 1626500000UL, 1660500000UL, 1525000000UL, 1559000000UL },
  { BAND_TYPE_FDD, 25, 1850000000UL, 1915000000UL, 1930000000UL, 1995000000UL },

  { BAND_TYPE_TDD, 33, 1900000000UL, 1920000000UL, 1900000000UL, 1920000000UL },
  { BAND_TYPE_TDD, 34, 2010000000UL, 2025000000UL, 2010000000UL, 2025000000UL },
  { BAND_TYPE_TDD, 35, 1850000000UL, 1910000000UL, 1850000000UL, 1910000000UL },
  { BAND_TYPE_TDD, 36, 1930000000UL, 1990000000UL, 1930000000UL, 1990000000UL },
  { BAND_TYPE_TDD, 37, 1910000000UL, 1930000000UL, 1910000000UL, 1930000000UL },
  { BAND_TYPE_TDD, 38, 2570000000UL, 2620000000UL, 2570000000UL, 2620000000UL },
  { BAND_TYPE_TDD, 39, 1880000000UL, 1920000000UL, 1880000000UL, 1920000000UL },
  { BAND_TYPE_TDD, 40, 2300000000UL, 2400000000UL, 2300000000UL, 2400000000UL },
  { BAND_TYPE_TDD, 41, 2496000000UL, 2690000000UL, 2496000000UL, 2690000000UL },
  { BAND_TYPE_TDD, 42, 3400000000UL, 3600000000UL, 3400000000UL, 3600000000UL },
  { BAND_TYPE_TDD, 43, 3600000000UL, 3800000000UL, 3600000000UL, 3800000000UL },
};

typedef struct {
  int band;
  unsigned long dl_flow;
  int dl_off;
  int dl_offmin, dl_offmax;
  unsigned long ul_flow;
  int ul_off;
  int ul_offmin, ul_offmax;
} earfcn;

static earfcn earfcn_table[] = {
  { 1, 2110000000UL, 0, 0, 599, 1920000000UL, 18000, 18000, 18599 },
  { 2, 1930000000UL, 600, 600, 1199, 1850000000UL, 18600, 18600, 19199 },
  { 3, 1805000000UL, 1200, 1200, 1949, 1710000000UL, 19200, 19200, 19949 },
  { 4, 2110000000UL, 1950, 1950, 2399, 1710000000UL, 19950, 19950, 20399 },
  { 5, 869000000UL, 2400, 2400, 2649, 824000000UL, 20400, 20400, 20649 },
  { 6, 875000000UL, 2650, 2650, 2749, 830000000UL, 20650, 20650, 20749 },
  { 7, 2620000000UL, 2750, 2750, 3449, 2500000000UL, 20750, 20750, 21449 },
  { 8, 925000000UL, 3450, 3450, 3799, 880000000UL, 21450, 21450, 21799 },
  { 9, 1844900000UL, 3800, 3800, 4149, 1749900000UL, 21800, 21800, 22149 },
  { 10, 2110000000UL, 4150, 4150, 4749, 1710000000UL, 22150, 22150, 22749 },
  { 11, 1475900000UL, 4750, 4750, 4949, 1427900000UL, 22750, 22750, 22949 },
  { 12, 729000000UL, 5010, 5010, 5179, 699000000UL, 23010, 23010, 23179 },
  { 13, 746000000UL, 5180, 5180, 5279, 777000000UL, 23180, 23180, 23279 },
  { 14, 758000000UL, 5280, 5280, 5379, 788000000UL, 23280, 23280, 23379 },
  { 17, 734000000UL, 5730, 5730, 5849, 704000000UL, 23730, 23730, 23849 },
  { 18, 860000000UL, 5850, 5850, 5999, 815000000UL, 23850, 23850, 23999 },
  { 19, 875000000UL, 6000, 6000, 6149, 830000000UL, 24000, 24000, 24149 },
  { 20, 791000000UL, 6150, 6150, 6449, 832000000UL, 24150, 24150, 24449 },
  { 21, 1495900000UL, 6450, 6450, 6599, 1447900000UL, 24450, 24450, 24599 },
  { 22, 3510000000UL, 6600, 6600, 7399, 3410000000UL, 24600, 24600, 25399 },
  { 23, 2180000000UL, 7500, 7500, 7699, 2000000000UL, 25500, 25500, 25699 },
  { 24, 1525000000UL, 7700, 7700, 8039, 1626500000UL, 25700, 25700, 26039 },
  { 25, 1930000000UL, 8040, 8040, 8689, 1850000000UL, 26040, 26040, 26689 },
  { 33, 1900000000UL, 36000, 36000, 36199, 1900000000UL, 36000, 36000, 36199 },
  { 34, 2010000000UL, 36200, 36200, 36349, 2010000000UL, 36200, 36200, 36349 },
  { 35, 1850000000UL, 36350, 36350, 36949, 1850000000UL, 36350, 36350, 36949 },
  { 36, 1930000000UL, 36950, 36950, 37549, 1930000000UL, 36950, 36950, 37549 },
  { 37, 1910000000UL, 37550, 37550, 37749, 1910000000UL, 37550, 37550, 37749 },
  { 38, 2570000000UL, 37750, 37750, 38249, 2570000000UL, 37750, 37750, 38249 },
  { 39, 1880000000UL, 38250, 38250, 38649, 1880000000UL, 38250, 38250, 38649 },
  { 40, 2300000000UL, 38650, 38650, 39649, 2300000000UL, 38650, 38650, 39649 },
  { 41, 2496000000UL, 39650, 39650, 41589, 2496000000UL, 39650, 39650, 41589 },
  { 42, 3400000000UL, 41590, 41590, 43589, 3400000000UL, 41590, 41590, 43589 },
  { 43, 3600000000UL, 43590, 43590, 45589, 3600000000UL, 43590, 43590, 45589 },
};
  
int freq_to_arfcn10(int band, unsigned long freq)
{ 
  int N = sizeof(earfcn_table) / sizeof(earfcn_table[0]); 
  int i;
  
  for (i = 0; i < N; i++) if (bands[i].band == band) break; 
  if (i == N) return -1;
  
  if (!(bands[i].dl_minfreq < freq && freq < bands[i].dl_maxfreq))
    return -1;
  
  return (freq - earfcn_table[i].dl_flow) / 100000UL + earfcn_table[i].dl_off;
}
