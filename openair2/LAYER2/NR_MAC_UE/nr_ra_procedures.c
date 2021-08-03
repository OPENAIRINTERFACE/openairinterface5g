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

/* Tools */
#include "SIMULATION/TOOLS/sim.h"	// for taus

/* RRC */
#include "NR_RACH-ConfigCommon.h"
#include "RRC/NR_UE/rrc_proto.h"

/* PHY */
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
#include "PHY/defs_common.h"
#include "PHY/defs_nr_common.h"
#include "PHY/NR_UE_ESTIMATION/nr_estimation.h"

/* MAC */
#include "LAYER2/NR_MAC_COMMON/nr_mac_extern.h"
#include "NR_MAC_COMMON/nr_mac.h"
#include "LAYER2/NR_MAC_UE/mac_proto.h"

#include <executables/softmodem-common.h>

void nr_get_RA_window(NR_UE_MAC_INST_t *mac);

// Random Access procedure initialization as per 5.1.1 and initialization of variables specific
// to Random Access type as specified in clause 5.1.1a (3GPP TS 38.321 version 16.2.1 Release 16)
// todo:
// - check if carrier to use is explicitly signalled then do (1) RA CARRIER SELECTION (SUL, NUL) (2) set PCMAX (currently hardcoded to 0)
void init_RA(module_id_t mod_id,
             NR_PRACH_RESOURCES_t *prach_resources,
             NR_RACH_ConfigCommon_t *nr_rach_ConfigCommon,
             NR_RACH_ConfigGeneric_t *rach_ConfigGeneric,
             NR_RACH_ConfigDedicated_t *rach_ConfigDedicated) {

  NR_UE_MAC_INST_t *mac = get_mac_inst(mod_id);

  RA_config_t *ra          = &mac->ra;
  ra->RA_active            = 1;
  ra->ra_PreambleIndex     = -1;
  ra->RA_usedGroupA        = 1;
  ra->RA_RAPID_found       = 0;
  ra->preambleTransMax     = 0;
  ra->first_Msg3           = 1;
  ra->starting_preamble_nb = 0;
  ra->RA_backoff_cnt       = 0;

  prach_resources->RA_PREAMBLE_BACKOFF = 0;
  prach_resources->RA_PCMAX = nr_get_Pcmax(mod_id);
  prach_resources->RA_PREAMBLE_TRANSMISSION_COUNTER = 1;
  prach_resources->RA_PREAMBLE_POWER_RAMPING_COUNTER = 1;
  prach_resources->POWER_OFFSET_2STEP_RA = 0;
  prach_resources->RA_SCALING_FACTOR_BI = 1;

  if (rach_ConfigDedicated) {
    if (rach_ConfigDedicated->cfra){
      LOG_I(MAC, "Initialization of 2-step contention-free random access procedure\n");
      prach_resources->RA_TYPE = RA_2STEP;
      ra->cfra = 1;
    } else if (rach_ConfigDedicated->ext1){
      if (rach_ConfigDedicated->ext1->cfra_TwoStep_r16){
        LOG_I(MAC, "In %s: setting RA type to 2-step...\n", __FUNCTION__);
        prach_resources->RA_TYPE = RA_2STEP;
        ra->cfra = 1;
      } else {
        LOG_E(MAC, "In %s: config not handled\n", __FUNCTION__);
      }
    } else {
      LOG_E(MAC, "In %s: config not handled\n", __FUNCTION__);
    }
  } else if (nr_rach_ConfigCommon){
    LOG_I(MAC, "Initialization of 4-step contention-based random access procedure\n");
    prach_resources->RA_TYPE = RA_4STEP;
    ra->cfra = 0;
  } else {
    LOG_E(MAC, "In %s: config not handled\n", __FUNCTION__);
  }

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

  switch (rach_ConfigGeneric->preambleTransMax) {
    case 0:
      ra->preambleTransMax = 3;
      break;
    case 1:
      ra->preambleTransMax = 4;
      break;
    case 2:
      ra->preambleTransMax = 5;
      break;
    case 3:
      ra->preambleTransMax = 6;
      break;
    case 4:
      ra->preambleTransMax = 7;
      break;
    case 5:
      ra->preambleTransMax = 8;
      break;
    case 6:
      ra->preambleTransMax = 10;
      break;
    case 7:
      ra->preambleTransMax = 20;
      break;
    case 8:
      ra->preambleTransMax = 50;
      break;
    case 9:
      ra->preambleTransMax = 100;
      break;
    case 10:
      ra->preambleTransMax = 200;
      break;
  }

  if (nr_rach_ConfigCommon->ext1) {
    if (nr_rach_ConfigCommon->ext1->ra_PrioritizationForAccessIdentity){
      LOG_D(MAC, "In %s:%d: Missing implementation for Access Identity initialization procedures\n", __FUNCTION__, __LINE__);
    }
  }
}

