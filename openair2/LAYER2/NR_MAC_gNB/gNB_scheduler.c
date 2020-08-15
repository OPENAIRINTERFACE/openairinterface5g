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

/*! \file gNB_scheduler.c
 * \brief gNB scheduler top level function operates on per subframe basis
 * \author  Navid Nikaein and Raymond Knopp, WEI-TAI CHEN
 * \date 2010 - 2014, 2018
 * \email: navid.nikaein@eurecom.fr, kroempa@gmail.com
 * \version 0.5
 * \company Eurecom, NTUST
 * @ingroup _mac

 */

#include "assertions.h"

#include "NR_MAC_COMMON/nr_mac_extern.h"
#include "NR_MAC_gNB/mac_proto.h"

#include "common/utils/LOG/log.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"

#include "RRC/NR/nr_rrc_extern.h"

//#include "LAYER2/MAC/pre_processor.c"
#include "pdcp.h"

#include "openair1/PHY/defs_gNB.h"
#include "openair1/PHY/NR_TRANSPORT/nr_dlsch.h"

#include "intertask_interface.h"

#include "executables/softmodem-common.h"
#include "nfapi/oai_integration/vendor_ext.h"

uint16_t nr_pdcch_order_table[6] = { 31, 31, 511, 2047, 2047, 8191 };

#define MAX_SSB_SCHED 8
#define L1_RSRP_HYSTERIS 10 //considering 10 dBm as hysterisis for avoiding frequent SSB Beam Switching. !Fixme provide exact value if any
//#define L1_DIFF_RSRP_STEP_SIZE 2
#define MAX_NUM_SSB 128
#define MIN_RSRP_VALUE -141
//Measured RSRP Values Table 10.1.16.1-1 from 36.133
//Stored all the upper limits[Max RSRP Value of corresponding index]
//stored -1 for invalid values
int L1_SSB_CSI_RSRP_measReport_mapping_38133_10_1_6_1_1[128] = {
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, //0 - 9
    -1, -1, -1, -1, -1, -1, -140, -139, -138, -137, //10 - 19
    -136, -135, -134, -133, -132, -131, -130, -129, -128, -127, //20 - 29
    -126, -125, -124, -123, -122, -121, -120, -119, -118, -117, //30 - 39
    -116, -115, -114, -113, -112, -111, -110, -109, -108, -107, //40 - 49
    -106, -105, -104, -103, -102, -101, -100, -99, -98, -97, //50 - 59
    -96, -95, -94, -93, -92, -91, -90, -89, -88, -87, //60 - 69
    -86, -85, -84, -83, -82, -81, -80, -79, -78, -77, //70 - 79
    -76, -75, -74, -73, -72, -71, -70, -69, -68, -67, //80 - 89
    -66, -65, -64, -63, -62, -61, -60, -59, -58, -57, //90 - 99
    -56, -55, -54, -53, -52, -51, -50, -49, -48, -47, //100 - 109
    -46, -45, -44, -44, -1, -1, -1, -1, -1, -1, //110 - 119
    -1, -1, -1, -1, -1, -1, -1, -1//120 - 127
  };

//Differential RSRP values Table 10.1.6.1-2 from 36.133
//Stored the upper limits[MAX RSRP Value]
int diff_rsrp_ssb_csi_meas_10_1_6_1_2[16] = {
  0, -2, -4, -6, -8, -10, -12, -14, -16, -18, //0 - 9
  -20, -22, -24, -26, -28, -30 //10 - 15
};


//returns the measured RSRP value (upper limit)
int get_measured_rsrp(uint8_t index) {
  //if index is invalid returning minimum rsrp -140
  if((index >= 0 && index <= 15) || index >= 114)
    return MIN_RSRP_VALUE;

  return L1_SSB_CSI_RSRP_measReport_mapping_38133_10_1_6_1_1[index];
}

//returns the differential RSRP value (upper limit)
int get_diff_rsrp(uint8_t index, int strongest_rsrp) {
  if(strongest_rsrp != -1) {
    return strongest_rsrp + diff_rsrp_ssb_csi_meas_10_1_6_1_2[index];
  } else
    return MIN_RSRP_VALUE;
}

int checkTargetSSBInFirst64TCIStates_pdschConfig(int ssb_index_t, int Mod_idP, int UE_id) {
  NR_UE_list_t *UE_list = &RC.nrmac[Mod_idP]->UE_list;
  NR_CellGroupConfig_t *secondaryCellGroup = UE_list->secondaryCellGroup[UE_id] ;
  int nb_tci_states = secondaryCellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->pdsch_Config->choice.setup->tci_StatesToAddModList->list.count;
  NR_TCI_State_t *tci =NULL;
  int i;

  for(i=0; i<nb_tci_states && i<64; i++) {
    tci = (NR_TCI_State_t *)secondaryCellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->pdsch_Config->choice.setup->tci_StatesToAddModList->list.array[i];

    if(tci != NULL) {
      if(tci->qcl_Type1.referenceSignal.present == NR_QCL_Info__referenceSignal_PR_ssb) {
        if(tci->qcl_Type1.referenceSignal.choice.ssb == ssb_index_t)
          return tci->tci_StateId;  // returned TCI state ID
      }
      // if type2 is configured
      else if(tci->qcl_Type2 != NULL && tci->qcl_Type2->referenceSignal.present == NR_QCL_Info__referenceSignal_PR_ssb) {
        if(tci->qcl_Type2->referenceSignal.choice.ssb == ssb_index_t)
          return tci->tci_StateId; // returned TCI state ID
      } else LOG_I(MAC,"SSB index is not found in first 64 TCI states of TCI_statestoAddModList[%d]", i);
    }
  }

  // tci state not identified in first 64 TCI States of PDSCH Config
  return -1;
}

