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

// table 7.2-1 TS 38.321
uint16_t table_7_2_1[16] = {
  5,    // row index 0
  10,   // row index 1
  20,   // row index 2
  30,   // row index 3
  40,   // row index 4
  60,   // row index 5
  80,   // row index 6
  120,  // row index 7
  160,  // row index 8
  240,  // row index 9
  320,  // row index 10
  480,  // row index 11
  960,  // row index 12
  1920, // row index 13
};

// WIP todo:
// - UL grant
uint16_t nr_ue_process_rar(module_id_t mod_id,
                           int CC_id,
                           frame_t frameP,
                           uint8_t * dlsch_buffer,
                           rnti_t * t_crnti,
                           uint8_t preamble_index,
                           uint8_t * selected_rar_buffer){

    NR_UE_MAC_INST_t *ue_mac = get_mac_inst(mod_id);
    NR_RA_HEADER_RAPID *rarh = (NR_RA_HEADER_RAPID *) dlsch_buffer; // RAR subheader pointer
    NR_MAC_RAR *rar = (NR_MAC_RAR *) (dlsch_buffer + 1);            // RAR subPDU pointer
    uint8_t n_subPDUs = 0;        // number of RAR payloads
    uint8_t n_subheaders = 0;     // number of MAC RAR subheaders
    uint8_t best_rx_rapid = -1;   // the closest RAPID receive from all RARs
    uint16_t ta_command = 0;

    AssertFatal(CC_id == 0, "RAR reception on secondary CCs is not supported yet\n");

    while (1) {
      n_subheaders++;
      if (rarh->T == 1) {
        n_subPDUs++;
        LOG_D(MAC, "[UE %d][RAPROC] Got RAPID RAR subPDU\n", mod_id);
      } else {
        n_subPDUs++;
        ue_mac->RA_backoff_indicator = table_7_2_1[((NR_RA_HEADER_BI *)rarh)->BI];
        ue_mac->RA_BI_found = 1;
        LOG_D(MAC, "[UE %d][RAPROC] Got BI RAR subPDU %d\n", mod_id, ue_mac->RA_backoff_indicator);
      }
      if (rarh->RAPID == preamble_index) {
        LOG_D(PHY, "[UE %d][RAPROC] Found RAR with the intended RAPID %d\n", mod_id, rarh->RAPID);
        rar = (NR_MAC_RAR *) (dlsch_buffer + n_subheaders + (n_subPDUs - 1) * sizeof(NR_MAC_RAR));
        ue_mac->RA_RAPID_found = 1;
        break;
      }
      // if (abs((int) rarh->RAPID - (int) preamble_index) < abs((int) best_rx_rapid - (int) preamble_index)) {
      //   best_rx_rapid = rarh->RAPID;
      //   rar = (NR_MAC_RAR *) (dlsch_buffer + n_subheaders + (n_subPDUs - 1) * sizeof(NR_MAC_RAR));
      // }
      if (rarh->E == 0) {
        LOG_I(PHY, "No RAR found with the intended RAPID. The closest RAPID in all RARs is %d\n", best_rx_rapid);
        break;
      } else {
        rarh += sizeof(NR_MAC_RAR) + 1;
      }
    };

    LOG_D(MAC, "number of RAR subheader %d; number of RAR pyloads %d\n", n_subheaders, n_subPDUs);

    // LOG_I(MAC, "[UE %d][RAPROC] Frame %d Received RAR (%02x|%02x.%02x.%02x.%02x.%02x.%02x) for preamble %d/%d\n",
    //   mod_id, frameP, *(uint8_t *) rarh, rar[0], rar[1], rar[2], rar[3], rar[4], rar[5], rarh->RAPID, preamble_index);

    if (ue_mac->RA_RAPID_found) {
      *t_crnti = rar->TCRNTI_2 + (rar->TCRNTI_1 << 8);
      ue_mac->t_crnti = *t_crnti;
      ue_mac->rnti_type = NR_RNTI_TC;
      ta_command = rar->TA2 + (rar->TA1 << 5);
    } else {
      ue_mac->t_crnti = 0;
      ta_command = (0xffff);
    }

    // move the selected RAR to the front of the RA_PDSCH buffer
    memcpy((void *) (selected_rar_buffer + 0), (void *) rarh, 1);
    memcpy((void *) (selected_rar_buffer + 1), (void *) rar, sizeof(NR_MAC_RAR));

    return ta_command;
}