void ssb_rach_config(RA_config_t *ra, NR_PRACH_RESOURCES_t *prach_resources, NR_RACH_ConfigCommon_t *nr_rach_ConfigCommon, fapi_nr_ul_config_prach_pdu *prach_pdu){

  // Determine the SSB to RACH mapping ratio
  // =======================================

  NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR ssb_perRACH_config = nr_rach_ConfigCommon->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->present;
  boolean_t multiple_ssb_per_ro; // true if more than one or exactly one SSB per RACH occasion, false if more than one RO per SSB
  uint8_t ssb_rach_ratio; // Nb of SSBs per RACH or RACHs per SSB
  int total_preambles_per_ssb;
  uint8_t ssb_nb_in_ro;
  int numberOfRA_Preambles = 64;

  switch (ssb_perRACH_config){
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_oneEighth:
      multiple_ssb_per_ro = false;
      ssb_rach_ratio = 8;
      ra->cb_preambles_per_ssb = 4 * (nr_rach_ConfigCommon->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.oneEighth + 1);
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_oneFourth:
      multiple_ssb_per_ro = false;
      ssb_rach_ratio = 4;
      ra->cb_preambles_per_ssb = 4 * (nr_rach_ConfigCommon->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.oneFourth + 1);
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_oneHalf:
      multiple_ssb_per_ro = false;
      ssb_rach_ratio = 2;
      ra->cb_preambles_per_ssb = 4 * (nr_rach_ConfigCommon->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.oneHalf + 1);
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_one:
      multiple_ssb_per_ro = true;
      ssb_rach_ratio = 1;
      ra->cb_preambles_per_ssb = 4 * (nr_rach_ConfigCommon->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.one + 1);
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_two:
      multiple_ssb_per_ro = true;
      ssb_rach_ratio = 2;
      ra->cb_preambles_per_ssb = 4 * (nr_rach_ConfigCommon->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.two + 1);
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_four:
      multiple_ssb_per_ro = true;
      ssb_rach_ratio = 4;
      ra->cb_preambles_per_ssb = nr_rach_ConfigCommon->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.four;
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_eight:
      multiple_ssb_per_ro = true;
      ssb_rach_ratio = 8;
      ra->cb_preambles_per_ssb = nr_rach_ConfigCommon->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.eight;
      break;
    case NR_RACH_ConfigCommon__ssb_perRACH_OccasionAndCB_PreamblesPerSSB_PR_sixteen:
      multiple_ssb_per_ro = true;
      ssb_rach_ratio = 16;
      ra->cb_preambles_per_ssb = nr_rach_ConfigCommon->ssb_perRACH_OccasionAndCB_PreamblesPerSSB->choice.sixteen;
      break;
    default:
      AssertFatal(1 == 0, "Unsupported ssb_perRACH_config %d\n", ssb_perRACH_config);
  }

  if (nr_rach_ConfigCommon->totalNumberOfRA_Preambles)
    numberOfRA_Preambles = *(nr_rach_ConfigCommon->totalNumberOfRA_Preambles);

  // Compute the proper Preamble selection params according to the selected SSB and the ssb_perRACH_OccasionAndCB_PreamblesPerSSB configuration
  if ((true == multiple_ssb_per_ro) && (ssb_rach_ratio > 1)) {
    total_preambles_per_ssb = numberOfRA_Preambles / ssb_rach_ratio;

    ssb_nb_in_ro = prach_pdu->ssb_nb_in_ro;
    ra->starting_preamble_nb = total_preambles_per_ssb * ssb_nb_in_ro;
  } else {
    total_preambles_per_ssb = numberOfRA_Preambles;
    ra->starting_preamble_nb = 0;
  }
}

