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

/*! \file     gNB_scheduler_RA.c
 * \brief     primitives used for random access
 * \author    Guido Casati
 * \date      2019
 * \email:    guido.casati@iis.fraunhofer.de
 * \version
 */

#include "platform_types.h"

/* MAC */
#include "nr_mac_gNB.h"
#include "NR_MAC_gNB/mac_proto.h"
#include "NR_MAC_COMMON/nr_mac_extern.h"

/* Utils */
#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "common/utils/nr/nr_common.h"
#include "UTIL/OPT/opt.h"
#include "SIMULATION/TOOLS/sim.h" // for taus

extern RAN_CONTEXT_t RC;
extern const uint8_t nr_slots_per_frame[5];

uint8_t DELTA[4]= {2,3,4,6};

void schedule_nr_prach(module_id_t module_idP, frame_t frameP, sub_frame_t slotP) {

  gNB_MAC_INST *gNB = RC.nrmac[module_idP];
  NR_COMMON_channels_t *cc = gNB->common_channels;
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
  nfapi_nr_ul_tti_request_t *UL_tti_req = &RC.nrmac[module_idP]->UL_tti_req[0];

  uint8_t config_index = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.prach_ConfigurationIndex;
  uint8_t mu,N_dur,N_t_slot,start_symbol;
  uint16_t format;

  if (scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing)
    mu = *scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing;
  else
    mu = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;

  // prach is scheduled according to configuration index and tables 6.3.3.2.2 to 6.3.3.2.4
  if ( get_nr_prach_info_from_index(config_index,
                                    (int)frameP,
                                    (int)slotP,
                                    scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA,
                                    mu,
                                    cc->frame_type,
                                    &format,
                                    &start_symbol,
                                    &N_t_slot,
                                    &N_dur) ) {

    int fdm = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.msg1_FDM;
    uint16_t format0 = format&0xff;      // first column of format from table
    uint16_t format1 = (format>>8)&0xff; // second column of format from table

    UL_tti_req->SFN = frameP;
    UL_tti_req->Slot = slotP;
    for (int n=0; n<(1<<fdm); n++) { // one structure per frequency domain occasion
      UL_tti_req->pdus_list[UL_tti_req->n_pdus].pdu_type = NFAPI_NR_UL_CONFIG_PRACH_PDU_TYPE;
      UL_tti_req->pdus_list[UL_tti_req->n_pdus].pdu_size = sizeof(nfapi_nr_prach_pdu_t);
      nfapi_nr_prach_pdu_t  *prach_pdu = &UL_tti_req->pdus_list[UL_tti_req->n_pdus].prach_pdu;
      memset(prach_pdu,0,sizeof(nfapi_nr_prach_pdu_t));
      UL_tti_req->n_pdus+=1;

      // filling the prach fapi structure
      prach_pdu->phys_cell_id = *scc->physCellId;
      prach_pdu->num_prach_ocas = N_t_slot;
      prach_pdu->prach_start_symbol = start_symbol;
      prach_pdu->num_ra = n;
      prach_pdu->num_cs = get_NCS(scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.zeroCorrelationZoneConfig,
                                  format0,
                                  scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->restrictedSetConfig);
      // SCF PRACH PDU format field does not consider A1/B1 etc. possibilities
      // We added 9 = A1/B1 10 = A2/B2 11 A3/B3
      if (format1!=0xff) {
        switch(format0) {
          case 0xa1:
            prach_pdu->prach_format = 9;
            break;
          case 0xa2:
            prach_pdu->prach_format = 10;
            break;
          case 0xa3:
            prach_pdu->prach_format = 11;
            break;
        default:
          AssertFatal(1==0,"Only formats A1/B1 A2/B2 A3/B3 are valid for dual format");
        }
      }
      else{
        switch(format0) {
          case 0xa1:
            prach_pdu->prach_format = 0;
            break;
          case 0xa2:
            prach_pdu->prach_format = 1;
            break;
          case 0xa3:
            prach_pdu->prach_format = 2;
            break;
          case 0xb1:
            prach_pdu->prach_format = 3;
            break;
          case 0xb2:
            prach_pdu->prach_format = 4;
            break;
          case 0xb3:
            prach_pdu->prach_format = 5;
            break;
          case 0xb4:
            prach_pdu->prach_format = 6;
            break;
          case 0xc0:
            prach_pdu->prach_format = 7;
            break;
          case 0xc2:
            prach_pdu->prach_format = 8;
            break;
          case 0:
            // long formats are handled @ PHY
            break;
          case 1:
            // long formats are handled @ PHY
            break;
          case 2:
            // long formats are handled @ PHY
            break;
          case 3:
            // long formats are handled @ PHY
            break;
        default:
          AssertFatal(1==0,"Invalid PRACH format");
        }
      }
    }
  }
}


