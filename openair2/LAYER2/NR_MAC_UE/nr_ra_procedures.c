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

/*! \file ra_procedures.c
 * \brief Routines for UE MAC-layer Random Access procedures (TS 38.321, Release 15)
 * \author R. Knopp, Navid Nikaein, Guido Casati
 * \date 2019
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr navid.nikaein@eurecom.fr, guido.casati@iis.fraunhofer.de
 * \note
 * \warning
 */

#include "mac.h"

/*
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "PHY_INTERFACE/phy_interface_extern.h"
#include "SCHED_UE/sched_UE.h"
#include "COMMON/mac_rrc_primitives.h"
#include "RRC/LTE/rrc_extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "common/utils/LOG/log.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"
#include "PHY/LTE_ESTIMATION/lte_estimation.h"*/

/* Tools */
#include "SIMULATION/TOOLS/sim.h"	// for taus

/* RRC */
#include "NR_RACH-ConfigCommon.h"

/* PHY */
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
#include "PHY/defs_common.h"
#include "PHY/defs_nr_common.h"
#include "PHY/NR_UE_ESTIMATION/nr_estimation.h"

/* MAC */
#include "LAYER2/NR_MAC_COMMON/nr_mac_extern.h"
#include "NR_MAC_COMMON/nr_mac.h"
#include "LAYER2/NR_MAC_UE/mac_proto.h"

extern int64_t table_6_3_3_2_3_prachConfig_Index [256][9];
extern const uint8_t nr_slots_per_frame[5];

//extern uint8_t  nfapi_mode;

void nr_get_RA_window(NR_UE_MAC_INST_t *mac);

