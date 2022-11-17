/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*! \file gNB_scheduler_phytest.c
 * \brief gNB scheduling procedures in phy_test mode
 * \author  Guy De Souza, G. Casati
 * \date 07/2018
 * \email: desouza@eurecom.fr, guido.casati@iis.fraunhofer.de
 * \version 1.0
 * @ingroup _mac
 */

#include "nr_mac_gNB.h"
#include "NR_MAC_gNB/mac_proto.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_common.h"
#include "executables/nr-softmodem.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac.h"
#include "executables/softmodem-common.h"
#include "common/utils/nr/nr_common.h"
#include "NR_SCS-SpecificCarrier.h"
#include "NR_TDD-UL-DL-ConfigCommon.h"
#include "NR_FrequencyInfoUL.h"
#include "NR_RACH-ConfigGeneric.h"
#include "NR_RACH-ConfigCommon.h"
#include "NR_PUSCH-TimeDomainResourceAllocation.h"
#include "NR_PUSCH-ConfigCommon.h"
#include "NR_PUCCH-ConfigCommon.h"
#include "NR_PDSCH-TimeDomainResourceAllocation.h"
#include "NR_PDSCH-ConfigCommon.h"
#include "NR_RateMatchPattern.h"
#include "NR_RateMatchPatternLTE-CRS.h"
#include "NR_SearchSpace.h"
#include "NR_ControlResourceSet.h"

//#define UL_HARQ_PRINT
extern RAN_CONTEXT_t RC;

//#define ENABLE_MAC_PAYLOAD_DEBUG 1

/*Scheduling of DLSCH with associated DCI in common search space
 * current version has only a DCI for type 1 PDCCH for C_RNTI*/
