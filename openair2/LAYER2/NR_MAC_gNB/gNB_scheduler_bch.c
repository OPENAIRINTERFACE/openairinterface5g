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

#include "GNB_APP/RRC_nr_paramsvalues.h"
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
extern uint8_t SSB_Table[38];

void schedule_nr_mib(module_id_t module_idP, frame_t frameP, sub_frame_t slotP, uint8_t slots_per_frame){

  gNB_MAC_INST *gNB = RC.nrmac[module_idP];
  NR_COMMON_channels_t *cc;
  
  nfapi_nr_dl_tti_request_t      *dl_tti_request;
  nfapi_nr_dl_tti_request_body_t *dl_req;
  nfapi_nr_dl_tti_request_pdu_t  *dl_config_pdu;

  int mib_sdu_length;
  int CC_id;

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    cc = &gNB->common_channels[CC_id];
    const long band = *cc->ServingCellConfigCommon->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0];
    const uint32_t ssb_offset0 = *cc->ServingCellConfigCommon->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencySSB - cc->ServingCellConfigCommon->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA;
    int ratio;
    switch (*cc->ServingCellConfigCommon->ssbSubcarrierSpacing) {
    case NR_SubcarrierSpacing_kHz15:
      AssertFatal(band <= 79,
                  "Band %ld is not possible for SSB with 15 kHz SCS\n",
                  band);
      if (band < 77)  // below 3GHz
        ratio = 3;    // NRARFCN step is 5 kHz
      else
        ratio = 1;  // NRARFCN step is 15 kHz
      break;
    case NR_SubcarrierSpacing_kHz30:
      AssertFatal(band <= 79,
                  "Band %ld is not possible for SSB with 15 kHz SCS\n",
                  band);
      if (band < 77)  // below 3GHz
        ratio = 6;    // NRARFCN step is 5 kHz
      else
        ratio = 2;  // NRARFCN step is 15 kHz
      break;
    case NR_SubcarrierSpacing_kHz120:
      AssertFatal(band >= 257,
                  "Band %ld is not possible for SSB with 120 kHz SCS\n",
                  band);
      ratio = 2;  // NRARFCN step is 15 kHz
      break;
    case NR_SubcarrierSpacing_kHz240:
      AssertFatal(band >= 257,
                  "Band %ld is not possible for SSB with 240 kHz SCS\n",
                  band);
      ratio = 4;  // NRARFCN step is 15 kHz
      break;
    default:
      AssertFatal(1 == 0, "SCS %ld not allowed for SSB \n",
                  *cc->ServingCellConfigCommon->ssbSubcarrierSpacing);
    }

    // scheduling MIB every 8 frames, PHY repeats it in between
    if((slotP == 0) && (frameP & 7) == 0) {
      dl_tti_request = &gNB->DL_req[CC_id];
      dl_req = &dl_tti_request->dl_tti_request_body;

      mib_sdu_length = mac_rrc_nr_data_req(module_idP, CC_id, frameP, MIBCH, 1, &cc->MIB_pdu.payload[0]);

      LOG_D(MAC, "Frame %d, slot %d: BCH PDU length %d\n", frameP, slotP, mib_sdu_length);

      if (mib_sdu_length > 0) {
        LOG_D(MAC,
              "Frame %d, slot %d: Adding BCH PDU in position %d (length %d)\n",
              frameP,
              slotP,
              dl_req->nPDUs,
              mib_sdu_length);

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

        const nfapi_nr_config_request_scf_t *cfg = &RC.nrmac[module_idP]->config[0];
        dl_config_pdu->ssb_pdu.ssb_pdu_rel15.SsbSubcarrierOffset = cfg->ssb_table.ssb_subcarrier_offset.value; //kSSB
        dl_config_pdu->ssb_pdu.ssb_pdu_rel15.ssbOffsetPointA     = ssb_offset0/(ratio*12) - 10; // absoluteFrequencySSB is the center of SSB
        dl_config_pdu->ssb_pdu.ssb_pdu_rel15.bchPayloadFlag      = 1;
        dl_config_pdu->ssb_pdu.ssb_pdu_rel15.bchPayload          = (*(uint32_t*)cc->MIB_pdu.payload) & ((1<<24)-1);
        dl_req->nPDUs++;
      }

      // Get type0_PDCCH_CSS_config parameters
      NR_DL_FRAME_PARMS *frame_parms = &RC.gNB[module_idP]->frame_parms;
      NR_MIB_t *mib = RC.nrrrc[module_idP]->carrier.mib.message.choice.mib;
      uint8_t gNB_xtra_byte = 0;
      for (int i = 0; i < 8; i++) {
        gNB_xtra_byte |= ((RC.gNB[module_idP]->pbch.pbch_a >> (31 - i)) & 1) << (7 - i);
      }
      get_type0_PDCCH_CSS_config_parameters(&gNB->type0_PDCCH_CSS_config, mib, gNB_xtra_byte, frame_parms->Lmax,
                                            frame_parms->ssb_index, frame_parms->ssb_start_subcarrier/NR_NB_SC_PER_RB);

    }

    // checking if there is any SSB in slot
    const int abs_slot = (slots_per_frame * frameP) + slotP;
    const int slot_per_period = (slots_per_frame>>1)<<(*cc->ServingCellConfigCommon->ssb_periodicityServingCell);
    int eff_120_slot;
    const BIT_STRING_t *shortBitmap = &cc->ServingCellConfigCommon->ssb_PositionsInBurst->choice.shortBitmap;
    const BIT_STRING_t *mediumBitmap = &cc->ServingCellConfigCommon->ssb_PositionsInBurst->choice.mediumBitmap;
    const BIT_STRING_t *longBitmap = &cc->ServingCellConfigCommon->ssb_PositionsInBurst->choice.longBitmap;
    uint8_t buf = 0;
    switch (cc->ServingCellConfigCommon->ssb_PositionsInBurst->present) {
      case 1:
        // presence of ssbs possible in the first 2 slots of ssb period
        if ((abs_slot % slot_per_period) < 2 &&
            (((shortBitmap->buf[0]) >> (6 - (slotP << 1))) & 3) != 0)
          fill_ssb_vrb_map(cc, (ssb_offset0 / (ratio * 12) - 10), CC_id);
        break;
      case 2:
        // presence of ssbs possible in the first 4 slots of ssb period
        if ((abs_slot % slot_per_period) < 4 &&
            (((mediumBitmap->buf[0]) >> (6 - (slotP << 1))) & 3) != 0)
          fill_ssb_vrb_map(cc, (ssb_offset0 / (ratio * 12) - 10), CC_id);
        break;
      case 3:
        AssertFatal(*cc->ServingCellConfigCommon->ssbSubcarrierSpacing ==
                      NR_SubcarrierSpacing_kHz120,
                    "240kHZ subcarrier spacing currently not supported for SSBs\n");
        if ((abs_slot % slot_per_period) < 8) {
          eff_120_slot = slotP;
          buf = longBitmap->buf[0];
        } else if ((abs_slot % slot_per_period) < 17) {
          eff_120_slot = slotP - 9;
          buf = longBitmap->buf[1];
        } else if ((abs_slot % slot_per_period) < 26) {
          eff_120_slot = slotP - 18;
          buf = longBitmap->buf[2];
        } else if ((abs_slot % slot_per_period) < 35) {
          eff_120_slot = slotP - 27;
          buf = longBitmap->buf[3];
        }
        if (((buf >> (6 - (eff_120_slot << 1))) & 3) != 0)
          fill_ssb_vrb_map(cc, ssb_offset0 / (ratio * 12) - 10, CC_id);
        break;
    default:
      AssertFatal(0,
                  "SSB bitmap size value %d undefined (allowed values 1,2,3)\n",
                  cc->ServingCellConfigCommon->ssb_PositionsInBurst->present);
    }
  }
}