void nr_schedule_msg2(uint16_t rach_frame, uint16_t rach_slot,
                      uint16_t *msg2_frame, uint16_t *msg2_slot,
                      NR_ServingCellConfigCommon_t *scc,
                      uint16_t monitoring_slot_period,
                      uint16_t monitoring_offset){

  // preferentially we schedule the msg2 in the mixed slot or in the last dl slot
  // if they are allowed by search space configuration

  uint8_t mu = *scc->ssbSubcarrierSpacing;
  uint8_t response_window = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.ra_ResponseWindow;
  uint8_t slot_window;
  // number of mixed slot or of last dl slot if there is no mixed slot
  uint8_t last_dl_slot_period = scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSlots;
  // lenght of tdd period in slots
  uint8_t tdd_period_slot =  scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSlots + scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSlots;
  if (scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSymbols == 0)
    last_dl_slot_period--;
  if ((scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSymbols > 0) || (scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSymbols > 0))
    tdd_period_slot++;

  // computing start of next period
  uint8_t start_next_period = (rach_slot-(rach_slot%tdd_period_slot)+tdd_period_slot)%nr_slots_per_frame[mu];
  *msg2_slot = start_next_period + last_dl_slot_period; // initializing scheduling of slot to next mixed (or last dl) slot
  *msg2_frame = (*msg2_slot>(rach_slot))? rach_frame : (rach_frame +1);

  switch(response_window){
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl1:
      slot_window = 1;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl2:
      slot_window = 2;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl4:
      slot_window = 4;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl8:
      slot_window = 8;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl10:
      slot_window = 10;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl20:
      slot_window = 20;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl40:
      slot_window = 40;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl80:
      slot_window = 80;
      break;
    default:
      AssertFatal(1==0,"Invalid response window value %d\n",response_window);
  }
  AssertFatal(slot_window<=nr_slots_per_frame[mu],"Msg2 response window needs to be lower or equal to 10ms");

  // slot and frame limit to transmit msg2 according to response window
  uint8_t slot_limit = (rach_slot + slot_window)%nr_slots_per_frame[mu];
  //uint8_t frame_limit = (slot_limit>(rach_slot))? rach_frame : (rach_frame +1);


  // go to previous slot if the current scheduled slot is beyond the response window
  // and if the slot is not among the PDCCH monitored ones (38.213 10.1)
  while ((*msg2_slot>slot_limit) || ((*msg2_frame*nr_slots_per_frame[mu]+*msg2_slot-monitoring_offset)%monitoring_slot_period !=0))  {
    if((*msg2_slot%tdd_period_slot) > 0)
      (*msg2_slot)--;
    else
      AssertFatal(1==0,"No available DL slot to schedule msg2 has been found");
  }
}


