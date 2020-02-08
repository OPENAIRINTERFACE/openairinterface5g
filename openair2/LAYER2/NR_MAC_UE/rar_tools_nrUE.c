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

/*! \file rar_tools_nrUE.c
 * \brief RA tools for NR UE
 * \author Guido Casati
 * \date 2019
 * \version 1.0
 * @ingroup _mac

 */

/* Sim */
#include "SIMULATION/TOOLS/sim.h"

/* Utils */
#include "common/utils/LOG/log.h"
#include "OCG.h"
#include "OCG_extern.h"
#include "UTIL/OPT/opt.h"

/* Common */
#include "common/ran_context.h"

/* MAC */
#include "NR_MAC_UE/mac.h"
#include "NR_MAC_UE/mac_proto.h"
#include "NR_MAC_COMMON/nr_mac_extern.h"

#define DEBUG_RAR

uint16_t nr_ue_process_rar(const module_id_t mod_id,
                           const int CC_id,
                           const frame_t frameP,
                           const rnti_t ra_rnti,
                           uint8_t * const dlsch_buffer,
                           rnti_t * const t_crnti,
                           const uint8_t preamble_index,
                           uint8_t * selected_rar_buffer){

    NR_UE_MAC_INST_t *nrUE_mac_inst = get_mac_inst(mod_id);
    NR_RA_HEADER_RAPID *rarh = (NR_RA_HEADER_RAPID *) dlsch_buffer;
    uint16_t ret = 0;
    //  NR_RAR_PDU *rar = (RAR_PDU *)(dlsch_buffer+1);
    uint8_t *rar = (uint8_t *) (dlsch_buffer + 1);
    // get the last RAR payload for working with CMW500
    uint8_t n_rarpy = 0;        // number of RAR payloads
    uint8_t n_rarh = 0;         // number of MAC RAR subheaders
    uint8_t best_rx_rapid = -1;     // the closest RAPID receive from all RARs

    AssertFatal(CC_id == 0, "RAR reception on secondary CCs is not supported yet\n");

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
          nrUE_mac_inst->RA_RAPID_found = 1;
          break;
      }

      if (abs((int) rarh->RAPID - (int) preamble_index) <
          abs((int) best_rx_rapid - (int) preamble_index)) {
          best_rx_rapid = rarh->RAPID;
          rar = (uint8_t *) (dlsch_buffer + n_rarh + (n_rarpy - 1) * 6);
      }

      if (rarh->E == 0) {
          LOG_I(PHY, "No RAR found with the intended RAPID. The closest RAPID in all RARs is %d\n", best_rx_rapid);
          break;
      } else {
          rarh++;
      }
    };
    LOG_D(MAC, "number of RAR subheader %d; number of RAR pyloads %d\n",
        n_rarh, n_rarpy);

    LOG_I(MAC,
        "[UE %d][RAPROC] Frame %d Received RAR (%02x|%02x.%02x.%02x.%02x.%02x.%02x) for preamble %d/%d\n",
        mod_id, frameP, *(uint8_t *) rarh, rar[0], rar[1], rar[2],
        rar[3], rar[4], rar[5], rarh->RAPID, preamble_index);
#ifdef DEBUG_RAR
    LOG_D(MAC, "[UE %d][RAPROC] rarh->E %d\n", mod_id, rarh->E);
    LOG_D(MAC, "[UE %d][RAPROC] rarh->T %d\n", mod_id, rarh->T);
    LOG_D(MAC, "[UE %d][RAPROC] rarh->RAPID %d\n", mod_id,
        rarh->RAPID);

    //  LOG_I(MAC,"[UE %d][RAPROC] rar->R %d\n",mod_id,rar->R);
    LOG_D(MAC, "[UE %d][RAPROC] rar->Timing_Advance_Command %d\n",
        mod_id, (((uint16_t) (rar[0] & 0x7f)) << 4) + (rar[1] >> 4));
    //  LOG_I(MAC,"[UE %d][RAPROC] rar->hopping_flag %d\n",mod_id,rar->hopping_flag);
    //  LOG_I(MAC,"[UE %d][RAPROC] rar->rb_alloc %d\n",mod_id,rar->rb_alloc);
    //  LOG_I(MAC,"[UE %d][RAPROC] rar->mcs %d\n",mod_id,rar->mcs);
    //  LOG_I(MAC,"[UE %d][RAPROC] rar->TPC %d\n",mod_id,rar->TPC);
    //  LOG_I(MAC,"[UE %d][RAPROC] rar->UL_delay %d\n",mod_id,rar->UL_delay);
    //  LOG_I(MAC,"[UE %d][RAPROC] rar->cqi_req %d\n",mod_id,rar->cqi_req);
    LOG_D(MAC, "[UE %d][RAPROC] rar->t_crnti %x\n", mod_id,
        (uint16_t) rar[5] + (rar[4] << 8));
#endif

    if (opt_enabled) {
      LOG_D(OPT,
            "[UE %d][RAPROC] CC_id %d RAR Frame %d trace pdu for ra-RNTI %x\n",
            mod_id, CC_id, frameP, ra_rnti);
      /*trace_pdu(DIRECTION_DOWNLINK, (uint8_t *) dlsch_buffer, n_rarh + n_rarpy * 6,
              mod_id, WS_RA_RNTI, ra_rnti, nrUE_mac_inst->rxFrame,
              nrUE_mac_inst->rxSubframe, 0, 0);*/ // TODO TBR fix rxframe and subframe
    }

    if (preamble_index == rarh->RAPID) { // TBR double check this
      *t_crnti = (uint16_t) rar[5] + (rar[4] << 8);   //rar->t_crnti;
      nrUE_mac_inst->crnti = *t_crnti;    //rar->t_crnti;
      //return(rar->Timing_Advance_Command);
      ret = ((((uint16_t) (rar[0] & 0x7f)) << 4) + (rar[1] >> 4));
    } else {
      nrUE_mac_inst->crnti = 0;
      ret = (0xffff);
    }

    // move the selected RAR to the front of the RA_PDSCH buffer
    memcpy(selected_rar_buffer + 0, (uint8_t *) rarh, 1);
    memcpy(selected_rar_buffer + 1, (uint8_t *) rar, 6);

    return ret;
}