void schedule_nr_SI(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP) {
//----------------------------------------  
}

void fill_ssb_vrb_map (NR_COMMON_channels_t *cc, int rbStart, int CC_id) {
  uint16_t *vrb_map = cc[CC_id].vrb_map;
  for (int rb = 0; rb < 20; rb++)
    vrb_map[rbStart + rb] = 1;
}

void schedule_control_sib1(module_id_t module_id,
                           int CC_id,
                           int time_domain_allocation,
                           uint8_t mcsTableIdx,
                           uint8_t mcs,
                           int num_total_bytes) {

  gNB_MAC_INST *gNB_mac = RC.nrmac[module_id];
  NR_ServingCellConfigCommon_t *servingcellconfigcommon = gNB_mac->common_channels[CC_id].ServingCellConfigCommon;
  uint16_t *vrb_map = RC.nrmac[module_id]->common_channels[CC_id].vrb_map;

  if (gNB_mac->sched_ctrlCommon == NULL){
    gNB_mac->sched_ctrlCommon = calloc(1,sizeof(*gNB_mac->sched_ctrlCommon));
    gNB_mac->sched_ctrlCommon->search_space = calloc(1,sizeof(*gNB_mac->sched_ctrlCommon->search_space));
    gNB_mac->sched_ctrlCommon->active_bwp = calloc(1,sizeof(*gNB_mac->sched_ctrlCommon->active_bwp));
    gNB_mac->sched_ctrlCommon->coreset = calloc(1,sizeof(*gNB_mac->sched_ctrlCommon->coreset));
    gNB_mac->sched_ctrlCommon->active_bwp = calloc(1,sizeof(*gNB_mac->sched_ctrlCommon->active_bwp));
    fill_default_searchSpaceZero(gNB_mac->sched_ctrlCommon->search_space);
    fill_default_coresetZero(gNB_mac->sched_ctrlCommon->coreset,servingcellconfigcommon);
    fill_default_initialDownlinkBWP(gNB_mac->sched_ctrlCommon->active_bwp,servingcellconfigcommon);
  }
  gNB_mac->sched_ctrlCommon->active_bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_AdditionalPosition = NULL;

  gNB_mac->sched_ctrlCommon->time_domain_allocation = time_domain_allocation;
  gNB_mac->sched_ctrlCommon->mcsTableIdx = mcsTableIdx;
  gNB_mac->sched_ctrlCommon->mcs = mcs;
  gNB_mac->sched_ctrlCommon->num_total_bytes = num_total_bytes;

  uint8_t nr_of_candidates;
  find_aggregation_candidates(&gNB_mac->sched_ctrlCommon->aggregation_level, &nr_of_candidates, gNB_mac->sched_ctrlCommon->search_space);

  gNB_mac->sched_ctrlCommon->cce_index = allocate_nr_CCEs(RC.nrmac[module_id],
                                                          gNB_mac->sched_ctrlCommon->active_bwp,
                                                          gNB_mac->sched_ctrlCommon->coreset,
                                                          gNB_mac->sched_ctrlCommon->aggregation_level,
                                                          0,
                                                          0,
                                                          nr_of_candidates);

  if (gNB_mac->sched_ctrlCommon->cce_index < 0) {
    LOG_E(MAC, "%s(): could not find CCE for coreset0\n", __func__);
    return;
  }

  const uint16_t bwpSize = gNB_mac->type0_PDCCH_CSS_config.num_rbs;
  int rbStart = gNB_mac->type0_PDCCH_CSS_config.cset_start_rb;

  // Calculate number of symbols
  struct NR_PDSCH_TimeDomainResourceAllocationList *tdaList = gNB_mac->sched_ctrlCommon->active_bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
  const int startSymbolAndLength = tdaList->list.array[gNB_mac->sched_ctrlCommon->time_domain_allocation]->startSymbolAndLength;
  int startSymbolIndex, nrOfSymbols;
  SLIV2SL(startSymbolAndLength, &startSymbolIndex, &nrOfSymbols);

  if (nrOfSymbols == 2) {
    gNB_mac->sched_ctrlCommon->numDmrsCdmGrpsNoData = 1;
  } else {
    gNB_mac->sched_ctrlCommon->numDmrsCdmGrpsNoData = 2;
  }

  // Calculate number of PRB_DMRS
  uint8_t N_PRB_DMRS = gNB_mac->sched_ctrlCommon->numDmrsCdmGrpsNoData * 6;
  uint16_t dlDmrsSymbPos = fill_dmrs_mask(gNB_mac->sched_ctrlCommon->active_bwp->bwp_Dedicated->pdsch_Config->choice.setup, gNB_mac->common_channels->ServingCellConfigCommon->dmrs_TypeA_Position, startSymbolIndex+nrOfSymbols);
  uint16_t dmrs_length = get_num_dmrs(dlDmrsSymbPos);

  int rbSize = 0;
  uint32_t TBS = 0;
  do {
    rbSize++;
    TBS = nr_compute_tbs(nr_get_Qm_dl(gNB_mac->sched_ctrlCommon->mcs, gNB_mac->sched_ctrlCommon->mcsTableIdx),
                         nr_get_code_rate_dl(gNB_mac->sched_ctrlCommon->mcs, gNB_mac->sched_ctrlCommon->mcsTableIdx),
                         rbSize, nrOfSymbols, N_PRB_DMRS * dmrs_length,0, 0,1) >> 3;

    // FIXME: Something is not working in nr_modulation() for TBS = 84, so we need to skip it
  } while ( (rbStart + rbSize < bwpSize && !vrb_map[rbStart + rbSize] && TBS < gNB_mac->sched_ctrlCommon->num_total_bytes) || (TBS==84) );

  gNB_mac->sched_ctrlCommon->rbSize = rbSize;
  gNB_mac->sched_ctrlCommon->rbStart = 0;

  LOG_D(MAC,"SLIV = %i\n", startSymbolAndLength);
  LOG_D(MAC,"startSymbolIndex = %i\n", startSymbolIndex);
  LOG_D(MAC,"nrOfSymbols = %i\n", nrOfSymbols);
  LOG_D(MAC,"rbSize = %i\n", gNB_mac->sched_ctrlCommon->rbSize);
  LOG_D(MAC,"TBS = %i\n", TBS);

  // Mark the corresponding RBs as used
  for (int rb = 0; rb < gNB_mac->sched_ctrlCommon->rbSize; rb++) {
    vrb_map[rb + rbStart] = 1;
  }
}

