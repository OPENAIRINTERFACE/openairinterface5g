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

#include <executables/softmodem-common.h>
extern RAN_CONTEXT_t RC;
extern const uint8_t nr_slots_per_frame[5];
extern uint16_t sl_ahead;

uint8_t DELTA[4]= {2,3,4,6};

#define MAX_NUMBER_OF_SSB 64		
float ssb_per_rach_occasion[8] = {0.125,0.25,0.5,1,2,4,8};

int16_t ssb_index_from_prach(module_id_t module_idP,
                             frame_t frameP,
			     sub_frame_t slotP,
			     uint16_t preamble_index,
			     uint8_t freq_index,
			     uint8_t symbol) {
  
  gNB_MAC_INST *gNB = RC.nrmac[module_idP];
  NR_COMMON_channels_t *cc = &gNB->common_channels[0];
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
  nfapi_nr_config_request_scf_t *cfg = &RC.nrmac[module_idP]->config[0];

  uint8_t config_index = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.prach_ConfigurationIndex;
  uint8_t fdm = cfg->prach_config.num_prach_fd_occasions.value;
  
  uint8_t total_RApreambles = MAX_NUM_NR_PRACH_PREAMBLES;
  if( scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->totalNumberOfRA_Preambles != NULL)
    total_RApreambles = *scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->totalNumberOfRA_Preambles;	
  
  float  num_ssb_per_RO = ssb_per_rach_occasion[cfg->prach_config.ssb_per_rach.value];	
  uint16_t start_symbol_index = 0;
  uint8_t mu,N_dur=0,N_t_slot=0,start_symbol = 0, temp_start_symbol = 0, N_RA_slot=0;
  uint16_t format,RA_sfn_index = -1;
  uint8_t config_period = 1;
  uint16_t prach_occasion_id = -1;
  uint8_t num_active_ssb = cc->num_active_ssb;

  if (scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing)
    mu = *scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing;
  else
    mu = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;

  get_nr_prach_info_from_index(config_index,
			       (int)frameP,
			       (int)slotP,
			       scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA,
			       mu,
			       cc->frame_type,
			       &format,
			       &start_symbol,
			       &N_t_slot,
			       &N_dur,
			       &RA_sfn_index,
			       &N_RA_slot,
			       &config_period);
  uint8_t index = 0,slot_index = 0;
  for (slot_index = 0;slot_index < N_RA_slot; slot_index++) {
    if (N_RA_slot <= 1) { //1 PRACH slot in a subframe
       if((mu == 1) || (mu == 3))
         slot_index = 1;  //For scs = 30khz and 120khz
    }
    for (int i=0; i< N_t_slot; i++) {
      temp_start_symbol = (start_symbol + i * N_dur + 14 * slot_index) % 14;
      if(symbol == temp_start_symbol) {
        start_symbol_index = i;
        break;
      }
    }
  }
  if (N_RA_slot <= 1) { //1 PRACH slot in a subframe
    if((mu == 1) || (mu == 3))
      slot_index = 0;  //For scs = 30khz and 120khz
  }

  //  prach_occasion_id = subframe_index * N_t_slot * N_RA_slot * fdm + N_RA_slot_index * N_t_slot * fdm + freq_index + fdm * start_symbol_index; 
  prach_occasion_id = (((frameP % (cc->max_association_period * config_period))/config_period)*cc->total_prach_occasions_per_config_period) +
                      (RA_sfn_index + slot_index) * N_t_slot * fdm + start_symbol_index * fdm + freq_index; 

  //one RO is shared by one or more SSB
  if(num_ssb_per_RO <= 1 )
    index = (int) (prach_occasion_id / (int)(1/num_ssb_per_RO)) % num_active_ssb;
  //one SSB have more than one continuous RO
  else if ( num_ssb_per_RO > 1) {
    index = (prach_occasion_id * (int)num_ssb_per_RO)% num_active_ssb ;
    for(int j = 0;j < num_ssb_per_RO;j++) {
      if(preamble_index <  (((j+1) * total_RApreambles) / num_ssb_per_RO))
        index = index + j;
    }
  }

  LOG_D(NR_MAC, "Frame %d, Slot %d: Prach Occasion id = %d ssb per RO = %f number of active SSB %u index = %d fdm %u symbol index %u freq_index %u total_RApreambles %u\n",
        frameP, slotP, prach_occasion_id, num_ssb_per_RO, num_active_ssb, index, fdm, start_symbol_index, freq_index, total_RApreambles);

  return index;
}


//Compute Total active SSBs and RO available
void find_SSB_and_RO_available(module_id_t module_idP) {

  gNB_MAC_INST *gNB = RC.nrmac[module_idP];
  NR_COMMON_channels_t *cc = &gNB->common_channels[0];
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
  nfapi_nr_config_request_scf_t *cfg = &RC.nrmac[module_idP]->config[0];

  uint8_t config_index = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.prach_ConfigurationIndex;
  uint8_t mu,N_dur=0,N_t_slot=0,start_symbol=0,N_RA_slot = 0;
  uint16_t format,N_RA_sfn = 0,unused_RA_occasion,repetition = 0;
  uint8_t num_active_ssb = 0;
  uint8_t max_association_period = 1;

  struct NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB *ssb_perRACH_OccasionAndCB_PreamblesPerSSB = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB;

  switch (ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present){
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_oneEighth:
      cc->cb_preambles_per_ssb = 4 * (ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.oneEighth + 1);
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_oneFourth:
      cc->cb_preambles_per_ssb = 4 * (ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.oneFourth + 1);
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_oneHalf:
      cc->cb_preambles_per_ssb = 4 * (ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.oneHalf + 1);
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_one:
      cc->cb_preambles_per_ssb = 4 * (ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.one + 1);
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_two:
      cc->cb_preambles_per_ssb = 4 * (ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.two + 1);
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_four:
      cc->cb_preambles_per_ssb = ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.four;
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_eight:
      cc->cb_preambles_per_ssb = ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.eight;
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_sixteen:
      cc->cb_preambles_per_ssb = ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.sixteen;
      break;
    default:
      AssertFatal(1 == 0, "Unsupported ssb_perRACH_config %d\n", ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present);
      break;
    }

  if (scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing)
    mu = *scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing;
  else
    mu = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;

  // prach is scheduled according to configuration index and tables 6.3.3.2.2 to 6.3.3.2.4
  get_nr_prach_occasion_info_from_index(config_index,
                                        scc->downlinkConfigCommon->frequencyInfoDL->absoluteFrequencyPointA,
                                        mu,
                                        cc->frame_type,
                                        &format,
                                        &start_symbol,
                                        &N_t_slot,
                                        &N_dur,
                                        &N_RA_slot,
                                        &N_RA_sfn,
                                        &max_association_period);

  float num_ssb_per_RO = ssb_per_rach_occasion[cfg->prach_config.ssb_per_rach.value];	
  uint8_t fdm = cfg->prach_config.num_prach_fd_occasions.value;
  uint64_t L_ssb = (((uint64_t) cfg->ssb_table.ssb_mask_list[0].ssb_mask.value)<<32) | cfg->ssb_table.ssb_mask_list[1].ssb_mask.value ;
  uint32_t total_RA_occasions = N_RA_sfn * N_t_slot * N_RA_slot * fdm;

  for(int i = 0;i < 64;i++) {
    if ((L_ssb >> (63-i)) & 0x01) { // only if the bit of L_ssb at current ssb index is 1
      cc->ssb_index[num_active_ssb] = i; 
      num_active_ssb++;
    }
  }

  cc->total_prach_occasions_per_config_period = total_RA_occasions;
  for(int i=1; (1 << (i-1)) <= max_association_period; i++) {
    cc->max_association_period = (1 <<(i-1));
    total_RA_occasions = total_RA_occasions * cc->max_association_period;
    if(total_RA_occasions >= (int) (num_active_ssb/num_ssb_per_RO)) {
      repetition = (uint16_t)((total_RA_occasions * num_ssb_per_RO )/num_active_ssb);
      break;
    }
  }

  unused_RA_occasion = total_RA_occasions - (int)((num_active_ssb * repetition)/num_ssb_per_RO);
  cc->total_prach_occasions = total_RA_occasions - unused_RA_occasion;
  cc->num_active_ssb = num_active_ssb;

  LOG_I(NR_MAC,
        "Total available RO %d, num of active SSB %d: unused RO = %d association_period %u N_RA_sfn %u total_prach_occasions_per_config_period %u\n",
        cc->total_prach_occasions,
        cc->num_active_ssb,
        unused_RA_occasion,
        cc->max_association_period,
        N_RA_sfn,
        cc->total_prach_occasions_per_config_period);
}		
		
void schedule_nr_prach(module_id_t module_idP, frame_t frameP, sub_frame_t slotP)
{
  gNB_MAC_INST *gNB = RC.nrmac[module_idP];
  NR_COMMON_channels_t *cc = gNB->common_channels;
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
  nfapi_nr_ul_tti_request_t *UL_tti_req = &RC.nrmac[module_idP]->UL_tti_req_ahead[0][slotP];
  nfapi_nr_config_request_scf_t *cfg = &RC.nrmac[module_idP]->config[0];

  if (is_nr_UL_slot(scc->tdd_UL_DL_ConfigurationCommon, slotP, cc->frame_type)) {

    uint8_t config_index = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.prach_ConfigurationIndex;
    uint8_t mu,N_dur,N_t_slot,start_symbol = 0,N_RA_slot;
    uint16_t RA_sfn_index = -1;
    uint8_t config_period = 1;
    uint16_t format;
    int slot_index = 0;
    uint16_t prach_occasion_id = -1;

    if (scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing)
      mu = *scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg1_SubcarrierSpacing;
    else
      mu = scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;

    int bwp_start = NRRIV2PRBOFFSET(scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);

    uint8_t fdm = cfg->prach_config.num_prach_fd_occasions.value;
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
                                      &N_dur,
                                      &RA_sfn_index,
                                      &N_RA_slot,
                                      &config_period) ) {

      uint16_t format0 = format&0xff;      // first column of format from table
      uint16_t format1 = (format>>8)&0xff; // second column of format from table

      if (N_RA_slot > 1) { //more than 1 PRACH slot in a subframe
        if (slotP%2 == 1)
          slot_index = 1;
        else
          slot_index = 0;
      }else if (N_RA_slot <= 1) { //1 PRACH slot in a subframe
        slot_index = 0;
      }

      UL_tti_req->SFN = frameP;
      UL_tti_req->Slot = slotP;
      for (int fdm_index=0; fdm_index < fdm; fdm_index++) { // one structure per frequency domain occasion
        for (int td_index=0; td_index<N_t_slot; td_index++) {

          prach_occasion_id = (((frameP % (cc->max_association_period * config_period))/config_period) * cc->total_prach_occasions_per_config_period) +
                              (RA_sfn_index + slot_index) * N_t_slot * fdm + td_index * fdm + fdm_index;

          if((prach_occasion_id < cc->total_prach_occasions) && (td_index == 0)){

            UL_tti_req->pdus_list[UL_tti_req->n_pdus].pdu_type = NFAPI_NR_UL_CONFIG_PRACH_PDU_TYPE;
            UL_tti_req->pdus_list[UL_tti_req->n_pdus].pdu_size = sizeof(nfapi_nr_prach_pdu_t);
            nfapi_nr_prach_pdu_t  *prach_pdu = &UL_tti_req->pdus_list[UL_tti_req->n_pdus].prach_pdu;
            memset(prach_pdu,0,sizeof(nfapi_nr_prach_pdu_t));
            UL_tti_req->n_pdus+=1;

            // filling the prach fapi structure
            prach_pdu->phys_cell_id = *scc->physCellId;
            prach_pdu->num_prach_ocas = N_t_slot;
            prach_pdu->prach_start_symbol = start_symbol;
            prach_pdu->num_ra = fdm_index;
            prach_pdu->num_cs = get_NCS(scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric.zeroCorrelationZoneConfig,
                                        format0,
                                        scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->restrictedSetConfig);

            LOG_D(NR_MAC, "Frame %d, Slot %d: Prach Occasion id = %u  fdm index = %u start symbol = %u slot index = %u subframe index = %u \n",
                  frameP, slotP,
                  prach_occasion_id, prach_pdu->num_ra,
                  prach_pdu->prach_start_symbol,
                  slot_index, RA_sfn_index);
            // SCF PRACH PDU format field does not consider A1/B1 etc. possibilities
            // We added 9 = A1/B1 10 = A2/B2 11 A3/B3
            if (format1!=0xff) {
              switch(format0) {
                case 0xa1:
                  prach_pdu->prach_format = 11;
                  break;
                case 0xa2:
                  prach_pdu->prach_format = 12;
                  break;
                case 0xa3:
                  prach_pdu->prach_format = 13;
                  break;
              default:
                AssertFatal(1==0,"Only formats A1/B1 A2/B2 A3/B3 are valid for dual format");
              }
            }
            else{
              switch(format0) {
                case 0:
                  prach_pdu->prach_format = 0;
                  break;
                case 1:
                  prach_pdu->prach_format = 1;
                  break;
                case 2:
                  prach_pdu->prach_format = 2;
                  break;
                case 3:
                  prach_pdu->prach_format = 3;
                  break;
                case 0xa1:
                  prach_pdu->prach_format = 4;
                  break;
                case 0xa2:
                  prach_pdu->prach_format = 5;
                  break;
                case 0xa3:
                  prach_pdu->prach_format = 6;
                  break;
                case 0xb1:
                  prach_pdu->prach_format = 7;
                  break;
                case 0xb4:
                  prach_pdu->prach_format = 8;
                  break;
                case 0xc0:
                  prach_pdu->prach_format = 9;
                  break;
                case 0xc2:
                  prach_pdu->prach_format = 10;
                  break;
              default:
                AssertFatal(1==0,"Invalid PRACH format");
              }
            }
          }
        }
      }

      // block resources in vrb_map_UL
      const NR_RACH_ConfigGeneric_t *rach_ConfigGeneric =
          &scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->rach_ConfigGeneric;
      const uint8_t mu_pusch =
          scc->uplinkConfigCommon->frequencyInfoUL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;
      const int16_t N_RA_RB = get_N_RA_RB(cfg->prach_config.prach_sub_c_spacing.value, mu_pusch);
      uint16_t *vrb_map_UL = &cc->vrb_map_UL[slotP * MAX_BWP_SIZE];
      for (int i = 0; i < N_RA_RB * fdm; ++i)
        vrb_map_UL[bwp_start + rach_ConfigGeneric->msg1_FrequencyStart + i] = 0xff; // all symbols
    }
  }
}