// WIP
// This routine implements Section 5.1.2 (UE Random Access Resource Selection)
// and Section 5.1.3 (Random Access Preamble Transmission) from 3GPP TS 38.321
void nr_get_prach_resources(module_id_t mod_id,
                            int CC_id,
                            uint8_t gNB_id,
                            uint8_t t_id,
                            uint8_t first_Msg3,
                            NR_PRACH_RESOURCES_t *prach_resources,
                            NR_RACH_ConfigDedicated_t * rach_ConfigDedicated){

  NR_UE_MAC_INST_t *mac = get_mac_inst(mod_id);
  NR_ServingCellConfigCommon_t *scc = mac->scc;
  NR_RACH_ConfigCommon_t *nr_rach_ConfigCommon;
  NR_RACH_ConfigGeneric_t *rach_ConfigGeneric;

  // NR_BeamFailureRecoveryConfig_t *beam_failure_recovery_config = &mac->RA_BeamFailureRecoveryConfig; // todo

  int messagePowerOffsetGroupB = 0, messageSizeGroupA, PLThreshold, sizeOfRA_PreamblesGroupA = 0, numberOfRA_Preambles, i, deltaPreamble_Msg3 = 0;
  uint8_t noGroupB = 0, s_id, f_id, ul_carrier_id, prach_ConfigIndex, SFN_nbr, Msg3_size;

  AssertFatal(scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup != NULL, "[UE %d] FATAL nr_rach_ConfigCommon is NULL !!!\n", mod_id);
  nr_rach_ConfigCommon = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup;
  rach_ConfigGeneric = &nr_rach_ConfigCommon->rach_ConfigGeneric;

  // NR_RSRP_Range_t rsrp_ThresholdSSB; // todo

  ///////////////////////////////////////////////////////////
  //////////* UE Random Access Resource Selection *//////////
  ///////////////////////////////////////////////////////////

  // todo: 
  // - switch initialisation cases
  // -- RA initiated by beam failure recovery operation (subclause 5.17 TS 38.321)
  // --- SSB selection, set prach_resources->ra_PreambleIndex
  // -- RA initiated by PDCCH: ra_preamble_index provided by PDCCH && ra_PreambleIndex != 0b000000 
  // --- set PREAMBLE_INDEX to ra_preamble_index
  // --- select the SSB signalled by PDCCH
  // -- RA initiated for SI request:
  // --- SSB selection, set prach_resources->ra_PreambleIndex

  if (rach_ConfigDedicated) {
    //////////* Contention free RA *//////////
    // - the PRACH preamble for the UE to transmit is set through RRC configuration
    // - this is the default mode in current implementation!
    // Operation for contention-free RA resources when:
    // - available SSB with SS-RSRP above rsrp-ThresholdSSB: SSB selection
    // - available CSI-RS with CSI-RSRP above rsrp-ThresholdCSI-RS: CSI-RS selection
    // - network controlled Mobility
    uint8_t cfra_ssb_resource_idx = 0;
    prach_resources->ra_PreambleIndex = rach_ConfigDedicated->cfra->resources.choice.ssb->ssb_ResourceList.list.array[cfra_ssb_resource_idx]->ra_PreambleIndex;
    LOG_D(MAC, "[RAPROC] - Selected RA preamble index %d for contention-free random access procedure... \n", prach_resources->ra_PreambleIndex);
  } else {
    //////////* Contention-based RA preamble selection *//////////
    // todo:
    // - selection of SSB with SS-RSRP above rsrp-ThresholdSSB else select any SSB
    // - todo determine next available PRACH occasion

    // rsrp_ThresholdSSB = *nr_rach_ConfigCommon->rsrp_ThresholdSSB;

    Msg3_size = mac->RA_Msg3_size;

    numberOfRA_Preambles = 64;
    if(nr_rach_ConfigCommon->totalNumberOfRA_Preambles != NULL)
      numberOfRA_Preambles = *(nr_rach_ConfigCommon->totalNumberOfRA_Preambles);

    if (!nr_rach_ConfigCommon->groupBconfigured) {
      noGroupB = 1;
    } else {
      // RA preambles group B is configured 
      // - Defining the number of RA preambles in RA Preamble Group A for each SSB */
      sizeOfRA_PreamblesGroupA = nr_rach_ConfigCommon->groupBconfigured->numberOfRA_PreamblesGroupA;
      switch (nr_rach_ConfigCommon->groupBconfigured->ra_Msg3SizeGroupA){
      /* - Threshold to determine the groups of RA preambles */
      case 0:
      messageSizeGroupA = 56;
      break;
      case 1:
      messageSizeGroupA = 144;
      break;
      case 2:
      messageSizeGroupA = 208;
      break;
      case 3:
      messageSizeGroupA = 256;
      break;
      case 4:
      messageSizeGroupA = 282;
      break;
      case 5:
      messageSizeGroupA = 480;
      break;
      case 6:
      messageSizeGroupA = 640;
      break;
      case 7:
      messageSizeGroupA = 800;
      break;
      case 8:
      messageSizeGroupA = 1000;
      break;
      case 9:
      messageSizeGroupA = 72;
      break;
      default:
      AssertFatal(1 == 0,"Unknown ra_Msg3SizeGroupA %lu\n", nr_rach_ConfigCommon->groupBconfigured->ra_Msg3SizeGroupA);
      /* todo cases 10 -15*/
      }

      /* Power offset for preamble selection in dB */
      messagePowerOffsetGroupB = -9999;
      switch (nr_rach_ConfigCommon->groupBconfigured->messagePowerOffsetGroupB){
      case 0:
      messagePowerOffsetGroupB = -9999;
      break;
      case 1:
      messagePowerOffsetGroupB = 0;
      break;
      case 2:
      messagePowerOffsetGroupB = 5;
      break;
      case 3:
      messagePowerOffsetGroupB = 8;
      break;
      case 4:
      messagePowerOffsetGroupB = 10;
      break;
      case 5:
      messagePowerOffsetGroupB = 12;
      break;
      case 6:
      messagePowerOffsetGroupB = 15;
      break;
      case 7:
      messagePowerOffsetGroupB = 18;
      break;
      default:
      AssertFatal(1 == 0,"Unknown messagePowerOffsetGroupB %lu\n", nr_rach_ConfigCommon->groupBconfigured->messagePowerOffsetGroupB);
      }

      // todo Msg3-DeltaPreamble should be provided from higher layers, otherwise is 0
      mac->deltaPreamble_Msg3 = 0;
      deltaPreamble_Msg3 = mac->deltaPreamble_Msg3;
    }

    PLThreshold = prach_resources->RA_PCMAX - rach_ConfigGeneric->preambleReceivedTargetPower - deltaPreamble_Msg3 - messagePowerOffsetGroupB;

    /* Msg3 has not been transmitted yet */
    if (first_Msg3 == 1) {
      if (noGroupB == 1) {
        // use Group A preamble
        prach_resources->ra_PreambleIndex = (taus()) % numberOfRA_Preambles;
        mac->RA_usedGroupA = 1;
      } else if ((Msg3_size < messageSizeGroupA) && (get_nr_PL(mod_id, CC_id, gNB_id) > PLThreshold)) {
        // Group B is configured and RA preamble Group A is used
        // - todo add condition on CCCH_sdu_size for initiation by CCCH
        prach_resources->ra_PreambleIndex = (taus()) % sizeOfRA_PreamblesGroupA;
        mac->RA_usedGroupA = 1;
      } else {
        // Group B preamble is configured and used
        // the first sizeOfRA_PreamblesGroupA RA preambles belong to RA Preambles Group A
        // the remaining belong to RA Preambles Group B
        prach_resources->ra_PreambleIndex = sizeOfRA_PreamblesGroupA + (taus()) % (numberOfRA_Preambles - sizeOfRA_PreamblesGroupA);
        mac->RA_usedGroupA = 0;
      }
    } else { // Msg3 is being retransmitted
      if (mac->RA_usedGroupA == 1 && noGroupB == 1) {
        prach_resources->ra_PreambleIndex = (taus()) % numberOfRA_Preambles;
      } else if (mac->RA_usedGroupA == 1 && noGroupB == 0){
        prach_resources->ra_PreambleIndex = (taus()) % sizeOfRA_PreamblesGroupA;
      } else {
        prach_resources->ra_PreambleIndex = sizeOfRA_PreamblesGroupA + (taus()) % (numberOfRA_Preambles - sizeOfRA_PreamblesGroupA);
      }
    }
    LOG_D(MAC, "[RAPROC] - Selected RA preamble index %d for contention-based random access procedure... \n", prach_resources->ra_PreambleIndex);
  }

  // todo determine next available PRACH occasion
  // - if RA initiated for SI request and ra_AssociationPeriodIndex and si-RequestPeriod are configured
  // - else if SSB is selected above
  // - else if CSI-RS is selected above

  /////////////////////////////////////////////////////////////////////////////
  //////////* Random Access Preamble Transmission (5.1.3 TS 38.321) *//////////
  /////////////////////////////////////////////////////////////////////////////
  // todo:
  // - condition on notification of suspending power ramping counter from lower layer (5.1.3 TS 38.321)
  // - check if SSB or CSI-RS have not changed since the selection in the last RA Preamble tranmission
  // - Extend RA_rnti computation (e.g. f_id selection, ul_carrier_id are hardcoded)

  if (mac->RA_PREAMBLE_TRANSMISSION_COUNTER > 1)
    mac->RA_PREAMBLE_POWER_RAMPING_COUNTER++;

  prach_resources->ra_PREAMBLE_RECEIVED_TARGET_POWER = nr_get_Po_NOMINAL_PUSCH(prach_resources, mod_id, CC_id);

   // RA-RNTI computation (associated to PRACH occasion in which the RA Preamble is transmitted)
   // 1) this does not apply to contention-free RA Preamble for beam failure recovery request
   // 2) getting star_symb, SFN_nbr from table 6.3.3.2-3 (TDD and FR1 scenario)

   prach_ConfigIndex = rach_ConfigGeneric->prach_ConfigurationIndex;

   // ra_RNTI computation
   // - todo: this is for TDD FR1 only
   // - ul_carrier_id: UL carrier used for RA preamble transmission, hardcoded for NUL carrier
   // - f_id: index of the PRACH occasion in the frequency domain
   // - s_id is starting symbol of the PRACH occasion [0...14]
   // - t_id is the first slot of the PRACH occasion in a system frame [0...80]

   ul_carrier_id = 0; // NUL
   f_id = rach_ConfigGeneric->msg1_FrequencyStart;
   SFN_nbr = table_6_3_3_2_3_prachConfig_Index[prach_ConfigIndex][4]; 
   s_id = table_6_3_3_2_3_prachConfig_Index[prach_ConfigIndex][5];

   // Pick the first slot of the PRACH occasion in a system frame
   for (i = 0; i < 10; i++){
    if (((SFN_nbr & (1 << i)) >> i) == 1){
      t_id = 2*i;
      break;
    }
   }
   prach_resources->ra_RNTI = 1 + s_id + 14 * t_id + 1120 * f_id + 8960 * ul_carrier_id;
   mac->ra_rnti = prach_resources->ra_RNTI;

   LOG_D(MAC, "Computed ra_RNTI is %x \n", prach_resources->ra_RNTI);
}

