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
* \email: navid.nikaein@eurecom.fr
*/

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>

#include "platform_types.h"
#include "openair_defs.h"

#ifdef CELLULAR
#include "rrc_L2_proto.h"
#else
#include "RRC/LITE/defs.h"
#endif //CELLULAR
#include "COMMON/mac_rrc_primitives.h"
#include "openair_rrc_L2_interface.h"

//------------------------------------------------------------------------------
int8_t
mac_rrc_data_req(
  const module_id_t module_idP,
  const int         CC_idP,
  const frame_t     frameP,
  const rb_id_t     srb_idP,
  const uint8_t     nb_tbP,
  uint8_t* const    buffer_pP,
  const eNB_flag_t  eNB_flagP,
  const uint8_t     eNB_indexP,
  const uint8_t     mbsfn_sync_areaP
)
//------------------------------------------------------------------------------
{
  return(mac_rrc_lite_data_req(
           module_idP,
           CC_idP,
           frameP,
           srb_idP,
           nb_tbP,
           buffer_pP,
           eNB_flagP,
           eNB_indexP,
           mbsfn_sync_areaP)
        );
}

//------------------------------------------------------------------------------
int8_t
mac_rrc_data_ind(
  const module_id_t     module_idP,
  const int             CC_idP,
  const frame_t         frameP,
  const sub_frame_t     subframeP,
  const rnti_t          rntiP,
  const rb_id_t         srb_idP,
  const uint8_t        *sduP,
  const sdu_size_t      sdu_lenP,
  const eNB_flag_t      eNB_flagP,
  const mac_enb_index_t eNB_indexP,
  const uint8_t         mbsfn_sync_area
)
//------------------------------------------------------------------------------
{
  return(mac_rrc_lite_data_ind(
           module_idP,
           CC_idP,
           frameP,subframeP,
           rntiP,
           srb_idP,
           sduP,
           sdu_lenP,
           eNB_flagP,
           eNB_indexP,
           mbsfn_sync_area)
        );
}

//------------------------------------------------------------------------------
void
rlcrrc_data_ind(
  const protocol_ctxt_t* const ctxt_pP,
  const rb_id_t                srb_idP,
  const sdu_size_t             sdu_lenP,
  const uint8_t  * const       buffer_pP
)
//------------------------------------------------------------------------------
{
  rrc_data_ind(ctxt_pP,
	       srb_idP,
	       sdu_lenP,
	       buffer_pP);
}

//------------------------------------------------------------------------------
uint8_t
pdcp_rrc_data_req(
  const protocol_ctxt_t* const ctxt_pP,
  const rb_id_t                rb_idP,
  const mui_t                  muiP,
  const confirm_t              confirmP,
  const sdu_size_t             sdu_buffer_sizeP,
  uint8_t* const               sdu_buffer_pP,
  const pdcp_transmission_mode_t modeP
)
//------------------------------------------------------------------------------
{
  return rrc_lite_data_req (
           ctxt_pP,
           rb_idP,
           muiP,
           confirmP,
           sdu_buffer_sizeP,
           sdu_buffer_pP,
           modeP);
}

//------------------------------------------------------------------------------
void
pdcp_rrc_data_ind(
  const protocol_ctxt_t* const ctxt_pP,
  const rb_id_t                srb_idP,
  const sdu_size_t             sdu_sizeP,
  uint8_t              * const buffer_pP
)
//------------------------------------------------------------------------------
{
  rrc_lite_data_ind(
    ctxt_pP,
    srb_idP,
    sdu_sizeP,
    buffer_pP);
}

//------------------------------------------------------------------------------
void
mac_rrc_meas_ind(
  const module_id_t module_idP,
  MAC_MEAS_REQ_ENTRY * const Meas_entry_pP
)
//------------------------------------------------------------------------------
{
  //  mac_rrc_meas_ind(module_idP,Meas_entry_pP);
}

//------------------------------------------------------------------------------
void
mac_sync_ind(
  const  module_id_t module_idP,
  const  uint8_t Status
)
//------------------------------------------------------------------------------
{
  mac_lite_sync_ind(module_idP,Status);
}

//------------------------------------------------------------------------------
void
mac_in_sync_ind(
  const module_id_t module_idP,
  const frame_t frameP,
  const uint16_t eNB_indexP
)
//------------------------------------------------------------------------------
{
  rrc_lite_in_sync_ind(module_idP,frameP,eNB_indexP);
}

//------------------------------------------------------------------------------
void
mac_out_of_sync_ind(
  const module_id_t module_idP,
  const frame_t frameP,
  const uint16_t eNB_indexP
)
//------------------------------------------------------------------------------
{
  rrc_lite_out_of_sync_ind(module_idP,frameP,eNB_indexP);
}

//------------------------------------------------------------------------------
int
mac_eNB_get_rrc_status(
  const module_id_t   module_idP,
  const rnti_t  rntiP
)
//------------------------------------------------------------------------------
{
  return mac_eNB_get_rrc_lite_status(module_idP, rntiP);
}
//------------------------------------------------------------------------------
int
mac_UE_get_rrc_status(
  const module_id_t module_idP,
  const uint8_t     sig_indexP
)
//------------------------------------------------------------------------------
{
  return mac_UE_get_rrc_lite_status(module_idP, sig_indexP);
}

//------------------------------------------------------------------------------
char
openair_rrc_ue_init(
  const module_id_t   module_idP,
  const unsigned char eNB_indexP
)
//------------------------------------------------------------------------------
{
  return openair_rrc_lite_ue_init(module_idP, eNB_indexP);
}

//------------------------------------------------------------------------------
char
openair_rrc_eNB_init(
  const module_id_t module_idP
)
//------------------------------------------------------------------------------
{
  return openair_rrc_lite_eNB_init(module_idP);
}
