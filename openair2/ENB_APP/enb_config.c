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
  enb_config.c
  -------------------
  AUTHOR  : Lionel GAUTHIER, navid nikaein, Laurent Winckel
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr, navid.nikaein@eurecom.fr
*/

#include <string.h>
#include <inttypes.h>

#include "common/utils/LOG/log.h"
#include "assertions.h"
#include "enb_config.h"
#include "UTIL/OTG/otg.h"
#include "UTIL/OTG/otg_externs.h"
#if defined(ENABLE_ITTI)
#include "intertask_interface.h"
#if defined(ENABLE_USE_MME)
#include "s1ap_eNB.h"
#include "sctp_eNB_task.h"
#else
#define EPC_MODE_ENABLED 0
#endif
#endif
#include "sctp_default_values.h"
#include "SystemInformationBlockType2.h"
#include "LAYER2/MAC/mac_extern.h"
#include "LAYER2/MAC/mac_proto.h"
#include "PHY/phy_extern.h"
#include "PHY/INIT/phy_init.h"
#include "targets/ARCH/ETHERNET/USERSPACE/LIB/ethernet_lib.h"
#include "nfapi_vnf.h"
#include "nfapi_pnf.h"

#include "L1_paramdef.h"
#include "MACRLC_paramdef.h"
#include "common/config/config_userapi.h"
#include "RRC_config_tools.h"
#include "enb_paramdef.h"

extern uint16_t sf_ahead;
extern void set_parallel_conf(char *parallel_conf);
extern void set_worker_conf(char *worker_conf);
extern PARALLEL_CONF_t get_thread_parallel_conf(void);
extern WORKER_CONF_t   get_thread_worker_conf(void);
extern uint32_t to_earfcn_DL(int eutra_bandP, uint32_t dl_CarrierFreq, uint32_t bw);
extern uint32_t to_earfcn_UL(int eutra_bandP, uint32_t ul_CarrierFreq, uint32_t bw);

void RCconfig_flexran() {
  uint16_t i;
  uint16_t num_enbs;
  char aprefix[MAX_OPTNAME_SIZE*2 + 8];
  /* this will possibly truncate the cell id (RRC assumes int32_t).
     Both Nid_cell and enb_id are signed in RRC case, but we use unsigned for
     the bitshifting to work properly */
  int32_t Nid_cell = 0;
  uint16_t Nid_cell_tr = 0;
  uint32_t enb_id = 0;
  /*
    the only reason for all these variables is, that they are "hard-encoded"
    into the CCPARAMS_DESC macro and we need it for the Nid_cell variable ...
  */
  char *frame_type, *prefix_type, *pbch_repetition, *prach_high_speed,
       *pusch_hoppingMode, *pusch_enable64QAM, *pusch_groupHoppingEnabled,
       *pusch_sequenceHoppingEnabled, *phich_duration, *phich_resource,
       *srs_enable, *srs_ackNackST, *srs_MaxUpPts, *pusch_alpha,
       *pucch_deltaF_Format1, *pucch_deltaF_Format1b, *pucch_deltaF_Format2,
       *pucch_deltaF_Format2a, *pucch_deltaF_Format2b,
       *rach_preamblesGroupAConfig, *rach_messagePowerOffsetGroupB, *pcch_nB;
  long long int     downlink_frequency;
  int32_t tdd_config, tdd_config_s, eutra_band, uplink_frequency_offset,
          Nid_cell_mbsfn, N_RB_DL, nb_antenna_ports, prach_root, prach_config_index,
          prach_zero_correlation, prach_freq_offset, pucch_delta_shift,
          pucch_nRB_CQI, pucch_nCS_AN, pucch_n1_AN, pdsch_referenceSignalPower,
          pdsch_p_b, pusch_n_SB, pusch_hoppingOffset, pusch_groupAssignment,
          pusch_nDMRS1, srs_BandwidthConfig, srs_SubframeConfig, pusch_p0_Nominal,
          pucch_p0_Nominal, msg3_delta_Preamble, rach_numberOfRA_Preambles,
          rach_sizeOfRA_PreamblesGroupA, rach_messageSizeGroupA,
          rach_powerRampingStep, rach_preambleInitialReceivedTargetPower,
          rach_preambleTransMax, rach_raResponseWindowSize,
          rach_macContentionResolutionTimer, rach_maxHARQ_Msg3Tx,
          pcch_defaultPagingCycle, bcch_modificationPeriodCoeff,
          ue_TimersAndConstants_t300, ue_TimersAndConstants_t301,
          ue_TimersAndConstants_t310, ue_TimersAndConstants_t311,
          ue_TimersAndConstants_n310, ue_TimersAndConstants_n311,
          ue_TransmissionMode, ue_multiple_max;
  /*
  int32_t     srb1_timer_poll_retransmit    = 0;
  int32_t     srb1_timer_reordering         = 0;
  int32_t     srb1_timer_status_prohibit    = 0;
  int32_t     srb1_poll_pdu                 = 0;
  int32_t     srb1_poll_byte                = 0;
  int32_t     srb1_max_retx_threshold       = 0;
  */

  const char       *rxPool_sc_CP_Len;
  const char       *rxPool_sc_Period;
  const char       *rxPool_data_CP_Len;
  libconfig_int     rxPool_ResourceConfig_prb_Num;
  libconfig_int     rxPool_ResourceConfig_prb_Start;
  libconfig_int     rxPool_ResourceConfig_prb_End;
  const char       *rxPool_ResourceConfig_offsetIndicator_present;
  libconfig_int     rxPool_ResourceConfig_offsetIndicator_choice;
  const char       *rxPool_ResourceConfig_subframeBitmap_present;
  char             *rxPool_ResourceConfig_subframeBitmap_choice_bs_buf;
  libconfig_int     rxPool_ResourceConfig_subframeBitmap_choice_bs_size;
  libconfig_int     rxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused;
  //SIB19
  //for discRxPool
  const char       *discRxPool_cp_Len;
  const char       *discRxPool_discPeriod;
  libconfig_int     discRxPool_numRetx;
  libconfig_int     discRxPool_numRepetition;
  libconfig_int     discRxPool_ResourceConfig_prb_Num;
  libconfig_int     discRxPool_ResourceConfig_prb_Start;
  libconfig_int     discRxPool_ResourceConfig_prb_End;
  const char       *discRxPool_ResourceConfig_offsetIndicator_present;
  libconfig_int     discRxPool_ResourceConfig_offsetIndicator_choice;
  const char       *discRxPool_ResourceConfig_subframeBitmap_present;
  char             *discRxPool_ResourceConfig_subframeBitmap_choice_bs_buf;
  libconfig_int     discRxPool_ResourceConfig_subframeBitmap_choice_bs_size;
  libconfig_int     discRxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused;
  //for discRxPoolPS
  const char       *discRxPoolPS_cp_Len;
  const char       *discRxPoolPS_discPeriod;
  libconfig_int     discRxPoolPS_numRetx;
  libconfig_int     discRxPoolPS_numRepetition;
  libconfig_int     discRxPoolPS_ResourceConfig_prb_Num;
  libconfig_int     discRxPoolPS_ResourceConfig_prb_Start;
  libconfig_int     discRxPoolPS_ResourceConfig_prb_End;
  const char       *discRxPoolPS_ResourceConfig_offsetIndicator_present;
  libconfig_int     discRxPoolPS_ResourceConfig_offsetIndicator_choice;
  const char       *discRxPoolPS_ResourceConfig_subframeBitmap_present;
  char             *discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_buf;
  libconfig_int     discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_size;
  libconfig_int     discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_bits_unused;
  //note: Uncomment below when FlexRAN support for eMTC is available
  /*
  int32_t     eMTC_configured                    = 0;
  int32_t     prach_root_emtc                    = 0;
  int32_t     prach_config_index_emtc            = 0;
  char        *prach_high_speed_emtc             = NULL;
  int32_t     prach_zero_correlation_emtc        = 0;
  int32_t     prach_freq_offset_emtc             = 0;
  int32_t     pucch_delta_shift_emtc             = 0;
  int32_t     pucch_nRB_CQI_emtc                 = 0;
  int32_t     pucch_nCS_AN_emtc                  = 0;
  //#if (RRC_VERSION >= MAKE_VERSION(10, 0, 0))
  int32_t     pucch_n1_AN_emtc                   = 0;
  //#endif
  int32_t     pdsch_referenceSignalPower_emtc    = 0;
  int32_t     pdsch_p_b_emtc                     = 0;
  int32_t     pusch_n_SB_emtc                    = 0;
  char        *pusch_hoppingMode_emtc            = NULL;
  int32_t     pusch_hoppingOffset_emtc           = 0;
  char        *pusch_enable64QAM_emtc            = NULL;
  char        *pusch_groupHoppingEnabled_emtc    = NULL;
  int32_t     pusch_groupAssignment_emtc         = 0;
  char        *pusch_sequenceHoppingEnabled_emtc = NULL;
  int32_t     pusch_nDMRS1_emtc                  = 0;
  char       *phich_duration_emtc                = NULL;
  char       *phich_resource_emtc                = NULL;
  char       *srs_enable_emtc                    = NULL;
  int32_t     srs_BandwidthConfig_emtc           = 0;
  int32_t     srs_SubframeConfig_emtc            = 0;
  char       *srs_ackNackST_emtc                 = NULL;
  char       *srs_MaxUpPts_emtc                  = NULL;
  int32_t     pusch_p0_Nominal_emtc              = 0;
  char       *pusch_alpha_emtc                   = NULL;
  int32_t     pucch_p0_Nominal_emtc              = 0;
  int32_t     msg3_delta_Preamble_emtc           = 0;
  //int32_t     ul_CyclicPrefixLength_emtc         = 0;
  char       *pucch_deltaF_Format1_emtc          = NULL;
  //const char*       pucch_deltaF_Format1a_emtc         = NULL;
  char       *pucch_deltaF_Format1b_emtc         = NULL;
  char       *pucch_deltaF_Format2_emtc          = NULL;
  char       *pucch_deltaF_Format2a_emtc         = NULL;
  char       *pucch_deltaF_Format2b_emtc         = NULL;
  int32_t     rach_numberOfRA_Preambles_emtc     = 0;
  char       *rach_preamblesGroupAConfig_emtc    = NULL;
  int32_t     rach_sizeOfRA_PreamblesGroupA_emtc = 0;
  int32_t     rach_messageSizeGroupA_emtc        = 0;
  char       *rach_messagePowerOffsetGroupB_emtc = NULL;
  int32_t     rach_powerRampingStep_emtc         = 0;
  int32_t     rach_preambleInitialReceivedTargetPower_emtc    = 0;
  int32_t     rach_preambleTransMax_emtc         = 0;
  int32_t     rach_raResponseWindowSize_emtc     = 10;
  int32_t     rach_macContentionResolutionTimer_emtc = 0;
  int32_t     rach_maxHARQ_Msg3Tx_emtc           = 0;
  int32_t     pcch_defaultPagingCycle_emtc       = 0;
  char       *pcch_nB_emtc                       = NULL;
  int32_t     bcch_modificationPeriodCoeff_emtc  = 0;
  int32_t     ue_TimersAndConstants_t300_emtc    = 0;
  int32_t     ue_TimersAndConstants_t301_emtc    = 0;
  int32_t     ue_TimersAndConstants_t310_emtc    = 0;
  int32_t     ue_TimersAndConstants_t311_emtc    = 0;
  int32_t     ue_TimersAndConstants_n310_emtc    = 0;
  int32_t     ue_TimersAndConstants_n311_emtc    = 0;
  int32_t     ue_TransmissionMode_emtc           = 0;

  int           si_Narrowband_r13             = 0;
  int           si_TBS_r13                    = 0;
  
  int           systemInfoValueTagSi_r13      = 0;
  
  int           firstPreamble_r13                     = 0;
  int           lastPreamble_r13                      = 0;
  int   ra_ResponseWindowSize_r13             = 0;
  int   mac_ContentionResolutionTimer_r13     = 0;
  int   rar_HoppingConfig_r13                 = 0;
  int           rsrp_range_br                         = 0;
  int  maxNumPreambleAttemptCE_r13           = 0;
  int    numRepetitionPerPreambleAttempt_r13   = 0;
  int   mpdcch_NumRepetition_RA_r13           = 0;
  int   prach_HoppingConfig_r13               = 0;
  int           *maxavailablenarrowband               = NULL;
  int           pucch_info_value                      = 0;

  int           prach_config_index_br                 = 0;
  int           prach_freq_offset_br                  = 0;
  int           prach_StartingSubframe_r13            = 0;
  
  int           paging_narrowbands_r13                = 0;
  int           mpdcch_numrepetition_paging_r13       = NULL;
  int           nb_v1310                              = NULL;
  
  
  char *   pucch_NumRepetitionCE_Msg4_Level0_r13 = NULL;
  char *   pucch_NumRepetitionCE_Msg4_Level1_r13 = NULL;
  char *   pucch_NumRepetitionCE_Msg4_Level2_r13 = NULL;
  char *   pucch_NumRepetitionCE_Msg4_Level3_r13 = NULL;
  
  int   sib2_mpdcch_pdsch_hoppingNB_r13                   = 0;
  int   sib2_interval_DLHoppingConfigCommonModeA_r13      = 0;
  int   sib2_interval_DLHoppingConfigCommonModeA_r13_val  = 0;
  int   sib2_interval_DLHoppingConfigCommonModeB_r13      = 0;
  int   sib2_interval_DLHoppingConfigCommonModeB_r13_val  = 0;
  
  int   sib2_interval_ULHoppingConfigCommonModeA_r13      = 0;
  int   sib2_interval_ULHoppingConfigCommonModeA_r13_val  = 0;
  int   sib2_interval_ULHoppingConfigCommonModeB_r13      = 0;
  int   sib2_interval_ULHoppingConfigCommonModeB_r13_val  = 0;
  int           sib2_mpdcch_pdsch_hoppingOffset_r13               = 0;
  
  int           pusch_HoppingOffset_v1310                         = 0;
  
  int           hyperSFN_r13                                      = 0;
  int           eDRX_Allowed_r13                                  = 0;
  int           q_RxLevMinCE_r13                                  = 0;
  int           q_QualMinRSRQ_CE_r13                              = 0;
  char*   si_WindowLength_BR_r13                            = NULL;
  char*   si_RepetitionPattern_r13                          = NULL;
  int           startSymbolBR_r13                                 = 0;
  char*   si_HoppingConfigCommon_r13                        = NULL;
  char*   si_ValidityTime_r13                               = NULL;
  char*   mpdcch_pdsch_HoppingNB_r13                        = NULL;
  int           interval_DLHoppingConfigCommonModeA_r13_val       = 0;
  int           interval_DLHoppingConfigCommonModeB_r13_val       = 0;
  int           mpdcch_pdsch_HoppingOffset_r13                    = 0;
  char*         preambleTransMax_CE_r13                           = NULL;
  char*         rach_numberOfRA_Preambles_br                      = NULL;
  
  int           schedulingInfoSIB1_BR_r13                         = 0;
  int64_t      fdd_DownlinkOrTddSubframeBitmapBR_val_r13         = 0;
  
  char* cellSelectionInfoCE_r13                                       = NULL;
  char* bandwidthReducedAccessRelatedInfo_r13                         = NULL;
  char* fdd_DownlinkOrTddSubframeBitmapBR_r13                         = NULL;
  char* fdd_UplinkSubframeBitmapBR_r13                                = NULL;
  char* freqHoppingParametersDL_r13                                   = NULL;
  char* interval_DLHoppingConfigCommonModeA_r13                       = NULL;
  char* interval_DLHoppingConfigCommonModeB_r13                       = NULL;
  */
  char*   prach_ConfigCommon_v1310                          = NULL;
  char*   mpdcch_startSF_CSS_RA_r13                         = NULL;
  char*   mpdcch_startSF_CSS_RA_r13_val                     = NULL;
  char*   pdsch_maxNumRepetitionCEmodeA_r13                 = NULL;
  char*   pdsch_maxNumRepetitionCEmodeB_r13                 = NULL;
  
  char*   pusch_maxNumRepetitionCEmodeA_r13                 = 0;
  char*   pusch_maxNumRepetitionCEmodeB_r13                 = 0;
  int     prach_HoppingOffset_r13                           = 0;

  // avoid gcc warnings
  (void)pdsch_maxNumRepetitionCEmodeB_r13;
  (void)pusch_maxNumRepetitionCEmodeB_r13;
  
  /* get number of eNBs */
  paramdef_t ENBSParams[] = ENBSPARAMS_DESC;
  config_get(ENBSParams, sizeof(ENBSParams)/sizeof(paramdef_t), NULL);
  num_enbs = ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt;
  /* for eNB ID */
  paramdef_t ENBParams[]  = ENBPARAMS_DESC;
  paramlist_def_t ENBParamList = {ENB_CONFIG_STRING_ENB_LIST, NULL, 0};
  /* for Nid_cell */
  checkedparam_t config_check_CCparams[] = CCPARAMS_CHECK;
  paramdef_t CCsParams[] = CCPARAMS_DESC;
  paramlist_def_t CCsParamList = {ENB_CONFIG_STRING_COMPONENT_CARRIERS, NULL, 0};

  // Note: these should be turned on for EMTC support inside of FlexRAN
  //paramdef_t brParams[]              = BRPARAMS_DESC;
  //paramdef_t schedulingInfoBrParams[] = SI_INFO_BR_DESC;
  //paramlist_def_t schedulingInfoBrParamList = {ENB_CONFIG_STRING_SCHEDULING_INFO_BR, NULL, 0};
  //paramdef_t rachcelevelParams[]     = RACH_CE_LEVELINFOLIST_R13_DESC;
  //paramlist_def_t rachcelevellist    = {ENB_CONFIG_STRING_RACH_CE_LEVELINFOLIST_R13, NULL, 0};
  //paramdef_t rsrprangeParams[]       = RSRP_RANGE_LIST_DESC;
  //paramlist_def_t rsrprangelist      = {ENB_CONFIG_STRING_RSRP_RANGE_LIST, NULL, 0};
  //paramdef_t prachParams[]           = PRACH_PARAMS_CE_R13_DESC;
  //paramlist_def_t prachParamslist    = {ENB_CONFIG_STRING_PRACH_PARAMETERS_CE_R13, NULL, 0};
  //paramdef_t n1PUCCH_ANR13Params[]   = N1PUCCH_AN_INFOLIST_R13_DESC;
  //paramlist_def_t n1PUCCHInfoList    = {ENB_CONFIG_STRING_N1PUCCH_AN_INFOLIST_R13, NULL, 0};
  //paramdef_t pcchv1310Params[]       = PCCH_CONFIG_V1310_DESC;
  //paramdef_t sib2freqhoppingParams[] = SIB2_FREQ_HOPPING_R13_DESC;
  //  paramdef_t SRB1Params[] = SRB1PARAMS_DESC;

  /* map parameter checking array instances to parameter definition array instances */
  for (int I = 0; I < (sizeof(CCsParams) / sizeof(paramdef_t)); I++) {
    CCsParams[I].chkPptr = &(config_check_CCparams[I]);
  }

  paramdef_t flexranParams[] = FLEXRANPARAMS_DESC;
  config_get(flexranParams, sizeof(flexranParams)/sizeof(paramdef_t), CONFIG_STRING_NETWORK_CONTROLLER_CONFIG);

  if (!RC.flexran) {
    RC.flexran = calloc(num_enbs, sizeof(flexran_agent_info_t *));
    AssertFatal(RC.flexran,
                "can't ALLOCATE %zu Bytes for %d flexran agent info with size %zu\n",
                num_enbs * sizeof(flexran_agent_info_t *),
                num_enbs, sizeof(flexran_agent_info_t *));
  }

  for (i = 0; i < num_enbs; i++) {
    RC.flexran[i] = calloc(1, sizeof(flexran_agent_info_t));
    AssertFatal(RC.flexran[i],
                "can't ALLOCATE %zu Bytes for flexran agent info (iteration %d/%d)\n",
                sizeof(flexran_agent_info_t), i + 1, num_enbs);
    /* if config says "yes", enable Agent, in all other cases it's like "no" */
    RC.flexran[i]->enabled          = strcasecmp(*(flexranParams[FLEXRAN_ENABLED].strptr), "yes") == 0;

    /* if not enabled, simply skip the rest, it is not needed anyway */
    if (!RC.flexran[i]->enabled)
      continue;

    RC.flexran[i]->interface_name   = strdup(*(flexranParams[FLEXRAN_INTERFACE_NAME_IDX].strptr));
    //inet_ntop(AF_INET, &(enb_properties->properties[mod_id]->flexran_agent_ipv4_address), in_ip, INET_ADDRSTRLEN);
    RC.flexran[i]->remote_ipv4_addr = strdup(*(flexranParams[FLEXRAN_IPV4_ADDRESS_IDX].strptr));
    RC.flexran[i]->remote_port      = *(flexranParams[FLEXRAN_PORT_IDX].uptr);
    RC.flexran[i]->cache_name       = strdup(*(flexranParams[FLEXRAN_CACHE_IDX].strptr));
    RC.flexran[i]->node_ctrl_state  = strcasecmp(*(flexranParams[FLEXRAN_AWAIT_RECONF_IDX].strptr), "yes") == 0 ? ENB_WAIT : ENB_NORMAL_OPERATION;
    config_getlist(&ENBParamList, ENBParams, sizeof(ENBParams)/sizeof(paramdef_t),NULL);

    /* eNB ID from configuration, as read in by RCconfig_RRC() */
    if (!ENBParamList.paramarray[i][ENB_ENB_ID_IDX].uptr) {
      // Calculate a default eNB ID
      if (EPC_MODE_ENABLED)
        enb_id = i + (s1ap_generate_eNB_id () & 0xFFFF8);
      else
        enb_id = i;
    } else {
      enb_id = *(ENBParamList.paramarray[i][ENB_ENB_ID_IDX].uptr);
    }

    /* cell ID */
    sprintf(aprefix, "%s.[%i]", ENB_CONFIG_STRING_ENB_LIST, i);
    config_getlist(&CCsParamList, NULL, 0, aprefix);

    if (CCsParamList.numelt > 0) {
      sprintf(aprefix, "%s.[%i].%s.[%i]", ENB_CONFIG_STRING_ENB_LIST, i, ENB_CONFIG_STRING_COMPONENT_CARRIERS, 0);
      config_get(CCsParams, sizeof(CCsParams)/sizeof(paramdef_t), aprefix);
      Nid_cell_tr = (uint16_t) Nid_cell;
    }

    RC.flexran[i]->mod_id   = i;
    RC.flexran[i]->agent_id = (((uint64_t)i) << 48) | (((uint64_t)enb_id) << 16) | ((uint64_t)Nid_cell_tr);
    /* assume for the moment the monolithic case, i.e. agent can provide
       information for all layers */
    RC.flexran[i]->capability_mask = FLEXRAN_CAP_LOPHY | FLEXRAN_CAP_HIPHY
      | FLEXRAN_CAP_LOMAC | FLEXRAN_CAP_HIMAC
      | FLEXRAN_CAP_RLC   | FLEXRAN_CAP_PDCP
      | FLEXRAN_CAP_SDAP  | FLEXRAN_CAP_RRC;
  }
}


