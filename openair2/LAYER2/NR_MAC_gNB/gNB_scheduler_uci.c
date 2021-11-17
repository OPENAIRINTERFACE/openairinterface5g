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

/*! \file gNB_scheduler_uci.c
 * \brief MAC procedures related to UCI
 * \date 2020
 * \version 1.0
 * \company Eurecom
 */

#include <softmodem-common.h>
#include "LAYER2/MAC/mac.h"
#include "NR_MAC_gNB/nr_mac_gNB.h"
#include "NR_MAC_COMMON/nr_mac_extern.h"
#include "NR_MAC_gNB/mac_proto.h"
#include "common/ran_context.h"
#include "nfapi/oai_integration/vendor_ext.h"

extern RAN_CONTEXT_t RC;


void nr_fill_nfapi_pucch(module_id_t mod_id,
                         frame_t frame,
                         sub_frame_t slot,
                         const NR_sched_pucch_t *pucch,
                         int UE_id)
{
  NR_UE_info_t *UE_info = &RC.nrmac[mod_id]->UE_info;

  nfapi_nr_ul_tti_request_t *future_ul_tti_req =
      &RC.nrmac[mod_id]->UL_tti_req_ahead[0][pucch->ul_slot];
  AssertFatal(future_ul_tti_req->SFN == pucch->frame
              && future_ul_tti_req->Slot == pucch->ul_slot,
              "future UL_tti_req's frame.slot %d.%d does not match PUCCH %d.%d\n",
              future_ul_tti_req->SFN,
              future_ul_tti_req->Slot,
              pucch->frame,
              pucch->ul_slot);
  future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_type = NFAPI_NR_UL_CONFIG_PUCCH_PDU_TYPE;
  future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pdu_size = sizeof(nfapi_nr_pucch_pdu_t);
  nfapi_nr_pucch_pdu_t *pucch_pdu = &future_ul_tti_req->pdus_list[future_ul_tti_req->n_pdus].pucch_pdu;
  memset(pucch_pdu, 0, sizeof(nfapi_nr_pucch_pdu_t));
  future_ul_tti_req->n_pdus += 1;

  LOG_D(NR_MAC,
        "%s %4d.%2d Scheduling pucch reception in %4d.%2d: bits SR %d, DAI %d, CSI %d on res %d\n",
        pucch->dai_c>0 ? "pucch_acknak" : "",
        frame,
        slot,
        pucch->frame,
        pucch->ul_slot,
        pucch->sr_flag,
        pucch->dai_c,
        pucch->csi_bits,
        pucch->resource_indicator);

  NR_ServingCellConfigCommon_t *scc = RC.nrmac[mod_id]->common_channels->ServingCellConfigCommon;
  NR_CellGroupConfig_t *cg=UE_info->CellGroup[UE_id];

  NR_BWP_UplinkDedicated_t *ubwpd;
  ubwpd = cg ? cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP:NULL;

  LOG_D(NR_MAC,"pucch_acknak: %d.%d Calling nr_configure_pucch (ubwpd %p,r_pucch %d) pucch in %d.%d\n",frame,slot,ubwpd,pucch->r_pucch,pucch->frame,pucch->ul_slot);
  nr_configure_pucch(pucch_pdu,
                     scc,
                     UE_info->CellGroup[UE_id],
                     UE_info->UE_sched_ctrl[UE_id].active_ubwp,
                     ubwpd,
                     UE_info->rnti[UE_id],
                     pucch->resource_indicator,
                     pucch->csi_bits,
                     pucch->dai_c,
                     pucch->sr_flag,
                     pucch->r_pucch);
}

#define MIN_RSRP_VALUE -141
#define MAX_NUM_SSB 128
#define MAX_SSB_SCHED 8
#define L1_RSRP_HYSTERIS 10 //considering 10 dBm as hysterisis for avoiding frequent SSB Beam Switching. !Fixme provide exact value if any
//#define L1_DIFF_RSRP_STEP_SIZE 2

int ssb_index_sorted[MAX_NUM_SSB] = {0};
int ssb_rsrp_sorted[MAX_NUM_SSB] = {0};

//Measured RSRP Values Table 10.1.16.1-1 from 36.133
//Stored all the upper limits[Max RSRP Value of corresponding index]
//stored -1 for invalid values
int L1_SSB_CSI_RSRP_measReport_mapping_38133_10_1_6_1_1[128] = {
  -1, -1, -1, -1, -1, -1, -1, -1, -1, -1, //0 - 9
     -1, -1, -1, -1, -1, -1, INT_MIN, -140, -139, -138, //10 - 19
    -137, -136, -135, -134, -133, -132, -131, -130, -129, -128, //20 - 29
    -127, -126, -125, -124, -123, -122, -121, -120, -119, -118, //30 - 39
    -117,-116, -115, -114, -113, -112, -111, -110, -109, -108, //40 - 49
    -107, -106, -105, -104, -103, -102, -101, -100, -99, -98, //50 - 59
    -97, -96, -95, -94, -93, -92, -91, -90, -89, -88, //60 - 69
    -87, -86, -85, -84, -83, -82, -81, -80, -79, -78, //70 - 79
    -77, -76, -75, -74, -73, -72, -71, -70, -69, -68, //80 - 89
    -67, -66, -65, -64, -63, -62, -61, -60, -59, -58, //90 - 99
    -57, -56, -55, -54, -53, -52, -51, -50, -49, -48, //100 - 109
    -47, -46, -45, -44, INT_MAX, -1, -1, -1, -1, -1, //110 - 119
    -1, -1, -1, -1, -1, -1, -1, -1//120 - 127
  };

//Differential RSRP values Table 10.1.6.1-2 from 36.133
//Stored the upper limits[MAX RSRP Value]
int diff_rsrp_ssb_csi_meas_10_1_6_1_2[16] = {
  0, -2, -4, -6, -8, -10, -12, -14, -16, -18, //0 - 9
  -20, -22, -24, -26, -28, -30 //10 - 15
};


void nr_schedule_pucch(int Mod_idP,
                       frame_t frameP,
                       sub_frame_t slotP)
{
  gNB_MAC_INST *nrmac = RC.nrmac[Mod_idP];
  if (!is_xlsch_in_slot(nrmac->ulsch_slot_bitmap[slotP / 64], slotP))
    return;

  NR_UE_info_t *UE_info = &nrmac->UE_info;
  const NR_list_t *UE_list = &UE_info->list;

  for (int UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
    NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    if (sched_ctrl->ul_failure==1 && get_softmodem_params()->phy_test==0) continue;
    const int n = sizeof(sched_ctrl->sched_pucch) / sizeof(*sched_ctrl->sched_pucch);
    for (int i = 0; i < n; i++) {
      NR_sched_pucch_t *curr_pucch = &UE_info->UE_sched_ctrl[UE_id].sched_pucch[i];
      const uint16_t O_ack = curr_pucch->dai_c;
      const uint16_t O_csi = curr_pucch->csi_bits;
      const uint8_t O_sr = curr_pucch->sr_flag;
      if (O_ack + O_csi + O_sr == 0
          || frameP != curr_pucch->frame
          || slotP != curr_pucch->ul_slot)
        continue;
      LOG_D(NR_MAC,"Scheduling PUCCH[%d] RX for UE %d in %d.%d O_ack %d\n",i,UE_id,curr_pucch->frame,curr_pucch->ul_slot,O_ack);
      nr_fill_nfapi_pucch(Mod_idP, frameP, slotP, curr_pucch, UE_id);
      memset(curr_pucch, 0, sizeof(*curr_pucch));
    }
  }
}


//! Calculating number of bits set
uint8_t number_of_bits_set (uint8_t buf,uint8_t * max_ri){
  uint8_t nb_of_bits_set = 0;
  uint8_t mask = 0xff;
  uint8_t index = 0;

  for (index=7; (buf & mask) && (index>=0)  ; index--){
    if (buf & (1<<index))
      nb_of_bits_set++;

    mask>>=1;
  }
  *max_ri = 8-index;
  return nb_of_bits_set;
}


void compute_rsrp_bitlen(struct NR_CSI_ReportConfig *csi_reportconfig,
                         uint8_t nb_resources,
                         nr_csi_report_t *csi_report){

  if (NR_CSI_ReportConfig__groupBasedBeamReporting_PR_disabled == csi_reportconfig->groupBasedBeamReporting.present) {
    if (NULL != csi_reportconfig->groupBasedBeamReporting.choice.disabled->nrofReportedRS)
      csi_report->CSI_report_bitlen.nb_ssbri_cri = *(csi_reportconfig->groupBasedBeamReporting.choice.disabled->nrofReportedRS)+1;
    else
      /*! From Spec 38.331
       * nrofReportedRS
       * The number (N) of measured RS resources to be reported per report setting in a non-group-based report. N <= N_max, where N_max is either 2 or 4 depending on UE
       * capability. FFS: The signaling mechanism for the gNB to select a subset of N beams for the UE to measure and report.
       * When the field is absent the UE applies the value 1
       */
      csi_report->CSI_report_bitlen.nb_ssbri_cri= 1;
  } else
    csi_report->CSI_report_bitlen.nb_ssbri_cri= 2;

  if (nb_resources) {
    csi_report->CSI_report_bitlen.cri_ssbri_bitlen =ceil(log2 (nb_resources));
    csi_report->CSI_report_bitlen.rsrp_bitlen = 7; //From spec 38.212 Table 6.3.1.1.2-6: CRI, SSBRI, and RSRP
    csi_report->CSI_report_bitlen.diff_rsrp_bitlen =4; //From spec 38.212 Table 6.3.1.1.2-6: CRI, SSBRI, and RSRP
  } else {
    csi_report->CSI_report_bitlen.cri_ssbri_bitlen =0;
    csi_report->CSI_report_bitlen.rsrp_bitlen = 0;
    csi_report->CSI_report_bitlen.diff_rsrp_bitlen =0;
  }
}