void nr_fill_nfapi_dl_sib1_pdu(int Mod_idP,
                               nfapi_nr_dl_tti_request_body_t *dl_req,
                               uint32_t TBS,
                               int StartSymbolIndex,
                               int NrOfSymbols) {

  gNB_MAC_INST *gNB_mac = RC.nrmac[Mod_idP];
  NR_COMMON_channels_t *cc = gNB_mac->common_channels;
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
  NR_CellGroupConfig_t *secondaryCellGroup = gNB_mac->secondaryCellGroupCommon;
  NR_BWP_Downlink_t *bwp = gNB_mac->sched_ctrlCommon->active_bwp;

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

  pdcch_pdu_rel15->CoreSetType = NFAPI_NR_CSET_CONFIG_MIB_SIB1;

  pdsch_pdu_rel15->pduBitmap = 0;
  pdsch_pdu_rel15->rnti = SI_RNTI;
  pdsch_pdu_rel15->pduIndex = gNB_mac->pdu_index[0]++;

  pdsch_pdu_rel15->BWPSize  = gNB_mac->type0_PDCCH_CSS_config.num_rbs;
  pdsch_pdu_rel15->BWPStart = gNB_mac->type0_PDCCH_CSS_config.cset_start_rb;

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
  pdsch_pdu_rel15->refPoint = 1;
  pdsch_pdu_rel15->dmrsConfigType = gNB_mac->sched_ctrlCommon->active_bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type == NULL ? 0 : 1;
  pdsch_pdu_rel15->dlDmrsScramblingId = *scc->physCellId;
  pdsch_pdu_rel15->SCID = 0;
  pdsch_pdu_rel15->numDmrsCdmGrpsNoData = gNB_mac->sched_ctrlCommon->numDmrsCdmGrpsNoData;
  pdsch_pdu_rel15->dmrsPorts = 1;
  pdsch_pdu_rel15->resourceAlloc = 1;
  pdsch_pdu_rel15->rbStart = gNB_mac->sched_ctrlCommon->rbStart;
  pdsch_pdu_rel15->rbSize = gNB_mac->sched_ctrlCommon->rbSize;
  pdsch_pdu_rel15->VRBtoPRBMapping = 0;
  pdsch_pdu_rel15->qamModOrder[0] = nr_get_Qm_dl(gNB_mac->sched_ctrlCommon->mcs, gNB_mac->sched_ctrlCommon->mcsTableIdx);
  pdsch_pdu_rel15->TBSize[0] = TBS;
  pdsch_pdu_rel15->mcsTable[0] = gNB_mac->sched_ctrlCommon->mcsTableIdx;
  pdsch_pdu_rel15->StartSymbolIndex = StartSymbolIndex;
  pdsch_pdu_rel15->NrOfSymbols = NrOfSymbols;

  pdsch_pdu_rel15->dlDmrsSymbPos = fill_dmrs_mask(bwp->bwp_Dedicated->pdsch_Config->choice.setup, scc->dmrs_TypeA_Position, pdsch_pdu_rel15->StartSymbolIndex+pdsch_pdu_rel15->NrOfSymbols);

  LOG_D(MAC,"dlDmrsSymbPos = 0x%x\n", pdsch_pdu_rel15->dlDmrsSymbPos);

  dci_pdu_rel15_t dci_pdu_rel15;
  memset(&dci_pdu_rel15, 0, sizeof(dci_pdu_rel15_t));

  dci_pdu_rel15.bwp_indicator.val = gNB_mac->sched_ctrlCommon->active_bwp->bwp_Id;

  // frequency domain assignment
  dci_pdu_rel15.frequency_domain_assignment.val = PRBalloc_to_locationandbandwidth0(
      pdsch_pdu_rel15->rbSize, pdsch_pdu_rel15->rbStart, gNB_mac->type0_PDCCH_CSS_config.num_rbs);

  dci_pdu_rel15.time_domain_assignment.val = gNB_mac->sched_ctrlCommon->time_domain_allocation;
  dci_pdu_rel15.mcs = gNB_mac->sched_ctrlCommon->mcs;
  dci_pdu_rel15.rv = pdsch_pdu_rel15->rvIndex[0];
  dci_pdu_rel15.harq_pid = 0;
  dci_pdu_rel15.ndi = 0;
  dci_pdu_rel15.dai[0].val = 0;
  dci_pdu_rel15.tpc = 0; // table 7.2.1-1 in 38.213
  dci_pdu_rel15.pucch_resource_indicator = 0;
  dci_pdu_rel15.pdsch_to_harq_feedback_timing_indicator.val = 0;
  dci_pdu_rel15.antenna_ports.val = 0;
  dci_pdu_rel15.dmrs_sequence_initialization.val = pdsch_pdu_rel15->SCID;

  nr_configure_pdcch(gNB_mac,
                     pdcch_pdu_rel15,
                     SI_RNTI,
                     gNB_mac->sched_ctrlCommon->search_space,
                     gNB_mac->sched_ctrlCommon->coreset,
                     scc,
                     bwp,
                     gNB_mac->sched_ctrlCommon->aggregation_level,
                     gNB_mac->sched_ctrlCommon->cce_index);

  int dci_format = NR_DL_DCI_FORMAT_1_0;
  int rnti_type = NR_RNTI_SI;

  fill_dci_pdu_rel15(scc,
                     secondaryCellGroup,
                     &pdcch_pdu_rel15->dci_pdu[pdcch_pdu_rel15->numDlDci - 1],
                     &dci_pdu_rel15,
                     dci_format,
                     rnti_type,
                     pdsch_pdu_rel15->BWPSize,
                     gNB_mac->sched_ctrlCommon->active_bwp->bwp_Id);

  dl_req->nPDUs += 2;

  LOG_D(MAC,"BWPSize: %i\n", pdcch_pdu_rel15->BWPSize);
  LOG_D(MAC,"BWPStart: %i\n", pdcch_pdu_rel15->BWPStart);
  LOG_D(MAC,"SubcarrierSpacing: %i\n", pdcch_pdu_rel15->SubcarrierSpacing);
  LOG_D(MAC,"CyclicPrefix: %i\n", pdcch_pdu_rel15->CyclicPrefix);
  LOG_D(MAC,"StartSymbolIndex: %i\n", pdcch_pdu_rel15->StartSymbolIndex);
  LOG_D(MAC,"DurationSymbols: %i\n", pdcch_pdu_rel15->DurationSymbols);
  for(int n=0;n<6;n++) LOG_D(MAC,"FreqDomainResource[%i]: %x\n",n, pdcch_pdu_rel15->FreqDomainResource[n]);
  LOG_D(MAC,"CceRegMappingType: %i\n", pdcch_pdu_rel15->CceRegMappingType);
  LOG_D(MAC,"RegBundleSize: %i\n", pdcch_pdu_rel15->RegBundleSize);
  LOG_D(MAC,"InterleaverSize: %i\n", pdcch_pdu_rel15->InterleaverSize);
  LOG_D(MAC,"CoreSetType: %i\n", pdcch_pdu_rel15->CoreSetType);
  LOG_D(MAC,"ShiftIndex: %i\n", pdcch_pdu_rel15->ShiftIndex);
  LOG_D(MAC,"precoderGranularity: %i\n", pdcch_pdu_rel15->precoderGranularity);
  LOG_D(MAC,"numDlDci: %i\n", pdcch_pdu_rel15->numDlDci);

}