void RCconfig_L1(void) {
  int               i,j;
  paramdef_t L1_Params[] = L1PARAMS_DESC;
  paramlist_def_t L1_ParamList = {CONFIG_STRING_L1_LIST,NULL,0};

  if (RC.eNB == NULL) {
    RC.eNB                       = (PHY_VARS_eNB ** *)malloc((1+NUMBER_OF_eNB_MAX)*sizeof(PHY_VARS_eNB **));
    LOG_I(PHY,"RC.eNB = %p\n",RC.eNB);
    memset(RC.eNB,0,(1+NUMBER_OF_eNB_MAX)*sizeof(PHY_VARS_eNB **));
    RC.nb_L1_CC = malloc((1+RC.nb_L1_inst)*sizeof(int));
  }

  config_getlist( &L1_ParamList,L1_Params,sizeof(L1_Params)/sizeof(paramdef_t), NULL);

  if (L1_ParamList.numelt > 0) {
    for (j = 0; j < RC.nb_L1_inst; j++) {
      RC.nb_L1_CC[j] = *(L1_ParamList.paramarray[j][L1_CC_IDX].uptr);

      if (RC.eNB[j] == NULL) {
        RC.eNB[j]                       = (PHY_VARS_eNB **)malloc((1+MAX_NUM_CCs)*sizeof(PHY_VARS_eNB *));
        LOG_I(PHY,"RC.eNB[%d] = %p\n",j,RC.eNB[j]);
        memset(RC.eNB[j],0,(1+MAX_NUM_CCs)*sizeof(PHY_VARS_eNB *));
      }

      for (i=0; i<RC.nb_L1_CC[j]; i++) {
        if (RC.eNB[j][i] == NULL) {
          RC.eNB[j][i] = (PHY_VARS_eNB *)malloc(sizeof(PHY_VARS_eNB));
          memset((void *)RC.eNB[j][i],0,sizeof(PHY_VARS_eNB));
          LOG_I(PHY,"RC.eNB[%d][%d] = %p\n",j,i,RC.eNB[j][i]);
          RC.eNB[j][i]->Mod_id  = j;
          RC.eNB[j][i]->CC_id   = i;
        }
      }

      if (strcmp(*(L1_ParamList.paramarray[j][L1_TRANSPORT_N_PREFERENCE_IDX].strptr), "local_mac") == 0) {
        sf_ahead = 4; // Need 4 subframe gap between RX and TX
      } else if (strcmp(*(L1_ParamList.paramarray[j][L1_TRANSPORT_N_PREFERENCE_IDX].strptr), "nfapi") == 0) {
        RC.eNB[j][0]->eth_params_n.local_if_name            = strdup(*(L1_ParamList.paramarray[j][L1_LOCAL_N_IF_NAME_IDX].strptr));
        RC.eNB[j][0]->eth_params_n.my_addr                  = strdup(*(L1_ParamList.paramarray[j][L1_LOCAL_N_ADDRESS_IDX].strptr));
        RC.eNB[j][0]->eth_params_n.remote_addr              = strdup(*(L1_ParamList.paramarray[j][L1_REMOTE_N_ADDRESS_IDX].strptr));
        RC.eNB[j][0]->eth_params_n.my_portc                 = *(L1_ParamList.paramarray[j][L1_LOCAL_N_PORTC_IDX].iptr);
        RC.eNB[j][0]->eth_params_n.remote_portc             = *(L1_ParamList.paramarray[j][L1_REMOTE_N_PORTC_IDX].iptr);
        RC.eNB[j][0]->eth_params_n.my_portd                 = *(L1_ParamList.paramarray[j][L1_LOCAL_N_PORTD_IDX].iptr);
        RC.eNB[j][0]->eth_params_n.remote_portd             = *(L1_ParamList.paramarray[j][L1_REMOTE_N_PORTD_IDX].iptr);
        RC.eNB[j][0]->eth_params_n.transp_preference        = ETH_UDP_MODE;
        sf_ahead = 2; // Cannot cope with 4 subframes betweem RX and TX - set it to 2
        RC.nb_macrlc_inst = 1;  // This is used by mac_top_init_eNB()
        // This is used by init_eNB_afterRU()
        RC.nb_CC = (int *)malloc((1+RC.nb_inst)*sizeof(int));
        RC.nb_CC[0]=1;
        RC.nb_inst =1; // DJP - feptx_prec uses num_eNB but phy_init_RU uses nb_inst
        LOG_I(PHY,"%s() NFAPI PNF mode - RC.nb_inst=1 this is because phy_init_RU() uses that to index and not RC.num_eNB - why the 2 similar variables?\n", __FUNCTION__);
        LOG_I(PHY,"%s() NFAPI PNF mode - RC.nb_CC[0]=%d for init_eNB_afterRU()\n", __FUNCTION__, RC.nb_CC[0]);
        LOG_I(PHY,"%s() NFAPI PNF mode - RC.nb_macrlc_inst:%d because used by mac_top_init_eNB()\n", __FUNCTION__, RC.nb_macrlc_inst);
        //mac_top_init_eNB();
        configure_nfapi_pnf(RC.eNB[j][0]->eth_params_n.remote_addr, RC.eNB[j][0]->eth_params_n.remote_portc, RC.eNB[j][0]->eth_params_n.my_addr, RC.eNB[j][0]->eth_params_n.my_portd,
                            RC.eNB[j][0]->eth_params_n     .remote_portd);
      } else { // other midhaul
      }
    }// j=0..num_inst

    printf("Initializing northbound interface for L1\n");
    l1_north_init_eNB();
  } else {
    LOG_I(PHY,"No " CONFIG_STRING_L1_LIST " configuration found");
    // DJP need to create some structures for VNF
    j = 0;
    RC.nb_L1_CC = malloc((1+RC.nb_L1_inst)*sizeof(int)); // DJP - 1 lot then???
    RC.nb_L1_CC[j]=1; // DJP - hmmm

    if (RC.eNB[j] == NULL) {
      RC.eNB[j]                       = (PHY_VARS_eNB **)malloc((1+MAX_NUM_CCs)*sizeof(PHY_VARS_eNB **));
      LOG_I(PHY,"RC.eNB[%d] = %p\n",j,RC.eNB[j]);
      memset(RC.eNB[j],0,(1+MAX_NUM_CCs)*sizeof(PHY_VARS_eNB ** *));
    }

    for (i=0; i<RC.nb_L1_CC[j]; i++) {
      if (RC.eNB[j][i] == NULL) {
        RC.eNB[j][i] = (PHY_VARS_eNB *)malloc(sizeof(PHY_VARS_eNB));
        memset((void *)RC.eNB[j][i],0,sizeof(PHY_VARS_eNB));
        LOG_I(PHY,"RC.eNB[%d][%d] = %p\n",j,i,RC.eNB[j][i]);
        RC.eNB[j][i]->Mod_id  = j;
        RC.eNB[j][i]->CC_id   = i;
      }
    }
  }
}

void RCconfig_macrlc() {
  int               j;
  paramdef_t MacRLC_Params[] = MACRLCPARAMS_DESC;
  paramlist_def_t MacRLC_ParamList = {CONFIG_STRING_MACRLC_LIST,NULL,0};
  config_getlist( &MacRLC_ParamList,MacRLC_Params,sizeof(MacRLC_Params)/sizeof(paramdef_t), NULL);

  if ( MacRLC_ParamList.numelt > 0) {
    RC.nb_macrlc_inst=MacRLC_ParamList.numelt;
    mac_top_init_eNB();
    RC.nb_mac_CC = (int *)malloc(RC.nb_macrlc_inst*sizeof(int));


    for (j=0; j<RC.nb_macrlc_inst; j++) {
      RC.mac[j]->puSch10xSnr = *(MacRLC_ParamList.paramarray[j][MACRLC_PUSCH10xSNR_IDX ].iptr);
      RC.mac[j]->puCch10xSnr = *(MacRLC_ParamList.paramarray[j][MACRLC_PUCCH10xSNR_IDX ].iptr);
      RC.nb_mac_CC[j] = *(MacRLC_ParamList.paramarray[j][MACRLC_CC_IDX].iptr);
      //RC.mac[j]->phy_test = *(MacRLC_ParamList.paramarray[j][MACRLC_PHY_TEST_IDX].iptr);
      //printf("PHY_TEST = %d,%d\n", RC.mac[j]->phy_test, j);

      if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_N_PREFERENCE_IDX].strptr), "local_RRC") == 0) {
        // check number of instances is same as RRC/PDCP
      } else if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_N_PREFERENCE_IDX].strptr), "cudu") == 0) {
        RC.mac[j]->eth_params_n.local_if_name            = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_IF_NAME_IDX].strptr));
        RC.mac[j]->eth_params_n.my_addr                  = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_ADDRESS_IDX].strptr));
        RC.mac[j]->eth_params_n.remote_addr              = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_ADDRESS_IDX].strptr));
        RC.mac[j]->eth_params_n.my_portc                 = *(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_PORTC_IDX].iptr);
        RC.mac[j]->eth_params_n.remote_portc             = *(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_PORTC_IDX].iptr);
        RC.mac[j]->eth_params_n.my_portd                 = *(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_PORTD_IDX].iptr);
        RC.mac[j]->eth_params_n.remote_portd             = *(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_PORTD_IDX].iptr);;
        RC.mac[j]->eth_params_n.transp_preference        = ETH_UDP_MODE;
      } else { // other midhaul
        AssertFatal(1==0,"MACRLC %d: %s unknown northbound midhaul\n",j, *(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_N_PREFERENCE_IDX].strptr));
      }

      if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_S_PREFERENCE_IDX].strptr), "local_L1") == 0) {
      } else if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_S_PREFERENCE_IDX].strptr), "nfapi") == 0) {
        RC.mac[j]->eth_params_s.local_if_name            = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_S_IF_NAME_IDX].strptr));
        RC.mac[j]->eth_params_s.my_addr                  = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_S_ADDRESS_IDX].strptr));
        RC.mac[j]->eth_params_s.remote_addr              = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_S_ADDRESS_IDX].strptr));
        RC.mac[j]->eth_params_s.my_portc                 = *(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_S_PORTC_IDX].iptr);
        RC.mac[j]->eth_params_s.remote_portc             = *(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_S_PORTC_IDX].iptr);
        RC.mac[j]->eth_params_s.my_portd                 = *(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_S_PORTD_IDX].iptr);
        RC.mac[j]->eth_params_s.remote_portd             = *(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_S_PORTD_IDX].iptr);
        RC.mac[j]->eth_params_s.transp_preference        = ETH_UDP_MODE;
        sf_ahead = 2; // Cannot cope with 4 subframes betweem RX and TX - set it to 2
        printf("**************** vnf_port:%d\n", RC.mac[j]->eth_params_s.my_portc);
        configure_nfapi_vnf(RC.mac[j]->eth_params_s.my_addr, RC.mac[j]->eth_params_s.my_portc);
        printf("**************** RETURNED FROM configure_nfapi_vnf() vnf_port:%d\n", RC.mac[j]->eth_params_s.my_portc);
      } else { // other midhaul
        AssertFatal(1==0,"MACRLC %d: %s unknown southbound midhaul\n",j,*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_S_PREFERENCE_IDX].strptr));
      }

      if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_SCHED_MODE_IDX].strptr), "default") == 0) {
        global_scheduler_mode=SCHED_MODE_DEFAULT;
        printf("sched mode = default %d [%s]\n",global_scheduler_mode,*(MacRLC_ParamList.paramarray[j][MACRLC_SCHED_MODE_IDX].strptr));
      } else if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_SCHED_MODE_IDX].strptr), "fairRR") == 0) {
        global_scheduler_mode=SCHED_MODE_FAIR_RR;
        printf("sched mode = fairRR %d [%s]\n",global_scheduler_mode,*(MacRLC_ParamList.paramarray[j][MACRLC_SCHED_MODE_IDX].strptr));
      } else {
        global_scheduler_mode=SCHED_MODE_DEFAULT;
        printf("sched mode = default %d [%s]\n",global_scheduler_mode,*(MacRLC_ParamList.paramarray[j][MACRLC_SCHED_MODE_IDX].strptr));
      }
    }// j=0..num_inst
  } else {// MacRLC_ParamList.numelt > 0
    AssertFatal (0,
                 "No " CONFIG_STRING_MACRLC_LIST " configuration found");
  }
}