// This routine implements RA preamble configuration according to
// section 5.1 (Random Access procedure) of 3GPP TS 38.321 version 16.2.1 Release 16
void ra_preambles_config(NR_PRACH_RESOURCES_t *prach_resources, NR_UE_MAC_INST_t *mac, int16_t dl_pathloss){

  int messageSizeGroupA = 0;
  int sizeOfRA_PreamblesGroupA = 0;
  int messagePowerOffsetGroupB = 0;
  int PLThreshold = 0;
  long deltaPreamble_Msg3 = 0;
  uint8_t noGroupB = 0;
  RA_config_t *ra = &mac->ra;
  NR_RACH_ConfigCommon_t *setup;
  if (mac->scc) setup = mac->scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup;
  else          setup = mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup;
  NR_RACH_ConfigGeneric_t *rach_ConfigGeneric = &setup->rach_ConfigGeneric;

  NR_BWP_UplinkCommon_t *initialUplinkBWP = (mac->scc) ? mac->scc->uplinkConfigCommon->initialUplinkBWP : &mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP;
  if (initialUplinkBWP->pusch_ConfigCommon->choice.setup->msg3_DeltaPreamble){
    deltaPreamble_Msg3 = (*initialUplinkBWP->pusch_ConfigCommon->choice.setup->msg3_DeltaPreamble) * 2; // dB
    LOG_D(MAC, "In %s: deltaPreamble_Msg3 set to %ld\n", __FUNCTION__, deltaPreamble_Msg3);
  }

  if (!setup->groupBconfigured) {
    noGroupB = 1;
    LOG_D(MAC, "In %s:%d: preambles group B is not configured...\n", __FUNCTION__, __LINE__);
  } else {
    // RA preambles group B is configured
    // - Random Access Preambles group B is configured for 4-step RA type
    // - Defining the number of RA preambles in RA Preamble Group A for each SSB
    LOG_D(MAC, "In %s:%d: preambles group B is configured...\n", __FUNCTION__, __LINE__);
    sizeOfRA_PreamblesGroupA = setup->groupBconfigured->numberOfRA_PreamblesGroupA;
    switch (setup->groupBconfigured->ra_Msg3SizeGroupA){
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
      AssertFatal(1 == 0, "Unknown ra_Msg3SizeGroupA %lu\n", setup->groupBconfigured->ra_Msg3SizeGroupA);
      /* todo cases 10 -15*/
      }

      /* Power offset for preamble selection in dB */
      messagePowerOffsetGroupB = -9999;
      switch (setup->groupBconfigured->messagePowerOffsetGroupB){
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
      AssertFatal(1 == 0,"Unknown messagePowerOffsetGroupB %lu\n", setup->groupBconfigured->messagePowerOffsetGroupB);
    }

    PLThreshold = prach_resources->RA_PCMAX - rach_ConfigGeneric->preambleReceivedTargetPower - deltaPreamble_Msg3 - messagePowerOffsetGroupB;

  }

  /* Msg3 has not been transmitted yet */
  if (ra->first_Msg3) {
    if(ra->ra_PreambleIndex < 0 || ra->ra_PreambleIndex > 63) {
      if (noGroupB) {
        // use Group A preamble
        ra->ra_PreambleIndex = ra->starting_preamble_nb + ((taus()) % ra->cb_preambles_per_ssb);
        ra->RA_usedGroupA = 1;
      } else if ((ra->Msg3_size < messageSizeGroupA) && (dl_pathloss > PLThreshold)) {
        // Group B is configured and RA preamble Group A is used
        // - todo add condition on CCCH_sdu_size for initiation by CCCH
        ra->ra_PreambleIndex = ra->starting_preamble_nb + ((taus()) % sizeOfRA_PreamblesGroupA);
        ra->RA_usedGroupA = 1;
      } else {
        // Group B preamble is configured and used
        // the first sizeOfRA_PreamblesGroupA RA preambles belong to RA Preambles Group A
        // the remaining belong to RA Preambles Group B
        ra->ra_PreambleIndex = ra->starting_preamble_nb + sizeOfRA_PreamblesGroupA + ((taus()) % (ra->cb_preambles_per_ssb - sizeOfRA_PreamblesGroupA));
        ra->RA_usedGroupA = 0;
      }
    }
  } else { // Msg3 is being retransmitted
    if (ra->RA_usedGroupA && noGroupB) {
      ra->ra_PreambleIndex = ra->starting_preamble_nb + ((taus()) % ra->cb_preambles_per_ssb);
    } else if (ra->RA_usedGroupA && !noGroupB){
      ra->ra_PreambleIndex = ra->starting_preamble_nb + ((taus()) % sizeOfRA_PreamblesGroupA);
    } else {
      ra->ra_PreambleIndex = ra->starting_preamble_nb + sizeOfRA_PreamblesGroupA + ((taus()) % (ra->cb_preambles_per_ssb - sizeOfRA_PreamblesGroupA));
    }
  }
  prach_resources->ra_PreambleIndex = ra->ra_PreambleIndex;
}

