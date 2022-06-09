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

/*
  enb_config_SL.c
  -------------------
  AUTHOR  : R. Knopp
  COMPANY : EURECOM
  EMAIL   : raymond.knopp@eurecom.fr
*/

#include <string.h>
#include <inttypes.h>

#include "common/utils/LOG/log.h"
#include "assertions.h"
#include "enb_config.h"
#include "intertask_interface.h"
#include "LTE_SystemInformationBlockType2.h"
#include "common/config/config_userapi.h"
#include "RRC_config_tools.h"
#include "enb_paramdef.h"

void fill_eMTC_configuration(MessageDef *msg_p,  ccparams_eMTC_t *eMTCconfig, int cell_idx,int cc_idx,char *config_fname,char *brparamspath) {

  paramdef_t schedulingInfoBrParams[] = SI_INFO_BR_DESC(eMTCconfig);
  paramlist_def_t schedulingInfoBrParamList = {ENB_CONFIG_STRING_SCHEDULING_INFO_BR, NULL, 0};
  paramdef_t rachcelevelParams[]     = RACH_CE_LEVELINFOLIST_R13_DESC(eMTCconfig);
  paramlist_def_t rachcelevellist    = {ENB_CONFIG_STRING_RACH_CE_LEVELINFOLIST_R13, NULL, 0};
  paramdef_t rsrprangeParams[]       = RSRP_RANGE_LIST_DESC(eMTCconfig);
  paramlist_def_t rsrprangelist      = {ENB_CONFIG_STRING_RSRP_RANGE_LIST, NULL, 0};
  paramdef_t prachParams[]           = PRACH_PARAMS_CE_R13_DESC(eMTCconfig);
  paramlist_def_t prachParamslist    = {ENB_CONFIG_STRING_PRACH_PARAMETERS_CE_R13, NULL, 0};
  paramdef_t n1PUCCH_ANR13Params[]   = N1PUCCH_AN_INFOLIST_R13_DESC(eMTCconfig);
  paramlist_def_t n1PUCCHInfoList    = {ENB_CONFIG_STRING_N1PUCCH_AN_INFOLIST_R13, NULL, 0};
  paramdef_t pcchv1310Params[]       = PCCH_CONFIG_V1310_DESC(eMTCconfig);
  paramdef_t sib2freqhoppingParams[] = SIB2_FREQ_HOPPING_R13_DESC(eMTCconfig);



  printf("Found parameters for eMTC from %s : %s\n",config_fname,brparamspath);
  RRC_CONFIGURATION_REQ(msg_p).schedulingInfoSIB1_BR_r13[cc_idx] = eMTCconfig->schedulingInfoSIB1_BR_r13;


  if (!strcmp(eMTCconfig->cellSelectionInfoCE_r13, "ENABLE")) {
    RRC_CONFIGURATION_REQ(msg_p).cellSelectionInfoCE_r13[cc_idx] = TRUE;
    RRC_CONFIGURATION_REQ(msg_p).q_RxLevMinCE_r13[cc_idx]= eMTCconfig->q_RxLevMinCE_r13;
    //                            RRC_CONFIGURATION_REQ(msg_p).q_QualMinRSRQ_CE_r13[cc_idx]= calloc(1, sizeof(long));
    //                            *RRC_CONFIGURATION_REQ(msg_p).q_QualMinRSRQ_CE_r13[cc_idx]= q_QualMinRSRQ_CE_r13;
  } else {
    RRC_CONFIGURATION_REQ(msg_p).cellSelectionInfoCE_r13[cc_idx] = FALSE;
  }



  if (!strcmp(eMTCconfig->bandwidthReducedAccessRelatedInfo_r13, "ENABLE")) {
    RRC_CONFIGURATION_REQ(msg_p).bandwidthReducedAccessRelatedInfo_r13[cc_idx] = TRUE;



    if (!strcmp(eMTCconfig->si_WindowLength_BR_r13, "ms20")) {
      RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[cc_idx] = 0;
    } else if (!strcmp(eMTCconfig->si_WindowLength_BR_r13, "ms40")) {
      RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[cc_idx] = 1;
    } else if (!strcmp(eMTCconfig->si_WindowLength_BR_r13, "ms60")) {
      RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[cc_idx] = 2;
    } else if (!strcmp(eMTCconfig->si_WindowLength_BR_r13, "ms80")) {
      RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[cc_idx] = 3;
    } else if (!strcmp(eMTCconfig->si_WindowLength_BR_r13, "ms120")) {
      RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[cc_idx] = 4;
    } else if (!strcmp(eMTCconfig->si_WindowLength_BR_r13, "ms160")) {
      RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[cc_idx] = 5;
    } else if (!strcmp(eMTCconfig->si_WindowLength_BR_r13, "ms200")) {
      RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[cc_idx] = 6;
    } else if (!strcmp(eMTCconfig->si_WindowLength_BR_r13, "spare")) {
      RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[cc_idx] = 7;
    }


    if (!strcmp(eMTCconfig->si_RepetitionPattern_r13, "everyRF")) {
      RRC_CONFIGURATION_REQ(msg_p).si_RepetitionPattern_r13[cc_idx] = 0;
    } else if (!strcmp(eMTCconfig->si_RepetitionPattern_r13, "every2ndRF")) {
      RRC_CONFIGURATION_REQ(msg_p).si_RepetitionPattern_r13[cc_idx] = 1;
    } else if (!strcmp(eMTCconfig->si_RepetitionPattern_r13, "every4thRF")) {
      RRC_CONFIGURATION_REQ(msg_p).si_RepetitionPattern_r13[cc_idx] = 2;
    } else if (!strcmp(eMTCconfig->si_RepetitionPattern_r13, "every8thRF")) {
      RRC_CONFIGURATION_REQ(msg_p).si_RepetitionPattern_r13[cc_idx] = 3;
    }

  } else {
    RRC_CONFIGURATION_REQ(msg_p).bandwidthReducedAccessRelatedInfo_r13[cc_idx] = FALSE;
  }

  char schedulingInfoBrPath[MAX_OPTNAME_SIZE * 2];
  config_getlist(&schedulingInfoBrParamList, NULL, 0, brparamspath);
  RRC_CONFIGURATION_REQ (msg_p).scheduling_info_br_size[cc_idx] = schedulingInfoBrParamList.numelt;
  int siInfoindex;
  for (siInfoindex = 0; siInfoindex < schedulingInfoBrParamList.numelt; siInfoindex++) {
    sprintf(schedulingInfoBrPath, "%s.%s.[%i]", brparamspath, ENB_CONFIG_STRING_EMTC_PARAMETERS, siInfoindex);
    config_get(schedulingInfoBrParams, sizeof(schedulingInfoBrParams) / sizeof(paramdef_t), schedulingInfoBrPath);
    RRC_CONFIGURATION_REQ (msg_p).si_Narrowband_r13[cc_idx][siInfoindex] = eMTCconfig->si_Narrowband_r13;
    RRC_CONFIGURATION_REQ (msg_p).si_TBS_r13[cc_idx][siInfoindex] = eMTCconfig->si_TBS_r13;
  }



  //                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[cc_idx].system_info_value_tag_SI_size[cc_idx] = 0;


  RRC_CONFIGURATION_REQ(msg_p).fdd_DownlinkOrTddSubframeBitmapBR_r13[cc_idx] = CALLOC(1, sizeof(BOOLEAN_t));
  if (!strcmp(eMTCconfig->fdd_DownlinkOrTddSubframeBitmapBR_r13, "subframePattern40-r13")) {
    *RRC_CONFIGURATION_REQ(msg_p).fdd_DownlinkOrTddSubframeBitmapBR_r13[cc_idx] = FALSE;
    RRC_CONFIGURATION_REQ(msg_p).fdd_DownlinkOrTddSubframeBitmapBR_val_r13[cc_idx] = eMTCconfig->fdd_DownlinkOrTddSubframeBitmapBR_val_r13;
  } else {
    *RRC_CONFIGURATION_REQ(msg_p).fdd_DownlinkOrTddSubframeBitmapBR_r13[cc_idx] = TRUE;
    RRC_CONFIGURATION_REQ(msg_p).fdd_DownlinkOrTddSubframeBitmapBR_val_r13[cc_idx] = eMTCconfig->fdd_DownlinkOrTddSubframeBitmapBR_val_r13;
  }

  RRC_CONFIGURATION_REQ(msg_p).startSymbolBR_r13[cc_idx] = eMTCconfig->startSymbolBR_r13;


  if (!strcmp(eMTCconfig->si_HoppingConfigCommon_r13, "off")) {
    RRC_CONFIGURATION_REQ(msg_p).si_HoppingConfigCommon_r13[cc_idx] = 1;
  } else if (!strcmp(eMTCconfig->si_HoppingConfigCommon_r13, "on")) {
    RRC_CONFIGURATION_REQ(msg_p).si_HoppingConfigCommon_r13[cc_idx] = 0;
  }


  RRC_CONFIGURATION_REQ(msg_p).si_ValidityTime_r13[cc_idx] = calloc(1, sizeof(long));
  if (!strcmp(eMTCconfig->si_ValidityTime_r13, "true")) {
    *RRC_CONFIGURATION_REQ(msg_p).si_ValidityTime_r13[cc_idx] = 0;
  } else {
    AssertFatal(0,
		"Failed to parse eNB configuration file %s, enb %d  si_ValidityTime_r13 unknown value!\n",
		config_fname, cell_idx);
  }


  if (!strcmp(eMTCconfig->freqHoppingParametersDL_r13, "ENABLE"))
    {
      RRC_CONFIGURATION_REQ(msg_p).freqHoppingParametersDL_r13[cc_idx] = TRUE;

      if (!strcmp(eMTCconfig->interval_DLHoppingConfigCommonModeA_r13, "interval-TDD-r13"))
	RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeA_r13[cc_idx] = FALSE;
      else
	RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeA_r13[cc_idx] = TRUE;
      RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeA_r13_val[cc_idx] = eMTCconfig->interval_DLHoppingConfigCommonModeA_r13_val;

      if (!strcmp(eMTCconfig->interval_DLHoppingConfigCommonModeB_r13, "interval-TDD-r13"))
	RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeB_r13[cc_idx] = FALSE;
      else
	RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeB_r13[cc_idx] = TRUE;
      RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeB_r13_val[cc_idx] = eMTCconfig->interval_DLHoppingConfigCommonModeB_r13_val;

      RRC_CONFIGURATION_REQ(msg_p).mpdcch_pdsch_HoppingNB_r13[cc_idx] = calloc(1, sizeof(long));
      if (!strcmp(eMTCconfig->mpdcch_pdsch_HoppingNB_r13, "nb2")) {
	*RRC_CONFIGURATION_REQ(msg_p).mpdcch_pdsch_HoppingNB_r13[cc_idx] = 0;
      } else if (!strcmp(eMTCconfig->mpdcch_pdsch_HoppingNB_r13, "nb4")) {
	*RRC_CONFIGURATION_REQ(msg_p).mpdcch_pdsch_HoppingNB_r13[cc_idx] = 1;
      } else {
	AssertFatal(0,
		    "Failed to parse eNB configuration file %s, enb %d  mpdcch_pdsch_HoppingNB_r13 unknown value!\n",
		    config_fname, cell_idx);
      }


      RRC_CONFIGURATION_REQ(msg_p).mpdcch_pdsch_HoppingOffset_r13[cc_idx] = calloc(1, sizeof(long));
      *RRC_CONFIGURATION_REQ(msg_p).mpdcch_pdsch_HoppingOffset_r13[cc_idx] = eMTCconfig->mpdcch_pdsch_HoppingOffset_r13;

    }
  else
    {
      RRC_CONFIGURATION_REQ(msg_p).freqHoppingParametersDL_r13[cc_idx] = FALSE;
    }

  /** ------------------------------SIB2/3 BR------------------------------------------ */


  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].prach_root =  eMTCconfig->ccparams.prach_root;

  if ((eMTCconfig->ccparams.prach_root <0) || (eMTCconfig->ccparams.prach_root > 1023))
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_root choice: 0..1023 !\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.prach_root);

  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].prach_config_index = eMTCconfig->ccparams.prach_config_index;

  if ((eMTCconfig->ccparams.prach_config_index <0) || (eMTCconfig->ccparams.prach_config_index > 63))
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_config_index choice: 0..1023 !\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.prach_config_index);

  if (!eMTCconfig->ccparams.prach_high_speed)
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
		 config_fname, cell_idx,ENB_CONFIG_STRING_PRACH_HIGH_SPEED);
  else if (strcmp(eMTCconfig->ccparams.prach_high_speed, "ENABLE") == 0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].prach_high_speed = TRUE;
  } else if (strcmp(eMTCconfig->ccparams.prach_high_speed, "DISABLE") == 0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].prach_high_speed = FALSE;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for prach_config choice: ENABLE,DISABLE !\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.prach_high_speed);

  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].prach_zero_correlation = eMTCconfig->ccparams.prach_zero_correlation;

  if ((eMTCconfig->ccparams.prach_zero_correlation <0) || (eMTCconfig->ccparams.prach_zero_correlation > 15))
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_zero_correlation choice: 0..15!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.prach_zero_correlation);

  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].prach_freq_offset = eMTCconfig->ccparams.prach_freq_offset;

  if ((eMTCconfig->ccparams.prach_freq_offset <0) || (eMTCconfig->ccparams.prach_freq_offset > 94))
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_freq_offset choice: 0..94!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.prach_freq_offset);

  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pucch_delta_shift = eMTCconfig->ccparams.pucch_delta_shift-1;

  if ((eMTCconfig->ccparams.pucch_delta_shift <1) || (eMTCconfig->ccparams.pucch_delta_shift > 3))
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_delta_shift choice: 1..3!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pucch_delta_shift);

  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pucch_nRB_CQI = eMTCconfig->ccparams.pucch_nRB_CQI;

  if ((eMTCconfig->ccparams.pucch_nRB_CQI <0) || (eMTCconfig->ccparams.pucch_nRB_CQI > 98))
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_nRB_CQI choice: 0..98!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pucch_nRB_CQI);

  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pucch_nCS_AN = eMTCconfig->ccparams.pucch_nCS_AN;

  if ((eMTCconfig->ccparams.pucch_nCS_AN <0) || (eMTCconfig->ccparams.pucch_nCS_AN > 7))
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_nCS_AN choice: 0..7!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pucch_nCS_AN);

  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pucch_n1_AN = eMTCconfig->ccparams.pucch_n1_AN;

  if ((eMTCconfig->ccparams.pucch_n1_AN <0) || (eMTCconfig->ccparams.pucch_n1_AN > 2047))
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_n1_AN choice: 0..2047!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pucch_n1_AN);

  //#endif
  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pdsch_referenceSignalPower = eMTCconfig->ccparams.pdsch_referenceSignalPower;

  if ((eMTCconfig->ccparams.pdsch_referenceSignalPower <-60) || (eMTCconfig->ccparams.pdsch_referenceSignalPower > 50))
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pdsch_referenceSignalPower choice:-60..50!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pdsch_referenceSignalPower);

  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pdsch_p_b = eMTCconfig->ccparams.pdsch_p_b;

  if ((eMTCconfig->ccparams.pdsch_p_b <0) || (eMTCconfig->ccparams.pdsch_p_b > 3))
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pdsch_p_b choice: 0..3!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pdsch_p_b);

  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pusch_n_SB = eMTCconfig->ccparams.pusch_n_SB;

  if ((eMTCconfig->ccparams.pusch_n_SB <1) || (eMTCconfig->ccparams.pusch_n_SB > 4))
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_n_SB choice: 1..4!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pusch_n_SB);

  if (!eMTCconfig->ccparams.pusch_hoppingMode)
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d define %s: interSubframe,intraAndInterSubframe!\n",
		 config_fname, cell_idx,ENB_CONFIG_STRING_PUSCH_HOPPINGMODE);
  else if (strcmp(eMTCconfig->ccparams.pusch_hoppingMode,"interSubFrame")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pusch_hoppingMode = LTE_PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_interSubFrame;
  }  else if (strcmp(eMTCconfig->ccparams.pusch_hoppingMode,"intraAndInterSubFrame")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pusch_hoppingMode = LTE_PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_intraAndInterSubFrame;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_hoppingMode choice: interSubframe,intraAndInterSubframe!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pusch_hoppingMode);

  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pusch_hoppingOffset = eMTCconfig->ccparams.pusch_hoppingOffset;

  if ((eMTCconfig->ccparams.pusch_hoppingOffset<0) || (eMTCconfig->ccparams.pusch_hoppingOffset>98))
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_hoppingOffset choice: 0..98!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pusch_hoppingMode);

  if (!eMTCconfig->ccparams.pusch_enable64QAM)
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
		 config_fname, cell_idx,ENB_CONFIG_STRING_PUSCH_ENABLE64QAM);
  else if (strcmp(eMTCconfig->ccparams.pusch_enable64QAM, "ENABLE") == 0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pusch_enable64QAM = TRUE;
  }  else if (strcmp(eMTCconfig->ccparams.pusch_enable64QAM, "DISABLE") == 0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pusch_enable64QAM = FALSE;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_enable64QAM choice: ENABLE,DISABLE!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pusch_enable64QAM);

  if (!eMTCconfig->ccparams.pusch_groupHoppingEnabled)
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
		 config_fname, cell_idx,ENB_CONFIG_STRING_PUSCH_GROUP_HOPPING_EN);
  else if (strcmp(eMTCconfig->ccparams.pusch_groupHoppingEnabled, "ENABLE") == 0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pusch_groupHoppingEnabled = TRUE;
  }  else if (strcmp(eMTCconfig->ccparams.pusch_groupHoppingEnabled, "DISABLE") == 0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pusch_groupHoppingEnabled= FALSE;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_groupHoppingEnabled choice: ENABLE,DISABLE!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pusch_groupHoppingEnabled);

  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pusch_groupAssignment = eMTCconfig->ccparams.pusch_groupAssignment;

  if ((eMTCconfig->ccparams.pusch_groupAssignment<0)||(eMTCconfig->ccparams.pusch_groupAssignment>29))
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_groupAssignment choice: 0..29!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pusch_groupAssignment);

  if (!eMTCconfig->ccparams.pusch_sequenceHoppingEnabled)
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
		 config_fname, cell_idx,ENB_CONFIG_STRING_PUSCH_SEQUENCE_HOPPING_EN);
  else if (strcmp(eMTCconfig->ccparams.pusch_sequenceHoppingEnabled, "ENABLE") == 0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pusch_sequenceHoppingEnabled = TRUE;
  }  else if (strcmp(eMTCconfig->ccparams.pusch_sequenceHoppingEnabled, "DISABLE") == 0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pusch_sequenceHoppingEnabled = FALSE;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_sequenceHoppingEnabled choice: ENABLE,DISABLE!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pusch_sequenceHoppingEnabled);

  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pusch_nDMRS1= eMTCconfig->ccparams.pusch_nDMRS1;  //cyclic_shift in RRC!

  if ((eMTCconfig->ccparams.pusch_nDMRS1 <0) || (eMTCconfig->ccparams.pusch_nDMRS1>7))
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_nDMRS1 choice: 0..7!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pusch_nDMRS1);

  if (strcmp(eMTCconfig->ccparams.phich_duration,"NORMAL")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].phich_duration= LTE_PHICH_Config__phich_Duration_normal;
  } else if (strcmp(eMTCconfig->ccparams.phich_duration,"EXTENDED")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].phich_duration= LTE_PHICH_Config__phich_Duration_extended;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for phich_duration choice: NORMAL,EXTENDED!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.phich_duration);

  if (strcmp(eMTCconfig->ccparams.phich_resource,"ONESIXTH")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].phich_resource= LTE_PHICH_Config__phich_Resource_oneSixth ;
  } else if (strcmp(eMTCconfig->ccparams.phich_resource,"HALF")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].phich_resource= LTE_PHICH_Config__phich_Resource_half;
  } else if (strcmp(eMTCconfig->ccparams.phich_resource,"ONE")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].phich_resource= LTE_PHICH_Config__phich_Resource_one;
  } else if (strcmp(eMTCconfig->ccparams.phich_resource,"TWO")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].phich_resource= LTE_PHICH_Config__phich_Resource_two;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for phich_resource choice: ONESIXTH,HALF,ONE,TWO!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.phich_resource);

  printf("phich.resource eMTC %ld (%s), phich.duration eMTC %ld (%s)\n",
	 RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].phich_resource,eMTCconfig->ccparams.phich_resource,
	 RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].phich_duration,eMTCconfig->ccparams.phich_duration);

  if (strcmp(eMTCconfig->ccparams.srs_enable, "ENABLE") == 0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].srs_enable= TRUE;
  } else if (strcmp(eMTCconfig->ccparams.srs_enable, "DISABLE") == 0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].srs_enable= FALSE;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_BandwidthConfig choice: ENABLE,DISABLE !\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.srs_enable);

  if (RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].srs_enable== TRUE) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].srs_BandwidthConfig= eMTCconfig->ccparams.srs_BandwidthConfig;

    if ((eMTCconfig->ccparams.srs_BandwidthConfig < 0) || (eMTCconfig->ccparams.srs_BandwidthConfig >7))
      AssertFatal (0, "Failed to parse eNB configuration file %s, enb %d unknown value %d for srs_BandwidthConfig choice: 0...7\n",
		   config_fname, cell_idx,eMTCconfig->ccparams.srs_BandwidthConfig);

    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].srs_SubframeConfig= eMTCconfig->ccparams.srs_SubframeConfig;

    if ((eMTCconfig->ccparams.srs_SubframeConfig<0) || (eMTCconfig->ccparams.srs_SubframeConfig>15))
      AssertFatal (0,
		   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for srs_SubframeConfig choice: 0..15 !\n",
		   config_fname, cell_idx,eMTCconfig->ccparams.srs_SubframeConfig);

    if (strcmp(eMTCconfig->ccparams.srs_ackNackST, "ENABLE") == 0) {
      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].srs_ackNackST= TRUE;
    } else if (strcmp(eMTCconfig->ccparams.srs_ackNackST, "DISABLE") == 0) {
      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].srs_ackNackST= FALSE;
    } else
      AssertFatal (0,
		   "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_BandwidthConfig choice: ENABLE,DISABLE !\n",
		   config_fname, cell_idx,eMTCconfig->ccparams.srs_ackNackST);

    if (strcmp(eMTCconfig->ccparams.srs_MaxUpPts, "ENABLE") == 0) {
      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].srs_MaxUpPts= TRUE;
    } else if (strcmp(eMTCconfig->ccparams.srs_MaxUpPts, "DISABLE") == 0) {
      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].srs_MaxUpPts= FALSE;
    } else
      AssertFatal (0,
		   "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_MaxUpPts choice: ENABLE,DISABLE !\n",
		   config_fname, cell_idx,eMTCconfig->ccparams.srs_MaxUpPts);
  }

  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pusch_p0_Nominal= eMTCconfig->ccparams.pusch_p0_Nominal;

  if ((eMTCconfig->ccparams.pusch_p0_Nominal<-126) || (eMTCconfig->ccparams.pusch_p0_Nominal>24))
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_p0_Nominal choice: -126..24 !\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pusch_p0_Nominal);

  if (strcmp(eMTCconfig->ccparams.pusch_alpha,"AL0")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pusch_alpha= LTE_Alpha_r12_al0;
  } else if (strcmp(eMTCconfig->ccparams.pusch_alpha,"AL04")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pusch_alpha= LTE_Alpha_r12_al04;
  } else if (strcmp(eMTCconfig->ccparams.pusch_alpha,"AL05")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pusch_alpha= LTE_Alpha_r12_al05;
  } else if (strcmp(eMTCconfig->ccparams.pusch_alpha,"AL06")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pusch_alpha= LTE_Alpha_r12_al06;
  } else if (strcmp(eMTCconfig->ccparams.pusch_alpha,"AL07")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pusch_alpha= LTE_Alpha_r12_al07;
  } else if (strcmp(eMTCconfig->ccparams.pusch_alpha,"AL08")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pusch_alpha= LTE_Alpha_r12_al08;
  } else if (strcmp(eMTCconfig->ccparams.pusch_alpha,"AL09")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pusch_alpha= LTE_Alpha_r12_al09;
  } else if (strcmp(eMTCconfig->ccparams.pusch_alpha,"AL1")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pusch_alpha= LTE_Alpha_r12_al1;
  }

  else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_Alpha choice: AL0,AL04,AL05,AL06,AL07,AL08,AL09,AL1!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pusch_alpha);

  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pucch_p0_Nominal= eMTCconfig->ccparams.pucch_p0_Nominal;

  if ((eMTCconfig->ccparams.pucch_p0_Nominal<-127) || (eMTCconfig->ccparams.pucch_p0_Nominal>-96))
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_p0_Nominal choice: -127..-96 !\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pucch_p0_Nominal);

  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].msg3_delta_Preamble= eMTCconfig->ccparams.msg3_delta_Preamble;

  if ((eMTCconfig->ccparams.msg3_delta_Preamble<-1) || (eMTCconfig->ccparams.msg3_delta_Preamble>6))
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for msg3_delta_Preamble choice: -1..6 !\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.msg3_delta_Preamble);

  if (strcmp(eMTCconfig->ccparams.pucch_deltaF_Format1,"deltaF_2")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pucch_deltaF_Format1= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF_2;
  } else if (strcmp(eMTCconfig->ccparams.pucch_deltaF_Format1,"deltaF0")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pucch_deltaF_Format1= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF0;
  } else if (strcmp(eMTCconfig->ccparams.pucch_deltaF_Format1,"deltaF2")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pucch_deltaF_Format1= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF2;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format1 choice: deltaF_2,dltaF0,deltaF2!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pucch_deltaF_Format1);

  if (strcmp(eMTCconfig->ccparams.pucch_deltaF_Format1b,"deltaF1")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pucch_deltaF_Format1b= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF1;
  } else if (strcmp(eMTCconfig->ccparams.pucch_deltaF_Format1b,"deltaF3")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pucch_deltaF_Format1b= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF3;
  } else if (strcmp(eMTCconfig->ccparams.pucch_deltaF_Format1b,"deltaF5")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pucch_deltaF_Format1b= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF5;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format1b choice: deltaF1,dltaF3,deltaF5!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pucch_deltaF_Format1b);

  if (strcmp(eMTCconfig->ccparams.pucch_deltaF_Format2,"deltaF_2")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pucch_deltaF_Format2= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF_2;
  } else if (strcmp(eMTCconfig->ccparams.pucch_deltaF_Format2,"deltaF0")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pucch_deltaF_Format2= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF0;
  } else if (strcmp(eMTCconfig->ccparams.pucch_deltaF_Format2,"deltaF1")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pucch_deltaF_Format2= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF1;
  } else if (strcmp(eMTCconfig->ccparams.pucch_deltaF_Format2,"deltaF2")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pucch_deltaF_Format2= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF2;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2 choice: deltaF_2,dltaF0,deltaF1,deltaF2!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pucch_deltaF_Format2);

  if (strcmp(eMTCconfig->ccparams.pucch_deltaF_Format2a,"deltaF_2")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pucch_deltaF_Format2a= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF_2;
  } else if (strcmp(eMTCconfig->ccparams.pucch_deltaF_Format2a,"deltaF0")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pucch_deltaF_Format2a= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF0;
  } else if (strcmp(eMTCconfig->ccparams.pucch_deltaF_Format2a,"deltaF2")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pucch_deltaF_Format2a= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF2;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2a choice: deltaF_2,dltaF0,deltaF2!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pucch_deltaF_Format2a);

  if (strcmp(eMTCconfig->ccparams.pucch_deltaF_Format2b,"deltaF_2")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pucch_deltaF_Format2b= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF_2;
  } else if (strcmp(eMTCconfig->ccparams.pucch_deltaF_Format2b,"deltaF0")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pucch_deltaF_Format2b= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF0;
  } else if (strcmp(eMTCconfig->ccparams.pucch_deltaF_Format2b,"deltaF2")==0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pucch_deltaF_Format2b= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF2;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2b choice: deltaF_2,dltaF0,deltaF2!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pucch_deltaF_Format2b);

  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_numberOfRA_Preambles= (eMTCconfig->ccparams.rach_numberOfRA_Preambles/4)-1;

  if ((eMTCconfig->ccparams.rach_numberOfRA_Preambles <4) || (eMTCconfig->ccparams.rach_numberOfRA_Preambles >64) || ((eMTCconfig->ccparams.rach_numberOfRA_Preambles&3)!=0))
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_numberOfRA_Preambles choice: 4,8,12,...,64!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.rach_numberOfRA_Preambles);

  if (strcmp(eMTCconfig->ccparams.rach_preamblesGroupAConfig, "ENABLE") == 0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_preamblesGroupAConfig= TRUE;
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_sizeOfRA_PreamblesGroupA= (eMTCconfig->ccparams.rach_sizeOfRA_PreamblesGroupA/4)-1;

    if ((eMTCconfig->ccparams.rach_numberOfRA_Preambles <4) || (eMTCconfig->ccparams.rach_numberOfRA_Preambles>60) || ((eMTCconfig->ccparams.rach_numberOfRA_Preambles&3)!=0))
      AssertFatal (0,
		   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_sizeOfRA_PreamblesGroupA choice: 4,8,12,...,60!\n",
		   config_fname, cell_idx,eMTCconfig->ccparams.rach_sizeOfRA_PreamblesGroupA);

    switch (eMTCconfig->ccparams.rach_messageSizeGroupA) {
    case 56:
      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_messageSizeGroupA= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b56;
      break;

    case 144:
      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_messageSizeGroupA= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b144;
      break;

    case 208:
      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_messageSizeGroupA= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b208;
      break;

    case 256:
      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_messageSizeGroupA= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b256;
      break;

    default:
      AssertFatal (0,
		   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_messageSizeGroupA choice: 56,144,208,256!\n",
		   config_fname, cell_idx,eMTCconfig->ccparams.rach_messageSizeGroupA);
      break;
    }

    if (strcmp(eMTCconfig->ccparams.rach_messagePowerOffsetGroupB,"minusinfinity")==0) {
      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_minusinfinity;
    } else if (strcmp(eMTCconfig->ccparams.rach_messagePowerOffsetGroupB,"dB0")==0) {
      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB0;
    } else if (strcmp(eMTCconfig->ccparams.rach_messagePowerOffsetGroupB,"dB5")==0) {
      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB5;
    } else if (strcmp(eMTCconfig->ccparams.rach_messagePowerOffsetGroupB,"dB8")==0) {
      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB8;
    } else if (strcmp(eMTCconfig->ccparams.rach_messagePowerOffsetGroupB,"dB10")==0) {
      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB10;
    } else if (strcmp(eMTCconfig->ccparams.rach_messagePowerOffsetGroupB,"dB12")==0) {
      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB12;
    } else if (strcmp(eMTCconfig->ccparams.rach_messagePowerOffsetGroupB,"dB15")==0) {
      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB15;
    } else if (strcmp(eMTCconfig->ccparams.rach_messagePowerOffsetGroupB,"dB18")==0) {
      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB18;
    } else
      AssertFatal (0,
		   "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rach_messagePowerOffsetGroupB choice: minusinfinity,dB0,dB5,dB8,dB10,dB12,dB15,dB18!\n",
		   config_fname, cell_idx,eMTCconfig->ccparams.rach_messagePowerOffsetGroupB);
  } else if (strcmp(eMTCconfig->ccparams.rach_preamblesGroupAConfig, "DISABLE") == 0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_preamblesGroupAConfig= FALSE;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rach_preamblesGroupAConfig choice: ENABLE,DISABLE !\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.rach_preamblesGroupAConfig);

  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_preambleInitialReceivedTargetPower= (eMTCconfig->ccparams.rach_preambleInitialReceivedTargetPower+120)/2;

  if ((eMTCconfig->ccparams.rach_preambleInitialReceivedTargetPower<-120) || (eMTCconfig->ccparams.rach_preambleInitialReceivedTargetPower>-90) || ((eMTCconfig->ccparams.rach_preambleInitialReceivedTargetPower&1)!=0))
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_preambleInitialReceivedTargetPower choice: -120,-118,...,-90 !\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.rach_preambleInitialReceivedTargetPower);

  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_powerRampingStep= eMTCconfig->ccparams.rach_powerRampingStep/2;

  if ((eMTCconfig->ccparams.rach_powerRampingStep<0) || (eMTCconfig->ccparams.rach_powerRampingStep>6) || ((eMTCconfig->ccparams.rach_powerRampingStep&1)!=0))
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_powerRampingStep choice: 0,2,4,6 !\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.rach_powerRampingStep);

  switch (eMTCconfig->ccparams.rach_preambleTransMax) {
  case 3:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_preambleTransMax=  LTE_PreambleTransMax_n3;
    break;

  case 4:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_preambleTransMax=  LTE_PreambleTransMax_n4;
    break;

  case 5:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_preambleTransMax=  LTE_PreambleTransMax_n5;
    break;

  case 6:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_preambleTransMax=  LTE_PreambleTransMax_n6;
    break;

  case 7:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_preambleTransMax=  LTE_PreambleTransMax_n7;
    break;

  case 8:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_preambleTransMax=  LTE_PreambleTransMax_n8;
    break;

  case 10:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_preambleTransMax=  LTE_PreambleTransMax_n10;
    break;

  case 20:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_preambleTransMax=  LTE_PreambleTransMax_n20;
    break;

  case 50:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_preambleTransMax=  LTE_PreambleTransMax_n50;
    break;

  case 100:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_preambleTransMax=  LTE_PreambleTransMax_n100;
    break;

  case 200:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_preambleTransMax=  LTE_PreambleTransMax_n200;
    break;

  default:
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_preambleTransMax choice: 3,4,5,6,7,8,10,20,50,100,200!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.rach_preambleTransMax);
    break;
  }

  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_raResponseWindowSize=  (eMTCconfig->ccparams.rach_raResponseWindowSize==10)?7:eMTCconfig->ccparams.rach_raResponseWindowSize-2;

  if ((eMTCconfig->ccparams.rach_raResponseWindowSize<0)||(eMTCconfig->ccparams.rach_raResponseWindowSize==9)||(eMTCconfig->ccparams.rach_raResponseWindowSize>10))
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_raResponseWindowSize choice: 2,3,4,5,6,7,8,10!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.rach_raResponseWindowSize);

  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_macContentionResolutionTimer= (eMTCconfig->ccparams.rach_macContentionResolutionTimer/8)-1;

  if ((eMTCconfig->ccparams.rach_macContentionResolutionTimer<8) || (eMTCconfig->ccparams.rach_macContentionResolutionTimer>64) || ((eMTCconfig->ccparams.rach_macContentionResolutionTimer&7)!=0))
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_macContentionResolutionTimer choice: 8,16,...,56,64!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.rach_macContentionResolutionTimer);

  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].rach_maxHARQ_Msg3Tx= eMTCconfig->ccparams.rach_maxHARQ_Msg3Tx;

  if ((eMTCconfig->ccparams.rach_maxHARQ_Msg3Tx<0) || (eMTCconfig->ccparams.rach_maxHARQ_Msg3Tx>8))
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_maxHARQ_Msg3Tx choice: 1..8!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.rach_maxHARQ_Msg3Tx);

  switch (eMTCconfig->preambleTransMax_CE_r13) {
  case 3:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].preambleTransMax_CE_r13=  LTE_PreambleTransMax_n3;
    break;

  case 4:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].preambleTransMax_CE_r13=  LTE_PreambleTransMax_n4;
    break;

  case 5:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].preambleTransMax_CE_r13=  LTE_PreambleTransMax_n5;
    break;

  case 6:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].preambleTransMax_CE_r13=  LTE_PreambleTransMax_n6;
    break;

  case 7:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].preambleTransMax_CE_r13=  LTE_PreambleTransMax_n7;
    break;

  case 8:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].preambleTransMax_CE_r13=  LTE_PreambleTransMax_n8;
    break;

  case 10:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].preambleTransMax_CE_r13=  LTE_PreambleTransMax_n10;
    break;

  case 20:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].preambleTransMax_CE_r13=  LTE_PreambleTransMax_n20;
    break;

  case 50:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].preambleTransMax_CE_r13=  LTE_PreambleTransMax_n50;
    break;

  case 100:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].preambleTransMax_CE_r13=  LTE_PreambleTransMax_n100;
    break;

  case 200:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].preambleTransMax_CE_r13=  LTE_PreambleTransMax_n200;
    break;

  default:
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_preambleTransMax_CE_r13 choice: 3,4,5,6,7,8,10,20,50,100,200!\n",
		 config_fname, cell_idx,eMTCconfig->preambleTransMax_CE_r13);
    break;
  }

  switch (eMTCconfig->ccparams.pcch_defaultPagingCycle) {
  case 32:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pcch_defaultPagingCycle= LTE_PCCH_Config__defaultPagingCycle_rf32;
    break;

  case 64:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pcch_defaultPagingCycle= LTE_PCCH_Config__defaultPagingCycle_rf64;
    break;

  case 128:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pcch_defaultPagingCycle= LTE_PCCH_Config__defaultPagingCycle_rf128;
    break;

  case 256:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pcch_defaultPagingCycle= LTE_PCCH_Config__defaultPagingCycle_rf256;
    break;

  default:
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pcch_defaultPagingCycle choice: 32,64,128,256!\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pcch_defaultPagingCycle);
    break;
  }

  if (strcmp(eMTCconfig->ccparams.pcch_nB, "fourT") == 0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pcch_nB= LTE_PCCH_Config__nB_fourT;
  } else if (strcmp(eMTCconfig->ccparams.pcch_nB, "twoT") == 0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pcch_nB= LTE_PCCH_Config__nB_twoT;
  } else if (strcmp(eMTCconfig->ccparams.pcch_nB, "oneT") == 0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pcch_nB= LTE_PCCH_Config__nB_oneT;
  } else if (strcmp(eMTCconfig->ccparams.pcch_nB, "halfT") == 0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pcch_nB= LTE_PCCH_Config__nB_halfT;
  } else if (strcmp(eMTCconfig->ccparams.pcch_nB, "quarterT") == 0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pcch_nB= LTE_PCCH_Config__nB_quarterT;
  } else if (strcmp(eMTCconfig->ccparams.pcch_nB, "oneEighthT") == 0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pcch_nB= LTE_PCCH_Config__nB_oneEighthT;
  } else if (strcmp(eMTCconfig->ccparams.pcch_nB, "oneSixteenthT") == 0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pcch_nB= LTE_PCCH_Config__nB_oneSixteenthT;
  } else if (strcmp(eMTCconfig->ccparams.pcch_nB, "oneThirtySecondT") == 0) {
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].pcch_nB= LTE_PCCH_Config__nB_oneThirtySecondT;
  } else
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pcch_nB choice: fourT,twoT,oneT,halfT,quarterT,oneighthT,oneSixteenthT,oneThirtySecondT !\n",
		 config_fname, cell_idx,eMTCconfig->ccparams.pcch_nB);

  switch (eMTCconfig->ccparams.bcch_modificationPeriodCoeff) {
  case 2:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].bcch_modificationPeriodCoeff= LTE_BCCH_Config__modificationPeriodCoeff_n2;
    break;

  case 4:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].bcch_modificationPeriodCoeff= LTE_BCCH_Config__modificationPeriodCoeff_n4;
    break;

  case 8:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].bcch_modificationPeriodCoeff= LTE_BCCH_Config__modificationPeriodCoeff_n8;
    break;

  case 16:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].bcch_modificationPeriodCoeff= LTE_BCCH_Config__modificationPeriodCoeff_n16;
    break;

  default:
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for bcch_modificationPeriodCoeff choice: 2,4,8,16",
		 config_fname, cell_idx,eMTCconfig->ccparams.bcch_modificationPeriodCoeff);
    break;
  }

  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].ue_TimersAndConstants_t300= eMTCconfig->ccparams.ue_TimersAndConstants_t300;
  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].ue_TimersAndConstants_t301= eMTCconfig->ccparams.ue_TimersAndConstants_t301;
  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].ue_TimersAndConstants_t310= eMTCconfig->ccparams.ue_TimersAndConstants_t310;
  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].ue_TimersAndConstants_t311= eMTCconfig->ccparams.ue_TimersAndConstants_t311;
  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].ue_TimersAndConstants_n310= eMTCconfig->ccparams.ue_TimersAndConstants_n310;
  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].ue_TimersAndConstants_n311= eMTCconfig->ccparams.ue_TimersAndConstants_n311;

  switch (eMTCconfig->ccparams.ue_TransmissionMode) {
  case 1:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].ue_TransmissionMode= LTE_AntennaInfoDedicated__transmissionMode_tm1;
    break;

  case 2:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].ue_TransmissionMode= LTE_AntennaInfoDedicated__transmissionMode_tm2;
    break;

  case 3:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].ue_TransmissionMode= LTE_AntennaInfoDedicated__transmissionMode_tm3;
    break;

  case 4:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].ue_TransmissionMode= LTE_AntennaInfoDedicated__transmissionMode_tm4;
    break;

  case 5:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].ue_TransmissionMode= LTE_AntennaInfoDedicated__transmissionMode_tm5;
    break;

  case 6:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].ue_TransmissionMode= LTE_AntennaInfoDedicated__transmissionMode_tm6;
    break;

  case 7:
    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[cc_idx].ue_TransmissionMode= LTE_AntennaInfoDedicated__transmissionMode_tm7;
    break;

  default:
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TransmissionMode choice: 1,2,3,4,5,6,7",
		 config_fname, cell_idx, eMTCconfig->ccparams.ue_TransmissionMode);
    break;
  }


  if (!strcmp(eMTCconfig->prach_ConfigCommon_v1310, "ENABLE")) {
    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[cc_idx].prach_ConfigCommon_v1310 = TRUE;

    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[cc_idx].mpdcch_startSF_CSS_RA_r13 = calloc(1, sizeof(BOOLEAN_t));

    if (!strcmp(eMTCconfig->mpdcch_startSF_CSS_RA_r13, "tdd-r13")) {
      *RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[cc_idx].mpdcch_startSF_CSS_RA_r13 = FALSE;
    } else {
      *RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[cc_idx].mpdcch_startSF_CSS_RA_r13 = TRUE;
    }

    if (!strcmp(eMTCconfig->mpdcch_startSF_CSS_RA_r13_val, "v1")) {
      RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[cc_idx].mpdcch_startSF_CSS_RA_r13_val = 0;
    } else if (!strcmp(eMTCconfig->mpdcch_startSF_CSS_RA_r13_val, "v1dot5")) {
      RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[cc_idx].mpdcch_startSF_CSS_RA_r13_val = 1;
    } else if (!strcmp(eMTCconfig->mpdcch_startSF_CSS_RA_r13_val, "v2")) {
      RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[cc_idx].mpdcch_startSF_CSS_RA_r13_val = 2;
    } else if (!strcmp(eMTCconfig->mpdcch_startSF_CSS_RA_r13_val, "v2dot5")) {
      RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[cc_idx].mpdcch_startSF_CSS_RA_r13_val = 3;
    } else if (!strcmp(eMTCconfig->mpdcch_startSF_CSS_RA_r13_val, "v4")) {
      RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[cc_idx].mpdcch_startSF_CSS_RA_r13_val = 4;
    } else if (!strcmp(eMTCconfig->mpdcch_startSF_CSS_RA_r13_val, "v5")) {
      RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[cc_idx].mpdcch_startSF_CSS_RA_r13_val = 5;
    } else if (!strcmp(eMTCconfig->mpdcch_startSF_CSS_RA_r13_val, "v8")) {
      RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[cc_idx].mpdcch_startSF_CSS_RA_r13_val = 6;
    } else if (!strcmp(eMTCconfig->mpdcch_startSF_CSS_RA_r13_val, "10")) {
      RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[cc_idx].mpdcch_startSF_CSS_RA_r13_val = 7;
    } else {
      AssertFatal(0,
		  "Failed to parse eNB configuration file %s, enb %d mpdcch_startSF_CSS_RA_r13_val! Unknown Value !!\n",
		  config_fname, cell_idx);
    }

    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[cc_idx].prach_HoppingOffset_r13 = calloc(1, sizeof(long));
    *RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[cc_idx].prach_HoppingOffset_r13 = eMTCconfig->prach_HoppingOffset_r13;
  } else {
    RRC_CONFIGURATION_REQ(msg_p).radioresourceconfig_BR[cc_idx].prach_ConfigCommon_v1310 = FALSE;
  }


  RRC_CONFIGURATION_REQ(msg_p).pdsch_maxNumRepetitionCEmodeA_r13[cc_idx] = CALLOC(1, sizeof(long));
  if (!strcmp(eMTCconfig->pdsch_maxNumRepetitionCEmodeA_r13, "r16")) {
    *RRC_CONFIGURATION_REQ(msg_p).pdsch_maxNumRepetitionCEmodeA_r13[cc_idx] = 0;
  } else if (!strcmp(eMTCconfig->pdsch_maxNumRepetitionCEmodeA_r13, "r32")) {
    *RRC_CONFIGURATION_REQ(msg_p).pdsch_maxNumRepetitionCEmodeA_r13[cc_idx] = 1;
  } else {
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, pdsch_maxNumRepetitionCEmodeA_r13 unknown value!\n",
		 config_fname);
  }


  RRC_CONFIGURATION_REQ(msg_p).pusch_maxNumRepetitionCEmodeA_r13[cc_idx] = CALLOC(1, sizeof(long));
  if (!strcmp(eMTCconfig->pusch_maxNumRepetitionCEmodeA_r13, "r8")) {
    *RRC_CONFIGURATION_REQ(msg_p).pusch_maxNumRepetitionCEmodeA_r13[cc_idx] =  0;
  } else if (!strcmp(eMTCconfig->pusch_maxNumRepetitionCEmodeA_r13, "r16")) {
    *RRC_CONFIGURATION_REQ(msg_p).pusch_maxNumRepetitionCEmodeA_r13[cc_idx] =  1;
  } else if (!strcmp(eMTCconfig->pusch_maxNumRepetitionCEmodeA_r13, "r32")) {
    *RRC_CONFIGURATION_REQ(msg_p).pusch_maxNumRepetitionCEmodeA_r13[cc_idx] =  2;
  } else {
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, pusch_maxNumRepetitionCEmodeA_r13 unknown value!\n",
		 config_fname);
  }

  RRC_CONFIGURATION_REQ(msg_p).pusch_repetitionLevelCEmodeA_r13[cc_idx] = CALLOC(1, sizeof(long));
  if (!strcmp(eMTCconfig->pusch_repetitionLevelCEmodeA_r13, "l1")) {
    *RRC_CONFIGURATION_REQ(msg_p).pusch_repetitionLevelCEmodeA_r13[cc_idx] =  0;
  } else if (!strcmp(eMTCconfig->pusch_repetitionLevelCEmodeA_r13, "l2")) {
    *RRC_CONFIGURATION_REQ(msg_p).pusch_repetitionLevelCEmodeA_r13[cc_idx] =  1;
  } else if (!strcmp(eMTCconfig->pusch_repetitionLevelCEmodeA_r13, "l3")) {
    *RRC_CONFIGURATION_REQ(msg_p).pusch_repetitionLevelCEmodeA_r13[cc_idx] =  2;
  } else if (!strcmp(eMTCconfig->pusch_repetitionLevelCEmodeA_r13, "l4")) {
    *RRC_CONFIGURATION_REQ(msg_p).pusch_repetitionLevelCEmodeA_r13[cc_idx] =  3;
  } else {
    AssertFatal (0,
    "Failed to parse eNB configuration file %s, pusch_repetitionLevelCEmodeA_r13 unknown value!\n",
    config_fname);
  }

  char rachCELevelInfoListPath[MAX_OPTNAME_SIZE * 2];
  config_getlist(&rachcelevellist, NULL, 0, brparamspath);
  RRC_CONFIGURATION_REQ (msg_p).rach_CE_LevelInfoList_r13_size[cc_idx] = rachcelevellist.numelt;
  int rachCEInfoIndex;
  for (rachCEInfoIndex = 0; rachCEInfoIndex < rachcelevellist.numelt; rachCEInfoIndex++) {
    sprintf(rachCELevelInfoListPath, "%s.%s.[%i]", brparamspath, ENB_CONFIG_STRING_RACH_CE_LEVELINFOLIST_R13, rachCEInfoIndex);
    config_get(rachcelevelParams, sizeof(rachcelevelParams) / sizeof(paramdef_t), rachCELevelInfoListPath);

    RRC_CONFIGURATION_REQ (msg_p).firstPreamble_r13[cc_idx][rachCEInfoIndex] = eMTCconfig->firstPreamble_r13;
    RRC_CONFIGURATION_REQ (msg_p).lastPreamble_r13[cc_idx][rachCEInfoIndex]  = eMTCconfig->lastPreamble_r13;

    switch (eMTCconfig->ra_ResponseWindowSize_r13) {
    case 20:
      RRC_CONFIGURATION_REQ (msg_p).ra_ResponseWindowSize_r13[cc_idx][rachCEInfoIndex] = LTE_RACH_CE_LevelInfo_r13__ra_ResponseWindowSize_r13_sf20;
      break;
    case 50:
      RRC_CONFIGURATION_REQ (msg_p).ra_ResponseWindowSize_r13[cc_idx][rachCEInfoIndex] = LTE_RACH_CE_LevelInfo_r13__ra_ResponseWindowSize_r13_sf50;
      break;
    case 80:
      RRC_CONFIGURATION_REQ (msg_p).ra_ResponseWindowSize_r13[cc_idx][rachCEInfoIndex] = LTE_RACH_CE_LevelInfo_r13__ra_ResponseWindowSize_r13_sf80;
      break;
    case 120:
      RRC_CONFIGURATION_REQ (msg_p).ra_ResponseWindowSize_r13[cc_idx][rachCEInfoIndex] = LTE_RACH_CE_LevelInfo_r13__ra_ResponseWindowSize_r13_sf120;
      break;
    case 180:
      RRC_CONFIGURATION_REQ (msg_p).ra_ResponseWindowSize_r13[cc_idx][rachCEInfoIndex] = LTE_RACH_CE_LevelInfo_r13__ra_ResponseWindowSize_r13_sf180;
      break;
    case 240:
      RRC_CONFIGURATION_REQ (msg_p).ra_ResponseWindowSize_r13[cc_idx][rachCEInfoIndex] = LTE_RACH_CE_LevelInfo_r13__ra_ResponseWindowSize_r13_sf240;
      break;
    case 320:
      RRC_CONFIGURATION_REQ (msg_p).ra_ResponseWindowSize_r13[cc_idx][rachCEInfoIndex] = LTE_RACH_CE_LevelInfo_r13__ra_ResponseWindowSize_r13_sf320;
      break;
    case 400:
      RRC_CONFIGURATION_REQ (msg_p).ra_ResponseWindowSize_r13[cc_idx][rachCEInfoIndex] = LTE_RACH_CE_LevelInfo_r13__ra_ResponseWindowSize_r13_sf400;
      break;
    default:
      AssertFatal(1==0,
		"Illegal ra_ResponseWindowSize_r13 %d\n",eMTCconfig->ra_ResponseWindowSize_r13);
    }


    switch(eMTCconfig->mac_ContentionResolutionTimer_r13) {
    case 80:
      RRC_CONFIGURATION_REQ (msg_p).mac_ContentionResolutionTimer_r13[cc_idx][rachCEInfoIndex] = LTE_RACH_CE_LevelInfo_r13__mac_ContentionResolutionTimer_r13_sf80;
      break;
    case 100:
      RRC_CONFIGURATION_REQ (msg_p).mac_ContentionResolutionTimer_r13[cc_idx][rachCEInfoIndex] = LTE_RACH_CE_LevelInfo_r13__mac_ContentionResolutionTimer_r13_sf100;
      break;
    case 120:
      RRC_CONFIGURATION_REQ (msg_p).mac_ContentionResolutionTimer_r13[cc_idx][rachCEInfoIndex] = LTE_RACH_CE_LevelInfo_r13__mac_ContentionResolutionTimer_r13_sf120;
      break;
    case 160:
      RRC_CONFIGURATION_REQ (msg_p).mac_ContentionResolutionTimer_r13[cc_idx][rachCEInfoIndex] = LTE_RACH_CE_LevelInfo_r13__mac_ContentionResolutionTimer_r13_sf160;
      break;
    case 200:
      RRC_CONFIGURATION_REQ (msg_p).mac_ContentionResolutionTimer_r13[cc_idx][rachCEInfoIndex] = LTE_RACH_CE_LevelInfo_r13__mac_ContentionResolutionTimer_r13_sf200;
      break;
    case 240:
      RRC_CONFIGURATION_REQ (msg_p).mac_ContentionResolutionTimer_r13[cc_idx][rachCEInfoIndex] = LTE_RACH_CE_LevelInfo_r13__mac_ContentionResolutionTimer_r13_sf240;
      break;
    case 480:
      RRC_CONFIGURATION_REQ (msg_p).mac_ContentionResolutionTimer_r13[cc_idx][rachCEInfoIndex] = LTE_RACH_CE_LevelInfo_r13__mac_ContentionResolutionTimer_r13_sf480;
      break;
    case 960:
      RRC_CONFIGURATION_REQ (msg_p).mac_ContentionResolutionTimer_r13[cc_idx][rachCEInfoIndex] = LTE_RACH_CE_LevelInfo_r13__mac_ContentionResolutionTimer_r13_sf960;
      break;
    default:
      AssertFatal(1==0,"Illegal mac_ContentionResolutionTimer_r13 %d\n",
		  eMTCconfig->mac_ContentionResolutionTimer_r13);
      break;
    }
    RRC_CONFIGURATION_REQ (msg_p).rar_HoppingConfig_r13[cc_idx][rachCEInfoIndex] = eMTCconfig->rar_HoppingConfig_r13;

    AssertFatal(eMTCconfig->rar_HoppingConfig_r13 == 1 ,
		            "illegal rar_HoppingConfig_r13 %d (should be 1 only for now, can be 0 when RAR frequency hopping is supported\n",eMTCconfig->rar_HoppingConfig_r13);
  } // end for loop (rach ce level info)

  char rsrpRangeListPath[MAX_OPTNAME_SIZE * 2];
  config_getlist(&rsrprangelist, NULL, 0, brparamspath);
  RRC_CONFIGURATION_REQ (msg_p).rsrp_range_list_size[cc_idx] = rsrprangelist.numelt;


  int rsrprangeindex;
  for (rsrprangeindex = 0; rsrprangeindex < rsrprangelist.numelt; rsrprangeindex++) {
    sprintf(rsrpRangeListPath, "%s.%s.[%i]", brparamspath, ENB_CONFIG_STRING_RSRP_RANGE_LIST, rsrprangeindex);
    config_get(rsrprangeParams, sizeof(rsrprangeParams) / sizeof(paramdef_t), rsrpRangeListPath);
    RRC_CONFIGURATION_REQ (msg_p).rsrp_range[cc_idx][rsrprangeindex] = eMTCconfig->rsrp_range_br;

  }


  char prachparameterscePath[MAX_OPTNAME_SIZE * 2];
  config_getlist(&prachParamslist, NULL, 0, brparamspath);
  RRC_CONFIGURATION_REQ (msg_p).prach_parameters_list_size[cc_idx] = prachParamslist.numelt;

  int prachparamsindex;
  for (prachparamsindex = 0; prachparamsindex < prachParamslist.numelt; prachparamsindex++) {
    sprintf(prachparameterscePath, "%s.%s.[%i]", brparamspath, ENB_CONFIG_STRING_PRACH_PARAMETERS_CE_R13, prachparamsindex);
    config_get(prachParams, sizeof(prachParams) / sizeof(paramdef_t), prachparameterscePath);

    RRC_CONFIGURATION_REQ (msg_p).prach_config_index[cc_idx][prachparamsindex]                  = eMTCconfig->prach_config_index_br;
    RRC_CONFIGURATION_REQ (msg_p).prach_freq_offset[cc_idx][prachparamsindex]                   = eMTCconfig->prach_freq_offset_br;

    RRC_CONFIGURATION_REQ (msg_p).prach_StartingSubframe_r13[cc_idx][prachparamsindex] = calloc(1, sizeof(long));
    switch(eMTCconfig->prach_StartingSubframe_r13) {
    case 2:
      *RRC_CONFIGURATION_REQ (msg_p).prach_StartingSubframe_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__prach_StartingSubframe_r13_sf2;
      break;
    case 4:
      *RRC_CONFIGURATION_REQ (msg_p).prach_StartingSubframe_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__prach_StartingSubframe_r13_sf4;
      break;
    case 8:
      *RRC_CONFIGURATION_REQ (msg_p).prach_StartingSubframe_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__prach_StartingSubframe_r13_sf8;
      break;
    case 16:
      *RRC_CONFIGURATION_REQ (msg_p).prach_StartingSubframe_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__prach_StartingSubframe_r13_sf16;
      break;
    case 32:
      *RRC_CONFIGURATION_REQ (msg_p).prach_StartingSubframe_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__prach_StartingSubframe_r13_sf32;
      break;
    case 64:
      *RRC_CONFIGURATION_REQ (msg_p).prach_StartingSubframe_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__prach_StartingSubframe_r13_sf64;
      break;
    case 128:
      *RRC_CONFIGURATION_REQ (msg_p).prach_StartingSubframe_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__prach_StartingSubframe_r13_sf128;
      break;
    case 256:
      *RRC_CONFIGURATION_REQ (msg_p).prach_StartingSubframe_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__prach_StartingSubframe_r13_sf256;
      break;
    default:
      AssertFatal(1==0,"prach_StartingSubframe_r13 %d is illegal\n",
		  eMTCconfig->prach_StartingSubframe_r13);
      break;
    }

    RRC_CONFIGURATION_REQ (msg_p).maxNumPreambleAttemptCE_r13[cc_idx][prachparamsindex] = calloc(1, sizeof(long));
    if (eMTCconfig->maxNumPreambleAttemptCE_r13==10)  *RRC_CONFIGURATION_REQ (msg_p).maxNumPreambleAttemptCE_r13[cc_idx][prachparamsindex] = 6;
    else                                              *RRC_CONFIGURATION_REQ (msg_p).maxNumPreambleAttemptCE_r13[cc_idx][prachparamsindex] = eMTCconfig->maxNumPreambleAttemptCE_r13-3;
    AssertFatal(eMTCconfig->maxNumPreambleAttemptCE_r13 > 2 && eMTCconfig->maxNumPreambleAttemptCE_r13 <11,
		"prachparamsindex %d: Illegal maxNumPreambleAttemptCE_r13 %d\n",
		prachparamsindex,eMTCconfig->maxNumPreambleAttemptCE_r13);


    switch(eMTCconfig->numRepetitionPerPreambleAttempt_r13) {
    case 1:
      RRC_CONFIGURATION_REQ (msg_p).numRepetitionPerPreambleAttempt_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__numRepetitionPerPreambleAttempt_r13_n1;
      break;
    case 2:
      RRC_CONFIGURATION_REQ (msg_p).numRepetitionPerPreambleAttempt_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__numRepetitionPerPreambleAttempt_r13_n2;
      break;
    case 4:
      RRC_CONFIGURATION_REQ (msg_p).numRepetitionPerPreambleAttempt_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__numRepetitionPerPreambleAttempt_r13_n4;
      break;
    case 8:
      RRC_CONFIGURATION_REQ (msg_p).numRepetitionPerPreambleAttempt_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__numRepetitionPerPreambleAttempt_r13_n8;
      break;
    case 16:
      RRC_CONFIGURATION_REQ (msg_p).numRepetitionPerPreambleAttempt_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__numRepetitionPerPreambleAttempt_r13_n16;
      break;
    case 32:
      RRC_CONFIGURATION_REQ (msg_p).numRepetitionPerPreambleAttempt_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__numRepetitionPerPreambleAttempt_r13_n32;
      break;
    case 64:
      RRC_CONFIGURATION_REQ (msg_p).numRepetitionPerPreambleAttempt_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__numRepetitionPerPreambleAttempt_r13_n64;
      break;
    case 128:
      RRC_CONFIGURATION_REQ (msg_p).numRepetitionPerPreambleAttempt_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__numRepetitionPerPreambleAttempt_r13_n128;
      break;
    default:
      AssertFatal(1==0,
		  "illegal numReptitionPerPreambleAttempt %d\n",
		  eMTCconfig->numRepetitionPerPreambleAttempt_r13);
      break;
    }
    switch (eMTCconfig->mpdcch_NumRepetition_RA_r13) {
    case 1:
      RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__mpdcch_NumRepetition_RA_r13_r1;
	break;
    case 2:
      RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__mpdcch_NumRepetition_RA_r13_r2;
	break;
    case 4:
      RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__mpdcch_NumRepetition_RA_r13_r4;
	break;
    case 8:
      RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__mpdcch_NumRepetition_RA_r13_r8;
	break;
    case 16:
      RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__mpdcch_NumRepetition_RA_r13_r16;
	break;
    case 32:
      RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__mpdcch_NumRepetition_RA_r13_r32;
	break;
    case 64:
      RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__mpdcch_NumRepetition_RA_r13_r64;
	break;
    case 128:
      RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__mpdcch_NumRepetition_RA_r13_r128;
	break;
    case 256:
      RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[cc_idx][prachparamsindex] = LTE_PRACH_ParametersCE_r13__mpdcch_NumRepetition_RA_r13_r256;
	break;
    default:
      AssertFatal (1==0,
		 "illegal mpdcch_NumRepeition_RA_r13 %d\n",
		   eMTCconfig->mpdcch_NumRepetition_RA_r13);
      break;
    }

    RRC_CONFIGURATION_REQ (msg_p).prach_HoppingConfig_r13[cc_idx][prachparamsindex] = eMTCconfig->prach_HoppingConfig_r13;

    AssertFatal (eMTCconfig->prach_HoppingConfig_r13 >=0 && eMTCconfig->prach_HoppingConfig_r13 < 2,
		 "Illegal prach_HoppingConfig_r13 %d\n",eMTCconfig->prach_HoppingConfig_r13);


    int maxavailablenarrowband_count = prachParams[7].numelt;

    RRC_CONFIGURATION_REQ (msg_p).max_available_narrow_band_size[cc_idx][prachparamsindex] = maxavailablenarrowband_count;
    int narrow_band_index;
    for (narrow_band_index = 0; narrow_band_index < maxavailablenarrowband_count; narrow_band_index++) {
      RRC_CONFIGURATION_REQ (msg_p).max_available_narrow_band[cc_idx][prachparamsindex][narrow_band_index] = prachParams[7].iptr[narrow_band_index];
    }
  }

  char n1PUCCHInfoParamsPath[MAX_OPTNAME_SIZE * 2];
  config_getlist(&n1PUCCHInfoList, NULL, 0, brparamspath);
  RRC_CONFIGURATION_REQ (msg_p).pucch_info_value_size[cc_idx] = n1PUCCHInfoList.numelt;

  int n1PUCCHinfolistindex;
  for (n1PUCCHinfolistindex = 0; n1PUCCHinfolistindex < n1PUCCHInfoList.numelt; n1PUCCHinfolistindex++) {
    sprintf(n1PUCCHInfoParamsPath, "%s.%s.[%i]", brparamspath, ENB_CONFIG_STRING_N1PUCCH_AN_INFOLIST_R13, n1PUCCHinfolistindex);
    config_get(n1PUCCH_ANR13Params, sizeof(n1PUCCH_ANR13Params) / sizeof(paramdef_t), n1PUCCHInfoParamsPath);
    RRC_CONFIGURATION_REQ (msg_p).pucch_info_value[cc_idx][n1PUCCHinfolistindex] = eMTCconfig->pucch_info_value;
  }

  char PCCHConfigv1310Path[MAX_OPTNAME_SIZE*2 + 16];
  sprintf(PCCHConfigv1310Path, "%s.%s", brparamspath, ENB_CONFIG_STRING_PCCH_CONFIG_V1310);
  config_get(pcchv1310Params, sizeof(pcchv1310Params)/sizeof(paramdef_t), PCCHConfigv1310Path);



  /** PCCH CONFIG V1310 */
  RRC_CONFIGURATION_REQ(msg_p).pcch_config_v1310[cc_idx] = TRUE;
  RRC_CONFIGURATION_REQ(msg_p).paging_narrowbands_r13[cc_idx] = eMTCconfig->paging_narrowbands_r13;
  RRC_CONFIGURATION_REQ(msg_p).mpdcch_numrepetition_paging_r13[cc_idx] = eMTCconfig->mpdcch_numrepetition_paging_r13;

  AssertFatal (eMTCconfig->mpdcch_numrepetition_paging_r13 == 0 ||
          eMTCconfig->mpdcch_numrepetition_paging_r13 == 1 ||
	       eMTCconfig->mpdcch_numrepetition_paging_r13 == 2 ||
	       eMTCconfig->mpdcch_numrepetition_paging_r13 == 4 ||
	       eMTCconfig->mpdcch_numrepetition_paging_r13 == 8 ||
	       eMTCconfig->mpdcch_numrepetition_paging_r13 == 16 ||
	       eMTCconfig->mpdcch_numrepetition_paging_r13 == 32 ||
	       eMTCconfig->mpdcch_numrepetition_paging_r13 == 64 ||
	       eMTCconfig->mpdcch_numrepetition_paging_r13 == 128 ||
	       eMTCconfig->mpdcch_numrepetition_paging_r13 == 256,
	       "illegal mpdcch_numrepetition_paging_r13 %d\n",
	       eMTCconfig->mpdcch_numrepetition_paging_r13);

  //                        RRC_CONFIGURATION_REQ(msg_p).nb_v1310[cc_idx] = CALLOC(1, sizeof(long));
  //                        if (!strcmp(nb_v1310, "one64thT")) {
  //                            *RRC_CONFIGURATION_REQ(msg_p).nb_v1310[cc_idx] = 0;
  //                        } else if (!strcmp(nb_v1310, "one128thT")) {
  //                            *RRC_CONFIGURATION_REQ(msg_p).nb_v1310[cc_idx] = 1;
  //                        } else if (!strcmp(nb_v1310, "one256thT")) {
  //                            *RRC_CONFIGURATION_REQ(msg_p).nb_v1310[cc_idx] = 2;
  //                        } else {
  //                            AssertFatal(0,
  //                                        "Failed to parse eNB configuration file %s, nb_v1310, unknown value !\n",
  //                                        config_fname);
  //                        }



  RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level0_r13[cc_idx] = CALLOC(1, sizeof(long));
  // ++cnt; // check this ,, the conter is up above
  if (!strcmp(eMTCconfig->pucch_NumRepetitionCE_Msg4_Level0_r13, "n1")) {
    *RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level0_r13[cc_idx] = 0;
  } else if (!strcmp(eMTCconfig->pucch_NumRepetitionCE_Msg4_Level0_r13, "n2")) {
    *RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level0_r13[cc_idx] = 1;
  } else if (!strcmp(eMTCconfig->pucch_NumRepetitionCE_Msg4_Level0_r13, "n4")) {
    *RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level0_r13[cc_idx] = 2;
  } else if (!strcmp(eMTCconfig->pucch_NumRepetitionCE_Msg4_Level0_r13, "n8")) {
    *RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level0_r13[cc_idx] = 3;
  } else {
    AssertFatal (0,
		 "Failed to parse eNB configuration file %s, pucch_NumRepetitionCE_Msg4_Level0_r13 unknown value!\n",
		 config_fname);
  }



  /** SIB2 FREQ HOPPING PARAMETERS R13 */
  RRC_CONFIGURATION_REQ(msg_p).sib2_freq_hoppingParameters_r13_exists[cc_idx] = TRUE;

  char sib2FreqHoppingParametersR13Path[MAX_OPTNAME_SIZE*2 + 16];
  sprintf(sib2FreqHoppingParametersR13Path, "%s.%s", brparamspath, ENB_CONFIG_STRING_SIB2_FREQ_HOPPINGPARAMETERS_R13);
  config_get(sib2freqhoppingParams, sizeof(sib2freqhoppingParams)/sizeof(paramdef_t), sib2FreqHoppingParametersR13Path);


  RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13[cc_idx] = CALLOC(1, sizeof(long));
  if (!strcmp(eMTCconfig->sib2_interval_ULHoppingConfigCommonModeA_r13, "FDD")) {
    *RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13[cc_idx] = 0;

    switch(eMTCconfig->sib2_interval_ULHoppingConfigCommonModeA_r13_val) {
    case 1:
      RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13_val[cc_idx] = 0;
      break;
    case 2:
      RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13_val[cc_idx] = 1;
      break;
    case 4:
      RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13_val[cc_idx] = 2;
      break;
    case 8:
      RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13_val[cc_idx] = 3;
      break;
    default:
      AssertFatal(1==0,
		  "illegal sib2_interval_ULHoppingConfigCommonModeA_r13_val %d\n",
		  eMTCconfig->sib2_interval_ULHoppingConfigCommonModeA_r13_val);
    }
  } else if (!strcmp(eMTCconfig->sib2_interval_ULHoppingConfigCommonModeA_r13, "TDD")) {
    *RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13[cc_idx] = 1;
    switch(eMTCconfig->sib2_interval_ULHoppingConfigCommonModeA_r13_val) {
    case 1:
      RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13_val[cc_idx] = 0;
      break;
    case 5:
      RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13_val[cc_idx] = 1;
      break;
    case 10:
      RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13_val[cc_idx] = 2;
      break;
    case 20:
      RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13_val[cc_idx] = 3;
      break;
    default:
      AssertFatal(1==0,
		  "illegal sib2_interval_ULHoppingConfigCommonModeA_r13_val %d\n",
		  eMTCconfig->sib2_interval_ULHoppingConfigCommonModeA_r13_val);
      break;
    }
  } else {
    AssertFatal (1==0,
		 "Failed to parse eNB configuration file %s, sib2_interval_ULHoppingConfigCommonModeA_r13 unknown value !!\n",
		 config_fname);
  }
}