int RCconfig_RRC(MessageDef *msg_p, uint32_t i, eNB_RRC_INST *rrc) {

  int               num_enbs                      = 0;
  int               j,k                           = 0;
  int32_t     enb_id                        = 0;
  int               nb_cc                         = 0;
  char       *frame_type                    = NULL;
  int32_t     tdd_config                    = 0;
  int32_t     tdd_config_s                  = 0;
  char       *prefix_type                   = NULL;
  char       *pbch_repetition               = NULL;
  int32_t     eutra_band                    = 0;
  long long int     downlink_frequency            = 0;
  int32_t     uplink_frequency_offset       = 0;
  int32_t     Nid_cell                      = 0;
  int32_t     Nid_cell_mbsfn                = 0;
  int32_t     N_RB_DL                       = 0;
  int32_t     nb_antenna_ports              = 0;
  int32_t     prach_root                    = 0;
  int32_t     prach_config_index            = 0;
  char            *prach_high_speed         = NULL;
  int32_t     prach_zero_correlation        = 0;
  int32_t     prach_freq_offset             = 0;
  int32_t     pucch_delta_shift             = 0;
  int32_t     pucch_nRB_CQI                 = 0;
  int32_t     pucch_nCS_AN                  = 0;
  //#if (RRC_VERSION >= MAKE_VERSION(10, 0, 0))
  int32_t     pucch_n1_AN                   = 0;
  //#endif
  int32_t     pdsch_referenceSignalPower    = 0;
  int32_t     pdsch_p_b                     = 0;
  int32_t     pusch_n_SB                    = 0;
  char       *pusch_hoppingMode             = NULL;
  int32_t     pusch_hoppingOffset           = 0;
  char          *pusch_enable64QAM          = NULL;
  char          *pusch_groupHoppingEnabled  = NULL;
  int32_t     pusch_groupAssignment         = 0;
  char          *pusch_sequenceHoppingEnabled = NULL;
  int32_t     pusch_nDMRS1                  = 0;
  char       *phich_duration                = NULL;
  char       *phich_resource                = NULL;
  char       *srs_enable                    = NULL;
  int32_t     srs_BandwidthConfig           = 0;
  int32_t     srs_SubframeConfig            = 0;
  char       *srs_ackNackST                 = NULL;
  char       *srs_MaxUpPts                  = NULL;
  int32_t     pusch_p0_Nominal              = 0;
  char       *pusch_alpha                   = NULL;
  int32_t     pucch_p0_Nominal              = 0;
  int32_t     msg3_delta_Preamble           = 0;
  //int32_t     ul_CyclicPrefixLength         = 0;
  char       *pucch_deltaF_Format1          = NULL;
  //const char*       pucch_deltaF_Format1a         = NULL;
  char       *pucch_deltaF_Format1b         = NULL;
  char       *pucch_deltaF_Format2          = NULL;
  char       *pucch_deltaF_Format2a         = NULL;
  char       *pucch_deltaF_Format2b         = NULL;
  int32_t     rach_numberOfRA_Preambles     = 0;
  char       *rach_preamblesGroupAConfig    = NULL;
  int32_t     rach_sizeOfRA_PreamblesGroupA = 0;
  int32_t     rach_messageSizeGroupA        = 0;
  char       *rach_messagePowerOffsetGroupB = NULL;
  int32_t     rach_powerRampingStep         = 0;
  int32_t     rach_preambleInitialReceivedTargetPower    = 0;
  int32_t     rach_preambleTransMax         = 0;
  int32_t     rach_raResponseWindowSize     = 10;
  int32_t     rach_macContentionResolutionTimer = 0;
  int32_t     rach_maxHARQ_Msg3Tx           = 0;
  int32_t     pcch_defaultPagingCycle       = 0;
  char       *pcch_nB                       = NULL;
  int32_t     bcch_modificationPeriodCoeff  = 0;
  int32_t     ue_TimersAndConstants_t300    = 0;
  int32_t     ue_TimersAndConstants_t301    = 0;
  int32_t     ue_TimersAndConstants_t310    = 0;
  int32_t     ue_TimersAndConstants_t311    = 0;
  int32_t     ue_TimersAndConstants_n310    = 0;
  int32_t     ue_TimersAndConstants_n311    = 0;
  int32_t     ue_TransmissionMode           = 0;
  int32_t     ue_multiple_max               = 0;
  //TTN - for D2D
  //SIB18
  const char       *rxPool_sc_CP_Len                                        = NULL;
  const char       *rxPool_sc_Period                                        = NULL;
  const char       *rxPool_data_CP_Len                                      = NULL;
  libconfig_int     rxPool_ResourceConfig_prb_Num                           = 0;
  libconfig_int     rxPool_ResourceConfig_prb_Start                         = 0;
  libconfig_int     rxPool_ResourceConfig_prb_End                           = 0;
  const char       *rxPool_ResourceConfig_offsetIndicator_present           = NULL;
  libconfig_int     rxPool_ResourceConfig_offsetIndicator_choice            = 0;
  const char       *rxPool_ResourceConfig_subframeBitmap_present            = NULL;
  char             *rxPool_ResourceConfig_subframeBitmap_choice_bs_buf      = NULL;
  libconfig_int     rxPool_ResourceConfig_subframeBitmap_choice_bs_size     = 0;
  libconfig_int     rxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused     = 0;
  //SIB19
  //For discRxPool
  const char       *discRxPool_cp_Len                                              = NULL;
  const char       *discRxPool_discPeriod                                          = NULL;
  libconfig_int     discRxPool_numRetx                                             = 0;
  libconfig_int     discRxPool_numRepetition                                       = 0;
  libconfig_int     discRxPool_ResourceConfig_prb_Num                              = 0;
  libconfig_int     discRxPool_ResourceConfig_prb_Start                            = 0;
  libconfig_int     discRxPool_ResourceConfig_prb_End                              = 0;
  const char       *discRxPool_ResourceConfig_offsetIndicator_present              = NULL;
  libconfig_int     discRxPool_ResourceConfig_offsetIndicator_choice               = 0;
  const char       *discRxPool_ResourceConfig_subframeBitmap_present               = NULL;
  char             *discRxPool_ResourceConfig_subframeBitmap_choice_bs_buf         = NULL;
  libconfig_int     discRxPool_ResourceConfig_subframeBitmap_choice_bs_size        = 0;
  libconfig_int     discRxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused = 0;
  //For discRxPoolPS
  const char       *discRxPoolPS_cp_Len                                              = NULL;
  const char       *discRxPoolPS_discPeriod                                          = NULL;
  libconfig_int     discRxPoolPS_numRetx                                             = 0;
  libconfig_int     discRxPoolPS_numRepetition                                       = 0;
  libconfig_int     discRxPoolPS_ResourceConfig_prb_Num                              = 0;
  libconfig_int     discRxPoolPS_ResourceConfig_prb_Start                            = 0;
  libconfig_int     discRxPoolPS_ResourceConfig_prb_End                              = 0;
  const char       *discRxPoolPS_ResourceConfig_offsetIndicator_present              = NULL;
  libconfig_int     discRxPoolPS_ResourceConfig_offsetIndicator_choice               = 0;
  const char       *discRxPoolPS_ResourceConfig_subframeBitmap_present               = NULL;
  char             *discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_buf         = NULL;
  libconfig_int     discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_size        = 0;
  libconfig_int     discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_bits_unused = 0;
  int32_t     srb1_timer_poll_retransmit    = 0;
  int32_t     srb1_timer_reordering         = 0;
  int32_t     srb1_timer_status_prohibit    = 0;
  int32_t     srb1_poll_pdu                 = 0;
  int32_t     srb1_poll_byte                = 0;
  int32_t     srb1_max_retx_threshold       = 0;
  int32_t     my_int;

  // eMTC definitions
  int32_t     eMTC_configured                    = 0;
  int32_t     prach_root_emtc                    = 0;
  int32_t     prach_config_index_emtc            = 0;
  char        *prach_high_speed_emtc             = NULL;
  int32_t     prach_zero_correlation_emtc        = 0;
  int32_t     prach_freq_offset_emtc             = 0;
  int32_t     pucch_delta_shift_emtc             = 0;
  int32_t     pucch_nRB_CQI_emtc                 = 0;
  int32_t     pucch_nCS_AN_emtc                  = 0;
  //#if (RRC_VERSION >= MAKE_VERSION(10, 0, 0))
  int32_t     pucch_n1_AN_emtc                   = 0;
  //#endif
  int32_t     pdsch_referenceSignalPower_emtc    = 0;
  int32_t     pdsch_p_b_emtc                     = 0;
  int32_t     pusch_n_SB_emtc                    = 0;
  char        *pusch_hoppingMode_emtc            = NULL;
  int32_t     pusch_hoppingOffset_emtc           = 0;
  char        *pusch_enable64QAM_emtc            = NULL;
  char        *pusch_groupHoppingEnabled_emtc    = NULL;
  int32_t     pusch_groupAssignment_emtc         = 0;
  char        *pusch_sequenceHoppingEnabled_emtc = NULL;
  int32_t     pusch_nDMRS1_emtc                  = 0;
  char       *phich_duration_emtc                = NULL;
  char       *phich_resource_emtc                = NULL;
  char       *srs_enable_emtc                    = NULL;
  int32_t     srs_BandwidthConfig_emtc           = 0;
  int32_t     srs_SubframeConfig_emtc            = 0;
  char       *srs_ackNackST_emtc                 = NULL;
  char       *srs_MaxUpPts_emtc                  = NULL;
  int32_t     pusch_p0_Nominal_emtc              = 0;
  char       *pusch_alpha_emtc                   = NULL;
  int32_t     pucch_p0_Nominal_emtc              = 0;
  int32_t     msg3_delta_Preamble_emtc           = 0;
  //int32_t     ul_CyclicPrefixLength_emtc         = 0;
  char       *pucch_deltaF_Format1_emtc          = NULL;
  //const char*       pucch_deltaF_Format1a_emtc         = NULL;
  char       *pucch_deltaF_Format1b_emtc         = NULL;
  char       *pucch_deltaF_Format2_emtc          = NULL;
  char       *pucch_deltaF_Format2a_emtc         = NULL;
  char       *pucch_deltaF_Format2b_emtc         = NULL;
  int32_t     rach_numberOfRA_Preambles_emtc     = 0;
  char       *rach_preamblesGroupAConfig_emtc    = NULL;
  int32_t     rach_sizeOfRA_PreamblesGroupA_emtc = 0;
  int32_t     rach_messageSizeGroupA_emtc        = 0;
  char       *rach_messagePowerOffsetGroupB_emtc = NULL;
  int32_t     rach_powerRampingStep_emtc         = 0;
  int32_t     rach_preambleInitialReceivedTargetPower_emtc    = 0;
  int32_t     rach_preambleTransMax_emtc         = 0;
  int32_t     rach_raResponseWindowSize_emtc     = 10;
  int32_t     rach_macContentionResolutionTimer_emtc = 0;
  int32_t     rach_maxHARQ_Msg3Tx_emtc           = 0;
  int32_t     pcch_defaultPagingCycle_emtc       = 0;
  char       *pcch_nB_emtc                       = NULL;
  int32_t     bcch_modificationPeriodCoeff_emtc  = 0;
  int32_t     ue_TimersAndConstants_t300_emtc    = 0;
  int32_t     ue_TimersAndConstants_t301_emtc    = 0;
  int32_t     ue_TimersAndConstants_t310_emtc    = 0;
  int32_t     ue_TimersAndConstants_t311_emtc    = 0;
  int32_t     ue_TimersAndConstants_n310_emtc    = 0;
  int32_t     ue_TimersAndConstants_n311_emtc    = 0;
  int32_t     ue_TransmissionMode_emtc           = 0;

  int           si_Narrowband_r13             = 0;
  int           si_TBS_r13                    = 0;
  
  int           systemInfoValueTagSi_r13      = 0;
  
  int           firstPreamble_r13                     = 0;
  int           lastPreamble_r13                      = 0;
  int  ra_ResponseWindowSize_r13              = 0;
  int   mac_ContentionResolutionTimer_r13     = 0;
  int   rar_HoppingConfig_r13                 = 0;
  int           rsrp_range_br                         = 0;
  int           prach_config_index_br                 = 0;
  int           prach_freq_offset_br                  = 0;
  int           prach_StartingSubframe_r13            = 0;

  int   maxNumPreambleAttemptCE_r13           = 0;
  int   numRepetitionPerPreambleAttempt_r13   = 0;
  int   mpdcch_NumRepetition_RA_r13           = 0;
  int   prach_HoppingConfig_r13               = 0;
  int           *maxavailablenarrowband               = NULL;
  int           pucch_info_value                      = 0;
  
  int           paging_narrowbands_r13                = 0;
  int           mpdcch_numrepetition_paging_r13       = NULL;
  int           nb_v1310                              = NULL;
  
  
  char *   pucch_NumRepetitionCE_Msg4_Level0_r13 = NULL;
  char *   pucch_NumRepetitionCE_Msg4_Level1_r13 = NULL;
  char *   pucch_NumRepetitionCE_Msg4_Level2_r13 = NULL;
  char *   pucch_NumRepetitionCE_Msg4_Level3_r13 = NULL;
  
  int   sib2_mpdcch_pdsch_hoppingNB_r13                   = 0;
  char  *sib2_interval_DLHoppingConfigCommonModeA_r13      = 0;
  int   sib2_interval_DLHoppingConfigCommonModeA_r13_val  = 0;
  char  *sib2_interval_DLHoppingConfigCommonModeB_r13      = 0;
  int   sib2_interval_DLHoppingConfigCommonModeB_r13_val  = 0;
  
  char  *sib2_interval_ULHoppingConfigCommonModeA_r13      = 0;
  int   sib2_interval_ULHoppingConfigCommonModeA_r13_val  = 0;
  char  *sib2_interval_ULHoppingConfigCommonModeB_r13      = 0;
  int   sib2_interval_ULHoppingConfigCommonModeB_r13_val  = 0;
  int   sib2_mpdcch_pdsch_hoppingOffset_r13       = 0;
  

  int           pusch_HoppingOffset_v1310                         = 0;
  
  int           hyperSFN_r13                                      = 0;
  int           eDRX_Allowed_r13                                  = 0;
  int           q_RxLevMinCE_r13                                  = 0;
  int           q_QualMinRSRQ_CE_r13                              = 0;
  char   *si_WindowLength_BR_r13                            = NULL;
  char   *si_RepetitionPattern_r13                          = NULL;
  int           startSymbolBR_r13                                 = 0;
  char   *si_HoppingConfigCommon_r13                        = NULL;
  char   *si_ValidityTime_r13                               = NULL;
  char   *mpdcch_pdsch_HoppingNB_r13                        = NULL;
  int           interval_DLHoppingConfigCommonModeA_r13_val       = 0;
  int           interval_DLHoppingConfigCommonModeB_r13_val       = 0;
  int           mpdcch_pdsch_HoppingOffset_r13                    = 0;
  char   *preambleTransMax_CE_r13                           = NULL;
  
  int           prach_HoppingOffset_r13                           = 0;
  int           schedulingInfoSIB1_BR_r13                         = 0;
  int64_t      fdd_DownlinkOrTddSubframeBitmapBR_val_r13         = 0;
  
  char* cellSelectionInfoCE_r13                                       = NULL;
  char* bandwidthReducedAccessRelatedInfo_r13                         = NULL;
  char* fdd_DownlinkOrTddSubframeBitmapBR_r13                         = NULL;
  char* fdd_UplinkSubframeBitmapBR_r13                                = NULL;
  (void)fdd_UplinkSubframeBitmapBR_r13;
  char* freqHoppingParametersDL_r13                                   = NULL;
  char* interval_DLHoppingConfigCommonModeA_r13                       = NULL;
  char* interval_DLHoppingConfigCommonModeB_r13                       = NULL;
  char* prach_ConfigCommon_v1310                                = NULL;
  char* mpdcch_startSF_CSS_RA_r13                               = NULL;
  char* mpdcch_startSF_CSS_RA_r13_val                           = NULL;
  char  *pdsch_maxNumRepetitionCEmodeA_r13                 = NULL;
  char  *pdsch_maxNumRepetitionCEmodeB_r13                 = NULL;
  char   *pusch_maxNumRepetitionCEmodeA_r13                 = NULL;
  char   *pusch_maxNumRepetitionCEmodeB_r13                 = NULL;
  // for no gcc warnings
  (void)my_int;
  (void)pdsch_maxNumRepetitionCEmodeB_r13;
  (void)pusch_maxNumRepetitionCEmodeB_r13;
  paramdef_t ENBSParams[] = ENBSPARAMS_DESC;
  paramdef_t ENBParams[]  = ENBPARAMS_DESC;
  paramlist_def_t ENBParamList = {ENB_CONFIG_STRING_ENB_LIST,NULL,0};
  checkedparam_t config_check_CCparams[] = CCPARAMS_CHECK;
  paramdef_t CCsParams[] = CCPARAMS_DESC;
  paramlist_def_t CCsParamList = {ENB_CONFIG_STRING_COMPONENT_CARRIERS,NULL,0};
  paramdef_t brParams[]              = BRPARAMS_DESC;
  paramdef_t schedulingInfoBrParams[] = SI_INFO_BR_DESC;
  paramlist_def_t schedulingInfoBrParamList = {ENB_CONFIG_STRING_SCHEDULING_INFO_BR, NULL, 0};
  paramdef_t rachcelevelParams[]     = RACH_CE_LEVELINFOLIST_R13_DESC;
  paramlist_def_t rachcelevellist    = {ENB_CONFIG_STRING_RACH_CE_LEVELINFOLIST_R13, NULL, 0};
  paramdef_t rsrprangeParams[]       = RSRP_RANGE_LIST_DESC;
  paramlist_def_t rsrprangelist      = {ENB_CONFIG_STRING_RSRP_RANGE_LIST, NULL, 0};
  paramdef_t prachParams[]           = PRACH_PARAMS_CE_R13_DESC;
  paramlist_def_t prachParamslist    = {ENB_CONFIG_STRING_PRACH_PARAMETERS_CE_R13, NULL, 0};
  paramdef_t n1PUCCH_ANR13Params[]   = N1PUCCH_AN_INFOLIST_R13_DESC;
  paramlist_def_t n1PUCCHInfoList    = {ENB_CONFIG_STRING_N1PUCCH_AN_INFOLIST_R13, NULL, 0};
  paramdef_t pcchv1310Params[]       = PCCH_CONFIG_V1310_DESC;
  paramdef_t sib2freqhoppingParams[] = SIB2_FREQ_HOPPING_R13_DESC;
  paramdef_t SRB1Params[] = SRB1PARAMS_DESC;

  /* map parameter checking array instances to parameter definition array instances */
  for (int I=0; I< ( sizeof(CCsParams)/ sizeof(paramdef_t)  ) ; I++) {
    CCsParams[I].chkPptr = &(config_check_CCparams[I]);
  }

  /* get global parameters, defined outside any section in the config file */
  config_get( ENBSParams,sizeof(ENBSParams)/sizeof(paramdef_t),NULL);
  num_enbs = ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt;
  AssertFatal (i<num_enbs,
               "Failed to parse config file no %ith element in %s \n",i, ENB_CONFIG_STRING_ACTIVE_ENBS);

  if (num_enbs>0) {
    // Output a list of all eNBs.
    config_getlist( &ENBParamList,ENBParams,sizeof(ENBParams)/sizeof(paramdef_t),NULL);

    if (ENBParamList.paramarray[i][ENB_ENB_ID_IDX].uptr == NULL) {
      // Calculate a default eNB ID
      if (EPC_MODE_ENABLED) {
        uint32_t hash;
        hash = s1ap_generate_eNB_id ();
        enb_id = i + (hash & 0xFFFF8);
      } else {
        enb_id = i;
      }
    } else {
      enb_id = *(ENBParamList.paramarray[i][ENB_ENB_ID_IDX].uptr);
    }

    printf("RRC %d: Southbound Transport %s\n",i,*(ENBParamList.paramarray[i][ENB_TRANSPORT_S_PREFERENCE_IDX].strptr));

    if (strcmp(*(ENBParamList.paramarray[i][ENB_TRANSPORT_S_PREFERENCE_IDX].strptr), "local_mac") == 0) {
    } else if (strcmp(*(ENBParamList.paramarray[i][ENB_TRANSPORT_S_PREFERENCE_IDX].strptr), "cudu") == 0) {
      rrc->eth_params_s.local_if_name            = strdup(*(ENBParamList.paramarray[i][ENB_LOCAL_S_IF_NAME_IDX].strptr));
      rrc->eth_params_s.my_addr                  = strdup(*(ENBParamList.paramarray[i][ENB_LOCAL_S_ADDRESS_IDX].strptr));
      rrc->eth_params_s.remote_addr              = strdup(*(ENBParamList.paramarray[i][ENB_REMOTE_S_ADDRESS_IDX].strptr));
      rrc->eth_params_s.my_portc                 = *(ENBParamList.paramarray[i][ENB_LOCAL_S_PORTC_IDX].uptr);
      rrc->eth_params_s.remote_portc             = *(ENBParamList.paramarray[i][ENB_REMOTE_S_PORTC_IDX].uptr);
      rrc->eth_params_s.my_portd                 = *(ENBParamList.paramarray[i][ENB_LOCAL_S_PORTD_IDX].uptr);
      rrc->eth_params_s.remote_portd             = *(ENBParamList.paramarray[i][ENB_REMOTE_S_PORTD_IDX].uptr);
      rrc->eth_params_s.transp_preference        = ETH_UDP_MODE;
    } else { // other midhaul
    }

    // search if in active list

    for (k=0; k <num_enbs ; k++) {
      if (strcmp(ENBSParams[ENB_ACTIVE_ENBS_IDX].strlistptr[k], *(ENBParamList.paramarray[i][ENB_ENB_NAME_IDX].strptr) )== 0) {
        char enbpath[MAX_OPTNAME_SIZE + 8];
        sprintf(enbpath,"%s.[%i]",ENB_CONFIG_STRING_ENB_LIST,k);
        paramdef_t PLMNParams[] = PLMNPARAMS_DESC;
        paramlist_def_t PLMNParamList = {ENB_CONFIG_STRING_PLMN_LIST, NULL, 0};
        /* map parameter checking array instances to parameter definition array instances */
        checkedparam_t config_check_PLMNParams [] = PLMNPARAMS_CHECK;

        for (int I = 0; I < sizeof(PLMNParams) / sizeof(paramdef_t); ++I)
          PLMNParams[I].chkPptr = &(config_check_PLMNParams[I]);

        RRC_CONFIGURATION_REQ (msg_p).cell_identity = enb_id;
        RRC_CONFIGURATION_REQ(msg_p).tac = *ENBParamList.paramarray[i][ENB_TRACKING_AREA_CODE_IDX].uptr;
        AssertFatal(!ENBParamList.paramarray[i][ENB_MOBILE_COUNTRY_CODE_IDX_OLD].strptr
                    && !ENBParamList.paramarray[i][ENB_MOBILE_NETWORK_CODE_IDX_OLD].strptr,
                    "It seems that you use an old configuration file. Please change the existing\n"
                    "    tracking_area_code  =  \"1\";\n"
                    "    mobile_country_code =  \"208\";\n"
                    "    mobile_network_code =  \"93\";\n"
                    "to\n"
                    "    tracking_area_code  =  1; // no string!!\n"
                    "    plmn_list = ( { mcc = 208; mnc = 93; mnc_length = 2; } )\n");
        config_getlist(&PLMNParamList, PLMNParams, sizeof(PLMNParams)/sizeof(paramdef_t), enbpath);

        if (PLMNParamList.numelt < 1 || PLMNParamList.numelt > 6)
          AssertFatal(0, "The number of PLMN IDs must be in [1,6], but is %d\n",
                      PLMNParamList.numelt);

        RRC_CONFIGURATION_REQ(msg_p).num_plmn = PLMNParamList.numelt;

        for (int l = 0; l < PLMNParamList.numelt; ++l) {
          RRC_CONFIGURATION_REQ(msg_p).mcc[l] = *PLMNParamList.paramarray[l][ENB_MOBILE_COUNTRY_CODE_IDX].uptr;
          RRC_CONFIGURATION_REQ(msg_p).mnc[l] = *PLMNParamList.paramarray[l][ENB_MOBILE_NETWORK_CODE_IDX].uptr;
          RRC_CONFIGURATION_REQ(msg_p).mnc_digit_length[l] = *PLMNParamList.paramarray[l][ENB_MNC_DIGIT_LENGTH].u8ptr;
          AssertFatal(RRC_CONFIGURATION_REQ(msg_p).mnc_digit_length[l] == 3
                      || RRC_CONFIGURATION_REQ(msg_p).mnc[l] < 100,
                      "MNC %d cannot be encoded in two digits as requested (change mnc_digit_length to 3)\n",
                      RRC_CONFIGURATION_REQ(msg_p).mnc[l]);
        }

        // Parse optional physical parameters
        config_getlist( &CCsParamList,NULL,0,enbpath);
        LOG_I(RRC,"num component carriers %d \n",CCsParamList.numelt);

        if ( CCsParamList.numelt> 0) {
          char ccspath[MAX_OPTNAME_SIZE*2 + 16];

          for (j = 0; j < CCsParamList.numelt ; j++) {
            sprintf(ccspath,"%s.%s.[%i]",enbpath,ENB_CONFIG_STRING_COMPONENT_CARRIERS,j);
            LOG_I(RRC, "enb_config::RCconfig_RRC() parameter number: %d, total number of parameters: %zd, ccspath: %s \n \n", j, sizeof(CCsParams)/sizeof(paramdef_t), ccspath);
            config_get( CCsParams,sizeof(CCsParams)/sizeof(paramdef_t),ccspath);
            //printf("Component carrier %d\n",component_carrier);
            nb_cc++;
            RRC_CONFIGURATION_REQ (msg_p).tdd_config[j] = tdd_config;
            AssertFatal (tdd_config <= TDD_Config__subframeAssignment_sa6,
                         "Failed to parse eNB configuration file %s, enb %d illegal tdd_config %d (should be 0-%d)!",
                         RC.config_file_name, i, tdd_config, TDD_Config__subframeAssignment_sa6);
            RRC_CONFIGURATION_REQ (msg_p).tdd_config_s[j] = tdd_config_s;
            AssertFatal (tdd_config_s <= TDD_Config__specialSubframePatterns_ssp8,
                         "Failed to parse eNB configuration file %s, enb %d illegal tdd_config_s %d (should be 0-%d)!",
                         RC.config_file_name, i, tdd_config_s, TDD_Config__specialSubframePatterns_ssp8);

            if (!prefix_type)
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d define %s: NORMAL,EXTENDED!\n",
                           RC.config_file_name, i, ENB_CONFIG_STRING_PREFIX_TYPE);
            else if (strcmp(prefix_type, "NORMAL") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).prefix_type[j] = NORMAL;
            } else  if (strcmp(prefix_type, "EXTENDED") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).prefix_type[j] = EXTENDED;
            } else {
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for prefix_type choice: NORMAL or EXTENDED !\n",
                           RC.config_file_name, i, prefix_type);
            }

#if (RRC_VERSION >= MAKE_VERSION(14, 0, 0))

            if (!pbch_repetition)
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d define %s: TRUE,FALSE!\n",
                           RC.config_file_name, i, ENB_CONFIG_STRING_PBCH_REPETITION);
            else if (strcmp(pbch_repetition, "TRUE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).pbch_repetition[j] = 1;
            } else  if (strcmp(pbch_repetition, "FALSE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).pbch_repetition[j] = 0;
            } else {
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pbch_repetition choice: TRUE or FALSE !\n",
                           RC.config_file_name, i, pbch_repetition);
            }

#endif
            RRC_CONFIGURATION_REQ (msg_p).eutra_band[j] = eutra_band;
            RRC_CONFIGURATION_REQ (msg_p).downlink_frequency[j] = (uint32_t) downlink_frequency;
            RRC_CONFIGURATION_REQ (msg_p).uplink_frequency_offset[j] = (unsigned int) uplink_frequency_offset;
            RRC_CONFIGURATION_REQ (msg_p).Nid_cell[j]= Nid_cell;

            if (Nid_cell>503) {
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for Nid_cell choice: 0...503 !\n",
                           RC.config_file_name, i, Nid_cell);
            }

            RRC_CONFIGURATION_REQ (msg_p).N_RB_DL[j]= N_RB_DL;

            if ((N_RB_DL!=6) && (N_RB_DL!=15) && (N_RB_DL!=25) && (N_RB_DL!=50) && (N_RB_DL!=75) && (N_RB_DL!=100)) {
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for N_RB_DL choice: 6,15,25,50,75,100 !\n",
                           RC.config_file_name, i, N_RB_DL);
            }

            if (strcmp(frame_type, "FDD") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).frame_type[j] = FDD;
            } else  if (strcmp(frame_type, "TDD") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).frame_type[j] = TDD;
            } else {
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for frame_type choice: FDD or TDD !\n",
                           RC.config_file_name, i, frame_type);
            }

            RRC_CONFIGURATION_REQ (msg_p).tdd_config[j] = tdd_config;
            AssertFatal (tdd_config <= TDD_Config__subframeAssignment_sa6,
                         "Failed to parse eNB configuration file %s, enb %d illegal tdd_config %d (should be 0-%d)!",
                         RC.config_file_name, i, tdd_config, TDD_Config__subframeAssignment_sa6);
            RRC_CONFIGURATION_REQ (msg_p).tdd_config_s[j] = tdd_config_s;
            AssertFatal (tdd_config_s <= TDD_Config__specialSubframePatterns_ssp8,
                         "Failed to parse eNB configuration file %s, enb %d illegal tdd_config_s %d (should be 0-%d)!",
                         RC.config_file_name, i, tdd_config_s, TDD_Config__specialSubframePatterns_ssp8);

            if (!prefix_type)
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d define %s: NORMAL,EXTENDED!\n",
                           RC.config_file_name, i, ENB_CONFIG_STRING_PREFIX_TYPE);
            else if (strcmp(prefix_type, "NORMAL") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).prefix_type[j] = NORMAL;
            } else  if (strcmp(prefix_type, "EXTENDED") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).prefix_type[j] = EXTENDED;
            } else {
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for prefix_type choice: NORMAL or EXTENDED !\n",
                           RC.config_file_name, i, prefix_type);
            }

            RRC_CONFIGURATION_REQ (msg_p).eutra_band[j] = eutra_band;
            // printf( "\teutra band:\t%d\n",RRC_CONFIGURATION_REQ (msg_p).eutra_band);
            RRC_CONFIGURATION_REQ (msg_p).downlink_frequency[j] = (uint32_t) downlink_frequency;
            //printf( "\tdownlink freq:\t%u\n",RRC_CONFIGURATION_REQ (msg_p).downlink_frequency);
            RRC_CONFIGURATION_REQ (msg_p).uplink_frequency_offset[j] = (unsigned int) uplink_frequency_offset;

            if (config_check_band_frequencies(j,
                                              RRC_CONFIGURATION_REQ (msg_p).eutra_band[j],
                                              RRC_CONFIGURATION_REQ (msg_p).downlink_frequency[j],
                                              RRC_CONFIGURATION_REQ (msg_p).uplink_frequency_offset[j],
                                              RRC_CONFIGURATION_REQ (msg_p).frame_type[j])) {
              AssertFatal(0, "error calling enb_check_band_frequencies\n");
            }

            if ((nb_antenna_ports <1) || (nb_antenna_ports > 2))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for nb_antenna_ports choice: 1..2 !\n",
                           RC.config_file_name, i, nb_antenna_ports);

            RRC_CONFIGURATION_REQ (msg_p).nb_antenna_ports[j] = nb_antenna_ports;

	    // Radio Resource Configuration (SIB2)

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_root =  prach_root;

            if ((prach_root <0) || (prach_root > 1023))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_root choice: 0..1023 !\n",
                           RC.config_file_name, i, prach_root);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_config_index = prach_config_index;

            if ((prach_config_index <0) || (prach_config_index > 63))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_config_index choice: 0..1023 !\n",
                           RC.config_file_name, i, prach_config_index);

            if (!prach_high_speed)
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                           RC.config_file_name, i, ENB_CONFIG_STRING_PRACH_HIGH_SPEED);
            else if (strcmp(prach_high_speed, "ENABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_high_speed = TRUE;
            } else if (strcmp(prach_high_speed, "DISABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_high_speed = FALSE;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for prach_config choice: ENABLE,DISABLE !\n",
                           RC.config_file_name, i, prach_high_speed);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_zero_correlation =prach_zero_correlation;

            if ((prach_zero_correlation <0) || (prach_zero_correlation > 15))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_zero_correlation choice: 0..15!\n",
                           RC.config_file_name, i, prach_zero_correlation);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_freq_offset = prach_freq_offset;

            if ((prach_freq_offset <0) || (prach_freq_offset > 94))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_freq_offset choice: 0..94!\n",
                           RC.config_file_name, i, prach_freq_offset);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_delta_shift = pucch_delta_shift-1;

            if ((pucch_delta_shift <1) || (pucch_delta_shift > 3))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_delta_shift choice: 1..3!\n",
                           RC.config_file_name, i, pucch_delta_shift);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_nRB_CQI = pucch_nRB_CQI;

            if ((pucch_nRB_CQI <0) || (pucch_nRB_CQI > 98))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_nRB_CQI choice: 0..98!\n",
                           RC.config_file_name, i, pucch_nRB_CQI);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_nCS_AN = pucch_nCS_AN;

            if ((pucch_nCS_AN <0) || (pucch_nCS_AN > 7))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_nCS_AN choice: 0..7!\n",
                           RC.config_file_name, i, pucch_nCS_AN);

            //#if (RRC_VERSION < MAKE_VERSION(10, 0, 0))
            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_n1_AN = pucch_n1_AN;

            if ((pucch_n1_AN <0) || (pucch_n1_AN > 2047))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_n1_AN choice: 0..2047!\n",
                           RC.config_file_name, i, pucch_n1_AN);

            //#endif
            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pdsch_referenceSignalPower = pdsch_referenceSignalPower;

            if ((pdsch_referenceSignalPower <-60) || (pdsch_referenceSignalPower > 50))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pdsch_referenceSignalPower choice:-60..50!\n",
                           RC.config_file_name, i, pdsch_referenceSignalPower);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pdsch_p_b = pdsch_p_b;

            if ((pdsch_p_b <0) || (pdsch_p_b > 3))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pdsch_p_b choice: 0..3!\n",
                           RC.config_file_name, i, pdsch_p_b);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_n_SB = pusch_n_SB;

            if ((pusch_n_SB <1) || (pusch_n_SB > 4))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_n_SB choice: 1..4!\n",
                           RC.config_file_name, i, pusch_n_SB);

            if (!pusch_hoppingMode)
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d define %s: interSubframe,intraAndInterSubframe!\n",
                           RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_HOPPINGMODE);
            else if (strcmp(pusch_hoppingMode,"interSubFrame")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_hoppingMode = PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_interSubFrame;
            }  else if (strcmp(pusch_hoppingMode,"intraAndInterSubFrame")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_hoppingMode = PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_intraAndInterSubFrame;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_hoppingMode choice: interSubframe,intraAndInterSubframe!\n",
                           RC.config_file_name, i, pusch_hoppingMode);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_hoppingOffset = pusch_hoppingOffset;

            if ((pusch_hoppingOffset<0) || (pusch_hoppingOffset>98))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_hoppingOffset choice: 0..98!\n",
                           RC.config_file_name, i, pusch_hoppingMode);

            if (!pusch_enable64QAM)
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                           RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_ENABLE64QAM);
            else if (strcmp(pusch_enable64QAM, "ENABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_enable64QAM = TRUE;
            }  else if (strcmp(pusch_enable64QAM, "DISABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_enable64QAM = FALSE;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_enable64QAM choice: ENABLE,DISABLE!\n",
                           RC.config_file_name, i, pusch_enable64QAM);

            if (!pusch_groupHoppingEnabled)
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                           RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_GROUP_HOPPING_EN);
            else if (strcmp(pusch_groupHoppingEnabled, "ENABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_groupHoppingEnabled = TRUE;
            }  else if (strcmp(pusch_groupHoppingEnabled, "DISABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_groupHoppingEnabled= FALSE;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_groupHoppingEnabled choice: ENABLE,DISABLE!\n",
                           RC.config_file_name, i, pusch_groupHoppingEnabled);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_groupAssignment = pusch_groupAssignment;

            if ((pusch_groupAssignment<0)||(pusch_groupAssignment>29))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_groupAssignment choice: 0..29!\n",
                           RC.config_file_name, i, pusch_groupAssignment);

            if (!pusch_sequenceHoppingEnabled)
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                           RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_SEQUENCE_HOPPING_EN);
            else if (strcmp(pusch_sequenceHoppingEnabled, "ENABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_sequenceHoppingEnabled = TRUE;
            }  else if (strcmp(pusch_sequenceHoppingEnabled, "DISABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_sequenceHoppingEnabled = FALSE;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_sequenceHoppingEnabled choice: ENABLE,DISABLE!\n",
                           RC.config_file_name, i, pusch_sequenceHoppingEnabled);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_nDMRS1= pusch_nDMRS1;  //cyclic_shift in RRC!

            if ((pusch_nDMRS1 <0) || (pusch_nDMRS1>7))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_nDMRS1 choice: 0..7!\n",
                           RC.config_file_name, i, pusch_nDMRS1);

            if (strcmp(phich_duration,"NORMAL")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_duration= PHICH_Config__phich_Duration_normal;
            } else if (strcmp(phich_duration,"EXTENDED")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_duration= PHICH_Config__phich_Duration_extended;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for phich_duration choice: NORMAL,EXTENDED!\n",
                           RC.config_file_name, i, phich_duration);

            if (strcmp(phich_resource,"ONESIXTH")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_resource= PHICH_Config__phich_Resource_oneSixth ;
            } else if (strcmp(phich_resource,"HALF")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_resource= PHICH_Config__phich_Resource_half;
            } else if (strcmp(phich_resource,"ONE")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_resource= PHICH_Config__phich_Resource_one;
            } else if (strcmp(phich_resource,"TWO")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_resource= PHICH_Config__phich_Resource_two;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for phich_resource choice: ONESIXTH,HALF,ONE,TWO!\n",
                           RC.config_file_name, i, phich_resource);

            printf("phich.resource %ld (%s), phich.duration %ld (%s)\n",
                   RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_resource,phich_resource,
                   RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_duration,phich_duration);

            if (strcmp(srs_enable, "ENABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_enable= TRUE;
            } else if (strcmp(srs_enable, "DISABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_enable= FALSE;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_BandwidthConfig choice: ENABLE,DISABLE !\n",
                           RC.config_file_name, i, srs_enable);

            if (RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_enable== TRUE) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_BandwidthConfig= srs_BandwidthConfig;

              if ((srs_BandwidthConfig < 0) || (srs_BandwidthConfig >7))
                AssertFatal (0, "Failed to parse eNB configuration file %s, enb %d unknown value %d for srs_BandwidthConfig choice: 0...7\n",
                             RC.config_file_name, i, srs_BandwidthConfig);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_SubframeConfig= srs_SubframeConfig;

              if ((srs_SubframeConfig<0) || (srs_SubframeConfig>15))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for srs_SubframeConfig choice: 0..15 !\n",
                             RC.config_file_name, i, srs_SubframeConfig);

              if (strcmp(srs_ackNackST, "ENABLE") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_ackNackST= TRUE;
              } else if (strcmp(srs_ackNackST, "DISABLE") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_ackNackST= FALSE;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_BandwidthConfig choice: ENABLE,DISABLE !\n",
                             RC.config_file_name, i, srs_ackNackST);

              if (strcmp(srs_MaxUpPts, "ENABLE") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_MaxUpPts= TRUE;
              } else if (strcmp(srs_MaxUpPts, "DISABLE") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_MaxUpPts= FALSE;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_MaxUpPts choice: ENABLE,DISABLE !\n",
                             RC.config_file_name, i, srs_MaxUpPts);
            }

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_p0_Nominal= pusch_p0_Nominal;

            if ((pusch_p0_Nominal<-126) || (pusch_p0_Nominal>24))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_p0_Nominal choice: -126..24 !\n",
                           RC.config_file_name, i, pusch_p0_Nominal);

#if (RRC_VERSION <= MAKE_VERSION(12, 0, 0))

            if (strcmp(pusch_alpha,"AL0")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= UplinkPowerControlCommon__alpha_al0;
            } else if (strcmp(pusch_alpha,"AL04")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= UplinkPowerControlCommon__alpha_al04;
            } else if (strcmp(pusch_alpha,"AL05")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= UplinkPowerControlCommon__alpha_al05;
            } else if (strcmp(pusch_alpha,"AL06")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= UplinkPowerControlCommon__alpha_al06;
            } else if (strcmp(pusch_alpha,"AL07")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= UplinkPowerControlCommon__alpha_al07;
            } else if (strcmp(pusch_alpha,"AL08")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= UplinkPowerControlCommon__alpha_al08;
            } else if (strcmp(pusch_alpha,"AL09")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= UplinkPowerControlCommon__alpha_al09;
            } else if (strcmp(pusch_alpha,"AL1")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= UplinkPowerControlCommon__alpha_al1;
            }

#endif
#if (RRC_VERSION >= MAKE_VERSION(12, 0, 0))

            if (strcmp(pusch_alpha,"AL0")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= Alpha_r12_al0;
            } else if (strcmp(pusch_alpha,"AL04")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= Alpha_r12_al04;
            } else if (strcmp(pusch_alpha,"AL05")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= Alpha_r12_al05;
            } else if (strcmp(pusch_alpha,"AL06")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= Alpha_r12_al06;
            } else if (strcmp(pusch_alpha,"AL07")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= Alpha_r12_al07;
            } else if (strcmp(pusch_alpha,"AL08")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= Alpha_r12_al08;
            } else if (strcmp(pusch_alpha,"AL09")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= Alpha_r12_al09;
            } else if (strcmp(pusch_alpha,"AL1")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= Alpha_r12_al1;
            }

#endif
            else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_Alpha choice: AL0,AL04,AL05,AL06,AL07,AL08,AL09,AL1!\n",
                           RC.config_file_name, i, pusch_alpha);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_p0_Nominal= pucch_p0_Nominal;

            if ((pucch_p0_Nominal<-127) || (pucch_p0_Nominal>-96))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_p0_Nominal choice: -127..-96 !\n",
                           RC.config_file_name, i, pucch_p0_Nominal);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].msg3_delta_Preamble= msg3_delta_Preamble;

            if ((msg3_delta_Preamble<-1) || (msg3_delta_Preamble>6))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for msg3_delta_Preamble choice: -1..6 !\n",
                           RC.config_file_name, i, msg3_delta_Preamble);

            if (strcmp(pucch_deltaF_Format1,"deltaF_2")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format1= DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF_2;
            } else if (strcmp(pucch_deltaF_Format1,"deltaF0")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format1= DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF0;
            } else if (strcmp(pucch_deltaF_Format1,"deltaF2")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format1= DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF2;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format1 choice: deltaF_2,dltaF0,deltaF2!\n",
                           RC.config_file_name, i, pucch_deltaF_Format1);

            if (strcmp(pucch_deltaF_Format1b,"deltaF1")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format1b= DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF1;
            } else if (strcmp(pucch_deltaF_Format1b,"deltaF3")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format1b= DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF3;
            } else if (strcmp(pucch_deltaF_Format1b,"deltaF5")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format1b= DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF5;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format1b choice: deltaF1,dltaF3,deltaF5!\n",
                           RC.config_file_name, i, pucch_deltaF_Format1b);

            if (strcmp(pucch_deltaF_Format2,"deltaF_2")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF_2;
            } else if (strcmp(pucch_deltaF_Format2,"deltaF0")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF0;
            } else if (strcmp(pucch_deltaF_Format2,"deltaF1")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF1;
            } else if (strcmp(pucch_deltaF_Format2,"deltaF2")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF2;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2 choice: deltaF_2,dltaF0,deltaF1,deltaF2!\n",
                           RC.config_file_name, i, pucch_deltaF_Format2);

            if (strcmp(pucch_deltaF_Format2a,"deltaF_2")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2a= DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF_2;
            } else if (strcmp(pucch_deltaF_Format2a,"deltaF0")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2a= DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF0;
            } else if (strcmp(pucch_deltaF_Format2a,"deltaF2")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2a= DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF2;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2a choice: deltaF_2,dltaF0,deltaF2!\n",
                           RC.config_file_name, i, pucch_deltaF_Format2a);

            if (strcmp(pucch_deltaF_Format2b,"deltaF_2")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2b= DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF_2;
            } else if (strcmp(pucch_deltaF_Format2b,"deltaF0")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2b= DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF0;
            } else if (strcmp(pucch_deltaF_Format2b,"deltaF2")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2b= DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF2;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2b choice: deltaF_2,dltaF0,deltaF2!\n",
                           RC.config_file_name, i, pucch_deltaF_Format2b);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_numberOfRA_Preambles= (rach_numberOfRA_Preambles/4)-1;

            if ((rach_numberOfRA_Preambles <4) || (rach_numberOfRA_Preambles>64) || ((rach_numberOfRA_Preambles&3)!=0))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_numberOfRA_Preambles choice: 4,8,12,...,64!\n",
                           RC.config_file_name, i, rach_numberOfRA_Preambles);

            if (strcmp(rach_preamblesGroupAConfig, "ENABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preamblesGroupAConfig= TRUE;
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_sizeOfRA_PreamblesGroupA= (rach_sizeOfRA_PreamblesGroupA/4)-1;

              if ((rach_numberOfRA_Preambles <4) || (rach_numberOfRA_Preambles>60) || ((rach_numberOfRA_Preambles&3)!=0))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_sizeOfRA_PreamblesGroupA choice: 4,8,12,...,60!\n",
                             RC.config_file_name, i, rach_sizeOfRA_PreamblesGroupA);

              switch (rach_messageSizeGroupA) {
	      case 56:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b56;
		break;

	      case 144:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b144;
		break;

	      case 208:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b208;
		break;

	      case 256:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b256;
		break;

	      default:
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_messageSizeGroupA choice: 56,144,208,256!\n",
			     RC.config_file_name, i, rach_messageSizeGroupA);
		break;
              }

              if (strcmp(rach_messagePowerOffsetGroupB,"minusinfinity")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_minusinfinity;
              } else if (strcmp(rach_messagePowerOffsetGroupB,"dB0")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB0;
              } else if (strcmp(rach_messagePowerOffsetGroupB,"dB5")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB5;
              } else if (strcmp(rach_messagePowerOffsetGroupB,"dB8")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB8;
              } else if (strcmp(rach_messagePowerOffsetGroupB,"dB10")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB10;
              } else if (strcmp(rach_messagePowerOffsetGroupB,"dB12")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB12;
              } else if (strcmp(rach_messagePowerOffsetGroupB,"dB15")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB15;
              } else if (strcmp(rach_messagePowerOffsetGroupB,"dB18")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB18;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rach_messagePowerOffsetGroupB choice: minusinfinity,dB0,dB5,dB8,dB10,dB12,dB15,dB18!\n",
                             RC.config_file_name, i, rach_messagePowerOffsetGroupB);
            } else if (strcmp(rach_preamblesGroupAConfig, "DISABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preamblesGroupAConfig= FALSE;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rach_preamblesGroupAConfig choice: ENABLE,DISABLE !\n",
                           RC.config_file_name, i, rach_preamblesGroupAConfig);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleInitialReceivedTargetPower= (rach_preambleInitialReceivedTargetPower+120)/2;

            if ((rach_preambleInitialReceivedTargetPower<-120) || (rach_preambleInitialReceivedTargetPower>-90) || ((rach_preambleInitialReceivedTargetPower&1)!=0))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_preambleInitialReceivedTargetPower choice: -120,-118,...,-90 !\n",
                           RC.config_file_name, i, rach_preambleInitialReceivedTargetPower);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_powerRampingStep= rach_powerRampingStep/2;

            if ((rach_powerRampingStep<0) || (rach_powerRampingStep>6) || ((rach_powerRampingStep&1)!=0))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_powerRampingStep choice: 0,2,4,6 !\n",
                           RC.config_file_name, i, rach_powerRampingStep);

            switch (rach_preambleTransMax) {
#if (RRC_VERSION < MAKE_VERSION(14, 0, 0))

	    case 3:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n3;
	      break;

	    case 4:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n4;
	      break;

	    case 5:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n5;
	      break;

	    case 6:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n6;
	      break;

	    case 7:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n7;
	      break;

	    case 8:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n8;
	      break;

	    case 10:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n10;
	      break;

	    case 20:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n20;
	      break;

	    case 50:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n50;
	      break;

	    case 100:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n100;
	      break;

	    case 200:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n200;
	      break;
#else

	    case 3:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n3;
	      break;

	    case 4:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n4;
	      break;

	    case 5:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n5;
	      break;

	    case 6:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n6;
	      break;

	    case 7:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n7;
	      break;

	    case 8:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n8;
	      break;

	    case 10:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n10;
	      break;

	    case 20:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n20;
	      break;

	    case 50:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n50;
	      break;

	    case 100:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n100;
	      break;

	    case 200:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax=  PreambleTransMax_n200;
	      break;
#endif

	    default:
	      AssertFatal (0,
			   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_preambleTransMax choice: 3,4,5,6,7,8,10,20,50,100,200!\n",
			   RC.config_file_name, i, rach_preambleTransMax);
	      break;
            }

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_raResponseWindowSize=  (rach_raResponseWindowSize==10)?7:rach_raResponseWindowSize-2;

            if ((rach_raResponseWindowSize<0)||(rach_raResponseWindowSize==9)||(rach_raResponseWindowSize>10))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_raResponseWindowSize choice: 2,3,4,5,6,7,8,10!\n",
                           RC.config_file_name, i, rach_preambleTransMax);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_macContentionResolutionTimer= (rach_macContentionResolutionTimer/8)-1;

            if ((rach_macContentionResolutionTimer<8) || (rach_macContentionResolutionTimer>64) || ((rach_macContentionResolutionTimer&7)!=0))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_macContentionResolutionTimer choice: 8,16,...,56,64!\n",
                           RC.config_file_name, i, rach_preambleTransMax);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_maxHARQ_Msg3Tx= rach_maxHARQ_Msg3Tx;

            if ((rach_maxHARQ_Msg3Tx<0) || (rach_maxHARQ_Msg3Tx>8))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_maxHARQ_Msg3Tx choice: 1..8!\n",
                           RC.config_file_name, i, rach_preambleTransMax);

            switch (pcch_defaultPagingCycle) {
	    case 32:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_defaultPagingCycle= PCCH_Config__defaultPagingCycle_rf32;
	      break;

	    case 64:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_defaultPagingCycle= PCCH_Config__defaultPagingCycle_rf64;
	      break;

	    case 128:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_defaultPagingCycle= PCCH_Config__defaultPagingCycle_rf128;
	      break;

	    case 256:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_defaultPagingCycle= PCCH_Config__defaultPagingCycle_rf256;
	      break;

	    default:
	      AssertFatal (0,
			   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pcch_defaultPagingCycle choice: 32,64,128,256!\n",
			   RC.config_file_name, i, pcch_defaultPagingCycle);
	      break;
            }

            if (strcmp(pcch_nB, "fourT") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= PCCH_Config__nB_fourT;
            } else if (strcmp(pcch_nB, "twoT") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= PCCH_Config__nB_twoT;
            } else if (strcmp(pcch_nB, "oneT") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= PCCH_Config__nB_oneT;
            } else if (strcmp(pcch_nB, "halfT") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= PCCH_Config__nB_halfT;
            } else if (strcmp(pcch_nB, "quarterT") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= PCCH_Config__nB_quarterT;
            } else if (strcmp(pcch_nB, "oneEighthT") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= PCCH_Config__nB_oneEighthT;
            } else if (strcmp(pcch_nB, "oneSixteenthT") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= PCCH_Config__nB_oneSixteenthT;
            } else if (strcmp(pcch_nB, "oneThirtySecondT") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= PCCH_Config__nB_oneThirtySecondT;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pcch_nB choice: fourT,twoT,oneT,halfT,quarterT,oneighthT,oneSixteenthT,oneThirtySecondT !\n",
                           RC.config_file_name, i, pcch_defaultPagingCycle);

            switch (bcch_modificationPeriodCoeff) {
	    case 2:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].bcch_modificationPeriodCoeff= BCCH_Config__modificationPeriodCoeff_n2;
	      break;

	    case 4:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].bcch_modificationPeriodCoeff= BCCH_Config__modificationPeriodCoeff_n4;
	      break;

	    case 8:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].bcch_modificationPeriodCoeff= BCCH_Config__modificationPeriodCoeff_n8;
	      break;

	    case 16:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].bcch_modificationPeriodCoeff= BCCH_Config__modificationPeriodCoeff_n16;
	      break;

	    default:
	      AssertFatal (0,
			   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for bcch_modificationPeriodCoeff choice: 2,4,8,16",
			   RC.config_file_name, i, bcch_modificationPeriodCoeff);
	      break;
            }

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t300= ue_TimersAndConstants_t300;
            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t301= ue_TimersAndConstants_t301;
            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t310= ue_TimersAndConstants_t310;
            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t311= ue_TimersAndConstants_t311;
            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n310= ue_TimersAndConstants_n310;
            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n311= ue_TimersAndConstants_n311;

            switch (ue_TransmissionMode) {
	    case 1:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm1;
	      break;

	    case 2:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm2;
	      break;

	    case 3:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm3;
	      break;

	    case 4:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm4;
	      break;

	    case 5:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm5;
	      break;

	    case 6:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm6;
	      break;

	    case 7:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm7;
	      break;

	    default:
	      AssertFatal (0,
			   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TransmissionMode choice: 1,2,3,4,5,6,7",
			   RC.config_file_name, i, ue_TransmissionMode);
	      break;
            }

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_multiple_max= ue_multiple_max;

            switch (N_RB_DL) {
	    case 25:
	      if ((ue_multiple_max < 1) || (ue_multiple_max > 4))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_multiple_max choice: 1..4!\n",
			     RC.config_file_name, i, ue_multiple_max);

	      break;

	    case 50:
	      if ((ue_multiple_max < 1) || (ue_multiple_max > 8))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_multiple_max choice: 1..8!\n",
			     RC.config_file_name, i, ue_multiple_max);

	      break;

	    case 100:
	      if ((ue_multiple_max < 1) || (ue_multiple_max > 16))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_multiple_max choice: 1..16!\n",
			     RC.config_file_name, i, ue_multiple_max);

	      break;

	    default:
	      AssertFatal (0,
			   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for N_RB_DL choice: 25,50,100 !\n",
			   RC.config_file_name, i, N_RB_DL);
	      break;
            }

	    char brparamspath[MAX_OPTNAME_SIZE*2 + 16];
	    sprintf(brparamspath,"%s.%s", ccspath, ENB_CONFIG_STRING_COMPONENT_BR_PARAMETERS);
	    config_get( brParams, sizeof(brParams)/sizeof(paramdef_t), brparamspath);
	    RRC_CONFIGURATION_REQ(msg_p).eMTC_configured = eMTC_configured&1;

	    if (eMTC_configured > 0) { 
	      printf("Found parameters for eMTC\n");
	      RRC_CONFIGURATION_REQ(msg_p).schedulingInfoSIB1_BR_r13[j] = schedulingInfoSIB1_BR_r13;
	      
	      
	      if (!strcmp(cellSelectionInfoCE_r13, "ENABLE")) {
		RRC_CONFIGURATION_REQ(msg_p).cellSelectionInfoCE_r13[j] = TRUE;
		RRC_CONFIGURATION_REQ(msg_p).q_RxLevMinCE_r13[j]= q_RxLevMinCE_r13;
		//                            RRC_CONFIGURATION_REQ(msg_p).q_QualMinRSRQ_CE_r13[j]= calloc(1, sizeof(long));
		//                            *RRC_CONFIGURATION_REQ(msg_p).q_QualMinRSRQ_CE_r13[j]= q_QualMinRSRQ_CE_r13;
	      } else {
		RRC_CONFIGURATION_REQ(msg_p).cellSelectionInfoCE_r13[j] = FALSE;
	      }
	      
	      
	      
	      if (!strcmp(bandwidthReducedAccessRelatedInfo_r13, "ENABLE")) {
		RRC_CONFIGURATION_REQ(msg_p).bandwidthReducedAccessRelatedInfo_r13[j] = TRUE;
		
		
		
		if (!strcmp(si_WindowLength_BR_r13, "ms20")) {
		  RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[j] = 0;
		} else if (!strcmp(si_WindowLength_BR_r13, "ms40")) {
		  RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[j] = 1;
		} else if (!strcmp(si_WindowLength_BR_r13, "ms60")) {
		  RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[j] = 2;
		} else if (!strcmp(si_WindowLength_BR_r13, "ms80")) {
		  RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[j] = 3;
		} else if (!strcmp(si_WindowLength_BR_r13, "ms120")) {
		  RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[j] = 4;
		} else if (!strcmp(si_WindowLength_BR_r13, "ms160")) {
		  RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[j] = 5;
		} else if (!strcmp(si_WindowLength_BR_r13, "ms200")) {
		  RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[j] = 6;
		} else if (!strcmp(si_WindowLength_BR_r13, "spare")) {
		  RRC_CONFIGURATION_REQ(msg_p).si_WindowLength_BR_r13[j] = 7;
		}
		
		
		if (!strcmp(si_RepetitionPattern_r13, "everyRF")) {
		  RRC_CONFIGURATION_REQ(msg_p).si_RepetitionPattern_r13[j] = 0;
		} else if (!strcmp(si_RepetitionPattern_r13, "every2ndRF")) {
		  RRC_CONFIGURATION_REQ(msg_p).si_RepetitionPattern_r13[j] = 1;
		} else if (!strcmp(si_RepetitionPattern_r13, "every4thRF")) {
		  RRC_CONFIGURATION_REQ(msg_p).si_RepetitionPattern_r13[j] = 2;
		} else if (!strcmp(si_RepetitionPattern_r13, "every8thRF")) {
		  RRC_CONFIGURATION_REQ(msg_p).si_RepetitionPattern_r13[j] = 3;
		}
		
	      } else {
		RRC_CONFIGURATION_REQ(msg_p).bandwidthReducedAccessRelatedInfo_r13[j] = FALSE;
	      }
	      
	      char schedulingInfoBrPath[MAX_OPTNAME_SIZE * 2];
	      config_getlist(&schedulingInfoBrParamList, NULL, 0, brparamspath);
	      RRC_CONFIGURATION_REQ (msg_p).scheduling_info_br_size[j] = schedulingInfoBrParamList.numelt;
	      int siInfoindex;
	      for (siInfoindex = 0; siInfoindex < schedulingInfoBrParamList.numelt; siInfoindex++) {
		sprintf(schedulingInfoBrPath, "%s.%s.[%i]", brparamspath, ENB_CONFIG_STRING_SCHEDULING_INFO_LIST, siInfoindex);
		config_get(schedulingInfoBrParams, sizeof(schedulingInfoBrParams) / sizeof(paramdef_t), schedulingInfoBrPath);
		RRC_CONFIGURATION_REQ (msg_p).si_Narrowband_r13[j][siInfoindex] = si_Narrowband_r13;
		RRC_CONFIGURATION_REQ (msg_p).si_TBS_r13[j][siInfoindex] = si_TBS_r13;
	      }
	      
	      
	      
	      //                        RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].system_info_value_tag_SI_size[j] = 0;
	      
	      
	      RRC_CONFIGURATION_REQ(msg_p).fdd_DownlinkOrTddSubframeBitmapBR_r13[j] = CALLOC(1, sizeof(BOOLEAN_t));
	      if (!strcmp(fdd_DownlinkOrTddSubframeBitmapBR_r13, "subframePattern40-r13")) {
		*RRC_CONFIGURATION_REQ(msg_p).fdd_DownlinkOrTddSubframeBitmapBR_r13[j] = FALSE;
		RRC_CONFIGURATION_REQ(msg_p).fdd_DownlinkOrTddSubframeBitmapBR_val_r13[j] = fdd_DownlinkOrTddSubframeBitmapBR_val_r13;
	      } else {
		*RRC_CONFIGURATION_REQ(msg_p).fdd_DownlinkOrTddSubframeBitmapBR_r13[j] = TRUE;
		RRC_CONFIGURATION_REQ(msg_p).fdd_DownlinkOrTddSubframeBitmapBR_val_r13[j] = fdd_DownlinkOrTddSubframeBitmapBR_val_r13;
	      }
	      
	      RRC_CONFIGURATION_REQ(msg_p).startSymbolBR_r13[j] = startSymbolBR_r13;
	      
	      
	      if (!strcmp(si_HoppingConfigCommon_r13, "off")) {
		RRC_CONFIGURATION_REQ(msg_p).si_HoppingConfigCommon_r13[j] = 1;
	      } else if (!strcmp(si_HoppingConfigCommon_r13, "on")) {
		RRC_CONFIGURATION_REQ(msg_p).si_HoppingConfigCommon_r13[j] = 0;
	      }
	      
	      
	      RRC_CONFIGURATION_REQ(msg_p).si_ValidityTime_r13[j] = calloc(1, sizeof(long));
	      if (!strcmp(si_ValidityTime_r13, "true")) {
		*RRC_CONFIGURATION_REQ(msg_p).si_ValidityTime_r13[j] = 0;
	      } else {
		AssertFatal(0,
			    "Failed to parse eNB configuration file %s, enb %d  si_ValidityTime_r13 unknown value!\n",
			    RC.config_file_name, i);
	      }
	      
	      
	      if (!strcmp(freqHoppingParametersDL_r13, "ENABLE"))
		{
		  RRC_CONFIGURATION_REQ(msg_p).freqHoppingParametersDL_r13[j] = TRUE;
		  
		  if (!strcmp(interval_DLHoppingConfigCommonModeA_r13, "interval-TDD-r13"))
		    RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeA_r13[j] = FALSE;
		  else
		    RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeA_r13[j] = TRUE;
		  RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeA_r13_val[j] = interval_DLHoppingConfigCommonModeA_r13_val;
		  
		  if (!strcmp(interval_DLHoppingConfigCommonModeB_r13, "interval-TDD-r13"))
		    RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeB_r13[j] = FALSE;
		  else
		    RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeB_r13[j] = TRUE;
		  RRC_CONFIGURATION_REQ(msg_p).interval_DLHoppingConfigCommonModeB_r13_val[j] = interval_DLHoppingConfigCommonModeB_r13_val;
		  
		  RRC_CONFIGURATION_REQ(msg_p).mpdcch_pdsch_HoppingNB_r13[j] = calloc(1, sizeof(long));
		  if (!strcmp(mpdcch_pdsch_HoppingNB_r13, "nb2")) {
		    *RRC_CONFIGURATION_REQ(msg_p).mpdcch_pdsch_HoppingNB_r13[j] = 0;
		  } else if (!strcmp(mpdcch_pdsch_HoppingNB_r13, "nb4")) {
		    *RRC_CONFIGURATION_REQ(msg_p).mpdcch_pdsch_HoppingNB_r13[j] = 1;
		  } else {
		    AssertFatal(0,
				"Failed to parse eNB configuration file %s, enb %d  mpdcch_pdsch_HoppingNB_r13 unknown value!\n",
				RC.config_file_name, i);
		}


		  RRC_CONFIGURATION_REQ(msg_p).mpdcch_pdsch_HoppingOffset_r13[j] = calloc(1, sizeof(long));
		  *RRC_CONFIGURATION_REQ(msg_p).mpdcch_pdsch_HoppingOffset_r13[j] = mpdcch_pdsch_HoppingOffset_r13;
		  
		}
	      else
		{
		  RRC_CONFIGURATION_REQ(msg_p).freqHoppingParametersDL_r13[j] = FALSE;
		}
	      
	      /** ------------------------------SIB2/3 BR------------------------------------------ */
	      
	      
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].prach_root =  prach_root_emtc;
	      
	      if ((prach_root_emtc <0) || (prach_root_emtc > 1023))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_root choice: 0..1023 !\n",
			     RC.config_file_name, i, prach_root_emtc);
	      
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].prach_config_index = prach_config_index_emtc;
	      
	      if ((prach_config_index_emtc <0) || (prach_config_index_emtc > 63))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_config_index choice: 0..1023 !\n",
			     RC.config_file_name, i, prach_config_index);
	      
	      if (!prach_high_speed_emtc)
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
			     RC.config_file_name, i, ENB_CONFIG_STRING_PRACH_HIGH_SPEED);
	      else if (strcmp(prach_high_speed_emtc, "ENABLE") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].prach_high_speed = TRUE;
	      } else if (strcmp(prach_high_speed_emtc, "DISABLE") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].prach_high_speed = FALSE;
	      } else
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for prach_config choice: ENABLE,DISABLE !\n",
			     RC.config_file_name, i, prach_high_speed_emtc);
	      
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].prach_zero_correlation =prach_zero_correlation_emtc;
	      
	      if ((prach_zero_correlation_emtc <0) || (prach_zero_correlation_emtc > 15))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_zero_correlation choice: 0..15!\n",
			     RC.config_file_name, i, prach_zero_correlation_emtc);
	      
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].prach_freq_offset = prach_freq_offset_emtc;
	      
	      if ((prach_freq_offset_emtc <0) || (prach_freq_offset_emtc > 94))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_freq_offset choice: 0..94!\n",
			     RC.config_file_name, i, prach_freq_offset);
	      
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_delta_shift = pucch_delta_shift_emtc-1;
	      
	      if ((pucch_delta_shift_emtc <1) || (pucch_delta_shift_emtc > 3))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_delta_shift choice: 1..3!\n",
			     RC.config_file_name, i, pucch_delta_shift_emtc);
	      
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_nRB_CQI = pucch_nRB_CQI_emtc;
	      
	      if ((pucch_nRB_CQI_emtc <0) || (pucch_nRB_CQI_emtc > 98))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_nRB_CQI choice: 0..98!\n",
			     RC.config_file_name, i, pucch_nRB_CQI_emtc);
	      
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_nCS_AN = pucch_nCS_AN_emtc;
	      
	      if ((pucch_nCS_AN_emtc <0) || (pucch_nCS_AN_emtc > 7))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_nCS_AN choice: 0..7!\n",
			     RC.config_file_name, i, pucch_nCS_AN_emtc);
	      
	      //#if (RRC_VERSION < MAKE_VERSION(10, 0, 0))
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_n1_AN = pucch_n1_AN_emtc;
	      
	      if ((pucch_n1_AN_emtc <0) || (pucch_n1_AN_emtc > 2047))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_n1_AN choice: 0..2047!\n",
			     RC.config_file_name, i, pucch_n1_AN_emtc);
	      
	      //#endif
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pdsch_referenceSignalPower = pdsch_referenceSignalPower_emtc;
	      
	      if ((pdsch_referenceSignalPower_emtc <-60) || (pdsch_referenceSignalPower_emtc > 50))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pdsch_referenceSignalPower choice:-60..50!\n",
			     RC.config_file_name, i, pdsch_referenceSignalPower);
	      
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pdsch_p_b = pdsch_p_b_emtc;
	      
	      if ((pdsch_p_b_emtc <0) || (pdsch_p_b_emtc > 3))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pdsch_p_b choice: 0..3!\n",
			     RC.config_file_name, i, pdsch_p_b_emtc);
	      
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_n_SB = pusch_n_SB_emtc;
	      
	      if ((pusch_n_SB_emtc <1) || (pusch_n_SB_emtc > 4))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_n_SB choice: 1..4!\n",
			     RC.config_file_name, i, pusch_n_SB_emtc);
	      
	      if (!pusch_hoppingMode_emtc)
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d define %s: interSubframe,intraAndInterSubframe!\n",
			     RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_HOPPINGMODE);
	      else if (strcmp(pusch_hoppingMode_emtc,"interSubFrame")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_hoppingMode = PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_interSubFrame;
	      }  else if (strcmp(pusch_hoppingMode_emtc,"intraAndInterSubFrame")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_hoppingMode = PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_intraAndInterSubFrame;
	      } else
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_hoppingMode choice: interSubframe,intraAndInterSubframe!\n",
			     RC.config_file_name, i, pusch_hoppingMode_emtc);
	      
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_hoppingOffset = pusch_hoppingOffset_emtc;
	      
	      if ((pusch_hoppingOffset_emtc<0) || (pusch_hoppingOffset_emtc>98))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_hoppingOffset choice: 0..98!\n",
			     RC.config_file_name, i, pusch_hoppingMode_emtc);
	      
	      if (!pusch_enable64QAM_emtc)
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
			     RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_ENABLE64QAM);
	      else if (strcmp(pusch_enable64QAM_emtc, "ENABLE") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_enable64QAM = TRUE;
	      }  else if (strcmp(pusch_enable64QAM_emtc, "DISABLE") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_enable64QAM = FALSE;
	      } else
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_enable64QAM choice: ENABLE,DISABLE!\n",
			     RC.config_file_name, i, pusch_enable64QAM_emtc);
	      
	      if (!pusch_groupHoppingEnabled_emtc)
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
			     RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_GROUP_HOPPING_EN);
	      else if (strcmp(pusch_groupHoppingEnabled_emtc, "ENABLE") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_groupHoppingEnabled = TRUE;
	      }  else if (strcmp(pusch_groupHoppingEnabled_emtc, "DISABLE") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_groupHoppingEnabled= FALSE;
	      } else
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_groupHoppingEnabled choice: ENABLE,DISABLE!\n",
			     RC.config_file_name, i, pusch_groupHoppingEnabled_emtc);
	      
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_groupAssignment = pusch_groupAssignment_emtc;
	      
	      if ((pusch_groupAssignment_emtc<0)||(pusch_groupAssignment_emtc>29))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_groupAssignment choice: 0..29!\n",
			     RC.config_file_name, i, pusch_groupAssignment_emtc);
	      
	      if (!pusch_sequenceHoppingEnabled_emtc)
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
			     RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_SEQUENCE_HOPPING_EN);
	      else if (strcmp(pusch_sequenceHoppingEnabled_emtc, "ENABLE") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_sequenceHoppingEnabled = TRUE;
	      }  else if (strcmp(pusch_sequenceHoppingEnabled_emtc, "DISABLE") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_sequenceHoppingEnabled = FALSE;
	      } else
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_sequenceHoppingEnabled choice: ENABLE,DISABLE!\n",
			     RC.config_file_name, i, pusch_sequenceHoppingEnabled_emtc);
	      
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_nDMRS1= pusch_nDMRS1_emtc;  //cyclic_shift in RRC!
	      
	      if ((pusch_nDMRS1_emtc <0) || (pusch_nDMRS1_emtc>7))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_nDMRS1 choice: 0..7!\n",
			     RC.config_file_name, i, pusch_nDMRS1_emtc);
	      
	      if (strcmp(phich_duration_emtc,"NORMAL")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].phich_duration= PHICH_Config__phich_Duration_normal;
	      } else if (strcmp(phich_duration_emtc,"EXTENDED")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].phich_duration= PHICH_Config__phich_Duration_extended;
	      } else
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for phich_duration choice: NORMAL,EXTENDED!\n",
			     RC.config_file_name, i, phich_duration_emtc);
	      
	      if (strcmp(phich_resource_emtc,"ONESIXTH")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].phich_resource= PHICH_Config__phich_Resource_oneSixth ;
	      } else if (strcmp(phich_resource_emtc,"HALF")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].phich_resource= PHICH_Config__phich_Resource_half;
	      } else if (strcmp(phich_resource_emtc,"ONE")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].phich_resource= PHICH_Config__phich_Resource_one;
	      } else if (strcmp(phich_resource_emtc,"TWO")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].phich_resource= PHICH_Config__phich_Resource_two;
	      } else
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for phich_resource choice: ONESIXTH,HALF,ONE,TWO!\n",
			     RC.config_file_name, i, phich_resource_emtc);
	      
	      printf("phich.resource eMTC %ld (%s), phich.duration eMTC %ld (%s)\n",
		     RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].phich_resource,phich_resource_emtc,
		     RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].phich_duration,phich_duration_emtc);
	      
	      if (strcmp(srs_enable_emtc, "ENABLE") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].srs_enable= TRUE;
	      } else if (strcmp(srs_enable_emtc, "DISABLE") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].srs_enable= FALSE;
	      } else
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_BandwidthConfig choice: ENABLE,DISABLE !\n",
			     RC.config_file_name, i, srs_enable_emtc);
	      
	      if (RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].srs_enable== TRUE) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].srs_BandwidthConfig= srs_BandwidthConfig_emtc;
		
		if ((srs_BandwidthConfig_emtc < 0) || (srs_BandwidthConfig_emtc >7))
		  AssertFatal (0, "Failed to parse eNB configuration file %s, enb %d unknown value %d for srs_BandwidthConfig choice: 0...7\n",
			       RC.config_file_name, i, srs_BandwidthConfig_emtc);
		
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].srs_SubframeConfig= srs_SubframeConfig_emtc;
		
		if ((srs_SubframeConfig_emtc<0) || (srs_SubframeConfig_emtc>15))
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for srs_SubframeConfig choice: 0..15 !\n",
			       RC.config_file_name, i, srs_SubframeConfig_emtc);
		
		if (strcmp(srs_ackNackST_emtc, "ENABLE") == 0) {
		  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].srs_ackNackST= TRUE;
		} else if (strcmp(srs_ackNackST, "DISABLE") == 0) {
		  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].srs_ackNackST= FALSE;
		} else
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_BandwidthConfig choice: ENABLE,DISABLE !\n",
			       RC.config_file_name, i, srs_ackNackST_emtc);
		
		if (strcmp(srs_MaxUpPts_emtc, "ENABLE") == 0) {
		  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].srs_MaxUpPts= TRUE;
		} else if (strcmp(srs_MaxUpPts_emtc, "DISABLE") == 0) {
		  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].srs_MaxUpPts= FALSE;
		} else
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_MaxUpPts choice: ENABLE,DISABLE !\n",
			       RC.config_file_name, i, srs_MaxUpPts_emtc);
	      }
	      
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_p0_Nominal= pusch_p0_Nominal_emtc;
	      
	      if ((pusch_p0_Nominal_emtc<-126) || (pusch_p0_Nominal_emtc>24))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_p0_Nominal choice: -126..24 !\n",
			     RC.config_file_name, i, pusch_p0_Nominal);
	      