int checkTargetSSBInTCIStates_pdcchConfig(int ssb_index_t, int Mod_idP, int UE_id) {
  NR_UE_list_t *UE_list = &RC.nrmac[Mod_idP]->UE_list;
  NR_CellGroupConfig_t *secondaryCellGroup = UE_list->secondaryCellGroup[UE_id] ;
  int nb_tci_states = secondaryCellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->pdsch_Config->choice.setup->tci_StatesToAddModList->list.count;
  NR_TCI_State_t *tci =NULL;
  NR_TCI_StateId_t *tci_id = NULL;
  int bwp_id = 1;
  NR_BWP_Downlink_t *bwp = secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[bwp_id-1];
  NR_ControlResourceSet_t *coreset = bwp->bwp_Dedicated->pdcch_Config->choice.setup->controlResourceSetToAddModList->list.array[bwp_id-1];
  int i;
  int flag = 0;
  int tci_stateID = -1;

  for(i=0; i<nb_tci_states && i<128; i++) {
    tci = (NR_TCI_State_t *)secondaryCellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->pdsch_Config->choice.setup->tci_StatesToAddModList->list.array[i];

    if(tci != NULL && tci->qcl_Type1.referenceSignal.present == NR_QCL_Info__referenceSignal_PR_ssb) {
      if(tci->qcl_Type1.referenceSignal.choice.ssb == ssb_index_t) {
        flag = 1;
        tci_stateID = tci->tci_StateId;
        break;
      } else if(tci->qcl_Type2 != NULL && tci->qcl_Type2->referenceSignal.present == NR_QCL_Info__referenceSignal_PR_ssb) {
        flag = 1;
        tci_stateID = tci->tci_StateId;
        break;
      }
    }

    if(flag != 0 && tci_stateID != -1 && coreset != NULL) {
      for(i=0; i<64 && i<coreset->tci_StatesPDCCH_ToAddList->list.count; i++) {
        tci_id = coreset->tci_StatesPDCCH_ToAddList->list.array[i];

        if(tci_id != NULL && *tci_id == tci_stateID)
          return tci_stateID;
      }
    }
  }

  // Need to implement once configuration is received
  return -1;
}

int ssb_index_sorted[MAX_NUM_SSB] = {0};
int ssb_rsrp_sorted[MAX_NUM_SSB] = {0};
//Sorts ssb_index and ssb_rsrp array data and keeps in ssb_index_sorted and
//ssb_rsrp_sorted respectively
void ssb_rsrp_sort(int *ssb_index, int *ssb_rsrp) {
  int i, j;

  for(i = 0; *(ssb_index+i) != 0; i++) {
    for(j = i; *(ssb_index+j) != 0; j++) {
      if(*(ssb_rsrp+j) >= *(ssb_rsrp+i)) {
        ssb_index_sorted[i] = *(ssb_index+j);
        ssb_rsrp_sorted[i] = *(ssb_rsrp+j);
      }
    }
  }
}

void clear_mac_stats(gNB_MAC_INST *gNB) {
  memset((void*)gNB->UE_info.mac_stats,0,MAX_MOBILES_PER_GNB*sizeof(NR_mac_stats_t));
}

void dump_mac_stats(gNB_MAC_INST *gNB)
{
  NR_UE_info_t *UE_info = &gNB->UE_info;
  int num = 1;
  for (int UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id]) {
    LOG_I(MAC, "UE ID %d RNTI %04x (%d/%d)\n", UE_id, UE_info->rnti[UE_id], num++, UE_info->num_UEs);
    const NR_mac_stats_t *stats = &UE_info->mac_stats[UE_id];
    LOG_I(MAC, "UE %d: dlsch_rounds %d/%d/%d/%d, dlsch_errors %d\n",
          UE_id,
          stats->dlsch_rounds[0], stats->dlsch_rounds[1],
          stats->dlsch_rounds[2], stats->dlsch_rounds[3], stats->dlsch_errors);
    LOG_I(MAC, "UE %d: dlsch_total_bytes %d\n", UE_id, stats->dlsch_total_bytes);
    LOG_I(MAC, "UE %d: ulsch_rounds %d/%d/%d/%d, ulsch_errors %d\n",
          UE_id,
          stats->ulsch_rounds[0], stats->ulsch_rounds[1],
          stats->ulsch_rounds[2], stats->ulsch_rounds[3], stats->ulsch_errors);
    LOG_I(MAC,
          "UE %d: ulsch_total_bytes_scheduled %d, ulsch_total_bytes_received %d\n",
          UE_id,
          stats->ulsch_total_bytes_scheduled, stats->ulsch_total_bytes_rx);
    for (int lc_id = 0; lc_id < 63; lc_id++) {
      if (stats->lc_bytes_tx[lc_id] > 0)
        LOG_I(MAC, "UE %d: LCID %d: %d bytes TX\n", UE_id, lc_id, stats->lc_bytes_tx[lc_id]);
      if (stats->lc_bytes_rx[lc_id] > 0)
        LOG_I(MAC, "UE %d: LCID %d: %d bytes RX\n", UE_id, lc_id, stats->lc_bytes_rx[lc_id]);
    }
  }
}