void nr_schedule_css_dlsch_phytest(module_id_t   module_idP,
                                   frame_t       frameP,
                                   sub_frame_t   slotP) {
  uint8_t  CC_id;
  gNB_MAC_INST                      *nr_mac      = RC.nrmac[module_idP];
  NR_COMMON_channels_t              *cc = &nr_mac->common_channels[0];
  nfapi_nr_dl_tti_request_body_t    *dl_req;
  nfapi_nr_dl_tti_request_pdu_t     *dl_tti_pdcch_pdu;
  nfapi_nr_dl_tti_request_pdu_t     *dl_tti_pdsch_pdu;
  nfapi_nr_pdu_t        *TX_req;

  uint16_t rnti = 0x1234;
  //  int time_domain_assignment,k0;

  NR_ServingCellConfigCommon_t *scc=cc->ServingCellConfigCommon;

  int dlBWP_carrier_bandwidth = NRRIV2BW(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);

  
  /*
  int scs               = scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.subcarrierSpacing;
  
  int slots_per_frame   = 10*(1<<scs);

  int FR                = *scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0] >= 257 ? nr_FR2 : nr_FR1;
  */

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    LOG_D(MAC, "Scheduling common search space DCI type 1 dlBWP BW.firstRB %d.%d\n",
	  dlBWP_carrier_bandwidth,
	  NRRIV2PRBOFFSET(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE));
    
    
    dl_req = &nr_mac->DL_req[CC_id].dl_tti_request_body;
    dl_tti_pdcch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs];
    memset((void*)dl_tti_pdcch_pdu,0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
    dl_tti_pdcch_pdu->PDUType = NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE;
    dl_tti_pdcch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdcch_pdu));
    
    dl_tti_pdsch_pdu = &nr_mac->DL_req[CC_id].dl_tti_request_body.dl_tti_pdu_list[nr_mac->DL_req[CC_id].dl_tti_request_body.nPDUs+1];
    memset((void *)dl_tti_pdsch_pdu,0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
    dl_tti_pdsch_pdu->PDUType = NFAPI_NR_DL_TTI_PDSCH_PDU_TYPE;
    dl_tti_pdsch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdsch_pdu));

    
    //    nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15 = &dl_tti_pdcch_pdu->pdcch_pdu.pdcch_pdu_rel15;
    nfapi_nr_dl_tti_pdsch_pdu_rel15_t *pdsch_pdu_rel15 = &dl_tti_pdsch_pdu->pdsch_pdu.pdsch_pdu_rel15;
    
    pdsch_pdu_rel15->pduBitmap = 0;
    pdsch_pdu_rel15->rnti = rnti;
    pdsch_pdu_rel15->pduIndex = 0;

    // BWP
    pdsch_pdu_rel15->BWPSize  = NRRIV2BW(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
    pdsch_pdu_rel15->BWPStart = NRRIV2PRBOFFSET(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
    pdsch_pdu_rel15->SubcarrierSpacing = scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.subcarrierSpacing;
    pdsch_pdu_rel15->CyclicPrefix = 0;
    pdsch_pdu_rel15->NrOfCodewords = 1;
    int mcsIndex = 9;
    pdsch_pdu_rel15->targetCodeRate[0] = nr_get_code_rate_dl(mcsIndex,0);
    pdsch_pdu_rel15->qamModOrder[0] = 2;
    pdsch_pdu_rel15->mcsIndex[0] = mcsIndex;
    pdsch_pdu_rel15->mcsTable[0] = 0;
    pdsch_pdu_rel15->rvIndex[0] = 0;
    pdsch_pdu_rel15->dataScramblingId = *scc->physCellId;
    pdsch_pdu_rel15->nrOfLayers = 1;    
    pdsch_pdu_rel15->transmissionScheme = 0;
    pdsch_pdu_rel15->refPoint = 0; // Point A
    
    pdsch_pdu_rel15->dmrsConfigType = 0; // Type 1 by default for InitialBWP
    pdsch_pdu_rel15->dlDmrsScramblingId = *scc->physCellId;
    pdsch_pdu_rel15->SCID = 0;
    pdsch_pdu_rel15->numDmrsCdmGrpsNoData = 1;
    pdsch_pdu_rel15->dmrsPorts = 1;
    pdsch_pdu_rel15->resourceAlloc = 1;
    pdsch_pdu_rel15->rbStart = 0;
    pdsch_pdu_rel15->rbSize = 6;
    pdsch_pdu_rel15->VRBtoPRBMapping = 1; // non-interleaved, check if this is ok for initialBWP
    // choose shortest PDSCH
    int startSymbolAndLength=0;
    int StartSymbolIndex=-1,NrOfSymbols=14;
    int StartSymbolIndex_tmp,NrOfSymbols_tmp;
    int mappingtype_tmp, mappingtype=0;

    for (int i=0;
	 i<scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.count;
	 i++) {
      startSymbolAndLength = scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->startSymbolAndLength;
      SLIV2SL(startSymbolAndLength,&StartSymbolIndex_tmp,&NrOfSymbols_tmp);
      mappingtype_tmp = scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->mappingType;
      if (NrOfSymbols_tmp < NrOfSymbols) {
	NrOfSymbols = NrOfSymbols_tmp;
        StartSymbolIndex = StartSymbolIndex_tmp;
        mappingtype = mappingtype_tmp;
      }
    }
    AssertFatal(StartSymbolIndex>=0,"StartSymbolIndex is negative\n");
    pdsch_pdu_rel15->StartSymbolIndex = StartSymbolIndex;
    pdsch_pdu_rel15->NrOfSymbols      = NrOfSymbols;
    pdsch_pdu_rel15->dlDmrsSymbPos = fill_dmrs_mask(NULL,
                                                    scc->dmrs_TypeA_Position,
                                                    NrOfSymbols,
                                                    StartSymbolIndex,
                                                    mappingtype,
                                                    1);

    nr_mac->DL_req[CC_id].dl_tti_request_body.nPDUs+=2;
    
    TX_req = &nr_mac->TX_req[CC_id].pdu_list[nr_mac->TX_req[CC_id].Number_of_PDUs];
    TX_req->PDU_length = 6;
    TX_req->PDU_index = nr_mac->pdu_index[CC_id]++;
    TX_req->num_TLV = 1;
    TX_req->TLVs[0].length = 8;
    // why do we copy from RAR_pdu here? Shouldn't we fill some more or less
    // meaningful data, e.g., padding + random data?
    //memcpy((void *)&TX_req->TLVs[0].value.direct[0], (void *)&cc[CC_id].RAR_pdu[0].payload[0], TX_req->TLVs[0].length);
    nr_mac->TX_req[CC_id].Number_of_PDUs++;
    nr_mac->TX_req[CC_id].SFN=frameP;
    nr_mac->TX_req[CC_id].Slot=slotP;
  }
}