uint8_t compute_ri_bitlen(struct NR_CSI_ReportConfig *csi_reportconfig,
                          nr_csi_report_t *csi_report){

  struct NR_CodebookConfig *codebookConfig = csi_reportconfig->codebookConfig;
  uint8_t nb_allowed_ri, ri_restriction,ri_bitlen;
  uint8_t  max_ri = 0;

  if (codebookConfig == NULL) {
    csi_report->csi_meas_bitlen.ri_bitlen=0;
    return max_ri;
  }

  // codebook type1 single panel
  if (NR_CodebookConfig__codebookType__type1__subType_PR_typeI_SinglePanel==codebookConfig->codebookType.choice.type1->subType.present){
    struct NR_CodebookConfig__codebookType__type1__subType__typeI_SinglePanel *type1single = codebookConfig->codebookType.choice.type1->subType.choice.typeI_SinglePanel;
    if (type1single->nrOfAntennaPorts.present == NR_CodebookConfig__codebookType__type1__subType__typeI_SinglePanel__nrOfAntennaPorts_PR_two){
      // two antenna ports case
      /*  From Spec 38.212
       *  If the higher layer parameter nrofCQIsPerReport=1, nRI in Table 6.3.1.1.2-3 is the number of allowed rank indicator
       *  values in the 4 LSBs of the higher layer parameter typeI-SinglePanel-ri-Restriction according to Subclause 5.2.2.2.1 [6,
       *  TS 38.214]; otherwise nRI in Table 6.3.1.1.2-3 is the number of allowed rank indicator values according to Subclause
       *  5.2.2.2.1 [6, TS 38.214].
       *
       *  But from Current RRC ASN structures nrofCQIsPerReport is not present. Present a dummy variable is present so using it to
       *  calculate RI for antennas equal or more than two.
       * */
      AssertFatal (NULL!=csi_reportconfig->dummy, "nrofCQIsPerReport is not present");

      ri_restriction = csi_reportconfig->codebookConfig->codebookType.choice.type1->subType.choice.typeI_SinglePanel->typeI_SinglePanel_ri_Restriction.buf[0];

      /* Replace dummy with the nrofCQIsPerReport from the CSIreport
         config when equalent ASN structure present */
      if (0==*(csi_reportconfig->dummy)){
        nb_allowed_ri = number_of_bits_set((ri_restriction & 0xf0), &max_ri);
        ri_bitlen = ceil(log2(nb_allowed_ri));
      }
      else{
        nb_allowed_ri = number_of_bits_set(ri_restriction, &max_ri);
        ri_bitlen = ceil(log2(nb_allowed_ri));
      }
      ri_bitlen = ri_bitlen<1?ri_bitlen:1; //from the spec 38.212 and table  6.3.1.1.2-3: RI, LI, CQI, and CRI of codebookType=typeI-SinglePanel
      csi_report->csi_meas_bitlen.ri_bitlen=ri_bitlen;
    }
    if (type1single->nrOfAntennaPorts.present == NR_CodebookConfig__codebookType__type1__subType__typeI_SinglePanel__nrOfAntennaPorts_PR_moreThanTwo){
      if (type1single->nrOfAntennaPorts.choice.moreThanTwo->n1_n2.present ==
          NR_CodebookConfig__codebookType__type1__subType__typeI_SinglePanel__nrOfAntennaPorts__moreThanTwo__n1_n2_PR_two_one_TypeI_SinglePanel_Restriction) {
        // 4 ports
        AssertFatal (NULL!=csi_reportconfig->dummy, "nrofCQIsPerReport is not present");

        ri_restriction = csi_reportconfig->codebookConfig->codebookType.choice.type1->subType.choice.typeI_SinglePanel->typeI_SinglePanel_ri_Restriction.buf[0];

        /* Replace dummy with the nrofCQIsPerReport from the CSIreport
           config when equalent ASN structure present*/
        if (0==*(csi_reportconfig->dummy)){
          nb_allowed_ri = number_of_bits_set((ri_restriction & 0xf0), &max_ri);
          ri_bitlen = ceil(log2(nb_allowed_ri));
        }
        else{
          nb_allowed_ri = number_of_bits_set(ri_restriction,&max_ri);
          ri_bitlen = ceil(log2(nb_allowed_ri));
        }
        ri_bitlen = ri_bitlen<2?ri_bitlen:2; //from the spec 38.212 and table  6.3.1.1.2-3: RI, LI, CQI, and CRI of codebookType=typeI-SinglePanel
        csi_report->csi_meas_bitlen.ri_bitlen=ri_bitlen;
      }
      else {
        // more than 4 ports
        AssertFatal (NULL!=csi_reportconfig->dummy, "nrofCQIsPerReport is not present");

        ri_restriction = csi_reportconfig->codebookConfig->codebookType.choice.type1->subType.choice.typeI_SinglePanel->typeI_SinglePanel_ri_Restriction.buf[0];

        /* Replace dummy with the nrofCQIsPerReport from the CSIreport
           config when equalent ASN structure present */
        if (0==*(csi_reportconfig->dummy)){
          nb_allowed_ri = number_of_bits_set((ri_restriction & 0xf0),&max_ri);
          ri_bitlen = ceil(log2(nb_allowed_ri));
        }
        else{
          nb_allowed_ri = number_of_bits_set(ri_restriction, &max_ri);
          ri_bitlen = ceil(log2(nb_allowed_ri));
        }
        csi_report->csi_meas_bitlen.ri_bitlen=ri_bitlen;
      }
    }
    return max_ri;
  }
  else
    AssertFatal(1==0,"Other configurations not yet implemented\n");
  return -1;
}

void compute_li_bitlen(struct NR_CSI_ReportConfig *csi_reportconfig,
                       uint8_t max_ri,
                       nr_csi_report_t *csi_report){

  struct NR_CodebookConfig *codebookConfig = csi_reportconfig->codebookConfig;
  if (codebookConfig == NULL) {
    csi_report->csi_meas_bitlen.li_bitlen=0;
    return;
  }
  // codebook type1 single panel
  if (NR_CodebookConfig__codebookType__type1__subType_PR_typeI_SinglePanel==codebookConfig->codebookType.choice.type1->subType.present){
    /* From Spec 38.212
     *  If the higher layer parameter nrofCQIsPerReport=1, nRI in Table 6.3.1.1.2-3 is the number of allowed rank indicator
     *  values in the 4 LSBs of the higher layer parameter typeI-SinglePanel-ri-Restriction according to Subclause 5.2.2.2.1 [6,
     *  TS 38.214]; otherwise nRI in Table 6.3.1.1.2-3 is the number of allowed rank indicator values according to Subclause
     *  5.2.2.2.1 [6, TS 38.214].
     *
     *  But from Current RRC ASN structures nrofCQIsPerReport is not present. Present a dummy variable is present so using it to
     *  calculate RI for antennas equal or more than two.
     */
     //! TODO: The bit length of LI is as follows LI = log2(RI), Need to confirm wheather we should consider maximum RI can be reported from ri_restricted
     //        or we should consider reported RI. If we need to consider reported RI for calculating LI bit length then we need to modify the code.
     csi_report->csi_meas_bitlen.li_bitlen=ceil(log2(max_ri))<2?ceil(log2(max_ri)):2;
  }
  else
    AssertFatal(1==0,"Other configurations not yet implemented\n");
}


void compute_cqi_bitlen(struct NR_CSI_ReportConfig *csi_reportconfig,
                        uint8_t max_ri,
                        nr_csi_report_t *csi_report){

  struct NR_CodebookConfig *codebookConfig = csi_reportconfig->codebookConfig;
  struct NR_CSI_ReportConfig__reportFreqConfiguration *freq_config = csi_reportconfig->reportFreqConfiguration;

  if (*freq_config->cqi_FormatIndicator == NR_CSI_ReportConfig__reportFreqConfiguration__cqi_FormatIndicator_widebandCQI) {
    csi_report->csi_meas_bitlen.cqi_bitlen = 4;
    if(codebookConfig != NULL) {
      if (NR_CodebookConfig__codebookType__type1__subType_PR_typeI_SinglePanel == codebookConfig->codebookType.choice.type1->subType.present){
        struct NR_CodebookConfig__codebookType__type1__subType__typeI_SinglePanel *type1single = codebookConfig->codebookType.choice.type1->subType.choice.typeI_SinglePanel;
        if (type1single->nrOfAntennaPorts.present == NR_CodebookConfig__codebookType__type1__subType__typeI_SinglePanel__nrOfAntennaPorts_PR_moreThanTwo) {
          if (type1single->nrOfAntennaPorts.choice.moreThanTwo->n1_n2.present >
              NR_CodebookConfig__codebookType__type1__subType__typeI_SinglePanel__nrOfAntennaPorts__moreThanTwo__n1_n2_PR_two_one_TypeI_SinglePanel_Restriction) {
            // more than 4 antenna ports
            if (max_ri > 4)
              csi_report->csi_meas_bitlen.cqi_bitlen += 4; // CQI for second TB
          }
        }
      }
    }
  }
  else
    AssertFatal(1==0,"Sub-band CQI reporting not yet supported");
}

