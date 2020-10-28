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

/*! \file gNB_scheduler_bch.c
 * \brief procedures related to eNB for the BCH transport channel
 * \author  Navid Nikaein and Raymond Knopp, WEI-TAI CHEN
 * \date 2010 - 2014, 2018
 * \email: navid.nikaein@eurecom.fr, kroempa@gmail.com
 * \version 1.0
 * \company Eurecom, NTUST
 * @ingroup _mac

 */

#include <GNB_APP/RRC_nr_paramsvalues.h>
#include "assertions.h"
#include "NR_MAC_gNB/nr_mac_gNB.h"
#include "NR_MAC_gNB/mac_proto.h"
#include "NR_MAC_COMMON/nr_mac_extern.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "RRC/NR/nr_rrc_extern.h"
#include "common/utils/nr/nr_common.h"


#include "pdcp.h"

#define ENABLE_MAC_PAYLOAD_DEBUG
#define DEBUG_eNB_SCHEDULER 1

#include "common/ran_context.h"

#include "executables/softmodem-common.h"

extern RAN_CONTEXT_t RC;

void schedule_nr_mib(module_id_t module_idP, frame_t frameP, sub_frame_t slotP){

  gNB_MAC_INST *gNB_mac = RC.nrmac[module_idP];
  NR_COMMON_channels_t *cc;

  nfapi_nr_dl_tti_request_t      *dl_tti_request;
  nfapi_nr_dl_tti_request_body_t *dl_req;
  nfapi_nr_dl_tti_request_pdu_t  *dl_config_pdu;

  int mib_sdu_length;
  int CC_id;

  AssertFatal(slotP == 0, "Subframe must be 0\n");
  AssertFatal((frameP & 7) == 0, "Frame must be a multiple of 8\n");

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {

    dl_tti_request = &gNB_mac->DL_req[CC_id];
    dl_req = &dl_tti_request->dl_tti_request_body;
    cc = &gNB_mac->common_channels[CC_id];

    mib_sdu_length = mac_rrc_nr_data_req(module_idP, CC_id, frameP, MIBCH, 1, &cc->MIB_pdu.payload[0]); // not used in this case

    LOG_D(MAC, "Frame %d, slot %d: BCH PDU length %d\n", frameP, slotP, mib_sdu_length);

    if (mib_sdu_length > 0) {

      LOG_D(MAC, "Frame %d, slot %d: Adding BCH PDU in position %d (length %d)\n", frameP, slotP, dl_req->nPDUs, mib_sdu_length);

      if ((frameP & 1023) < 80){
        LOG_I(MAC,"[gNB %d] Frame %d : MIB->BCH  CC_id %d, Received %d bytes\n",module_idP, frameP, CC_id, mib_sdu_length);
      }

      dl_config_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs];
      memset((void *) dl_config_pdu, 0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
      dl_config_pdu->PDUType      = NFAPI_NR_DL_TTI_SSB_PDU_TYPE;
      dl_config_pdu->PDUSize      =2 + sizeof(nfapi_nr_dl_tti_ssb_pdu_rel15_t);

      AssertFatal(cc->ServingCellConfigCommon->physCellId!=NULL,"cc->ServingCellConfigCommon->physCellId is null\n");
      dl_config_pdu->ssb_pdu.ssb_pdu_rel15.PhysCellId          = *cc->ServingCellConfigCommon->physCellId;
      dl_config_pdu->ssb_pdu.ssb_pdu_rel15.BetaPss             = 0;
      dl_config_pdu->ssb_pdu.ssb_pdu_rel15.SsbBlockIndex       = 0;
      AssertFatal(cc->ServingCellConfigCommon->downlinkConfigCommon!=NULL,"scc->downlinkConfigCommonL is null\n");
      AssertFatal(cc->ServingCellConfigCommon->downlinkConfigCommon->frequencyInfoDL!=NULL,"scc->downlinkConfigCommon->frequencyInfoDL is null\n");
      AssertFatal(cc->ServingCellConfigCommon->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB!=NULL,"scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB is null\n");
      AssertFatal(cc->ServingCellConfigCommon->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.count==1,"Frequency Band list does not have 1 element (%d)\n",cc->ServingCellConfigCommon->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.count);
      AssertFatal(cc->ServingCellConfigCommon->ssbSubcarrierSpacing,"ssbSubcarrierSpacing is null\n");
      AssertFatal(cc->ServingCellConfigCommon->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0],"band is null\n");
      long band = *cc->ServingCellConfigCommon->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0];
      uint32_t ssb_offset0 = *cc->ServingCellConfigCommon->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB - cc->ServingCellConfigCommon->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA;
      int ratio;
      switch (*cc->ServingCellConfigCommon->ssbSubcarrierSpacing) {
        case NR_SubcarrierSpacing_kHz15:
          AssertFatal(band <= 79, "Band %ld is not possible for SSB with 15 kHz SCS\n",band);
          if (band<77) // below 3GHz
            ratio=3; // NRARFCN step is 5 kHz
          else
            ratio=1; // NRARFCN step is 15 kHz
          break;
        case NR_SubcarrierSpacing_kHz30:
          AssertFatal(band <= 79, "Band %ld is not possible for SSB with 15 kHz SCS\n",band);
          if (band<77) // below 3GHz
            ratio=6; // NRARFCN step is 5 kHz
          else
            ratio=2; // NRARFCN step is 15 kHz
          break;
        case NR_SubcarrierSpacing_kHz120:
          AssertFatal(band >= 257, "Band %ld is not possible for SSB with 120 kHz SCS\n",band);
          ratio=2; // NRARFCN step is 15 kHz
          break;
        case NR_SubcarrierSpacing_kHz240:
          AssertFatal(band >= 257, "Band %ld is not possible for SSB with 240 kHz SCS\n",band);
          ratio=4; // NRARFCN step is 15 kHz
          break;
        default:
          AssertFatal(1==0,"SCS %ld not allowed for SSB \n", *cc->ServingCellConfigCommon->ssbSubcarrierSpacing);
      }
      dl_config_pdu->ssb_pdu.ssb_pdu_rel15.SsbSubcarrierOffset = 0; //kSSB
      dl_config_pdu->ssb_pdu.ssb_pdu_rel15.ssbOffsetPointA     = ssb_offset0/(ratio*12) - 10; // absoluteFrequencySSB is the center of SSB
      dl_config_pdu->ssb_pdu.ssb_pdu_rel15.bchPayloadFlag      = 1;
      dl_config_pdu->ssb_pdu.ssb_pdu_rel15.bchPayload          = (*(uint32_t*)cc->MIB_pdu.payload) & ((1<<24)-1);
      dl_req->nPDUs++;

      uint8_t *vrb_map = cc[CC_id].vrb_map;
      const int rbStart = dl_config_pdu->ssb_pdu.ssb_pdu_rel15.ssbOffsetPointA;
      for (int rb = 0; rb < 20; rb++)
        vrb_map[rbStart + rb] = 1;
    }
  }

  // Get type0_PDCCH_CSS_config parameters
  PHY_VARS_gNB *gNB = RC.gNB[module_idP];
  NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;
  NR_MIB_t *mib = RC.nrrrc[module_idP]->carrier.mib.message.choice.mib;
  uint8_t gNB_xtra_byte = 0;
  for (int i = 0; i < 8; i++) {
    gNB_xtra_byte |= ((gNB->pbch.pbch_a >> (31 - i)) & 1) << (7 - i);
  }
  get_type0_PDCCH_CSS_config_parameters(&gNB_mac->type0_PDCCH_CSS_config, mib, gNB_xtra_byte, frame_parms->Lmax,
                                        frame_parms->ssb_index);
}