// RA-RNTI computation (associated to PRACH occasion in which the RA Preamble is transmitted)
// - this does not apply to contention-free RA Preamble for beam failure recovery request
// - getting star_symb, SFN_nbr from table 6.3.3.2-3 (TDD and FR1 scenario)
// - ul_carrier_id: UL carrier used for RA preamble transmission, hardcoded for NUL carrier
// - f_id: index of the PRACH occasion in the frequency domain
// - s_id is starting symbol of the PRACH occasion [0...14]
// - t_id is the first slot of the PRACH occasion in a system frame [0...80]
uint16_t set_ra_rnti(NR_UE_MAC_INST_t *mac, fapi_nr_ul_config_prach_pdu *prach_pdu){

  RA_config_t *ra = &mac->ra;
  uint8_t ul_carrier_id = 0; // NUL
  uint8_t f_id = prach_pdu->num_ra;
  uint8_t t_id = prach_pdu->prach_slot;
  uint8_t s_id = prach_pdu->prach_start_symbol;

  ra->ra_rnti = 1 + s_id + 14 * t_id + 1120 * f_id + 8960 * ul_carrier_id;

  LOG_D(MAC, "Computed ra_RNTI is %x \n", ra->ra_rnti);

  return ra->ra_rnti;

}

// This routine implements Section 5.1.2 (UE Random Access Resource Selection)
// and Section 5.1.3 (Random Access Preamble Transmission) from 3GPP TS 38.321
// - currently the PRACH preamble is set through RRC configuration for 4-step CFRA mode
// todo:
// - determine next available PRACH occasion
// -- if RA initiated for SI request and ra_AssociationPeriodIndex and si-RequestPeriod are configured
// -- else if SSB is selected above
// -- else if CSI-RS is selected above
// - switch initialisation cases
// -- RA initiated by beam failure recovery operation (subclause 5.17 TS 38.321)
// --- SSB selection, set prach_resources->ra_PreambleIndex
// -- RA initiated by PDCCH: ra_preamble_index provided by PDCCH && ra_PreambleIndex != 0b000000
// --- set PREAMBLE_INDEX to ra_preamble_index
// --- select the SSB signalled by PDCCH
// -- RA initiated for SI request:
// --- SSB selection, set prach_resources->ra_PreambleIndex
// - condition on notification of suspending power ramping counter from lower layer (5.1.3 TS 38.321)
// - check if SSB or CSI-RS have not changed since the selection in the last RA Preamble tranmission
// - Contention-based RA preamble selection:
// -- selection of SSB with SS-RSRP above rsrp-ThresholdSSB else select any SSB
void nr_get_prach_resources(module_id_t mod_id,
                            int CC_id,
                            uint8_t gNB_id,
                            NR_PRACH_RESOURCES_t *prach_resources,
                            fapi_nr_ul_config_prach_pdu *prach_pdu,
                            NR_RACH_ConfigDedicated_t * rach_ConfigDedicated){

  NR_UE_MAC_INST_t *mac = get_mac_inst(mod_id);
  RA_config_t *ra = &mac->ra;

  NR_RACH_ConfigCommon_t *nr_rach_ConfigCommon = (mac->scc)?
    mac->scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup : 
    mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup;

  LOG_D(PHY, "In %s: getting PRACH resources frame (first_Msg3 %d)\n", __FUNCTION__, ra->first_Msg3);

  if (rach_ConfigDedicated) {
    if (rach_ConfigDedicated->cfra){
      uint8_t cfra_ssb_resource_idx = 0;
      prach_resources->ra_PreambleIndex = rach_ConfigDedicated->cfra->resources.choice.ssb->ssb_ResourceList.list.array[cfra_ssb_resource_idx]->ra_PreambleIndex;
      LOG_D(MAC, "In %s: selected RA preamble index %d for contention-free random access procedure for SSB with Id %d\n", __FUNCTION__, prach_resources->ra_PreambleIndex, cfra_ssb_resource_idx);
    }
  } else {
    int16_t dl_pathloss = get_nr_PL(mod_id, CC_id, gNB_id);
    ssb_rach_config(ra, prach_resources, nr_rach_ConfigCommon, prach_pdu);
    ra_preambles_config(prach_resources, mac, dl_pathloss);
    LOG_D(MAC, "[RAPROC] - Selected RA preamble index %d for contention-based random access procedure... \n", prach_resources->ra_PreambleIndex);
  }

  if (prach_resources->RA_PREAMBLE_TRANSMISSION_COUNTER > 1)
    prach_resources->RA_PREAMBLE_POWER_RAMPING_COUNTER++;
  prach_resources->ra_PREAMBLE_RECEIVED_TARGET_POWER = nr_get_Po_NOMINAL_PUSCH(prach_resources, mod_id, CC_id);
  prach_resources->ra_RNTI = set_ra_rnti(mac, prach_pdu);

}

