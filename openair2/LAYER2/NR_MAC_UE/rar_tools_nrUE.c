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

/* PHY */
#include "openair1/SCHED_NR_UE/defs.h"

/* MAC */
#include "NR_MAC_UE/mac_proto.h"
#include "NR_MAC_UE/mac_extern.h"
#include "NR_MAC_COMMON/nr_mac_extern.h"
#include <common/utils/nr/nr_common.h>

// #define DEBUG_RAR
// #define DEBUG_MSG3

void nr_config_Msg3_pdu(NR_UE_MAC_INST_t *mac,
                        int Msg3_f_alloc,
                        uint8_t Msg3_t_alloc,
                        uint8_t mcs,
                        uint8_t freq_hopping){

  RA_config_t *ra = &mac->ra;
  int f_alloc, mask, StartSymbolIndex, NrOfSymbols;
  uint8_t nb_dmrs_re_per_rb;
  uint16_t number_dmrs_symbols = 0;
  int N_PRB_oh;
  fapi_nr_ul_config_request_t *ul_config = &mac->ul_config_request[0];
  nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu = &ul_config->ul_config_list[ul_config->number_pdus].pusch_config_pdu;
  NR_ServingCellConfigCommon_t *scc = mac->scc;
  NR_BWP_Uplink_t *ubwp = mac->ULbwp[0];
  NR_BWP_UplinkDedicated_t *ibwp = mac->scg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP;
  NR_PUSCH_Config_t *pusch_Config = ibwp->pusch_Config->choice.setup;
  int startSymbolAndLength = ubwp->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[Msg3_t_alloc]->startSymbolAndLength;

  #ifdef DEBUG_MSG3
  printf("[DEBUG_MSG3] Configuring 1 Msg3 PDU of %d UL pdus \n", ul_config->number_pdus);
  #endif

  // Num PRB Overhead from PUSCH-ServingCellConfig
  if (mac->scg->spCellConfig->spCellConfigDedicated->uplinkConfig->pusch_ServingCellConfig->choice.setup->xOverhead == NULL)
    N_PRB_oh = 0;
  else
    N_PRB_oh = *mac->scg->spCellConfig->spCellConfigDedicated->uplinkConfig->pusch_ServingCellConfig->choice.setup->xOverhead;

  // active BWP start
  int abwp_start = NRRIV2PRBOFFSET(ubwp->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  int abwp_size = NRRIV2BW(ubwp->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);

  // initial BWP start
  int ibwp_start = NRRIV2PRBOFFSET(scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  int ibwp_size = NRRIV2BW(scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);

  // BWP start selection according to 8.3 of TS 38.213
  pusch_config_pdu->bwp_size = ibwp_size;
  if ((ibwp_start < abwp_start) || (ibwp_size > abwp_size))
    pusch_config_pdu->bwp_start = abwp_start;
  else
    pusch_config_pdu->bwp_start = ibwp_start;

  //// Resource assignment from RAR
  // Frequency domain allocation according to 8.3 of TS 38.213
  if (ibwp_size < 180)
    mask = (1 << ((int) ceil(log2((ibwp_size*(ibwp_size+1))>>1)))) - 1;
  else
    mask = (1 << (28 - (int)(ceil(log2((ibwp_size*(ibwp_size+1))>>1))))) - 1;
  f_alloc = Msg3_f_alloc & mask;
  nr_ue_process_dci_freq_dom_resource_assignment(pusch_config_pdu, NULL, ibwp_size, 0, f_alloc);

  // virtual resource block to physical resource mapping for Msg3 PUSCH (6.3.1.7 in 38.211)
  pusch_config_pdu->rb_start += ibwp_start - abwp_start;

  // Time domain allocation
  SLIV2SL(startSymbolAndLength, &StartSymbolIndex, &NrOfSymbols);
  pusch_config_pdu->ul_dmrs_symb_pos = 1<<StartSymbolIndex;
  pusch_config_pdu->start_symbol_index = StartSymbolIndex;
  pusch_config_pdu->nr_of_symbols = NrOfSymbols;

  #ifdef DEBUG_MSG3
  printf("[DEBUG_MSG3] Freq assignment (RB start %d size %d) BWP (start %d, size %d), Time assignment (sliv_S %d sliv_L %d) \n",
    pusch_config_pdu->rb_start,
    pusch_config_pdu->rb_size,
    pusch_config_pdu->bwp_start,
    pusch_config_pdu->bwp_size,
    StartSymbolIndex,
    NrOfSymbols);
  #endif

  // MCS
  pusch_config_pdu->mcs_index = mcs;
  // Frequency hopping
  pusch_config_pdu->frequency_hopping = freq_hopping;
  // TC-RNTI
  pusch_config_pdu->rnti = ra->t_crnti;

  // DM-RS configuration according to 6.2.2 UE DM-RS transmission procedure in 38.214
  pusch_config_pdu->dmrs_config_type = pusch_dmrs_type1;
  pusch_config_pdu->num_dmrs_cdm_grps_no_data = 2;
  pusch_config_pdu->dmrs_ports = 1;
  get_num_re_dmrs(pusch_config_pdu,
                  &nb_dmrs_re_per_rb,
                  &number_dmrs_symbols);

  // DMRS sequence initialization [TS 38.211, sec 6.4.1.1.1].
  // Should match what is sent in DCI 0_1, otherwise set to 0.
  pusch_config_pdu->scid = 0;

  // Transform precoding according to 6.1.3 UE procedure for applying transform precoding on PUSCH in 38.214
  if (scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoder == NULL)
    pusch_config_pdu->transform_precoding = 1;
  else
    pusch_config_pdu->transform_precoding = 0;

  // Resource allocation in frequency domain according to 6.1.2.2 in TS 38.214
  pusch_config_pdu->resource_alloc = pusch_Config->resourceAllocation;

  //// Completing PUSCH PDU
  pusch_config_pdu->pdu_bit_map = PUSCH_PDU_BITMAP_PUSCH_DATA;
  pusch_config_pdu->mcs_table = 0;
  pusch_config_pdu->nrOfLayers = 1;
  pusch_config_pdu->cyclic_prefix = 0;
  pusch_config_pdu->target_code_rate = nr_get_code_rate_ul(pusch_config_pdu->mcs_index, pusch_config_pdu->mcs_table);
  pusch_config_pdu->qam_mod_order = nr_get_Qm_ul(pusch_config_pdu->mcs_index, pusch_config_pdu->mcs_table);
  pusch_config_pdu->data_scrambling_id = *scc->physCellId;
  pusch_config_pdu->ul_dmrs_scrambling_id = *scc->physCellId;
  pusch_config_pdu->subcarrier_spacing = ubwp->bwp_Common->genericParameters.subcarrierSpacing;
  pusch_config_pdu->vrb_to_prb_mapping = 0;
  pusch_config_pdu->uplink_frequency_shift_7p5khz = 0;
  //Optional Data only included if indicated in pduBitmap
  pusch_config_pdu->pusch_data.rv_index = 0;  // 8.3 in 38.213
  pusch_config_pdu->pusch_data.harq_process_id = 0;
  pusch_config_pdu->pusch_data.new_data_indicator = 1; // new data
  pusch_config_pdu->pusch_data.num_cb = 0;

  // Compute TBS
  pusch_config_pdu->pusch_data.tb_size = nr_compute_tbs(pusch_config_pdu->qam_mod_order,
                                                        pusch_config_pdu->target_code_rate,
                                                        pusch_config_pdu->rb_size,
                                                        pusch_config_pdu->nr_of_symbols,
                                                        nb_dmrs_re_per_rb*number_dmrs_symbols,
                                                        N_PRB_oh,
                                                        0, // TBR to verify tb scaling
                                                        pusch_config_pdu->nrOfLayers)/8;

}

/////////////////////////////////////
//    Random Access Response PDU   //
//         TS 38.213 ch 8.2        //
//        TS 38.321 ch 6.2.3       //
/////////////////////////////////////
//| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |// bit-wise
//| E | T |       R A P I D       |//
//| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |//
//| R |           T A             |//
//|       T A         |  UL grant |//
//|            UL grant           |//
//|            UL grant           |//
//|            UL grant           |//
//|         T C - R N T I         |//
//|         T C - R N T I         |//
/////////////////////////////////////
//       UL grant  (27 bits)       //
/////////////////////////////////////
//| 0 | 1 | 2 | 3 | 4 | 5 | 6 | 7 |// bit-wise
//|-------------------|FHF|F_alloc|//
//|        Freq allocation        |//
//|    F_alloc    |Time allocation|//
//|      MCS      |     TPC   |CSI|//
/////////////////////////////////////
// TbD WIP Msg3 development ongoing
// - apply UL grant freq alloc & time alloc as per 8.2 TS 38.213
// - apply tpc command
// WIP fix:
// - time domain indication hardcoded to 0 for k2 offset
// - extend TS 38.213 ch 8.3 Msg3 PUSCH
// - b buffer
// - ulsch power offset
// - optimize: mu_pusch, j and table_6_1_2_1_1_2_time_dom_res_alloc_A are already defined in nr_ue_procedures
void nr_ue_process_rar(nr_downlink_indication_t *dl_info, NR_UL_TIME_ALIGNMENT_t *ul_time_alignment, int pdu_id){

  module_id_t mod_id       = dl_info->module_id;
  frame_t frame            = dl_info->frame;
  int slot                 = dl_info->slot;
  int cc_id                = dl_info->cc_id;
  uint8_t gNB_id           = dl_info->gNB_index;
  NR_UE_MAC_INST_t *ue_mac = get_mac_inst(mod_id);
  RA_config_t *ra = &ue_mac->ra;
  uint8_t n_subPDUs        = 0;  // number of RAR payloads
  uint8_t n_subheaders     = 0;  // number of MAC RAR subheaders
  uint8_t *dlsch_buffer    = dl_info->rx_ind->rx_indication_body[pdu_id].pdsch_pdu.pdu;
  NR_RA_HEADER_RAPID *rarh = (NR_RA_HEADER_RAPID *) dlsch_buffer; // RAR subheader pointer
  NR_MAC_RAR *rar          = (NR_MAC_RAR *) (dlsch_buffer + 1);   // RAR subPDU pointer
  uint8_t preamble_index   = get_ra_PreambleIndex(mod_id, cc_id, gNB_id); //prach_resources->ra_PreambleIndex;

  LOG_D(MAC, "In %s:[%d.%d]: [UE %d][RAPROC] invoking MAC for received RAR (current preamble %d)\n", __FUNCTION__, frame, slot, mod_id, preamble_index);

  while (1) {
    n_subheaders++;
    if (rarh->T == 1) {
      n_subPDUs++;
      LOG_D(MAC, "[UE %d][RAPROC] Got RAPID RAR subPDU\n", mod_id);
    } else {
      n_subPDUs++;
      ra->RA_backoff_indicator = table_7_2_1[((NR_RA_HEADER_BI *)rarh)->BI];
      ra->RA_BI_found = 1;
      LOG_D(MAC, "[UE %d][RAPROC] Got BI RAR subPDU %d\n", mod_id, ra->RA_backoff_indicator);
    }
    if (rarh->RAPID == preamble_index) {
      LOG_I(MAC, "[UE %d][RAPROC][%d.%d] Found RAR with the intended RAPID %d\n", mod_id, frame, slot, rarh->RAPID);
      rar = (NR_MAC_RAR *) (dlsch_buffer + n_subheaders + (n_subPDUs - 1) * sizeof(NR_MAC_RAR));
      ra->RA_RAPID_found = 1;
      break;
    }
    if (rarh->E == 0) {
      LOG_W(PHY,"[UE %d][RAPROC] Received RAR preamble (%d) doesn't match the intended RAPID...\n", mod_id, preamble_index);
      break;
    } else {
      rarh += sizeof(NR_MAC_RAR) + 1;
    }
  }

  #ifdef DEBUG_RAR
  LOG_D(MAC, "[DEBUG_RAR] (%d,%d) number of RAR subheader %d; number of RAR pyloads %d\n", frame, slot, n_subheaders, n_subPDUs);
  LOG_D(MAC, "[DEBUG_RAR] Received RAR (%02x|%02x.%02x.%02x.%02x.%02x.%02x) for preamble %d/%d\n", *(uint8_t *) rarh, rar[0], rar[1], rar[2], rar[3], rar[4], rar[5], rarh->RAPID, preamble_index);
  #endif

  if (ra->RA_RAPID_found) {

    uint8_t freq_hopping, mcs, Msg3_t_alloc, Msg3_f_alloc;
    unsigned char tpc_command;
#ifdef DEBUG_RAR
    unsigned char csi_req;
#endif

  // TC-RNTI
  ra->t_crnti = rar->TCRNTI_2 + (rar->TCRNTI_1 << 8);

  // TA command
  ul_time_alignment->apply_ta = 1;
  ul_time_alignment->ta_command = rar->TA2 + (rar->TA1 << 5);

#ifdef DEBUG_RAR
  // CSI
  csi_req = (unsigned char) (rar->UL_GRANT_4 & 0x01);
#endif

  // TPC
  tpc_command = (unsigned char) ((rar->UL_GRANT_4 >> 1) & 0x07);
  switch (tpc_command){
    case 0:
      ra->Msg3_TPC = -6;
      break;
    case 1:
      ra->Msg3_TPC = -4;
      break;
    case 2:
      ra->Msg3_TPC = -2;
      break;
    case 3:
      ra->Msg3_TPC = 0;
      break;
    case 4:
      ra->Msg3_TPC = 2;
      break;
    case 5:
      ra->Msg3_TPC = 4;
      break;
    case 6:
      ra->Msg3_TPC = 6;
      break;
    case 7:
      ra->Msg3_TPC = 8;
      break;
  }
  // MCS
  mcs = (unsigned char) (rar->UL_GRANT_4 >> 4);
  // time alloc
  Msg3_t_alloc = (unsigned char) (rar->UL_GRANT_3 & 0x07);
  // frequency alloc
  Msg3_f_alloc = (uint16_t) ((rar->UL_GRANT_3 >> 4) | (rar->UL_GRANT_2 << 4) | ((rar->UL_GRANT_1 & 0x03) << 12));
  // frequency hopping
  freq_hopping = (unsigned char) (rar->UL_GRANT_1 >> 2);

  #ifdef DEBUG_RAR
  LOG_D(MAC, "In %s:[%d.%d]: [UE %d] Received RAR with t_alloc %d f_alloc %d ta_command %d mcs %d freq_hopping %d tpc_command %d csi_req %d t_crnti %x \n",
    __FUNCTION__,
    frame,
    slot,
    mod_id,
    Msg3_t_alloc,
    Msg3_f_alloc,
    ul_time_alignment->ta_command,
    mcs,
    freq_hopping,
    tpc_command,
    csi_req,
    ra->t_crnti);
  #endif

  // Config Msg3 PDU
  nr_config_Msg3_pdu(ue_mac, Msg3_f_alloc, Msg3_t_alloc, mcs, freq_hopping);
  // Schedule Msg3
  nr_ue_msg3_scheduler(ue_mac, frame, slot, Msg3_t_alloc);

  } else {
    ra->t_crnti = 0;
    ul_time_alignment->ta_command = (0xffff);
  }

}