void schedule_control_sib1(module_id_t module_id,
                           int CC_id,
                           int bwp_id,
                           int time_domain_allocation,
                           uint8_t mcsTableIdx,
                           uint8_t mcs,
                           uint8_t numDmrsCdmGrpsNoData) {

  gNB_MAC_INST *gNB_mac = RC.nrmac[module_id];

  if (gNB_mac->sched_ctrlCommon == NULL){
    gNB_mac->sched_ctrlCommon = calloc(1,sizeof(*gNB_mac->sched_ctrlCommon));
    gNB_mac->sched_ctrlCommon->search_space = calloc(1,sizeof(*gNB_mac->sched_ctrlCommon->search_space));
    gNB_mac->sched_ctrlCommon->active_bwp = calloc(1,sizeof(*gNB_mac->sched_ctrlCommon->active_bwp));
    gNB_mac->sched_ctrlCommon->coreset = calloc(1,sizeof(*gNB_mac->sched_ctrlCommon->coreset));
    fill_default_searchSpaceZero(gNB_mac->sched_ctrlCommon->search_space);
    fill_default_coresetZero(gNB_mac->sched_ctrlCommon->coreset);
  }



  gNB_mac->sched_ctrlCommon->active_bwp = gNB_mac->secondaryCellGroupCommon->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[bwp_id-1];






  //NR_COMMON_channels_t                  *cc = &gNB_mac->common_channels[0];
  //NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
  //scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth;

  gNB_mac->sched_ctrlCommon->rbSize = 0;
  gNB_mac->sched_ctrlCommon->time_domain_allocation = time_domain_allocation;
  gNB_mac->sched_ctrlCommon->mcsTableIdx = mcsTableIdx;
  gNB_mac->sched_ctrlCommon->mcs = mcs;
  gNB_mac->sched_ctrlCommon->numDmrsCdmGrpsNoData = numDmrsCdmGrpsNoData;

  uint8_t nr_of_candidates;
  find_aggregation_candidates(&gNB_mac->sched_ctrlCommon->aggregation_level, &nr_of_candidates,gNB_mac->sched_ctrlCommon->search_space);

  gNB_mac->sched_ctrlCommon->cce_index = allocate_nr_CCEs(RC.nrmac[module_id],
                                                          gNB_mac->sched_ctrlCommon->active_bwp,
                                                          gNB_mac->sched_ctrlCommon->coreset,
                                                          gNB_mac->sched_ctrlCommon->aggregation_level,
                                                          0,
                                                          0);

  // Calculate rbStart and rbSize
  gNB_mac->sched_ctrlCommon->rbStart = NRRIV2PRBOFFSET(gNB_mac->sched_ctrlCommon->active_bwp->bwp_Common->genericParameters.locationAndBandwidth, 275);
  gNB_mac->sched_ctrlCommon->rbSize = 4; //24; //gNB_mac->type0_PDCCH_CSS_config.num_rbs;

  // Mark the corresponding RBs as used
  uint8_t *vrb_map = RC.nrmac[module_id]->common_channels[CC_id].vrb_map;
  for (int rb = 0; rb < gNB_mac->sched_ctrlCommon->rbSize; rb++) {
    vrb_map[rb + gNB_mac->sched_ctrlCommon->rbStart] = 1;
  }

  printf("searchSpaceId = %li\n", gNB_mac->sched_ctrlCommon->search_space->searchSpaceId);
  printf("locationAndBandwidth = %li\n", gNB_mac->sched_ctrlCommon->active_bwp->bwp_Common->genericParameters.locationAndBandwidth);
  printf("cce_index = %i\n", gNB_mac->sched_ctrlCommon->cce_index);
  printf("aggregation_level = %i\n", gNB_mac->sched_ctrlCommon->aggregation_level);
  printf("nr_of_candidates = %i\n", nr_of_candidates);
  printf("rbSize = %i\n", gNB_mac->sched_ctrlCommon->rbSize);
  printf("rbStart = %i\n", gNB_mac->sched_ctrlCommon->rbStart);
}