#if (RRC_VERSION <= MAKE_VERSION(12, 0, 0))
	      
	      if (strcmp(pusch_alpha_emtc,"AL0")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_alpha= UplinkPowerControlCommon__alpha_al0;
	      } else if (strcmp(pusch_alpha_emtc,"AL04")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_alpha= UplinkPowerControlCommon__alpha_al04;
	      } else if (strcmp(pusch_alpha_emtc,"AL05")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_alpha= UplinkPowerControlCommon__alpha_al05;
	      } else if (strcmp(pusch_alpha_emtc,"AL06")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_alpha= UplinkPowerControlCommon__alpha_al06;
	      } else if (strcmp(pusch_alpha_emtc,"AL07")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_alpha= UplinkPowerControlCommon__alpha_al07;
	      } else if (strcmp(pusch_alpha_emtc,"AL08")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_alpha= UplinkPowerControlCommon__alpha_al08;
	      } else if (strcmp(pusch_alpha_emtc,"AL09")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_alpha= UplinkPowerControlCommon__alpha_al09;
	      } else if (strcmp(pusch_alpha_emtc,"AL1")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_alpha= UplinkPowerControlCommon__alpha_al1;
	      }
	      
#endif
#if (RRC_VERSION >= MAKE_VERSION(12, 0, 0))
	      
	      if (strcmp(pusch_alpha_emtc,"AL0")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_alpha= Alpha_r12_al0;
	      } else if (strcmp(pusch_alpha_emtc,"AL04")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_alpha= Alpha_r12_al04;
	      } else if (strcmp(pusch_alpha_emtc,"AL05")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_alpha= Alpha_r12_al05;
	      } else if (strcmp(pusch_alpha_emtc,"AL06")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_alpha= Alpha_r12_al06;
	      } else if (strcmp(pusch_alpha_emtc,"AL07")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_alpha= Alpha_r12_al07;
	      } else if (strcmp(pusch_alpha_emtc,"AL08")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_alpha= Alpha_r12_al08;
	      } else if (strcmp(pusch_alpha_emtc,"AL09")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_alpha= Alpha_r12_al09;
	      } else if (strcmp(pusch_alpha_emtc,"AL1")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pusch_alpha= Alpha_r12_al1;
	      }
	      
