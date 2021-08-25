int oai_nfapi_hi_dci0_req(nfapi_hi_dci0_request_t *hi_dci0_req)             { return(0);  }
int oai_nfapi_tx_req(nfapi_tx_request_t *tx_req)                            { return(0);  }
int oai_nfapi_dl_config_req(nfapi_dl_config_request_t *dl_config_req)       { return(0);  }
int oai_nfapi_ul_config_req(nfapi_ul_config_request_t *ul_config_req)       { return(0);  }
int oai_nfapi_dl_tti_req(nfapi_nr_dl_tti_request_t *dl_config_req) { return(0);  }
int oai_nfapi_tx_data_req(nfapi_nr_tx_data_request_t *tx_data_req){ return(0);  }
int oai_nfapi_ul_dci_req(nfapi_nr_ul_dci_request_t *ul_dci_req){ return(0);  }
int oai_nfapi_ul_tti_req(nfapi_nr_ul_tti_request_t *ul_tti_req){ return(0);  }
int oai_nfapi_nr_rx_data_indication(nfapi_nr_rx_data_indication_t *ind) { return(0);  }
int oai_nfapi_nr_crc_indication(nfapi_nr_crc_indication_t *ind) { return(0);  }
int oai_nfapi_nr_srs_indication(nfapi_nr_srs_indication_t *ind) { return(0);  }
int oai_nfapi_nr_uci_indication(nfapi_nr_uci_indication_t *ind) { return(0);  }
int oai_nfapi_nr_rach_indication(nfapi_nr_rach_indication_t *ind) { return(0);  }

int32_t get_uldl_offset(int nr_bandP)                                       { return(0);  }
NR_IF_Module_t *NR_IF_Module_init(int Mod_id)                               {return(NULL);}
int dummy_nr_ue_dl_indication(nr_downlink_indication_t *dl_info)            { return(0);  }
int dummy_nr_ue_ul_indication(nr_uplink_indication_t *ul_info)              { return(0);  }
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
#include "LAYER2/MAC/mac.h"

extern int64_t table_6_3_3_2_2_prachConfig_Index [256][9];
extern int64_t table_6_3_3_2_3_prachConfig_Index [256][9];

//extern uint8_t  nfapi_mode;

// WIP
// This routine implements Section 5.1.2 (UE Random Access Resource Selection)
// and Section 5.1.3 (Random Access Preamble Transmission) from 3GPP TS 38.321
void nr_get_prach_resources(module_id_t mod_id,
                            int CC_id,
                            uint8_t gNB_id,
                            uint8_t t_id,
                            NR_PRACH_RESOURCES_t *prach_resources,
                            NR_RACH_ConfigDedicated_t * rach_ConfigDedicated){

  NR_UE_MAC_INST_t *mac = get_mac_inst(mod_id);
  NR_RACH_ConfigCommon_t *nr_rach_ConfigCommon;
  // NR_BeamFailureRecoveryConfig_t *beam_failure_recovery_config = &mac->RA_BeamFailureRecoveryConfig; // todo

  int messagePowerOffsetGroupB = 0, messageSizeGroupA, PLThreshold, sizeOfRA_PreamblesGroupA, numberOfRA_Preambles, i, deltaPreamble_Msg3 = 0;
  uint8_t noGroupB = 0, s_id, f_id, ul_carrier_id, msg1_FDM, prach_ConfigIndex, SFN_nbr, Msg3_size;

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

  // if (rach_ConfigDedicated) {  // This is for network controlled Mobility
  //   // operation for contention-free RA resources when:
  //   // - available SSB with SS-RSRP above rsrp-ThresholdSSB: SSB selection
  //   // - availalbe CSI-RS with CSI-RSRP above rsrp-ThresholdCSI-RS: CSI-RS selection
  //   prach_resources->ra_PreambleIndex = rach_ConfigDedicated->ra_PreambleIndex;
  //   return;
  // }

  //////////* Contention-based RA preamble selection *//////////
  // todo:
  // - selection of SSB with SS-RSRP above rsrp-ThresholdSSB else select any SSB
  // - todo determine next available PRACH occasion

  // rsrp_ThresholdSSB = *nr_rach_ConfigCommon->rsrp_ThresholdSSB;

  AssertFatal(mac->nr_rach_ConfigCommon != NULL, "[UE %d] FATAL  nr_rach_ConfigCommon is NULL !!!\n", mod_id);

  nr_rach_ConfigCommon = mac->nr_rach_ConfigCommon;

  Msg3_size = mac->RA_Msg3_size;
  numberOfRA_Preambles = *nr_rach_ConfigCommon->totalNumberOfRA_Preambles;

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

  }

  PLThreshold = prach_resources->RA_PCMAX - nr_rach_ConfigCommon->rach_ConfigGeneric.preambleReceivedTargetPower - deltaPreamble_Msg3 - messagePowerOffsetGroupB;

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
    mac->RA_PREAMBLE_TRANSMISSION_COUNTER++;

  prach_resources->ra_PREAMBLE_RECEIVED_TARGET_POWER = nr_get_Po_NOMINAL_PUSCH(prach_resources, mod_id, CC_id);

   // RA-RNTI computation (associated to PRACH occasion in which the RA Preamble is transmitted)
   // 1) this does not apply to contention-free RA Preamble for beam failure recovery request
   // 2) getting star_symb, SFN_nbr from table 6.3.3.2-3 (TDD and FR1 scenario)

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
    AssertFatal(1 == 0,"Unknown msg1_FDM %lu\n", nr_rach_ConfigCommon->rach_ConfigGeneric.msg1_FDM);
   }

   prach_ConfigIndex = nr_rach_ConfigCommon->rach_ConfigGeneric.prach_ConfigurationIndex;

   // ra_RNTI computation
   // - todo: this is for TDD FR1 only
   // - ul_carrier_id: UL carrier used for RA preamble transmission, hardcoded for NUL carrier
   // - f_id: index of the PRACH occasion in the frequency domain
   // - s_id is starting symbol of the PRACH occasion [0...14]
   // - t_id is the first slot of the PRACH occasion in a system frame [0...80]

   ul_carrier_id = 0; // NUL
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
   mac->ra_rnti = prach_resources->ra_RNTI;

   LOG_D(MAC, "Computed ra_RNTI is %d", prach_resources->ra_RNTI);
}

// TbD: RA_attempt_number not used
void nr_Msg1_transmitted(module_id_t mod_id, uint8_t CC_id, frame_t frameP, uint8_t gNB_id){
  AssertFatal(CC_id == 0, "Transmission on secondary CCs is not supported yet\n");
  NR_UE_MAC_INST_t *mac = get_mac_inst(mod_id);
  mac->ra_state = WAIT_RAR;
  // Start contention resolution timer
  mac->RA_attempt_number++;
}

void nr_Msg3_transmitted(module_id_t mod_id, uint8_t CC_id, frame_t frameP, slot_t slotP, uint8_t gNB_id){
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