void nr_fill_nfapi_dci_sib1_pdu(int Mod_idP, nfapi_nr_dl_tti_request_body_t *dl_req) {

  gNB_MAC_INST *gNB_mac = RC.nrmac[Mod_idP];
  NR_ServingCellConfigCommon_t *scc = gNB_mac->common_channels->ServingCellConfigCommon;
  NR_BWP_Downlink_t *bwp = gNB_mac->sched_ctrlCommon->active_bwp;

  long locationAndBandwidth = bwp->bwp_Common->genericParameters.locationAndBandwidth;

  //long locationAndBandwidth = scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth;

  int dci10_bw = NRRIV2BW(locationAndBandwidth,275);

  nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdcch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs];
  memset((void*)dl_tti_pdcch_pdu,0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
  dl_tti_pdcch_pdu->PDUType = NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE;
  dl_tti_pdcch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdcch_pdu));

  nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15 = &dl_tti_pdcch_pdu->pdcch_pdu.pdcch_pdu_rel15;

  dci_pdu_rel15_t dci_pdu_rel15[MAX_DCI_CORESET];
  memset(dci_pdu_rel15, 0, sizeof(dci_pdu_rel15_t) * MAX_DCI_CORESET);

  dci_pdu_rel15[0].bwp_indicator.val = gNB_mac->sched_ctrlCommon->active_bwp->bwp_Id - 1;
  dci_pdu_rel15[0].frequency_domain_assignment.val = PRBalloc_to_locationandbandwidth0(gNB_mac->sched_ctrlCommon->rbSize,
                                                                                       gNB_mac->sched_ctrlCommon->rbStart,
                                                                                       dci10_bw);

  dci_pdu_rel15[0].time_domain_assignment.val = gNB_mac->sched_ctrlCommon->time_domain_allocation;
  dci_pdu_rel15[0].mcs = gNB_mac->sched_ctrlCommon->mcs;
  dci_pdu_rel15[0].rv = 0;
  dci_pdu_rel15[0].harq_pid = 0;
  dci_pdu_rel15[0].ndi = 0;
  dci_pdu_rel15[0].dai[0].val = 0;
  dci_pdu_rel15[0].tpc = 0;
  dci_pdu_rel15[0].pucch_resource_indicator = 0;
  dci_pdu_rel15[0].pdsch_to_harq_feedback_timing_indicator.val = 0;
  dci_pdu_rel15[0].antenna_ports.val = 0;
  dci_pdu_rel15[0].dmrs_sequence_initialization.val = 0;
  dci_pdu_rel15[0].system_info_indicator = 0;

  nr_configure_pdcch(gNB_mac,
                     pdcch_pdu_rel15,
                     0xFFFF, // SI-RNTI - 3GPP TS 38.321 Table 7.1-1: RNTI values
                     gNB_mac->sched_ctrlCommon->search_space,
                     gNB_mac->sched_ctrlCommon->coreset,
                     scc,
                     bwp,
                     gNB_mac->sched_ctrlCommon->aggregation_level,
                     gNB_mac->sched_ctrlCommon->cce_index);

  int dci_formats[2];
  int rnti_types[2];
  dci_formats[0]  = NR_DL_DCI_FORMAT_1_0;
  rnti_types[0]   = NR_RNTI_SI;

  fill_dci_pdu_rel15(scc,gNB_mac->secondaryCellGroupCommon,pdcch_pdu_rel15,dci_pdu_rel15,dci_formats,rnti_types, dci10_bw,bwp->bwp_Id);
  //fill_dci_pdu_rel15(scc,gNB_mac->secondaryCellGroupCommon,pdcch_pdu_rel15,dci_pdu_rel15,dci_formats,rnti_types, 50,bwp->bwp_Id);

  //dl_req->nPDUs += 1; // -----------------------------

  printf("locationAndBandwidth = %li\n", locationAndBandwidth);
  printf("dci10_bw = %i\n", dci10_bw);
}

