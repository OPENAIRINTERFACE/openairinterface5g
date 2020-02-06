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
// TBR missing NR_nr_UE_mac_inst in mac.h

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

/* MAC */
#include "LAYER2/NR_MAC_COMMON/nr_mac_extern.h"
#include "NR_MAC_COMMON/nr_mac.h"
#include "LAYER2/NR_MAC_UE/mac_proto.h"

extern UE_MODE_t get_nrUE_mode(uint8_t Mod_id, uint8_t CC_id, uint8_t gNB_id);

extern int64_t table_6_3_3_2_2_prachConfig_Index [256][9];
extern int64_t table_6_3_3_2_3_prachConfig_Index [256][9];

//extern uint8_t  nfapi_mode;

// This routine implements Section 5.1.2 (UE Random Access Resource Selection)
// and Section 5.1.3 (Random Access Preamble Transmission) from 3GPP TS 38.321
void nr_get_prach_resources(module_id_t mod_id,
                            int CC_id,
                            uint8_t gNB_id,
                            uint8_t t_id,
                            uint8_t first_Msg3,
                            NR_RACH_ConfigDedicated_t * rach_ConfigDedicated){

  NR_UE_MAC_INST_t *nr_UE_mac_inst = get_mac_inst(mod_id);
  NR_PRACH_RESOURCES_t *prach_resources = &nr_UE_mac_inst->RA_prach_resources;
  NR_RACH_ConfigCommon_t *nr_rach_ConfigCommon = nr_UE_mac_inst->nr_rach_ConfigCommon;
  // NR_BeamFailureRecoveryConfig_t *beam_failure_recovery_config = &nr_UE_mac_inst->RA_BeamFailureRecoveryConfig; // todo

  int messagePowerOffsetGroupB, messageSizeGroupA, PLThreshold, sizeOfRA_PreamblesGroupA, numberOfRA_Preambles, frequencyStart, i, deltaPreamble_Msg3;
  uint8_t noGroupB = 0, s_id, f_id, ul_carrier_id, msg1_FDM, prach_ConfigIndex, SFN_nbr, Msg3_size;

  // NR_RSRP_Range_t rsrp_ThresholdSSB; // TBR todo

  ///////////////////////////////////////////////////////////
  //////////* UE Random Access Resource Selection *//////////
  ///////////////////////////////////////////////////////////

  // todo: switch initialisation cases
  // - RA initiated by beam failure recovery operation (subclause 5.17 TS 38.321)
  // -- SSB selection, set prach_resources->ra_PreambleIndex
  // - RA initiated by PDCCH: ra_preamble_index provided by PDCCH && ra_PreambleIndex != 0b000000 
  // -- TBR coming from dci_pdu_rel15[0].ra_preamble_index
  // -- set PREAMBLE_INDEX to ra_preamble_index
  // -- select the SSB signalled by PDCCH
  // - RA initiated for SI request:
  // -- SSB selection, set prach_resources->ra_PreambleIndex

  // if (rach_ConfigDedicated) {  // This is for network controlled Mobility
  //   // TBR operation for contention-free RA resources when:
  //   // - available SSB with SS-RSRP above rsrp-ThresholdSSB: SSB selection
  //   // - availalbe CSI-RS with CSI-RSRP above rsrp-ThresholdCSI-RS: CSI-RS selection
  //   prach_resources->ra_PreambleIndex = rach_ConfigDedicated->ra_PreambleIndex;
  //   return;
  // }

  //////////* Contention-based RA preamble selection *//////////

  // todo: selection of SSB with SS-RSRP above rsrp-ThresholdSSB else select any SSB
  // rsrp_ThresholdSSB = *nr_rach_ConfigCommon->rsrp_ThresholdSSB; // TBR

  Msg3_size = nr_UE_mac_inst->RA_Msg3_size;
  numberOfRA_Preambles = nr_rach_ConfigCommon->totalNumberOfRA_Preambles;

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
    AssertFatal(1 == 0,"Unknown ra_Msg3SizeGroupA %d\n", nr_rach_ConfigCommon->groupBconfigured->ra_Msg3SizeGroupA);
    /* todo cases 10 -15*/
    }

    /* Power offset for preamble selection in dB */
    /* TBR: what value to use as default? Shall it be converted ? */
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
    AssertFatal(1 == 0,"Unknown messagePowerOffsetGroupB %d\n", nr_rach_ConfigCommon->groupBconfigured->messagePowerOffsetGroupB);
    }

    // todo Msg3-DeltaPreamble should be provided from higher layers, otherwise is 0
    nr_UE_mac_inst->deltaPreamble_Msg3 = 0;
    deltaPreamble_Msg3 = nr_UE_mac_inst->deltaPreamble_Msg3;
  }

  PLThreshold = prach_resources->RA_PCMAX - nr_rach_ConfigCommon->rach_ConfigGeneric.preambleReceivedTargetPower - deltaPreamble_Msg3 - messagePowerOffsetGroupB;

  /* Msg3 has not been transmitted yet */
  if (first_Msg3 == 1) {
    if (noGroupB == 1) {
      // use Group A preamble
      prach_resources->ra_PreambleIndex = (taus()) % numberOfRA_Preambles;
      nr_UE_mac_inst->RA_usedGroupA = 1;
    } else if ((Msg3_size < messageSizeGroupA) && (get_PL(mod_id, 0, gNB_id) > PLThreshold)) {
      // Group B is configured and RA preamble Group A is used
      // - todo TBR update get_PL to NR get_nr_PL (needs PHY_VARS_NR_UE)
      // - todo add condition on CCCH_sdu_size for initiation by CCCH
      prach_resources->ra_PreambleIndex = (taus()) % sizeOfRA_PreamblesGroupA;
      nr_UE_mac_inst->RA_usedGroupA = 1;
    } else {
      // Group B preamble is configured and used
      // the first sizeOfRA_PreamblesGroupA RA preambles belong to RA Preambles Group A
      // the remaining belong to RA Preambles Group B
      prach_resources->ra_PreambleIndex = sizeOfRA_PreamblesGroupA + (taus()) % (numberOfRA_Preambles - sizeOfRA_PreamblesGroupA);
      nr_UE_mac_inst->RA_usedGroupA = 0;
    }
  } else { // Msg3 is being retransmitted
    if (nr_UE_mac_inst->RA_usedGroupA == 1) {
     if (nr_rach_ConfigCommon->groupBconfigured){
     	prach_resources->ra_PreambleIndex = (taus()) % sizeOfRA_PreamblesGroupA;
       } else {
         prach_resources->ra_PreambleIndex = (taus()) & 0x3f; // TBR check this
       } 
     } else {
       prach_resources->ra_PreambleIndex = sizeOfRA_PreamblesGroupA + (taus()) % (numberOfRA_Preambles - sizeOfRA_PreamblesGroupA);
     }
    }

  // todo determine next available PRACH occasion
  // - if RA initiated for SI request and ra_AssociationPeriodIndex and si-RequestPeriod are configured
  // - else if SSB is selected above
  // - else if CSI-RS is selected above

  /////////////////////////////////////////////////////////////////////////////
  //////////* Random Access Preamble Transmission (5.1.3 TS 38.321) *//////////
  /////////////////////////////////////////////////////////////////////////////

  // todo condition on notification of suspending power ramping counter from lower layer (5.1.3 TS 38.321) // TBR
  // todo check if SSB or CSI-RS have not changed since the selection in the last RA Preamble tranmission
  if (nr_UE_mac_inst->RA_PREAMBLE_TRANSMISSION_COUNTER > 1)
    nr_UE_mac_inst->RA_PREAMBLE_TRANSMISSION_COUNTER++;

  prach_resources->ra_PREAMBLE_RECEIVED_TARGET_POWER = nr_get_Po_NOMINAL_PUSCH(mod_id, CC_id);

   /* RA-RNTI computation (associated to PRACH occasion in which the RA Preamble is transmitted)
   // 1) this does not apply to contention-free RA Preamble for beam failure recovery request
   // 2) getting star_symb, SFN_nbr from table 6.3.3.2-3 (TDD and FR1 scenario)
   // 3) TBR extend this (e.g. f_id selection, ul_carrier_id are hardcoded) */

   switch (nr_rach_ConfigCommon->rach_ConfigGeneric.msg1_FDM){ // todo this is not used
    case 0:
    msg1_FDM = 1;
    break;
    case 1:
    msg1_FDM = 2;
    break;
    case 2:
    msg1_FDM = 4;
    break;
    case 3:
    msg1_FDM = 8;
    break;
    default:
    AssertFatal(1 == 0,"Unknown msg1_FDM %d\n", nr_rach_ConfigCommon->rach_ConfigGeneric.msg1_FDM);
   }

   prach_ConfigIndex = nr_rach_ConfigCommon->rach_ConfigGeneric.prach_ConfigurationIndex;

   // ra_RNTI computation
   // - todo: this is for TDD FR1 only
   // - ul_carrier_id: UL carrier used for RA preamble transmission, hardcoded for NUL carrier
   // - f_id: index of the PRACH occasion in the frequency domain
   // - s_id is starting symbol of the PRACH occasion [0...14]
   // - t_id is the first slot of the PRACH occasion in a system frame [0...80]

   ul_carrier_id = 0; // todo SUL
   f_id = nr_rach_ConfigCommon->rach_ConfigGeneric.msg1_FrequencyStart;
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

   LOG_D(MAC, "Computed ra_RNTI is %d", prach_resources->ra_RNTI);
}

