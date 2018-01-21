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

  //#ifdef USER_MODE
  //  Rrc_xface = (RRC_XFACE*)malloc16(sizeof(RRC_XFACE));
  //#endif //USRE_MODE

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