#endif
	      else
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_Alpha choice: AL0,AL04,AL05,AL06,AL07,AL08,AL09,AL1!\n",
			     RC.config_file_name, i, pusch_alpha);
	      
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_p0_Nominal= pucch_p0_Nominal_emtc;
	      
	      if ((pucch_p0_Nominal_emtc<-127) || (pucch_p0_Nominal_emtc>-96))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_p0_Nominal choice: -127..-96 !\n",
			     RC.config_file_name, i, pucch_p0_Nominal_emtc);
	      
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].msg3_delta_Preamble= msg3_delta_Preamble_emtc;
	      
	      if ((msg3_delta_Preamble_emtc<-1) || (msg3_delta_Preamble_emtc>6))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for msg3_delta_Preamble choice: -1..6 !\n",
			     RC.config_file_name, i, msg3_delta_Preamble_emtc);
	      
	      if (strcmp(pucch_deltaF_Format1_emtc,"deltaF_2")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format1= DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF_2;
	      } else if (strcmp(pucch_deltaF_Format1_emtc,"deltaF0")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format1= DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF0;
	      } else if (strcmp(pucch_deltaF_Format1_emtc,"deltaF2")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format1= DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF2;
	      } else
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format1 choice: deltaF_2,dltaF0,deltaF2!\n",
			     RC.config_file_name, i, pucch_deltaF_Format1_emtc);
	      
	      if (strcmp(pucch_deltaF_Format1b_emtc,"deltaF1")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format1b= DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF1;
	      } else if (strcmp(pucch_deltaF_Format1b_emtc,"deltaF3")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format1b= DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF3;
	      } else if (strcmp(pucch_deltaF_Format1b_emtc,"deltaF5")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format1b= DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF5;
	      } else
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format1b choice: deltaF1,dltaF3,deltaF5!\n",
			     RC.config_file_name, i, pucch_deltaF_Format1b_emtc);
	      
	      if (strcmp(pucch_deltaF_Format2_emtc,"deltaF_2")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF_2;
	      } else if (strcmp(pucch_deltaF_Format2_emtc,"deltaF0")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF0;
	      } else if (strcmp(pucch_deltaF_Format2_emtc,"deltaF1")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF1;
	      } else if (strcmp(pucch_deltaF_Format2_emtc,"deltaF2")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2= DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF2;
	      } else
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2 choice: deltaF_2,dltaF0,deltaF1,deltaF2!\n",
			     RC.config_file_name, i, pucch_deltaF_Format2_emtc);
	      
	      if (strcmp(pucch_deltaF_Format2a_emtc,"deltaF_2")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2a= DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF_2;
	      } else if (strcmp(pucch_deltaF_Format2a_emtc,"deltaF0")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2a= DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF0;
	      } else if (strcmp(pucch_deltaF_Format2a_emtc,"deltaF2")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2a= DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF2;
	      } else
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2a choice: deltaF_2,dltaF0,deltaF2!\n",
			     RC.config_file_name, i, pucch_deltaF_Format2a_emtc);
	      
	      if (strcmp(pucch_deltaF_Format2b_emtc,"deltaF_2")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2b= DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF_2;
	      } else if (strcmp(pucch_deltaF_Format2b_emtc,"deltaF0")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2b= DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF0;
	      } else if (strcmp(pucch_deltaF_Format2b_emtc,"deltaF2")==0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pucch_deltaF_Format2b= DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF2;
	      } else
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2b choice: deltaF_2,dltaF0,deltaF2!\n",
			     RC.config_file_name, i, pucch_deltaF_Format2b_emtc);
	      
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_numberOfRA_Preambles= (rach_numberOfRA_Preambles_emtc/4)-1;
	      
	      if ((rach_numberOfRA_Preambles_emtc <4) || (rach_numberOfRA_Preambles_emtc >64) || ((rach_numberOfRA_Preambles_emtc&3)!=0))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_numberOfRA_Preambles choice: 4,8,12,...,64!\n",
			     RC.config_file_name, i, rach_numberOfRA_Preambles_emtc);
	      
	      if (strcmp(rach_preamblesGroupAConfig_emtc, "ENABLE") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preamblesGroupAConfig= TRUE;
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_sizeOfRA_PreamblesGroupA= (rach_sizeOfRA_PreamblesGroupA_emtc/4)-1;
		
		if ((rach_numberOfRA_Preambles_emtc <4) || (rach_numberOfRA_Preambles_emtc>60) || ((rach_numberOfRA_Preambles_emtc&3)!=0))
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_sizeOfRA_PreamblesGroupA choice: 4,8,12,...,60!\n",
			       RC.config_file_name, i, rach_sizeOfRA_PreamblesGroupA_emtc);
		
		switch (rach_messageSizeGroupA_emtc) {
		case 56:
		  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b56;
		  break;
		  
		case 144:
		  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b144;
		  break;
		  
		case 208:
		  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b208;
		  break;
		  
		case 256:
		  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messageSizeGroupA= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b256;
		  break;
		  
		default:
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_messageSizeGroupA choice: 56,144,208,256!\n",
			       RC.config_file_name, i, rach_messageSizeGroupA_emtc);
		  break;
		}
		
		if (strcmp(rach_messagePowerOffsetGroupB_emtc,"minusinfinity")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_minusinfinity;
		} else if (strcmp(rach_messagePowerOffsetGroupB_emtc,"dB0")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB0;
		} else if (strcmp(rach_messagePowerOffsetGroupB_emtc,"dB5")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB5;
		} else if (strcmp(rach_messagePowerOffsetGroupB_emtc,"dB8")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB8;
		} else if (strcmp(rach_messagePowerOffsetGroupB_emtc,"dB10")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB10;
		} else if (strcmp(rach_messagePowerOffsetGroupB_emtc,"dB12")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB12;
		} else if (strcmp(rach_messagePowerOffsetGroupB_emtc,"dB15")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB15;
		} else if (strcmp(rach_messagePowerOffsetGroupB_emtc,"dB18")==0) {
		  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_messagePowerOffsetGroupB= RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB18;
		} else
		  AssertFatal (0,
			       "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rach_messagePowerOffsetGroupB choice: minusinfinity,dB0,dB5,dB8,dB10,dB12,dB15,dB18!\n",
			       RC.config_file_name, i, rach_messagePowerOffsetGroupB_emtc);
	      } else if (strcmp(rach_preamblesGroupAConfig_emtc, "DISABLE") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preamblesGroupAConfig= FALSE;
	      } else
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rach_preamblesGroupAConfig choice: ENABLE,DISABLE !\n",
			     RC.config_file_name, i, rach_preamblesGroupAConfig_emtc);
	      
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleInitialReceivedTargetPower= (rach_preambleInitialReceivedTargetPower_emtc+120)/2;
	      
	      if ((rach_preambleInitialReceivedTargetPower_emtc<-120) || (rach_preambleInitialReceivedTargetPower_emtc>-90) || ((rach_preambleInitialReceivedTargetPower_emtc&1)!=0))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_preambleInitialReceivedTargetPower choice: -120,-118,...,-90 !\n",
			     RC.config_file_name, i, rach_preambleInitialReceivedTargetPower_emtc);
	      
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_powerRampingStep= rach_powerRampingStep_emtc/2;
	      
	      if ((rach_powerRampingStep_emtc<0) || (rach_powerRampingStep_emtc>6) || ((rach_powerRampingStep_emtc&1)!=0))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_powerRampingStep choice: 0,2,4,6 !\n",
			     RC.config_file_name, i, rach_powerRampingStep_emtc);
	      
	      switch (rach_preambleTransMax_emtc) {
#if (RRC_VERSION < MAKE_VERSION(14, 0, 0))
		
	      case 3:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n3;
		break;
		
	      case 4:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n4;
		break;
		
	      case 5:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n5;
		break;
		
	      case 6:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n6;
		break;
		
	      case 7:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n7;
		break;
		
	      case 8:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n8;
		break;
		
	      case 10:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n10;
		break;
		
	      case 20:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n20;
		break;
		
	      case 50:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n50;
		break;
		
	      case 100:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n100;
		break;
		
	      case 200:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n200;
		break;
#else
		
	      case 3:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n3;
		break;
		
	      case 4:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n4;
		break;
		
	      case 5:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n5;
		break;
		
	      case 6:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n6;
		break;
		
	      case 7:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n7;
		break;
		
	      case 8:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n8;
		break;
		
	      case 10:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n10;
		break;
		
	      case 20:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n20;
		break;
		
	      case 50:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n50;
		break;
		
	      case 100:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n100;
		break;
		
	      case 200:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_preambleTransMax=  PreambleTransMax_n200;
		break;
#endif
		
	      default:
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_preambleTransMax choice: 3,4,5,6,7,8,10,20,50,100,200!\n",
			     RC.config_file_name, i, rach_preambleTransMax_emtc);
		break;
	      }
	      
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_raResponseWindowSize=  (rach_raResponseWindowSize_emtc==10)?7:rach_raResponseWindowSize_emtc-2;
	      
	      if ((rach_raResponseWindowSize_emtc<0)||(rach_raResponseWindowSize_emtc==9)||(rach_raResponseWindowSize_emtc>10))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_raResponseWindowSize choice: 2,3,4,5,6,7,8,10!\n",
			     RC.config_file_name, i, rach_raResponseWindowSize_emtc);
	      
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_macContentionResolutionTimer= (rach_macContentionResolutionTimer_emtc/8)-1;
	      
	      if ((rach_macContentionResolutionTimer_emtc<8) || (rach_macContentionResolutionTimer_emtc>64) || ((rach_macContentionResolutionTimer_emtc&7)!=0))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_macContentionResolutionTimer choice: 8,16,...,56,64!\n",
			     RC.config_file_name, i, rach_macContentionResolutionTimer_emtc);
	      
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].rach_maxHARQ_Msg3Tx= rach_maxHARQ_Msg3Tx_emtc;
	      
	      if ((rach_maxHARQ_Msg3Tx_emtc<0) || (rach_maxHARQ_Msg3Tx_emtc>8))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_maxHARQ_Msg3Tx choice: 1..8!\n",
			     RC.config_file_name, i, rach_maxHARQ_Msg3Tx_emtc);
	      
	      switch (pcch_defaultPagingCycle_emtc) {
	      case 32:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_defaultPagingCycle= PCCH_Config__defaultPagingCycle_rf32;
		break;
		
	      case 64:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_defaultPagingCycle= PCCH_Config__defaultPagingCycle_rf64;
		break;
		
	      case 128:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_defaultPagingCycle= PCCH_Config__defaultPagingCycle_rf128;
		break;
		
	      case 256:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_defaultPagingCycle= PCCH_Config__defaultPagingCycle_rf256;
		break;
		
	      default:
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pcch_defaultPagingCycle choice: 32,64,128,256!\n",
			     RC.config_file_name, i, pcch_defaultPagingCycle_emtc);
		break;
	      }
	      
	      if (strcmp(pcch_nB_emtc, "fourT") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_fourT;
	      } else if (strcmp(pcch_nB_emtc, "twoT") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_twoT;
	      } else if (strcmp(pcch_nB_emtc, "oneT") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_oneT;
	      } else if (strcmp(pcch_nB_emtc, "halfT") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_halfT;
	      } else if (strcmp(pcch_nB_emtc, "quarterT") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_quarterT;
	      } else if (strcmp(pcch_nB_emtc, "oneEighthT") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_oneEighthT;
	      } else if (strcmp(pcch_nB_emtc, "oneSixteenthT") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_oneSixteenthT;
	      } else if (strcmp(pcch_nB_emtc, "oneThirtySecondT") == 0) {
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].pcch_nB= PCCH_Config__nB_oneThirtySecondT;
	      } else
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pcch_nB choice: fourT,twoT,oneT,halfT,quarterT,oneighthT,oneSixteenthT,oneThirtySecondT !\n",
			     RC.config_file_name, i, pcch_nB_emtc);
	      
	      switch (bcch_modificationPeriodCoeff_emtc) {
	      case 2:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].bcch_modificationPeriodCoeff= BCCH_Config__modificationPeriodCoeff_n2;
		break;
		
	      case 4:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].bcch_modificationPeriodCoeff= BCCH_Config__modificationPeriodCoeff_n4;
		break;
		
	      case 8:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].bcch_modificationPeriodCoeff= BCCH_Config__modificationPeriodCoeff_n8;
		break;
		
	      case 16:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].bcch_modificationPeriodCoeff= BCCH_Config__modificationPeriodCoeff_n16;
		break;
		
	      default:
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for bcch_modificationPeriodCoeff choice: 2,4,8,16",
			     RC.config_file_name, i, bcch_modificationPeriodCoeff);
		break;
	      }
	      
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t300= ue_TimersAndConstants_t300;
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t301= ue_TimersAndConstants_t301;
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t310= ue_TimersAndConstants_t310;
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_t311= ue_TimersAndConstants_t311;
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n310= ue_TimersAndConstants_n310;
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TimersAndConstants_n311= ue_TimersAndConstants_n311;
	      
	      switch (ue_TransmissionMode_emtc) {
	      case 1:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm1;
		break;
		
	      case 2:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm2;
		break;
		
	      case 3:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm3;
		break;
		
	      case 4:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm4;
		break;
		
	      case 5:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm5;
		break;
		
	      case 6:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm6;
		break;
		
	      case 7:
		RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig_BR[j].ue_TransmissionMode= AntennaInfoDedicated__transmissionMode_tm7;
		break;
		
	      default:
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TransmissionMode choice: 1,2,3,4,5,6,7",
			     RC.config_file_name, i, ue_TransmissionMode);
		break;
	      }
	      
	      
	      
	      
	      RRC_CONFIGURATION_REQ(msg_p).pdsch_maxNumRepetitionCEmodeA_r13[j] = CALLOC(1, sizeof(long));
	      if (!strcmp(pdsch_maxNumRepetitionCEmodeA_r13, "r16")) {
		*RRC_CONFIGURATION_REQ(msg_p).pdsch_maxNumRepetitionCEmodeA_r13[j] = 0;
	      } else if (!strcmp(pdsch_maxNumRepetitionCEmodeA_r13, "r32")) {
		*RRC_CONFIGURATION_REQ(msg_p).pdsch_maxNumRepetitionCEmodeA_r13[j] = 1;
	      } else {
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, pdsch_maxNumRepetitionCEmodeA_r13 unknown value!\n",
			     RC.config_file_name);
	      }
	      
	      
	      RRC_CONFIGURATION_REQ(msg_p).pusch_maxNumRepetitionCEmodeA_r13[j] = CALLOC(1, sizeof(long));
	      if (!strcmp(pusch_maxNumRepetitionCEmodeA_r13, "r8")) {
		*RRC_CONFIGURATION_REQ(msg_p).pusch_maxNumRepetitionCEmodeA_r13[j] =  0;
	      } else if (!strcmp(pusch_maxNumRepetitionCEmodeA_r13, "r16")) {
		*RRC_CONFIGURATION_REQ(msg_p).pusch_maxNumRepetitionCEmodeA_r13[j] =  1;
	      } else if (!strcmp(pusch_maxNumRepetitionCEmodeA_r13, "r32")) {
		*RRC_CONFIGURATION_REQ(msg_p).pusch_maxNumRepetitionCEmodeA_r13[j] =  2;
	      } else {
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, pusch_maxNumRepetitionCEmodeA_r13 unknown value!\n",
			     RC.config_file_name);
	      }
	      
	      char rachCELevelInfoListPath[MAX_OPTNAME_SIZE * 2];
	      config_getlist(&rachcelevellist, NULL, 0, brparamspath);
	      RRC_CONFIGURATION_REQ (msg_p).rach_CE_LevelInfoList_r13_size[j] = rachcelevellist.numelt;
	      int rachCEInfoIndex;
	      for (rachCEInfoIndex = 0; rachCEInfoIndex < rachcelevellist.numelt; rachCEInfoIndex++) {
		sprintf(rachCELevelInfoListPath, "%s.%s.[%i]", brparamspath, ENB_CONFIG_STRING_RACH_CE_LEVELINFOLIST_R13, rachCEInfoIndex);
		config_get(rachcelevelParams, sizeof(rachcelevelParams) / sizeof(paramdef_t), rachCELevelInfoListPath);
		
		RRC_CONFIGURATION_REQ (msg_p).firstPreamble_r13[j][rachCEInfoIndex] = firstPreamble_r13;
		RRC_CONFIGURATION_REQ (msg_p).lastPreamble_r13[j][rachCEInfoIndex]  = lastPreamble_r13;
		
		RRC_CONFIGURATION_REQ (msg_p).ra_ResponseWindowSize_r13[j][rachCEInfoIndex] = ra_ResponseWindowSize_r13;
		AssertFatal(ra_ResponseWindowSize_r13 == 20 ||
			    ra_ResponseWindowSize_r13 == 50 ||
			    ra_ResponseWindowSize_r13 == 80 ||
			    ra_ResponseWindowSize_r13 == 120 ||
			    ra_ResponseWindowSize_r13 == 180 ||
			    ra_ResponseWindowSize_r13 == 240 ||
			    ra_ResponseWindowSize_r13 == 320 ||
			    ra_ResponseWindowSize_r13 == 400,
			    "Illegal ra_ResponseWindowSize_r13 %d\n",ra_ResponseWindowSize_r13);
		
		RRC_CONFIGURATION_REQ (msg_p).mac_ContentionResolutionTimer_r13[j][rachCEInfoIndex] = mac_ContentionResolutionTimer_r13;
		AssertFatal(mac_ContentionResolutionTimer_r13 == 80 ||
			    mac_ContentionResolutionTimer_r13 == 100 ||
			    mac_ContentionResolutionTimer_r13 == 120 ||
			    mac_ContentionResolutionTimer_r13 == 80 ||
			    mac_ContentionResolutionTimer_r13 == 160 ||
			    mac_ContentionResolutionTimer_r13 == 200 ||
			    mac_ContentionResolutionTimer_r13 == 240 ||
			    mac_ContentionResolutionTimer_r13 == 480 ||
			    mac_ContentionResolutionTimer_r13 == 960,
			    "Illegal mac_ContentionResolutionTimer_r13 %d\n",
			    mac_ContentionResolutionTimer_r13);
		
		RRC_CONFIGURATION_REQ (msg_p).rar_HoppingConfig_r13[j][rachCEInfoIndex] = rar_HoppingConfig_r13;
		AssertFatal(rar_HoppingConfig_r13 == 0 || rar_HoppingConfig_r13 == 1,
			    "illegal rar_HoppingConfig_r13 %d\n",rar_HoppingConfig_r13);
	      } // end for loop (rach ce level info)
	      
	      char rsrpRangeListPath[MAX_OPTNAME_SIZE * 2];
	      config_getlist(&rsrprangelist, NULL, 0, brparamspath);
	      RRC_CONFIGURATION_REQ (msg_p).rsrp_range_list_size[j] = rsrprangelist.numelt;
	      
	      
	      int rsrprangeindex;
	      for (rsrprangeindex = 0; rsrprangeindex < rsrprangelist.numelt; rsrprangeindex++) {
		sprintf(rsrpRangeListPath, "%s.%s.[%i]", brparamspath, ENB_CONFIG_STRING_RSRP_RANGE_LIST, rsrprangeindex);
		config_get(rsrprangeParams, sizeof(rsrprangeParams) / sizeof(paramdef_t), rsrpRangeListPath);
		RRC_CONFIGURATION_REQ (msg_p).rsrp_range[j][rsrprangeindex] = rsrp_range_br;
		
	      }
	      
	      
	      char prachparameterscePath[MAX_OPTNAME_SIZE * 2];
	      config_getlist(&prachParamslist, NULL, 0, brparamspath);
	      RRC_CONFIGURATION_REQ (msg_p).prach_parameters_list_size[j] = prachParamslist.numelt;
	      
	      int prachparamsindex;
	      for (prachparamsindex = 0; prachparamsindex < prachParamslist.numelt; prachparamsindex++) {
		sprintf(prachparameterscePath, "%s.%s.[%i]", brparamspath, ENB_CONFIG_STRING_PRACH_PARAMETERS_CE_R13, prachparamsindex);
		config_get(prachParams, sizeof(prachParams) / sizeof(paramdef_t), prachparameterscePath);
		
		RRC_CONFIGURATION_REQ (msg_p).prach_config_index[j][prachparamsindex]                  = prach_config_index_br;
		RRC_CONFIGURATION_REQ (msg_p).prach_freq_offset[j][prachparamsindex]                   = prach_freq_offset_br;
		
		RRC_CONFIGURATION_REQ (msg_p).prach_StartingSubframe_r13[j][prachparamsindex] = calloc(1, sizeof(long));
		*RRC_CONFIGURATION_REQ (msg_p).prach_StartingSubframe_r13[j][prachparamsindex] = prach_StartingSubframe_r13;
		
		RRC_CONFIGURATION_REQ (msg_p).maxNumPreambleAttemptCE_r13[j][prachparamsindex] = calloc(1, sizeof(long));
		*RRC_CONFIGURATION_REQ (msg_p).maxNumPreambleAttemptCE_r13[j][prachparamsindex] = maxNumPreambleAttemptCE_r13-3;
		AssertFatal(maxNumPreambleAttemptCE_r13 > 2 && maxNumPreambleAttemptCE_r13 <11,
			    "prachparamsindex %d: Illegal maxNumPreambleAttemptCE_r13 %d\n",
			    prachparamsindex,maxNumPreambleAttemptCE_r13);
		
		RRC_CONFIGURATION_REQ (msg_p).numRepetitionPerPreambleAttempt_r13[j][prachparamsindex] = numRepetitionPerPreambleAttempt_r13;
		AssertFatal(numRepetitionPerPreambleAttempt_r13 == 1 ||
			    numRepetitionPerPreambleAttempt_r13 == 2 ||
			    numRepetitionPerPreambleAttempt_r13 == 4 ||
			    numRepetitionPerPreambleAttempt_r13 == 8 ||
			    numRepetitionPerPreambleAttempt_r13 == 16 ||
			    numRepetitionPerPreambleAttempt_r13 == 32 ||
			    numRepetitionPerPreambleAttempt_r13 == 64 ||
			    numRepetitionPerPreambleAttempt_r13 == 128,
			    "illegal numReptitionPerPreambleAttempt %d\n",
			    numRepetitionPerPreambleAttempt_r13);
		
		RRC_CONFIGURATION_REQ (msg_p).mpdcch_NumRepetition_RA_r13[j][prachparamsindex] = mpdcch_NumRepetition_RA_r13;
		AssertFatal (mpdcch_NumRepetition_RA_r13 == 1 ||
			     mpdcch_NumRepetition_RA_r13 == 2 ||
			     mpdcch_NumRepetition_RA_r13 == 4 ||
			     mpdcch_NumRepetition_RA_r13 == 8 ||
			     mpdcch_NumRepetition_RA_r13 == 16 ||
			     mpdcch_NumRepetition_RA_r13 == 32 ||
			     mpdcch_NumRepetition_RA_r13 == 64 ||
			     mpdcch_NumRepetition_RA_r13 == 128 ||
			     mpdcch_NumRepetition_RA_r13 == 256 ||
			     mpdcch_NumRepetition_RA_r13 == 512 ||
			     mpdcch_NumRepetition_RA_r13 == 1024 ||
			     mpdcch_NumRepetition_RA_r13 == 2048,
			     "illegal mpdcch_NumRepeition_RA_r13 %d\n",
			     mpdcch_NumRepetition_RA_r13);
		
		
		RRC_CONFIGURATION_REQ (msg_p).prach_HoppingConfig_r13[j][prachparamsindex] = prach_HoppingConfig_r13;
		
		AssertFatal (prach_HoppingConfig_r13 >=0 && prach_HoppingConfig_r13 < 2,
			     "Illegal prach_HoppingConfig_r13 %d\n",prach_HoppingConfig_r13);
		
		
		int maxavailablenarrowband_count = prachParams[7].numelt;
		
		RRC_CONFIGURATION_REQ (msg_p).max_available_narrow_band_size[j][prachparamsindex] = maxavailablenarrowband_count;
		int narrow_band_index;
		for (narrow_band_index = 0; narrow_band_index < maxavailablenarrowband_count; narrow_band_index++) {
		  RRC_CONFIGURATION_REQ (msg_p).max_available_narrow_band[j][prachparamsindex][narrow_band_index] = prachParams[7].iptr[narrow_band_index];
		}
	      }

	      char n1PUCCHInfoParamsPath[MAX_OPTNAME_SIZE * 2];
	      config_getlist(&n1PUCCHInfoList, NULL, 0, brparamspath);
	      RRC_CONFIGURATION_REQ (msg_p).pucch_info_value_size[j] = n1PUCCHInfoList.numelt;
	      
	      int n1PUCCHinfolistindex;
	      for (n1PUCCHinfolistindex = 0; n1PUCCHinfolistindex < n1PUCCHInfoList.numelt; n1PUCCHinfolistindex++) {
		sprintf(n1PUCCHInfoParamsPath, "%s.%s.[%i]", brparamspath, ENB_CONFIG_STRING_N1PUCCH_AN_INFOLIST_R13, n1PUCCHinfolistindex);
		config_get(n1PUCCH_ANR13Params, sizeof(n1PUCCH_ANR13Params) / sizeof(paramdef_t), n1PUCCHInfoParamsPath);
		RRC_CONFIGURATION_REQ (msg_p).pucch_info_value[j][n1PUCCHinfolistindex] = pucch_info_value;
	      }
	      
	      char PCCHConfigv1310Path[MAX_OPTNAME_SIZE*2 + 16];
	      sprintf(PCCHConfigv1310Path, "%s.%s", brparamspath, ENB_CONFIG_STRING_PCCH_CONFIG_V1310);
	      config_get(pcchv1310Params, sizeof(pcchv1310Params)/sizeof(paramdef_t), PCCHConfigv1310Path);
	      
	      
	      
	      /** PCCH CONFIG V1310 */
	      
	      RRC_CONFIGURATION_REQ(msg_p).pcch_config_v1310[j] = TRUE;
	      RRC_CONFIGURATION_REQ(msg_p).paging_narrowbands_r13[j] = paging_narrowbands_r13;
	      RRC_CONFIGURATION_REQ(msg_p).mpdcch_numrepetition_paging_r13[j] = mpdcch_numrepetition_paging_r13;
	      AssertFatal (mpdcch_numrepetition_paging_r13 == 1 ||
			   mpdcch_numrepetition_paging_r13 == 2 ||
			   mpdcch_numrepetition_paging_r13 == 4 ||
			   mpdcch_numrepetition_paging_r13 == 8 ||
			   mpdcch_numrepetition_paging_r13 == 16 ||
			   mpdcch_numrepetition_paging_r13 == 32 ||
			   mpdcch_numrepetition_paging_r13 == 64 ||
			   mpdcch_numrepetition_paging_r13 == 128 ||
			   mpdcch_numrepetition_paging_r13 == 256,
			   "illegal mpdcch_numrepetition_paging_r13 %d\n",
			   mpdcch_numrepetition_paging_r13);
	      
	      
	      //                        RRC_CONFIGURATION_REQ(msg_p).nb_v1310[j] = CALLOC(1, sizeof(long));
	      //                        if (!strcmp(nb_v1310, "one64thT")) {
	      //                            *RRC_CONFIGURATION_REQ(msg_p).nb_v1310[j] = 0;
	      //                        } else if (!strcmp(nb_v1310, "one128thT")) {
	      //                            *RRC_CONFIGURATION_REQ(msg_p).nb_v1310[j] = 1;
	      //                        } else if (!strcmp(nb_v1310, "one256thT")) {
	      //                            *RRC_CONFIGURATION_REQ(msg_p).nb_v1310[j] = 2;
	      //                        } else {
	      //                            AssertFatal(0,
	      //                                        "Failed to parse eNB configuration file %s, nb_v1310, unknown value !\n",
	      //                                        RC.config_file_name);
	      //                        }
	      
	      
	      
	      RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level0_r13[j] = CALLOC(1, sizeof(long));
	      // ++cnt; // check this ,, the conter is up above
	      if (!strcmp(pucch_NumRepetitionCE_Msg4_Level0_r13, "n1")) {
		*RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level0_r13[j] = 0;
	      } else if (!strcmp(pucch_NumRepetitionCE_Msg4_Level0_r13, "n2")) {
		*RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level0_r13[j] = 1;
	      } else if (!strcmp(pucch_NumRepetitionCE_Msg4_Level0_r13, "n4")) {
		*RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level0_r13[j] = 2;
	      } else if (!strcmp(pucch_NumRepetitionCE_Msg4_Level0_r13, "n8")) {
		*RRC_CONFIGURATION_REQ (msg_p).pucch_NumRepetitionCE_Msg4_Level0_r13[j] = 3;
	      } else {
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, pucch_NumRepetitionCE_Msg4_Level0_r13 unknown value!\n",
			     RC.config_file_name);
	      }
	      
	      
	      
	      /** SIB2 FREQ HOPPING PARAMETERS R13 */
	      RRC_CONFIGURATION_REQ(msg_p).sib2_freq_hoppingParameters_r13_exists[j] = TRUE;
	      
	      char sib2FreqHoppingParametersR13Path[MAX_OPTNAME_SIZE*2 + 16];
	      sprintf(sib2FreqHoppingParametersR13Path, "%s.%s", brparamspath, ENB_CONFIG_STRING_SIB2_FREQ_HOPPINGPARAMETERS_R13);
	      config_get(sib2freqhoppingParams, sizeof(sib2freqhoppingParams)/sizeof(paramdef_t), sib2FreqHoppingParametersR13Path);
	      
	      
	      RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13[j] = CALLOC(1, sizeof(long));
	      if (!strcmp(sib2_interval_ULHoppingConfigCommonModeA_r13, "FDD")) {
		*RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13[j] = 0;
		RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13_val[j] = sib2_interval_ULHoppingConfigCommonModeA_r13_val;
		AssertFatal(sib2_interval_ULHoppingConfigCommonModeA_r13_val==1 ||
			    sib2_interval_ULHoppingConfigCommonModeA_r13_val==2 ||
			    sib2_interval_ULHoppingConfigCommonModeA_r13_val==4 ||
			    sib2_interval_ULHoppingConfigCommonModeA_r13_val==8,
			    "illegal sib2_interval_ULHoppingConfigCommonModeA_r13_val %d\n",
			    sib2_interval_ULHoppingConfigCommonModeA_r13_val);
	      } else if (!strcmp(sib2_interval_ULHoppingConfigCommonModeA_r13, "TDD")) {
		*RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13[j] = 1;
		RRC_CONFIGURATION_REQ(msg_p).sib2_interval_ULHoppingConfigCommonModeA_r13_val[j] = sib2_interval_ULHoppingConfigCommonModeA_r13_val;
		AssertFatal(sib2_interval_ULHoppingConfigCommonModeA_r13_val==1 ||
			    sib2_interval_ULHoppingConfigCommonModeA_r13_val==5 ||
			    sib2_interval_ULHoppingConfigCommonModeA_r13_val==10 ||
			    sib2_interval_ULHoppingConfigCommonModeA_r13_val==20,
			    "illegal sib2_interval_ULHoppingConfigCommonModeA_r13_val %d\n",
			    sib2_interval_ULHoppingConfigCommonModeA_r13_val);
	      } else {
		AssertFatal (1==0,
			     "Failed to parse eNB configuration file %s, sib2_interval_ULHoppingConfigCommonModeA_r13 unknown value !!\n",
			     RC.config_file_name);
	      }
	      RRC_CONFIGURATION_REQ (msg_p).eMTC_configured=1;
	    } // BR parameters > 0
	    else {
	      printf("No eMTC configuration, skipping it\n");
	      RRC_CONFIGURATION_REQ (msg_p).eMTC_configured=0;
	    }

	    // Sidelink Resource pool information
            //SIB18
            if (strcmp(rxPool_sc_CP_Len,"normal")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_sc_CP_Len[j] = SL_CP_Len_r12_normal;
            } else if (strcmp(rxPool_sc_CP_Len,"extended")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_sc_CP_Len[j] = SL_CP_Len_r12_extended;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rxPool_sc_CP_Len choice: normal,extended!\n",
                           RC.config_file_name, i, rxPool_sc_CP_Len);

            if (strcmp(rxPool_sc_Period,"sf40")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_sc_Period[j] = SL_PeriodComm_r12_sf40;
            } else if (strcmp(rxPool_sc_Period,"sf60")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_sc_Period[j] = SL_PeriodComm_r12_sf60;
            } else if (strcmp(rxPool_sc_Period,"sf70")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_sc_Period[j] = SL_PeriodComm_r12_sf70;
            } else if (strcmp(rxPool_sc_Period,"sf80")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_sc_Period[j] = SL_PeriodComm_r12_sf80;
            } else if (strcmp(rxPool_sc_Period,"sf120")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_sc_Period[j] = SL_PeriodComm_r12_sf120;
            } else if (strcmp(rxPool_sc_Period,"sf140")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_sc_Period[j] = SL_PeriodComm_r12_sf140;
            } else if (strcmp(rxPool_sc_Period,"sf160")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_sc_Period[j] = SL_PeriodComm_r12_sf160;
            } else if (strcmp(rxPool_sc_Period,"sf240")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_sc_Period[j] = SL_PeriodComm_r12_sf240;
            } else if (strcmp(rxPool_sc_Period,"sf280")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_sc_Period[j] = SL_PeriodComm_r12_sf280;
            } else if (strcmp(rxPool_sc_Period,"sf320")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_sc_Period[j] = SL_PeriodComm_r12_sf320;
            } else if (strcmp(rxPool_sc_Period,"spare6")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_sc_Period[j] = SL_PeriodComm_r12_spare6;
            } else if (strcmp(rxPool_sc_Period,"spare5")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_sc_Period[j] = SL_PeriodComm_r12_spare5;
            } else if (strcmp(rxPool_sc_Period,"spare4")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_sc_Period[j] = SL_PeriodComm_r12_spare4;
            } else if (strcmp(rxPool_sc_Period,"spare3")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_sc_Period[j] = SL_PeriodComm_r12_spare3;
            } else if (strcmp(rxPool_sc_Period,"spare2")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_sc_Period[j] = SL_PeriodComm_r12_spare2;
            } else if (strcmp(rxPool_sc_Period,"spare")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_sc_Period[j] = SL_PeriodComm_r12_spare;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rxPool_sc_Period choice: sf40,sf60,sf70,sf80,sf120,sf140,sf160,sf240,sf280,sf320,spare6,spare5,spare4,spare3,spare2,spare!\n",
                           RC.config_file_name, i, rxPool_sc_Period);

            if (strcmp(rxPool_data_CP_Len,"normal")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_data_CP_Len[j] = SL_CP_Len_r12_normal;
            } else if (strcmp(rxPool_data_CP_Len,"extended")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_data_CP_Len[j] = SL_CP_Len_r12_extended;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rxPool_data_CP_Len choice: normal,extended!\n",
                           RC.config_file_name, i, rxPool_data_CP_Len);

            RRC_CONFIGURATION_REQ (msg_p).rxPool_ResourceConfig_prb_Num[j] = rxPool_ResourceConfig_prb_Num;
            RRC_CONFIGURATION_REQ (msg_p).rxPool_ResourceConfig_prb_Start[j] = rxPool_ResourceConfig_prb_Start;
            RRC_CONFIGURATION_REQ (msg_p).rxPool_ResourceConfig_prb_End[j] = rxPool_ResourceConfig_prb_End;

            if (strcmp(rxPool_ResourceConfig_offsetIndicator_present,"prNothing")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_ResourceConfig_offsetIndicator_present[j] = SL_OffsetIndicator_r12_PR_NOTHING;
            } else if (strcmp(rxPool_ResourceConfig_offsetIndicator_present,"prSmall")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_ResourceConfig_offsetIndicator_present[j] = SL_OffsetIndicator_r12_PR_small_r12;
            } else if (strcmp(rxPool_ResourceConfig_offsetIndicator_present,"prLarge")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_ResourceConfig_offsetIndicator_present[j] = SL_OffsetIndicator_r12_PR_large_r12;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rxPool_ResourceConfig_offsetIndicator_present choice: prNothing,prSmal,prLarge!\n",
                           RC.config_file_name, i, rxPool_ResourceConfig_offsetIndicator_present);

            RRC_CONFIGURATION_REQ (msg_p).rxPool_ResourceConfig_offsetIndicator_choice[j] = rxPool_ResourceConfig_offsetIndicator_choice;

            if (strcmp(rxPool_ResourceConfig_subframeBitmap_present,"prNothing")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_NOTHING;
            } else if (strcmp(rxPool_ResourceConfig_subframeBitmap_present,"prBs4")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_bs4_r12;
            } else if (strcmp(rxPool_ResourceConfig_subframeBitmap_present,"prBs8")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_bs8_r12;
            } else if (strcmp(rxPool_ResourceConfig_subframeBitmap_present,"prBs12")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_bs12_r12;
            } else if (strcmp(rxPool_ResourceConfig_subframeBitmap_present,"prBs16")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_bs16_r12;
            } else if (strcmp(rxPool_ResourceConfig_subframeBitmap_present,"prBs30")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_bs30_r12;
            } else if (strcmp(rxPool_ResourceConfig_subframeBitmap_present,"prBs40")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_bs40_r12;
            } else if (strcmp(rxPool_ResourceConfig_subframeBitmap_present,"prBs42")==0) {
              RRC_CONFIGURATION_REQ (msg_p).rxPool_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_bs42_r12;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rxPool_ResourceConfig_subframeBitmap_present choice: prNothing,prBs4,prBs8,prBs12,prBs16,prBs30,prBs40,prBs42!\n",
                           RC.config_file_name, i, rxPool_ResourceConfig_subframeBitmap_present);

            RRC_CONFIGURATION_REQ (msg_p).rxPool_ResourceConfig_subframeBitmap_choice_bs_buf[j] = rxPool_ResourceConfig_subframeBitmap_choice_bs_buf;
            RRC_CONFIGURATION_REQ (msg_p).rxPool_ResourceConfig_subframeBitmap_choice_bs_size[j] = rxPool_ResourceConfig_subframeBitmap_choice_bs_size;
            RRC_CONFIGURATION_REQ (msg_p).rxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused[j] = rxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused;

            //SIB19 - for discRxPool
            if (strcmp(discRxPool_cp_Len,"normal")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPool_cp_Len[j] = SL_CP_Len_r12_normal;
            } else if (strcmp(discRxPool_cp_Len,"extended")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPool_cp_Len[j] = SL_CP_Len_r12_extended;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for discRxPool_cp_Len choice: normal,extended!\n",
                           RC.config_file_name, i, discRxPool_cp_Len);

            if (strcmp(discRxPool_discPeriod,"rf32")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPool_discPeriod[j] = SL_DiscResourcePool_r12__discPeriod_r12_rf32;
            } else if (strcmp(discRxPool_discPeriod,"rf64")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPool_discPeriod[j] = SL_DiscResourcePool_r12__discPeriod_r12_rf64;
            } else if (strcmp(discRxPool_discPeriod,"rf128")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPool_discPeriod[j] = SL_DiscResourcePool_r12__discPeriod_r12_rf128;
            } else if (strcmp(discRxPool_discPeriod,"rf256")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPool_discPeriod[j] = SL_DiscResourcePool_r12__discPeriod_r12_rf256;
            } else if (strcmp(discRxPool_discPeriod,"rf512")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPool_discPeriod[j] = SL_DiscResourcePool_r12__discPeriod_r12_rf512;
            } else if (strcmp(discRxPool_discPeriod,"rf1024")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPool_discPeriod[j] = SL_DiscResourcePool_r12__discPeriod_r12_rf1024;
            } else if (strcmp(discRxPool_discPeriod,"rf16")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPool_discPeriod[j] = SL_DiscResourcePool_r12__discPeriod_r12_rf16_v1310;
            } else if (strcmp(discRxPool_discPeriod,"spare")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPool_discPeriod[j] = SL_DiscResourcePool_r12__discPeriod_r12_spare;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for discRxPool_discPeriod choice: rf32,rf64,rf128,rf512,rf1024,rf16,spare!\n",
                           RC.config_file_name, i, discRxPool_discPeriod);

            RRC_CONFIGURATION_REQ (msg_p).discRxPool_numRetx[j] = discRxPool_numRetx;
            RRC_CONFIGURATION_REQ (msg_p).discRxPool_numRepetition[j] = discRxPool_numRepetition;
            RRC_CONFIGURATION_REQ (msg_p).discRxPool_ResourceConfig_prb_Num[j] = discRxPool_ResourceConfig_prb_Num;
            RRC_CONFIGURATION_REQ (msg_p).discRxPool_ResourceConfig_prb_Start[j] = discRxPool_ResourceConfig_prb_Start;
            RRC_CONFIGURATION_REQ (msg_p).discRxPool_ResourceConfig_prb_End[j] = discRxPool_ResourceConfig_prb_End;

            if (strcmp(discRxPool_ResourceConfig_offsetIndicator_present,"prNothing")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPool_ResourceConfig_offsetIndicator_present[j] = SL_OffsetIndicator_r12_PR_NOTHING;
            } else if (strcmp(discRxPool_ResourceConfig_offsetIndicator_present,"prSmall")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPool_ResourceConfig_offsetIndicator_present[j] = SL_OffsetIndicator_r12_PR_small_r12;
            } else if (strcmp(discRxPool_ResourceConfig_offsetIndicator_present,"prLarge")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPool_ResourceConfig_offsetIndicator_present[j] = SL_OffsetIndicator_r12_PR_large_r12;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for discRxPool_ResourceConfig_offsetIndicator_present choice: prNothing,prSmal,prLarge!\n",
                           RC.config_file_name, i, discRxPool_ResourceConfig_offsetIndicator_present);

            RRC_CONFIGURATION_REQ (msg_p).discRxPool_ResourceConfig_offsetIndicator_choice[j] = discRxPool_ResourceConfig_offsetIndicator_choice;

            if (strcmp(discRxPool_ResourceConfig_subframeBitmap_present,"prNothing")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPool_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_NOTHING;
            } else if (strcmp(discRxPool_ResourceConfig_subframeBitmap_present,"prBs4")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPool_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_bs4_r12;
            } else if (strcmp(discRxPool_ResourceConfig_subframeBitmap_present,"prBs8")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPool_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_bs8_r12;
            } else if (strcmp(discRxPool_ResourceConfig_subframeBitmap_present,"prBs12")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPool_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_bs12_r12;
            } else if (strcmp(discRxPool_ResourceConfig_subframeBitmap_present,"prBs16")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPool_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_bs16_r12;
            } else if (strcmp(discRxPool_ResourceConfig_subframeBitmap_present,"prBs30")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPool_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_bs30_r12;
            } else if (strcmp(discRxPool_ResourceConfig_subframeBitmap_present,"prBs40")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPool_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_bs40_r12;
            } else if (strcmp(discRxPool_ResourceConfig_subframeBitmap_present,"prBs42")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPool_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_bs42_r12;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for discRxPool_ResourceConfig_subframeBitmap_present choice: prNothing,prBs4,prBs8,prBs12,prBs16,prBs30,prBs40,prBs42!\n",
                           RC.config_file_name, i, discRxPool_ResourceConfig_subframeBitmap_present);

            RRC_CONFIGURATION_REQ (msg_p).discRxPool_ResourceConfig_subframeBitmap_choice_bs_buf[j] = discRxPool_ResourceConfig_subframeBitmap_choice_bs_buf;
            RRC_CONFIGURATION_REQ (msg_p).discRxPool_ResourceConfig_subframeBitmap_choice_bs_size[j] = discRxPool_ResourceConfig_subframeBitmap_choice_bs_size;
            RRC_CONFIGURATION_REQ (msg_p).discRxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused[j] = discRxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused;

            //SIB19 - For discRxPoolPS
            if (strcmp(discRxPoolPS_cp_Len,"normal")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_cp_Len[j] = SL_CP_Len_r12_normal;
            } else if (strcmp(discRxPoolPS_cp_Len,"extended")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_cp_Len[j] = SL_CP_Len_r12_extended;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for discRxPoolPS_cp_Len choice: normal,extended!\n",
                           RC.config_file_name, i, discRxPoolPS_cp_Len);

            if (strcmp(discRxPoolPS_discPeriod,"rf32")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_discPeriod[j] = SL_DiscResourcePool_r12__discPeriod_r12_rf32;
            } else if (strcmp(discRxPoolPS_discPeriod,"rf64")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_discPeriod[j] = SL_DiscResourcePool_r12__discPeriod_r12_rf64;
            } else if (strcmp(discRxPoolPS_discPeriod,"rf128")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_discPeriod[j] = SL_DiscResourcePool_r12__discPeriod_r12_rf128;
            } else if (strcmp(discRxPoolPS_discPeriod,"rf256")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_discPeriod[j] = SL_DiscResourcePool_r12__discPeriod_r12_rf256;
            } else if (strcmp(discRxPoolPS_discPeriod,"rf512")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_discPeriod[j] = SL_DiscResourcePool_r12__discPeriod_r12_rf512;
            } else if (strcmp(discRxPoolPS_discPeriod,"rf1024")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_discPeriod[j] = SL_DiscResourcePool_r12__discPeriod_r12_rf1024;
            } else if (strcmp(discRxPoolPS_discPeriod,"rf16")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_discPeriod[j] = SL_DiscResourcePool_r12__discPeriod_r12_rf16_v1310;
            } else if (strcmp(discRxPoolPS_discPeriod,"spare")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_discPeriod[j] = SL_DiscResourcePool_r12__discPeriod_r12_spare;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for discRxPoolPS_discPeriod choice: rf32,rf64,rf128,rf512,rf1024,rf16,spare!\n",
                           RC.config_file_name, i, discRxPoolPS_discPeriod);

            RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_numRetx[j] = discRxPoolPS_numRetx;
            RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_numRepetition[j] = discRxPoolPS_numRepetition;
            RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_ResourceConfig_prb_Num[j] = discRxPoolPS_ResourceConfig_prb_Num;
            RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_ResourceConfig_prb_Start[j] = discRxPoolPS_ResourceConfig_prb_Start;
            RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_ResourceConfig_prb_End[j] = discRxPoolPS_ResourceConfig_prb_End;

            if (strcmp(discRxPoolPS_ResourceConfig_offsetIndicator_present,"prNothing")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_ResourceConfig_offsetIndicator_present[j] = SL_OffsetIndicator_r12_PR_NOTHING;
            } else if (strcmp(discRxPoolPS_ResourceConfig_offsetIndicator_present,"prSmall")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_ResourceConfig_offsetIndicator_present[j] = SL_OffsetIndicator_r12_PR_small_r12;
            } else if (strcmp(discRxPoolPS_ResourceConfig_offsetIndicator_present,"prLarge")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_ResourceConfig_offsetIndicator_present[j] = SL_OffsetIndicator_r12_PR_large_r12;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for discRxPoolPS_ResourceConfig_offsetIndicator_present choice: prNothing,prSmal,prLarge!\n",
                           RC.config_file_name, i, discRxPoolPS_ResourceConfig_offsetIndicator_present);

            RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_ResourceConfig_offsetIndicator_choice[j] = discRxPoolPS_ResourceConfig_offsetIndicator_choice;

            if (strcmp(discRxPoolPS_ResourceConfig_subframeBitmap_present,"prNothing")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_NOTHING;
            } else if (strcmp(discRxPoolPS_ResourceConfig_subframeBitmap_present,"prBs4")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_bs4_r12;
            } else if (strcmp(discRxPoolPS_ResourceConfig_subframeBitmap_present,"prBs8")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_bs8_r12;
            } else if (strcmp(discRxPoolPS_ResourceConfig_subframeBitmap_present,"prBs12")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_bs12_r12;
            } else if (strcmp(discRxPoolPS_ResourceConfig_subframeBitmap_present,"prBs16")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_bs16_r12;
            } else if (strcmp(discRxPoolPS_ResourceConfig_subframeBitmap_present,"prBs30")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_bs30_r12;
            } else if (strcmp(discRxPoolPS_ResourceConfig_subframeBitmap_present,"prBs40")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_bs40_r12;
            } else if (strcmp(discRxPoolPS_ResourceConfig_subframeBitmap_present,"prBs42")==0) {
              RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_ResourceConfig_subframeBitmap_present[j] = SubframeBitmapSL_r12_PR_bs42_r12;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for discRxPoolPS_ResourceConfig_subframeBitmap_present choice: prNothing,prBs4,prBs8,prBs12,prBs16,prBs30,prBs40,prBs42!\n",
                           RC.config_file_name, i, discRxPoolPS_ResourceConfig_subframeBitmap_present);

            RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_buf[j] = discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_buf;
            RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_size[j] = discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_size;
            RRC_CONFIGURATION_REQ (msg_p).discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_bits_unused[j] = discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_bits_unused;
          }
        }

        char srb1path[MAX_OPTNAME_SIZE*2 + 8];
        sprintf(srb1path,"%s.%s",enbpath,ENB_CONFIG_STRING_SRB1);
        int npar = config_get( SRB1Params,sizeof(SRB1Params)/sizeof(paramdef_t), srb1path);

        if (npar == sizeof(SRB1Params)/sizeof(paramdef_t)) {
          switch (srb1_max_retx_threshold) {
	  case 1:
	    rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t1;
	    break;

	  case 2:
	    rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t2;
	    break;

	  case 3:
	    rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t3;
	    break;

	  case 4:
	    rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t4;
	    break;

	  case 6:
	    rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t6;
	    break;

	  case 8:
	    rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t8;
	    break;

	  case 16:
	    rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t16;
	    break;

	  case 32:
	    rrc->srb1_max_retx_threshold = UL_AM_RLC__maxRetxThreshold_t32;
	    break;

	  default:
	    AssertFatal (0,
			 "Bad config value when parsing eNB configuration file %s, enb %d  srb1_max_retx_threshold %u!\n",
			 RC.config_file_name, i, srb1_max_retx_threshold);
          }

          switch (srb1_poll_pdu) {
	  case 4:
	    rrc->srb1_poll_pdu = PollPDU_p4;
	    break;

	  case 8:
	    rrc->srb1_poll_pdu = PollPDU_p8;
	    break;

	  case 16:
	    rrc->srb1_poll_pdu = PollPDU_p16;
	    break;

	  case 32:
	    rrc->srb1_poll_pdu = PollPDU_p32;
	    break;

	  case 64:
	    rrc->srb1_poll_pdu = PollPDU_p64;
	    break;

	  case 128:
	    rrc->srb1_poll_pdu = PollPDU_p128;
	    break;

	  case 256:
	    rrc->srb1_poll_pdu = PollPDU_p256;
	    break;

	  default:
	    if (srb1_poll_pdu >= 10000)
	      rrc->srb1_poll_pdu = PollPDU_pInfinity;
	    else
	      AssertFatal (0,
			   "Bad config value when parsing eNB configuration file %s, enb %d  srb1_poll_pdu %u!\n",
			   RC.config_file_name, i, srb1_poll_pdu);
          }

          rrc->srb1_poll_byte             = srb1_poll_byte;

          switch (srb1_poll_byte) {
	  case 25:
	    rrc->srb1_poll_byte = PollByte_kB25;
	    break;

	  case 50:
	    rrc->srb1_poll_byte = PollByte_kB50;
	    break;

	  case 75:
	    rrc->srb1_poll_byte = PollByte_kB75;
	    break;

	  case 100:
	    rrc->srb1_poll_byte = PollByte_kB100;
	    break;

	  case 125:
	    rrc->srb1_poll_byte = PollByte_kB125;
	    break;

	  case 250:
	    rrc->srb1_poll_byte = PollByte_kB250;
	    break;

	  case 375:
	    rrc->srb1_poll_byte = PollByte_kB375;
	    break;

	  case 500:
	    rrc->srb1_poll_byte = PollByte_kB500;
	    break;

	  case 750:
	    rrc->srb1_poll_byte = PollByte_kB750;
	    break;

	  case 1000:
	    rrc->srb1_poll_byte = PollByte_kB1000;
	    break;

	  case 1250:
	    rrc->srb1_poll_byte = PollByte_kB1250;
	    break;

	  case 1500:
	    rrc->srb1_poll_byte = PollByte_kB1500;
	    break;

	  case 2000:
	    rrc->srb1_poll_byte = PollByte_kB2000;
	    break;

	  case 3000:
	    rrc->srb1_poll_byte = PollByte_kB3000;
	    break;

	  default:
	    if (srb1_poll_byte >= 10000)
	      rrc->srb1_poll_byte = PollByte_kBinfinity;
	    else
	      AssertFatal (0,
			   "Bad config value when parsing eNB configuration file %s, enb %d  srb1_poll_byte %u!\n",
			   RC.config_file_name, i, srb1_poll_byte);
          }

          if (srb1_timer_poll_retransmit <= 250) {
            rrc->srb1_timer_poll_retransmit = (srb1_timer_poll_retransmit - 5)/5;
          } else if (srb1_timer_poll_retransmit <= 500) {
            rrc->srb1_timer_poll_retransmit = (srb1_timer_poll_retransmit - 300)/50 + 50;
          } else {
            AssertFatal (0,
                         "Bad config value when parsing eNB configuration file %s, enb %d  srb1_timer_poll_retransmit %u!\n",
                         RC.config_file_name, i, srb1_timer_poll_retransmit);
          }

          if (srb1_timer_status_prohibit <= 250) {
            rrc->srb1_timer_status_prohibit = srb1_timer_status_prohibit/5;
          } else if ((srb1_timer_poll_retransmit >= 300) && (srb1_timer_poll_retransmit <= 500)) {
            rrc->srb1_timer_status_prohibit = (srb1_timer_status_prohibit - 300)/50 + 51;
          } else {
            AssertFatal (0,
                         "Bad config value when parsing eNB configuration file %s, enb %d  srb1_timer_status_prohibit %u!\n",
                         RC.config_file_name, i, srb1_timer_status_prohibit);
          }

          switch (srb1_timer_reordering) {
	  case 0:
	    rrc->srb1_timer_reordering = T_Reordering_ms0;
	    break;

	  case 5:
	    rrc->srb1_timer_reordering = T_Reordering_ms5;
	    break;

	  case 10:
	    rrc->srb1_timer_reordering = T_Reordering_ms10;
	    break;

	  case 15:
	    rrc->srb1_timer_reordering = T_Reordering_ms15;
	    break;

	  case 20:
	    rrc->srb1_timer_reordering = T_Reordering_ms20;
	    break;

	  case 25:
	    rrc->srb1_timer_reordering = T_Reordering_ms25;
	    break;

	  case 30:
	    rrc->srb1_timer_reordering = T_Reordering_ms30;
	    break;

	  case 35:
	    rrc->srb1_timer_reordering = T_Reordering_ms35;
	    break;

	  case 40:
	    rrc->srb1_timer_reordering = T_Reordering_ms40;
	    break;

	  case 45:
	    rrc->srb1_timer_reordering = T_Reordering_ms45;
	    break;

	  case 50:
	    rrc->srb1_timer_reordering = T_Reordering_ms50;
	    break;

	  case 55:
	    rrc->srb1_timer_reordering = T_Reordering_ms55;
	    break;

	  case 60:
	    rrc->srb1_timer_reordering = T_Reordering_ms60;
	    break;

	  case 65:
	    rrc->srb1_timer_reordering = T_Reordering_ms65;
	    break;

	  case 70:
	    rrc->srb1_timer_reordering = T_Reordering_ms70;
	    break;

	  case 75:
	    rrc->srb1_timer_reordering = T_Reordering_ms75;
	    break;

	  case 80:
	    rrc->srb1_timer_reordering = T_Reordering_ms80;
	    break;

	  case 85:
	    rrc->srb1_timer_reordering = T_Reordering_ms85;
	    break;

	  case 90:
	    rrc->srb1_timer_reordering = T_Reordering_ms90;
	    break;

	  case 95:
	    rrc->srb1_timer_reordering = T_Reordering_ms95;
	    break;

	  case 100:
	    rrc->srb1_timer_reordering = T_Reordering_ms100;
	    break;

	  case 110:
	    rrc->srb1_timer_reordering = T_Reordering_ms110;
	    break;

	  case 120:
	    rrc->srb1_timer_reordering = T_Reordering_ms120;
	    break;

	  case 130:
	    rrc->srb1_timer_reordering = T_Reordering_ms130;
	    break;

	  case 140:
	    rrc->srb1_timer_reordering = T_Reordering_ms140;
	    break;

	  case 150:
	    rrc->srb1_timer_reordering = T_Reordering_ms150;
	    break;

	  case 160:
	    rrc->srb1_timer_reordering = T_Reordering_ms160;
	    break;

	  case 170:
	    rrc->srb1_timer_reordering = T_Reordering_ms170;
	    break;

	  case 180:
	    rrc->srb1_timer_reordering = T_Reordering_ms180;
	    break;

	  case 190:
	    rrc->srb1_timer_reordering = T_Reordering_ms190;
	    break;

	  case 200:
	    rrc->srb1_timer_reordering = T_Reordering_ms200;
	    break;

	  default:
	    AssertFatal (0,
			 "Bad config value when parsing eNB configuration file %s, enb %d  srb1_timer_reordering %u!\n",
			 RC.config_file_name, i, srb1_timer_reordering);
          }
        } else {
          rrc->srb1_timer_poll_retransmit = T_PollRetransmit_ms80;
          rrc->srb1_timer_reordering      = T_Reordering_ms35;
          rrc->srb1_timer_status_prohibit = T_StatusProhibit_ms0;
          rrc->srb1_poll_pdu              = PollPDU_p4;
          rrc->srb1_poll_byte             = PollByte_kBinfinity;
          rrc->srb1_max_retx_threshold    = UL_AM_RLC__maxRetxThreshold_t8;
        }

        break;
      }
    }
  }

  return 0;

}

