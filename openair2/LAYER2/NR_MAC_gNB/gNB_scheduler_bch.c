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

  // Calculate rbStart and rbSize
  gNB_mac->sched_ctrlCommon->rbStart = NRRIV2PRBOFFSET(gNB_mac->sched_ctrlCommon->active_bwp->bwp_Common->genericParameters.locationAndBandwidth, 275);
  gNB_mac->sched_ctrlCommon->rbSize = gNB_mac->type0_PDCCH_CSS_config.num_rbs;
  uint32_t TBS = nr_compute_tbs(nr_get_Qm_dl(gNB_mac->sched_ctrlCommon->mcs, gNB_mac->sched_ctrlCommon->mcsTableIdx),
                         nr_get_code_rate_dl(gNB_mac->sched_ctrlCommon->mcs, gNB_mac->sched_ctrlCommon->mcsTableIdx),
                                gNB_mac->sched_ctrlCommon->rbSize, nrOfSymbols, N_PRB_DMRS,0,0,1) >> 3;

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
  printf("startSymbolIndex = %i\n", startSymbolIndex);
  printf("nrOfSymbols = %i\n", nrOfSymbols);
  printf("rbSize = %i\n", gNB_mac->sched_ctrlCommon->rbSize);
  printf("rbStart = %i\n", gNB_mac->sched_ctrlCommon->rbStart);
  printf("TBS = %i\n", TBS);
  printf("N_PRB_DMRS = %i\n", N_PRB_DMRS);
}

void nr_fill_nfapi_dci_sib1_pdu(int Mod_idP, nfapi_nr_dl_tti_request_body_t *dl_req) {

  gNB_MAC_INST *gNB_mac = RC.nrmac[Mod_idP];
  NR_ServingCellConfigCommon_t *scc = gNB_mac->common_channels->ServingCellConfigCommon;
  NR_BWP_Downlink_t *bwp = gNB_mac->sched_ctrlCommon->active_bwp;

  long locationAndBandwidth = bwp->bwp_Common->genericParameters.locationAndBandwidth;
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

  dl_req->nPDUs += 1;

  printf("locationAndBandwidth = %li\n", locationAndBandwidth);
  printf("dci10_bw = %i\n", dci10_bw);
}

void schedule_nr_sib1(module_id_t module_idP, frame_t frameP, sub_frame_t slotP) {

  printf("\n\n--- Schedule_nr_sib1: Start\n");
  printf("frameP = %i, slotP = %i\n", frameP, slotP);

  int CC_id = 0;
  int bwp_id = 1;
  int time_domain_allocation = 0;
  uint8_t mcsTableIdx = 0;
  uint8_t mcs = 9;
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

    // TODO: Schedule broadcast the SIB1

  }

    printf("--- Schedule_nr_sib1: End\n\n\n");

}