// TbD: RA_attempt_number not used
void nr_Msg1_transmitted(module_id_t mod_id, uint8_t CC_id, frame_t frameP, uint8_t gNB_id){
  NR_UE_MAC_INST_t *mac = get_mac_inst(mod_id);
  RA_config_t *ra = &mac->ra;
  ra->ra_state = WAIT_RAR;
  ra->RA_attempt_number++;
}

void nr_Msg3_transmitted(module_id_t mod_id, uint8_t CC_id, frame_t frameP, uint8_t gNB_id){

  NR_UE_MAC_INST_t *mac = get_mac_inst(mod_id);
  NR_RACH_ConfigCommon_t *nr_rach_ConfigCommon = (mac->scc) ? 
    mac->scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup:
    mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup;
  RA_config_t *ra = &mac->ra;

  LOG_D(MAC,"In %s: [UE %d] Frame %d, CB-RA: starting contention resolution timer\n", __FUNCTION__, mod_id, frameP);

  // start contention resolution timer
  ra->RA_contention_resolution_cnt = (nr_rach_ConfigCommon->ra_ContentionResolutionTimer + 1) * 8;
  ra->RA_contention_resolution_timer_active = 1;
  ra->ra_state = WAIT_CONTENTION_RESOLUTION;

}

/**
 * Function:            handles Random Access Preamble Initialization (5.1.1 TS 38.321)
 *                      handles Random Access Response reception (5.1.4 TS 38.321)
 * Note:                In SA mode the Msg3 contains a CCCH SDU, therefore no C-RNTI MAC CE is transmitted.
 *
 * @prach_resources     pointer to PRACH resources
 * @prach_pdu           pointer to FAPI UL PRACH PDU
 * @mod_id              module ID
 * @CC_id               CC ID
 * @frame               current UL TX frame
 * @gNB_id              gNB ID
 * @nr_slot_tx          current UL TX slot
 */