void schedule_nr_sib1(module_id_t module_idP, frame_t frameP, sub_frame_t slotP) {

  LOG_D(MAC,"Schedule_nr_sib1: frameP = %i, slotP = %i\n", frameP, slotP);

  // TODO: Get these values from RRC
  const int CC_id = 0;
  int time_domain_allocation = 0;
  uint8_t mcsTableIdx = 0;
  uint8_t mcs = 6;

  gNB_MAC_INST *gNB_mac = RC.nrmac[module_idP];

  if( (frameP%2 == gNB_mac->type0_PDCCH_CSS_config.sfn_c) && (slotP == gNB_mac->type0_PDCCH_CSS_config.n_0) && (gNB_mac->type0_PDCCH_CSS_config.num_rbs > 0) ) {

    LOG_D(MAC,"> SIB1 transmission\n");

    // Get SIB1
    uint8_t sib1_payload[NR_MAX_SIB_LENGTH/8];
    uint8_t sib1_sdu_length = mac_rrc_nr_data_req(module_idP, CC_id, frameP, BCCH, 1, sib1_payload);
    LOG_D(MAC,"sib1_sdu_length = %i\n", sib1_sdu_length);
    LOG_D(MAC,"SIB1: \n");
    for (int i=0;i<sib1_sdu_length;i++) LOG_D(MAC,"byte %d : %x\n",i,((uint8_t*)sib1_payload)[i]);

    // Configure sched_ctrlCommon for SIB1
    schedule_control_sib1(module_idP, CC_id, time_domain_allocation, mcsTableIdx, mcs, sib1_sdu_length);

    // Calculate number of symbols
    int startSymbolIndex, nrOfSymbols;
    struct NR_PDSCH_TimeDomainResourceAllocationList *tdaList = gNB_mac->sched_ctrlCommon->active_bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
    const int startSymbolAndLength = tdaList->list.array[gNB_mac->sched_ctrlCommon->time_domain_allocation]->startSymbolAndLength;
    SLIV2SL(startSymbolAndLength, &startSymbolIndex, &nrOfSymbols);

    // Calculate number of PRB_DMRS
    uint8_t N_PRB_DMRS = gNB_mac->sched_ctrlCommon->numDmrsCdmGrpsNoData * 6;
    uint16_t dlDmrsSymbPos = fill_dmrs_mask(gNB_mac->sched_ctrlCommon->active_bwp->bwp_Dedicated->pdsch_Config->choice.setup, gNB_mac->common_channels->ServingCellConfigCommon->dmrs_TypeA_Position, startSymbolIndex+nrOfSymbols);
    uint16_t dmrs_length = get_num_dmrs(dlDmrsSymbPos);

    const uint32_t TBS = nr_compute_tbs(nr_get_Qm_dl(gNB_mac->sched_ctrlCommon->mcs, gNB_mac->sched_ctrlCommon->mcsTableIdx),
                                        nr_get_code_rate_dl(gNB_mac->sched_ctrlCommon->mcs, gNB_mac->sched_ctrlCommon->mcsTableIdx),
                                        gNB_mac->sched_ctrlCommon->rbSize, nrOfSymbols, N_PRB_DMRS * dmrs_length,0 ,0 ,1 ) >> 3;

    nfapi_nr_dl_tti_request_body_t *dl_req = &gNB_mac->DL_req[CC_id].dl_tti_request_body;
    nr_fill_nfapi_dl_sib1_pdu(module_idP, dl_req, TBS, startSymbolIndex, nrOfSymbols);

    const int ntx_req = gNB_mac->TX_req[CC_id].Number_of_PDUs;
    nfapi_nr_pdu_t *tx_req = &gNB_mac->TX_req[CC_id].pdu_list[ntx_req];

    // Data to be transmitted
    bzero(tx_req->TLVs[0].value.direct,MAX_NR_DLSCH_PAYLOAD_BYTES);
    memcpy(tx_req->TLVs[0].value.direct, sib1_payload, sib1_sdu_length);

    tx_req->PDU_length = TBS;
    tx_req->PDU_index  = gNB_mac->pdu_index[0]++;
    tx_req->num_TLV = 1;
    tx_req->TLVs[0].length = TBS + 2;
    gNB_mac->TX_req[CC_id].Number_of_PDUs++;
    gNB_mac->TX_req[CC_id].SFN = frameP;
    gNB_mac->TX_req[CC_id].Slot = slotP;
  }
}
