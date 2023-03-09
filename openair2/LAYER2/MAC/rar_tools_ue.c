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

/*! \file rar_tools.c
 * \brief random access tools
 * \author Raymond Knopp and navid nikaein
 * \date 2011 - 2014
 * \version 1.0
 * @ingroup _mac

 */

#include "mac.h"
#include "mac_proto.h"
#include "mac_extern.h"
#include "SIMULATION/TOOLS/sim.h"
#include "common/utils/LOG/log.h"
#include "UTIL/OPT/opt.h"
#include "common/ran_context.h"

#define DEBUG_RAR

//------------------------------------------------------------------------------
uint16_t ue_process_rar(const module_id_t module_idP, const int CC_id, const frame_t frameP, const rnti_t ra_rnti, uint8_t *const dlsch_buffer, rnti_t *const t_crnti, const uint8_t preamble_index,
                        uint8_t *selected_rar_buffer   // output argument for storing the selected RAR header and RAR payload
                       )
//------------------------------------------------------------------------------
{
  uint16_t ret = 0;   // return value
  RA_HEADER_RAPID *rarh = (RA_HEADER_RAPID *) dlsch_buffer;
  //  RAR_PDU *rar = (RAR_PDU *)(dlsch_buffer+1);
  uint8_t *rar = (uint8_t *) (dlsch_buffer + 1);
  // get the last RAR payload for working with CMW500
  uint8_t n_rarpy = 0;  // number of RAR payloads
  uint8_t n_rarh = 0;   // number of MAC RAR subheaders
  uint8_t best_rx_rapid = -1; // the closest RAPID receive from all RARs

  while (1) {
    n_rarh++;

    if (rarh->T == 1) {
      n_rarpy++;
      LOG_D(MAC, "RAPID %d\n", rarh->RAPID);
    }

    if (rarh->RAPID == preamble_index) {
      LOG_D(PHY, "Found RAR with the intended RAPID %d\n",
            rarh->RAPID);
      rar = (uint8_t *) (dlsch_buffer + n_rarh + (n_rarpy - 1) * 6);
      UE_mac_inst[module_idP].UE_mode[0] = RA_RESPONSE;
      break;
    }

    if (abs((int) rarh->RAPID - (int) preamble_index) <
        abs((int) best_rx_rapid - (int) preamble_index)) {
      best_rx_rapid = rarh->RAPID;
      rar = (uint8_t *) (dlsch_buffer + n_rarh + (n_rarpy - 1) * 6);
    }

    if (rarh->E == 0) {
      LOG_I(MAC,
            "No RAR found with the intended RAPID. The closest RAPID in all RARs is %d\n",
            best_rx_rapid);
      UE_mac_inst[module_idP].UE_mode[0] = PRACH;
      break;
    } else {
      rarh++;
    }
  };

  LOG_D(MAC, "number of RAR subheader %d; number of RAR pyloads %d\n",
        n_rarh, n_rarpy);

  if (CC_id > 0) {
    LOG_W(MAC, "Should not have received RAR on secondary CCs! \n");
    return (0xffff);
  }

  LOG_A(MAC,
        "[UE %d][RAPROC] Frame %d Received RAR (%02x|%02x.%02x.%02x.%02x.%02x.%02x) for preamble %d/%d\n",
        module_idP, frameP, *(uint8_t *) rarh, rar[0], rar[1], rar[2],
        rar[3], rar[4], rar[5], rarh->RAPID, preamble_index);
#ifdef DEBUG_RAR
  LOG_D(MAC, "[UE %d][RAPROC] rarh->E %d\n", module_idP, rarh->E);
  LOG_D(MAC, "[UE %d][RAPROC] rarh->T %d\n", module_idP, rarh->T);
  LOG_D(MAC, "[UE %d][RAPROC] rarh->RAPID %d\n", module_idP,
        rarh->RAPID);
  //  LOG_I(MAC,"[UE %d][RAPROC] rar->R %d\n",module_idP,rar->R);
  LOG_D(MAC, "[UE %d][RAPROC] rar->Timing_Advance_Command %d\n",
        module_idP, (((uint16_t) (rar[0] & 0x7f)) << 4) + (rar[1] >> 4));
  //  LOG_I(MAC,"[UE %d][RAPROC] rar->hopping_flag %d\n",module_idP,rar->hopping_flag);
  //  LOG_I(MAC,"[UE %d][RAPROC] rar->rb_alloc %d\n",module_idP,rar->rb_alloc);
  //  LOG_I(MAC,"[UE %d][RAPROC] rar->mcs %d\n",module_idP,rar->mcs);
  //  LOG_I(MAC,"[UE %d][RAPROC] rar->TPC %d\n",module_idP,rar->TPC);
  //  LOG_I(MAC,"[UE %d][RAPROC] rar->UL_delay %d\n",module_idP,rar->UL_delay);
  //  LOG_I(MAC,"[UE %d][RAPROC] rar->cqi_req %d\n",module_idP,rar->cqi_req);
  LOG_D(MAC, "[UE %d][RAPROC] rar->t_crnti %x\n", module_idP,
        (uint16_t) rar[5] + (rar[4] << 8));
#endif
  trace_pdu(DIRECTION_DOWNLINK, (uint8_t *) dlsch_buffer, n_rarh + n_rarpy * 6,
            module_idP, WS_RA_RNTI, ra_rnti, UE_mac_inst[module_idP].rxFrame,
            UE_mac_inst[module_idP].rxSubframe, 0, 0);

  if (preamble_index == rarh->RAPID) {
    *t_crnti = (uint16_t) rar[5] + (rar[4] << 8); //rar->t_crnti;
    UE_mac_inst[module_idP].crnti = *t_crnti; //rar->t_crnti;
    //return(rar->Timing_Advance_Command);
    ret = ((((uint16_t) (rar[0] & 0x7f)) << 4) + (rar[1] >> 4));
  } else {
    UE_mac_inst[module_idP].crnti = 0;
    ret = (0xffff);
  }

  // move the selected RAR to the front of the RA_PDSCH buffer
  memmove(selected_rar_buffer + 0, (uint8_t *) rarh, 1);
  memmove(selected_rar_buffer + 1, (uint8_t *) rar, 6);
  return ret;
}