uint32_t target_dl_mcs = 9;
uint32_t target_dl_Nl = 1;
uint32_t target_dl_bw = 50;
uint64_t dlsch_slot_bitmap = (1<<1);
/* schedules whole bandwidth for first user, all the time */
void nr_preprocessor_phytest(module_id_t module_id,
                             frame_t frame,
                             sub_frame_t slot)
{
  if (!is_xlsch_in_slot(dlsch_slot_bitmap, slot))
    return;
  NR_UE_info_t *UE = RC.nrmac[module_id]->UE_info.list[0];
  NR_ServingCellConfigCommon_t *scc = RC.nrmac[module_id]->common_channels[0].ServingCellConfigCommon;
  NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
  NR_UE_DL_BWP_t *dl_bwp = &UE->current_DL_BWP;
  const int CC_id = 0;

  const int tda = get_dl_tda(RC.nrmac[module_id], scc, slot);
  NR_tda_info_t tda_info = nr_get_pdsch_tda_info(dl_bwp, tda);
  sched_ctrl->sched_pdsch.tda_info = tda_info;
  sched_ctrl->sched_pdsch.time_domain_allocation = tda;

  /* find largest unallocated chunk */
  const int bwpSize = dl_bwp->BWPSize;
  const int BWPStart = dl_bwp->BWPStart;

  int rbStart = 0;
  int rbSize = 0;
  if (target_dl_bw>bwpSize)
    target_dl_bw = bwpSize;
  uint16_t *vrb_map = RC.nrmac[module_id]->common_channels[CC_id].vrb_map;
  /* loop ensures that we allocate exactly target_dl_bw, or return */
  while (true) {
    /* advance to first free RB */
    while (rbStart < bwpSize &&
           (vrb_map[rbStart + BWPStart]&SL_to_bitmap(tda_info.startSymbolIndex, tda_info.nrOfSymbols)))
      rbStart++;
    rbSize = 1;
    /* iterate until we are at target_dl_bw or no available RBs */
    while (rbStart + rbSize < bwpSize &&
           !(vrb_map[rbStart + rbSize + BWPStart]&SL_to_bitmap(tda_info.startSymbolIndex, tda_info.nrOfSymbols)) &&
           rbSize < target_dl_bw)
      rbSize++;
    /* found target_dl_bw? */
    if (rbSize == target_dl_bw)
      break;
    /* at end and below target_dl_bw? */
    if (rbStart + rbSize >= bwpSize)
      return;
    rbStart += rbSize;
  }

  sched_ctrl->num_total_bytes = 0;
  sched_ctrl->dl_lc_num = 1;
  const int lcid = DL_SCH_LCID_DTCH;
  sched_ctrl->dl_lc_ids[sched_ctrl->dl_lc_num - 1] = lcid;
  const uint16_t rnti = UE->rnti;
  /* update sched_ctrl->num_total_bytes so that postprocessor schedules data,
   * if available */
  sched_ctrl->rlc_status[lcid] = mac_rlc_status_ind(module_id,
                                                    rnti,
                                                    module_id,
                                                    frame,
                                                    slot,
                                                    ENB_FLAG_YES,
                                                    MBMS_FLAG_NO,
                                                    lcid,
                                                    0,
                                                    0);
  sched_ctrl->num_total_bytes += sched_ctrl->rlc_status[lcid].bytes_in_buffer;

  int CCEIndex = get_cce_index(RC.nrmac[module_id],
                               CC_id, slot, UE->rnti,
                               &sched_ctrl->aggregation_level,
                               sched_ctrl->search_space,
                               sched_ctrl->coreset,
                               &sched_ctrl->sched_pdcch,
                               false);
  AssertFatal(CCEIndex >= 0,
              "%s(): could not find CCE for UE %04x\n",
              __func__,
              UE->rnti);

  int r_pucch = nr_get_pucch_resource(sched_ctrl->coreset, UE->current_UL_BWP.pucch_Config, CCEIndex);
  const int alloc = nr_acknack_scheduling(RC.nrmac[module_id], UE, frame, slot, r_pucch, 0);
  if (alloc < 0) {
    LOG_D(MAC,
          "%s(): could not find PUCCH for UE %04x@%d.%d\n",
          __func__,
          rnti,
          frame,
          slot);
    return;
  }

  sched_ctrl->cce_index = CCEIndex;

  fill_pdcch_vrb_map(RC.nrmac[module_id],
                     CC_id,
                     &sched_ctrl->sched_pdcch,
                     CCEIndex,
                     sched_ctrl->aggregation_level);

  //AssertFatal(alloc,
  //            "could not find uplink slot for PUCCH (RNTI %04x@%d.%d)!\n",
  //            rnti, frame, slot);

  NR_sched_pdsch_t *sched_pdsch = &sched_ctrl->sched_pdsch;
  sched_pdsch->pucch_allocation = alloc;
  sched_pdsch->rbStart = rbStart;
  sched_pdsch->rbSize = rbSize;

  sched_pdsch->dmrs_parms = get_dl_dmrs_params(scc,
                                               dl_bwp,
                                               &tda_info,
                                               target_dl_Nl);

  sched_pdsch->mcs = target_dl_mcs;
  sched_pdsch->nrOfLayers = target_dl_Nl;
  sched_pdsch->Qm = nr_get_Qm_dl(sched_pdsch->mcs, dl_bwp->mcsTableIdx);
  sched_pdsch->R = nr_get_code_rate_dl(sched_pdsch->mcs, dl_bwp->mcsTableIdx);
  sched_ctrl->dl_bler_stats.mcs = target_dl_mcs; /* for logging output */
  sched_pdsch->tb_size = nr_compute_tbs(sched_pdsch->Qm,
                                        sched_pdsch->R,
                                        sched_pdsch->rbSize,
                                        tda_info.nrOfSymbols,
                                        sched_pdsch->dmrs_parms.N_PRB_DMRS * sched_pdsch->dmrs_parms.N_DMRS_SLOT,
                                        0 /* N_PRB_oh, 0 for initialBWP */,
                                        0 /* tb_scaling */,
                                        sched_pdsch->nrOfLayers)
                         >> 3;

  /* get the PID of a HARQ process awaiting retransmission, or -1 otherwise */
  sched_pdsch->dl_harq_pid = sched_ctrl->retrans_dl_harq.head;

  /* mark the corresponding RBs as used */
  for (int rb = 0; rb < sched_pdsch->rbSize; rb++)
    vrb_map[rb + sched_pdsch->rbStart + BWPStart] = SL_to_bitmap(tda_info.startSymbolIndex, tda_info.nrOfSymbols);

  if ((frame&127) == 0) LOG_D(MAC,"phytest: %d.%d DL mcs %d, DL rbStart %d, DL rbSize %d\n", frame, slot, sched_pdsch->mcs, rbStart,rbSize);
}

