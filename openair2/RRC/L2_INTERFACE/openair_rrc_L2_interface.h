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

/*! \file l2_interface.c
* \brief layer 2 interface
* \author Navid Nikaein and Raymond Knopp
* \date 2011
* \version 1.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr,raymond.knopp@eurecom.fr
*/
#ifndef __OPENAIR_RRC_L2_INTERFACE_H__
#define __OPENAIR_RRC_L2_INTERFACE_H__

#include "COMMON/mac_rrc_primitives.h"
#include "COMMON/platform_types.h"

int8_t
mac_rrc_data_req(
  const module_id_t     module_idP,
  const int             CC_idP,
  const frame_t         frameP,
  const rb_id_t         srb_idP,
  const uint8_t         nb_tbP,
  uint8_t* const        buffer_pP,
  const uint8_t         mbsfn_sync_areaP
);

int8_t
mac_rrc_data_ind(
  const module_id_t     module_idP,
  const int             CC_idP,
  const frame_t         frameP,
  const sub_frame_t     sub_frameP,
  const rnti_t          rntiP,
  const rb_id_t         srb_idP,
  const uint8_t        *sduP,
  const sdu_size_t      sdu_lenP,
  const uint8_t         mbsfn_sync_area
);

int8_t
mac_rrc_data_req_ue(
  const module_id_t     module_idP,
  const int             CC_idP,
  const frame_t         frameP,
  const rb_id_t         srb_idP,
  const uint8_t         nb_tbP,
  uint8_t* const        buffer_pP,
  const mac_enb_index_t eNB_indexP,
  const uint8_t         mbsfn_sync_areaP
);

int8_t
mac_rrc_data_ind_ue(
  const module_id_t     module_idP,
  const int             CC_idP,
  const frame_t         frameP,
  const sub_frame_t     sub_frameP,
  const rnti_t          rntiP,
  const rb_id_t         srb_idP,
  const uint8_t        *sduP,
  const sdu_size_t      sdu_lenP,
  const mac_enb_index_t eNB_indexP,
  const uint8_t         mbsfn_sync_area
);

void mac_lite_sync_ind(
  const module_id_t module_idP,
  const uint8_t statusP);

void mac_rrc_meas_ind(
  const module_id_t,
  MAC_MEAS_REQ_ENTRY*const );

void
rlcrrc_data_ind(
  const protocol_ctxt_t* const ctxt_pP,
  const rb_id_t                rb_idP,
  const sdu_size_t             sdu_sizeP,
  const uint8_t * const        buffer_pP
);

uint8_t
pdcp_rrc_data_req(
  const protocol_ctxt_t* const ctxt_pP,
  const rb_id_t                rb_idP,
  const mui_t                  muiP,
  const confirm_t              confirmP,
  const sdu_size_t             sdu_buffer_sizeP,
  uint8_t* const               sdu_buffer_pP,
  const pdcp_transmission_mode_t modeP
);

void
pdcp_rrc_data_ind(
  const protocol_ctxt_t* const ctxt_pP,
  const rb_id_t                srb_idP,
  const sdu_size_t             sdu_sizeP,
  uint8_t              * const buffer_pP
);

void mac_out_of_sync_ind(
  const module_id_t module_idP,
  const frame_t frameP,
  const uint16_t CH_index);

char openair_rrc_eNB_init(
  const module_id_t module_idP);

char openair_rrc_ue_init(
  const module_id_t module_idP,
  const unsigned char eNB_indexP);

int
mac_eNB_get_rrc_status(
  const module_id_t module_idP,
  const rnti_t      indexP
);
int
mac_UE_get_rrc_status(
  const module_id_t module_idP,
  const uint8_t     sig_indexP
);

char
openair_rrc_ue_init(
  const module_id_t   module_idP,
  const unsigned char eNB_indexP
);

void mac_in_sync_ind(
  const module_id_t module_idP,
  const frame_t frameP,
  const uint16_t eNB_indexP);

#endif