void nr_fill_nfapi_sib1_pdu(int Mod_idP, nfapi_nr_dl_tti_request_body_t *dl_req) {

  gNB_MAC_INST *gNB_mac = RC.nrmac[Mod_idP];
  NR_BWP_Downlink_t *bwp = gNB_mac->sched_ctrlCommon->active_bwp;

  nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdsch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs+1];
  memset((void*)dl_tti_pdsch_pdu,0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
  dl_tti_pdsch_pdu->PDUType = NFAPI_NR_DL_TTI_PDSCH_PDU_TYPE;
  dl_tti_pdsch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdsch_pdu));

  nfapi_nr_dl_tti_pdsch_pdu_rel15_t *pdsch_pdu_rel15 = &dl_tti_pdsch_pdu->pdsch_pdu.pdsch_pdu_rel15;

  // Calculate number of symbols
  struct NR_PDSCH_TimeDomainResourceAllocationList *tdaList = bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
  const int startSymbolAndLength = tdaList->list.array[gNB_mac->sched_ctrlCommon->time_domain_allocation]->startSymbolAndLength;
  int startSymbolIndex, nrOfSymbols;
  SLIV2SL(startSymbolAndLength, &startSymbolIndex, &nrOfSymbols);

  // Calculate number of PRB_DMRS
  uint8_t N_PRB_DMRS;
  if(bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type == NFAPI_NR_DMRS_TYPE1) {
    N_PRB_DMRS = gNB_mac->sched_ctrlCommon->numDmrsCdmGrpsNoData * 6;
  } else {
    N_PRB_DMRS = gNB_mac->sched_ctrlCommon->numDmrsCdmGrpsNoData * 4;
  }

  pdsch_pdu_rel15->pduBitmap = 0;
  pdsch_pdu_rel15->rnti = 0xFFFF;
  pdsch_pdu_rel15->pduIndex = gNB_mac->pdu_index[0]++;

  pdsch_pdu_rel15->BWPSize  = NRRIV2BW(bwp->bwp_Common->genericParameters.locationAndBandwidth,275);
  pdsch_pdu_rel15->BWPStart = NRRIV2PRBOFFSET(bwp->bwp_Common->genericParameters.locationAndBandwidth,275);
  pdsch_pdu_rel15->SubcarrierSpacing = bwp->bwp_Common->genericParameters.subcarrierSpacing;

  if (bwp->bwp_Common->genericParameters.cyclicPrefix) {
    pdsch_pdu_rel15->CyclicPrefix = *bwp->bwp_Common->genericParameters.cyclicPrefix;
  }
  else {
    pdsch_pdu_rel15->CyclicPrefix = 0;
  }

  pdsch_pdu_rel15->NrOfCodewords = 1;
  pdsch_pdu_rel15->targetCodeRate[0] = nr_get_code_rate_dl(gNB_mac->sched_ctrlCommon->mcs,0);
  pdsch_pdu_rel15->qamModOrder[0] = nr_get_Qm_dl(gNB_mac->sched_ctrlCommon->mcs, gNB_mac->sched_ctrlCommon->mcsTableIdx);
  pdsch_pdu_rel15->mcsIndex[0] = gNB_mac->sched_ctrlCommon->mcs;
  pdsch_pdu_rel15->mcsTable[0] = 0;
  pdsch_pdu_rel15->rvIndex[0] = 0;
  pdsch_pdu_rel15->dataScramblingId =  *gNB_mac->common_channels->ServingCellConfigCommon->physCellId;
  pdsch_pdu_rel15->nrOfLayers = 1;
  pdsch_pdu_rel15->transmissionScheme = 0;
  pdsch_pdu_rel15->refPoint = 0; // Point A
  pdsch_pdu_rel15->dmrsConfigType = 0;
  pdsch_pdu_rel15->dlDmrsScramblingId = *gNB_mac->common_channels->ServingCellConfigCommon->physCellId;
  pdsch_pdu_rel15->SCID = 0;
  pdsch_pdu_rel15->numDmrsCdmGrpsNoData = gNB_mac->sched_ctrlCommon->numDmrsCdmGrpsNoData;
  pdsch_pdu_rel15->dmrsPorts = 1;
  pdsch_pdu_rel15->resourceAlloc = 1;
  pdsch_pdu_rel15->rbStart = gNB_mac->sched_ctrlCommon->rbStart;
  pdsch_pdu_rel15->rbSize = gNB_mac->sched_ctrlCommon->rbSize;
  pdsch_pdu_rel15->VRBtoPRBMapping = 1; // non-interleaved, check if this is ok for initialBWP
  pdsch_pdu_rel15->targetCodeRate[0] = nr_get_code_rate_dl(gNB_mac->sched_ctrlCommon->mcs, gNB_mac->sched_ctrlCommon->mcsTableIdx);
  pdsch_pdu_rel15->mcsTable[0] = gNB_mac->sched_ctrlCommon->mcsTableIdx;
  pdsch_pdu_rel15->StartSymbolIndex = startSymbolIndex;
  pdsch_pdu_rel15->NrOfSymbols = nrOfSymbols;

  pdsch_pdu_rel15->TBSize[0] = nr_compute_tbs(nr_get_Qm_dl(gNB_mac->sched_ctrlCommon->mcs, gNB_mac->sched_ctrlCommon->mcsTableIdx),
                                              nr_get_code_rate_dl(gNB_mac->sched_ctrlCommon->mcs, gNB_mac->sched_ctrlCommon->mcsTableIdx),
                                              gNB_mac->sched_ctrlCommon->rbSize, nrOfSymbols, N_PRB_DMRS,0,0,1) >> 3;

  pdsch_pdu_rel15->dlDmrsSymbPos = fill_dmrs_mask(bwp->bwp_Dedicated->pdsch_Config->choice.setup,
                                                  gNB_mac->common_channels->ServingCellConfigCommon->dmrs_TypeA_Position,
                                                  pdsch_pdu_rel15->NrOfSymbols);

  dl_req->nPDUs += 2; // ----------------------------
}