//identifies the target SSB Beam index
//keeps the required date for PDCCH and PDSCH TCI state activation/deactivation CE consutruction globally
//handles triggering of PDCCH and PDSCH MAC CEs
void tci_handling(module_id_t Mod_idP, int UE_id, int CC_id, NR_UE_sched_ctrl_t *sched_ctrl, frame_t frame, slot_t slot) {

  int strongest_ssb_rsrp = 0;
  int cqi_idx = 0;
  int curr_ssb_beam_index = 0; //ToDo: yet to know how to identify the serving ssb beam index
  uint8_t target_ssb_beam_index = curr_ssb_beam_index;
  //uint8_t max_reported_RSRP = 16;
  //int serving_SSB_Beam_RSRP;
  uint8_t is_triggering_ssb_beam_switch =0;
  uint8_t ssb_idx = 0;
  int pdsch_bwp_id =0;
  int ssb_index[MAX_NUM_SSB] = {0};
  int ssb_rsrp[MAX_NUM_SSB] = {0};
  uint8_t idx = 0;
  int bwp_id  = 1;
  NR_UE_list_t *UE_list = &RC.nrmac[Mod_idP]->UE_list;
  //NR_COMMON_channels_t *cc = RC.nrmac[Mod_idP]->common_channels;
  NR_CellGroupConfig_t *secondaryCellGroup = UE_list->secondaryCellGroup[UE_id];
  NR_BWP_Downlink_t *bwp = secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[bwp_id-1];
  //NR_CSI_MeasConfig_t *csi_MeasConfig = UE_list->secondaryCellGroup[UE_id]->spCellConfig->spCellConfigDedicated->csi_MeasConfig->choice.setup;
  //bwp indicator
  int n_dl_bwp = secondaryCellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count;
  uint8_t nr_ssbri_cri = 0;
  uint8_t nb_of_csi_ssb_report = UE_list->csi_report_template[UE_id][cqi_idx].nb_of_csi_ssb_report;
  //uint8_t bitlen_ssbri = log (nb_of_csi_ssb_report)/log (2);
  //uint8_t max_rsrp_reported = -1;
  int better_rsrp_reported = -140-(-0); /*minimum_measured_RSRP_value - minimum_differntail_RSRP_value*///considering the minimum RSRP value as better RSRP initially
  uint8_t diff_rsrp_idx = 0;
  uint8_t i, j;

  if (n_dl_bwp < 4)
    pdsch_bwp_id = bwp_id;
  else
    pdsch_bwp_id = bwp_id - 1; // as per table 7.3.1.1.2-1 in 38.212

  /*Example:
  CRI_SSBRI: 1 2 3 4| 5 6 7 8| 9 10 1 2|
  nb_of_csi_ssb_report = 3 //3 sets as above
  nr_ssbri_cri = 4 //each set has 4 elements
  storing ssb indexes in ssb_index array as ssb_index[0] = 1 .. ssb_index[4] = 5
  ssb_rsrp[0] = strongest rsrp in first set, ssb_rsrp[4] = strongest rsrp in second set, ..
  idx: resource set index
  */

  //for all reported SSB
  for (idx = 0; idx < nb_of_csi_ssb_report; idx++) {
    nr_ssbri_cri = sched_ctrl->CSI_report[idx].choice.ssb_cri_report.nr_ssbri_cri;
    //if group based beam Reporting is disabled
    /*if(NR_CSI_ReportConfig__groupBasedBeamReporting_PR_disabled ==
        csi_MeasConfig->csi_ReportConfigToAddModList->list.array[0]->groupBasedBeamReporting.present ) {*/
      //extracting the ssb indexes
      for (ssb_idx = 0; ssb_idx < nr_ssbri_cri; ssb_idx++) {
        ssb_index[idx * nb_of_csi_ssb_report + ssb_idx] = sched_ctrl->CSI_report[idx].choice.ssb_cri_report.CRI_SSBRI[ssb_idx];
      }

      //if strongest measured RSRP is configured
      strongest_ssb_rsrp = get_measured_rsrp(sched_ctrl->CSI_report[idx].choice.ssb_cri_report.RSRP);
      ssb_rsrp[idx * nb_of_csi_ssb_report] = strongest_ssb_rsrp;
		  LOG_I(MAC,"ssb_rsrp = %d\n",strongest_ssb_rsrp);

      //if current ssb rsrp is greater than better rsrp
      if(ssb_rsrp[idx * nb_of_csi_ssb_report] > better_rsrp_reported) {
        better_rsrp_reported = ssb_rsrp[idx * nb_of_csi_ssb_report];
        target_ssb_beam_index = idx * nb_of_csi_ssb_report;
      }

      for(diff_rsrp_idx =1; diff_rsrp_idx < nr_ssbri_cri; diff_rsrp_idx++) {
        ssb_rsrp[idx * nb_of_csi_ssb_report + diff_rsrp_idx] = get_diff_rsrp(sched_ctrl->CSI_report[idx].choice.ssb_cri_report.diff_RSRP[diff_rsrp_idx-1], strongest_ssb_rsrp);

        //if current reported rsrp is greater than better rsrp
        if(ssb_rsrp[idx * nb_of_csi_ssb_report + diff_rsrp_idx] > better_rsrp_reported) {
          better_rsrp_reported = ssb_rsrp[idx * nb_of_csi_ssb_report + diff_rsrp_idx];
          target_ssb_beam_index = idx * nb_of_csi_ssb_report + diff_rsrp_idx;
        }
      }
#if 0
      //}
    //if group based beam reporting is enabled
    else if (NR_CSI_ReportConfig__groupBasedBeamReporting_PR_disabled !=
             csi_MeasConfig->csi_ReportConfigToAddModList->list.array[0]->groupBasedBeamReporting.present ) {
      //extracting the ssb indexes
      //for group based reporting only 2 SSB RS are reported, 38.331
      for (ssb_idx = 0; ssb_idx < 2; ssb_idx++) {
        ssb_index[idx * nb_of_csi_ssb_report + ssb_idx] = sched_ctrl->CSI_report[UE_id][idx].choice.ssb_cri_report.CRI_SSBRI[ssb_idx];
      }

      strongest_ssb_rsrp = get_measured_rsrp(sched_ctrl->CSI_report[UE_id][idx].choice.ssb_cri_report.RSRP);
      ssb_rsrp[idx * nb_of_csi_ssb_report] = strongest_ssb_rsrp;

      if(ssb_rsrp[idx * nb_of_csi_ssb_report] > better_rsrp_reported) {
        better_rsrp_reported = ssb_rsrp[idx * nb_of_csi_ssb_report];
        target_ssb_beam_index = idx * nb_of_csi_ssb_report;
      }

      ssb_rsrp[idx * nb_of_csi_ssb_report + 1] = get_diff_rsrp(sched_ctrl->CSI_report[UE_id][idx].choice.ssb_cri_report.diff_RSRP[diff_rsrp_idx], strongest_ssb_rsrp);

      if(ssb_rsrp[idx * nb_of_csi_ssb_report + 1] > better_rsrp_reported) {
        better_rsrp_reported = ssb_rsrp[idx * nb_of_csi_ssb_report + 1];
        target_ssb_beam_index = idx * nb_of_csi_ssb_report + 1;
      }
    }
#endif
  }


  if(ssb_index[target_ssb_beam_index] != ssb_index[curr_ssb_beam_index] && ssb_rsrp[target_ssb_beam_index] > ssb_rsrp[curr_ssb_beam_index]) {
    if( ssb_rsrp[target_ssb_beam_index] - ssb_rsrp[curr_ssb_beam_index] > L1_RSRP_HYSTERIS) {
      is_triggering_ssb_beam_switch = 1;
      LOG_I(MAC, "Triggering ssb beam switching using tci\n");
    }
  }

  if(is_triggering_ssb_beam_switch) {
    //filling pdcch tci state activativation mac ce structure fields
    sched_ctrl->UE_mac_ce_ctrl.pdcch_state_ind.is_scheduled = 1;
    //OAI currently focusing on Non CA usecase hence 0 is considered as serving
    //cell id
    sched_ctrl->UE_mac_ce_ctrl.pdcch_state_ind.servingCellId = 0; //0 for PCell as 38.331 v15.9.0 page 353 //serving cell id for which this MAC CE applies
    sched_ctrl->UE_mac_ce_ctrl.pdcch_state_ind.coresetId = 0; //coreset id for which the TCI State id is being indicated

    /* 38.321 v15.8.0 page 66
    TCI State ID: This field indicates the TCI state identified by TCI-StateId as specified in TS 38.331 [5] applicable
    to the Control Resource Set identified by CORESET ID field.
    If the field of CORESET ID is set to 0,
      this field indicates a TCI-StateId for a TCI state of the first 64 TCI-states configured by tci-States-ToAddModList and tciStates-ToReleaseList in the PDSCH-Config in the active BWP.
    If the field of CORESET ID is set to the other value than 0,
     this field indicates a TCI-StateId configured by tci-StatesPDCCH-ToAddList and tciStatesPDCCH-ToReleaseList in the controlResourceSet identified by the indicated CORESET ID.
    The length of the field is 7 bits
     */
    if(sched_ctrl->UE_mac_ce_ctrl.pdcch_state_ind.coresetId == 0) {
      int tci_state_id = checkTargetSSBInFirst64TCIStates_pdschConfig(ssb_index[target_ssb_beam_index], Mod_idP, UE_id);

      if( tci_state_id != -1)
        sched_ctrl->UE_mac_ce_ctrl.pdcch_state_ind.tciStateId = tci_state_id;
      else {
        //identify the best beam within first 64 TCI States of PDSCH
        //Config TCI-states-to-addModList
        int flag = 0;

        for(i =0; ssb_index_sorted[i]!=0; i++) {
          tci_state_id = checkTargetSSBInFirst64TCIStates_pdschConfig(ssb_index_sorted[i], Mod_idP, UE_id) ;

          if(tci_state_id != -1 && ssb_rsrp_sorted[i] > ssb_rsrp[curr_ssb_beam_index] && ssb_rsrp_sorted[i] - ssb_rsrp[curr_ssb_beam_index] > L1_RSRP_HYSTERIS) {
            sched_ctrl->UE_mac_ce_ctrl.pdcch_state_ind.tciStateId = tci_state_id;
            flag = 1;
            break;
          }
        }

        if(flag == 0 || ssb_rsrp_sorted[i] < ssb_rsrp[curr_ssb_beam_index] || ssb_rsrp_sorted[i] - ssb_rsrp[curr_ssb_beam_index] < L1_RSRP_HYSTERIS) {
          sched_ctrl->UE_mac_ce_ctrl.pdcch_state_ind.is_scheduled = 0;
        }
      }
    } else {
      int tci_state_id = checkTargetSSBInTCIStates_pdcchConfig(ssb_index[target_ssb_beam_index], Mod_idP, UE_id);

      if (tci_state_id !=-1)
        sched_ctrl->UE_mac_ce_ctrl.pdcch_state_ind.tciStateId = tci_state_id;
      else {
        //identify the best beam within CORESET/PDCCH
        ////Config TCI-states-to-addModList
        int flag = 0;

        for(i =0; ssb_index_sorted[i]!=0; i++) {
          tci_state_id = checkTargetSSBInTCIStates_pdcchConfig(ssb_index_sorted[i], Mod_idP, UE_id);

          if( tci_state_id != -1 && ssb_rsrp_sorted[i] > ssb_rsrp[curr_ssb_beam_index] && ssb_rsrp_sorted[i] - ssb_rsrp[curr_ssb_beam_index] > L1_RSRP_HYSTERIS) {
            sched_ctrl->UE_mac_ce_ctrl.pdcch_state_ind.tciStateId = tci_state_id;
            flag = 1;
            break;
          }
        }

        if(flag == 0 || ssb_rsrp_sorted[i] < ssb_rsrp[curr_ssb_beam_index] || ssb_rsrp_sorted[i] - ssb_rsrp[curr_ssb_beam_index] < L1_RSRP_HYSTERIS) {
          sched_ctrl->UE_mac_ce_ctrl.pdcch_state_ind.is_scheduled = 0;
        }
      }
    }

    sched_ctrl->UE_mac_ce_ctrl.pdcch_state_ind.tci_present_inDCI = bwp->bwp_Dedicated->pdcch_Config->choice.setup->controlResourceSetToAddModList->list.array[bwp_id-1]->tci_PresentInDCI;

    //filling pdsch tci state activation deactivation mac ce structure fields
    if(sched_ctrl->UE_mac_ce_ctrl.pdcch_state_ind.tci_present_inDCI) {
      sched_ctrl->UE_mac_ce_ctrl.pdsch_TCI_States_ActDeact.is_scheduled = 1;
      /*
      Serving Cell ID: This field indicates the identity of the Serving Cell for which the MAC CE applies
      Considering only PCell exists. Serving cell index of PCell is always 0, hence configuring 0
      */
      sched_ctrl->UE_mac_ce_ctrl.pdsch_TCI_States_ActDeact.servingCellId = 0;
      /*
      BWP ID: This field indicates a DL BWP for which the MAC CE applies as the codepoint of the DCI bandwidth
      part indicator field as specified in TS 38.212
      */
      sched_ctrl->UE_mac_ce_ctrl.pdsch_TCI_States_ActDeact.bwpId = pdsch_bwp_id;

      /*
       * TODO ssb_rsrp_sort() API yet to code to find 8 best beams, rrc configuration
       * is required
       */
      for(i = 0; i<8; i++) {
        sched_ctrl->UE_mac_ce_ctrl.pdsch_TCI_States_ActDeact.tciStateActDeact[i] = i;
      }

      sched_ctrl->UE_mac_ce_ctrl.pdsch_TCI_States_ActDeact.highestTciStateActivated = 8;

      for(i = 0, j =0; i<MAX_TCI_STATES; i++) {
        if(sched_ctrl->UE_mac_ce_ctrl.pdsch_TCI_States_ActDeact.tciStateActDeact[i]) {
          sched_ctrl->UE_mac_ce_ctrl.pdsch_TCI_States_ActDeact.codepoint[j] = i;
          j++;
        }
      }
    }//tci_presentInDCI
  }//is-triggering_beam_switch
}//tci handling