void nr_schedule_msg2(uint16_t rach_frame, uint16_t rach_slot,
                      uint16_t *msg2_frame, uint16_t *msg2_slot,
                      NR_ServingCellConfigCommon_t *scc,
                      uint16_t monitoring_slot_period,
                      uint16_t monitoring_offset,uint8_t beam_index,
                      uint8_t num_active_ssb,
                      int16_t *tdd_beam_association){

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
  uint8_t frame_limit = (slot_limit>(rach_slot))? rach_frame : (rach_frame +1);

  // computing start of next period

  int FR = *scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0] >= 257 ? nr_FR2 : nr_FR1;

  uint8_t start_next_period = (rach_slot-(rach_slot%tdd_period_slot)+tdd_period_slot)%nr_slots_per_frame[mu];
  *msg2_slot = start_next_period + last_dl_slot_period; // initializing scheduling of slot to next mixed (or last dl) slot
  *msg2_frame = ((*msg2_slot>(rach_slot))? rach_frame : (rach_frame+1))%1024;

  // we can't schedule msg2 before sl_ahead since prach
  int eff_slot = *msg2_slot+(*msg2_frame-rach_frame)*nr_slots_per_frame[mu];
  if ((eff_slot-rach_slot)<=sl_ahead) {
    *msg2_slot = (*msg2_slot+tdd_period_slot)%nr_slots_per_frame[mu];
    *msg2_frame = ((*msg2_slot>(rach_slot))? rach_frame : (rach_frame+1))%1024;
  }
  if (FR==nr_FR2) {
    int num_tdd_period = *msg2_slot/tdd_period_slot;
    while((tdd_beam_association[num_tdd_period]!=-1)&&(tdd_beam_association[num_tdd_period]!=beam_index)) {
      *msg2_slot = (*msg2_slot+tdd_period_slot)%nr_slots_per_frame[mu];
      *msg2_frame = ((*msg2_slot>(rach_slot))? rach_frame : (rach_frame+1))%1024;
      num_tdd_period = *msg2_slot/tdd_period_slot;
    }
    if(tdd_beam_association[num_tdd_period] == -1)
      tdd_beam_association[num_tdd_period] = beam_index;
  }

  // go to previous slot if the current scheduled slot is beyond the response window
  // and if the slot is not among the PDCCH monitored ones (38.213 10.1)
  while (((*msg2_slot>slot_limit)&&(*msg2_frame>frame_limit)) || ((*msg2_frame*nr_slots_per_frame[mu]+*msg2_slot-monitoring_offset)%monitoring_slot_period !=0))  {
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

  uint16_t msg2_frame, msg2_slot,monitoring_slot_period,monitoring_offset;
  gNB_MAC_INST *nr_mac = RC.nrmac[module_idP];
  NR_COMMON_channels_t *cc = &nr_mac->common_channels[CC_id];
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;

  uint8_t total_RApreambles = MAX_NUM_NR_PRACH_PREAMBLES;
  uint8_t  num_ssb_per_RO = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present;
  int pr_found;

  if( scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->totalNumberOfRA_Preambles != NULL)
    total_RApreambles = *scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->totalNumberOfRA_Preambles;

  if(num_ssb_per_RO > 3) { /*num of ssb per RO >= 1*/
    num_ssb_per_RO -= 3;
    total_RApreambles = total_RApreambles/num_ssb_per_RO ;
  }

  for (int i = 0; i < NR_NB_RA_PROC_MAX; i++) {
    NR_RA_t *ra = &cc->ra[i];
    pr_found = 0;
    if (ra->state == RA_IDLE) {
      for(int j = 0; j < ra->preambles.num_preambles; j++) {
        //check if the preamble received correspond to one of the listed or configured preambles
        if (preamble_index == ra->preambles.preamble_list[j]) {
          pr_found=1;
          break;
        }
      }
      if (pr_found == 0) {
         continue;
      }

      uint16_t ra_rnti;

      // ra_rnti from 5.1.3 in 38.321
      // FK: in case of long PRACH the phone seems to expect the subframe number instead of the slot number here.
      if (scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->prach_RootSequenceIndex.present
          == NR_RACH_ConfigCommon__prach_RootSequenceIndex_PR_l839)
        ra_rnti = 1 + symbol + (9 /*slotP*/ * 14) + (freq_index * 14 * 80) + (ul_carrier_id * 14 * 80 * 8);
      else
        ra_rnti = 1 + symbol + (slotP * 14) + (freq_index * 14 * 80) + (ul_carrier_id * 14 * 80 * 8);

      // This should be handled differently when we use the initialBWP for RA
      ra->bwp_id = 0;
      NR_BWP_Downlink_t *bwp=NULL;
      if (ra->CellGroup && ra->CellGroup->spCellConfig && ra->CellGroup->spCellConfig->spCellConfigDedicated &&
          ra->CellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList) {
        ra->bwp_id = 1;
        bwp = ra->CellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[ra->bwp_id - 1];
      }

      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_INITIATE_RA_PROC, 1);

      LOG_D(NR_MAC,
            "[gNB %d][RAPROC] CC_id %d Frame %d, Slot %d  Initiating RA procedure for preamble index %d\n",
            module_idP,
            CC_id,
            frameP,
            slotP,
            preamble_index);

      uint8_t beam_index = ssb_index_from_prach(module_idP, frameP, slotP, preamble_index, freq_index, symbol);

      // the UE sent a RACH either for starting RA procedure or RA procedure failed and UE retries
      if (ra->cfra) {
        // if the preamble received correspond to one of the listed
        if (!(preamble_index == ra->preambles.preamble_list[beam_index])) {
          LOG_E(
              NR_MAC,
              "[gNB %d][RAPROC] FAILURE: preamble %d does not correspond to any of the ones in rach_ConfigDedicated\n",
              module_idP,
              preamble_index);
          continue; // if the PRACH preamble does not correspond to any of the ones sent through RRC abort RA proc
        }
      }
      LOG_D(NR_MAC, "Frame %d, Slot %d: Activating RA process \n", frameP, slotP);
      ra->state = Msg2;
      ra->timing_offset = timing_offset;
      ra->preamble_slot = slotP;

      NR_SearchSpaceId_t	ra_SearchSpace = 0;
      struct NR_PDCCH_ConfigCommon__commonSearchSpaceList *commonSearchSpaceList = NULL;
      if(bwp) {
        commonSearchSpaceList = bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->commonSearchSpaceList;
        ra_SearchSpace = *bwp->bwp_Common->pdcch_ConfigCommon->choice.setup->ra_SearchSpace;
      } else {
        commonSearchSpaceList = scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->commonSearchSpaceList;
        ra_SearchSpace = *scc->downlinkConfigCommon->initialDownlinkBWP->pdcch_ConfigCommon->choice.setup->ra_SearchSpace;
      }
      AssertFatal(commonSearchSpaceList->list.count > 0, "common SearchSpace list has 0 elements\n");

      // Common SearchSpace list
      for (int i = 0; i < commonSearchSpaceList->list.count; i++) {
        ss = commonSearchSpaceList->list.array[i];
        if (ss->searchSpaceId == ra_SearchSpace)
          ra->ra_ss = ss;
      }

      AssertFatal(ra->ra_ss!=NULL,"SearchSpace cannot be null for RA\n");

      // retrieving ra pdcch monitoring period and offset
      find_monitoring_periodicity_offset_common(ra->ra_ss, &monitoring_slot_period, &monitoring_offset);

      nr_schedule_msg2(frameP,
                       slotP,
                       &msg2_frame,
                       &msg2_slot,
                       scc,
                       monitoring_slot_period,
                       monitoring_offset,
                       beam_index,
                       cc->num_active_ssb,
                       nr_mac->tdd_beam_association);

      ra->Msg2_frame = msg2_frame;
      ra->Msg2_slot = msg2_slot;

      LOG_D(NR_MAC, "%s() Msg2[%04d%d] SFN/SF:%04d%d\n", __FUNCTION__, ra->Msg2_frame, ra->Msg2_slot, frameP, slotP);

      int loop = 0;
      if (ra->rnti == 0) { // This condition allows for the usage of a preconfigured rnti for the CFRA
        do {
          ra->rnti = (taus() % 65518) + 1;
          loop++;
        } while (loop != 100
                 && !((find_nr_UE_id(module_idP, ra->rnti) == -1) && (find_nr_RA_id(module_idP, CC_id, ra->rnti) == -1)
                      && ra->rnti >= 1 && ra->rnti <= 65519));
        if (loop == 100) {
          LOG_E(NR_MAC, "%s:%d:%s: [RAPROC] initialisation random access aborted\n", __FILE__, __LINE__, __FUNCTION__);
          abort();
        }
      }

      ra->RA_rnti = ra_rnti;
      ra->preamble_index = preamble_index;
      ra->beam_id = beam_index;

      LOG_D(NR_MAC,
            "[gNB %d][RAPROC] CC_id %d Frame %d Activating Msg2 generation in frame %d, slot %d using RA rnti %x SSB "
            "index %u\n",
            module_idP,
            CC_id,
            frameP,
            ra->Msg2_frame,
            ra->Msg2_slot,
            ra->RA_rnti,
            cc->ssb_index[beam_index]);

      return;
    }
  }
  LOG_E(NR_MAC, "[gNB %d][RAPROC] FAILURE: CC_id %d Frame %d initiating RA procedure for preamble index %d\n", module_idP, CC_id, frameP, preamble_index);

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_INITIATE_RA_PROC, 0);
}