void schedule_nr_sib1(module_id_t module_idP, frame_t frameP, sub_frame_t slotP) {

  printf("\n\n--- Schedule_nr_sib1: Start\n");
  printf("frameP = %i, slotP = %i\n", frameP, slotP);

  int CC_id = 0;
  int bwp_id = 1;
  int time_domain_allocation = 0;
  uint8_t mcsTableIdx = 0;
  uint8_t mcs = 3; // 9
  uint8_t numDmrsCdmGrpsNoData = 1;

  gNB_MAC_INST *gNB_mac = RC.nrmac[module_idP];

  if(gNB_mac->type0_PDCCH_CSS_config.num_rbs<=0) {
    return;
  }

  printf("search_space_duration = %i\n", gNB_mac->type0_PDCCH_CSS_config.search_space_duration);
  printf("number_of_search_space_per_slot = %i\n", gNB_mac->type0_PDCCH_CSS_config.number_of_search_space_per_slot);
  printf("type0_pdcch_ss_mux_pattern = %i\n", gNB_mac->type0_PDCCH_CSS_config.type0_pdcch_ss_mux_pattern);
  printf("ssb_index = %i\n", gNB_mac->type0_PDCCH_CSS_config.ssb_index);
  printf("first_symbol_index = %i\n", gNB_mac->type0_PDCCH_CSS_config.first_symbol_index);
  printf("num_symbols = %i\n", gNB_mac->type0_PDCCH_CSS_config.num_symbols);
  printf("ssb_length = %i\n", gNB_mac->type0_PDCCH_CSS_config.ssb_length);
  printf("frame = %i\n", gNB_mac->type0_PDCCH_CSS_config.frame);
  printf("n_c = %i\n", gNB_mac->type0_PDCCH_CSS_config.n_c);
  printf("num_rbs = %i\n", gNB_mac->type0_PDCCH_CSS_config.num_rbs);
  printf("rb_offset = %i\n", gNB_mac->type0_PDCCH_CSS_config.rb_offset);
  printf("sfn_c = %i\n", gNB_mac->type0_PDCCH_CSS_config.sfn_c);
  printf("n_0 = %i\n", gNB_mac->type0_PDCCH_CSS_config.n_0);

  if( (frameP%2 == gNB_mac->type0_PDCCH_CSS_config.sfn_c) && (slotP == gNB_mac->type0_PDCCH_CSS_config.n_0) ) {

    printf("\nSIB1 will be transmitted here\n");

    // Computation of the sched_ctrlCommon: bwp, coreset0, rbStart, rbSize, etc.
    schedule_control_sib1(module_idP, CC_id, bwp_id, time_domain_allocation, mcsTableIdx, mcs, numDmrsCdmGrpsNoData);

    // Schedule broadcast DCI for the SIB1
    nfapi_nr_dl_tti_request_body_t *dl_req = &gNB_mac->DL_req[CC_id].dl_tti_request_body;
    nr_fill_nfapi_dci_sib1_pdu(module_idP, dl_req);

    // Get SIB1
    uint8_t sib1_payload[100];
    uint8_t sib1_sdu_length = mac_rrc_nr_data_req(module_idP, CC_id, frameP, BCCH, 1, sib1_payload);
    printf("sib1_sdu_length = %i\n", sib1_sdu_length);
    for(int i = 0; i<sib1_sdu_length; i++) {
      printf("%i ", sib1_payload[i]);

    }
    printf("\n");

    // Schedule broadcast the SIB1
    nr_fill_nfapi_sib1_pdu(module_idP, dl_req);







       const int ta_len =  2; // 0 or 2

       // Get RLC data TODO: remove random data retrieval
       int header_length_total = 0;
       int header_length_last = 0;
       int sdu_length_total = 0;
       int num_sdus = 0;
       uint16_t sdu_lengths[NB_RB_MAX] = {0};
       uint8_t mac_sdus[MAX_NR_DLSCH_PAYLOAD_BYTES];
       unsigned char sdu_lcids[NB_RB_MAX] = {0};
       const int lcid = DL_SCH_LCID_DTCH;



       // Calculate number of PRB_DMRS
       uint8_t N_PRB_DMRS;
       if(gNB_mac->sched_ctrlCommon->active_bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type == NFAPI_NR_DMRS_TYPE1) {
         N_PRB_DMRS = gNB_mac->sched_ctrlCommon->numDmrsCdmGrpsNoData * 6;
       } else {
         N_PRB_DMRS = gNB_mac->sched_ctrlCommon->numDmrsCdmGrpsNoData * 4;
       }



       // Calculate number of symbols
       struct NR_PDSCH_TimeDomainResourceAllocationList *tdaList = gNB_mac->sched_ctrlCommon->active_bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
       const int startSymbolAndLength = tdaList->list.array[gNB_mac->sched_ctrlCommon->time_domain_allocation]->startSymbolAndLength;
       int startSymbolIndex, nrOfSymbols;
       SLIV2SL(startSymbolAndLength, &startSymbolIndex, &nrOfSymbols);



       uint32_t TBS = nr_compute_tbs(nr_get_Qm_dl(gNB_mac->sched_ctrlCommon->mcs, gNB_mac->sched_ctrlCommon->mcsTableIdx),
                                     nr_get_code_rate_dl(gNB_mac->sched_ctrlCommon->mcs, gNB_mac->sched_ctrlCommon->mcsTableIdx),
                                     gNB_mac->sched_ctrlCommon->rbSize, nrOfSymbols, N_PRB_DMRS,0,0,1) >> 3;



    printf("TBS = %i\n", TBS);


       LOG_D(MAC, "Configuring DL_TX in %d.%d: random data\n", frameP, slotP);
       // fill dlsch_buffer with random data
       for (int i = 0; i < TBS; i++)
         mac_sdus[i] = (unsigned char) (lrand48()&0xff);

       sdu_lcids[0] = 0x3f; // DRB
       sdu_lengths[0] = TBS - ta_len - 3;
       header_length_total += 2 + (sdu_lengths[0] >= 128);
       sdu_length_total += sdu_lengths[0];
       num_sdus +=1;

       // Check if there is data from RLC or CE
       const int post_padding = TBS >= 2 + header_length_total + sdu_length_total + ta_len;
       // padding param currently not in use
       //padding = TBS - header_length_total - sdu_length_total - ta_len - 1;

       const int ntx_req = gNB_mac->TX_req[CC_id].Number_of_PDUs;
       nfapi_nr_pdu_t *tx_req = &gNB_mac->TX_req[CC_id].pdu_list[ntx_req];
       // pointer to directly generate the PDU into the nFAPI structure
       uint32_t *buf = tx_req->TLVs[0].value.direct;

       const int offset = nr_generate_dlsch_pdu(
           module_idP,
           gNB_mac->sched_ctrlCommon,
           (unsigned char *)mac_sdus,
           (unsigned char *)buf,
           num_sdus, // num_sdus
           sdu_lengths,
           sdu_lcids,
           255, // no drx
           NULL, // contention res id
           post_padding);

       // Padding: fill remainder of DLSCH with 0
       if (post_padding > 0) {
         for (int j = 0; j < TBS - offset; j++)
           buf[offset + j] = 0;
       }


       tx_req->PDU_length = TBS;
       tx_req->PDU_index  = gNB_mac->pdu_index[0]++;
       tx_req->num_TLV = 1;
       tx_req->TLVs[0].length = TBS + 2;
       gNB_mac->TX_req[CC_id].Number_of_PDUs++;
       gNB_mac->TX_req[CC_id].SFN = frameP;
       gNB_mac->TX_req[CC_id].Slot = slotP;
  }

    printf("--- Schedule_nr_sib1: End\n\n\n");

}