void nr_initiate_ra_proc(module_id_t module_idP,
                         int CC_id,
                         frame_t frameP,
                         sub_frame_t slotP,
                         uint16_t preamble_index,
                         uint8_t freq_index,
                         uint8_t symbol,
                         int16_t timing_offset){

  uint8_t ul_carrier_id = 0; // 0 for NUL 1 for SUL
  NR_SearchSpace_t *ss;
  int UE_id = 0;
  // ra_rnti from 5.1.3 in 38.321
  uint16_t ra_rnti=1+symbol+(slotP*14)+(freq_index*14*80)+(ul_carrier_id*14*80*8);

  uint16_t msg2_frame, msg2_slot,monitoring_slot_period,monitoring_offset;
  gNB_MAC_INST *nr_mac = RC.nrmac[module_idP];
  NR_UE_list_t *UE_list = &nr_mac->UE_list;
  NR_CellGroupConfig_t *secondaryCellGroup = UE_list->secondaryCellGroup[UE_id];
  NR_COMMON_channels_t *cc = &nr_mac->common_channels[CC_id];
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
  NR_RA_t *ra = &cc->ra[0];

  // if the preamble received correspond to one of the listed
  // the UE sent a RACH either for starting RA procedure or RA procedure failed and UE retries
  int pr_found=0;
  for (int i=0;i<UE_list->preambles[UE_id].num_preambles;i++) {
    if (preamble_index == UE_list->preambles[UE_id].preamble_list[i]) {
      pr_found=1;
      break;
    }
  }
  if (pr_found)
    UE_list->fiveG_connected[UE_id] = false;
  else {
    LOG_E(MAC, "[gNB %d][RAPROC] FAILURE: preamble %d does not correspond to any of the ones in rach_ConfigDedicated for UE_id %d\n",
          module_idP, preamble_index, UE_id);
    return; // if the PRACH preamble does not correspond to any of the ones sent through RRC abort RA proc
  }

  // This should be handled differently when we use the initialBWP for RA
  ra->bwp_id=1;
  NR_BWP_Downlink_t *bwp=secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[ra->bwp_id-1];

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_INITIATE_RA_PROC, 1);

  LOG_I(MAC, "[gNB %d][RAPROC] CC_id %d Frame %d, Slot %d  Initiating RA procedure for preamble index %d\n", module_idP, CC_id, frameP, slotP, preamble_index);

  if (ra->state == RA_IDLE) {
    int loop = 0;
    LOG_D(MAC, "Frame %d, Slot %d: Activating RA process \n", frameP, slotP);
    ra->state = Msg2;
    ra->timing_offset = timing_offset;
    ra->preamble_slot = slotP;

    struct NR_PDCCH_ConfigCommon__commonSearchSpaceList *commonSearchSpaceList = bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->commonSearchSpaceList;
    AssertFatal(commonSearchSpaceList->list.count>0,
	        "common SearchSpace list has 0 elements\n");
    // Common searchspace list
    for (int i=0;i<commonSearchSpaceList->list.count;i++) {
      ss=commonSearchSpaceList->list.array[i];
      if(ss->searchSpaceId == *bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->ra_SearchSpace)
        ra->ra_ss=ss;
    }

    // retrieving ra pdcch monitoring period and offset
    find_monitoring_periodicity_offset_common(ra->ra_ss,
                                              &monitoring_slot_period,
                                              &monitoring_offset);

    nr_schedule_msg2(frameP, slotP, &msg2_frame, &msg2_slot, scc, monitoring_slot_period, monitoring_offset);

    ra->Msg2_frame = msg2_frame;
    ra->Msg2_slot = msg2_slot;

    LOG_D(MAC, "%s() Msg2[%04d%d] SFN/SF:%04d%d\n", __FUNCTION__, ra->Msg2_frame, ra->Msg2_slot, frameP, slotP);

    do {
      ra->rnti = (taus() % 65518) + 1;
      loop++;
    }
    while (loop != 100 && !(find_nr_UE_id(module_idP, ra->rnti) == -1 && ra->rnti >= 1 && ra->rnti <= 65519));
    if (loop == 100) {
      LOG_E(MAC,"%s:%d:%s: [RAPROC] initialisation random access aborted\n", __FILE__, __LINE__, __FUNCTION__);
      abort();
    }

    ra->RA_rnti = ra_rnti;
    ra->preamble_index = preamble_index;
    UE_list->tc_rnti[UE_id] = ra->rnti;

    LOG_I(MAC,"[gNB %d][RAPROC] CC_id %d Frame %d Activating Msg2 generation in frame %d, slot %d using RA rnti %x\n",
      module_idP,
      CC_id,
      frameP,
      ra->Msg2_frame,
      ra->Msg2_slot,
      ra->RA_rnti);

    return;
  }
  LOG_E(MAC, "[gNB %d][RAPROC] FAILURE: CC_id %d Frame %d initiating RA procedure for preamble index %d\n", module_idP, CC_id, frameP, preamble_index);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_INITIATE_RA_PROC, 0);
}

void nr_schedule_RA(module_id_t module_idP, frame_t frameP, sub_frame_t slotP){

  //uint8_t i = 0;
  int CC_id = 0;
  gNB_MAC_INST *mac = RC.nrmac[module_idP];
  NR_COMMON_channels_t *cc = &mac->common_channels[CC_id];
  NR_RA_t *ra = &cc->ra[0];

  start_meas(&mac->schedule_ra);

  //for (i = 0; i < NR_NB_RA_PROC_MAX; i++) {
  LOG_D(MAC,"RA[state:%d]\n",ra->state);
  switch (ra->state){
    case Msg2:
      nr_generate_Msg2(module_idP, CC_id, frameP, slotP);
      break;
    case Msg4:
      //generate_Msg4(module_idP, CC_id, frameP, slotP, ra);
      break;
    case WAIT_Msg4_ACK:
      //check_Msg4_retransmission(module_idP, CC_id, frameP, slotP, ra);
      break;
    default:
    break;
  }
  //}
  stop_meas(&mac->schedule_ra);
}

