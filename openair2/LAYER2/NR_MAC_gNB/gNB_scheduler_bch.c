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

#include "assertions.h"
#include "NR_MAC_gNB/nr_mac_gNB.h"
#include "NR_MAC_gNB/mac_proto.h"
#include "NR_MAC_COMMON/nr_mac_extern.h"
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"
#include "RRC/NR/nr_rrc_extern.h"
#include "common/utils/nr/nr_common.h"


//#include "LAYER2/MAC/pre_processor.c"
#include "pdcp.h"

#include "intertask_interface.h"

#define ENABLE_MAC_PAYLOAD_DEBUG
#define DEBUG_eNB_SCHEDULER 1

#include "common/ran_context.h"

extern RAN_CONTEXT_t RC;

void schedule_nr_mib(module_id_t module_idP, frame_t frameP, sub_frame_t slotP){

  gNB_MAC_INST *gNB = RC.nrmac[module_idP];
  NR_COMMON_channels_t *cc;
  
  nfapi_nr_dl_tti_request_t      *dl_tti_request;
  nfapi_nr_dl_tti_request_body_t *dl_req;
  nfapi_nr_dl_tti_request_pdu_t  *dl_config_pdu;

  int mib_sdu_length;
  int CC_id;

  AssertFatal(slotP == 0, "Subframe must be 0\n");
  AssertFatal((frameP & 7) == 0, "Frame must be a multiple of 8\n");

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {

    dl_tti_request = &gNB->DL_req[CC_id];
    dl_req = &dl_tti_request->dl_tti_request_body;
    cc = &gNB->common_channels[CC_id];

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
}

void schedule_nr_sib1(module_id_t module_idP, frame_t frameP, sub_frame_t slotP) {

  printf("\n\n--- Schedule_nr_sib1: Start\n");

  // Get type0_PDCCH_CSS_config parameters
  PHY_VARS_gNB *gNB = RC.gNB[module_idP];
  gNB_MAC_INST *mac = RC.nrmac[module_idP];
  NR_DL_FRAME_PARMS *frame_parms = &gNB->frame_parms;
  NR_MIB_t *mib = RC.nrrrc[module_idP]->carrier.mib.message.choice.mib;
  uint8_t gNB_xtra_byte = 0;
  for (int i = 0; i < 8; i++) {
    gNB_xtra_byte |= ((gNB->pbch.pbch_a >> (31 - i)) & 1) << (7 - i);
  }
  get_type0_PDCCH_CSS_config_parameters(&mac->type0_PDCCH_CSS_config, mib, gNB_xtra_byte, frame_parms->Lmax,
                                        frame_parms->ssb_index);


  // Get SIB1
  int CC_id = 0;
  uint8_t sib1_payload[100];
  uint8_t sib1_sdu_length = mac_rrc_nr_data_req(module_idP, CC_id, frameP, BCCH, 1, sib1_payload);
  printf("sib1_sdu_length = %i\n", sib1_sdu_length);
  for(int i = 0; i<sib1_sdu_length; i++) {
    printf("%i ", sib1_payload[i]);

  }
  printf("\n");



  // Schedule SIB1

  int UE_id = 0;
  int mcsIndex = 0;
  int startSymbolAndLength = 0;
  int NrOfSymbols = 14;
  int StartSymbolIndex_tmp;
  int NrOfSymbols_tmp;
  int StartSymbolIndex = -1;
  int time_domain_assignment = 0;
  int dci10_bw;
  long locationAndBandwidth;
  int target_ss = NR_SearchSpace__searchSpaceType_PR_ue_Specific;

  NR_COMMON_channels_t *cc          = &mac->common_channels[CC_id];
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;

  NR_UE_info_t *UE_info                     = &RC.nrmac[module_idP]->UE_info;
  NR_CellGroupConfig_t *secondaryCellGroup  = UE_info->secondaryCellGroup[UE_id];
  NR_UE_sched_ctrl_t   *sched_ctrl          = &UE_info->UE_sched_ctrl[UE_id];
  long bwp_Id                               = sched_ctrl->active_bwp->bwp_Id;

  nfapi_nr_dl_tti_request_body_t  *dl_req   = &mac->DL_req[CC_id].dl_tti_request_body;
  nfapi_nr_pdu_t                  *tx_req   = &mac->TX_req[CC_id].pdu_list[mac->TX_req[CC_id].Number_of_PDUs];

  NR_SearchSpace_t *ss;

  nfapi_nr_dl_tti_request_pdu_t   *dl_tti_pdcch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs];
  memset((void*)dl_tti_pdcch_pdu,0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
  dl_tti_pdcch_pdu->PDUType = NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE;
  dl_tti_pdcch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdcch_pdu));

  nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdsch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs+1];
  memset((void *)dl_tti_pdsch_pdu,0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
  dl_tti_pdsch_pdu->PDUType = NFAPI_NR_DL_TTI_PDSCH_PDU_TYPE;
  dl_tti_pdsch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdsch_pdu));

  nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15 = &dl_tti_pdcch_pdu->pdcch_pdu.pdcch_pdu_rel15;
  nfapi_nr_dl_tti_pdsch_pdu_rel15_t *pdsch_pdu_rel15 = &dl_tti_pdsch_pdu->pdsch_pdu.pdsch_pdu_rel15;

  NR_BWP_Downlink_t *bwp    = secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[bwp_Id - 1];
  NR_BWP_Uplink_t   *ubwp   = secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[bwp_Id-1];

  pdsch_pdu_rel15->pduBitmap = 0;
  pdsch_pdu_rel15->rnti = 0; //UE_info->rnti[UE_id];
  pdsch_pdu_rel15->pduIndex = 0; //mac->pdu_index[0]++;

  pdsch_pdu_rel15->BWPSize  = NRRIV2BW(bwp->bwp_Common->genericParameters.locationAndBandwidth,275);
  pdsch_pdu_rel15->BWPStart = NRRIV2PRBOFFSET(bwp->bwp_Common->genericParameters.locationAndBandwidth,275);
  pdsch_pdu_rel15->SubcarrierSpacing = bwp->bwp_Common->genericParameters.subcarrierSpacing;
  pdsch_pdu_rel15->CyclicPrefix = 0;
  pdsch_pdu_rel15->NrOfCodewords = 1;
  pdsch_pdu_rel15->targetCodeRate[0] = nr_get_code_rate_dl(mcsIndex,0);
  pdsch_pdu_rel15->qamModOrder[0] = 2;
  pdsch_pdu_rel15->mcsIndex[0] = mcsIndex;
  if (bwp->bwp_Dedicated->pdsch_Config->choice.setup->mcs_Table == NULL)
    pdsch_pdu_rel15->mcsTable[0] = 0;
  else{
    if (*bwp->bwp_Dedicated->pdsch_Config->choice.setup->mcs_Table == 0)
      pdsch_pdu_rel15->mcsTable[0] = 1;
    else
      pdsch_pdu_rel15->mcsTable[0] = 2;
  }
  pdsch_pdu_rel15->rvIndex[0] = 0;
  pdsch_pdu_rel15->dataScramblingId = *scc->physCellId;
  pdsch_pdu_rel15->nrOfLayers = 1;
  pdsch_pdu_rel15->transmissionScheme = 0;
  pdsch_pdu_rel15->refPoint = 0;
  pdsch_pdu_rel15->dmrsConfigType = 0;
  pdsch_pdu_rel15->dlDmrsScramblingId = *scc->physCellId;
  pdsch_pdu_rel15->SCID = 0;
  pdsch_pdu_rel15->numDmrsCdmGrpsNoData = 2;
  pdsch_pdu_rel15->dmrsPorts = 1;
  pdsch_pdu_rel15->resourceAlloc = 1;
  pdsch_pdu_rel15->rbStart = 0;
  pdsch_pdu_rel15->rbSize = 6;
  pdsch_pdu_rel15->VRBtoPRBMapping = 0; // non interleaved


  for (int i=0; i<bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.count; i++) {
    startSymbolAndLength = bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList->list.array[i]->startSymbolAndLength;
    SLIV2SL(startSymbolAndLength, &StartSymbolIndex_tmp, &NrOfSymbols_tmp);
    if (NrOfSymbols_tmp < NrOfSymbols) {
      NrOfSymbols = NrOfSymbols_tmp;
      StartSymbolIndex = StartSymbolIndex_tmp;
      time_domain_assignment = i; // this is short PDSCH added to the config to fit mixed slot
    }
  }

  pdsch_pdu_rel15->StartSymbolIndex = StartSymbolIndex;
  pdsch_pdu_rel15->NrOfSymbols      = NrOfSymbols;
  pdsch_pdu_rel15->dlDmrsSymbPos    = fill_dmrs_mask(NULL, scc->dmrs_TypeA_Position, NrOfSymbols);

  locationAndBandwidth = scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth;
  dci10_bw = NRRIV2BW(locationAndBandwidth,275);

  dci_pdu_rel15_t dci_pdu_rel15[MAX_DCI_CORESET];
  dci_pdu_rel15[0].frequency_domain_assignment.val = PRBalloc_to_locationandbandwidth0(pdsch_pdu_rel15->rbSize,
                                                                                       pdsch_pdu_rel15->rbStart,dci10_bw);
  dci_pdu_rel15[0].time_domain_assignment.val = time_domain_assignment;
  dci_pdu_rel15[0].vrb_to_prb_mapping.val = 0;
  dci_pdu_rel15[0].mcs = pdsch_pdu_rel15->mcsIndex[0];
  dci_pdu_rel15[0].tb_scaling = 0;

  int found = 0;
  for ( int i=0; i<bwp->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList->list.count; i++) {
    ss=bwp->bwp_Dedicated->pdcch_Config->choice.setup->searchSpacesToAddModList->list.array[i];
    if (ss->searchSpaceType->present == target_ss) {
      found=1;
      break;
    }
  }

  uint8_t nr_of_candidates, aggregation_level;
  find_aggregation_candidates(&aggregation_level, &nr_of_candidates, ss);
  NR_ControlResourceSet_t *coreset = get_coreset(bwp, ss, 0);
  int CCEIndex = allocate_nr_CCEs(mac,bwp,coreset,aggregation_level,0,0);

  if (CCEIndex < 0) {
    LOG_E(MAC, "%s(): cannot find free CCE for RNTI %04x!\n", __func__, pdsch_pdu_rel15->rnti);
    return;
  }

  nr_configure_pdcch(mac,
                     pdcch_pdu_rel15,
                     pdsch_pdu_rel15->rnti,
                     ss,
                     coreset,
                     scc,
                     bwp,
                     aggregation_level,
                     CCEIndex);

  LOG_I(MAC, "Frame %d: Subframe %d : Adding common DL DCI for RNTI %x\n", frameP, slotP, pdsch_pdu_rel15->rnti);

  int dci_formats[2], rnti_types[2];
  dci_formats[0] = NR_DL_DCI_FORMAT_1_0;
  rnti_types[0] = NR_RNTI_SI;
  fill_dci_pdu_rel15(scc,secondaryCellGroup,pdcch_pdu_rel15, &dci_pdu_rel15[0], dci_formats, rnti_types,dci10_bw,bwp_Id);

  dl_req->nPDUs+=2;

  int x_Overhead = 0;
  nr_get_tbs_dl(&dl_tti_pdsch_pdu->pdsch_pdu, x_Overhead, pdsch_pdu_rel15->numDmrsCdmGrpsNoData, dci_pdu_rel15[0].tb_scaling);

  // DL TX request
  tx_req->PDU_length = pdsch_pdu_rel15->TBSize[0];
  tx_req->PDU_index = mac->pdu_index[CC_id]++;
  tx_req->num_TLV = 1;
  tx_req->TLVs[0].length = sib1_sdu_length;
  mac->TX_req[CC_id].SFN = frameP;
  mac->TX_req[CC_id].Number_of_PDUs++;
  mac->TX_req[CC_id].Slot = slotP;
  memcpy((void*)&tx_req->TLVs[0].value.direct[0], (void*)sib1_payload, sib1_sdu_length);

  // Mark the corresponding RBs as used
  uint8_t *vrb_map = cc[CC_id].vrb_map;
  for (int rb = 0; rb < pdsch_pdu_rel15->rbSize; rb++) {
    vrb_map[rb + pdsch_pdu_rel15->rbStart] = 1;
  }


  printf("--- Schedule_nr_sib1: End\n\n\n");
}