// TbD: RA_attempt_number not used
void nr_Msg1_transmitted(module_id_t mod_id, uint8_t CC_id, frame_t frameP, uint8_t gNB_id){
  AssertFatal(CC_id == 0, "Transmission on secondary CCs is not supported yet\n");
  NR_UE_MAC_INST_t *mac = get_mac_inst(mod_id);
  mac->ra_state = WAIT_RAR;
  // Start contention resolution timer
  mac->RA_attempt_number++;
}

void nr_Msg3_transmitted(module_id_t mod_id, uint8_t CC_id, frame_t frameP, uint8_t gNB_id){
  AssertFatal(CC_id == 0, "Transmission on secondary CCs is not supported yet\n");
  LOG_D(MAC,"[UE %d][RAPROC] Frame %d : Msg3_tx: Starting contention resolution timer\n", mod_id, frameP);
  NR_UE_MAC_INST_t *mac = get_mac_inst(mod_id);
  // start contention resolution timer
  mac->RA_contention_resolution_cnt = 0;
  mac->RA_contention_resolution_timer_active = 1;
}

/////////////////////////////////////////////////////////////////////////
///////* Random Access Preamble Initialization (5.1.1 TS 38.321) *///////
/////////////////////////////////////////////////////////////////////////
/// Handling inizialization by PDCCH order, MAC entity or RRC (TS 38.300)
/// Only one RA procedure is ongoing at any point in time in a MAC entity
/// the RA procedure on a SCell shall only be initiated by PDCCH order
/// in the current implementation, RA is contention free only