void nr_schedule_RA(module_id_t module_idP, frame_t frameP, sub_frame_t slotP) {

  gNB_MAC_INST *mac = RC.nrmac[module_idP];

  start_meas(&mac->schedule_ra);
  for (int CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    NR_COMMON_channels_t *cc = &mac->common_channels[CC_id];
    for (int i = 0; i < NR_NB_RA_PROC_MAX; i++) {
      NR_RA_t *ra = &cc->ra[i];
      LOG_D(NR_MAC, "RA[state:%d]\n", ra->state);
      switch (ra->state) {
        case Msg2:
          nr_generate_Msg2(module_idP, CC_id, frameP, slotP, ra);
          break;
        case Msg3_retransmission:
          nr_generate_Msg3_retransmission(module_idP, CC_id, frameP, slotP, ra);
          break;
        case Msg4:
          nr_generate_Msg4(module_idP, CC_id, frameP, slotP, ra);
          break;
        case WAIT_Msg4_ACK:
          nr_check_Msg4_Ack(module_idP, CC_id, frameP, slotP, ra);
          break;
        default:
          break;
      }
    }
  }
  stop_meas(&mac->schedule_ra);
}


void nr_generate_Msg3_retransmission(module_id_t module_idP, int CC_id, frame_t frame, sub_frame_t slot, NR_RA_t *ra) {

  gNB_MAC_INST *nr_mac = RC.nrmac[module_idP];
  NR_COMMON_channels_t *cc = &nr_mac->common_channels[CC_id];
  NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;

  NR_BWP_Uplink_t *ubwp = NULL;
  NR_BWP_UplinkDedicated_t *ubwpd = NULL;
  NR_PUSCH_TimeDomainResourceAllocationList_t *pusch_TimeDomainAllocationList = NULL;
  NR_BWP_t *genericParameters = NULL;
  if(ra->CellGroup) {
    ubwp = ra->CellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[ra->bwp_id-1];
    ubwpd = ra->CellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP;
    genericParameters = &ubwp->bwp_Common->genericParameters;
    pusch_TimeDomainAllocationList = ubwp->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList;
  } else {
    genericParameters = &scc->uplinkConfigCommon->initialUplinkBWP->genericParameters;
    pusch_TimeDomainAllocationList = scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList;
  }

  int mu = genericParameters->subcarrierSpacing;
  uint8_t K2 = *pusch_TimeDomainAllocationList->list.array[ra->Msg3_tda_id]->k2;
  const int sched_frame = frame + (slot + K2 >= nr_slots_per_frame[mu]);
  const int sched_slot = (slot + K2) % nr_slots_per_frame[mu];

  if (is_xlsch_in_slot(RC.nrmac[module_idP]->ulsch_slot_bitmap[sched_slot / 64], sched_slot)) {
    // beam association for FR2
    int16_t *tdd_beam_association = nr_mac->tdd_beam_association;
    if (*scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0] >= 257) {
      uint8_t tdd_period_slot =  scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSlots + scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSlots;
      if ((scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSymbols > 0) || (scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSymbols > 0))
        tdd_period_slot++;
      int num_tdd_period = sched_slot/tdd_period_slot;
      if((tdd_beam_association[num_tdd_period]!=-1)&&(tdd_beam_association[num_tdd_period]!=ra->beam_id))
        return; // can't schedule retransmission in this slot
      else
        tdd_beam_association[num_tdd_period] = ra->beam_id;
    }

    int scs = scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.subcarrierSpacing;
    int fh = 0;
    int startSymbolAndLength = scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[ra->Msg3_tda_id]->startSymbolAndLength;
    int mappingtype = scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[ra->Msg3_tda_id]->mappingType;

    uint16_t *vrb_map_UL = &RC.nrmac[module_idP]->common_channels[CC_id].vrb_map_UL[sched_slot * MAX_BWP_SIZE];

    int BWPStart = nr_mac->type0_PDCCH_CSS_config[ra->beam_id].cset_start_rb;
    int BWPSize  = nr_mac->type0_PDCCH_CSS_config[ra->beam_id].num_rbs;
    int rbStart = 0;
    for (int i = 0; (i < ra->msg3_nb_rb) && (rbStart <= (BWPSize - ra->msg3_nb_rb)); i++) {
      if (vrb_map_UL[rbStart + BWPStart + i]) {
        rbStart += i;
        i = 0;
      }
    }
    if (rbStart > (BWPSize - ra->msg3_nb_rb)) {
      // cannot find free vrb_map for msg3 retransmission in this slot
      return;
    }

    LOG_I(NR_MAC, "[gNB %d][RAPROC] Frame %d, Slot %d : CC_id %d Scheduling retransmission of Msg3 in (%d,%d)\n",
          module_idP, frame, slot, CC_id, sched_frame, sched_slot);

    nfapi_nr_ul_tti_request_t *future_ul_tti_req = &RC.nrmac[module_idP]->UL_tti_req_ahead[CC_id][sched_slot];
    AssertFatal(future_ul_tti_req->SFN == sched_frame
                && future_ul_tti_req->Slot == sched_slot,
                "future UL_tti_req's frame.slot %d.%d does not match PUSCH %d.%d\n",
                future_ul_tti_req->SFN,
                future_ul_tti_req->Slot,
                sched_frame,
                sched_slot);
    future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_type = NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE;
    future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_size = sizeof(nfapi_nr_pusch_pdu_t);
    nfapi_nr_pusch_pdu_t *pusch_pdu = &future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pusch_pdu;
    memset(pusch_pdu, 0, sizeof(nfapi_nr_pusch_pdu_t));

    fill_msg3_pusch_pdu(pusch_pdu, scc,
                        ra->msg3_round,
                        startSymbolAndLength,
                        ra->rnti, scs,
                        BWPSize, BWPStart,
                        mappingtype, fh,
                        rbStart, ra->msg3_nb_rb);
    future_ul_tti_req->n_pdus += 1;

    // generation of DCI 0_0 to schedule msg3 retransmission
    NR_SearchSpace_t *ss = ra->ra_ss;
    NR_ControlResourceSet_t *coreset = get_coreset(module_idP, scc, NULL, ss, NR_SearchSpace__searchSpaceType_PR_common);
    AssertFatal(coreset!=NULL,"Coreset cannot be null for RA-Msg3 retransmission\n");

    nfapi_nr_ul_dci_request_t *ul_dci_req = &nr_mac->UL_dci_req[CC_id];

    const int coresetid = coreset->controlResourceSetId;
    nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15 = nr_mac->pdcch_pdu_idx[CC_id][ra->bwp_id][coresetid];
    if (!pdcch_pdu_rel15) {
      nfapi_nr_ul_dci_request_pdus_t *ul_dci_request_pdu = &ul_dci_req->ul_dci_pdu_list[ul_dci_req->numPdus];
      memset(ul_dci_request_pdu, 0, sizeof(nfapi_nr_ul_dci_request_pdus_t));
      ul_dci_request_pdu->PDUType = NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE;
      ul_dci_request_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdcch_pdu));
      pdcch_pdu_rel15 = &ul_dci_request_pdu->pdcch_pdu.pdcch_pdu_rel15;
      ul_dci_req->numPdus += 1;
      nr_configure_pdcch(nr_mac, pdcch_pdu_rel15, ss, coreset, scc, genericParameters, NULL);
      nr_mac->pdcch_pdu_idx[CC_id][ra->bwp_id][coresetid] = pdcch_pdu_rel15;
    }

    uint8_t aggregation_level;
    uint8_t nr_of_candidates;
    for (int i=0; i<5; i++) {
      // for now taking the lowest value among the available aggregation levels
      find_aggregation_candidates(&aggregation_level, &nr_of_candidates, ss, 1<<i);
      if(nr_of_candidates>0) break;
    }
    AssertFatal(nr_of_candidates>0,"nr_of_candidates is 0\n");
    int CCEIndex = allocate_nr_CCEs(nr_mac, NULL, coreset, aggregation_level, 0, 0, nr_of_candidates);
    if (CCEIndex < 0) {
      LOG_E(NR_MAC, "%s(): cannot find free CCE for RA RNTI 0x%04x!\n", __func__, ra->rnti);
      return;
    }

    // Fill PDCCH DL DCI PDU
    nfapi_nr_dl_dci_pdu_t *dci_pdu = &pdcch_pdu_rel15->dci_pdu[pdcch_pdu_rel15->numDlDci];
    pdcch_pdu_rel15->numDlDci++;
    dci_pdu->RNTI = ra->rnti;
    dci_pdu->ScramblingId = *scc->physCellId;
    dci_pdu->ScramblingRNTI = 0;
    dci_pdu->AggregationLevel = aggregation_level;
    dci_pdu->CceIndex = CCEIndex;
    dci_pdu->beta_PDCCH_1_0 = 0;
    dci_pdu->powerControlOffsetSS = 1;

    dci_pdu_rel15_t uldci_payload;
    memset(&uldci_payload, 0, sizeof(uldci_payload));

    config_uldci(ubwp,
                 ubwpd,
                 scc,
                 pusch_pdu,
                 &uldci_payload,
                 NR_UL_DCI_FORMAT_0_0,
                 ra->Msg3_tda_id,
                 ra->msg3_TPC,
                 0, // not used in format 0_0
                 ra->bwp_id);

    fill_dci_pdu_rel15(scc,
                       ra->CellGroup,
                       dci_pdu,
                       &uldci_payload,
                       NR_UL_DCI_FORMAT_0_0,
                       NR_RNTI_TC,
                       pusch_pdu->bwp_size,
                       ra->bwp_id);

    // Mark the corresponding RBs as used
    for (int rb = 0; rb < ra->msg3_nb_rb; rb++) {
      vrb_map_UL[rbStart + BWPStart + rb] = 1;
    }

    // reset state to wait msg3
    ra->state = WAIT_Msg3;
    ra->Msg3_frame = sched_frame;
    ra->Msg3_slot = sched_slot;

  }

}

