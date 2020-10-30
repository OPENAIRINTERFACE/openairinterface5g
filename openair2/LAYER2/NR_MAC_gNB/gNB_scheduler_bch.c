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
                           uint8_t numDmrsCdmGrpsNoData,
                           int num_total_bytes) {

  gNB_MAC_INST *gNB_mac = RC.nrmac[module_id];
  uint8_t *vrb_map = RC.nrmac[module_id]->common_channels[CC_id].vrb_map;

  if (gNB_mac->sched_ctrlCommon == NULL){
    gNB_mac->sched_ctrlCommon = calloc(1,sizeof(*gNB_mac->sched_ctrlCommon));
    gNB_mac->sched_ctrlCommon->search_space = calloc(1,sizeof(*gNB_mac->sched_ctrlCommon->search_space));
    gNB_mac->sched_ctrlCommon->active_bwp = calloc(1,sizeof(*gNB_mac->sched_ctrlCommon->active_bwp));
    gNB_mac->sched_ctrlCommon->coreset = calloc(1,sizeof(*gNB_mac->sched_ctrlCommon->coreset));
    fill_default_searchSpaceZero(gNB_mac->sched_ctrlCommon->search_space);
    fill_default_coresetZero(gNB_mac->sched_ctrlCommon->coreset);
  }

  gNB_mac->sched_ctrlCommon->active_bwp = gNB_mac->secondaryCellGroupCommon->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[bwp_id-1];

  gNB_mac->sched_ctrlCommon->time_domain_allocation = time_domain_allocation;
  gNB_mac->sched_ctrlCommon->mcsTableIdx = mcsTableIdx;
  gNB_mac->sched_ctrlCommon->mcs = mcs;
  gNB_mac->sched_ctrlCommon->numDmrsCdmGrpsNoData = numDmrsCdmGrpsNoData;
  gNB_mac->sched_ctrlCommon->num_total_bytes = num_total_bytes;

  uint8_t nr_of_candidates;
  find_aggregation_candidates(&gNB_mac->sched_ctrlCommon->aggregation_level, &nr_of_candidates, gNB_mac->sched_ctrlCommon->search_space);

  gNB_mac->sched_ctrlCommon->cce_index = allocate_nr_CCEs(RC.nrmac[module_id],
                                                          gNB_mac->sched_ctrlCommon->active_bwp,
                                                          gNB_mac->sched_ctrlCommon->coreset,
                                                          gNB_mac->sched_ctrlCommon->aggregation_level,
                                                          0,
                                                              0);

  const uint16_t bwpSize = NRRIV2BW(gNB_mac->sched_ctrlCommon->active_bwp->bwp_Common->genericParameters.locationAndBandwidth, 275);
  int rbStart = NRRIV2PRBOFFSET(gNB_mac->sched_ctrlCommon->active_bwp->bwp_Common->genericParameters.locationAndBandwidth, 275);

  // Freq-domain allocation
  while (rbStart < bwpSize && vrb_map[rbStart]) rbStart++;

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

  int rbSize = 0;
  uint32_t TBS = 0;
  do {
    rbSize++;
    TBS = nr_compute_tbs(nr_get_Qm_dl(gNB_mac->sched_ctrlCommon->mcs, gNB_mac->sched_ctrlCommon->mcsTableIdx),
                           nr_get_code_rate_dl(gNB_mac->sched_ctrlCommon->mcs, gNB_mac->sched_ctrlCommon->mcsTableIdx),
                           rbSize, nrOfSymbols, N_PRB_DMRS,0, 0,1) >> 3;
  } while (rbStart + rbSize < bwpSize && !vrb_map[rbStart + rbSize] && TBS < gNB_mac->sched_ctrlCommon->num_total_bytes);
  gNB_mac->sched_ctrlCommon->rbSize = rbSize;
  gNB_mac->sched_ctrlCommon->rbStart = rbStart;

  // Mark the corresponding RBs as used
  for (int rb = 0; rb < gNB_mac->sched_ctrlCommon->rbSize; rb++) {
    vrb_map[rb + gNB_mac->sched_ctrlCommon->rbStart] = 1;
  }

}