// WIP
// todo TS 38.321:
// - check if carrier to use is explicitly signalled then do (1) RA CARRIER SELECTION (SUL, NUL) (2) set PCMAX
// - BWP operation (subclause 5.15 TS 38.321)
// - handle initialization by beam failure recovery
// - handle initialization by handover
// - handle transmission on DCCH using PRACH (during handover, or sending SR for example)
// - take into account MAC CEs in size_sdu (currently hardcoded size to 1 MAC subPDU and 1 padding subheader)
// - fix rrc data req logic
// - retrieve TBS
// - add mac_rrc_nr_data_req_ue, etc ...
// - add the backoff condition here if we have it from a previous RA reponse which failed (i.e. backoff indicator)

uint8_t nr_ue_get_rach(NR_PRACH_RESOURCES_t *prach_resources,
                       module_id_t mod_id,
                       int CC_id,
                       UE_MODE_t UE_mode,
                       frame_t frame,
                       uint8_t gNB_id,
                       int nr_tti_tx){

  NR_UE_MAC_INST_t *mac = get_mac_inst(mod_id);
  uint8_t mac_sdus[MAX_NR_ULSCH_PAYLOAD_BYTES];
  uint8_t lcid = UL_SCH_LCID_CCCH_MSG3, *payload;
  //uint8_t ra_ResponseWindow;
  uint16_t size_sdu = 0;
  unsigned short post_padding;
  //fapi_nr_config_request_t *cfg = &mac->phy_config.config_req;
  NR_ServingCellConfigCommon_t *scc = mac->scc;
  NR_RACH_ConfigCommon_t *setup = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup;
  NR_RACH_ConfigGeneric_t *rach_ConfigGeneric = &setup->rach_ConfigGeneric;
  //NR_FrequencyInfoDL_t *frequencyInfoDL = scc->downlinkConfigCommon->frequencyInfoDL;
  NR_RACH_ConfigDedicated_t *rach_ConfigDedicated = mac->rach_ConfigDedicated;

  // int32_t frame_diff = 0;

  uint8_t sdu_lcids[NB_RB_MAX] = {0};
  uint16_t sdu_lengths[NB_RB_MAX] = {0};
  int TBS_bytes = 848, header_length_total=0, num_sdus, offset, preambleTransMax, mac_ce_len;

  AssertFatal(CC_id == 0,"Transmission on secondary CCs is not supported yet\n");

  if (UE_mode < PUSCH && prach_resources->init_msg1) {

    LOG_D(MAC, "nr_ue_get_rach, RA_active value: %d", mac->RA_active);

    AssertFatal(setup != NULL, "[UE %d] FATAL nr_rach_ConfigCommon is NULL !!!\n", mod_id);

    if (mac->RA_active == 0) {
      /* RA not active - checking if RRC is ready to initiate the RA procedure */

      LOG_I(MAC, "RA not active. Starting RA preamble initialization.\n");

      mac->RA_RAPID_found = 0;

      /* Set RA_PREAMBLE_POWER_RAMPING_STEP */
      switch (rach_ConfigGeneric->powerRampingStep){ // in dB
       case 0:
       prach_resources->RA_PREAMBLE_POWER_RAMPING_STEP = 0;
       break;
       case 1:
       prach_resources->RA_PREAMBLE_POWER_RAMPING_STEP = 2;
       break;
       case 2:
       prach_resources->RA_PREAMBLE_POWER_RAMPING_STEP = 4;
       break;
       case 3:
       prach_resources->RA_PREAMBLE_POWER_RAMPING_STEP = 6;
       break;
      }

      prach_resources->RA_PREAMBLE_BACKOFF = 0;
      prach_resources->RA_SCALING_FACTOR_BI = 1;
      prach_resources->RA_PCMAX = 0; // currently hardcoded to 0

      payload = (uint8_t*) &mac->CCCH_pdu.payload;

      mac_ce_len = 0;
      num_sdus = 1;
      post_padding = 1;

      if (0){
        // initialisation by RRC
        // CCCH PDU
        // size_sdu = (uint16_t) mac_rrc_data_req_ue(mod_id,
        //                                           CC_id,
        //                                           frame,
        //                                           CCCH,
        //                                           1,
        //                                           mac_sdus,
        //                                           gNB_id,
        //                                           0);
        LOG_D(MAC,"[UE %d] Frame %d: Requested RRCConnectionRequest, got %d bytes\n", mod_id, frame, size_sdu);
      } else {
        // fill ulsch_buffer with random data
        for (int i = 0; i < TBS_bytes; i++){
          mac_sdus[i] = (unsigned char) (lrand48()&0xff);
        }
        //Sending SDUs with size 1
        //Initialize elements of sdu_lcids and sdu_lengths
        sdu_lcids[0] = lcid;
        sdu_lengths[0] = TBS_bytes - 3 - post_padding - mac_ce_len;
        header_length_total += 2 + (sdu_lengths[0] >= 128);
        size_sdu += sdu_lengths[0];
      }

      //mac->RA_tx_frame = frame;
      //mac->RA_tx_subframe = nr_tti_tx;
      //mac->RA_backoff_frame = frame;
      //mac->RA_backoff_subframe = nr_tti_tx;

      if (size_sdu > 0) {

        LOG_I(MAC, "[UE %d] Frame %d: Initialisation Random Access Procedure\n", mod_id, frame);

        mac->RA_PREAMBLE_TRANSMISSION_COUNTER = 1;
        mac->RA_PREAMBLE_POWER_RAMPING_COUNTER = 1;
        mac->RA_Msg3_size = size_sdu + sizeof(NR_MAC_SUBHEADER_SHORT) + sizeof(NR_MAC_SUBHEADER_SHORT);
        mac->RA_prachMaskIndex = 0;
        // todo: add the backoff condition here
        mac->RA_backoff_cnt = 0;
        mac->RA_active = 1;
        prach_resources->Msg3 = payload;

        nr_get_RA_window(mac);

        // Fill in preamble and PRACH resources
        if (mac->generate_nr_prach == 1)
          nr_get_prach_resources(mod_id, CC_id, gNB_id, nr_tti_tx, 1, prach_resources, rach_ConfigDedicated);

        offset = nr_generate_ulsch_pdu((uint8_t *) mac_sdus,              // sdus buffer
                                       (uint8_t *) payload,               // UL MAC pdu pointer
                                       num_sdus,                          // num sdus
                                       sdu_lengths,                       // sdu length
                                       sdu_lcids,                         // sdu lcid
                                       0,                                 // power headroom
                                       0,                                 // crnti
                                       0,                                 // truncated bsr
                                       0,                                 // short bsr
                                       0,                                 // long_bsr
                                       post_padding,
                                       0);

        // Padding: fill remainder with 0
        if (post_padding > 0){
          for (int j = 0; j < (TBS_bytes - offset); j++)
            payload[offset + j] = 0; // mac_pdu[offset + j] = 0;
        }
      } 
    } else if (mac->RA_window_cnt != -1) { // RACH is active

      ////////////////////////////////////////////////////////////////
      /////* Random Access Response reception (5.1.4 TS 38.321) */////
      ////////////////////////////////////////////////////////////////
      // Handling ra_responseWindow, RA_PREAMBLE_TRANSMISSION_COUNTER
      // and RA_backoff_cnt
      // todo:
      // - handle beam failure recovery request
      // - handle DL assignment on PDCCH for RA-RNTI
      // - handle backoff and raResponseWindow params

      // LOG_D(MAC, "[MAC][UE %d][RAPROC] frame %d, subframe %d: RA Active, window cnt %d (RA_tx_frame %d, RA_tx_subframe %d)\n",
      //   mod_id, frame, nr_tti_tx, mac->RA_window_cnt, mac->RA_tx_frame, mac->RA_tx_subframe);

      if (mac->RA_BI_found){
        prach_resources->RA_PREAMBLE_BACKOFF = prach_resources->RA_SCALING_FACTOR_BI * mac->RA_backoff_indicator;
      } else {
        prach_resources->RA_PREAMBLE_BACKOFF = 0;
      }

      if (mac->RA_window_cnt >= 0 && mac->RA_RAPID_found == 1) {

        mac->RA_window_cnt = -1;
        mac->ra_state = RA_SUCCEEDED;
        LOG_I(MAC, "[MAC][UE %d][RAPROC] Frame %d: nr_tti_tx %d: RAR successfully received \n", mod_id, frame, nr_tti_tx);

      } else if (mac->RA_window_cnt == 0 && !mac->RA_RAPID_found) {

        LOG_I(MAC, "[MAC][UE %d][RAPROC] Frame %d: nr_tti_tx %d: RAR reception failed \n", mod_id, frame, nr_tti_tx);

        mac->ra_state = RA_UE_IDLE;
        mac->RA_PREAMBLE_TRANSMISSION_COUNTER++;

        preambleTransMax = -1;
        switch (rach_ConfigGeneric->preambleTransMax) {
        case 0:
          preambleTransMax = 3;
          break;
        case 1:
          preambleTransMax = 4;
          break;
        case 2:
          preambleTransMax = 5;
          break;
        case 3:
          preambleTransMax = 6;
          break;
        case 4:
          preambleTransMax = 7;
          break;
        case 5:
          preambleTransMax = 8;
          break;
        case 6:
          preambleTransMax = 10;
          break;
        case 7:
          preambleTransMax = 20;
          break;
        case 8:
          preambleTransMax = 50;
          break;
        case 9:
          preambleTransMax = 100;
          break;
        case 10:
          preambleTransMax = 200;
          break;
        }

        // Resetting RA window
        nr_get_RA_window(mac);

        if (mac->RA_PREAMBLE_TRANSMISSION_COUNTER == preambleTransMax + 1){
          LOG_D(MAC, "[UE %d] Frame %d: Maximum number of RACH attempts (%d)\n", mod_id, frame, preambleTransMax);
          mac->RA_backoff_cnt = rand() % (prach_resources->RA_PREAMBLE_BACKOFF + 1);
          mac->RA_PREAMBLE_TRANSMISSION_COUNTER = 1;
          prach_resources->RA_PREAMBLE_POWER_RAMPING_STEP += prach_resources->RA_PREAMBLE_POWER_RAMPING_STEP << 1; // 2 dB increment
          prach_resources->ra_PREAMBLE_RECEIVED_TARGET_POWER = nr_get_Po_NOMINAL_PUSCH(prach_resources, mod_id, CC_id);
        }

        // compute backoff parameters
        // if (mac->RA_backoff_cnt > 0){
        //   frame_diff = (sframe_t) frame - mac->RA_backoff_frame;
        //   if (frame_diff < 0) frame_diff = -frame_diff;
        //   mac->RA_backoff_frame = frame;
        //   mac->RA_backoff_subframe = nr_tti_tx;
        // }
        // compute RA window parameters
        // if (mac->RA_window_cnt > 0){
        //   frame_diff = (frame_t) frame - mac->RA_tx_frame;
        //   if (frame_diff < 0) frame_diff = -frame_diff;
        //   mac->RA_window_cnt -= ((10 * frame_diff) + (nr_tti_tx - mac->RA_tx_subframe));
        //   LOG_D(MAC, "[MAC][UE %d][RAPROC] Frame %d, subframe %d: RA Active, adjusted window cnt %d\n", mod_id, frame, nr_tti_tx, mac->RA_window_cnt);
        // }

        // mac->RA_tx_frame = frame;
        // mac->RA_tx_subframe = nr_tti_tx;

        // Fill in preamble and PRACH resources
        if (mac->generate_nr_prach == 1)
          nr_get_prach_resources(mod_id, CC_id, gNB_id, nr_tti_tx, 0, prach_resources, rach_ConfigDedicated);

      } else {

        mac->RA_window_cnt--;

        LOG_I(MAC, "[MAC][UE %d][RAPROC] Frame %d: nr_tti_tx %d: RAR reception not successful, (RA window count %d) \n",
          mod_id,
          frame,
          nr_tti_tx,
          mac->RA_window_cnt);

        // Fill in preamble and PRACH resources
        if (mac->generate_nr_prach == 1)
          nr_get_prach_resources(mod_id, CC_id, gNB_id, nr_tti_tx, 0, prach_resources, rach_ConfigDedicated);

      }
    }
  } else if (UE_mode == PUSCH) {
    LOG_D(MAC, "[UE %d] FATAL: Should not have checked for RACH in PUSCH yet ...", mod_id);
    AssertFatal(1 == 0, "");
  }
 return mac->generate_nr_prach;
}