//!TODO : same function can be written to handle csi_resources
void compute_csi_bitlen(NR_CSI_MeasConfig_t *csi_MeasConfig, NR_UE_info_t *UE_info, int UE_id, module_id_t Mod_idP){
  uint8_t csi_report_id = 0;
  uint8_t nb_resources = 0;
  uint8_t max_ri = 0;
  NR_CSI_ReportConfig__reportQuantity_PR reportQuantity_type;
  NR_CSI_ResourceConfigId_t csi_ResourceConfigId;
  struct NR_CSI_ResourceConfig *csi_resourceconfig;

  // for each CSI measurement report configuration (list of CSI-ReportConfig)
  for (csi_report_id=0; csi_report_id < csi_MeasConfig->csi_ReportConfigToAddModList->list.count; csi_report_id++){
    struct NR_CSI_ReportConfig *csi_reportconfig = csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id];
    // MAC structure for CSI measurement reports (per UE and per report)
    nr_csi_report_t *csi_report = &UE_info->csi_report_template[UE_id][csi_report_id];
    // csi-ResourceConfigId of a CSI-ResourceConfig included in the configuration
    // (either CSI-RS or SSB)
    csi_ResourceConfigId = csi_reportconfig->resourcesForChannelMeasurement;
    // looking for CSI-ResourceConfig
    int found_resource = 0;
    int csi_resourceidx = 0;
    while (found_resource == 0 && csi_resourceidx < csi_MeasConfig->csi_ResourceConfigToAddModList->list.count) {
      csi_resourceconfig = csi_MeasConfig->csi_ResourceConfigToAddModList->list.array[csi_resourceidx];
      if ( csi_resourceconfig->csi_ResourceConfigId == csi_ResourceConfigId)
        found_resource = 1;
      csi_resourceidx++;
    }
    AssertFatal(found_resource==1,"Not able to found any CSI-ResourceConfig with csi-ResourceConfigId %ld\n",
                csi_ResourceConfigId);

    long resourceType = csi_resourceconfig->resourceType;

    reportQuantity_type = csi_reportconfig->reportQuantity.present;
    csi_report->reportQuantity_type = reportQuantity_type;

    // setting the CSI or SSB index list
    if (NR_CSI_ReportConfig__reportQuantity_PR_ssb_Index_RSRP == csi_report->reportQuantity_type) {
      for (int csi_idx = 0; csi_idx < csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.count; csi_idx++) {
        if (csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.array[csi_idx]->csi_SSB_ResourceSetId ==
            *(csi_resourceconfig->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->csi_SSB_ResourceSetList->list.array[0])){
          //We can configure only one SSB resource set from spec 38.331 IE CSI-ResourceConfig
          nb_resources=  csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.array[csi_idx]->csi_SSB_ResourceList.list.count;
          csi_report->SSB_Index_list = csi_MeasConfig->csi_SSB_ResourceSetToAddModList->list.array[csi_idx]->csi_SSB_ResourceList.list.array;
          csi_report->CSI_Index_list = NULL;
          break;
        }
      }
    }
    else {
      if (resourceType == NR_CSI_ResourceConfig__resourceType_periodic) {
        AssertFatal(csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList != NULL,
                    "Wrong settings! Report quantity requires CSI-RS but csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList is NULL\n");
        for (int csi_idx = 0; csi_idx < csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList->list.count; csi_idx++) {
          if (csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList->list.array[csi_idx]->nzp_CSI_ResourceSetId ==
              *(csi_resourceconfig->csi_RS_ResourceSetList.choice.nzp_CSI_RS_SSB->nzp_CSI_RS_ResourceSetList->list.array[0])) {
            //For periodic and semi-persistent CSI Resource Settings, the number of CSI-RS Resource Sets configured is limited to S=1 for spec 38.212
            nb_resources = csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList->list.array[csi_idx]->nzp_CSI_RS_Resources.list.count;
            csi_report->CSI_Index_list = csi_MeasConfig->nzp_CSI_RS_ResourceSetToAddModList->list.array[csi_idx]->nzp_CSI_RS_Resources.list.array;
            csi_report->SSB_Index_list = NULL;
            break;
          }
        }
      }
      else AssertFatal(1==0,"Only periodic resource configuration currently supported\n");
    }

    // computation of bit length depending on the report type
    switch(reportQuantity_type){
      case (NR_CSI_ReportConfig__reportQuantity_PR_ssb_Index_RSRP):
        compute_rsrp_bitlen(csi_reportconfig, nb_resources, csi_report);
        break;
      case (NR_CSI_ReportConfig__reportQuantity_PR_cri_RSRP):
        compute_rsrp_bitlen(csi_reportconfig, nb_resources, csi_report);
        break;
      case (NR_CSI_ReportConfig__reportQuantity_PR_cri_RI_CQI):
        csi_report->csi_meas_bitlen.cri_bitlen=ceil(log2(nb_resources));
        max_ri = compute_ri_bitlen(csi_reportconfig, csi_report);
        compute_cqi_bitlen(csi_reportconfig, max_ri, csi_report);
        break;
    default:
      AssertFatal(1==0,"Not yet supported CSI report quantity type");
    }
  }
}


uint16_t nr_get_csi_bitlen(int Mod_idP,
                           int UE_id,
                           uint8_t csi_report_id) {

  uint16_t csi_bitlen =0;
  NR_UE_info_t *UE_info = &RC.nrmac[Mod_idP]->UE_info;
  L1_RSRP_bitlen_t * CSI_report_bitlen = NULL;
  CSI_Meas_bitlen_t * csi_meas_bitlen = NULL;

  if (NR_CSI_ReportConfig__reportQuantity_PR_ssb_Index_RSRP==UE_info->csi_report_template[UE_id][csi_report_id].reportQuantity_type||
      NR_CSI_ReportConfig__reportQuantity_PR_cri_RSRP==UE_info->csi_report_template[UE_id][csi_report_id].reportQuantity_type){
    CSI_report_bitlen = &(UE_info->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen); //This might need to be moodif for Aperiodic CSI-RS measurements
    csi_bitlen+= ((CSI_report_bitlen->cri_ssbri_bitlen * CSI_report_bitlen->nb_ssbri_cri) +
                  CSI_report_bitlen->rsrp_bitlen +(CSI_report_bitlen->diff_rsrp_bitlen *
                  (CSI_report_bitlen->nb_ssbri_cri -1 )));
  } else{
   csi_meas_bitlen = &(UE_info->csi_report_template[UE_id][csi_report_id].csi_meas_bitlen); //This might need to be moodif for Aperiodic CSI-RS measurements
   csi_bitlen+= (csi_meas_bitlen->cri_bitlen +csi_meas_bitlen->ri_bitlen+csi_meas_bitlen->li_bitlen+csi_meas_bitlen->cqi_bitlen+csi_meas_bitlen->pmi_x1_bitlen+csi_meas_bitlen->pmi_x2_bitlen);
 }

  return csi_bitlen;
}


void nr_csi_meas_reporting(int Mod_idP,
                           frame_t frame,
                           sub_frame_t slot) {

  NR_ServingCellConfigCommon_t *scc = RC.nrmac[Mod_idP]->common_channels->ServingCellConfigCommon;
  const int n_slots_frame = nr_slots_per_frame[*scc->ssbSubcarrierSpacing];

  NR_UE_info_t *UE_info = &RC.nrmac[Mod_idP]->UE_info;
  NR_list_t *UE_list = &UE_info->list;
  for (int UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
    const NR_CellGroupConfig_t *CellGroup = UE_info->CellGroup[UE_id];
    NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    if (sched_ctrl->ul_failure==1 && get_softmodem_params()->phy_test==0) continue;
    if (!CellGroup || !CellGroup->spCellConfig || !CellGroup->spCellConfig->spCellConfigDedicated ||
	      !CellGroup->spCellConfig->spCellConfigDedicated->csi_MeasConfig) continue;
    const NR_CSI_MeasConfig_t *csi_measconfig = CellGroup->spCellConfig->spCellConfigDedicated->csi_MeasConfig->choice.setup;
    AssertFatal(csi_measconfig->csi_ReportConfigToAddModList->list.count > 0,
                "NO CSI report configuration available");
    NR_PUCCH_Config_t *pucch_Config = sched_ctrl->active_ubwp->bwp_Dedicated->pucch_Config->choice.setup;

    for (int csi_report_id = 0; csi_report_id < csi_measconfig->csi_ReportConfigToAddModList->list.count; csi_report_id++){
      NR_CSI_ReportConfig_t *csirep = csi_measconfig->csi_ReportConfigToAddModList->list.array[csi_report_id];

      AssertFatal(csirep->reportConfigType.choice.periodic,
                  "Only periodic CSI reporting is implemented currently\n");
      int period, offset;
      csi_period_offset(csirep, NULL, &period, &offset);
      const int sched_slot = (period + offset) % n_slots_frame;
      // prepare to schedule csi measurement reception according to 5.2.1.4 in 38.214
      // preparation is done in first slot of tdd period
      if (frame % (period / n_slots_frame) != offset / n_slots_frame)
        continue;
      LOG_D(NR_MAC, "CSI reporting in frame %d slot %d\n", frame, sched_slot);

      const NR_PUCCH_CSI_Resource_t *pucchcsires = csirep->reportConfigType.choice.periodic->pucch_CSI_ResourceList.list.array[0];
      const NR_PUCCH_ResourceSet_t *pucchresset = pucch_Config->resourceSetToAddModList->list.array[1]; // set with formats >1
      const int n = pucchresset->resourceList.list.count;
      int res_index = 0;
      for (; res_index < n; res_index++)
        if (*pucchresset->resourceList.list.array[res_index] == pucchcsires->pucch_Resource)
          break;
      AssertFatal(res_index < n,
                  "CSI resource not found among PUCCH resources\n");

      // find free PUCCH that is in order with possibly existing PUCCH
      // schedulings (other CSI, SR)
      NR_sched_pucch_t *curr_pucch = &sched_ctrl->sched_pucch[1];
      AssertFatal(curr_pucch->csi_bits == 0
                  && !curr_pucch->sr_flag
                  && curr_pucch->dai_c == 0,
                  "PUCCH not free at index 1 for UE %04x\n",
                  UE_info->rnti[UE_id]);
      curr_pucch->r_pucch = -1;
      curr_pucch->frame = frame;
      curr_pucch->ul_slot = sched_slot;
      curr_pucch->resource_indicator = res_index;
      curr_pucch->csi_bits +=
          nr_get_csi_bitlen(Mod_idP,UE_id,csi_report_id);

      NR_BWP_t *genericParameters = sched_ctrl->active_ubwp ?
        &sched_ctrl->active_ubwp->bwp_Common->genericParameters:
        &scc->uplinkConfigCommon->initialUplinkBWP->genericParameters;
      int bwp_start = NRRIV2PRBOFFSET(genericParameters->locationAndBandwidth,MAX_BWP_SIZE);

      // going through the list of PUCCH resources to find the one indexed by resource_id
      uint16_t *vrb_map_UL = &RC.nrmac[Mod_idP]->common_channels[0].vrb_map_UL[sched_slot * MAX_BWP_SIZE];
      const int m = pucch_Config->resourceToAddModList->list.count;
      for (int j = 0; j < m; j++) {
        NR_PUCCH_Resource_t *pucchres = pucch_Config->resourceToAddModList->list.array[j];
        if (pucchres->pucch_ResourceId != *pucchresset->resourceList.list.array[res_index])
          continue;
        int start = pucchres->startingPRB;
        int len = 1;
        uint64_t mask = 0;
        switch(pucchres->format.present){
          case NR_PUCCH_Resource__format_PR_format2:
            len = pucchres->format.choice.format2->nrofPRBs;
            mask = ((1 << pucchres->format.choice.format2->nrofSymbols) - 1) << pucchres->format.choice.format2->startingSymbolIndex;
            curr_pucch->simultaneous_harqcsi = pucch_Config->format2->choice.setup->simultaneousHARQ_ACK_CSI;
            break;
          case NR_PUCCH_Resource__format_PR_format3:
            len = pucchres->format.choice.format3->nrofPRBs;
            mask = ((1 << pucchres->format.choice.format3->nrofSymbols) - 1) << pucchres->format.choice.format3->startingSymbolIndex;
            curr_pucch->simultaneous_harqcsi = pucch_Config->format3->choice.setup->simultaneousHARQ_ACK_CSI;
            break;
          case NR_PUCCH_Resource__format_PR_format4:
            mask = ((1 << pucchres->format.choice.format4->nrofSymbols) - 1) << pucchres->format.choice.format4->startingSymbolIndex;
            curr_pucch->simultaneous_harqcsi = pucch_Config->format4->choice.setup->simultaneousHARQ_ACK_CSI;
            break;
        default:
          AssertFatal(0, "Invalid PUCCH format type\n");
        }
        // verify resources are free
        for (int i = start; i < start + len; ++i) {
          vrb_map_UL[i+bwp_start] |= mask;
        }
      }
    }
  }
}