void clear_nr_nfapi_information(gNB_MAC_INST *gNB,
                                int CC_idP,
                                frame_t frameP,
                                sub_frame_t slotP){
  NR_ServingCellConfigCommon_t *scc = gNB->common_channels->ServingCellConfigCommon;
  const int num_slots = nr_slots_per_frame[*scc->ssbSubcarrierSpacing];

  nfapi_nr_dl_tti_request_t    *DL_req = &gNB->DL_req[0];
  nfapi_nr_dl_tti_pdcch_pdu_rel15_t ***pdcch = (nfapi_nr_dl_tti_pdcch_pdu_rel15_t ***)gNB->pdcch_pdu_idx[CC_idP];
  nfapi_nr_ul_tti_request_t    *future_ul_tti_req =
      &gNB->UL_tti_req_ahead[CC_idP][(slotP + num_slots - 1) % num_slots];
  nfapi_nr_ul_dci_request_t    *UL_dci_req = &gNB->UL_dci_req[0];
  nfapi_nr_tx_data_request_t   *TX_req = &gNB->TX_req[0];
  gNB->pdu_index[CC_idP] = 0;

  if (NFAPI_MODE == NFAPI_MONOLITHIC || NFAPI_MODE == NFAPI_MODE_PNF) { // monolithic or PNF

    DL_req[CC_idP].SFN                                   = frameP;
    DL_req[CC_idP].Slot                                  = slotP;
    DL_req[CC_idP].dl_tti_request_body.nPDUs             = 0;
    DL_req[CC_idP].dl_tti_request_body.nGroup            = 0;
    //DL_req[CC_idP].dl_tti_request_body.transmission_power_pcfich           = 6000;
    memset(pdcch, 0, sizeof(**pdcch) * MAX_NUM_BWP * MAX_NUM_CORESET);

    UL_dci_req[CC_idP].SFN                         = frameP;
    UL_dci_req[CC_idP].Slot                        = slotP;
    UL_dci_req[CC_idP].numPdus                     = 0;

    /* advance last round's future UL_tti_req to be ahead of current frame/slot */
    future_ul_tti_req->SFN = (slotP == 0 ? frameP : frameP + 1) % 1024;
    /* future_ul_tti_req->Slot is fixed! */
    future_ul_tti_req->n_pdus = 0;
    future_ul_tti_req->n_ulsch = 0;
    future_ul_tti_req->n_ulcch = 0;
    future_ul_tti_req->n_group = 0;

    /* UL_tti_req is a simple pointer into the current UL_tti_req_ahead, i.e.,
     * it walks over UL_tti_req_ahead in a circular fashion */
    gNB->UL_tti_req[CC_idP] = &gNB->UL_tti_req_ahead[CC_idP][slotP];

    TX_req[CC_idP].Number_of_PDUs                  = 0;
  }
}
/*
void check_nr_ul_failure(module_id_t module_idP,
                         int CC_id,
                         int UE_id,
                         frame_t frameP,
                         sub_frame_t slotP) {

  NR_UE_info_t                 *UE_info  = &RC.nrmac[module_idP]->UE_info;
  nfapi_nr_dl_dci_request_t  *DL_req   = &RC.nrmac[module_idP]->DL_req[0];
  uint16_t                      rnti      = UE_RNTI(module_idP, UE_id);
  NR_COMMON_channels_t          *cc       = RC.nrmac[module_idP]->common_channels;

  // check uplink failure
  if ((UE_info->UE_sched_ctrl[UE_id].ul_failure_timer > 0) &&
      (UE_info->UE_sched_ctrl[UE_id].ul_out_of_sync == 0)) {
    LOG_I(MAC, "UE %d rnti %x: UL Failure timer %d \n", UE_id, rnti,
    UE_info->UE_sched_ctrl[UE_id].ul_failure_timer);
    if (UE_info->UE_sched_ctrl[UE_id].ra_pdcch_order_sent == 0) {
      UE_info->UE_sched_ctrl[UE_id].ra_pdcch_order_sent = 1;

      // add a format 1A dci for this UE to request an RA procedure (only one UE per subframe)
      nfapi_nr_dl_dci_request_pdu_t *dl_config_pdu                    = &DL_req[CC_id].dl_tti_request_body.dl_config_pdu_list[DL_req[CC_id].dl_tti_request_body.number_pdu];
      memset((void *) dl_config_pdu, 0,sizeof(nfapi_dl_dci_request_pdu_t));
      dl_config_pdu->pdu_type                                         = NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE;
      dl_config_pdu->pdu_size                                         = (uint8_t) (2 + sizeof(nfapi_dl_config_dci_dl_pdu));
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tl.tag                = NFAPI_DL_DCI_REQUEST_DCI_DL_PDU_REL8_TAG;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format            = NFAPI_DL_DCI_FORMAT_1A;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level     = get_aggregation(get_bw_index(module_idP, CC_id),
                      UE_info->UE_sched_ctrl[UE_id].
                      dl_cqi[CC_id], format1A);
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti                  = rnti;
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type             = 1;  // CRNTI : see Table 4-10 from SCF082 - nFAPI specifications
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.transmission_power    = 6000; // equal to RS power

      AssertFatal((cc[CC_id].mib->message.dl_Bandwidth >= 0) && (cc[CC_id].mib->message.dl_Bandwidth < 6),
      "illegal dl_Bandwidth %d\n",
      (int) cc[CC_id].mib->message.dl_Bandwidth);
      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding = nr_pdcch_order_table[cc[CC_id].mib->message.dl_Bandwidth];
      DL_req[CC_id].dl_tti_request_body.number_dci++;
      DL_req[CC_id].dl_tti_request_body.number_pdu++;
      DL_req[CC_id].dl_tti_request_body.tl.tag                      = NFAPI_DL_TTI_REQUEST_BODY_TAG;
      LOG_I(MAC,
      "UE %d rnti %x: sending PDCCH order for RAPROC (failure timer %d), resource_block_coding %d \n",
      UE_id, rnti,
      UE_info->UE_sched_ctrl[UE_id].ul_failure_timer,
      dl_config_pdu->dci_dl_pdu.
      dci_dl_pdu_rel8.resource_block_coding);
    } else {    // ra_pdcch_sent==1
      LOG_I(MAC,
      "UE %d rnti %x: sent PDCCH order for RAPROC waiting (failure timer %d) \n",
      UE_id, rnti,
      UE_info->UE_sched_ctrl[UE_id].ul_failure_timer);
      if ((UE_info->UE_sched_ctrl[UE_id].ul_failure_timer % 40) == 0) UE_info->UE_sched_ctrl[UE_id].ra_pdcch_order_sent = 0;  // resend every 4 frames
    }

    UE_info->UE_sched_ctrl[UE_id].ul_failure_timer++;
    // check threshold
    if (UE_info->UE_sched_ctrl[UE_id].ul_failure_timer > 20000) {
      // inform RRC of failure and clear timer
      LOG_I(MAC,
      "UE %d rnti %x: UL Failure after repeated PDCCH orders: Triggering RRC \n",
      UE_id, rnti);
      mac_eNB_rrc_ul_failure(module_idP, CC_id, frameP, subframeP,rnti);
      UE_info->UE_sched_ctrl[UE_id].ul_failure_timer = 0;
      UE_info->UE_sched_ctrl[UE_id].ul_out_of_sync   = 1;

      //Inform the controller about the UE deactivation. Should be moved to RRC agent in the future
      if (rrc_agent_registered[module_idP]) {
        LOG_W(MAC, "notify flexran Agent of UE state change\n");
        agent_rrc_xface[module_idP]->flexran_agent_notify_ue_state_change(module_idP,
            rnti, PROTOCOL__FLEX_UE_STATE_CHANGE_TYPE__FLUESC_DEACTIVATED);
      }
    }
  }       // ul_failure_timer>0

}
*/
/*
void schedule_nr_SRS(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP)
{
  gNB_MAC_INST *gNB = RC.nrmac[module_idP];
  NR_UE_info_t *UE_info = &gNB->UE_info;
  nfapi_ul_config_request_body_t *ul_req;
  int CC_id, UE_id;
  NR_COMMON_channels_t *cc = RC.nrmac[module_idP]->common_channels;
  SoundingRS_UL_ConfigCommon_t *soundingRS_UL_ConfigCommon;
  struct SoundingRS_UL_ConfigDedicated *soundingRS_UL_ConfigDedicated;
  uint8_t TSFC;
  uint16_t deltaTSFC;   // bitmap
  uint8_t srs_SubframeConfig;

  // table for TSFC (Period) and deltaSFC (offset)
  const uint16_t deltaTSFCTabType1[15][2] = { {1, 1}, {1, 2}, {2, 2}, {1, 5}, {2, 5}, {4, 5}, {8, 5}, {3, 5}, {12, 5}, {1, 10}, {2, 10}, {4, 10}, {8, 10}, {351, 10}, {383, 10} };  // Table 5.5.3.3-2 3GPP 36.211 FDD
  const uint16_t deltaTSFCTabType2[14][2] = { {2, 5}, {6, 5}, {10, 5}, {18, 5}, {14, 5}, {22, 5}, {26, 5}, {30, 5}, {70, 10}, {74, 10}, {194, 10}, {326, 10}, {586, 10}, {210, 10} }; // Table 5.5.3.3-2 3GPP 36.211 TDD

  uint16_t srsPeriodicity, srsOffset;

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    soundingRS_UL_ConfigCommon = &cc[CC_id].radioResourceConfigCommon->soundingRS_UL_ConfigCommon;
    // check if SRS is enabled in this frame/subframe
    if (soundingRS_UL_ConfigCommon) {
      srs_SubframeConfig = soundingRS_UL_ConfigCommon->choice.setup.srs_SubframeConfig;
      if (cc[CC_id].tdd_Config == NULL) { // FDD
  deltaTSFC = deltaTSFCTabType1[srs_SubframeConfig][0];
  TSFC = deltaTSFCTabType1[srs_SubframeConfig][1];
      } else {    // TDD
  deltaTSFC = deltaTSFCTabType2[srs_SubframeConfig][0];
  TSFC = deltaTSFCTabType2[srs_SubframeConfig][1];
      }
      // Sounding reference signal subframes are the subframes satisfying ns/2 mod TSFC (- deltaTSFC
      uint16_t tmp = (subframeP % TSFC);

      if ((1 << tmp) & deltaTSFC) {
  // This is an SRS subframe, loop over UEs
  for (UE_id = 0; UE_id < MAX_MOBILES_PER_GNB; UE_id++) {
    if (!RC.nrmac[module_idP]->UE_info.active[UE_id]) continue;
    ul_req = &RC.nrmac[module_idP]->UL_req[CC_id].ul_config_request_body;
    // drop the allocation if the UE hasn't send RRCConnectionSetupComplete yet
    if (mac_eNB_get_rrc_status(module_idP,UE_RNTI(module_idP, UE_id)) < RRC_CONNECTED) continue;

    AssertFatal(UE_info->UE_template[CC_id][UE_id].physicalConfigDedicated != NULL,
          "physicalConfigDedicated is null for UE %d\n",
          UE_id);

    if ((soundingRS_UL_ConfigDedicated = UE_info->UE_template[CC_id][UE_id].physicalConfigDedicated->soundingRS_UL_ConfigDedicated) != NULL) {
      if (soundingRS_UL_ConfigDedicated->present == SoundingRS_UL_ConfigDedicated_PR_setup) {
        get_srs_pos(&cc[CC_id],
        soundingRS_UL_ConfigDedicated->choice.
        setup.srs_ConfigIndex,
        &srsPeriodicity, &srsOffset);
        if (((10 * frameP + subframeP) % srsPeriodicity) == srsOffset) {
    // Program SRS
    ul_req->srs_present = 1;
    nfapi_ul_config_request_pdu_t * ul_config_pdu = &ul_req->ul_config_pdu_list[ul_req->number_of_pdus];
    memset((void *) ul_config_pdu, 0, sizeof(nfapi_ul_config_request_pdu_t));
    ul_config_pdu->pdu_type =  NFAPI_UL_CONFIG_SRS_PDU_TYPE;
    ul_config_pdu->pdu_size =  2 + (uint8_t) (2 + sizeof(nfapi_ul_config_srs_pdu));
    ul_config_pdu->srs_pdu.srs_pdu_rel8.tl.tag = NFAPI_UL_CONFIG_REQUEST_SRS_PDU_REL8_TAG;
    ul_config_pdu->srs_pdu.srs_pdu_rel8.size = (uint8_t)sizeof(nfapi_ul_config_srs_pdu);
    ul_config_pdu->srs_pdu.srs_pdu_rel8.rnti = UE_info->UE_template[CC_id][UE_id].rnti;
    ul_config_pdu->srs_pdu.srs_pdu_rel8.srs_bandwidth = soundingRS_UL_ConfigDedicated->choice.setup.srs_Bandwidth;
    ul_config_pdu->srs_pdu.srs_pdu_rel8.frequency_domain_position = soundingRS_UL_ConfigDedicated->choice.setup.freqDomainPosition;
    ul_config_pdu->srs_pdu.srs_pdu_rel8.srs_hopping_bandwidth = soundingRS_UL_ConfigDedicated->choice.setup.srs_HoppingBandwidth;;
    ul_config_pdu->srs_pdu.srs_pdu_rel8.transmission_comb = soundingRS_UL_ConfigDedicated->choice.setup.transmissionComb;
    ul_config_pdu->srs_pdu.srs_pdu_rel8.i_srs = soundingRS_UL_ConfigDedicated->choice.setup.srs_ConfigIndex;
    ul_config_pdu->srs_pdu.srs_pdu_rel8.sounding_reference_cyclic_shift = soundingRS_UL_ConfigDedicated->choice.setup.cyclicShift;    //              ul_config_pdu->srs_pdu.srs_pdu_rel10.antenna_port                   = ;//
    //              ul_config_pdu->srs_pdu.srs_pdu_rel13.number_of_combs                = ;//
    RC.nrmac[module_idP]->UL_req[CC_id].sfn_sf = (frameP << 4) + subframeP;
    RC.nrmac[module_idP]->UL_req[CC_id].header.message_id = NFAPI_UL_CONFIG_REQUEST;
    ul_req->number_of_pdus++;
        } // if (((10*frameP+subframeP) % srsPeriodicity) == srsOffset)
      } // if (soundingRS_UL_ConfigDedicated->present == SoundingRS_UL_ConfigDedicated_PR_setup)
    }   // if ((soundingRS_UL_ConfigDedicated = UE_info->UE_template[CC_id][UE_id].physicalConfigDedicated->soundingRS_UL_ConfigDedicated)!=NULL)
  }   // for (UE_id ...
      }     // if((1<<tmp) & deltaTSFC)

    }     // SRS config
  }
}
*/