uint8_t nr_ue_get_rach(NR_PRACH_RESOURCES_t *prach_resources,
                       fapi_nr_ul_config_prach_pdu *prach_pdu,
                       module_id_t mod_id,
                       int CC_id,
                       frame_t frame,
                       uint8_t gNB_id,
                       int nr_slot_tx){

  NR_UE_MAC_INST_t *mac = get_mac_inst(mod_id);
  RA_config_t *ra = &mac->ra;
  NR_RACH_ConfigCommon_t *setup;
  if (mac->scc) setup = mac->scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup;
  else          setup = mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup;
  AssertFatal(&setup->rach_ConfigGeneric != NULL, "In %s: FATAL! rach_ConfigGeneric is NULL...\n", __FUNCTION__);
  NR_RACH_ConfigGeneric_t *rach_ConfigGeneric = &setup->rach_ConfigGeneric;
  NR_RACH_ConfigDedicated_t *rach_ConfigDedicated = ra->rach_ConfigDedicated;

  // Delay init RA procedure to allow the convergence of the IIR filter on PRACH noise measurements at gNB side
  if (!prach_resources->init_msg1) {
    if ( (mac->common_configuration_complete>0 || get_softmodem_params()->do_ra==1) && ((MAX_FRAME_NUMBER+frame-prach_resources->sync_frame)%MAX_FRAME_NUMBER)>150 ){
      prach_resources->init_msg1 = 1;
    } else {
      LOG_D(NR_MAC,"PRACH Condition not met: frame %d, prach_resources->sync_frame %d\n",frame,prach_resources->sync_frame);
      return 0;
    }
  }

  LOG_D(NR_MAC, "In %s: [UE %d][%d.%d]: init_msg1 %d, ra_state %d, RA_active %d\n",
    __FUNCTION__,
    mod_id,
    frame,
    nr_slot_tx,
    prach_resources->init_msg1,
    ra->ra_state,
    ra->RA_active);

  if (prach_resources->init_msg1 && ra->ra_state != RA_SUCCEEDED) {

    if (ra->RA_active == 0) {
      /* RA not active - checking if RRC is ready to initiate the RA procedure */

      LOG_D(NR_MAC, "In %s: RA not active. Checking for data to transmit from upper layers...\n", __FUNCTION__);

      const uint8_t lcid = UL_SCH_LCID_CCCH;
      const uint8_t sh_size = sizeof(NR_MAC_SUBHEADER_FIXED);
      const uint8_t TBS_max = 8 + sizeof(NR_MAC_SUBHEADER_SHORT) + sizeof(NR_MAC_SUBHEADER_SHORT); // Note: unclear the reason behind the selection of such TBS_max
      int8_t size_sdu = 0;
      uint8_t mac_ce[16] = {0};
      uint8_t *pdu = get_softmodem_params()->sa ? mac->CCCH_pdu.payload : mac_ce;
      uint8_t *payload = pdu;

      // Concerning the C-RNTI MAC CE, it has to be included if the UL transmission (Msg3) is not being made for the CCCH logical channel.
      // Therefore it has been assumed that this event only occurs only when RA is done and it is not SA mode.
      if (get_softmodem_params()->sa) {

        NR_MAC_SUBHEADER_FIXED *header = (NR_MAC_SUBHEADER_FIXED *) pdu;
        pdu += sh_size;

        // initialisation by RRC
        nr_rrc_ue_generate_RRCSetupRequest(mod_id,gNB_id);

        // CCCH PDU
        size_sdu = nr_mac_rrc_data_req_ue(mod_id, CC_id, gNB_id, frame, CCCH, pdu);
        LOG_D(NR_MAC, "In %s: [UE %d][%d.%d]: Requested RRCConnectionRequest, got %d bytes for LCID 0x%02x \n", __FUNCTION__, mod_id, frame, nr_slot_tx, size_sdu, lcid);

        if (size_sdu > 0) {

          // UE Contention Resolution Identity
          // Store the first 48 bits belonging to the uplink CCCH SDU within Msg3 to determine whether or not the
          // Random Access Procedure has been successful after reception of Msg4
          memcpy(ra->cont_res_id, pdu, sizeof(uint8_t) * 6);

          pdu += size_sdu;
          ra->Msg3_size = size_sdu + sh_size;

          // Build header
          header->R = 0;
          header->LCID = lcid;

        } else {
          pdu -= sh_size;
        }

      } else {

        size_sdu = nr_write_ce_ulsch_pdu(pdu, mac);
        pdu += size_sdu;
        ra->Msg3_size = size_sdu;

      }

      if (size_sdu > 0 && ra->generate_nr_prach == GENERATE_PREAMBLE) {

        LOG_D(NR_MAC, "In %s: [UE %d][%d.%d]: starting initialisation Random Access Procedure...\n", __FUNCTION__, mod_id, frame, nr_slot_tx);
        AssertFatal(TBS_max > ra->Msg3_size, "In %s: allocated resources are not enough for Msg3!\n", __FUNCTION__);

        // Init RA procedure
        init_RA(mod_id, prach_resources, setup, rach_ConfigGeneric, rach_ConfigDedicated);
        nr_get_RA_window(mac);
        // Fill in preamble and PRACH resources
        nr_get_prach_resources(mod_id, CC_id, gNB_id, prach_resources, prach_pdu, rach_ConfigDedicated);

        // Padding: fill remainder with 0
        if (TBS_max - ra->Msg3_size > 0) {
          LOG_D(NR_MAC, "In %s: remaining %d bytes, filling with padding\n", __FUNCTION__, TBS_max - ra->Msg3_size);
          ((NR_MAC_SUBHEADER_FIXED *) pdu)->R = 0;
          ((NR_MAC_SUBHEADER_FIXED *) pdu)->LCID = UL_SCH_LCID_PADDING;
          pdu += sizeof(NR_MAC_SUBHEADER_FIXED);
          for (int j = 0; j < TBS_max - ra->Msg3_size - sizeof(NR_MAC_SUBHEADER_FIXED); j++) {
            pdu[j] = 0;
          }
        }

        // Dumping ULSCH payload
        LOG_D(NR_MAC, "In %s: dumping UL Msg3 MAC PDU with length %d: \n", __FUNCTION__, TBS_max);
        for(int k = 0; k < TBS_max; k++) {
          LOG_D(NR_MAC,"(%i): %i\n", k, payload[k]);
        }

        // Msg3 was initialized with TBS_max bytes because the RA_Msg3_size will only be known after
        // receiving Msg2 (which contains the Msg3 resource reserve).
        // Msg3 will be transmitted with RA_Msg3_size bytes, removing unnecessary 0s.
        mac->ulsch_pdu.Pdu_size = TBS_max;
        memcpy(mac->ulsch_pdu.payload, payload, TBS_max);

      } else {
        return 0;
      }
    } else if (ra->RA_window_cnt != -1) { // RACH is active

      LOG_D(MAC, "In %s [%d.%d] RA is active: RA window count %d, RA backoff count %d\n", __FUNCTION__, frame, nr_slot_tx, ra->RA_window_cnt, ra->RA_backoff_cnt);

      if (ra->RA_BI_found){
        prach_resources->RA_PREAMBLE_BACKOFF = prach_resources->RA_SCALING_FACTOR_BI * ra->RA_backoff_indicator;
      } else {
        prach_resources->RA_PREAMBLE_BACKOFF = 0;
      }

      if (ra->RA_window_cnt >= 0 && ra->RA_RAPID_found == 1) {

        if(ra->cfra) {
          // Reset RA_active flag: it disables Msg3 retransmission (8.3 of TS 38.213)
          nr_ra_succeeded(mod_id, frame, nr_slot_tx);
        } else {
          ra->generate_nr_prach = GENERATE_IDLE;
        }

      } else if (ra->RA_window_cnt == 0 && !ra->RA_RAPID_found) {

        LOG_I(MAC, "[UE %d][%d:%d] RAR reception failed \n", mod_id, frame, nr_slot_tx);

        nr_ra_failed(mod_id, CC_id, prach_resources, frame, nr_slot_tx);

      } else if (ra->RA_window_cnt > 0) {

        LOG_D(MAC, "[UE %d][%d.%d]: RAR not received yet (RA window count %d) \n", mod_id, frame, nr_slot_tx, ra->RA_window_cnt);

        // Fill in preamble and PRACH resources
        ra->RA_window_cnt--;
        if (ra->generate_nr_prach == GENERATE_PREAMBLE) {
          nr_get_prach_resources(mod_id, CC_id, gNB_id, prach_resources, prach_pdu, rach_ConfigDedicated);
        }
      } else if (ra->RA_backoff_cnt > 0) {

        LOG_D(MAC, "[UE %d][%d.%d]: RAR not received yet (RA backoff count %d) \n", mod_id, frame, nr_slot_tx, ra->RA_backoff_cnt);

        ra->RA_backoff_cnt--;

        if ((ra->RA_backoff_cnt > 0 && ra->generate_nr_prach == GENERATE_PREAMBLE) || ra->RA_backoff_cnt == 0) {
          nr_get_prach_resources(mod_id, CC_id, gNB_id, prach_resources, prach_pdu, rach_ConfigDedicated);
        }

      }
    }
  }

  if (ra->RA_contention_resolution_timer_active){
    nr_ue_contention_resolution(mod_id, CC_id, frame, nr_slot_tx, prach_resources);
  }

  LOG_D(MAC,"ra->generate_nr_prach %d ra->ra_state %d (GENERATE_IDLE %d)\n",ra->generate_nr_prach,ra->ra_state,GENERATE_IDLE);
  if(ra->generate_nr_prach != GENERATE_IDLE) {
    return ra->generate_nr_prach;
  } else {
    return ra->ra_state;
  }

}