void nr_get_RA_window(NR_UE_MAC_INST_t *mac){

  uint8_t mu, ra_ResponseWindow;
  NR_ServingCellConfigCommon_t *scc = mac->scc;
  NR_RACH_ConfigCommon_t *setup = scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup;
  NR_RACH_ConfigGeneric_t *rach_ConfigGeneric = &setup->rach_ConfigGeneric;
  NR_FrequencyInfoDL_t *frequencyInfoDL = scc->downlinkConfigCommon->frequencyInfoDL;

  ra_ResponseWindow = rach_ConfigGeneric->ra_ResponseWindow;

  if (setup->msg1_SubcarrierSpacing)
    mu = *setup->msg1_SubcarrierSpacing;
  else
    mu = frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;

  mac->RA_window_cnt = mac->RA_offset*nr_slots_per_frame[mu]; // taking into account the 2 frames gap introduced by OAI gNB

  switch (ra_ResponseWindow) {
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl1:
      mac->RA_window_cnt += 1;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl2:
      mac->RA_window_cnt += 2;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl4:
      mac->RA_window_cnt += 4;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl8:
      mac->RA_window_cnt += 8;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl10:
      mac->RA_window_cnt += 10;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl20:
      mac->RA_window_cnt += 20;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl40:
      mac->RA_window_cnt += 40;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl80:
      mac->RA_window_cnt += 80;
      break;
  }
}