int RCconfig_gtpu(void ) {
  int               num_enbs                      = 0;
  char             *enb_interface_name_for_S1U    = NULL;
  char             *enb_ipv4_address_for_S1U      = NULL;
  uint32_t          enb_port_for_S1U              = 0;
  char             *address                       = NULL;
  char             *cidr                          = NULL;
  char gtpupath[MAX_OPTNAME_SIZE*2 + 8];
  paramdef_t ENBSParams[] = ENBSPARAMS_DESC;
  paramdef_t GTPUParams[]  = GTPUPARAMS_DESC;
  LOG_I(GTPU,"Configuring GTPu\n");
  /* get number of active eNodeBs */
  config_get( ENBSParams,sizeof(ENBSParams)/sizeof(paramdef_t),NULL);
  num_enbs = ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt;
  AssertFatal (num_enbs >0,
               "Failed to parse config file no active eNodeBs in %s \n", ENB_CONFIG_STRING_ACTIVE_ENBS);
  sprintf(gtpupath,"%s.[%i].%s",ENB_CONFIG_STRING_ENB_LIST,0,ENB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);
  config_get( GTPUParams,sizeof(GTPUParams)/sizeof(paramdef_t),gtpupath);
  cidr = enb_ipv4_address_for_S1U;
  address = strtok(cidr, "/");

  if (address) {
    MessageDef *message;
    AssertFatal((message = itti_alloc_new_message(TASK_ENB_APP, GTPV1U_ENB_S1_REQ))!=NULL,"");
    IPV4_STR_ADDR_TO_INT_NWBO ( address, GTPV1U_ENB_S1_REQ(message).enb_ip_address_for_S1u_S12_S4_up, "BAD IP ADDRESS FORMAT FOR eNB S1_U !\n" );
    LOG_I(GTPU,"Configuring GTPu address : %s -> %x\n",address,GTPV1U_ENB_S1_REQ(message).enb_ip_address_for_S1u_S12_S4_up);
    GTPV1U_ENB_S1_REQ(message).enb_port_for_S1u_S12_S4_up = enb_port_for_S1U;
    itti_send_msg_to_task (TASK_GTPV1_U, 0, message); // data model is wrong: gtpu doesn't have enb_id (or module_id)
  } else
    LOG_E(GTPU,"invalid address for S1U\n");

  return 0;
}