void nr_get_Msg3alloc(module_id_t module_id,
                      int CC_id,
                      NR_ServingCellConfigCommon_t *scc,
                      NR_BWP_Uplink_t *ubwp,
                      sub_frame_t current_slot,
                      frame_t current_frame,
                      NR_RA_t *ra,
                      int16_t *tdd_beam_association) {

  // msg3 is scheduled in mixed slot in the following TDD period

  uint16_t msg3_nb_rb = 8; // sdu has 6 or 8 bytes

  int mu = ubwp ?
    ubwp->bwp_Common->genericParameters.subcarrierSpacing :
    scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.subcarrierSpacing;
  int StartSymbolIndex = 0;
  int NrOfSymbols = 0;
  int startSymbolAndLength = 0;
  int temp_slot = 0;
  ra->Msg3_tda_id = 16; // initialization to a value above limit

  NR_PUSCH_TimeDomainResourceAllocationList_t *pusch_TimeDomainAllocationList= ubwp ?
    ubwp->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList:
    scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList;

  uint8_t k2 = 0;
  for (int i=0; i<pusch_TimeDomainAllocationList->list.count; i++) {
    startSymbolAndLength = pusch_TimeDomainAllocationList->list.array[i]->startSymbolAndLength;
    SLIV2SL(startSymbolAndLength, &StartSymbolIndex, &NrOfSymbols);
    // we want to transmit in the uplink symbols of mixed slot
    if (NrOfSymbols == scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSymbols) {
      k2 = *pusch_TimeDomainAllocationList->list.array[i]->k2;
      temp_slot = current_slot + k2 + DELTA[mu]; // msg3 slot according to 8.3 in 38.213
      ra->Msg3_slot = temp_slot%nr_slots_per_frame[mu];
      if (is_xlsch_in_slot(RC.nrmac[module_id]->ulsch_slot_bitmap[ra->Msg3_slot / 64], ra->Msg3_slot)) {
        ra->Msg3_tda_id = i;
        break;
      }
    }
  }

  AssertFatal(ra->Msg3_tda_id<16,"Unable to find Msg3 time domain allocation in list\n");

  if (nr_slots_per_frame[mu]>temp_slot)
    ra->Msg3_frame = current_frame;
  else
    ra->Msg3_frame = (current_frame + (temp_slot/nr_slots_per_frame[mu]))%1024;

  // beam association for FR2
  if (*scc->downlinkConfigCommon->frequencyInfoDL->frequencyBandList.list.array[0] >= 257) {
    uint8_t tdd_period_slot =  scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSlots + scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSlots;
    if ((scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSymbols > 0) || (scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSymbols > 0))
      tdd_period_slot++;
    int num_tdd_period = ra->Msg3_slot/tdd_period_slot;
    if((tdd_beam_association[num_tdd_period]!=-1)&&(tdd_beam_association[num_tdd_period]!=ra->beam_id))
      AssertFatal(1==0,"Cannot schedule MSG3\n");
    else
      tdd_beam_association[num_tdd_period] = ra->beam_id;
  }

  LOG_D(NR_MAC, "[RAPROC] Msg3 slot %d: current slot %u Msg3 frame %u k2 %u Msg3_tda_id %u start symbol index %u\n", ra->Msg3_slot, current_slot, ra->Msg3_frame, k2,ra->Msg3_tda_id, StartSymbolIndex);
  uint16_t *vrb_map_UL =
      &RC.nrmac[module_id]->common_channels[CC_id].vrb_map_UL[ra->Msg3_slot * MAX_BWP_SIZE];

  int bwpSize = NRRIV2BW(scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  int bwpStart = NRRIV2PRBOFFSET(scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);

  if (ra->CellGroup) {
    AssertFatal(ra->CellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count == 1,
		"downlinkBWP_ToAddModList has %d BWP!\n", ra->CellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count);
    NR_BWP_Uplink_t *ubwp = ra->CellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[ra->bwp_id - 1];
    int act_bwp_start = NRRIV2PRBOFFSET(ubwp->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
    int act_bwp_size  = NRRIV2BW(ubwp->bwp_Common->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
    if (!((bwpStart >= act_bwp_start) && ((bwpStart+bwpSize) <= (act_bwp_start+act_bwp_size))))
      bwpStart = act_bwp_start;
  }

  /* search msg3_nb_rb free RBs */
  int rbSize = 0;
  int rbStart = 0;
  while (rbSize < msg3_nb_rb) {
    rbStart += rbSize; /* last iteration rbSize was not enough, skip it */
    rbSize = 0;
    while (rbStart < bwpSize && vrb_map_UL[rbStart + bwpStart])
      rbStart++;
    AssertFatal(rbStart < bwpSize - msg3_nb_rb, "no space to allocate Msg 3 for RA!\n");
    while (rbStart + rbSize < bwpSize
           && !vrb_map_UL[rbStart + bwpStart + rbSize]
           && rbSize < msg3_nb_rb)
      rbSize++;
  }
  ra->msg3_nb_rb = msg3_nb_rb;
  ra->msg3_first_rb = rbStart;
  ra->msg3_bwp_start = bwpStart;
}


void fill_msg3_pusch_pdu(nfapi_nr_pusch_pdu_t *pusch_pdu,
                         NR_ServingCellConfigCommon_t *scc,
                         int round,
                         int startSymbolAndLength,
                         rnti_t rnti, int scs,
                         int bwp_size, int bwp_start,
                         int mappingtype, int fh,
                         int msg3_first_rb, int msg3_nb_rb) {


  int start_symbol_index,nr_of_symbols;
  SLIV2SL(startSymbolAndLength, &start_symbol_index, &nr_of_symbols);

  pusch_pdu->pdu_bit_map = PUSCH_PDU_BITMAP_PUSCH_DATA;
  pusch_pdu->rnti = rnti;
  pusch_pdu->handle = 0;
  pusch_pdu->bwp_start = bwp_start;
  pusch_pdu->bwp_size = bwp_size;
  pusch_pdu->subcarrier_spacing = scs;
  pusch_pdu->cyclic_prefix = 0;
  pusch_pdu->mcs_index = 0;
  pusch_pdu->mcs_table = 0;
  pusch_pdu->target_code_rate = nr_get_code_rate_ul(pusch_pdu->mcs_index,pusch_pdu->mcs_table);
  pusch_pdu->qam_mod_order = nr_get_Qm_ul(pusch_pdu->mcs_index,pusch_pdu->mcs_table);
  if (scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup->msg3_transformPrecoder == NULL)
    pusch_pdu->transformPrecoder = 1;
  else
    pusch_pdu->transformPrecoder = 0;
  pusch_pdu->data_scrambling_id = *scc->physCellId;
  pusch_pdu->nrOfLayers = 1;
  pusch_pdu->ul_dmrs_symb_pos = get_l_prime(nr_of_symbols,mappingtype,pusch_dmrs_pos2,pusch_len1,start_symbol_index, scc->dmrs_TypeA_Position);
  LOG_D(MAC, "MSG3 start_sym:%d NR Symb:%d mappingtype:%d , ul_dmrs_symb_pos:%x\n", start_symbol_index, nr_of_symbols, mappingtype, pusch_pdu->ul_dmrs_symb_pos);
  pusch_pdu->dmrs_config_type = 0;
  pusch_pdu->ul_dmrs_scrambling_id = *scc->physCellId; //If provided and the PUSCH is not a msg3 PUSCH, otherwise, L2 should set this to physical cell id.
  pusch_pdu->scid = 0; //DMRS sequence initialization [TS38.211, sec 6.4.1.1.1]. Should match what is sent in DCI 0_1, otherwise set to 0.
  pusch_pdu->dmrs_ports = 1;  // 6.2.2 in 38.214 only port 0 to be used
  pusch_pdu->num_dmrs_cdm_grps_no_data = 2;  // no data in dmrs symbols as in 6.2.2 in 38.214
  pusch_pdu->resource_alloc = 1; //type 1

  pusch_pdu->rb_start = msg3_first_rb;
  if (msg3_nb_rb > pusch_pdu->bwp_size)
    AssertFatal(1==0,"MSG3 allocated number of RBs exceed the BWP size\n");
  else
    pusch_pdu->rb_size = msg3_nb_rb;
  pusch_pdu->vrb_to_prb_mapping = 0;

  pusch_pdu->frequency_hopping = fh;
  //pusch_pdu->tx_direct_current_location;//The uplink Tx Direct Current location for the carrier. Only values in the value range of this field between 0 and 3299, which indicate the subcarrier index within the carrier corresponding 1o the numerology of the corresponding uplink BWP and value 3300, which indicates "Outside the carrier" and value 3301, which indicates "Undetermined position within the carrier" are used. [TS38.331, UplinkTxDirectCurrentBWP IE]
  pusch_pdu->uplink_frequency_shift_7p5khz = 0;
  //Resource Allocation in time domain
  pusch_pdu->start_symbol_index = start_symbol_index;
  pusch_pdu->nr_of_symbols = nr_of_symbols;
  //Optional Data only included if indicated in pduBitmap
  pusch_pdu->pusch_data.rv_index = nr_rv_round_map[round];
  pusch_pdu->pusch_data.harq_process_id = 0;
  pusch_pdu->pusch_data.new_data_indicator = 1;
  pusch_pdu->pusch_data.num_cb = 0;
  pusch_pdu->pusch_data.tb_size = nr_compute_tbs(pusch_pdu->qam_mod_order,
                                                 pusch_pdu->target_code_rate,
                                                 pusch_pdu->rb_size,
                                                 pusch_pdu->nr_of_symbols,
                                                 12, // nb dmrs set for no data in dmrs symbol
                                                 0, //nb_rb_oh
                                                 0, // to verify tb scaling
                                                 pusch_pdu->nrOfLayers)>>3;

}

void nr_add_msg3(module_id_t module_idP, int CC_id, frame_t frameP, sub_frame_t slotP, NR_RA_t *ra, uint8_t *RAR_pdu)
{
  gNB_MAC_INST                                   *mac = RC.nrmac[module_idP];
  NR_COMMON_channels_t                            *cc = &mac->common_channels[CC_id];
  NR_ServingCellConfigCommon_t                   *scc = cc->ServingCellConfigCommon;

  if (ra->state == RA_IDLE) {
    LOG_W(NR_MAC,"RA is not active for RA %X. skipping msg3 scheduling\n", ra->rnti);
    return;
  }

  uint16_t *vrb_map_UL =
      &RC.nrmac[module_idP]->common_channels[CC_id].vrb_map_UL[ra->Msg3_slot * MAX_BWP_SIZE];
  for (int i = 0; i < ra->msg3_nb_rb; ++i) {
    AssertFatal(!vrb_map_UL[i + ra->msg3_first_rb + ra->msg3_bwp_start],
                "RB %d in %4d.%2d is already taken, cannot allocate Msg3!\n",
                i + ra->msg3_first_rb,
                ra->Msg3_frame,
                ra->Msg3_slot);
    vrb_map_UL[i + ra->msg3_first_rb + ra->msg3_bwp_start] = 1;
  }

  LOG_D(NR_MAC, "[gNB %d][RAPROC] Frame %d, Slot %d : CC_id %d RA is active, Msg3 in (%d,%d)\n", module_idP, frameP, slotP, CC_id, ra->Msg3_frame, ra->Msg3_slot);

  nfapi_nr_ul_tti_request_t *future_ul_tti_req = &RC.nrmac[module_idP]->UL_tti_req_ahead[CC_id][ra->Msg3_slot];
  AssertFatal(future_ul_tti_req->SFN == ra->Msg3_frame
              && future_ul_tti_req->Slot == ra->Msg3_slot,
              "future UL_tti_req's frame.slot %d.%d does not match PUSCH %d.%d\n",
              future_ul_tti_req->SFN,
              future_ul_tti_req->Slot,
              ra->Msg3_frame,
              ra->Msg3_slot);
  future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_type = NFAPI_NR_UL_CONFIG_PUSCH_PDU_TYPE;
  future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_size = sizeof(nfapi_nr_pusch_pdu_t);
  nfapi_nr_pusch_pdu_t *pusch_pdu = &future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pusch_pdu;
  memset(pusch_pdu, 0, sizeof(nfapi_nr_pusch_pdu_t));

  int ibwp_size  = NRRIV2BW(scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
  int scs = scc->uplinkConfigCommon->initialUplinkBWP->genericParameters.subcarrierSpacing;
  int fh = 0;
  int startSymbolAndLength = scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[ra->Msg3_tda_id]->startSymbolAndLength;
  int mappingtype = scc->uplinkConfigCommon->initialUplinkBWP->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[ra->Msg3_tda_id]->mappingType;

  if (ra->CellGroup) {
    AssertFatal(ra->CellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count == 1,
		"downlinkBWP_ToAddModList has %d BWP!\n", ra->CellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count);
    NR_BWP_Uplink_t *ubwp = ra->CellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[ra->bwp_id - 1];

    startSymbolAndLength = ubwp->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[ra->Msg3_tda_id]->startSymbolAndLength;
    mappingtype = ubwp->bwp_Common->pusch_ConfigCommon->choice.setup->pusch_TimeDomainAllocationList->list.array[ra->Msg3_tda_id]->mappingType;
    scs = ubwp->bwp_Common->genericParameters.subcarrierSpacing;
    fh = ubwp->bwp_Dedicated->pusch_Config->choice.setup->frequencyHopping ? 1 : 0;
  }

  LOG_D(NR_MAC, "Frame %d, Subframe %d Adding Msg3 UL Config Request for (%d,%d) : (%d,%d,%d) for rnti: %d\n",
    frameP,
    slotP,
    ra->Msg3_frame,
    ra->Msg3_slot,
    ra->msg3_nb_rb,
    ra->msg3_first_rb,
    ra->msg3_round,
    ra->rnti);

  fill_msg3_pusch_pdu(pusch_pdu,scc,
                      ra->msg3_round,
                      startSymbolAndLength,
                      ra->rnti, scs,
                      ibwp_size, ra->msg3_bwp_start,
                      mappingtype, fh,
                      ra->msg3_first_rb, ra->msg3_nb_rb);
  future_ul_tti_req->n_pdus += 1;

  // calling function to fill rar message
  nr_fill_rar(module_idP, ra, RAR_pdu, pusch_pdu);
}

void nr_generate_Msg2(module_id_t module_idP, int CC_id, frame_t frameP, sub_frame_t slotP, NR_RA_t *ra)
{

  gNB_MAC_INST *nr_mac = RC.nrmac[module_idP];
  NR_COMMON_channels_t *cc = &nr_mac->common_channels[CC_id];

  if ((ra->Msg2_frame == frameP) && (ra->Msg2_slot == slotP)) {

    uint8_t time_domain_assignment = 1;
    uint8_t mcsIndex = 0;
    int rbStart = 0;
    int rbSize = 8;

    NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
    NR_SearchSpace_t *ss = ra->ra_ss;

    NR_BWP_Downlink_t *bwp = NULL;
    NR_ControlResourceSet_t *coreset = NULL;
    NR_BWP_t *genericParameters = NULL;
    NR_PDSCH_TimeDomainResourceAllocationList_t *pdsch_TimeDomainAllocationList=NULL;

    if (ra->CellGroup &&
        ra->CellGroup->spCellConfig &&
        ra->CellGroup->spCellConfig->spCellConfigDedicated &&
        ra->CellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList &&
        ra->CellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[ra->bwp_id-1]) {
      bwp = ra->CellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[ra->bwp_id-1];
      genericParameters = &bwp->bwp_Common->genericParameters;
      pdsch_TimeDomainAllocationList = bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
    }
    else {
      genericParameters= &scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters;
      pdsch_TimeDomainAllocationList = scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
    }

    long BWPStart = 0;
    long BWPSize = 0;
    NR_Type0_PDCCH_CSS_config_t *type0_PDCCH_CSS_config = NULL;
    if(*ss->controlResourceSetId!=0) {
      BWPStart = NRRIV2PRBOFFSET(genericParameters->locationAndBandwidth, MAX_BWP_SIZE);
      BWPSize  = NRRIV2BW(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
    } else {
      type0_PDCCH_CSS_config = &nr_mac->type0_PDCCH_CSS_config[ra->beam_id];
      BWPStart = type0_PDCCH_CSS_config->cset_start_rb;
      BWPSize = type0_PDCCH_CSS_config->num_rbs;
    }

    coreset = get_coreset(module_idP, scc, bwp, ss, NR_SearchSpace__searchSpaceType_PR_common);

    AssertFatal(coreset!=NULL,"Coreset cannot be null for RA-Msg2\n");

    uint16_t *vrb_map = cc[CC_id].vrb_map;
    for (int i = 0; (i < rbSize) && (rbStart <= (BWPSize - rbSize)); i++) {
      if (vrb_map[BWPStart + rbStart + i]) {
        rbStart += i;
        i = 0;
      }
    }

    if (rbStart > (BWPSize - rbSize)) {
      LOG_E(NR_MAC, "%s(): cannot find free vrb_map for RA RNTI %04x!\n", __func__, ra->RA_rnti);
      return;
    }

    // Checking if the DCI allocation is feasible in current subframe
    nfapi_nr_dl_tti_request_body_t *dl_req = &nr_mac->DL_req[CC_id].dl_tti_request_body;
    if (dl_req->nPDUs > NFAPI_NR_MAX_DL_TTI_PDUS - 2) {
      LOG_I(NR_MAC, "[RAPROC] Subframe %d: FAPI DL structure is full, skip scheduling UE %d\n", slotP, ra->RA_rnti);
      return;
    }

    uint8_t aggregation_level;
    uint8_t nr_of_candidates;
    for (int i=0; i<5; i++) {
      // for now taking the lowest value among the available aggregation levels
      find_aggregation_candidates(&aggregation_level, &nr_of_candidates, ss, 1<<i);
      if(nr_of_candidates>0) break;
    }
    AssertFatal(nr_of_candidates>0,"nr_of_candidates is 0\n");
    int CCEIndex = allocate_nr_CCEs(nr_mac, bwp, coreset, aggregation_level,0,0,nr_of_candidates);
    if (CCEIndex < 0) {
      LOG_E(NR_MAC, "%s(): cannot find free CCE for RA RNTI 0x%04x!\n", __func__, ra->rnti);
      return;
    }

    // Calculate number of symbols
    int startSymbolIndex, nrOfSymbols;
    const int startSymbolAndLength = pdsch_TimeDomainAllocationList->list.array[time_domain_assignment]->startSymbolAndLength;
    SLIV2SL(startSymbolAndLength, &startSymbolIndex, &nrOfSymbols);
    AssertFatal(startSymbolIndex >= 0, "StartSymbolIndex is negative\n");

    LOG_D(NR_MAC,"Msg2 startSymbolIndex.nrOfSymbols %d.%d\n",startSymbolIndex,nrOfSymbols);

    int mappingtype = pdsch_TimeDomainAllocationList->list.array[time_domain_assignment]->mappingType;

    // look up the PDCCH PDU for this CC, BWP, and CORESET. If it does not exist, create it. This is especially
    // important if we have multiple RAs, and the DLSCH has to reuse them, so we need to mark them
    const int bwpid = bwp ? bwp->bwp_Id : 0;
    const int coresetid = coreset->controlResourceSetId;
    nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15 = nr_mac->pdcch_pdu_idx[CC_id][bwpid][coresetid];
    if (!pdcch_pdu_rel15) {
      nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdcch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs];
      memset(dl_tti_pdcch_pdu, 0, sizeof(nfapi_nr_dl_tti_request_pdu_t));
      dl_tti_pdcch_pdu->PDUType = NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE;
      dl_tti_pdcch_pdu->PDUSize = (uint8_t)(2 + sizeof(nfapi_nr_dl_tti_pdcch_pdu));
      dl_req->nPDUs += 1;
      pdcch_pdu_rel15 = &dl_tti_pdcch_pdu->pdcch_pdu.pdcch_pdu_rel15;
      nr_configure_pdcch(nr_mac, pdcch_pdu_rel15, ss, coreset, scc, genericParameters, NULL);
      nr_mac->pdcch_pdu_idx[CC_id][bwpid][coresetid] = pdcch_pdu_rel15;
    }

    nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdsch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs];
    memset((void *)dl_tti_pdsch_pdu,0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
    dl_tti_pdsch_pdu->PDUType = NFAPI_NR_DL_TTI_PDSCH_PDU_TYPE;
    dl_tti_pdsch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdsch_pdu));
    dl_req->nPDUs+=1;
    nfapi_nr_dl_tti_pdsch_pdu_rel15_t *pdsch_pdu_rel15 = &dl_tti_pdsch_pdu->pdsch_pdu.pdsch_pdu_rel15;

    LOG_I(NR_MAC,"[gNB %d][RAPROC] CC_id %d Frame %d, slotP %d: Generating RA-Msg2 DCI, rnti 0x%04x, state %d, CoreSetType %d\n",
          module_idP, CC_id, frameP, slotP, ra->RA_rnti, ra->state,pdcch_pdu_rel15->CoreSetType);

    // SCF222: PDU index incremented for each PDSCH PDU sent in TX control message. This is used to associate control
    // information to data and is reset every slot.
    const int pduindex = nr_mac->pdu_index[CC_id]++;

    uint8_t mcsTableIdx = 0;
    if (bwp &&
        bwp->bwp_Dedicated &&
        bwp->bwp_Dedicated->pdsch_Config &&
        bwp->bwp_Dedicated->pdsch_Config->choice.setup &&
        bwp->bwp_Dedicated->pdsch_Config->choice.setup->mcs_Table) {
      if (*bwp->bwp_Dedicated->pdsch_Config->choice.setup->mcs_Table == 0)
        mcsTableIdx = 1;
      else
        mcsTableIdx = 2;
    }
    else mcsTableIdx = 0;

    int dmrsConfigType=0;
    if (bwp &&
        bwp->bwp_Dedicated &&
        bwp->bwp_Dedicated->pdsch_Config &&
        bwp->bwp_Dedicated->pdsch_Config->choice.setup &&
        bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA &&
        bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup &&
        bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type)
      dmrsConfigType = 1;

    pdsch_pdu_rel15->pduBitmap = 0;
    pdsch_pdu_rel15->rnti = ra->RA_rnti;
    pdsch_pdu_rel15->pduIndex = pduindex;
    pdsch_pdu_rel15->BWPSize  = BWPSize;
    pdsch_pdu_rel15->BWPStart = BWPStart;
    pdsch_pdu_rel15->SubcarrierSpacing = genericParameters->subcarrierSpacing;
    pdsch_pdu_rel15->CyclicPrefix = 0;
    pdsch_pdu_rel15->NrOfCodewords = 1;
    pdsch_pdu_rel15->targetCodeRate[0] = nr_get_code_rate_dl(mcsIndex,mcsTableIdx);
    pdsch_pdu_rel15->qamModOrder[0] = 2;
    pdsch_pdu_rel15->mcsIndex[0] = mcsIndex;
    pdsch_pdu_rel15->mcsTable[0] = mcsTableIdx;
    pdsch_pdu_rel15->rvIndex[0] = 0;
    pdsch_pdu_rel15->dataScramblingId = *scc->physCellId;
    pdsch_pdu_rel15->nrOfLayers = 1;
    pdsch_pdu_rel15->transmissionScheme = 0;
    pdsch_pdu_rel15->refPoint = 0;
    pdsch_pdu_rel15->dmrsConfigType = dmrsConfigType;
    pdsch_pdu_rel15->dlDmrsScramblingId = *scc->physCellId;
    pdsch_pdu_rel15->SCID = 0;
    pdsch_pdu_rel15->numDmrsCdmGrpsNoData = nrOfSymbols <= 2 ? 1 : 2;
    pdsch_pdu_rel15->dmrsPorts = 1;
    pdsch_pdu_rel15->resourceAlloc = 1;
    pdsch_pdu_rel15->rbStart = rbStart;
    pdsch_pdu_rel15->rbSize = rbSize;
    pdsch_pdu_rel15->VRBtoPRBMapping = 0;
    pdsch_pdu_rel15->StartSymbolIndex = startSymbolIndex;
    pdsch_pdu_rel15->NrOfSymbols = nrOfSymbols;
    pdsch_pdu_rel15->dlDmrsSymbPos = fill_dmrs_mask(NULL,
                                                    nr_mac->common_channels->ServingCellConfigCommon->dmrs_TypeA_Position,
                                                    nrOfSymbols,
                                                    startSymbolIndex,
                                                    mappingtype);

    int x_Overhead = 0;
    uint8_t tb_scaling = 0;
    nr_get_tbs_dl(&dl_tti_pdsch_pdu->pdsch_pdu, x_Overhead, pdsch_pdu_rel15->numDmrsCdmGrpsNoData, tb_scaling);

    // Fill PDCCH DL DCI PDU
    nfapi_nr_dl_dci_pdu_t *dci_pdu = &pdcch_pdu_rel15->dci_pdu[pdcch_pdu_rel15->numDlDci];
    pdcch_pdu_rel15->numDlDci++;
    dci_pdu->RNTI = ra->RA_rnti;
    dci_pdu->ScramblingId = *scc->physCellId;
    dci_pdu->ScramblingRNTI = 0;
    dci_pdu->AggregationLevel = aggregation_level;
    dci_pdu->CceIndex = CCEIndex;
    dci_pdu->beta_PDCCH_1_0 = 0;
    dci_pdu->powerControlOffsetSS = 1;

    dci_pdu_rel15_t dci_payload;
    dci_payload.frequency_domain_assignment.val = PRBalloc_to_locationandbandwidth0(pdsch_pdu_rel15->rbSize,
                                                                                    pdsch_pdu_rel15->rbStart,
                                                                                    BWPSize);

    LOG_D(NR_MAC,"Msg2 rbSize.rbStart.BWPsize %d.%d.%ld\n",pdsch_pdu_rel15->rbSize,
          pdsch_pdu_rel15->rbStart,
          BWPSize);

    dci_payload.time_domain_assignment.val = time_domain_assignment;
    dci_payload.vrb_to_prb_mapping.val = 0;
    dci_payload.mcs = pdsch_pdu_rel15->mcsIndex[0];
    dci_payload.tb_scaling = tb_scaling;

    LOG_D(NR_MAC,
          "[RAPROC] DCI type 1 payload: freq_alloc %d (%d,%d,%ld), time_alloc %d, vrb to prb %d, mcs %d tb_scaling %d \n",
          dci_payload.frequency_domain_assignment.val,
          pdsch_pdu_rel15->rbStart,
          pdsch_pdu_rel15->rbSize,
          BWPSize,
          dci_payload.time_domain_assignment.val,
          dci_payload.vrb_to_prb_mapping.val,
          dci_payload.mcs,
          dci_payload.tb_scaling);

    LOG_D(NR_MAC,
          "[RAPROC] DCI params: rnti 0x%x, rnti_type %d, dci_format %d coreset params: FreqDomainResource %llx, start_symbol %d  n_symb %d\n",
          pdcch_pdu_rel15->dci_pdu[0].RNTI,
          NR_RNTI_RA,
          NR_DL_DCI_FORMAT_1_0,
          *(unsigned long long *)pdcch_pdu_rel15->FreqDomainResource,
          pdcch_pdu_rel15->StartSymbolIndex,
          pdcch_pdu_rel15->DurationSymbols);

    fill_dci_pdu_rel15(scc,
                       ra->CellGroup,
                       &pdcch_pdu_rel15->dci_pdu[pdcch_pdu_rel15->numDlDci - 1],
                       &dci_payload,
                       NR_DL_DCI_FORMAT_1_0,
                       NR_RNTI_RA,
                       BWPSize,
                       bwpid);

    // DL TX request
    nfapi_nr_pdu_t *tx_req = &nr_mac->TX_req[CC_id].pdu_list[nr_mac->TX_req[CC_id].Number_of_PDUs];

    // Program UL processing for Msg3
    NR_BWP_Uplink_t *ubwp = ra->CellGroup ?
      ra->CellGroup->spCellConfig->spCellConfigDedicated->uplinkConfig->uplinkBWP_ToAddModList->list.array[ra->bwp_id-1] :
      NULL;
    nr_get_Msg3alloc(module_idP, CC_id, scc, ubwp, slotP, frameP, ra, nr_mac->tdd_beam_association);
    nr_add_msg3(module_idP, CC_id, frameP, slotP, ra, (uint8_t *) &tx_req->TLVs[0].value.direct[0]);

    if(ra->cfra) {
      LOG_I(NR_MAC, "Frame %d, Subframe %d: Setting RA-Msg3 reception for Frame %d Subframe %d\n", frameP, slotP, ra->Msg3_frame, ra->Msg3_slot);
    }

    T(T_GNB_MAC_DL_RAR_PDU_WITH_DATA, T_INT(module_idP), T_INT(CC_id), T_INT(ra->RA_rnti), T_INT(frameP),
      T_INT(slotP), T_INT(0), T_BUFFER(&tx_req->TLVs[0].value.direct[0], tx_req->TLVs[0].length));

    tx_req->PDU_length = pdsch_pdu_rel15->TBSize[0];
    tx_req->PDU_index = pduindex;
    tx_req->num_TLV = 1;
    tx_req->TLVs[0].length = tx_req->PDU_length + 2;
    nr_mac->TX_req[CC_id].SFN = frameP;
    nr_mac->TX_req[CC_id].Number_of_PDUs++;
    nr_mac->TX_req[CC_id].Slot = slotP;

    // Mark the corresponding RBs as used
    for (int rb = 0; rb < rbSize; rb++) {
      vrb_map[BWPStart + rb + rbStart] = 1;
    }

    ra->state = WAIT_Msg3;
    LOG_D(NR_MAC,"[gNB %d][RAPROC] Frame %d, Subframe %d: RA state %d\n", module_idP, frameP, slotP, ra->state);
  }
}

void nr_generate_Msg4(module_id_t module_idP, int CC_id, frame_t frameP, sub_frame_t slotP, NR_RA_t *ra) {

  gNB_MAC_INST *nr_mac = RC.nrmac[module_idP];
  NR_COMMON_channels_t *cc = &nr_mac->common_channels[CC_id];

  if (ra->Msg4_frame == frameP && ra->Msg4_slot == slotP ) {

    uint8_t time_domain_assignment = 0;
    uint8_t mcsIndex = 0;

    NR_ServingCellConfigCommon_t *scc = cc->ServingCellConfigCommon;
    NR_SearchSpace_t *ss = ra->ra_ss;

    NR_BWP_Downlink_t *bwp = NULL;
    NR_ControlResourceSet_t *coreset = NULL;
    NR_PDSCH_TimeDomainResourceAllocationList_t *pdsch_TimeDomainAllocationList=NULL;

    if (ra->CellGroup &&
        ra->CellGroup->spCellConfig &&
        ra->CellGroup->spCellConfig->spCellConfigDedicated &&
        ra->CellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList &&
        ra->CellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[ra->bwp_id-1]) {
      bwp = ra->CellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[ra->bwp_id-1];
      pdsch_TimeDomainAllocationList = bwp->bwp_Common->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
    }
    else {
      pdsch_TimeDomainAllocationList = scc->downlinkConfigCommon->initialDownlinkBWP->pdsch_ConfigCommon->choice.setup->pdsch_TimeDomainAllocationList;
    }

    coreset = get_coreset(module_idP, scc, bwp, ss, NR_SearchSpace__searchSpaceType_PR_common);

    AssertFatal(coreset!=NULL,"Coreset cannot be null for RA-Msg4\n");

    rnti_t tc_rnti = ra->rnti;
    // If UE is known by the network, C-RNTI to be used instead of TC-RNTI
    if(ra->msg3_dcch_dtch) {
      ra->rnti = ra->crnti;
    }

    int UE_id = find_nr_UE_id(module_idP, ra->rnti);
    NR_UE_info_t *UE_info = &nr_mac->UE_info;
    NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];

    NR_BWP_t *genericParameters = bwp ? & bwp->bwp_Common->genericParameters : &scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters;

    long BWPStart = 0;
    long BWPSize = 0;
    NR_Type0_PDCCH_CSS_config_t *type0_PDCCH_CSS_config = NULL;
    if(*ss->controlResourceSetId!=0) {
      BWPStart = NRRIV2PRBOFFSET(genericParameters->locationAndBandwidth, MAX_BWP_SIZE);
      BWPSize  = NRRIV2BW(scc->downlinkConfigCommon->initialDownlinkBWP->genericParameters.locationAndBandwidth, MAX_BWP_SIZE);
    } else {
      type0_PDCCH_CSS_config = &nr_mac->type0_PDCCH_CSS_config[ra->beam_id];
      BWPStart = type0_PDCCH_CSS_config->cset_start_rb;
      BWPSize = type0_PDCCH_CSS_config->num_rbs;
    }

    /* get the PID of a HARQ process awaiting retrnasmission, or -1 otherwise */
    int current_harq_pid = sched_ctrl->retrans_dl_harq.head;
    // HARQ management
    if (current_harq_pid < 0) {
      AssertFatal(sched_ctrl->available_dl_harq.head >= 0,
                  "UE context not initialized: no HARQ processes found\n");
      current_harq_pid = sched_ctrl->available_dl_harq.head;
      remove_front_nr_list(&sched_ctrl->available_dl_harq);
    }
    NR_UE_harq_t *harq = &sched_ctrl->harq_processes[current_harq_pid];
    DevAssert(!harq->is_waiting);
    add_tail_nr_list(&sched_ctrl->feedback_dl_harq, current_harq_pid);
    harq->is_waiting = true;
    ra->harq_pid = current_harq_pid;

    // Remove UE associated to TC-RNTI
    if(harq->round==0 && ra->msg3_dcch_dtch) {
      mac_remove_nr_ue(module_idP, tc_rnti);
    }

    // get CCEindex, needed also for PUCCH and then later for PDCCH
    uint8_t aggregation_level;
    uint8_t nr_of_candidates;
    for (int i=0; i<5; i++) {
      // for now taking the lowest value among the available aggregation levels
      find_aggregation_candidates(&aggregation_level, &nr_of_candidates, ss, 1<<i);
      if(nr_of_candidates>0) break;
    }
    AssertFatal(nr_of_candidates>0,"nr_of_candidates is 0\n");
    int CCEIndex = allocate_nr_CCEs(nr_mac, bwp, coreset, aggregation_level,0,0,nr_of_candidates);
    if (CCEIndex < 0) {
      LOG_E(NR_MAC, "%s(): cannot find free CCE for RA RNTI 0x%04x!\n", __func__, ra->rnti);
      return;
    }

    int n_rb=0;
    for (int i=0;i<6;i++)
      for (int j=0;j<8;j++) {
        n_rb+=((coreset->frequencyDomainResources.buf[i]>>j)&1);
      }
    n_rb*=6;
    const uint16_t N_cce = n_rb * coreset->duration / NR_NB_REG_PER_CCE;
    const int delta_PRI=0;
    int r_pucch = ((CCEIndex<<1)/N_cce)+(delta_PRI<<1);

    LOG_D(NR_MAC,"[RAPROC] Msg4 r_pucch %d (CCEIndex %d, N_cce %d, nb_of_candidates %d,delta_PRI %d)\n",r_pucch,CCEIndex,N_cce,nr_of_candidates,delta_PRI);
    int alloc = nr_acknack_scheduling(module_idP, UE_id, frameP, slotP, r_pucch, 1);
    AssertFatal(alloc>=0,"Couldn't find a pucch allocation for ack nack (msg4)\n");
    NR_sched_pucch_t *pucch = &sched_ctrl->sched_pucch[alloc];
    harq->feedback_slot = pucch->ul_slot;
    harq->feedback_frame = pucch->frame;

    uint8_t *buf = (uint8_t *) harq->tb;
    // Bytes to be transmitted
    if (harq->round == 0) {
      if (ra->msg3_dcch_dtch) {
        // If the UE used MSG3 to transfer a DCCH or DTCH message, then contention resolution is successful if the UE receives a PDCCH transmission which has its CRC bits scrambled by the C-RNTI
        // Just send padding LCID
        ra->mac_pdu_length = 0;
      } else {
        uint16_t mac_pdu_length = nr_write_ce_dlsch_pdu(module_idP, nr_mac->sched_ctrlCommon, buf, 255, ra->cont_res_id);
        LOG_D(NR_MAC,"Encoded contention resolution mac_pdu_length %d\n",mac_pdu_length);
        uint16_t mac_sdu_length = mac_rrc_nr_data_req(module_idP, CC_id, frameP, CCCH, ra->rnti, 1, &buf[mac_pdu_length+2]);
        ((NR_MAC_SUBHEADER_SHORT *) &buf[mac_pdu_length])->R = 0;
        ((NR_MAC_SUBHEADER_SHORT *) &buf[mac_pdu_length])->F = 0;
        ((NR_MAC_SUBHEADER_SHORT *) &buf[mac_pdu_length])->LCID = DL_SCH_LCID_CCCH;
        ((NR_MAC_SUBHEADER_SHORT *) &buf[mac_pdu_length])->L = mac_sdu_length;
        ra->mac_pdu_length = mac_pdu_length + mac_sdu_length + sizeof(NR_MAC_SUBHEADER_SHORT);
        LOG_D(NR_MAC,"Encoded RRCSetup Piggyback (%d + %d bytes), mac_pdu_length %d\n", mac_sdu_length, (int)sizeof(NR_MAC_SUBHEADER_SHORT), ra->mac_pdu_length);
      }
    }

    // Calculate number of symbols
    int startSymbolIndex, nrOfSymbols;
    const int startSymbolAndLength = pdsch_TimeDomainAllocationList->list.array[time_domain_assignment]->startSymbolAndLength;
    SLIV2SL(startSymbolAndLength, &startSymbolIndex, &nrOfSymbols);
    AssertFatal(startSymbolIndex >= 0, "StartSymbolIndex is negative\n");

    int mappingtype = pdsch_TimeDomainAllocationList->list.array[time_domain_assignment]->mappingType;

    uint16_t dlDmrsSymbPos = fill_dmrs_mask(NULL,
                                            scc->dmrs_TypeA_Position,
                                            nrOfSymbols,
                                            startSymbolIndex,
                                            mappingtype);

    uint16_t N_DMRS_SLOT = get_num_dmrs(dlDmrsSymbPos);

    long dmrsConfigType = bwp!=NULL ? (bwp->bwp_Dedicated->pdsch_Config->choice.setup->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup->dmrs_Type == NULL ? 0 : 1) : 0;

    nr_mac->sched_ctrlCommon->pdsch_semi_static.numDmrsCdmGrpsNoData = 2;
    if (nrOfSymbols == 2) {
      nr_mac->sched_ctrlCommon->pdsch_semi_static.numDmrsCdmGrpsNoData = 1;
    }

    AssertFatal(nr_mac->sched_ctrlCommon->pdsch_semi_static.numDmrsCdmGrpsNoData == 1
                || nr_mac->sched_ctrlCommon->pdsch_semi_static.numDmrsCdmGrpsNoData == 2,
                "nr_mac->schedCtrlCommon->pdsch_semi_static.numDmrsCdmGrpsNoData %d is not possible",
                nr_mac->sched_ctrlCommon->pdsch_semi_static.numDmrsCdmGrpsNoData);

    uint8_t N_PRB_DMRS = 0;
    if (dmrsConfigType==NFAPI_NR_DMRS_TYPE1) {
      N_PRB_DMRS = nr_mac->sched_ctrlCommon->pdsch_semi_static.numDmrsCdmGrpsNoData * 6;
    }
    else {
      N_PRB_DMRS = nr_mac->sched_ctrlCommon->pdsch_semi_static.numDmrsCdmGrpsNoData * 4;
    }

    uint8_t mcsTableIdx = 0;
    if (bwp &&
        bwp->bwp_Dedicated &&
        bwp->bwp_Dedicated->pdsch_Config &&
        bwp->bwp_Dedicated->pdsch_Config->choice.setup &&
        bwp->bwp_Dedicated->pdsch_Config->choice.setup->mcs_Table) {
      if (*bwp->bwp_Dedicated->pdsch_Config->choice.setup->mcs_Table == 0)
        mcsTableIdx = 1;
      else
        mcsTableIdx = 2;
    }
    else mcsTableIdx = 0;

    int rbStart = 0;
    int rbSize = 0;
    uint8_t tb_scaling = 0;
    uint16_t *vrb_map = cc[CC_id].vrb_map;
    do {
      rbSize++;
      LOG_D(NR_MAC,"Calling nr_compute_tbs with N_PRB_DMRS %d, N_DMRS_SLOT %d\n",N_PRB_DMRS,N_DMRS_SLOT);
      harq->tb_size = nr_compute_tbs(nr_get_Qm_dl(mcsIndex, mcsTableIdx),
                           nr_get_code_rate_dl(mcsIndex, mcsTableIdx),
                           rbSize, nrOfSymbols, N_PRB_DMRS * N_DMRS_SLOT, 0, tb_scaling,1) >> 3;
    } while (rbSize < BWPSize && harq->tb_size < ra->mac_pdu_length);

    int i = 0;
    while ((i < rbSize) && (rbStart + rbSize <= BWPSize)) {
      if (vrb_map[BWPStart + rbStart + i]) {
        rbStart += i+1;
        i = 0;
      } else {
        i++;
      }
    }

    if (rbStart > (BWPSize - rbSize)) {
      LOG_E(NR_MAC, "%s(): cannot find free vrb_map for RNTI %04x!\n", __func__, ra->rnti);
      return;
    }

    // Checking if the DCI allocation is feasible in current subframe
    nfapi_nr_dl_tti_request_body_t *dl_req = &nr_mac->DL_req[CC_id].dl_tti_request_body;
    if (dl_req->nPDUs > NFAPI_NR_MAX_DL_TTI_PDUS - 2) {
      LOG_I(NR_MAC, "[RAPROC] Subframe %d: FAPI DL structure is full, skip scheduling UE %d\n", slotP, ra->rnti);
      return;
    }


    // look up the PDCCH PDU for this CC, BWP, and CORESET. If it does not exist, create it. This is especially
    // important if we have multiple RAs, and the DLSCH has to reuse them, so we need to mark them
    const int bwpid = bwp ? bwp->bwp_Id : 0;
    const int coresetid = coreset->controlResourceSetId;
    nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15 = nr_mac->pdcch_pdu_idx[CC_id][bwpid][coresetid];
    if (!pdcch_pdu_rel15) {
      nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdcch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs];
      memset(dl_tti_pdcch_pdu, 0, sizeof(nfapi_nr_dl_tti_request_pdu_t));
      dl_tti_pdcch_pdu->PDUType = NFAPI_NR_DL_TTI_PDCCH_PDU_TYPE;
      dl_tti_pdcch_pdu->PDUSize = (uint8_t)(2 + sizeof(nfapi_nr_dl_tti_pdcch_pdu));
      dl_req->nPDUs += 1;
      pdcch_pdu_rel15 = &dl_tti_pdcch_pdu->pdcch_pdu.pdcch_pdu_rel15;
      nr_configure_pdcch(nr_mac, pdcch_pdu_rel15, ss, coreset, scc, genericParameters, NULL);
      nr_mac->pdcch_pdu_idx[CC_id][bwpid][coresetid] = pdcch_pdu_rel15;
    }

    nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdsch_pdu = &dl_req->dl_tti_pdu_list[dl_req->nPDUs];
    memset((void *)dl_tti_pdsch_pdu,0,sizeof(nfapi_nr_dl_tti_request_pdu_t));
    dl_tti_pdsch_pdu->PDUType = NFAPI_NR_DL_TTI_PDSCH_PDU_TYPE;
    dl_tti_pdsch_pdu->PDUSize = (uint8_t)(2+sizeof(nfapi_nr_dl_tti_pdsch_pdu));
    dl_req->nPDUs+=1;
    nfapi_nr_dl_tti_pdsch_pdu_rel15_t *pdsch_pdu_rel15 = &dl_tti_pdsch_pdu->pdsch_pdu.pdsch_pdu_rel15;

    LOG_I(NR_MAC,"[gNB %d] [RAPROC] CC_id %d Frame %d, slotP %d: Generating RA-Msg4 DCI, state %d\n", module_idP, CC_id, frameP, slotP, ra->state);

    // SCF222: PDU index incremented for each PDSCH PDU sent in TX control message. This is used to associate control
    // information to data and is reset every slot.
    const int pduindex = nr_mac->pdu_index[CC_id]++;

    pdsch_pdu_rel15->pduBitmap = 0;
    pdsch_pdu_rel15->rnti = ra->rnti;
    pdsch_pdu_rel15->pduIndex = pduindex;
    pdsch_pdu_rel15->BWPSize  = BWPSize;
    pdsch_pdu_rel15->BWPStart = BWPStart;
    pdsch_pdu_rel15->SubcarrierSpacing = genericParameters->subcarrierSpacing;
    pdsch_pdu_rel15->CyclicPrefix = 0;
    pdsch_pdu_rel15->NrOfCodewords = 1;
    pdsch_pdu_rel15->targetCodeRate[0] = nr_get_code_rate_dl(mcsIndex,mcsTableIdx);
    pdsch_pdu_rel15->qamModOrder[0] = 2;
    pdsch_pdu_rel15->mcsIndex[0] = mcsIndex;
    pdsch_pdu_rel15->mcsTable[0] = mcsTableIdx;
    pdsch_pdu_rel15->rvIndex[0] = nr_rv_round_map[harq->round];
    pdsch_pdu_rel15->dataScramblingId = *scc->physCellId;
    pdsch_pdu_rel15->nrOfLayers = 1;
    pdsch_pdu_rel15->transmissionScheme = 0;
    pdsch_pdu_rel15->refPoint = 0;
    pdsch_pdu_rel15->dmrsConfigType = dmrsConfigType;
    pdsch_pdu_rel15->dlDmrsScramblingId = *scc->physCellId;
    pdsch_pdu_rel15->SCID = 0;
    pdsch_pdu_rel15->numDmrsCdmGrpsNoData = nrOfSymbols <= 2 ? 1 : 2;
    pdsch_pdu_rel15->dmrsPorts = 1;
    pdsch_pdu_rel15->resourceAlloc = 1;
    pdsch_pdu_rel15->rbStart = rbStart;
    pdsch_pdu_rel15->rbSize = rbSize;
    pdsch_pdu_rel15->VRBtoPRBMapping = 0;
    pdsch_pdu_rel15->StartSymbolIndex = startSymbolIndex;
    pdsch_pdu_rel15->NrOfSymbols = nrOfSymbols;
    pdsch_pdu_rel15->dlDmrsSymbPos = dlDmrsSymbPos;

    int x_Overhead = 0;
    nr_get_tbs_dl(&dl_tti_pdsch_pdu->pdsch_pdu, x_Overhead, pdsch_pdu_rel15->numDmrsCdmGrpsNoData, tb_scaling);

    pdsch_pdu_rel15->precodingAndBeamforming.num_prgs=1;
    pdsch_pdu_rel15->precodingAndBeamforming.prg_size=275;
    pdsch_pdu_rel15->precodingAndBeamforming.dig_bf_interfaces=1;
    pdsch_pdu_rel15->precodingAndBeamforming.prgs_list[0].pm_idx = 0;
    pdsch_pdu_rel15->precodingAndBeamforming.prgs_list[0].dig_bf_interface_list[0].beam_idx = ra->beam_id;

    /* Fill PDCCH DL DCI PDU */
    nfapi_nr_dl_dci_pdu_t *dci_pdu = &pdcch_pdu_rel15->dci_pdu[pdcch_pdu_rel15->numDlDci];
    pdcch_pdu_rel15->numDlDci++;
    dci_pdu->RNTI = ra->rnti;
    dci_pdu->ScramblingId = *scc->physCellId;
    dci_pdu->ScramblingRNTI = 0;
    dci_pdu->AggregationLevel = aggregation_level;
    dci_pdu->CceIndex = CCEIndex;
    dci_pdu->beta_PDCCH_1_0 = 0;
    dci_pdu->powerControlOffsetSS = 1;

    dci_pdu_rel15_t dci_payload;
    dci_payload.frequency_domain_assignment.val = PRBalloc_to_locationandbandwidth0(pdsch_pdu_rel15->rbSize,
                                                                                    pdsch_pdu_rel15->rbStart,
                                                                                    BWPSize);

    dci_payload.format_indicator = 1;
    dci_payload.time_domain_assignment.val = time_domain_assignment;
    dci_payload.vrb_to_prb_mapping.val = 0;
    dci_payload.mcs = pdsch_pdu_rel15->mcsIndex[0];
    dci_payload.tb_scaling = tb_scaling;
    dci_payload.rv = pdsch_pdu_rel15->rvIndex[0];
    dci_payload.harq_pid = current_harq_pid;
    dci_payload.ndi = harq->ndi;
    dci_payload.dai[0].val = (pucch->dai_c-1)&3;
    dci_payload.tpc = sched_ctrl->tpc1; // TPC for PUCCH: table 7.2.1-1 in 38.213
    dci_payload.pucch_resource_indicator = delta_PRI; // This is delta_PRI from 9.2.1 in 38.213
    dci_payload.pdsch_to_harq_feedback_timing_indicator.val = pucch->timing_indicator;

    LOG_D(NR_MAC,
          "[RAPROC] DCI 1_0 payload: freq_alloc %d (%d,%d,%d), time_alloc %d, vrb to prb %d, mcs %d tb_scaling %d pucchres %d harqtiming %d\n",
          dci_payload.frequency_domain_assignment.val,
          pdsch_pdu_rel15->rbStart,
          pdsch_pdu_rel15->rbSize,
          pdsch_pdu_rel15->BWPSize,
          dci_payload.time_domain_assignment.val,
          dci_payload.vrb_to_prb_mapping.val,
          dci_payload.mcs,
          dci_payload.tb_scaling,
          dci_payload.pucch_resource_indicator,
          dci_payload.pdsch_to_harq_feedback_timing_indicator.val);

    LOG_D(NR_MAC,
          "[RAPROC] DCI params: rnti 0x%x, rnti_type %d, dci_format %d coreset params: FreqDomainResource %llx, start_symbol %d  n_symb %d, BWPsize %d\n",
          pdcch_pdu_rel15->dci_pdu[0].RNTI,
          NR_RNTI_TC,
          NR_DL_DCI_FORMAT_1_0,
          (unsigned long long)pdcch_pdu_rel15->FreqDomainResource,
          pdcch_pdu_rel15->StartSymbolIndex,
          pdcch_pdu_rel15->DurationSymbols,
          pdsch_pdu_rel15->BWPSize);

    fill_dci_pdu_rel15(scc,
                       ra->CellGroup,
                       &pdcch_pdu_rel15->dci_pdu[pdcch_pdu_rel15->numDlDci - 1],
                       &dci_payload,
                       NR_DL_DCI_FORMAT_1_0,
                       NR_RNTI_TC,
                       pdsch_pdu_rel15->BWPSize,
                       bwpid);

    // Add padding header and zero rest out if there is space left
    if (ra->mac_pdu_length < harq->tb_size) {
      NR_MAC_SUBHEADER_FIXED *padding = (NR_MAC_SUBHEADER_FIXED *) &buf[ra->mac_pdu_length];
      padding->R = 0;
      padding->LCID = DL_SCH_LCID_PADDING;
      for(int k = ra->mac_pdu_length+1; k<harq->tb_size; k++) {
        buf[k] = 0;
      }
    }

    T(T_GNB_MAC_DL_PDU_WITH_DATA, T_INT(module_idP), T_INT(CC_id), T_INT(ra->rnti),
      T_INT(frameP), T_INT(slotP), T_INT(current_harq_pid), T_BUFFER(harq->tb, harq->tb_size));

    // DL TX request
    nfapi_nr_pdu_t *tx_req = &nr_mac->TX_req[CC_id].pdu_list[nr_mac->TX_req[CC_id].Number_of_PDUs];
    memcpy(tx_req->TLVs[0].value.direct, harq->tb, sizeof(uint8_t) * harq->tb_size);
    tx_req->PDU_length =  harq->tb_size;
    tx_req->PDU_index = pduindex;
    tx_req->num_TLV = 1;
    tx_req->TLVs[0].length =  harq->tb_size + 2;
    nr_mac->TX_req[CC_id].SFN = frameP;
    nr_mac->TX_req[CC_id].Number_of_PDUs++;
    nr_mac->TX_req[CC_id].Slot = slotP;

    // Mark the corresponding RBs as used
    for (int rb = 0; rb < pdsch_pdu_rel15->rbSize; rb++) {
      vrb_map[BWPStart + rb + pdsch_pdu_rel15->rbStart] = 1;
    }

    LOG_D(NR_MAC,"BWPSize: %i\n", pdcch_pdu_rel15->BWPSize);
    LOG_D(NR_MAC,"BWPStart: %i\n", pdcch_pdu_rel15->BWPStart);
    LOG_D(NR_MAC,"SubcarrierSpacing: %i\n", pdcch_pdu_rel15->SubcarrierSpacing);
    LOG_D(NR_MAC,"CyclicPrefix: %i\n", pdcch_pdu_rel15->CyclicPrefix);
    LOG_D(NR_MAC,"StartSymbolIndex: %i\n", pdcch_pdu_rel15->StartSymbolIndex);
    LOG_D(NR_MAC,"DurationSymbols: %i\n", pdcch_pdu_rel15->DurationSymbols);
    for(int n=0;n<6;n++) LOG_D(NR_MAC,"FreqDomainResource[%i]: %x\n",n, pdcch_pdu_rel15->FreqDomainResource[n]);
    LOG_D(NR_MAC,"CceRegMappingType: %i\n", pdcch_pdu_rel15->CceRegMappingType);
    LOG_D(NR_MAC,"RegBundleSize: %i\n", pdcch_pdu_rel15->RegBundleSize);
    LOG_D(NR_MAC,"InterleaverSize: %i\n", pdcch_pdu_rel15->InterleaverSize);
    LOG_D(NR_MAC,"CoreSetType: %i\n", pdcch_pdu_rel15->CoreSetType);
    LOG_D(NR_MAC,"ShiftIndex: %i\n", pdcch_pdu_rel15->ShiftIndex);
    LOG_D(NR_MAC,"precoderGranularity: %i\n", pdcch_pdu_rel15->precoderGranularity);
    LOG_D(NR_MAC,"numDlDci: %i\n", pdcch_pdu_rel15->numDlDci);

    if(ra->msg3_dcch_dtch) {
      // If the UE used MSG3 to transfer a DCCH or DTCH message, then contention resolution is successful upon transmission of PDCCH
      LOG_I(NR_MAC, "(ue %i, rnti 0x%04x) CBRA procedure succeeded!\n", UE_id, ra->rnti);
      nr_clear_ra_proc(module_idP, CC_id, frameP, ra);
      UE_info->active[UE_id] = true;
      UE_info->Msg4_ACKed[UE_id] = true;

      remove_front_nr_list(&sched_ctrl->feedback_dl_harq);
      harq->feedback_slot = -1;
      harq->is_waiting = false;
      add_tail_nr_list(&sched_ctrl->available_dl_harq, current_harq_pid);
      harq->round = 0;
      harq->ndi ^= 1;
    } else {
      ra->state = WAIT_Msg4_ACK;
      LOG_D(NR_MAC,"[gNB %d][RAPROC] Frame %d, Subframe %d: RA state %d\n", module_idP, frameP, slotP, ra->state);
    }
  }
}