void nr_get_Msg3alloc(NR_ServingCellConfigCommon_t *scc,
                      NR_BWP_Uplink_t *ubwp,
                      sub_frame_t current_slot,
                      frame_t current_frame,
                      NR_RA_t *ra) {

  // msg3 is schedulend in mixed slot in the following TDD period
  // for now we consider a TBS of 18 bytes

  int mu = ubwp->bwp_Common->genericParameters.subcarrierSpacing;
  int StartSymbolIndex, NrOfSymbols, startSymbolAndLength, temp_slot;
  ra->Msg3_tda_id = 16; // initialization to a value above limit

  for (int i=0; i<ubwp->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.count; i++) {
    startSymbolAndLength = ubwp->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[i]->startSymbolAndLength;
    SLIV2SL(startSymbolAndLength, &StartSymbolIndex, &NrOfSymbols);
    // we want to transmit in the uplink symbols of mixed slot
    if (NrOfSymbols == scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSymbols) {
      ra->Msg3_tda_id = i;
      break;
    }
  }
  AssertFatal(ra->Msg3_tda_id<16,"Unable to find Msg3 time domain allocation in list\n");

  uint8_t k2 = *ubwp->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[ra->Msg3_tda_id]->k2;

  temp_slot = current_slot + k2 + DELTA[mu]; // msg3 slot according to 8.3 in 38.213
  ra->Msg3_slot = temp_slot%nr_slots_per_frame[mu];
  if (nr_slots_per_frame[mu]>temp_slot)
    ra->Msg3_frame = current_frame;
  else
    ra->Msg3_frame = current_frame + (temp_slot/nr_slots_per_frame[mu]);

  ra->msg3_nb_rb = 18;
  ra->msg3_first_rb = 0;
}


void nr_schedule_reception_msg3(module_id_t module_idP, int CC_id, frame_t frameP, sub_frame_t slotP){
  gNB_MAC_INST                                *mac = RC.nrmac[module_idP];
  nfapi_nr_ul_tti_request_t                   *ul_req = &mac->UL_tti_req[0];
  NR_COMMON_channels_t                        *cc = &mac->common_channels[CC_id];
  NR_RA_t                                     *ra = &cc->ra[0];

  if (ra->state == WAIT_Msg3) {
    if ((frameP == ra->Msg3_frame) && (slotP == ra->Msg3_slot) ){
      ul_req->SFN = ra->Msg3_frame;
      ul_req->Slot = ra->Msg3_slot;
      ul_req->pdus_list[ul_req->n_pdus].pdu_type = NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE;
      ul_req->pdus_list[ul_req->n_pdus].pdu_size = sizeof(nfapi_nr_pusch_pdu_t);
      ul_req->pdus_list[ul_req->n_pdus].pusch_pdu = ra->pusch_pdu;
      ul_req->n_pdus+=1;
      ra->state = RA_IDLE;
    }
  }
}