int RCconfig_S1(MessageDef *msg_p, uint32_t i) {
  int               j,k                           = 0;
  int enb_id;
  int32_t     my_int;
  const char       *active_enb[MAX_ENB];
  char             *address                       = NULL;
  char             *cidr                          = NULL;
  // for no gcc warnings
  (void)my_int;
  memset((char *)active_enb,     0, MAX_ENB * sizeof(char *));
  paramdef_t ENBSParams[] = ENBSPARAMS_DESC;
  paramdef_t ENBParams[]  = ENBPARAMS_DESC;
  paramlist_def_t ENBParamList = {ENB_CONFIG_STRING_ENB_LIST,NULL,0};
  /* get global parameters, defined outside any section in the config file */
  config_get( ENBSParams,sizeof(ENBSParams)/sizeof(paramdef_t),NULL);
  AssertFatal (i<ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt,
               "Failed to parse config file %s, %uth attribute %s \n",
               RC.config_file_name, i, ENB_CONFIG_STRING_ACTIVE_ENBS);

  if (ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt>0) {
    // Output a list of all eNBs.
    config_getlist( &ENBParamList,ENBParams,sizeof(ENBParams)/sizeof(paramdef_t),NULL);

    if (ENBParamList.numelt > 0) {
      for (k = 0; k < ENBParamList.numelt; k++) {
        if (ENBParamList.paramarray[k][ENB_ENB_ID_IDX].uptr == NULL) {
          // Calculate a default eNB ID
          if (EPC_MODE_ENABLED) {
            uint32_t hash;
            hash = s1ap_generate_eNB_id ();
            enb_id = k + (hash & 0xFFFF8);
          } else {
            enb_id = k;
          }
        } else {
          enb_id = *(ENBParamList.paramarray[k][ENB_ENB_ID_IDX].uptr);
        }

        // search if in active list
        for (j=0; j < ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt; j++) {
          if (strcmp(ENBSParams[ENB_ACTIVE_ENBS_IDX].strlistptr[j], *(ENBParamList.paramarray[k][ENB_ENB_NAME_IDX].strptr)) == 0) {
            paramdef_t PLMNParams[] = PLMNPARAMS_DESC;
            paramlist_def_t PLMNParamList = {ENB_CONFIG_STRING_PLMN_LIST, NULL, 0};
            /* map parameter checking array instances to parameter definition array instances */
            checkedparam_t config_check_PLMNParams [] = PLMNPARAMS_CHECK;

            for (int I = 0; I < sizeof(PLMNParams) / sizeof(paramdef_t); ++I)
              PLMNParams[I].chkPptr = &(config_check_PLMNParams[I]);

            paramdef_t S1Params[]  = S1PARAMS_DESC;
            paramlist_def_t S1ParamList = {ENB_CONFIG_STRING_MME_IP_ADDRESS,NULL,0};
            paramdef_t SCTPParams[]  = SCTPPARAMS_DESC;
            paramdef_t NETParams[]  =  NETPARAMS_DESC;
            char aprefix[MAX_OPTNAME_SIZE*2 + 8];
            sprintf(aprefix,"%s.[%i]",ENB_CONFIG_STRING_ENB_LIST,k);
            S1AP_REGISTER_ENB_REQ (msg_p).eNB_id = enb_id;

            if (strcmp(*(ENBParamList.paramarray[k][ENB_CELL_TYPE_IDX].strptr), "CELL_MACRO_ENB") == 0) {
              S1AP_REGISTER_ENB_REQ (msg_p).cell_type = CELL_MACRO_ENB;
            } else  if (strcmp(*(ENBParamList.paramarray[k][ENB_CELL_TYPE_IDX].strptr), "CELL_HOME_ENB") == 0) {
              S1AP_REGISTER_ENB_REQ (msg_p).cell_type = CELL_HOME_ENB;
            } else {
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for cell_type choice: CELL_MACRO_ENB or CELL_HOME_ENB !\n",
                           RC.config_file_name, i, *(ENBParamList.paramarray[k][ENB_CELL_TYPE_IDX].strptr));
            }

            S1AP_REGISTER_ENB_REQ (msg_p).eNB_name         = strdup(*(ENBParamList.paramarray[k][ENB_ENB_NAME_IDX].strptr));
            S1AP_REGISTER_ENB_REQ(msg_p).tac               = *ENBParamList.paramarray[k][ENB_TRACKING_AREA_CODE_IDX].uptr;
            AssertFatal(!ENBParamList.paramarray[k][ENB_MOBILE_COUNTRY_CODE_IDX_OLD].strptr
                        && !ENBParamList.paramarray[k][ENB_MOBILE_NETWORK_CODE_IDX_OLD].strptr,
                        "It seems that you use an old configuration file. Please change the existing\n"
                        "    tracking_area_code  =  \"1\";\n"
                        "    mobile_country_code =  \"208\";\n"
                        "    mobile_network_code =  \"93\";\n"
                        "to\n"
                        "    tracking_area_code  =  1; // no string!!\n"
                        "    plmn_list = ( { mcc = 208; mnc = 93; mnc_length = 2; } )\n");
            config_getlist(&PLMNParamList, PLMNParams, sizeof(PLMNParams)/sizeof(paramdef_t), aprefix);

            if (PLMNParamList.numelt < 1 || PLMNParamList.numelt > 6)
              AssertFatal(0, "The number of PLMN IDs must be in [1,6], but is %d\n",
                          PLMNParamList.numelt);

            S1AP_REGISTER_ENB_REQ(msg_p).num_plmn = PLMNParamList.numelt;

            for (int l = 0; l < PLMNParamList.numelt; ++l) {
              S1AP_REGISTER_ENB_REQ(msg_p).mcc[l] = *PLMNParamList.paramarray[l][ENB_MOBILE_COUNTRY_CODE_IDX].uptr;
              S1AP_REGISTER_ENB_REQ(msg_p).mnc[l] = *PLMNParamList.paramarray[l][ENB_MOBILE_NETWORK_CODE_IDX].uptr;
              S1AP_REGISTER_ENB_REQ(msg_p).mnc_digit_length[l] = *PLMNParamList.paramarray[l][ENB_MNC_DIGIT_LENGTH].u8ptr;
              AssertFatal(S1AP_REGISTER_ENB_REQ(msg_p).mnc_digit_length[l] == 3
                          || S1AP_REGISTER_ENB_REQ(msg_p).mnc[l] < 100,
                          "MNC %d cannot be encoded in two digits as requested (change mnc_digit_length to 3)\n",
                          S1AP_REGISTER_ENB_REQ(msg_p).mnc[l]);
            }

            S1AP_REGISTER_ENB_REQ(msg_p).default_drx = 0;
            config_getlist( &S1ParamList,S1Params,sizeof(S1Params)/sizeof(paramdef_t),aprefix);
            S1AP_REGISTER_ENB_REQ (msg_p).nb_mme = 0;

            for (int l = 0; l < S1ParamList.numelt; l++) {
              S1AP_REGISTER_ENB_REQ (msg_p).nb_mme += 1;
              strcpy(S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[l].ipv4_address,*(S1ParamList.paramarray[l][ENB_MME_IPV4_ADDRESS_IDX].strptr));
              strcpy(S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[l].ipv6_address,*(S1ParamList.paramarray[l][ENB_MME_IPV6_ADDRESS_IDX].strptr));

              if (strcmp(*(S1ParamList.paramarray[l][ENB_MME_IP_ADDRESS_PREFERENCE_IDX].strptr), "ipv4") == 0) {
                S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[l].ipv4 = 1;
              } else if (strcmp(*(S1ParamList.paramarray[l][ENB_MME_IP_ADDRESS_PREFERENCE_IDX].strptr), "ipv6") == 0) {
                S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[l].ipv6 = 1;
              } else if (strcmp(*(S1ParamList.paramarray[l][ENB_MME_IP_ADDRESS_PREFERENCE_IDX].strptr), "no") == 0) {
                S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[l].ipv4 = 1;
                S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[l].ipv6 = 1;
              }

              if (S1ParamList.paramarray[l][ENB_MME_BROADCAST_PLMN_INDEX].iptr)
                S1AP_REGISTER_ENB_REQ(msg_p).broadcast_plmn_num[l] = S1ParamList.paramarray[l][ENB_MME_BROADCAST_PLMN_INDEX].numelt;
              else
                S1AP_REGISTER_ENB_REQ(msg_p).broadcast_plmn_num[l] = 0;

              AssertFatal(S1AP_REGISTER_ENB_REQ(msg_p).broadcast_plmn_num[l] <= S1AP_REGISTER_ENB_REQ(msg_p).num_plmn,
                          "List of broadcast PLMN to be sent to MME can not be longer than actual "
                          "PLMN list (max %d, but is %d)\n",
                          S1AP_REGISTER_ENB_REQ(msg_p).num_plmn,
                          S1AP_REGISTER_ENB_REQ(msg_p).broadcast_plmn_num[l]);

              for (int el = 0; el < S1AP_REGISTER_ENB_REQ(msg_p).broadcast_plmn_num[l]; ++el) {
                /* UINTARRAY gets mapped to int, see config_libconfig.c:223 */
                S1AP_REGISTER_ENB_REQ(msg_p).broadcast_plmn_index[l][el] = S1ParamList.paramarray[l][ENB_MME_BROADCAST_PLMN_INDEX].iptr[el];
                AssertFatal(S1AP_REGISTER_ENB_REQ(msg_p).broadcast_plmn_index[l][el] >= 0
                            && S1AP_REGISTER_ENB_REQ(msg_p).broadcast_plmn_index[l][el] < S1AP_REGISTER_ENB_REQ(msg_p).num_plmn,
                            "index for MME's MCC/MNC (%d) is an invalid index for the registered PLMN IDs (%d)\n",
                            S1AP_REGISTER_ENB_REQ(msg_p).broadcast_plmn_index[l][el],
                            S1AP_REGISTER_ENB_REQ(msg_p).num_plmn);
              }

              /* if no broadcasst_plmn array is defined, fill default values */
              if (S1AP_REGISTER_ENB_REQ(msg_p).broadcast_plmn_num[l] == 0) {
                S1AP_REGISTER_ENB_REQ(msg_p).broadcast_plmn_num[l] = S1AP_REGISTER_ENB_REQ(msg_p).num_plmn;

                for (int el = 0; el < S1AP_REGISTER_ENB_REQ(msg_p).num_plmn; ++el)
                  S1AP_REGISTER_ENB_REQ(msg_p).broadcast_plmn_index[l][el] = el;
              }
            }

            // SCTP SETTING
            S1AP_REGISTER_ENB_REQ (msg_p).sctp_out_streams = SCTP_OUT_STREAMS;
            S1AP_REGISTER_ENB_REQ (msg_p).sctp_in_streams  = SCTP_IN_STREAMS;

            if (EPC_MODE_ENABLED) {
              sprintf(aprefix,"%s.[%i].%s",ENB_CONFIG_STRING_ENB_LIST,k,ENB_CONFIG_STRING_SCTP_CONFIG);
              config_get( SCTPParams,sizeof(SCTPParams)/sizeof(paramdef_t),aprefix);
              S1AP_REGISTER_ENB_REQ (msg_p).sctp_in_streams = (uint16_t)*(SCTPParams[ENB_SCTP_INSTREAMS_IDX].uptr);
              S1AP_REGISTER_ENB_REQ (msg_p).sctp_out_streams = (uint16_t)*(SCTPParams[ENB_SCTP_OUTSTREAMS_IDX].uptr);
            }

            sprintf(aprefix,"%s.[%i].%s",ENB_CONFIG_STRING_ENB_LIST,k,ENB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);
            // NETWORK_INTERFACES
            config_get( NETParams,sizeof(NETParams)/sizeof(paramdef_t),aprefix);
            //    S1AP_REGISTER_ENB_REQ (msg_p).enb_interface_name_for_S1U = strdup(enb_interface_name_for_S1U);
            cidr = *(NETParams[ENB_IPV4_ADDRESS_FOR_S1_MME_IDX].strptr);
            address = strtok(cidr, "/");
            S1AP_REGISTER_ENB_REQ (msg_p).enb_ip_address.ipv6 = 0;
            S1AP_REGISTER_ENB_REQ (msg_p).enb_ip_address.ipv4 = 1;
            strcpy(S1AP_REGISTER_ENB_REQ (msg_p).enb_ip_address.ipv4_address, address);
            break;
          }
        }
      }
    }
  }

  return 0;
}