void nr_get_RA_window(NR_UE_MAC_INST_t *mac){

  uint8_t mu, ra_ResponseWindow;
  RA_config_t *ra = &mac->ra;
  NR_RACH_ConfigCommon_t *setup;
  if (mac->scc) setup = mac->scc->uplinkConfigCommon->initialUplinkBWP->rach_ConfigCommon->choice.setup;
  else          setup = mac->scc_SIB->uplinkConfigCommon->initialUplinkBWP.rach_ConfigCommon->choice.setup;
  AssertFatal(&setup->rach_ConfigGeneric != NULL, "In %s: FATAL! rach_ConfigGeneric is NULL...\n", __FUNCTION__);
  NR_RACH_ConfigGeneric_t *rach_ConfigGeneric = &setup->rach_ConfigGeneric;
  long scs = (mac->scc) ? 
    mac->scc->downlinkConfigCommon->frequencyInfoDL->scs_SpecificCarrierList.list.array[0]->subcarrierSpacing :
    mac->scc_SIB->downlinkConfigCommon.frequencyInfoDL.scs_SpecificCarrierList.list.array[0]->subcarrierSpacing;
 
  ra_ResponseWindow = rach_ConfigGeneric->ra_ResponseWindow;

  if (setup->msg1_SubcarrierSpacing)
    mu = *setup->msg1_SubcarrierSpacing;
  else
    mu = scs;

  ra->RA_window_cnt = ra->RA_offset*nr_slots_per_frame[mu]; // taking into account the 2 frames gap introduced by OAI gNB

  switch (ra_ResponseWindow) {
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl1:
      ra->RA_window_cnt += 1;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl2:
      ra->RA_window_cnt += 2;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl4:
      ra->RA_window_cnt += 4;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl8:
      ra->RA_window_cnt += 8;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl10:
      ra->RA_window_cnt += 10;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl20:
      ra->RA_window_cnt += 20;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl40:
      ra->RA_window_cnt += 40;
      break;
    case NR_RACH_ConfigGeneric__ra_ResponseWindow_sl80:
      ra->RA_window_cnt += 80;
      break;
  }
}