void nr_Msg1_transmitted(module_id_t mod_id, uint8_t CC_id, frame_t frameP, uint8_t gNB_id){

  NR_UE_MAC_INST_t *nr_UE_mac_inst = get_mac_inst(mod_id);

  AssertFatal(CC_id == 0, "Transmission on secondary CCs is not supported yet\n");

  // Start contention resolution timer
  nr_UE_mac_inst->RA_attempt_number++; // todo this is not used

}

void nr_Msg3_transmitted(module_id_t mod_id, uint8_t CC_id, frame_t frameP, uint8_t gNB_id){
  #if 0 // TBR
  AssertFatal(CC_id == 0, "Transmission on secondary CCs is not supported yet\n");

  LOG_D(MAC,"[UE %d][RAPROC] Frame %d : Msg3_tx: Setting contention resolution timer\n", mod_id, frameP);

  // start contention resolution timer  
  nr_UE_mac_inst->RA_contention_resolution_cnt = 0;
  nr_UE_mac_inst->RA_contention_resolution_timer_active = 1;

  #endif
}

/////////////////////////////////////////////////////////////////////////
///////* Random Access Preamble Initialization (5.1.1 TS 38.321) *///////
/////////////////////////////////////////////////////////////////////////
/// Handling inizialization by PDCCH order, MAC entity or RRC (TS 38.300)
/// Only one RA procedure is ongoing at any point in time in a MAC entity
/// the RA procedure on a SCell shall only be initiated by PDCCH order