int RCconfig_X2(MessageDef *msg_p, uint32_t i) {
  int   I, J, j, k, l;
  int   enb_id;
  char *address = NULL;
  char *cidr    = NULL;
  paramdef_t ENBSParams[] = ENBSPARAMS_DESC;
  paramdef_t ENBParams[]  = ENBPARAMS_DESC;
  paramlist_def_t ENBParamList = {ENB_CONFIG_STRING_ENB_LIST,NULL,0};
  /* get global parameters, defined outside any section in the config file */
  config_get( ENBSParams,sizeof(ENBSParams)/sizeof(paramdef_t),NULL);
  /* define CC params */
  int32_t Nid_cell = 0;
  char *frame_type, *prefix_type, *pbch_repetition, *prach_high_speed,
    *pusch_hoppingMode, *pusch_enable64QAM, *pusch_groupHoppingEnabled,
    *pusch_sequenceHoppingEnabled, *phich_duration, *phich_resource,
    *srs_enable, *srs_ackNackST, *srs_MaxUpPts, *pusch_alpha,
    *pucch_deltaF_Format1, *pucch_deltaF_Format1b, *pucch_deltaF_Format2,
    *pucch_deltaF_Format2a, *pucch_deltaF_Format2b,
    *rach_preamblesGroupAConfig, *rach_messagePowerOffsetGroupB, *pcch_nB;
  long long int     downlink_frequency;
  int32_t tdd_config, tdd_config_s, eutra_band, uplink_frequency_offset,
    Nid_cell_mbsfn, N_RB_DL, nb_antenna_ports, prach_root, prach_config_index,
    prach_zero_correlation, prach_freq_offset, pucch_delta_shift,
    pucch_nRB_CQI, pucch_nCS_AN, pucch_n1_AN, pdsch_referenceSignalPower,
    pdsch_p_b, pusch_n_SB, pusch_hoppingOffset, pusch_groupAssignment,
    pusch_nDMRS1, srs_BandwidthConfig, srs_SubframeConfig, pusch_p0_Nominal,
    pucch_p0_Nominal, msg3_delta_Preamble;
  char *rach_numberOfRA_Preambles;
  int32_t rach_sizeOfRA_PreamblesGroupA, rach_messageSizeGroupA,
    rach_powerRampingStep, rach_preambleInitialReceivedTargetPower,
    rach_preambleTransMax, rach_raResponseWindowSize,
    rach_macContentionResolutionTimer, rach_maxHARQ_Msg3Tx,
    pcch_defaultPagingCycle, bcch_modificationPeriodCoeff,
    ue_TimersAndConstants_t300, ue_TimersAndConstants_t301,
    ue_TimersAndConstants_t310, ue_TimersAndConstants_t311,
    ue_TimersAndConstants_n310, ue_TimersAndConstants_n311,
    ue_TransmissionMode, ue_multiple_max;
  char*   prach_ConfigCommon_v1310                          = NULL;
  char*   mpdcch_startSF_CSS_RA_r13                         = NULL;
  char*   mpdcch_startSF_CSS_RA_r13_val                     = NULL;
  char*   pdsch_maxNumRepetitionCEmodeA_r13                 = NULL;
  char*   pdsch_maxNumRepetitionCEmodeB_r13                 = NULL;
  
  char*   pusch_maxNumRepetitionCEmodeA_r13                 = 0;
  char*   pusch_maxNumRepetitionCEmodeB_r13                 = 0;
  int     prach_HoppingOffset_r13                           = 0;

  // avoid gcc warnings
  (void)pdsch_maxNumRepetitionCEmodeB_r13;
  (void)pusch_maxNumRepetitionCEmodeB_r13;

  const char       *rxPool_sc_CP_Len;
  const char       *rxPool_sc_Period;
  const char       *rxPool_data_CP_Len;
  libconfig_int     rxPool_ResourceConfig_prb_Num;
  libconfig_int     rxPool_ResourceConfig_prb_Start;
  libconfig_int     rxPool_ResourceConfig_prb_End;
  const char       *rxPool_ResourceConfig_offsetIndicator_present;
  libconfig_int     rxPool_ResourceConfig_offsetIndicator_choice;
  const char       *rxPool_ResourceConfig_subframeBitmap_present;
  char             *rxPool_ResourceConfig_subframeBitmap_choice_bs_buf;
  libconfig_int     rxPool_ResourceConfig_subframeBitmap_choice_bs_size;
  libconfig_int     rxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused;
  //SIB19
  //for discRxPool
  const char       *discRxPool_cp_Len;
  const char       *discRxPool_discPeriod;
  libconfig_int     discRxPool_numRetx;
  libconfig_int     discRxPool_numRepetition;
  libconfig_int     discRxPool_ResourceConfig_prb_Num;
  libconfig_int     discRxPool_ResourceConfig_prb_Start;
  libconfig_int     discRxPool_ResourceConfig_prb_End;
  const char       *discRxPool_ResourceConfig_offsetIndicator_present;
  libconfig_int     discRxPool_ResourceConfig_offsetIndicator_choice;
  const char       *discRxPool_ResourceConfig_subframeBitmap_present;
  char             *discRxPool_ResourceConfig_subframeBitmap_choice_bs_buf;
  libconfig_int     discRxPool_ResourceConfig_subframeBitmap_choice_bs_size;
  libconfig_int     discRxPool_ResourceConfig_subframeBitmap_choice_bs_bits_unused;
  //for discRxPoolPS
  const char       *discRxPoolPS_cp_Len;
  const char       *discRxPoolPS_discPeriod;
  libconfig_int     discRxPoolPS_numRetx;
  libconfig_int     discRxPoolPS_numRepetition;
  libconfig_int     discRxPoolPS_ResourceConfig_prb_Num;
  libconfig_int     discRxPoolPS_ResourceConfig_prb_Start;
  libconfig_int     discRxPoolPS_ResourceConfig_prb_End;
  const char       *discRxPoolPS_ResourceConfig_offsetIndicator_present;
  libconfig_int     discRxPoolPS_ResourceConfig_offsetIndicator_choice;
  const char       *discRxPoolPS_ResourceConfig_subframeBitmap_present;
  char             *discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_buf;
  libconfig_int     discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_size;
  libconfig_int     discRxPoolPS_ResourceConfig_subframeBitmap_choice_bs_bits_unused;
  checkedparam_t config_check_CCparams[] = CCPARAMS_CHECK;
  paramdef_t CCsParams[] = CCPARAMS_DESC;
  paramlist_def_t CCsParamList = {ENB_CONFIG_STRING_COMPONENT_CARRIERS, NULL, 0};

  /* map parameter checking array instances to parameter definition array instances */
  for (I = 0; I < (sizeof(CCsParams) / sizeof(paramdef_t)); I++) {
    CCsParams[I].chkPptr = &(config_check_CCparams[I]);
  }

  /*#if defined(ENABLE_ITTI) && defined(ENABLE_USE_MME)
    if (strcasecmp( *(ENBSParams[ENB_ASN1_VERBOSITY_IDX].strptr), ENB_CONFIG_STRING_ASN1_VERBOSITY_NONE) == 0) {
    asn_debug      = 0;
    asn1_xer_print = 0;
    } else if (strcasecmp( *(ENBSParams[ENB_ASN1_VERBOSITY_IDX].strptr), ENB_CONFIG_STRING_ASN1_VERBOSITY_INFO) == 0) {
    asn_debug      = 1;
    asn1_xer_print = 1;
    } else if (strcasecmp(*(ENBSParams[ENB_ASN1_VERBOSITY_IDX].strptr) , ENB_CONFIG_STRING_ASN1_VERBOSITY_ANNOYING) == 0) {
    asn_debug      = 1;
    asn1_xer_print = 2;
    } else {
    asn_debug      = 0;
    asn1_xer_print = 0;
    }
    #endif */
  AssertFatal(i < ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt,
    "Failed to parse config file %s, %uth attribute %s \n",
    RC.config_file_name, i, ENB_CONFIG_STRING_ACTIVE_ENBS);

  if (ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt > 0) {
  // Output a list of all eNBs.
  config_getlist( &ENBParamList,ENBParams,sizeof(ENBParams)/sizeof(paramdef_t),NULL);

  if (ENBParamList.numelt > 0) {
  for (k = 0; k < ENBParamList.numelt; k++) {
  if (ENBParamList.paramarray[k][ENB_ENB_ID_IDX].uptr == NULL) {
  // Calculate a default eNB ID
# if defined(ENABLE_USE_MME)
  uint32_t hash;
  hash = s1ap_generate_eNB_id ();
  enb_id = k + (hash & 0xFFFF8);
# else
  enb_id = k;
# endif
} else {
  enb_id = *(ENBParamList.paramarray[k][ENB_ENB_ID_IDX].uptr);
}

  // search if in active list
  for (j = 0; j < ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt; j++) {
  if (strcmp(ENBSParams[ENB_ACTIVE_ENBS_IDX].strlistptr[j], *(ENBParamList.paramarray[k][ENB_ENB_NAME_IDX].strptr)) == 0) {
  paramdef_t PLMNParams[] = PLMNPARAMS_DESC;
  paramlist_def_t PLMNParamList = {ENB_CONFIG_STRING_PLMN_LIST, NULL, 0};
  /* map parameter checking array instances to parameter definition array instances */
  checkedparam_t config_check_PLMNParams [] = PLMNPARAMS_CHECK;

  for (int I = 0; I < sizeof(PLMNParams) / sizeof(paramdef_t); ++I)
    PLMNParams[I].chkPptr = &(config_check_PLMNParams[I]);

  paramdef_t X2Params[]  = X2PARAMS_DESC;
  paramlist_def_t X2ParamList = {ENB_CONFIG_STRING_TARGET_ENB_X2_IP_ADDRESS,NULL,0};
  paramdef_t SCTPParams[]  = SCTPPARAMS_DESC;
  paramdef_t NETParams[]  =  NETPARAMS_DESC;
  /* TODO: fix the size - if set lower we have a crash (MAX_OPTNAME_SIZE was 64 when this code was written) */
  /* this is most probably a problem with the config module */
  char aprefix[MAX_OPTNAME_SIZE*80 + 8];
  sprintf(aprefix,"%s.[%i]",ENB_CONFIG_STRING_ENB_LIST,k);
  /* Some default/random parameters */
  X2AP_REGISTER_ENB_REQ (msg_p).eNB_id = enb_id;

  if (strcmp(*(ENBParamList.paramarray[k][ENB_CELL_TYPE_IDX].strptr), "CELL_MACRO_ENB") == 0) {
  X2AP_REGISTER_ENB_REQ (msg_p).cell_type = CELL_MACRO_ENB;
} else  if (strcmp(*(ENBParamList.paramarray[k][ENB_CELL_TYPE_IDX].strptr), "CELL_HOME_ENB") == 0) {
  X2AP_REGISTER_ENB_REQ (msg_p).cell_type = CELL_HOME_ENB;
} else {
  AssertFatal (0,
    "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for cell_type choice: CELL_MACRO_ENB or CELL_HOME_ENB !\n",
    RC.config_file_name, i, *(ENBParamList.paramarray[k][ENB_CELL_TYPE_IDX].strptr));
}

  X2AP_REGISTER_ENB_REQ (msg_p).eNB_name         = strdup(*(ENBParamList.paramarray[k][ENB_ENB_NAME_IDX].strptr));
  X2AP_REGISTER_ENB_REQ (msg_p).tac              = *ENBParamList.paramarray[k][ENB_TRACKING_AREA_CODE_IDX].uptr;
  config_getlist(&PLMNParamList, PLMNParams, sizeof(PLMNParams)/sizeof(paramdef_t), aprefix);

  if (PLMNParamList.numelt < 1 || PLMNParamList.numelt > 6)
    AssertFatal(0, "The number of PLMN IDs must be in [1,6], but is %d\n",
    PLMNParamList.numelt);

  if (PLMNParamList.numelt > 1)
    LOG_W(X2AP, "X2AP currently handles only one PLMN, ignoring the others!\n");

  X2AP_REGISTER_ENB_REQ (msg_p).mcc = *PLMNParamList.paramarray[0][ENB_MOBILE_COUNTRY_CODE_IDX].uptr;
  X2AP_REGISTER_ENB_REQ (msg_p).mnc = *PLMNParamList.paramarray[0][ENB_MOBILE_NETWORK_CODE_IDX].uptr;
  X2AP_REGISTER_ENB_REQ (msg_p).mnc_digit_length = *PLMNParamList.paramarray[0][ENB_MNC_DIGIT_LENGTH].u8ptr;
  AssertFatal(X2AP_REGISTER_ENB_REQ(msg_p).mnc_digit_length == 3
    || X2AP_REGISTER_ENB_REQ(msg_p).mnc < 100,
    "MNC %d cannot be encoded in two digits as requested (change mnc_digit_length to 3)\n",
    X2AP_REGISTER_ENB_REQ(msg_p).mnc);

  /* CC params */
  config_getlist(&CCsParamList, NULL, 0, aprefix);

  X2AP_REGISTER_ENB_REQ (msg_p).num_cc = CCsParamList.numelt;

  if (CCsParamList.numelt > 0) {
  //char ccspath[MAX_OPTNAME_SIZE*2 + 16];
  for (J = 0; J < CCsParamList.numelt ; J++) {
  sprintf(aprefix, "%s.[%i].%s.[%i]", ENB_CONFIG_STRING_ENB_LIST, k, ENB_CONFIG_STRING_COMPONENT_CARRIERS, J);
  printf("X2: Getting %s\n",aprefix);
  config_get(CCsParams, sizeof(CCsParams)/sizeof(paramdef_t), aprefix);
  X2AP_REGISTER_ENB_REQ (msg_p).eutra_band[J] = eutra_band;
  X2AP_REGISTER_ENB_REQ (msg_p).downlink_frequency[J] = (uint32_t) downlink_frequency;
  X2AP_REGISTER_ENB_REQ (msg_p).uplink_frequency_offset[J] = (unsigned int) uplink_frequency_offset;
  X2AP_REGISTER_ENB_REQ (msg_p).Nid_cell[J]= Nid_cell;

  if (Nid_cell>503) {
  AssertFatal (0,
    "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for Nid_cell choice: 0...503 !\n",
    RC.config_file_name, k, Nid_cell);
}

  X2AP_REGISTER_ENB_REQ (msg_p).N_RB_DL[J]= N_RB_DL;

  if ((N_RB_DL!=6) && (N_RB_DL!=15) && (N_RB_DL!=25) && (N_RB_DL!=50) && (N_RB_DL!=75) && (N_RB_DL!=100)) {
  AssertFatal (0,
    "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for N_RB_DL choice: 6,15,25,50,75,100 !\n",
    RC.config_file_name, k, N_RB_DL);
}

  if (strcmp(frame_type, "FDD") == 0) {
  X2AP_REGISTER_ENB_REQ (msg_p).frame_type[J] = FDD;
} else  if (strcmp(frame_type, "TDD") == 0) {
  X2AP_REGISTER_ENB_REQ (msg_p).frame_type[J] = TDD;
} else {
  AssertFatal (0,
    "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for frame_type choice: FDD or TDD !\n",
    RC.config_file_name, k, frame_type);
}

  X2AP_REGISTER_ENB_REQ (msg_p).fdd_earfcn_DL[J] = to_earfcn_DL(eutra_band, downlink_frequency, N_RB_DL);
  X2AP_REGISTER_ENB_REQ (msg_p).fdd_earfcn_UL[J] = to_earfcn_UL(eutra_band, downlink_frequency + uplink_frequency_offset, N_RB_DL);
}
}

  sprintf(aprefix,"%s.[%i]",ENB_CONFIG_STRING_ENB_LIST,k);
  config_getlist( &X2ParamList,X2Params,sizeof(X2Params)/sizeof(paramdef_t),aprefix);
  AssertFatal(X2ParamList.numelt <= X2AP_MAX_NB_ENB_IP_ADDRESS,
    "value of X2ParamList.numelt %d must be lower than X2AP_MAX_NB_ENB_IP_ADDRESS %d value: reconsider to increase X2AP_MAX_NB_ENB_IP_ADDRESS\n",
    X2ParamList.numelt,X2AP_MAX_NB_ENB_IP_ADDRESS);

  X2AP_REGISTER_ENB_REQ (msg_p).nb_x2 = 0;

  for (l = 0; l < X2ParamList.numelt; l++) {
  X2AP_REGISTER_ENB_REQ (msg_p).nb_x2 += 1;
  strcpy(X2AP_REGISTER_ENB_REQ (msg_p).target_enb_x2_ip_address[l].ipv4_address,*(X2ParamList.paramarray[l][ENB_X2_IPV4_ADDRESS_IDX].strptr));
  strcpy(X2AP_REGISTER_ENB_REQ (msg_p).target_enb_x2_ip_address[l].ipv6_address,*(X2ParamList.paramarray[l][ENB_X2_IPV6_ADDRESS_IDX].strptr));

  if (strcmp(*(X2ParamList.paramarray[l][ENB_X2_IP_ADDRESS_PREFERENCE_IDX].strptr), "ipv4") == 0) {
  X2AP_REGISTER_ENB_REQ (msg_p).target_enb_x2_ip_address[l].ipv4 = 1;
} else if (strcmp(*(X2ParamList.paramarray[l][ENB_X2_IP_ADDRESS_PREFERENCE_IDX].strptr), "ipv6") == 0) {
  X2AP_REGISTER_ENB_REQ (msg_p).target_enb_x2_ip_address[l].ipv6 = 1;
} else if (strcmp(*(X2ParamList.paramarray[l][ENB_X2_IP_ADDRESS_PREFERENCE_IDX].strptr), "no") == 0) {
  X2AP_REGISTER_ENB_REQ (msg_p).target_enb_x2_ip_address[l].ipv4 = 1;
  X2AP_REGISTER_ENB_REQ (msg_p).target_enb_x2_ip_address[l].ipv6 = 1;
}
}

  // SCTP SETTING
  X2AP_REGISTER_ENB_REQ (msg_p).sctp_out_streams = SCTP_OUT_STREAMS;
  X2AP_REGISTER_ENB_REQ (msg_p).sctp_in_streams  = SCTP_IN_STREAMS;
# if defined(ENABLE_USE_MME)
  sprintf(aprefix,"%s.[%i].%s",ENB_CONFIG_STRING_ENB_LIST,k,ENB_CONFIG_STRING_SCTP_CONFIG);
  config_get( SCTPParams,sizeof(SCTPParams)/sizeof(paramdef_t),aprefix);
  X2AP_REGISTER_ENB_REQ (msg_p).sctp_in_streams = (uint16_t)*(SCTPParams[ENB_SCTP_INSTREAMS_IDX].uptr);
  X2AP_REGISTER_ENB_REQ (msg_p).sctp_out_streams = (uint16_t)*(SCTPParams[ENB_SCTP_OUTSTREAMS_IDX].uptr);
#endif
  sprintf(aprefix,"%s.[%i].%s",ENB_CONFIG_STRING_ENB_LIST,k,ENB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);
  // NETWORK_INTERFACES
  config_get( NETParams,sizeof(NETParams)/sizeof(paramdef_t),aprefix);
  X2AP_REGISTER_ENB_REQ (msg_p).enb_port_for_X2C = (uint32_t)*(NETParams[ENB_PORT_FOR_X2C_IDX].uptr);

  if ((NETParams[ENB_IPV4_ADDR_FOR_X2C_IDX].strptr == NULL) || (X2AP_REGISTER_ENB_REQ (msg_p).enb_port_for_X2C == 0)) {
    LOG_E(RRC,"Add eNB IPv4 address and/or port for X2C in the CONF file!\n");
    exit(1);
  }

  cidr = *(NETParams[ENB_IPV4_ADDR_FOR_X2C_IDX].strptr);
  address = strtok(cidr, "/");
  X2AP_REGISTER_ENB_REQ (msg_p).enb_x2_ip_address.ipv6 = 0;
  X2AP_REGISTER_ENB_REQ (msg_p).enb_x2_ip_address.ipv4 = 1;
  strcpy(X2AP_REGISTER_ENB_REQ (msg_p).enb_x2_ip_address.ipv4_address, address);
  }
  }
}
}
}

return 0;
}

int RCconfig_parallel(void) {
  char *parallel_conf = NULL;
  char *worker_conf   = NULL;
  extern char *parallel_config;
  extern char *worker_config;
  paramdef_t ThreadParams[]  = THREAD_CONF_DESC;
  paramlist_def_t THREADParamList = {THREAD_CONFIG_STRING_THREAD_STRUCT,NULL,0};
  config_getlist( &THREADParamList,NULL,0,NULL);

  if(THREADParamList.numelt>0) {
    config_getlist( &THREADParamList,ThreadParams,sizeof(ThreadParams)/sizeof(paramdef_t),NULL);
    parallel_conf = strdup(*(THREADParamList.paramarray[0][THREAD_PARALLEL_IDX].strptr));
  } else {
    parallel_conf = strdup("PARALLEL_RU_L1_TRX_SPLIT");
  }

  if(THREADParamList.numelt>0) {
    config_getlist( &THREADParamList,ThreadParams,sizeof(ThreadParams)/sizeof(paramdef_t),NULL);
    worker_conf   = strdup(*(THREADParamList.paramarray[0][THREAD_WORKER_IDX].strptr));
  } else {
    worker_conf   = strdup("WORKER_ENABLE");
  }

  if(parallel_config == NULL) set_parallel_conf(parallel_conf);

  if(worker_config == NULL)   set_worker_conf(worker_conf);

  return 0;
}

void RCConfig(void) {
  paramlist_def_t MACRLCParamList = {CONFIG_STRING_MACRLC_LIST,NULL,0};
  paramlist_def_t L1ParamList = {CONFIG_STRING_L1_LIST,NULL,0};
  paramlist_def_t RUParamList = {CONFIG_STRING_RU_LIST,NULL,0};
  paramdef_t ENBSParams[] = ENBSPARAMS_DESC;
  paramlist_def_t CCsParamList = {ENB_CONFIG_STRING_COMPONENT_CARRIERS,NULL,0};
  char aprefix[MAX_OPTNAME_SIZE*2 + 8];
  /* get global parameters, defined outside any section in the config file */
  printf("Getting ENBSParams\n");
  config_get( ENBSParams,sizeof(ENBSParams)/sizeof(paramdef_t),NULL);
# if defined(ENABLE_USE_MME)
  EPC_MODE_ENABLED = ((*ENBSParams[ENB_NOS1_IDX].uptr) == 0);
#endif
  RC.nb_inst = ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt;

  if (RC.nb_inst > 0) {
    RC.nb_CC = (int *)malloc((1+RC.nb_inst)*sizeof(int));
    for (int i=0; i<RC.nb_inst; i++) {
      sprintf(aprefix,"%s.[%i]",ENB_CONFIG_STRING_ENB_LIST,i);
      config_getlist( &CCsParamList,NULL,0, aprefix);
      RC.nb_CC[i]    = CCsParamList.numelt;
    }
  }

  // Get num MACRLC instances
  config_getlist( &MACRLCParamList,NULL,0, NULL);
  RC.nb_macrlc_inst  = MACRLCParamList.numelt;
  // Get num L1 instances
  config_getlist( &L1ParamList,NULL,0, NULL);
  RC.nb_L1_inst = L1ParamList.numelt;
  // Get num RU instances
  config_getlist( &RUParamList,NULL,0, NULL);
  RC.nb_RU     = RUParamList.numelt;
  RCconfig_parallel();
}