void nr_add_msg3(module_id_t module_idP, int CC_id, frame_t frameP, sub_frame_t slotP){

  gNB_MAC_INST                                   *mac = RC.nrmac[module_idP];
  NR_COMMON_channels_t                            *cc = &mac->common_channels[CC_id];
  NR_ServingCellConfigCommon_t                   *scc = cc->ServingCellConfigCommon;
  NR_RA_t                                         *ra = &cc->ra[0];
  NR_UE_list_t                               *UE_list = &mac->UE_list;
  int UE_id = 0;

  AssertFatal(ra->state != RA_IDLE, "RA is not active for RA %X\n", ra->rnti);

  LOG_D(MAC, "[gNB %d][RAPROC] Frame %d, Subframe %d : CC_id %d RA is active, Msg3 in (%d,%d)\n", module_idP, frameP, slotP, CC_id, ra->Msg3_frame, ra->Msg3_slot);

  nfapi_nr_pusch_pdu_t  *pusch_pdu = &ra->pusch_pdu;
  memset(pusch_pdu, 0, sizeof(nfapi_nr_pusch_pdu_t));


  AssertFatal(UE_list->active[UE_id] >=0,"Cannot find UE_id %d is not active\n", UE_id);

  NR_CellGroupConfig_t *secondaryCellGroup = UE_list->secondaryCellGroup[UE_id];
  AssertFatal(secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count == 1,
    "downlinkBWP_ToAddModList has %d BWP!\n", secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count);
  NR_BWP_Uplink_t *ubwp=secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[ra->bwp_id-1];
  LOG_D(MAC, "Frame %d, Subframe %d Adding Msg3 UL Config Request for (%d,%d) : (%d,%d,%d) for rnti: %d\n",
    frameP,
    slotP,
    ra->Msg3_frame,
    ra->Msg3_slot,
    ra->msg3_nb_rb,
    ra->msg3_first_rb,
    ra->msg3_round,
    ra->rnti);

  int startSymbolAndLength = ubwp->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[ra->Msg3_tda_id]->startSymbolAndLength;
  int start_symbol_index,nr_of_symbols;
  SLIV2SL(startSymbolAndLength, &start_symbol_index, &nr_of_symbols);

  pusch_pdu->pdu_bit_map = PUSCH_PDU_BITMAP_PUSCH_DATA;
  pusch_pdu->rnti = ra->rnti;
  pusch_pdu->handle = 0;
  int abwp_size  = NRRIV2BW(ubwp->bwp_Common->genericParameters.locationAndBandwidth,275);
  int abwp_start = NRRIV2PRBOFFSET(ubwp->bwp_Common->genericParameters.locationAndBandwidth,275);
  int ibwp_size  = NRRIV2BW(scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth,275);
  int ibwp_start = NRRIV2PRBOFFSET(scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth,275);
  if ((ibwp_start < abwp_start) || (ibwp_size > abwp_size))
    pusch_pdu->bwp_start = abwp_start;
  else
    pusch_pdu->bwp_start = ibwp_start;
  pusch_pdu->bwp_size = ibwp_size;
  pusch_pdu->subcarrier_spacing = ubwp->bwp_Common->genericParameters.subcarrierSpacing;
  pusch_pdu->cyclic_prefix = 0;
  pusch_pdu->mcs_index = 0;
  pusch_pdu->mcs_table = 0;
  pusch_pdu->target_code_rate = nr_get_code_rate_ul(pusch_pdu->mcs_index,pusch_pdu->mcs_table);
  pusch_pdu->qam_mod_order = nr_get_Qm_ul(pusch_pdu->mcs_index,pusch_pdu->mcs_table);
  if (scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoder == NULL)
    pusch_pdu->transform_precoding = 1;
  else
    pusch_pdu->transform_precoding = 0;
  pusch_pdu->data_scrambling_id = *scc->physCellId;
  pusch_pdu->nrOfLayers = 1;
  pusch_pdu->ul_dmrs_symb_pos = 1<<start_symbol_index; // ok for now but use fill dmrs mask later
  pusch_pdu->dmrs_config_type = 0;
  pusch_pdu->ul_dmrs_scrambling_id = *scc->physCellId; //If provided and the PUSCH is not a msg3 PUSCH, otherwise, L2 should set this to physical cell id.
  pusch_pdu->scid = 0; //DMRS sequence initialization [TS38.211, sec 6.4.1.1.1]. Should match what is sent in DCI 0_1, otherwise set to 0.
  pusch_pdu->dmrs_ports = 1;  // 6.2.2 in 38.214 only port 0 to be used
  pusch_pdu->num_dmrs_cdm_grps_no_data = 2;  // no data in dmrs symbols as in 6.2.2 in 38.214
  pusch_pdu->resource_alloc = 1; //type 1
  pusch_pdu->rb_start = ra->msg3_first_rb + ibwp_start - abwp_start; // as for 6.3.1.7 in 38.211
  if (ra->msg3_nb_rb > pusch_pdu->bwp_size)
    AssertFatal(1==0,"MSG3 allocated number of RBs exceed the BWP size\n");
  else
    pusch_pdu->rb_size = ra->msg3_nb_rb;
  pusch_pdu->vrb_to_prb_mapping = 0;
  if (ubwp->bwp_Dedicated->pusch_Config->choice.setup->frequencyHopping == NULL)
    pusch_pdu->frequency_hopping = 0;
  else
    pusch_pdu->frequency_hopping = 1;
  //pusch_pdu->tx_direct_current_location;//The uplink Tx Direct Current location for the carrier. Only values in the value range of this field between 0 and 3299, which indicate the subcarrier index within the carrier corresponding 1o the numerology of the corresponding uplink BWP and value 3300, which indicates "Outside the carrier" and value 3301, which indicates "Undetermined position within the carrier" are used. [TS38.331, UplinkTxDirectCurrentBWP IE]
  pusch_pdu->uplink_frequency_shift_7p5khz = 0;
  //Resource Allocation in time domain
  pusch_pdu->start_symbol_index = start_symbol_index;
  pusch_pdu->nr_of_symbols = nr_of_symbols;
  //Optional Data only included if indicated in pduBitmap
  pusch_pdu->pusch_data.rv_index = 0;  // 8.3 in 38.213
  pusch_pdu->pusch_data.harq_process_id = 0;
  pusch_pdu->pusch_data.new_data_indicator = 1; // new data
  pusch_pdu->pusch_data.num_cb = 0;
  pusch_pdu->pusch_data.tb_size = nr_compute_tbs(pusch_pdu->qam_mod_order,
                                                 pusch_pdu->target_code_rate,
                                                 pusch_pdu->rb_size,
                                                 pusch_pdu->nr_of_symbols,
                                                 12, // nb dmrs set for no data in dmrs symbol
                                                 0, //nb_rb_oh
                                                 0, // to verify tb scaling
                                                 pusch_pdu->nrOfLayers = 1)>>3;

  // calling function to fill rar message
  nr_fill_rar(module_idP, ra, cc->RAR_pdu.payload, pusch_pdu);

}