void nr_fill_nfapi_dl_sib1_pdu(int Mod_idP,
                              nfapi_nr_dl_tti_request_body_t *dl_req,
                              uint32_t TBS,
                              int StartSymbolIndex,
                              int NrOfSymbols) {

  // static values
  int rnti = 0xFFFF;
  int dci_format = NR_DL_DCI_FORMAT_1_0;
  int rnti_type = NR_RNTI_SI;

  gNB_MAC_INST *gNB_mac = RC.nrmac[Mod_idP];
  NR_COMMON_channels_t *cc = gNB_mac->common_channels;
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
  NR_CellGroupConfig_t *secondaryCellGroup = gNB_mac->secondaryCellGroupCommon;
  NR_BWP_Downlink_t *bwp = gNB_mac->sched_ctrlCommon->active_bwp;

  // Uncommenting these lines, the DLSCH is decoded in the UE:
  //
  //rnti = 0x1234;
  //dci_format = NR_DL_DCI_FORMAT_1_1;
  //rnti_type = NR_RNTI_C;
  //gNB_mac->type0_PDCCH_CSS_config.n_0 = 6;
  //

  nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdcch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs];
  memset((void*)dl_tti_pdcch_pdu,0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
  dl_tti_pdcch_pdu->PDUType = NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE;
  dl_tti_pdcch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdcch_pdu));

  nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdsch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs+1];
  memset((void*)dl_tti_pdsch_pdu,0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
  dl_tti_pdsch_pdu->PDUType = NFAPI_NR_DL_TTI_PDSCH_PDU_TYPE;
  dl_tti_pdsch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdsch_pdu));

  nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15 = &dl_tti_pdcch_pdu->pdcch_pdu.pdcch_pdu_rel15;
  nfapi_nr_dl_tti_pdsch_pdu_rel15_t *pdsch_pdu_rel15 = &dl_tti_pdsch_pdu->pdsch_pdu.pdsch_pdu_rel15;

  pdsch_pdu_rel15->pduBitmap = 0;
  pdsch_pdu_rel15->rnti = rnti;
  pdsch_pdu_rel15->pduIndex = gNB_mac->pdu_index[0]++;

  pdsch_pdu_rel15->BWPSize  = NRRIV2BW(bwp->bwp_Common->genericParameters.locationAndBandwidth,275);
  pdsch_pdu_rel15->BWPStart = NRRIV2PRBOFFSET(bwp->bwp_Common->genericParameters.locationAndBandwidth,275);
  pdsch_pdu_rel15->SubcarrierSpacing = bwp->bwp_Common->genericParameters.subcarrierSpacing;
  if (bwp->bwp_Common->genericParameters.cyclicPrefix) {
    pdsch_pdu_rel15->CyclicPrefix = *bwp->bwp_Common->genericParameters.cyclicPrefix;
  } else {
    pdsch_pdu_rel15->CyclicPrefix = 0;
  }

  pdsch_pdu_rel15->NrOfCodewords = 1;
  pdsch_pdu_rel15->targetCodeRate[0] = nr_get_code_rate_dl(gNB_mac->sched_ctrlCommon->mcs,0);
  pdsch_pdu_rel15->qamModOrder[0] = 2;
  pdsch_pdu_rel15->mcsIndex[0] = gNB_mac->sched_ctrlCommon->mcs;
  pdsch_pdu_rel15->mcsTable[0] = 0;
  pdsch_pdu_rel15->rvIndex[0] = nr_rv_round_map[0];
  pdsch_pdu_rel15->dataScramblingId = *scc->physCellId;
  pdsch_pdu_rel15->nrOfLayers = 1;
  pdsch_pdu_rel15->transmissionScheme = 0;
  pdsch_pdu_rel15->refPoint = 0; // Point A
  pdsch_pdu_rel15->dmrsConfigType = gNB_mac->sched_ctrlCommon->active_bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type == NULL ? 0 : 1;
  pdsch_pdu_rel15->dlDmrsScramblingId = *scc->physCellId;
  pdsch_pdu_rel15->SCID = 0;
  pdsch_pdu_rel15->numDmrsCdmGrpsNoData = gNB_mac->sched_ctrlCommon->numDmrsCdmGrpsNoData;
  pdsch_pdu_rel15->dmrsPorts = 1;
  pdsch_pdu_rel15->resourceAlloc = 1;
  pdsch_pdu_rel15->rbStart = gNB_mac->sched_ctrlCommon->rbStart;
  pdsch_pdu_rel15->rbSize = gNB_mac->sched_ctrlCommon->rbSize;
  pdsch_pdu_rel15->VRBtoPRBMapping = 1;
  pdsch_pdu_rel15->qamModOrder[0] = nr_get_Qm_dl(gNB_mac->sched_ctrlCommon->mcs, gNB_mac->sched_ctrlCommon->mcsTableIdx);
  pdsch_pdu_rel15->TBSize[0] = TBS;
  pdsch_pdu_rel15->mcsTable[0] = gNB_mac->sched_ctrlCommon->mcsTableIdx;
  pdsch_pdu_rel15->StartSymbolIndex = StartSymbolIndex;
  pdsch_pdu_rel15->NrOfSymbols = NrOfSymbols;

  pdsch_pdu_rel15->dlDmrsSymbPos = fill_dmrs_mask(bwp->bwp_Dedicated->pdsch_Config->choice.setup, scc->dmrs_TypeA_Position, pdsch_pdu_rel15->NrOfSymbols);

  dci_pdu_rel15_t dci_pdu_rel15[MAX_DCI_CORESET];
  memset(dci_pdu_rel15, 0, sizeof(dci_pdu_rel15_t) * MAX_DCI_CORESET);

  int n_dl_bwp = secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count;

  if (n_dl_bwp < 4) {
    dci_pdu_rel15[0].bwp_indicator.val = gNB_mac->sched_ctrlCommon->active_bwp->bwp_Id;
  } else {
    dci_pdu_rel15[0].bwp_indicator.val = gNB_mac->sched_ctrlCommon->active_bwp->bwp_Id - 1; // as per table 7.3.1.1.2-1 in 38.212
  }

  // frequency domain assignment
  dci_pdu_rel15[0].frequency_domain_assignment.val =
      PRBalloc_to_locationandbandwidth0(pdsch_pdu_rel15->rbSize,
                                        pdsch_pdu_rel15->rbStart,
                                NRRIV2BW(bwp->bwp_Common->genericParameters.locationAndBandwidth,275));

  dci_pdu_rel15[0].time_domain_assignment.val = gNB_mac->sched_ctrlCommon->time_domain_allocation;
  dci_pdu_rel15[0].mcs = gNB_mac->sched_ctrlCommon->mcs;
  dci_pdu_rel15[0].rv = pdsch_pdu_rel15->rvIndex[0];
  dci_pdu_rel15[0].harq_pid = 0;
  dci_pdu_rel15[0].ndi = 0;
  dci_pdu_rel15[0].dai[0].val = 0;
  dci_pdu_rel15[0].tpc = 0; // table 7.2.1-1 in 38.213
  dci_pdu_rel15[0].pucch_resource_indicator = 0;
  dci_pdu_rel15[0].pdsch_to_harq_feedback_timing_indicator.val = 0;
  dci_pdu_rel15[0].antenna_ports.val = 0;
  dci_pdu_rel15[0].dmrs_sequence_initialization.val = pdsch_pdu_rel15->SCID;

  nr_configure_pdcch(gNB_mac,
                     pdcch_pdu_rel15,
                     rnti,
                     gNB_mac->sched_ctrlCommon->search_space,
                     gNB_mac->sched_ctrlCommon->coreset,
                     scc,
                     bwp,
                     gNB_mac->sched_ctrlCommon->aggregation_level,
                     gNB_mac->sched_ctrlCommon->cce_index);

  int dci_formats[2];
  int rnti_types[2];

  dci_formats[0]  = dci_format;
  rnti_types[0]   = rnti_type;

  fill_dci_pdu_rel15(scc,secondaryCellGroup,pdcch_pdu_rel15,dci_pdu_rel15,dci_formats,rnti_types,pdsch_pdu_rel15->BWPSize,gNB_mac->sched_ctrlCommon->active_bwp->bwp_Id);

  dl_req->nPDUs += 2;
}