uint32_t target_ul_mcs = 9;
uint32_t target_ul_bw = 50;
uint32_t target_ul_Nl = 1;
uint64_t ulsch_slot_bitmap = (1 << 8);
bool nr_ul_preprocessor_phytest(module_id_t module_id, frame_t frame, sub_frame_t slot)
{
  gNB_MAC_INST *nr_mac = RC.nrmac[module_id];
  NR_COMMON_channels_t *cc = nr_mac->common_channels;
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
  NR_UE_info_t *UE = nr_mac->UE_info.list[0];

  AssertFatal(nr_mac->UE_info.list[1] == NULL,
              "cannot handle more than one UE\n");
  if (UE == NULL)
    return false;

  const int CC_id = 0;

  NR_UE_sched_ctrl_t *sched_ctrl = &UE->UE_sched_ctrl;
  NR_UE_UL_BWP_t *ul_bwp = &UE->current_UL_BWP;
  const int mu = ul_bwp->scs;

  const struct NR_PUSCH_TimeDomainResourceAllocationList *tdaList = ul_bwp->tdaList;
  const int temp_tda = get_ul_tda(nr_mac, scc, slot);
  if (temp_tda < 0)
    return false;
  AssertFatal(temp_tda < tdaList->list.count,
              "time domain assignment %d >= %d\n",
              temp_tda,
              tdaList->list.count);
  int K2 = get_K2(ul_bwp->tdaList, temp_tda, mu);
  const int sched_frame = frame + (slot + K2 >= nr_slots_per_frame[mu]);
  const int sched_slot = (slot + K2) % nr_slots_per_frame[mu];
  const int tda = get_ul_tda(nr_mac, scc, sched_slot);
  if (tda < 0)
    return false;
  AssertFatal(tda < tdaList->list.count,
              "time domain assignment %d >= %d\n",
              tda,
              tdaList->list.count);
  /* check if slot is UL, and that slot is 8 (assuming K2=6 because of UE
   * limitations).  Note that if K2 or the TDD configuration is changed, below
   * conditions might exclude each other and never be true */
  if (!is_xlsch_in_slot(ulsch_slot_bitmap, sched_slot))
    return false;

  uint16_t rbStart = 0;
  uint16_t rbSize;

  const int bw = ul_bwp->BWPSize;
  const int BWPStart = ul_bwp->BWPStart;

  if (target_ul_bw>bw)
    rbSize = bw;
  else
    rbSize = target_ul_bw;

  NR_tda_info_t tda_info = nr_get_pusch_tda_info(ul_bwp, tda);
  sched_ctrl->sched_pusch.tda_info = tda_info;

  uint16_t *vrb_map_UL =
      &RC.nrmac[module_id]->common_channels[CC_id].vrb_map_UL[sched_slot * MAX_BWP_SIZE];
  for (int i = rbStart; i < rbStart + rbSize; ++i) {
    if ((vrb_map_UL[i+BWPStart] & SL_to_bitmap(tda_info.startSymbolIndex, tda_info.nrOfSymbols)) != 0) {
      LOG_E(MAC,
            "%s(): %4d.%2d RB %d is already reserved, cannot schedule UE\n",
            __func__,
            frame,
            slot,
            i);
      return false;
    }
  }

  sched_ctrl->sched_pusch.slot = sched_slot;
  sched_ctrl->sched_pusch.frame = sched_frame;

  int CCEIndex = get_cce_index(nr_mac,
                               CC_id, slot, UE->rnti,
                               &sched_ctrl->aggregation_level,
                               sched_ctrl->search_space,
                               sched_ctrl->coreset,
                               &sched_ctrl->sched_pdcch,
                               false);
  if (CCEIndex < 0) {
    LOG_E(MAC, "%s(): CCE list not empty, couldn't schedule PUSCH\n", __func__);
    return false;
  }

  sched_ctrl->cce_index = CCEIndex;

  const int mcs = target_ul_mcs;
  NR_sched_pusch_t *sched_pusch = &sched_ctrl->sched_pusch;
  sched_pusch->mcs = mcs;
  sched_ctrl->ul_bler_stats.mcs = mcs; /* for logging output */
  sched_pusch->rbStart = rbStart;
  sched_pusch->rbSize = rbSize;
  /* get the PID of a HARQ process awaiting retransmission, or -1 for "any new" */
  sched_pusch->ul_harq_pid = sched_ctrl->retrans_ul_harq.head;

  /* Calculate TBS from MCS */
  sched_pusch->nrOfLayers = target_ul_Nl;
  sched_pusch->R = nr_get_code_rate_ul(mcs, ul_bwp->mcs_table);
  sched_pusch->Qm = nr_get_Qm_ul(mcs, ul_bwp->mcs_table);
  if (ul_bwp->pusch_Config->tp_pi2BPSK
      && ((ul_bwp->mcs_table == 3 && mcs < 2) || (ul_bwp->mcs_table == 4 && mcs < 6))) {
    sched_pusch->R >>= 1;
    sched_pusch->Qm <<= 1;
  }

  NR_pusch_dmrs_t dmrs = get_ul_dmrs_params(scc,
                                            ul_bwp,
                                            &tda_info,
                                            sched_pusch->nrOfLayers);
  sched_ctrl->sched_pusch.dmrs_info = dmrs;

  sched_pusch->tb_size = nr_compute_tbs(sched_pusch->Qm,
                                        sched_pusch->R,
                                        sched_pusch->rbSize,
                                        tda_info.nrOfSymbols,
                                        dmrs.N_PRB_DMRS * dmrs.num_dmrs_symb,
                                        0, // nb_rb_oh
                                        0,
                                        sched_pusch->nrOfLayers /* NrOfLayers */)
                         >> 3;

  /* mark the corresponding RBs as used */
  fill_pdcch_vrb_map(nr_mac,
                     CC_id,
                     &sched_ctrl->sched_pdcch,
                     CCEIndex,
                     sched_ctrl->aggregation_level);

  for (int rb = rbStart; rb < rbStart + rbSize; rb++)
    vrb_map_UL[rb+BWPStart] |= SL_to_bitmap(tda_info.startSymbolIndex, tda_info.nrOfSymbols);
  return true;
}