// WIP
// todo:
// - fix me
// - get msg3 alloc (see nr_process_rar)
void nr_generate_Msg2(module_id_t module_idP,
                      int CC_id,
                      frame_t frameP,
                      sub_frame_t slotP){

  int UE_id = 0, dci_formats[2], rnti_types[2], mcsIndex;
  int startSymbolAndLength = 0, StartSymbolIndex = -1, NrOfSymbols = 14, StartSymbolIndex_tmp, NrOfSymbols_tmp, x_Overhead, time_domain_assignment;
  gNB_MAC_INST                      *nr_mac = RC.nrmac[module_idP];
  NR_COMMON_channels_t                  *cc = &nr_mac->common_channels[0];
  NR_RA_t                               *ra = &cc->ra[0];
  NR_UE_list_t                     *UE_list = &nr_mac->UE_list;
  NR_SearchSpace_t *ss = ra->ra_ss;

  uint16_t RA_rnti = ra->RA_rnti;
  long locationAndBandwidth;
  // uint8_t *vrb_map = cc[CC_id].vrb_map, CC_id;

  // check if UE is doing RA on CORESET0 , InitialBWP or configured BWP from SCD
  // get the BW of the PDCCH for PDCCH size and RAR PDSCH size

  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
  int dci10_bw;

  if (ra->coreset0_configured == 1) {
    AssertFatal(1==0,"This is a standalone condition\n");
  }
  else { // on configured BWP or initial LDBWP, bandwidth parameters in DCI correspond size of initialBWP
    locationAndBandwidth = scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth;
    dci10_bw = NRRIV2BW(locationAndBandwidth,275); 
  }

  if ((ra->Msg2_frame == frameP) && (ra->Msg2_slot == slotP)) {

    nfapi_nr_dl_tti_request_body_t *dl_req = &nr_mac->DL_req[CC_id].dl_tti_request_body;
    nfapi_nr_pdu_t *tx_req = &nr_mac->TX_req[CC_id].pdu_list[nr_mac->TX_req[CC_id].Number_of_PDUs];

    nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdcch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs];
    memset((void*)dl_tti_pdcch_pdu,0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
    dl_tti_pdcch_pdu->PDUType = NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE;
    dl_tti_pdcch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdcch_pdu));

    nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdsch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs+1];
    memset((void *)dl_tti_pdsch_pdu,0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
    dl_tti_pdsch_pdu->PDUType = NFAPI_NR_DL_TTI_PDSCH_PDU_TYPE;
    dl_tti_pdsch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdsch_pdu));

    nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15 = &dl_tti_pdcch_pdu->pdcch_pdu.pdcch_pdu_rel15;
    nfapi_nr_dl_tti_pdsch_pdu_rel15_t *pdsch_pdu_rel15 = &dl_tti_pdsch_pdu->pdsch_pdu.pdsch_pdu_rel15;

    // Checking if the DCI allocation is feasible in current subframe
    if (dl_req->nPDUs == NFAPI_NR_MAX_DL_TTI_PDUS) {
      LOG_I(MAC, "[RAPROC] Subframe %d: FAPI DL structure is full, skip scheduling UE %d\n", slotP, RA_rnti);
      return;
    }

    LOG_I(MAC,"[gNB %d] [RAPROC] CC_id %d Frame %d, slotP %d: Generating RAR DCI, state %d\n", module_idP, CC_id, frameP, slotP, ra->state);

    // This code from this point on will not work on initialBWP or CORESET0
    AssertFatal(ra->bwp_id>0,"cannot work on initialBWP for now\n");

    NR_CellGroupConfig_t *secondaryCellGroup = UE_list->secondaryCellGroup[UE_id];
    AssertFatal(secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count == 1,
      "downlinkBWP_ToAddModList has %d BWP!\n", secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count);
    NR_BWP_Downlink_t *bwp = secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[ra->bwp_id - 1];
    NR_BWP_Uplink_t *ubwp=secondaryCellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[ra->bwp_id-1];

    LOG_D(MAC, "[RAPROC] Scheduling common search space DCI type 1 dlBWP BW %d\n", dci10_bw);

    // Qm>2 not allowed for RAR
    if (get_softmodem_params()->do_ra)
      mcsIndex = 9;
    else
      mcsIndex = 0;

    pdsch_pdu_rel15->pduBitmap = 0;
    pdsch_pdu_rel15->rnti = RA_rnti;
    pdsch_pdu_rel15->pduIndex = 0;


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

    AssertFatal(StartSymbolIndex >= 0, "StartSymbolIndex is negative\n");

    pdsch_pdu_rel15->StartSymbolIndex = StartSymbolIndex;
    pdsch_pdu_rel15->NrOfSymbols      = NrOfSymbols;
    pdsch_pdu_rel15->dlDmrsSymbPos = fill_dmrs_mask(NULL, scc->dmrs_TypeA_Position, NrOfSymbols);

    dci_pdu_rel15_t dci_pdu_rel15[MAX_DCI_CORESET];
    dci_pdu_rel15[0].frequency_domain_assignment.val = PRBalloc_to_locationandbandwidth0(pdsch_pdu_rel15->rbSize,
										     pdsch_pdu_rel15->rbStart,dci10_bw);
    dci_pdu_rel15[0].time_domain_assignment.val = time_domain_assignment;
    dci_pdu_rel15[0].vrb_to_prb_mapping.val = 0;
    dci_pdu_rel15[0].mcs = pdsch_pdu_rel15->mcsIndex[0];
    dci_pdu_rel15[0].tb_scaling = 0;

    LOG_I(MAC, "[RAPROC] DCI type 1 payload: freq_alloc %d (%d,%d,%d), time_alloc %d, vrb to prb %d, mcs %d tb_scaling %d \n",
	  dci_pdu_rel15[0].frequency_domain_assignment.val,
	  pdsch_pdu_rel15->rbStart,
	  pdsch_pdu_rel15->rbSize,
	  dci10_bw,
	  dci_pdu_rel15[0].time_domain_assignment.val,
	  dci_pdu_rel15[0].vrb_to_prb_mapping.val,
	  dci_pdu_rel15[0].mcs,
	  dci_pdu_rel15[0].tb_scaling);

    nr_configure_pdcch(nr_mac, pdcch_pdu_rel15, RA_rnti, 0, ss, scc, bwp);

    LOG_I(MAC, "Frame %d: Subframe %d : Adding common DL DCI for RA_RNTI %x\n", frameP, slotP, RA_rnti);

    dci_formats[0] = NR_DL_DCI_FORMAT_1_0;
    rnti_types[0] = NR_RNTI_RA;

    LOG_D(MAC, "[RAPROC] DCI params: rnti %d, rnti_type %d, dci_format %d coreset params: FreqDomainResource %llx, start_symbol %d  n_symb %d\n",
      pdcch_pdu_rel15->dci_pdu.RNTI[0],
      rnti_types[0],
      dci_formats[0],
      (unsigned long long)pdcch_pdu_rel15->FreqDomainResource,
      pdcch_pdu_rel15->StartSymbolIndex,
      pdcch_pdu_rel15->DurationSymbols);

    fill_dci_pdu_rel15(scc,secondaryCellGroup,pdcch_pdu_rel15, &dci_pdu_rel15[0], dci_formats, rnti_types,dci10_bw,ra->bwp_id);

    dl_req->nPDUs+=2;

    // Program UL processing for Msg3
    nr_get_Msg3alloc(scc, ubwp, slotP, frameP, ra);
    LOG_I(MAC, "Frame %d, Subframe %d: Setting Msg3 reception for Frame %d Subframe %d\n", frameP, slotP, ra->Msg3_frame, ra->Msg3_slot);
    nr_add_msg3(module_idP, CC_id, frameP, slotP);
    ra->state = WAIT_Msg3;
    LOG_D(MAC,"[gNB %d][RAPROC] Frame %d, Subframe %d: RA state %d\n", module_idP, frameP, slotP, ra->state);

    x_Overhead = 0;
    nr_get_tbs_dl(&dl_tti_pdsch_pdu->pdsch_pdu, x_Overhead, pdsch_pdu_rel15->numDmrsCdmGrpsNoData, dci_pdu_rel15[0].tb_scaling);

    // DL TX request
    tx_req->PDU_length = pdsch_pdu_rel15->TBSize[0];
    tx_req->PDU_index = nr_mac->pdu_index[CC_id]++;
    tx_req->num_TLV = 1;
    tx_req->TLVs[0].length = 8;
    nr_mac->TX_req[CC_id].SFN = frameP;
    nr_mac->TX_req[CC_id].Number_of_PDUs++;
    nr_mac->TX_req[CC_id].Slot = slotP;
    memcpy((void*)&tx_req->TLVs[0].value.direct[0], (void*)&cc[CC_id].RAR_pdu.payload[0], tx_req->TLVs[0].length);
  }
}