void nr_check_Msg4_Ack(module_id_t module_id, int CC_id, frame_t frame, sub_frame_t slot, NR_RA_t *ra) {

  int UE_id = find_nr_UE_id(module_id, ra->rnti);
  const int current_harq_pid = ra->harq_pid;

  NR_UE_info_t *UE_info = &RC.nrmac[module_id]->UE_info;
  NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
  NR_UE_harq_t *harq = &sched_ctrl->harq_processes[current_harq_pid];
  NR_mac_stats_t *stats = &UE_info->mac_stats[UE_id];

  LOG_D(NR_MAC, "ue %d, rnti 0x%04x, harq is waiting %d, round %d, frame %d %d, harq id %d\n", UE_id, ra->rnti, harq->is_waiting, harq->round, frame, slot, current_harq_pid);

  if (harq->is_waiting == 0) {
    if (harq->round == 0) {
      if (stats->dlsch_errors == 0) {
        LOG_I(NR_MAC, "(ue %i, rnti 0x%04x) Received Ack of RA-Msg4. CBRA procedure succeeded!\n", UE_id, ra->rnti);
        UE_info->active[UE_id] = true;
        UE_info->Msg4_ACKed[UE_id] = true;
      }
      else {
        LOG_I(NR_MAC, "(ue %i, rnti 0x%04x) RA Procedure failed at Msg4!\n", UE_id, ra->rnti);
      }

      nr_clear_ra_proc(module_id, CC_id, frame, ra);
      if(sched_ctrl->retrans_dl_harq.head >= 0) {
        remove_nr_list(&sched_ctrl->retrans_dl_harq, current_harq_pid);
      }
    }
    else {
      LOG_I(NR_MAC, "(ue %i, rnti 0x%04x) Received Nack of RA-Msg4. Preparing retransmission!\n", UE_id, ra->rnti);
      ra->Msg4_frame = (frame + 1) % 1024;
      ra->Msg4_slot = 1;
      ra->state = Msg4;
    }
  }
}