NR_PRACH_RESOURCES_t *nr_ue_get_rach(module_id_t mod_id,
                                     int CC_id,
                                     UE_MODE_t UE_mode,
                                     frame_t frame,
                                     uint8_t gNB_id,
                                     int nr_tti_tx){

  NR_UE_MAC_INST_t *nr_UE_mac_inst = get_mac_inst(mod_id);
  uint8_t lcid = CCCH, dcch_header_len = 0, mac_sdus[MAX_NR_ULSCH_PAYLOAD_BYTES], * payload;
  uint16_t size_sdu = 0;
  unsigned short post_padding;
  NR_RACH_ConfigCommon_t *nr_rach_ConfigCommon = (struct NR_RACH_ConfigCommon_t *) NULL;
  NR_PRACH_RESOURCES_t *prach_resources = &nr_UE_mac_inst->RA_prach_resources;
  int32_t frame_diff = 0;

  uint8_t sdu_lcids[NB_RB_MAX] = {0}; // TBR
  uint16_t sdu_lengths[NB_RB_MAX] = {0};  // TBR
  int TBS_bytes = 848, header_length_total, num_sdus, offset; // TBR

  AssertFatal(CC_id == 0,"Transmission on secondary CCs is not supported yet\n");

  if (UE_mode == PRACH) {

    LOG_D(MAC, "nr_ue_get_rach, RA_active value: %d", nr_UE_mac_inst->RA_active);

    AssertFatal(nr_UE_mac_inst->nr_rach_ConfigCommon != NULL, "[UE %d] FATAL  nr_rach_ConfigCommon is NULL !!!\n", mod_id);

    if (nr_UE_mac_inst->nr_rach_ConfigCommon) { // TBR check the condition
      nr_rach_ConfigCommon =	nr_UE_mac_inst->nr_rach_ConfigCommon;
    } else return NULL;

    if (nr_UE_mac_inst->RA_active == 0) {
      /* RA not active - checking if RRC is ready to initiate the RA procedure */

      LOG_I(MAC, "RA not active. Starting RA preamble initialization.\n");

      /* TBR flush Msg3 Buffer
      this was done like this but at PHY level
      for(i=0; i<NUMBER_OF_CONNECTED_eNB_MAX; i++) {
        // flush Msg3 buffer
        PHY_VARS_NR_UE *ue = PHY_vars_UE_g[Mod_id][CC_id];
        ue->ulsch_Msg3_active[i] = 0;
      }
      */

      nr_UE_mac_inst->RA_RAPID_found = 0;

      /* Set RA_PREAMBLE_POWER_RAMPING_STEP */
      switch (nr_rach_ConfigCommon->rach_ConfigGeneric.powerRampingStep){ // in dB
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

      // todo:
      // - check if carrier to use is explicitly signalled then do (1) RA CARRIER SELECTION (SUL, NUL) (2) set PCMAX
      // - BWP operation (subclause 5.15 TS 38.321)
      // - handle initialization by beam failure recovery
      // - handle initialization by handover

      if (!IS_SOFTMODEM_NOS1){
        // todo mac_rrc_nr_data_req_ue
        payload = &nr_UE_mac_inst->CCCH_pdu.payload;
        size_sdu = (uint16_t) mac_rrc_nr_data_req_ue(mod_id,
                                                     CC_id,
                                                     frame,
                                                     CCCH,
                                                     payload[sizeof(NR_MAC_SUBHEADER_SHORT) + 1]);

        LOG_D(MAC,"[UE %d] Frame %d: Requested RRCConnectionRequest, got %d bytes\n", mod_id,frame, size_sdu);

        // todo: else triggers a transmission on DCCH using PRACH (during handover, or sending SR for example)

      } else {
        // fill ulsch_buffer with random data
        payload = (uint8_t *)nr_UE_mac_inst->ulsch_pdu.payload[0];
        for (int i = 0; i < TBS_bytes; i++){
          mac_sdus[i] = (unsigned char) (lrand48()&0xff);
        }
        //Sending SDUs with size 1
        //Initialize elements of sdu_lcids and sdu_lengths
        sdu_lcids[0] = UL_SCH_LCID_DTCH;
        sdu_lengths[0] = TBS_bytes - 3;
        header_length_total += 2 + (sdu_lengths[0] >= 128);
        size_sdu += sdu_lengths[0];
        num_sdus +=1;
      }

      // TBR which frame and slot ?
      nr_UE_mac_inst->RA_tx_frame = frame;
      nr_UE_mac_inst->RA_tx_subframe = nr_tti_tx;
      nr_UE_mac_inst->RA_backoff_frame = frame;
      nr_UE_mac_inst->RA_backoff_subframe = nr_tti_tx;

      if (size_sdu > 0) { // TBR size_sdu + MAC_Header_len + mac_ce_len
        /* TBR initialisation by RRC
        // PDU from CCCH */

        LOG_I(MAC, "[UE %d] Frame %d: Initialisation Random Access Procedure\n", mod_id, frame);

        nr_UE_mac_inst->RA_PREAMBLE_TRANSMISSION_COUNTER = 1;
        nr_UE_mac_inst->RA_PREAMBLE_POWER_RAMPING_COUNTER = 1;
        nr_UE_mac_inst->RA_Msg3_size = size_sdu + sizeof(NR_MAC_SUBHEADER_SHORT) + sizeof(NR_MAC_SUBHEADER_SHORT); // TBR size_sdu + MAC_Header_len + mac_ce_len
        nr_UE_mac_inst->RA_prachMaskIndex = 0;
        // TBR todo: add the backoff condition here if we have it from a previous RA reponse which failed (i.e. backoff indicator)
        nr_UE_mac_inst->RA_backoff_cnt = 0;
        nr_UE_mac_inst->RA_active = 1;
        prach_resources->Msg3 = payload;

        // TBR todo double check window count
        // nr_UE_mac_inst->RA_window_cnt = 2 + nr_rach_ConfigCommon->ra_SupervisionInfo.ra_ResponseWindowSize;
        // if (nr_UE_mac_inst->RA_window_cnt == 9) {
        //     nr_UE_mac_inst->RA_window_cnt = 10;	// Note: 9 subframe window doesn't exist, after 8 is 10!
        // }

        // Fill in preamble and PRACH resources
        nr_get_prach_resources(mod_id, CC_id, gNB_id, nr_tti_tx, 1, NULL);

        offset = nr_generate_ulsch_pdu((uint8_t *) mac_sdus,              // sdus buffer
                                       (uint8_t *) payload,               // logical channel payload
                                       num_sdus,                          // num sdus
                                       sdu_lengths,                       // sdu length
                                       sdu_lcids,                         // sdu lcid
                                       0,                                 // power headroom
                                       0,                                 // crnti
                                       0,                                 // truncated bsr
                                       0,                                 // short bsr
                                       0,                                 // long_bsr
                                       1);                                // post_padding

        // Padding: fill remainder of DLSCH with 0
        if (post_padding > 0){
          for (int j = 0; j < (TBS_bytes - offset); j++)
            payload[offset + j] = 0; // mac_pdu[offset + j] = 0;
        }

        return (prach_resources);
      } 
    } else { // RACH is active

      ////////////////////////////////////////////////////////////////
      /////* Random Access Response reception (5.1.4 TS 38.321) */////
      ////////////////////////////////////////////////////////////////
      // Handling ra_responseWindow, RA_PREAMBLE_TRANSMISSION_COUNTER
      // and RA_backoff_cnt
      // todo:
      // - handle beam failure recovery request
      // - handle DL assignment on PDCCH for RA-RNTI

      LOG_D(MAC, "[MAC][UE %d][RAPROC] frame %d, subframe %d: RA Active, window cnt %d (RA_tx_frame %d, RA_tx_subframe %d)\n",
        mod_id, frame, nr_tti_tx, nr_UE_mac_inst->RA_window_cnt, nr_UE_mac_inst->RA_tx_frame, nr_UE_mac_inst->RA_tx_subframe);

      if (nr_UE_mac_inst->rnti_type = NR_RNTI_RA){ // add condition on BI
        // TBR todo
      } else {
        prach_resources->RA_PREAMBLE_BACKOFF = 0;
      }
      // compute backoff parameters
      if (nr_UE_mac_inst->RA_backoff_cnt > 0){
        frame_diff = (sframe_t) frame - nr_UE_mac_inst->RA_backoff_frame;
        if (frame_diff < 0) frame_diff = -frame_diff;
        nr_UE_mac_inst->RA_backoff_cnt -= ((10 * frame_diff) + (nr_tti_tx - nr_UE_mac_inst->RA_backoff_subframe));
        nr_UE_mac_inst->RA_backoff_frame = frame;
        nr_UE_mac_inst->RA_backoff_subframe = nr_tti_tx;
      }
      // - TBR check
      
      // - TBR check
      // compute RA window parameters
      if (nr_UE_mac_inst->RA_window_cnt > 0){
        frame_diff = (frame_t) frame - nr_UE_mac_inst->RA_tx_frame;
        if (frame_diff < 0) frame_diff = -frame_diff;
        nr_UE_mac_inst->RA_window_cnt -= ((10 * frame_diff) + (nr_tti_tx - nr_UE_mac_inst->RA_tx_subframe));
        LOG_D(MAC, "[MAC][UE %d][RAPROC] Frame %d, subframe %d: RA Active, adjusted window cnt %d\n", mod_id, frame, nr_tti_tx, nr_UE_mac_inst->RA_window_cnt);
      }
      // - TBR check

      //if ((nr_UE_mac_inst->RA_window_cnt <= 0) && (nr_UE_mac_inst->RA_backoff_cnt <= 0)){
      if ((nr_UE_mac_inst->RA_window_cnt <= 0) && (nr_UE_mac_inst->RA_RAPID_found == 0)){ // TODO TBR double check the condition
        
        LOG_I(MAC, "[MAC][UE %d][RAPROC] Frame %d: subframe %d: RAR reception not successful, (RA window count %d) \n", mod_id, frame, nr_tti_tx, nr_UE_mac_inst->RA_window_cnt);
        
        nr_UE_mac_inst->RA_PREAMBLE_TRANSMISSION_COUNTER++;

        // - TBR check RA_tx_frame RA_tx_subframe
        nr_UE_mac_inst->RA_tx_frame = frame;
        nr_UE_mac_inst->RA_tx_subframe = nr_tti_tx;
        prach_resources->ra_PREAMBLE_RECEIVED_TARGET_POWER += (prach_resources->RA_PREAMBLE_POWER_RAMPING_STEP << 1); // 2dB increments in ASN.1 definition
        // - TBR check ra_PREAMBLE_RECEIVED_TARGET_POWER increment

        int preambleTransMax = -1;

        switch (nr_rach_ConfigCommon->rach_ConfigGeneric.preambleTransMax) {
        case NR_RACH_ConfigGeneric__preambleTransMax_n3:
          preambleTransMax = 3;
          break;
        case NR_RACH_ConfigGeneric__preambleTransMax_n4:
          preambleTransMax = 4;
          break;
        case NR_RACH_ConfigGeneric__preambleTransMax_n5:
          preambleTransMax = 5;
          break;
        case NR_RACH_ConfigGeneric__preambleTransMax_n6:
          preambleTransMax = 6;
          break;
        case NR_RACH_ConfigGeneric__preambleTransMax_n7:
          preambleTransMax = 7;
          break;
        case NR_RACH_ConfigGeneric__preambleTransMax_n8:
          preambleTransMax = 8;
          break;
        case NR_RACH_ConfigGeneric__preambleTransMax_n10:
          preambleTransMax = 10;
          break;
        case NR_RACH_ConfigGeneric__preambleTransMax_n20:
          preambleTransMax = 20;
          break;
        case NR_RACH_ConfigGeneric__preambleTransMax_n50:
          preambleTransMax = 50;
          break;
        case NR_RACH_ConfigGeneric__preambleTransMax_n100:
          preambleTransMax = 100;
          break;
        case NR_RACH_ConfigGeneric__preambleTransMax_n200:
          preambleTransMax = 200;
          break;
        }

        if (nr_UE_mac_inst->RA_PREAMBLE_TRANSMISSION_COUNTER == preambleTransMax + 1){

          LOG_D(MAC, "[UE %d] Frame %d: Maximum number of RACH attempts (%d)\n", mod_id, frame, preambleTransMax);

          // TBR select random backoff according to a uniform distribution between 0 and PREAMBLE_BACKOFF
          // (rand() % prach_resources->RA_PREAMBLE_BACKOFF

          // TODO TBR send message to RRC
          nr_UE_mac_inst->RA_PREAMBLE_TRANSMISSION_COUNTER = 1;
          prach_resources->ra_PREAMBLE_RECEIVED_TARGET_POWER = nr_get_Po_NOMINAL_PUSCH(mod_id, CC_id); // TBR check if this is necessary
        }

        // TBR ra_SupervisionInfo.ra_ResponseWindowSize is missing
        // nr_UE_mac_inst->RA_window_cnt = 2 + nr_rach_ConfigCommon->ra_SupervisionInfo.ra_ResponseWindowSize;
        nr_UE_mac_inst->RA_backoff_cnt = 0;

        // Fill in preamble and PRACH resources
        nr_get_prach_resources(mod_id, CC_id, gNB_id, nr_tti_tx, 0, NULL);
        return (&nr_UE_mac_inst->RA_prach_resources);
      }
    }
  } else if (UE_mode == PUSCH) {
      LOG_D(MAC, "[UE %d] FATAL: Should not have checked for RACH in PUSCH yet ...", mod_id);
      AssertFatal(1 == 0, "");
  }
 return NULL;
}