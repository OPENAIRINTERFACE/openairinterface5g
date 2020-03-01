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

/*! \file nr_ rrc_common.c
 * \brief rrc common procedures for gNB
 * \author Navid Nikaein and Raymond Knopp, WEI-TAI CHEN
 * \date 2011 - 2014, 2018
 * \version 1.0
 * \company Eurecom, NTUST
 * \email:  navid.nikaein@eurecom.fr and raymond.knopp@eurecom.fr, kroempa@gmail.com
 */

#include "nr_rrc_extern.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_extern.h"
#include "COMMON/openair_defs.h"
#include "COMMON/platform_types.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "LAYER2/RLC/rlc.h"
#include "COMMON/mac_rrc_primitives.h"
#include "common/utils/LOG/log.h"
#include "asn1_msg.h"
#include "pdcp.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "rrc_eNB_UE_context.h"
#include "common/ran_context.h"

#define DEBUG_NR_RRC 1

extern RAN_CONTEXT_t RC;
extern UE_MAC_INST *UE_mac_inst;
extern mui_t rrc_gNB_mui;

//-----------------------------------------------------------------------------
int rrc_init_nr_global_param(void){

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
void rrc_config_nr_buffer(NR_SRB_INFO* Srb_info,
                          uint8_t Lchan_type,
                          uint8_t Role
                         ){

  Srb_info->Rx_buffer.payload_size = 0;
  Srb_info->Tx_buffer.payload_size = 0;
}

//-----------------------------------------------------------------------------