////////////////////////////////////////////////////////////////////////////
/////////* Random Access Contention Resolution (5.1.35 TS 38.321) */////////
////////////////////////////////////////////////////////////////////////////
// Handling contention resolution timer
// WIP todo:
// - beam failure recovery
// - RA completed
void nr_ue_contention_resolution(module_id_t module_id, int cc_id, frame_t frame, int slot, NR_PRACH_RESOURCES_t *prach_resources){
  
  NR_UE_MAC_INST_t *mac = get_mac_inst(module_id);
  RA_config_t *ra = &mac->ra;

  if (ra->RA_contention_resolution_timer_active == 1) {

      ra->RA_contention_resolution_cnt--;

      LOG_D(MAC, "In %s: [%d.%d] RA contention resolution timer %d\n", __FUNCTION__, frame, slot, ra->RA_contention_resolution_cnt);

      if (ra->RA_contention_resolution_cnt == 0) {
        ra->t_crnti = 0;
        ra->RA_active = 0;
        ra->RA_contention_resolution_timer_active = 0;
        // Signal PHY to quit RA procedure
        LOG_E(MAC, "[UE %d] CB-RA: Contention resolution timer has expired, RA procedure has failed...\n", module_id);
        nr_ra_failed(module_id, cc_id, prach_resources, frame, slot);
      }
    
  }
}

// Handlig successful RA completion @ MAC layer
// according to section 5 of 3GPP TS 38.321 version 16.2.1 Release 16
// todo:
// - complete handling of received contention-based RA preamble
void nr_ra_succeeded(module_id_t mod_id, frame_t frame, int slot){

  NR_UE_MAC_INST_t *mac = get_mac_inst(mod_id);
  RA_config_t *ra = &mac->ra;

  if (ra->cfra) {

    LOG_I(MAC, "[UE %d][%d.%d][RAPROC] RA procedure succeeded. CF-RA: RAR successfully received.\n", mod_id, frame, slot);

    ra->RA_window_cnt = -1;

  } else {

    LOG_I(MAC, "[UE %d][%d.%d][RAPROC] RA procedure succeeded. CB-RA: Contention Resolution is successful.\n", mod_id, frame, slot);

    ra->RA_contention_resolution_cnt = -1;
    ra->RA_contention_resolution_timer_active = 0;
    mac->crnti = ra->t_crnti;
    ra->t_crnti = 0;

    LOG_D(MAC, "In %s: [UE %d][%d.%d] CB-RA: cleared contention resolution timer...\n", __FUNCTION__, mod_id, frame, slot);

  }

  LOG_D(MAC, "In %s: [UE %d] clearing RA_active flag...\n", __FUNCTION__, mod_id);
  ra->RA_active = 0;
  ra->generate_nr_prach = GENERATE_IDLE;
  ra->ra_state = RA_SUCCEEDED;

}

// Handling failure of RA procedure @ MAC layer
// according to section 5 of 3GPP TS 38.321 version 16.2.1 Release 16
// todo:
// - complete handling of received contention-based RA preamble
void nr_ra_failed(uint8_t mod_id, uint8_t CC_id, NR_PRACH_RESOURCES_t *prach_resources, frame_t frame, int slot) {

  NR_UE_MAC_INST_t *mac = get_mac_inst(mod_id);
  RA_config_t *ra = &mac->ra;

  ra->first_Msg3 = 1;
  ra->ra_PreambleIndex = -1;
  ra->generate_nr_prach = RA_FAILED;
  ra->ra_state = RA_UE_IDLE;

  prach_resources->RA_PREAMBLE_TRANSMISSION_COUNTER++;

  if (prach_resources->RA_PREAMBLE_TRANSMISSION_COUNTER == ra->preambleTransMax + 1){

    LOG_D(MAC, "In %s: [UE %d][%d.%d] Maximum number of RACH attempts (%d) reached, selecting backoff time...\n",
          __FUNCTION__, mod_id, frame, slot, ra->preambleTransMax);

    ra->RA_backoff_cnt = rand() % (prach_resources->RA_PREAMBLE_BACKOFF + 1);
    prach_resources->RA_PREAMBLE_TRANSMISSION_COUNTER = 1;
    prach_resources->RA_PREAMBLE_POWER_RAMPING_STEP += 2; // 2 dB increment
    prach_resources->ra_PREAMBLE_RECEIVED_TARGET_POWER = nr_get_Po_NOMINAL_PUSCH(prach_resources, mod_id, CC_id);

  } else {
    // Resetting RA window
    nr_get_RA_window(mac);
  }

}