__attribute__((unused))
static void handle_dl_harq(module_id_t mod_id,
                           int UE_id,
                           int8_t harq_pid,
                           bool success)
{
  NR_UE_info_t *UE_info = &RC.nrmac[mod_id]->UE_info;
  NR_UE_harq_t *harq = &UE_info->UE_sched_ctrl[UE_id].harq_processes[harq_pid];
  harq->feedback_slot = -1;
  harq->is_waiting = false;
  if (success) {
    add_tail_nr_list(&UE_info->UE_sched_ctrl[UE_id].available_dl_harq, harq_pid);
    harq->round = 0;
    harq->ndi ^= 1;
  } else if (harq->round >= MAX_HARQ_ROUNDS - 1) {
    add_tail_nr_list(&UE_info->UE_sched_ctrl[UE_id].available_dl_harq, harq_pid);
    harq->round = 0;
    harq->ndi ^= 1;
    NR_mac_stats_t *stats = &UE_info->mac_stats[UE_id];
    stats->dlsch_errors++;
    LOG_D(NR_MAC, "retransmission error for UE %d (total %d)\n", UE_id, stats->dlsch_errors);
  } else {
    add_tail_nr_list(&UE_info->UE_sched_ctrl[UE_id].retrans_dl_harq, harq_pid);
    harq->round++;
  }
}

int checkTargetSSBInFirst64TCIStates_pdschConfig(int ssb_index_t, int Mod_idP, int UE_id) {
  NR_UE_info_t *UE_info = &RC.nrmac[Mod_idP]->UE_info;
  NR_CellGroupConfig_t *CellGroup = UE_info->CellGroup[UE_id] ;
  int nb_tci_states = CellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->pdsch_Config->choice.setup->tci_StatesToAddModList->list.count;
  NR_TCI_State_t *tci =NULL;
  int i;

  for(i=0; i<nb_tci_states && i<64; i++) {
    tci = (NR_TCI_State_t *)CellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->pdsch_Config->choice.setup->tci_StatesToAddModList->list.array[i];

    if(tci != NULL) {
      if(tci->qcl_Type1.referenceSignal.present == NR_QCL_Info__referenceSignal_PR_ssb) {
        if(tci->qcl_Type1.referenceSignal.choice.ssb == ssb_index_t)
          return tci->tci_StateId;  // returned TCI state ID
      }
      // if type2 is configured
      else if(tci->qcl_Type2 != NULL && tci->qcl_Type2->referenceSignal.present == NR_QCL_Info__referenceSignal_PR_ssb) {
        if(tci->qcl_Type2->referenceSignal.choice.ssb == ssb_index_t)
          return tci->tci_StateId; // returned TCI state ID
      } else LOG_I(NR_MAC,"SSB index is not found in first 64 TCI states of TCI_statestoAddModList[%d]", i);
    }
  }

  // tci state not identified in first 64 TCI States of PDSCH Config
  return -1;
}