void nr_clear_ra_proc(module_id_t module_idP, int CC_id, frame_t frameP, NR_RA_t *ra){
  LOG_D(NR_MAC,"[gNB %d][RAPROC] CC_id %d Frame %d Clear Random access information rnti %x\n", module_idP, CC_id, frameP, ra->rnti);
  ra->state = RA_IDLE;
  ra->timing_offset = 0;
  ra->RRC_timer = 20;
  ra->msg3_round = 0;
  ra->msg3_dcch_dtch = false;
  ra->crnti = 0;
  if(ra->cfra == false) {
    ra->rnti = 0;
  }
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

  LOG_I(NR_MAC, "[gNB] Generate RAR MAC PDU frame %d slot %d preamble index %u TA command %d \n", ra->Msg2_frame, ra-> Msg2_slot, ra->preamble_index, ra->timing_offset);
  NR_RA_HEADER_BI *rarbi = (NR_RA_HEADER_BI *) dlsch_buffer;
  NR_RA_HEADER_RAPID *rarh = (NR_RA_HEADER_RAPID *) (dlsch_buffer + 1);
  NR_MAC_RAR *rar = (NR_MAC_RAR *) (dlsch_buffer + 2);
  unsigned char csi_req = 0, tpc_command;
  //uint8_t N_UL_Hop;
  uint8_t valid_bits;
  uint32_t ul_grant;
  uint16_t f_alloc, prb_alloc, bwp_size, truncation=0;

  tpc_command = 3; // this is 0 dB

  /// E/T/R/R/BI subheader ///
  // E = 1, MAC PDU includes another MAC sub-PDU (RAPID)
  // T = 0, Back-off indicator subheader
  // R = 2, Reserved
  // BI = 0, 5ms
  rarbi->E = 1;
  rarbi->T = 0;
  rarbi->R = 0;
  rarbi->BI = 0;

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

#ifdef DEBUG_RAR
  //LOG_I(NR_MAC, "rarbi->E = 0x%x\n", rarbi->E);
  //LOG_I(NR_MAC, "rarbi->T = 0x%x\n", rarbi->T);
  //LOG_I(NR_MAC, "rarbi->R = 0x%x\n", rarbi->R);
  //LOG_I(NR_MAC, "rarbi->BI = 0x%x\n", rarbi->BI);

  LOG_I(NR_MAC, "rarh->E = 0x%x\n", rarh->E);
  LOG_I(NR_MAC, "rarh->T = 0x%x\n", rarh->T);
  LOG_I(NR_MAC, "rarh->RAPID = 0x%x (%i)\n", rarh->RAPID, rarh->RAPID);

  LOG_I(NR_MAC, "rar->R = 0x%x\n", rar->R);
  LOG_I(NR_MAC, "rar->TA1 = 0x%x\n", rar->TA1);

  LOG_I(NR_MAC, "rar->TA2 = 0x%x\n", rar->TA2);
  LOG_I(NR_MAC, "rar->UL_GRANT_1 = 0x%x\n", rar->UL_GRANT_1);

  LOG_I(NR_MAC, "rar->UL_GRANT_2 = 0x%x\n", rar->UL_GRANT_2);
  LOG_I(NR_MAC, "rar->UL_GRANT_3 = 0x%x\n", rar->UL_GRANT_3);
  LOG_I(NR_MAC, "rar->UL_GRANT_4 = 0x%x\n", rar->UL_GRANT_4);

  LOG_I(NR_MAC, "rar->TCRNTI_1 = 0x%x\n", rar->TCRNTI_1);
  LOG_I(NR_MAC, "rar->TCRNTI_2 = 0x%x\n", rar->TCRNTI_2);
#endif

  int mcs = (unsigned char) (rar->UL_GRANT_4 >> 4);
  // time alloc
  int Msg3_t_alloc = (unsigned char) (rar->UL_GRANT_3 & 0x07);
  // frequency alloc
  int Msg3_f_alloc = (uint16_t) ((rar->UL_GRANT_3 >> 4) | (rar->UL_GRANT_2 << 4) | ((rar->UL_GRANT_1 & 0x03) << 12));
  // frequency hopping
  int freq_hopping = (unsigned char) (rar->UL_GRANT_1 >> 2);
  // TA command
  int  ta_command = rar->TA2 + (rar->TA1 << 5);
  // TC-RNTI
  int t_crnti = rar->TCRNTI_2 + (rar->TCRNTI_1 << 8);

  LOG_D(NR_MAC, "In %s: Transmitted RAR with t_alloc %d f_alloc %d ta_command %d mcs %d freq_hopping %d tpc_command %d csi_req %d t_crnti %x \n",
        __FUNCTION__,
        Msg3_t_alloc,
        Msg3_f_alloc,
        ta_command,
        mcs,
        freq_hopping,
        tpc_command,
        csi_req,
        t_crnti);
}