void nr_clear_ra_proc(module_id_t module_idP, int CC_id, frame_t frameP){
  NR_RA_t *ra = &RC.nrmac[module_idP]->common_channels[CC_id].ra[0];
  LOG_D(MAC,"[gNB %d][RAPROC] CC_id %d Frame %d Clear Random access information rnti %x\n", module_idP, CC_id, frameP, ra->rnti);
  ra->state = IDLE;
  ra->timing_offset = 0;
  ra->RRC_timer = 20;
  ra->rnti = 0;
  ra->msg3_round = 0;
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
// WIP
// todo:
// - handle MAC RAR BI subheader
// - sending only 1 RAR subPDU
// - UL Grant: hardcoded CSI, TPC, time alloc
// - padding
void nr_fill_rar(uint8_t Mod_idP,
                 NR_RA_t * ra,
                 uint8_t * dlsch_buffer,
                 nfapi_nr_pusch_pdu_t  *pusch_pdu){

  LOG_D(MAC, "[gNB] Generate RAR MAC PDU frame %d slot %d ", ra->Msg2_frame, ra-> Msg2_slot);
  NR_RA_HEADER_RAPID *rarh = (NR_RA_HEADER_RAPID *) dlsch_buffer;
  NR_MAC_RAR *rar = (NR_MAC_RAR *) (dlsch_buffer + 1);
  unsigned char csi_req = 0, tpc_command;
  //uint8_t N_UL_Hop;
  uint8_t valid_bits;
  uint32_t ul_grant;
  uint16_t f_alloc, prb_alloc, bwp_size, truncation=0;

  tpc_command = 3; // this is 0 dB

  /// E/T/RAPID subheader ///
  // E = 0, one only RAR, first and last
  // T = 1, RAPID
  rarh->E = 0;
  rarh->T = 1;
  rarh->RAPID = ra->preamble_index;

  /// RAR MAC payload ///
  rar->R = 0;

  // TA command
  rar->TA1 = (uint8_t) (ra->timing_offset >> 5);    // 7 MSBs of timing advance
  rar->TA2 = (uint8_t) (ra->timing_offset & 0x1f);  // 5 LSBs of timing advance

  // TC-RNTI
  rar->TCRNTI_1 = (uint8_t) (ra->rnti >> 8);        // 8 MSBs of rnti
  rar->TCRNTI_2 = (uint8_t) (ra->rnti & 0xff);      // 8 LSBs of rnti

  // UL grant

  ra->msg3_TPC = tpc_command;

  bwp_size = pusch_pdu->bwp_size;
  prb_alloc = PRBalloc_to_locationandbandwidth0(ra->msg3_nb_rb, ra->msg3_first_rb, bwp_size);
  if (bwp_size>180) {
    AssertFatal(1==0,"Initial UBWP larger than 180 currently not supported");
  }
  else {
    valid_bits = (uint8_t)ceil(log2(bwp_size*(bwp_size+1)>>1));
  }

  if (pusch_pdu->frequency_hopping){
    AssertFatal(1==0,"PUSCH with frequency hopping currently not supported");
  } else {
    for (int i=0; i<valid_bits; i++)
      truncation |= (1<<i);
    f_alloc = (prb_alloc&truncation);
  }

  ul_grant = csi_req | (tpc_command << 1) | (pusch_pdu->mcs_index << 4) | (ra->Msg3_tda_id << 8) | (f_alloc << 12) | (pusch_pdu->frequency_hopping << 26);

  rar->UL_GRANT_1 = (uint8_t) (ul_grant >> 24) & 0x07;
  rar->UL_GRANT_2 = (uint8_t) (ul_grant >> 16) & 0xff;
  rar->UL_GRANT_3 = (uint8_t) (ul_grant >> 8) & 0xff;
  rar->UL_GRANT_4 = (uint8_t) ul_grant & 0xff;

}