bool is_xlsch_in_slot(uint64_t bitmap, sub_frame_t slot) {
  return (bitmap >> slot) & 0x01;
}


void gNB_dlsch_ulsch_scheduler(module_id_t module_idP,
                               frame_t frame,
                               sub_frame_t slot){

  protocol_ctxt_t   ctxt;
  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_idP, ENB_FLAG_YES, NOT_A_RNTI, frame, slot,module_idP);
 
  int nb_periods_per_frame;

  const int bwp_id = 1;

  gNB_MAC_INST *gNB = RC.nrmac[module_idP];
  NR_COMMON_channels_t *cc = gNB->common_channels;
  NR_ServingCellConfigCommon_t        *scc     = cc->ServingCellConfigCommon;
  NR_TDD_UL_DL_Pattern_t *tdd_pattern = &scc->tdd_UL_DL_ConfigurationCommon->pattern1;

  switch(scc->tdd_UL_DL_ConfigurationCommon->pattern1.dl_UL_TransmissionPeriodicity) {
    case 0:
      nb_periods_per_frame = 20; // 10ms/0p5ms
      break;

    case 1:
      nb_periods_per_frame = 16; // 10ms/0p625ms
      break;

    case 2:
      nb_periods_per_frame = 10; // 10ms/1ms
      break;

    case 3:
      nb_periods_per_frame = 8; // 10ms/1p25ms
      break;

    case 4:
      nb_periods_per_frame = 5; // 10ms/2ms
      break;

    case 5:
      nb_periods_per_frame = 4; // 10ms/2p5ms
      break;

    case 6:
      nb_periods_per_frame = 2; // 10ms/5ms
      break;

    case 7:
      nb_periods_per_frame = 1; // 10ms/10ms
      break;

    default:
      AssertFatal(1==0,"Undefined tdd period %ld\n", scc->tdd_UL_DL_ConfigurationCommon->pattern1.dl_UL_TransmissionPeriodicity);
  }

  int num_slots_per_tdd = (nr_slots_per_frame[*scc->ssbSubcarrierSpacing])/nb_periods_per_frame;

  const int nr_ulmix_slots = tdd_pattern->nrofUplinkSlots + (tdd_pattern->nrofUplinkSymbols!=0);

  start_meas(&RC.nrmac[module_idP]->eNB_scheduler);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_gNB_DLSCH_ULSCH_SCHEDULER,VCD_FUNCTION_IN);
  pdcp_run(&ctxt);
  /* send tick to RLC and RRC every ms */
  if ((slot & ((1 << *scc->ssbSubcarrierSpacing) - 1)) == 0) {
    void nr_rlc_tick(int frame, int subframe);
    nr_rlc_tick(frame, slot >> *scc->ssbSubcarrierSpacing);
    nr_rrc_trigger(&ctxt, 0 /*CC_id*/, frame, slot >> *scc->ssbSubcarrierSpacing);
  }