int checkTargetSSBInTCIStates_pdcchConfig(int ssb_index_t, int Mod_idP, int UE_id) {
  NR_UE_info_t *UE_info = &RC.nrmac[Mod_idP]->UE_info;
  NR_CellGroupConfig_t *CellGroup = UE_info->CellGroup[UE_id] ;
  int nb_tci_states = CellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->pdsch_Config->choice.setup->tci_StatesToAddModList->list.count;
  NR_TCI_State_t *tci =NULL;
  NR_TCI_StateId_t *tci_id = NULL;
  int bwp_id = 1;
  NR_BWP_Downlink_t *bwp = CellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[bwp_id-1];
  NR_ControlResourceSet_t *coreset = bwp->bwp_Dedicated->pdcch_Config->choice.setup->controlResourceSetToAddModList->list.array[bwp_id-1];
  int i;
  int flag = 0;
  int tci_stateID = -1;

  for(i=0; i<nb_tci_states && i<128; i++) {
    tci = (NR_TCI_State_t *)CellGroup->spCellConfig->spCellConfigDedicated->initialDownlinkBWP->pdsch_Config->choice.setup->tci_StatesToAddModList->list.array[i];

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

//returns the measured RSRP value (upper limit)
int get_measured_rsrp(uint8_t index) {
  //if index is invalid returning minimum rsrp -140
  if(index <= 15 || index >= 114)
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

//identifies the target SSB Beam index
//keeps the required date for PDCCH and PDSCH TCI state activation/deactivation CE consutruction globally
//handles triggering of PDCCH and PDSCH MAC CEs
void tci_handling(module_id_t Mod_idP, int UE_id, frame_t frame, slot_t slot) {

  int strongest_ssb_rsrp = 0;
  int cqi_idx = 0;
  int curr_ssb_beam_index = 0; //ToDo: yet to know how to identify the serving ssb beam index
  uint8_t target_ssb_beam_index = curr_ssb_beam_index;
  uint8_t is_triggering_ssb_beam_switch =0;
  uint8_t ssb_idx = 0;
  int pdsch_bwp_id =0;
  int ssb_index[MAX_NUM_SSB] = {0};
  int ssb_rsrp[MAX_NUM_SSB] = {0};
  uint8_t idx = 0;
  int bwp_id  = 1;
  NR_UE_info_t *UE_info = &RC.nrmac[Mod_idP]->UE_info;
  NR_CellGroupConfig_t *CellGroup = UE_info->CellGroup[UE_id];
  NR_BWP_Downlink_t *bwp = CellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.array[bwp_id-1];
  //bwp indicator
  int n_dl_bwp = CellGroup->spCellConfig->spCellConfigDedicated->downlinkBWP_ToAddModList->list.count;
  uint8_t nr_ssbri_cri = 0;
  uint8_t nb_of_csi_ssb_report = UE_info->csi_report_template[UE_id][cqi_idx].nb_of_csi_ssb_report;
  int better_rsrp_reported = -140-(-0); /*minimum_measured_RSRP_value - minimum_differntail_RSRP_value*///considering the minimum RSRP value as better RSRP initially
  uint8_t diff_rsrp_idx = 0;
  uint8_t i, j;
  NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
  NR_mac_stats_t *stats = &UE_info->mac_stats[UE_id];

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
      //extracting the ssb indexes
      for (ssb_idx = 0; ssb_idx < nr_ssbri_cri; ssb_idx++) {
        ssb_index[idx * nb_of_csi_ssb_report + ssb_idx] = sched_ctrl->CSI_report[idx].choice.ssb_cri_report.CRI_SSBRI[ssb_idx];
      }

      //if strongest measured RSRP is configured
      strongest_ssb_rsrp = get_measured_rsrp(sched_ctrl->CSI_report[idx].choice.ssb_cri_report.RSRP);
      // including ssb rsrp in mac stats
      stats->cumul_rsrp += strongest_ssb_rsrp;
      stats->num_rsrp_meas++;
      ssb_rsrp[idx * nb_of_csi_ssb_report] = strongest_ssb_rsrp;
      LOG_D(NR_MAC,"ssb_rsrp = %d\n",strongest_ssb_rsrp);

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
  }


  if(ssb_index[target_ssb_beam_index] != ssb_index[curr_ssb_beam_index] && ssb_rsrp[target_ssb_beam_index] > ssb_rsrp[curr_ssb_beam_index]) {
    if( ssb_rsrp[target_ssb_beam_index] - ssb_rsrp[curr_ssb_beam_index] > L1_RSRP_HYSTERIS) {
      is_triggering_ssb_beam_switch = 1;
      LOG_D(NR_MAC, "Triggering ssb beam switching using tci\n");
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


uint8_t pickandreverse_bits(uint8_t *payload, uint16_t bitlen, uint8_t start_bit) {
  uint8_t rev_bits = 0;
  for (int i=0; i<bitlen; i++)
    rev_bits |= ((payload[(start_bit+i)/8]>>((start_bit+i)%8))&0x01)<<(bitlen-i-1);
  return rev_bits;
}


void evaluate_rsrp_report(NR_UE_info_t *UE_info,
                             NR_UE_sched_ctrl_t *sched_ctrl,
                             int UE_id,
                             uint8_t csi_report_id,
                             uint8_t *payload,
                             int *cumul_bits,
                             NR_CSI_ReportConfig__reportQuantity_PR reportQuantity_type){

  uint8_t cri_ssbri_bitlen = UE_info->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen.cri_ssbri_bitlen;
  uint16_t curr_payload;

  /*! As per the spec 38.212 and table:  6.3.1.1.2-12 in a single UCI sequence we can have multiple CSI_report
  * the number of CSI_report will depend on number of CSI resource sets that are configured in CSI-ResourceConfig RRC IE
  * From spec 38.331 from the IE CSI-ResourceConfig for SSB RSRP reporting we can configure only one resource set
  * From spec 38.214 section 5.2.1.2 For periodic and semi-persistent CSI Resource Settings, the number of CSI-RS Resource Sets configured is limited to S=1
  */

  /** from 38.214 sec 5.2.1.4.2
  - if the UE is configured with the higher layer parameter groupBasedBeamReporting set to 'disabled', the UE is
    not required to update measurements for more than 64 CSI-RS and/or SSB resources, and the UE shall report in
    a single report nrofReportedRS (higher layer configured) different CRI or SSBRI for each report setting

  - if the UE is configured with the higher layer parameter groupBasedBeamReporting set to 'enabled', the UE is not
    required to update measurements for more than 64 CSI-RS and/or SSB resources, and the UE shall report in a
    single reporting instance two different CRI or SSBRI for each report setting, where CSI-RS and/or SSB
    resources can be received simultaneously by the UE either with a single spatial domain receive filter, or with
    multiple simultaneous spatial domain receive filter
  */

  int idx = 0; //Since for SSB RSRP reporting in RRC can configure only one ssb resource set per one report config
  sched_ctrl->CSI_report[idx].choice.ssb_cri_report.nr_ssbri_cri = UE_info->csi_report_template[UE_id][csi_report_id].CSI_report_bitlen.nb_ssbri_cri;

  for (int csi_ssb_idx = 0; csi_ssb_idx < sched_ctrl->CSI_report[idx].choice.ssb_cri_report.nr_ssbri_cri ; csi_ssb_idx++) {
    curr_payload = pickandreverse_bits(payload, cri_ssbri_bitlen, *cumul_bits);

    if (NR_CSI_ReportConfig__reportQuantity_PR_ssb_Index_RSRP == reportQuantity_type)
      sched_ctrl->CSI_report[idx].choice.ssb_cri_report.CRI_SSBRI [csi_ssb_idx] =
        *(UE_info->csi_report_template[UE_id][csi_report_id].SSB_Index_list[cri_ssbri_bitlen>0?((curr_payload)&~(~1<<(cri_ssbri_bitlen-1))):cri_ssbri_bitlen]);
    else
      sched_ctrl->CSI_report[idx].choice.ssb_cri_report.CRI_SSBRI [csi_ssb_idx] =
        *(UE_info->csi_report_template[UE_id][csi_report_id].CSI_Index_list[cri_ssbri_bitlen>0?((curr_payload)&~(~1<<(cri_ssbri_bitlen-1))):cri_ssbri_bitlen]);

    *cumul_bits += cri_ssbri_bitlen;
    LOG_D(MAC,"SSB_index = %d\n",sched_ctrl->CSI_report[idx].choice.ssb_cri_report.CRI_SSBRI [csi_ssb_idx]);
  }

  curr_payload = pickandreverse_bits(payload, 7, *cumul_bits);
  sched_ctrl->CSI_report[idx].choice.ssb_cri_report.RSRP = curr_payload & 0x7f;
  *cumul_bits += 7;

  for (int diff_rsrp_idx =0; diff_rsrp_idx < sched_ctrl->CSI_report[idx].choice.ssb_cri_report.nr_ssbri_cri - 1; diff_rsrp_idx++ ) {
    curr_payload = pickandreverse_bits(payload, 4, *cumul_bits);
    sched_ctrl->CSI_report[idx].choice.ssb_cri_report.diff_RSRP[diff_rsrp_idx] = curr_payload & 0x0f;
    *cumul_bits += 4;
  }
  UE_info->csi_report_template[UE_id][csi_report_id].nb_of_csi_ssb_report++;
  LOG_D(MAC,"rsrp_id = %d rsrp = %d\n",
        sched_ctrl->CSI_report[idx].choice.ssb_cri_report.RSRP,
        get_measured_rsrp(sched_ctrl->CSI_report[idx].choice.ssb_cri_report.RSRP));
}


void evaluate_cri_report(uint8_t *payload,
                         uint8_t cri_bitlen,
                         int cumul_bits,
                         NR_UE_sched_ctrl_t *sched_ctrl){

  int idx = 0; // FIXME not sure about this index. Should it be the same as csi_report_id?

  uint8_t temp_cri = pickandreverse_bits(payload, cri_bitlen, cumul_bits);
  sched_ctrl->CSI_report[idx].choice.cri_ri_li_pmi_cqi_report.cri = temp_cri;
}

void evaluate_ri_report(uint8_t *payload,
                        uint8_t ri_bitlen,
                        NR_UE_sched_ctrl_t *sched_ctrl){

  AssertFatal(1==0,"Evaluation of RI report not yet implemented\n");
}


void evaluate_cqi_report(uint8_t *payload,
                         uint8_t cqi_bitlen,
                         int *cumul_bits,
                         NR_UE_sched_ctrl_t *sched_ctrl){

  //TODO sub-band CQI report not yet implemented
  int idx = 0; // FIXME not sure about this index. Should it be the same as csi_report_id?
  
  uint8_t temp_cqi = pickandreverse_bits(payload, 4, *cumul_bits);
  sched_ctrl->CSI_report[idx].choice.cri_ri_li_pmi_cqi_report.wb_cqi_1tb = temp_cqi;
  *cumul_bits += 4;
  LOG_I(MAC,"Wide-band CQI for the first TB %d\n", temp_cqi);
  if (cqi_bitlen > 4) {
    temp_cqi = pickandreverse_bits(payload, 4, *cumul_bits);
    sched_ctrl->CSI_report[idx].choice.cri_ri_li_pmi_cqi_report.wb_cqi_2tb = temp_cqi;
    LOG_D(MAC,"Wide-band CQI for the second TB %d\n", temp_cqi);
  }
}

void extract_pucch_csi_report(NR_CSI_MeasConfig_t *csi_MeasConfig,
                              const nfapi_nr_uci_pucch_pdu_format_2_3_4_t *uci_pdu,
                              frame_t frame,
                              slot_t slot,
                              int UE_id,
                              module_id_t Mod_idP) {

  /** From Table 6.3.1.1.2-3: RI, LI, CQI, and CRI of codebookType=typeI-SinglePanel */
  NR_ServingCellConfigCommon_t *scc =
      RC.nrmac[Mod_idP]->common_channels->ServingCellConfigCommon;
  const int n_slots_frame = nr_slots_per_frame[*scc->ssbSubcarrierSpacing];
  uint8_t *payload = uci_pdu->csi_part1.csi_part1_payload;
  NR_CSI_ReportConfig__reportQuantity_PR reportQuantity_type = NR_CSI_ReportConfig__reportQuantity_PR_NOTHING;
  NR_UE_info_t *UE_info = &(RC.nrmac[Mod_idP]->UE_info);
  NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
  int cumul_bits = 0;
  for (int csi_report_id = 0; csi_report_id < csi_MeasConfig->csi_ReportConfigToAddModList->list.count; csi_report_id++ ) {
    UE_info->csi_report_template[UE_id][csi_report_id].nb_of_csi_ssb_report = 0;
    uint8_t cri_bitlen = 0;
    uint8_t ri_bitlen = 0;
    uint8_t cqi_bitlen = 0;
    NR_CSI_ReportConfig_t *csirep = csi_MeasConfig->csi_ReportConfigToAddModList->list.array[csi_report_id];
    int period, offset;
    csi_period_offset(csirep, NULL, &period, &offset);
    // verify if report with current id has been scheduled for this frame and slot
    if ((n_slots_frame*frame + slot - offset)%period == 0) {
      reportQuantity_type = UE_info->csi_report_template[UE_id][csi_report_id].reportQuantity_type;
      LOG_D(MAC,"SFN/SF:%d/%d reportQuantity type = %d\n",frame,slot,reportQuantity_type);
      switch(reportQuantity_type){
        case NR_CSI_ReportConfig__reportQuantity_PR_cri_RSRP:
          evaluate_rsrp_report(UE_info,sched_ctrl,UE_id,csi_report_id,payload,&cumul_bits,reportQuantity_type);
          break;
        case NR_CSI_ReportConfig__reportQuantity_PR_ssb_Index_RSRP:
          evaluate_rsrp_report(UE_info,sched_ctrl,UE_id,csi_report_id,payload,&cumul_bits,reportQuantity_type);
          break;
        case NR_CSI_ReportConfig__reportQuantity_PR_cri_RI_CQI:
          cri_bitlen = UE_info->csi_report_template[UE_id][csi_report_id].csi_meas_bitlen.cri_bitlen;
          if(cri_bitlen)
            evaluate_cri_report(payload,cri_bitlen,cumul_bits,sched_ctrl);
          cumul_bits += cri_bitlen;
          ri_bitlen = UE_info->csi_report_template[UE_id][csi_report_id].csi_meas_bitlen.ri_bitlen;
          if(ri_bitlen)
            evaluate_ri_report(payload,ri_bitlen,sched_ctrl);
          cumul_bits += ri_bitlen;
          //TODO add zero padding bits when needed
          cqi_bitlen = UE_info->csi_report_template[UE_id][csi_report_id].csi_meas_bitlen.cqi_bitlen;
          if(cqi_bitlen)
            evaluate_cqi_report(payload,cqi_bitlen,&cumul_bits,sched_ctrl);
          break;
        default:
          AssertFatal(1==0, "Invalid or not supported CSI measurement report\n");
      }
    }
  }
}

static NR_UE_harq_t *find_harq(module_id_t mod_id, frame_t frame, sub_frame_t slot, int UE_id)
{
  /* In case of realtime problems: we can only identify a HARQ process by
   * timing. If the HARQ process's feedback_frame/feedback_slot is not the one we
   * expected, we assume that processing has been aborted and we need to
   * skip this HARQ process, which is what happens in the loop below.
   * Similarly, we might be "in advance", in which case we need to skip
   * this result. */
  NR_UE_sched_ctrl_t *sched_ctrl = &RC.nrmac[mod_id]->UE_info.UE_sched_ctrl[UE_id];
  int8_t pid = sched_ctrl->feedback_dl_harq.head;
  if (pid < 0)
    return NULL;
  NR_UE_harq_t *harq = &sched_ctrl->harq_processes[pid];
  /* old feedbacks we missed: mark for retransmission */
  while (harq->feedback_frame != frame
         || (harq->feedback_frame == frame && harq->feedback_slot < slot)) {
    LOG_W(NR_MAC,
          "expected HARQ pid %d feedback at %d.%d, but is at %d.%d instead (HARQ feedback is in the past)\n",
          pid,
          harq->feedback_frame,
          harq->feedback_slot,
          frame,
          slot);
    remove_front_nr_list(&sched_ctrl->feedback_dl_harq);
    handle_dl_harq(mod_id, UE_id, pid, 0);
    pid = sched_ctrl->feedback_dl_harq.head;
    if (pid < 0)
      return NULL;
    harq = &sched_ctrl->harq_processes[pid];
  }
  /* feedbacks that we wait for in the future: don't do anything */
  if (harq->feedback_slot > slot) {
    LOG_W(NR_MAC,
          "expected HARQ pid %d feedback at %d.%d, but is at %d.%d instead (HARQ feedback is in the future)\n",
          pid,
          harq->feedback_frame,
          harq->feedback_slot,
          frame,
          slot);
    return NULL;
  }
  return harq;
}

void handle_nr_uci_pucch_0_1(module_id_t mod_id,
                             frame_t frame,
                             sub_frame_t slot,
                             const nfapi_nr_uci_pucch_pdu_format_0_1_t *uci_01)
{
  int UE_id = find_nr_UE_id(mod_id, uci_01->rnti);
  if (UE_id < 0) {
    LOG_E(NR_MAC, "%s(): unknown RNTI %04x in PUCCH UCI\n", __func__, uci_01->rnti);
    return;
  }
  NR_UE_info_t *UE_info = &RC.nrmac[mod_id]->UE_info;
  NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];

  if (((uci_01->pduBitmap >> 1) & 0x01)) {
    // iterate over received harq bits
    for (int harq_bit = 0; harq_bit < uci_01->harq->num_harq; harq_bit++) {
      const uint8_t harq_value = uci_01->harq->harq_list[harq_bit].harq_value;
      const uint8_t harq_confidence = uci_01->harq->harq_confidence_level;
      NR_UE_harq_t *harq = find_harq(mod_id, frame, slot, UE_id);
      if (!harq) {
        LOG_E(NR_MAC, "Oh no! Could not find a harq in %s!\n", __FUNCTION__);
        break;
      }
      DevAssert(harq->is_waiting);
      const int8_t pid = sched_ctrl->feedback_dl_harq.head;
      remove_front_nr_list(&sched_ctrl->feedback_dl_harq);
      /* According to nfapi_nr_interface_scf.h, harq_value = 0 is a pass
      (The check below was for harq_value == 1 in the develop branch)
      TODO: After update in develop branch, the following condition will be updated.*/
      if (get_softmodem_params()->emulate_l1)
        handle_dl_harq(mod_id, UE_id, pid, harq_value == 0 && harq_confidence == 0);
      else
        handle_dl_harq(mod_id, UE_id, pid, harq_value == 1 && harq_confidence == 0);
      if (harq_confidence == 1)  UE_info->mac_stats[UE_id].pucch0_DTX++;
    }
  }

  // check scheduling request result, confidence_level == 0 is good
  if (uci_01->pduBitmap & 0x1 && uci_01->sr->sr_indication && uci_01->sr->sr_confidence_level == 0 && uci_01->ul_cqi >= 148) {
    // SR detected with SNR >= 10dB
    sched_ctrl->SR |= true;
    LOG_D(NR_MAC, "SR UE %04x ul_cqi %d\n", uci_01->rnti, uci_01->ul_cqi);
  }

  // tpc (power control) only if we received AckNack or positive SR. For a
  // negative SR, the UE won't have sent anything, and the SNR is not valid
  if (((uci_01->pduBitmap >> 1) & 0x1) || sched_ctrl->SR) {
    if ((uci_01->harq) && (uci_01->harq->harq_confidence_level==0)) sched_ctrl->tpc1 = nr_get_tpc(RC.nrmac[mod_id]->pucch_target_snrx10, uci_01->ul_cqi, 30);
    else                                        sched_ctrl->tpc1 = 3;
    sched_ctrl->pucch_snrx10 = uci_01->ul_cqi * 5 - 640;
  }
}

void handle_nr_uci_pucch_2_3_4(module_id_t mod_id,
                               frame_t frame,
                               sub_frame_t slot,
                               const nfapi_nr_uci_pucch_pdu_format_2_3_4_t *uci_234)
{
  int UE_id = find_nr_UE_id(mod_id, uci_234->rnti);
  if (UE_id < 0) {
    LOG_E(NR_MAC, "%s(): unknown RNTI %04x in PUCCH UCI\n", __func__, uci_234->rnti);
    return;
  }
  AssertFatal(RC.nrmac[mod_id]->UE_info.CellGroup[UE_id],"Cellgroup is null for UE %d/%x\n",UE_id,uci_234->rnti);
  AssertFatal(RC.nrmac[mod_id]->UE_info.CellGroup[UE_id]->spCellConfig, "Cellgroup->spCellConfig is null for UE %d/%x\n",UE_id,uci_234->rnti);
  AssertFatal(RC.nrmac[mod_id]->UE_info.CellGroup[UE_id]->spCellConfig->spCellConfigDedicated, "Cellgroup->spCellConfig->spCellConfigDedicated is null for UE %d/%x\n",UE_id,uci_234->rnti);
  if ( RC.nrmac[mod_id]->UE_info.CellGroup[UE_id]->spCellConfig->spCellConfigDedicated->csi_MeasConfig==NULL) return;

  NR_CSI_MeasConfig_t *csi_MeasConfig = RC.nrmac[mod_id]->UE_info.CellGroup[UE_id]->spCellConfig->spCellConfigDedicated->csi_MeasConfig->choice.setup;
  NR_UE_info_t *UE_info = &RC.nrmac[mod_id]->UE_info;
  NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];

  // tpc (power control)
  sched_ctrl->tpc1 = nr_get_tpc(RC.nrmac[mod_id]->pucch_target_snrx10,
                                uci_234->ul_cqi,
                                30);
  sched_ctrl->pucch_snrx10 = uci_234->ul_cqi * 5 - 640;

  if ((uci_234->pduBitmap >> 1) & 0x01) {
    // iterate over received harq bits
    for (int harq_bit = 0; harq_bit < uci_234->harq.harq_bit_len; harq_bit++) {
      const int acknack = ((uci_234->harq.harq_payload[harq_bit >> 3]) >> harq_bit) & 0x01;
      NR_UE_harq_t *harq = find_harq(mod_id, frame, slot, UE_id);
      if (!harq)
        break;
      DevAssert(harq->is_waiting);
      const int8_t pid = sched_ctrl->feedback_dl_harq.head;
      remove_front_nr_list(&sched_ctrl->feedback_dl_harq);
      handle_dl_harq(mod_id, UE_id, pid, uci_234->harq.harq_crc != 1 && acknack);
    }
  }
  if ((uci_234->pduBitmap >> 2) & 0x01) {
    //API to parse the csi report and store it into sched_ctrl
    extract_pucch_csi_report (csi_MeasConfig, uci_234, frame, slot, UE_id, mod_id);
    //TCI handling function
    tci_handling(mod_id, UE_id,frame, slot);
  }
  if ((uci_234->pduBitmap >> 3) & 0x01) {
    //@TODO:Handle CSI Report 2
  }
}


// this function returns an index to NR_sched_pucch structure
// currently this structure contains PUCCH0 at index 0 and PUCCH2 at index 1
// if the function returns -1 it was not possible to schedule acknack
// when current pucch is ready to be scheduled nr_fill_nfapi_pucch is called
int nr_acknack_scheduling(int mod_id,
                          int UE_id,
                          frame_t frame,
                          sub_frame_t slot,
                          int r_pucch,
                          int is_common) {

  const NR_ServingCellConfigCommon_t *scc = RC.nrmac[mod_id]->common_channels->ServingCellConfigCommon;
  const int n_slots_frame = nr_slots_per_frame[*scc->ssbSubcarrierSpacing];
  const NR_TDD_UL_DL_Pattern_t *tdd = &scc->tdd_UL_DL_ConfigurationCommon->pattern1;
  const int nr_mix_slots = tdd->nrofDownlinkSymbols != 0 || tdd->nrofUplinkSymbols != 0;
  const int nr_slots_period = tdd->nrofDownlinkSlots + tdd->nrofUplinkSlots + nr_mix_slots;
  const int first_ul_slot_tdd = tdd->nrofDownlinkSlots + nr_slots_period * (slot / nr_slots_period);
  const int first_ul_slot_period = first_ul_slot_tdd%nr_slots_period;
  const int CC_id = 0;
  NR_sched_pucch_t *csi_pucch;

  AssertFatal(slot < first_ul_slot_tdd + (tdd->nrofUplinkSymbols != 0),
              "cannot handle multiple TDD periods (yet): slot %d first_ul_slot_tdd %d nrofUplinkSlots %ld\n",
              slot,
              first_ul_slot_tdd,
              tdd->nrofUplinkSlots);

  /* for the moment, we consider:
   * * only pucch_sched[0] holds HARQ (and SR)
   * * we do not multiplex with CSI, which is always in pucch_sched[2]
   * * SR uses format 0 and is allocated in the first UL (mixed) slot (and not
   *   later)
   * * each UE has dedicated PUCCH Format 0 resources, and we use index 0! */
  NR_UE_sched_ctrl_t *sched_ctrl = &RC.nrmac[mod_id]->UE_info.UE_sched_ctrl[UE_id];
  NR_sched_pucch_t *pucch = &sched_ctrl->sched_pucch[0];
  LOG_D(NR_MAC,"pucch_acknak %d.%d Trying to allocate pucch, current DAI %d\n",frame,slot,pucch->dai_c);
  pucch->r_pucch=r_pucch;
  AssertFatal(pucch->csi_bits == 0,
              "%s(): csi_bits %d in sched_pucch[0]\n",
              __func__,
              pucch->csi_bits);
  /* if the currently allocated PUCCH of this UE is full, allocate it */
  if (pucch->dai_c == 2) {
    /* advance the UL slot information in PUCCH by one so we won't schedule in
     * the same slot again */
    const int f = pucch->frame;
    const int s = pucch->ul_slot;
    LOG_D(NR_MAC,"pucch_acknak : %d.%d DAI = 2 pucch currently in %d.%d, advancing by 1 slot\n",frame,slot,f,s);
    nr_fill_nfapi_pucch(mod_id, frame, slot, pucch, UE_id);
    memset(pucch, 0, sizeof(*pucch));
    pucch->frame = s == n_slots_frame - 1 ? (f + 1) % 1024 : f;
    if(((s + 1)%nr_slots_period) == 0)
      pucch->ul_slot = (s + 1 + first_ul_slot_period) % n_slots_frame;
    else
      pucch->ul_slot = (s + 1) % n_slots_frame;
    // we assume that only two indices over the array sched_pucch exist
    csi_pucch = &sched_ctrl->sched_pucch[1];
    // skip the CSI PUCCH if it is present and if in the next frame/slot
    // and if we don't multiplex
    csi_pucch->r_pucch=-1;
    if (csi_pucch->csi_bits > 0
        && csi_pucch->frame == pucch->frame
        && csi_pucch->ul_slot == pucch->ul_slot
        && !csi_pucch->simultaneous_harqcsi) {
      nr_fill_nfapi_pucch(mod_id, frame, slot, csi_pucch, UE_id);
      memset(csi_pucch, 0, sizeof(*csi_pucch));
      pucch->frame = s == n_slots_frame - 1 ? (f + 1) % 1024 : f;
      if(((s + 1)%nr_slots_period) == 0)
        pucch->ul_slot = (s + 1 + first_ul_slot_period) % n_slots_frame;
      else
        pucch->ul_slot = (s + 1) % n_slots_frame;
    }
  }

  LOG_D(NR_MAC,"pucch_acknak 1. DL %d.%d, UL_ACK %d.%d, DAI_C %d\n",frame,slot,pucch->frame,pucch->ul_slot,pucch->dai_c);

  // this is hardcoded for now as ue specific only if we are not on the initialBWP (to be fixed to allow ue_Specific also on initialBWP
  NR_CellGroupConfig_t *cg = RC.nrmac[mod_id]->UE_info.CellGroup[UE_id];
  NR_BWP_UplinkDedicated_t *ubwpd=NULL;

  if (cg &&
      cg->spCellConfig &&
      cg->spCellConfig->spCellConfigDedicated &&
      cg->spCellConfig->spCellConfigDedicated->uplinkConfig &&
      cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP)
    ubwpd = cg->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP;

  NR_SearchSpace__searchSpaceType_PR ss_type = (is_common==0 && (sched_ctrl->active_bwp || ubwpd)) ? NR_SearchSpace__searchSpaceType_PR_ue_Specific: NR_SearchSpace__searchSpaceType_PR_common;
  uint8_t pdsch_to_harq_feedback[8];
  int bwp_Id = 0;
  if (sched_ctrl->active_ubwp) bwp_Id = sched_ctrl->active_ubwp->bwp_Id;

  int max_fb_time = 0;
  get_pdsch_to_harq_feedback(mod_id, UE_id, bwp_Id, ss_type, &max_fb_time, pdsch_to_harq_feedback);
  int max_absslot = frame*n_slots_frame + slot + max_fb_time;

  LOG_D(NR_MAC,"pucch_acknak 1b. DL %d.%d, UL_ACK %d.%d, DAI_C %d\n",frame,slot,pucch->frame,pucch->ul_slot,pucch->dai_c);
  /* there is a HARQ. Check whether we can use it for this ACKNACK */
  if (pucch->dai_c > 0) {
    /* this UE already has a PUCCH occasion */
    // Find the right timing_indicator value.
    int i = 0;
    while (i < 8) {
      if (pdsch_to_harq_feedback[i] == (pucch->ul_slot - slot) % n_slots_frame)
        break;
      ++i;
    }
    if (i >= 8) {
      // we cannot reach this timing anymore, allocate and try again
      const int f = pucch->frame;
      const int s = pucch->ul_slot;
      const int n_slots_frame = nr_slots_per_frame[*scc->ssbSubcarrierSpacing];
      LOG_D(NR_MAC,"pucch_acknak : %d.%d DAI > 0, cannot reach timing for pucch in %d.%d, advancing slot by 1 and trying again\n",frame,slot,f,s);
      nr_fill_nfapi_pucch(mod_id, frame, slot, pucch, UE_id);
      memset(pucch, 0, sizeof(*pucch));
      pucch->frame = s == n_slots_frame - 1 ? (f + 1) % 1024 : f;
      if(((s + 1)%nr_slots_period) == 0)
        pucch->ul_slot = (s + 1 + first_ul_slot_period) % n_slots_frame;
      else
        pucch->ul_slot = (s + 1) % n_slots_frame;
      return nr_acknack_scheduling(mod_id, UE_id, frame, slot, r_pucch,is_common);
    }

    pucch->timing_indicator = i;
    pucch->dai_c++;
    // retain old resource indicator, and we are good
    LOG_D(NR_MAC,"pucch_acknak : %d.%d. DAI > 0, pucch allocated for %d.%d (index %d)\n",frame,slot,pucch->frame,pucch->ul_slot,pucch->timing_indicator);
    return 0;
  }

  LOG_D(NR_MAC,"pucch_acknak : %d.%d DAI = 0, looking for new pucch occasion\n",frame,slot);
  /* we need to find a new PUCCH occasion */

  /*Inizialization of timing information*/
  if (pucch->frame == 0 && pucch->ul_slot == 0) {
    AssertFatal(pucch->sr_flag + pucch->dai_c == 0,
                "expected no SR/AckNack for UE %d in %4d.%2d, but has %d/%d for %4d.%2d\n",
                UE_id, frame, slot, pucch->sr_flag, pucch->dai_c, pucch->frame, pucch->ul_slot);
    pucch->frame = frame;
    pucch->ul_slot = first_ul_slot_tdd;
  }

  // Find the right timing_indicator value.
  int ind_found = -1;
  // while we are within the feedback limits and it has not been
  while ((pucch->frame*n_slots_frame + pucch->ul_slot) <= max_absslot) {
    int i = 0;
    while (i < 8) {
      LOG_D(NR_MAC,"pdsch_to_harq_feedback[%d] = %d (pucch->ul_slot %d - slot %d)\n",
            i,pdsch_to_harq_feedback[i],pucch->ul_slot,slot);
      if (pdsch_to_harq_feedback[i] == (pucch->ul_slot - slot) % n_slots_frame) {
        ind_found = i;
        break;
      }
      ++i;
    }
    if (ind_found!=-1)
      break;
    // advance to the next ul slot
    const int f = pucch->frame;
    const int s = pucch->ul_slot;
    pucch->frame = s == n_slots_frame - 1 ? (f + 1) % 1024 : f;
    if(((s + 1)%nr_slots_period) == 0)
      pucch->ul_slot = (s + 1 + first_ul_slot_period) % n_slots_frame;
    else
      pucch->ul_slot = (s + 1) % n_slots_frame;
  }
  if (ind_found==-1) {
    LOG_W(NR_MAC,
          "%4d.%2d could not find pdsch_to_harq_feedback for UE %d: earliest "
          "ack slot %d\n",
          frame,
          slot,
          UE_id,
          pucch->ul_slot);
    return -1;
  }

  // is there already CSI in this slot?
  csi_pucch = &sched_ctrl->sched_pucch[1];
  if (csi_pucch &&
      csi_pucch->csi_bits > 0 &&
      csi_pucch->frame == pucch->frame &&
      csi_pucch->ul_slot == pucch->ul_slot) {
    // skip the CSI PUCCH if it is present and if in the next frame/slot
    // and if we don't multiplex
    // FIXME currently we support at most 11 bits in pucch2 so skip also in that case
    if(!csi_pucch->simultaneous_harqcsi
       || ((csi_pucch->csi_bits + csi_pucch->dai_c) >= 11)) {
      nr_fill_nfapi_pucch(mod_id, frame, slot, csi_pucch, UE_id);
      memset(csi_pucch, 0, sizeof(*csi_pucch));
      /* advance the UL slot information in PUCCH by one so we won't schedule in
       * the same slot again */
      const int f = pucch->frame;
      const int s = pucch->ul_slot;
      memset(pucch, 0, sizeof(*pucch));
      pucch->frame = s == n_slots_frame - 1 ? (f + 1) % 1024 : f;
      if(((s + 1)%nr_slots_period) == 0)
        pucch->ul_slot = (s + 1 + first_ul_slot_period) % n_slots_frame;
      else
        pucch->ul_slot = (s + 1) % n_slots_frame;
      return nr_acknack_scheduling(mod_id, UE_id, frame, slot, r_pucch,is_common);
    }
    // multiplexing harq and csi in a pucch
    else {
      csi_pucch->timing_indicator = ind_found;
      csi_pucch->dai_c++;
      return 1;
    }
  }

  pucch->timing_indicator = ind_found; // index in the list of timing indicators

  LOG_D(NR_MAC,"pucch_acknak 2. DAI 0 DL %d.%d, UL_ACK %d.%d (index %d)\n",frame,slot,pucch->frame,pucch->ul_slot,pucch->timing_indicator);

  pucch->dai_c++;
  pucch->resource_indicator = 0; // each UE has dedicated PUCCH resources
  pucch->r_pucch=r_pucch;
  NR_PUCCH_Config_t *pucch_Config = NULL;
  if (sched_ctrl->active_ubwp) {
    pucch_Config = sched_ctrl->active_ubwp->bwp_Dedicated->pucch_Config->choice.setup;
  } else if (RC.nrmac[mod_id]->UE_info.CellGroup[UE_id] &&
             RC.nrmac[mod_id]->UE_info.CellGroup[UE_id]->spCellConfig &&
             RC.nrmac[mod_id]->UE_info.CellGroup[UE_id]->spCellConfig->spCellConfigDedicated &&
             RC.nrmac[mod_id]->UE_info.CellGroup[UE_id]->spCellConfig->spCellConfigDedicated->uplinkConfig &&
             RC.nrmac[mod_id]->UE_info.CellGroup[UE_id]->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP &&
             RC.nrmac[mod_id]->UE_info.CellGroup[UE_id]->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup) {
    pucch_Config = RC.nrmac[mod_id]->UE_info.CellGroup[UE_id]->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup;
  }

  NR_BWP_t *genericParameters = sched_ctrl->active_ubwp ?
    &sched_ctrl->active_ubwp->bwp_Common->genericParameters:
    &scc->uplinkConfigCommon->initialUplinkBWP->genericParameters;
  int bwp_start = NRRIV2PRBOFFSET(genericParameters->locationAndBandwidth,MAX_BWP_SIZE);

  /* verify that at that slot and symbol, resources are free. We only do this
   * for initialCyclicShift 0 (we assume it always has that one), so other
   * initialCyclicShifts can overlap with ICS 0!*/
  if (pucch_Config) {
    const NR_PUCCH_Resource_t *resource = pucch_Config->resourceToAddModList->list.array[pucch->resource_indicator];
    DevAssert(resource->format.present == NR_PUCCH_Resource__format_PR_format0);
    int second_hop_prb = resource->secondHopPRB!= NULL ?  *resource->secondHopPRB : 0;
    int nr_of_symbols = resource->format.choice.format0->nrofSymbols;
    if (resource->format.choice.format0->initialCyclicShift == 0) {
      uint16_t *vrb_map_UL = &RC.nrmac[mod_id]->common_channels[CC_id].vrb_map_UL[pucch->ul_slot * MAX_BWP_SIZE];
      for (int l=0; l<nr_of_symbols; l++) {
        uint16_t symb = 1 << (resource->format.choice.format0->startingSymbolIndex + l);
        int prb;
        if (l==1 && second_hop_prb != 0)
          prb = second_hop_prb;
        else
          prb = resource->startingPRB;
        if ((vrb_map_UL[bwp_start+prb] & symb) != 0)
          LOG_W(MAC, "symbol 0x%x is not free for PUCCH alloc in vrb_map_UL at RB %ld and slot %d.%d\n", symb, resource->startingPRB, pucch->frame, pucch->ul_slot);
        vrb_map_UL[bwp_start+prb] |= symb;
      }
    }
  }
  return 0;
}


void nr_sr_reporting(int Mod_idP, frame_t SFN, sub_frame_t slot)
{
  gNB_MAC_INST *nrmac = RC.nrmac[Mod_idP];
  if (!is_xlsch_in_slot(nrmac->ulsch_slot_bitmap[slot / 64], slot))
    return;
  NR_ServingCellConfigCommon_t *scc = nrmac->common_channels->ServingCellConfigCommon;
  const int n_slots_frame = nr_slots_per_frame[*scc->ssbSubcarrierSpacing];
  NR_UE_info_t *UE_info = &nrmac->UE_info;
  NR_list_t *UE_list = &UE_info->list;
  for (int UE_id = UE_list->head; UE_id >= 0; UE_id = UE_list->next[UE_id]) {
    NR_UE_sched_ctrl_t *sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];

    if (sched_ctrl->ul_failure==1) continue;
    NR_PUCCH_Config_t *pucch_Config = NULL;
    if (sched_ctrl->active_ubwp) {
      pucch_Config = sched_ctrl->active_ubwp->bwp_Dedicated->pucch_Config->choice.setup;
    } else if (RC.nrmac[Mod_idP]->UE_info.CellGroup[UE_id] &&
             RC.nrmac[Mod_idP]->UE_info.CellGroup[UE_id]->spCellConfig &&
             RC.nrmac[Mod_idP]->UE_info.CellGroup[UE_id]->spCellConfig->spCellConfigDedicated &&
             RC.nrmac[Mod_idP]->UE_info.CellGroup[UE_id]->spCellConfig->spCellConfigDedicated->uplinkConfig &&
             RC.nrmac[Mod_idP]->UE_info.CellGroup[UE_id]->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP &&
             RC.nrmac[Mod_idP]->UE_info.CellGroup[UE_id]->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup) {
      pucch_Config = RC.nrmac[Mod_idP]->UE_info.CellGroup[UE_id]->spCellConfig->spCellConfigDedicated->uplinkConfig->initialUplinkBWP->pucch_Config->choice.setup;
    }
    else continue;
    if (!pucch_Config->schedulingRequestResourceToAddModList) continue;
    AssertFatal(pucch_Config->schedulingRequestResourceToAddModList->list.count>0,"NO SR configuration available");

    for (int SR_resource_id =0; SR_resource_id < pucch_Config->schedulingRequestResourceToAddModList->list.count;SR_resource_id++) {
      NR_SchedulingRequestResourceConfig_t *SchedulingRequestResourceConfig = pucch_Config->schedulingRequestResourceToAddModList->list.array[SR_resource_id];

      int SR_period; int SR_offset;

      find_period_offest_SR(SchedulingRequestResourceConfig,&SR_period,&SR_offset);
      // convert to int to avoid underflow of uint
      int sfn_sf = SFN * n_slots_frame + slot;
      LOG_D(NR_MAC,"SR_resource_id %d: SR_period %d, SR_offset %d\n",SR_resource_id,SR_period,SR_offset);
      if ((sfn_sf - SR_offset) % SR_period != 0)
        continue;
      LOG_D(NR_MAC, "%4d.%2d Scheduling Request identified\n", SFN, slot);
      NR_PUCCH_ResourceId_t *PucchResourceId = SchedulingRequestResourceConfig->resource;

      int found = -1;
      NR_PUCCH_ResourceSet_t *pucchresset = pucch_Config->resourceSetToAddModList->list.array[0]; // set with formats 0,1
      int n_list = pucchresset->resourceList.list.count;
       for (int i=0; i<n_list; i++) {
        if (*pucchresset->resourceList.list.array[i] == *PucchResourceId )
          found = i;
      }
      AssertFatal(found>-1,"SR resource not found among PUCCH resources");

      /* loop through nFAPI PUCCH messages: if the UEs is in there in this slot
       * with the resource_indicator, it means we already allocated that PUCCH
       * resource for AckNack (e.g., the UE has been scheduled often), and we
       * just need to add the SR_flag. Otherwise, just allocate in the internal
       * PUCCH resource, and nr_schedule_pucch() will handle the rest */
      NR_PUCCH_Resource_t *pucch_res = pucch_Config->resourceToAddModList->list.array[found];
      /* for the moment, can only handle SR on PUCCH Format 0 */
      DevAssert(pucch_res->format.present == NR_PUCCH_Resource__format_PR_format0);
      nfapi_nr_ul_tti_request_t *ul_tti_req = &nrmac->UL_tti_req_ahead[0][slot];
      bool nfapi_allocated = false;
      for (int i = 0; i < ul_tti_req->n_pdus; ++i) {
        if (ul_tti_req->pdus_list[i].pdu_type != NFAPI_NR_UL_CONFIG_PUCCH_PDU_TYPE)
          continue;
        nfapi_nr_pucch_pdu_t *pdu = &ul_tti_req->pdus_list[i].pucch_pdu;
        /* check that it is our PUCCH F0. Assuming there can be only one */
        if (pdu->rnti == UE_info->rnti[UE_id]
            && pdu->format_type == 0 // does not use NR_PUCCH_Resource__format_PR_format0
            && pdu->initial_cyclic_shift == pucch_res->format.choice.format0->initialCyclicShift
            && pdu->nr_of_symbols == pucch_res->format.choice.format0->nrofSymbols
            && pdu->start_symbol_index == pucch_res->format.choice.format0->startingSymbolIndex) {
          LOG_D(NR_MAC,"%4d.%2d adding SR_flag 1 to PUCCH nFAPI SR for RNTI %04x\n", SFN, slot, pdu->rnti);
          pdu->sr_flag = 1;
          nfapi_allocated = true;
          break;
        }
      }

      if (nfapi_allocated)  // break scheduling resource loop, continue next UE
        break;

      /* we did not find it: check if current PUCCH is for the current slot, in
       * which case we add the SR to it; otherwise, allocate SR separately */
      NR_sched_pucch_t *curr_pucch = &sched_ctrl->sched_pucch[0];
      if (curr_pucch->frame == SFN && curr_pucch->ul_slot == slot) {
        if (curr_pucch->resource_indicator != found) {
          LOG_W(NR_MAC, "%4d.%2d expected PUCCH in this slot to have resource indicator of SR (%d), skipping SR\n", SFN, slot, found);
          continue;
        }
        curr_pucch->sr_flag = true;
      } else {
        NR_sched_pucch_t sched_sr;
        memset(&sched_sr, 0, sizeof(sched_sr));
        sched_sr.frame = SFN;
        sched_sr.ul_slot = slot;
        sched_sr.resource_indicator = found;
        sched_sr.sr_flag = true;
        nr_fill_nfapi_pucch(Mod_idP, SFN, slot, &sched_sr, UE_id);
      }
    }
  }
}