void schedule_nr_sib1(module_id_t module_idP, frame_t frameP, sub_frame_t slotP) {

  printf("\n\nSchedule_nr_sib1: frameP = %i, slotP = %i\n", frameP, slotP);

  // static values
  const int CC_id = 0;
  int time_domain_allocation = 2;
  uint8_t mcsTableIdx = 0;
  uint8_t mcs = 9;
  uint8_t numDmrsCdmGrpsNoData = 1;
  int bwp_id = 1;

  gNB_MAC_INST *gNB_mac = RC.nrmac[module_idP];

  if( (frameP%2 == gNB_mac->type0_PDCCH_CSS_config.sfn_c) && (slotP == gNB_mac->type0_PDCCH_CSS_config.n_0 ) ) {

    printf("> SIB1 will be transmitted here\n");

    // Get SIB1
    uint8_t sib1_payload[100];
    uint8_t sib1_sdu_length = mac_rrc_nr_data_req(module_idP, CC_id, frameP, BCCH, 1, sib1_payload);
    printf("sib1_sdu_length = %i\n", sib1_sdu_length);
    printf("SIB1: ");
    for(int i = 0; i<sib1_sdu_length; i++) {
      printf("%x ", sib1_payload[i]);

    }
    printf("\n\n");

    // Configure sched_ctrlCommon for SIB1
    schedule_control_sib1(module_idP, CC_id, bwp_id, time_domain_allocation, mcsTableIdx, mcs, numDmrsCdmGrpsNoData, sib1_sdu_length);

    // Calculate number of symbols
    int startSymbolIndex, nrOfSymbols;
    struct NR_PDSCH_TimeDomainResourceAllocationList *tdaList = gNB_mac->sched_ctrlCommon->active_bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
    const int startSymbolAndLength = tdaList->list.array[gNB_mac->sched_ctrlCommon->time_domain_allocation]->startSymbolAndLength;
    SLIV2SL(startSymbolAndLength, &startSymbolIndex, &nrOfSymbols);

    // Calculate number of PRB_DMRS
    uint8_t N_PRB_DMRS;
    if(gNB_mac->sched_ctrlCommon->active_bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type == NFAPI_NR_DMRS_TYPE1) {
      N_PRB_DMRS = gNB_mac->sched_ctrlCommon->numDmrsCdmGrpsNoData * 6;
    } else {
      N_PRB_DMRS = gNB_mac->sched_ctrlCommon->numDmrsCdmGrpsNoData * 4;
    }

    const uint32_t TBS = nr_compute_tbs(nr_get_Qm_dl(gNB_mac->sched_ctrlCommon->mcs, gNB_mac->sched_ctrlCommon->mcsTableIdx),
                         nr_get_code_rate_dl(gNB_mac->sched_ctrlCommon->mcs, gNB_mac->sched_ctrlCommon->mcsTableIdx),
                                        gNB_mac->sched_ctrlCommon->rbSize, nrOfSymbols, N_PRB_DMRS,0 ,0 ,1 ) >> 3;

    nfapi_nr_dl_tti_request_body_t *dl_req = &gNB_mac->DL_req[CC_id].dl_tti_request_body;
    nr_fill_nfapi_dl_sib1_pdu(module_idP, dl_req, TBS, startSymbolIndex, nrOfSymbols);

    // reserve space for timing advance of UE if necessary,
    const int ta_len = (gNB_mac->sched_ctrlCommon->ta_apply) ? 2 : 0;

    int header_length_total = 0;
    int sdu_length_total = 0;
    int num_sdus = 0;
    uint16_t sdu_lengths[NB_RB_MAX] = {0};
    uint8_t mac_sdus[MAX_NR_DLSCH_PAYLOAD_BYTES];
    unsigned char sdu_lcids[NB_RB_MAX] = {0};

    // Data to be transmitted
    bzero(mac_sdus,MAX_NR_DLSCH_PAYLOAD_BYTES);
    //mac_sdus[2] = 0xFF;
    //mac_sdus[5] = 0xFF;
    memcpy(mac_sdus, sib1_payload, sib1_sdu_length);

    sdu_lcids[0] = 0x3f; // DRB
    sdu_lengths[0] = TBS - ta_len - 3;
    header_length_total += 2 + (sdu_lengths[0] >= 128);
    sdu_length_total += sdu_lengths[0];
    num_sdus +=1;

    // Check if there is data from RLC or CE
    const int post_padding = TBS >= 2 + header_length_total + sdu_length_total + ta_len;

    const int ntx_req = gNB_mac->TX_req[CC_id].Number_of_PDUs;
    nfapi_nr_pdu_t *tx_req = &gNB_mac->TX_req[CC_id].pdu_list[ntx_req];

    // pointer to directly generate the PDU into the nFAPI structure
    uint32_t *buf = tx_req->TLVs[0].value.direct;

    const int offset = nr_generate_dlsch_pdu(
          module_idP,
          gNB_mac->sched_ctrlCommon,
          (unsigned char *)mac_sdus,
          (unsigned char *)buf,
          num_sdus,
          sdu_lengths,
          sdu_lcids,
          255,
          NULL,
          post_padding);

    // Padding: fill remainder of DLSCH with 0
    if (post_padding > 0) {
      for (int j = 0; j < TBS - offset; j++) {
        buf[offset + j] = 0;
      }
    }

    tx_req->PDU_length = TBS;
    tx_req->PDU_index  = gNB_mac->pdu_index[0]++;
    tx_req->num_TLV = 1;
    tx_req->TLVs[0].length = TBS + 2;
    gNB_mac->TX_req[CC_id].Number_of_PDUs++;
    gNB_mac->TX_req[CC_id].SFN = frameP;
    gNB_mac->TX_req[CC_id].Slot = slotP;

  }

}