#define BIT(x) (1 << (x))
  const uint64_t dlsch_in_slot_bitmap = BIT( 1) | BIT( 2) | BIT( 3) | BIT( 4) | BIT( 5) | BIT( 6)
                                      | BIT(11) | BIT(12) | BIT(13) | BIT(14) | BIT(15) | BIT(16);
  const uint64_t ulsch_in_slot_bitmap = BIT( 8) | BIT(18);

  memset(RC.nrmac[module_idP]->cce_list[bwp_id][0],0,MAX_NUM_CCE*sizeof(int)); // coreset0
  memset(RC.nrmac[module_idP]->cce_list[bwp_id][1],0,MAX_NUM_CCE*sizeof(int)); // coresetid 1
  NR_UE_info_t *UE_info = &RC.nrmac[module_idP]->UE_info;
  for (int UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id])
    for (int i=0; i<MAX_NUM_CORESET; i++)
      UE_info->num_pdcch_cand[UE_id][i] = 0;
  for (int CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    //mbsfn_status[CC_id] = 0;

    // clear vrb_maps
    memset(cc[CC_id].vrb_map, 0, sizeof(uint16_t) * MAX_BWP_SIZE);
    // clear last scheduled slot's content (only)!
    const int num_slots = nr_slots_per_frame[*scc->ssbSubcarrierSpacing];
    const int last_slot = (slot + num_slots - 1) % num_slots;
    uint16_t *vrb_map_UL = cc[CC_id].vrb_map_UL;
    memset(&vrb_map_UL[last_slot * MAX_BWP_SIZE], 0, sizeof(uint16_t) * MAX_BWP_SIZE);

    clear_nr_nfapi_information(RC.nrmac[module_idP], CC_id, frame, slot);
  }


  if ((slot == 0) && (frame & 127) == 0) dump_mac_stats(RC.nrmac[module_idP]);

  // This schedules MIB
  schedule_nr_mib(module_idP, frame, slot, nr_slots_per_frame[*scc->ssbSubcarrierSpacing]);

  // This schedules SIB1
  if ( get_softmodem_params()->sa == 1 )
    schedule_nr_sib1(module_idP, frame, slot);


  // This schedule PRACH if we are not in phy_test mode
  if (get_softmodem_params()->phy_test == 0) {
    /* we need to make sure that resources for PRACH are free. To avoid that
       e.g. PUSCH has already been scheduled, make sure we schedule before
       anything else: below, we simply assume an advance one frame (minus one
       slot, because otherwise we would allocate the current slot in
       UL_tti_req_ahead), but be aware that, e.g., K2 is allowed to be larger
       (schedule_nr_prach will assert if resources are not free). */
    const sub_frame_t n_slots_ahead = nr_slots_per_frame[*scc->ssbSubcarrierSpacing] - 1;
    const frame_t f = (frame + (slot + n_slots_ahead) / nr_slots_per_frame[*scc->ssbSubcarrierSpacing]) % 1024;
    const sub_frame_t s = (slot + n_slots_ahead) % nr_slots_per_frame[*scc->ssbSubcarrierSpacing];
    schedule_nr_prach(module_idP, f, s);
  }

  // This schedule SR
  // TODO

  // Schedule CSI measurement reporting: check in slot 0 for the whole frame
  if (slot == 0)
    nr_csi_meas_reporting(module_idP, frame, slot);

  // This schedule RA procedure if not in phy_test mode
  // Otherwise already consider 5G already connected
  if (get_softmodem_params()->phy_test == 0) {
    nr_schedule_RA(module_idP, frame, slot);
  }

  // This schedules the DCI for Uplink and subsequently PUSCH
  {
    nr_schedule_ulsch(module_idP, frame, slot, num_slots_per_tdd, nr_ulmix_slots, ulsch_in_slot_bitmap);
  }

  // This schedules the DCI for Downlink and PDSCH
  if (is_xlsch_in_slot(dlsch_in_slot_bitmap, slot))
    nr_schedule_ue_spec(module_idP, frame, slot);


  nr_schedule_pucch(module_idP, frame, slot);

  stop_meas(&RC.nrmac[module_idP]->eNB_scheduler);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_gNB_DLSCH_ULSCH_SCHEDULER,VCD_FUNCTION_OUT);
}
