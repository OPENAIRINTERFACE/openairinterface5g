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
#include <dlfcn.h>

#include "common/utils/LOG/log.h"
#include "assertions.h"
#include "enb_config.h"
#include "UTIL/OTG/otg.h"
#include "UTIL/OTG/otg_externs.h"
#include "intertask_interface.h"
#include "s1ap_eNB.h"
#include "sctp_eNB_task.h"
#include "common/ran_context.h"
#include "sctp_default_values.h"
#include "LTE_SystemInformationBlockType2.h"
#include "LAYER2/MAC/mac_extern.h"
#include "LAYER2/MAC/mac_proto.h"
#include "PHY/phy_extern.h"
#include "PHY/INIT/phy_init.h"
#include "targets/ARCH/ETHERNET/USERSPACE/LIB/ethernet_lib.h"
#include "nfapi_vnf.h"
#include "nfapi_pnf.h"
#include "targets/RT/USER/lte-softmodem.h"
#include "L1_paramdef.h"
#include "MACRLC_paramdef.h"
#include "common/config/config_userapi.h"
#include "RRC_config_tools.h"
#include "enb_paramdef.h"
#include "proto_agent.h"
#include "executables/thread-common.h"
#include <openair3/ocp-gtpu/gtp_itf.h>

extern uint32_t to_earfcn_DL(int eutra_bandP, uint32_t dl_CarrierFreq, uint32_t bw);
extern uint32_t to_earfcn_UL(int eutra_bandP, uint32_t ul_CarrierFreq, uint32_t bw);
extern char *parallel_config;
extern char *worker_config;

RAN_CONTEXT_t RC;

void RCconfig_flexran() {
  /* get number of eNBs */
  paramdef_t ENBSParams[] = ENBSPARAMS_DESC;
  config_get(ENBSParams, sizeof(ENBSParams)/sizeof(paramdef_t), NULL);
  uint16_t num_enbs = ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt;
  paramdef_t flexranParams[] = FLEXRANPARAMS_DESC;
  config_get(flexranParams, sizeof(flexranParams)/sizeof(paramdef_t), CONFIG_STRING_NETWORK_CONTROLLER_CONFIG);

  if (!RC.flexran) {
    RC.flexran = calloc(num_enbs, sizeof(flexran_agent_info_t *));
    AssertFatal(RC.flexran,
                "can't ALLOCATE %zu Bytes for %d flexran agent info with size %zu\n",
                num_enbs * sizeof(flexran_agent_info_t *),
                num_enbs, sizeof(flexran_agent_info_t *));
  }

  for (uint16_t i = 0; i < num_enbs; i++) {
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
    RC.flexran[i]->mod_id  = i;
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
        RC.eNB[j]  = (PHY_VARS_eNB **)malloc((1+MAX_NUM_CCs)*sizeof(PHY_VARS_eNB *));
        LOG_I(PHY,"RC.eNB[%d] = %p\n",j,RC.eNB[j]);
        memset(RC.eNB[j],0,(1+MAX_NUM_CCs)*sizeof(PHY_VARS_eNB *));
      }

      for (i=0; i<RC.nb_L1_CC[j]; i++) {
        if (RC.eNB[j][i] == NULL) {
          RC.eNB[j][i] = (PHY_VARS_eNB *)calloc(1, sizeof(PHY_VARS_eNB));
          LOG_I(PHY,"RC.eNB[%d][%d] = %p\n",j,i,RC.eNB[j][i]);
          RC.eNB[j][i]->Mod_id  = j;
          RC.eNB[j][i]->CC_id   = i;
        }
      }

      if (strcmp(*(L1_ParamList.paramarray[j][L1_TRANSPORT_N_PREFERENCE_IDX].strptr), "local_mac") == 0) {
      } else if (strcmp(*(L1_ParamList.paramarray[j][L1_TRANSPORT_N_PREFERENCE_IDX].strptr), "nfapi") == 0) {
        RC.eNB[j][0]->eth_params_n.local_if_name            = strdup(*(L1_ParamList.paramarray[j][L1_LOCAL_N_IF_NAME_IDX].strptr));
        RC.eNB[j][0]->eth_params_n.my_addr                  = strdup(*(L1_ParamList.paramarray[j][L1_LOCAL_N_ADDRESS_IDX].strptr));
        RC.eNB[j][0]->eth_params_n.remote_addr              = strdup(*(L1_ParamList.paramarray[j][L1_REMOTE_N_ADDRESS_IDX].strptr));
        RC.eNB[j][0]->eth_params_n.my_portc                 = *(L1_ParamList.paramarray[j][L1_LOCAL_N_PORTC_IDX].iptr);
        RC.eNB[j][0]->eth_params_n.remote_portc             = *(L1_ParamList.paramarray[j][L1_REMOTE_N_PORTC_IDX].iptr);
        RC.eNB[j][0]->eth_params_n.my_portd                 = *(L1_ParamList.paramarray[j][L1_LOCAL_N_PORTD_IDX].iptr);
        RC.eNB[j][0]->eth_params_n.remote_portd             = *(L1_ParamList.paramarray[j][L1_REMOTE_N_PORTD_IDX].iptr);
        RC.eNB[j][0]->eth_params_n.transp_preference        = ETH_UDP_MODE;
        RC.nb_macrlc_inst = 1;  // This is used by mac_top_init_eNB()
        // This is used by init_eNB_afterRU()
        RC.nb_CC = (int *)malloc((1+RC.nb_inst)*sizeof(int));
        RC.nb_CC[0]=1;
        RC.nb_inst =1; // DJP - feptx_prec uses num_eNB but phy_init_RU uses nb_inst
        LOG_I(PHY,"%s() NFAPI PNF mode - RC.nb_inst=1 this is because phy_init_RU() uses that to index and not RC.num_eNB - why the 2 similar variables?\n", __FUNCTION__);
        LOG_I(PHY,"%s() NFAPI PNF mode - RC.nb_CC[0]=%d for init_eNB_afterRU()\n", __FUNCTION__, RC.nb_CC[0]);
        LOG_I(PHY,"%s() NFAPI PNF mode - RC.nb_macrlc_inst:%d because used by mac_top_init_eNB()\n", __FUNCTION__, RC.nb_macrlc_inst);
        mac_top_init_eNB();
        configure_nfapi_pnf(RC.eNB[j][0]->eth_params_n.remote_addr, RC.eNB[j][0]->eth_params_n.remote_portc, RC.eNB[j][0]->eth_params_n.my_addr, RC.eNB[j][0]->eth_params_n.my_portd,
                            RC.eNB[j][0]->eth_params_n     .remote_portd);
      } else { // other midhaul
      }

      // PRACH/PUCCH parameters
      RC.eNB[j][0]->prach_DTX_threshold    = *(L1_ParamList.paramarray[j][L1_PRACH_DTX_THRESHOLD_IDX].iptr);
      RC.eNB[j][0]->pucch1_DTX_threshold   = *(L1_ParamList.paramarray[j][L1_PUCCH1_DTX_THRESHOLD_IDX].iptr);
      RC.eNB[j][0]->pucch1ab_DTX_threshold = *(L1_ParamList.paramarray[j][L1_PUCCH1AB_DTX_THRESHOLD_IDX].iptr);

      for (int ce_level=0; ce_level<4; ce_level++) {
        RC.eNB[j][0]->prach_DTX_threshold_emtc[ce_level]    = *(L1_ParamList.paramarray[j][L1_PRACH_DTX_EMTC0_THRESHOLD_IDX+ce_level].iptr);
        RC.eNB[j][0]->pucch1_DTX_threshold_emtc[ce_level]   = *(L1_ParamList.paramarray[j][L1_PUCCH1_DTX_EMTC0_THRESHOLD_IDX+ce_level].iptr);
        RC.eNB[j][0]->pucch1ab_DTX_threshold_emtc[ce_level] = *(L1_ParamList.paramarray[j][L1_PUCCH1AB_DTX_EMTC0_THRESHOLD_IDX+ce_level].iptr);
      }


      RC.eNB[j][0]->pusch_signal_threshold    = *(L1_ParamList.paramarray[j][L1_PUSCH_SIGNAL_THRESHOLD_IDX].iptr);
      LOG_I(ENB_APP,"PUSCH singal threshold = %d \n",RC.eNB[j][0]->pusch_signal_threshold);
    }// j=0..num_inst

    LOG_I(ENB_APP,"Initializing northbound interface for L1\n");
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
      memset(RC.eNB[j],0,(1+MAX_NUM_CCs)*sizeof(PHY_VARS_eNB***));
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

void RCconfig_macrlc(int macrlc_has_f1[MAX_MAC_INST]) {
  int               j;
  paramdef_t MacRLC_Params[] = MACRLCPARAMS_DESC;
  paramlist_def_t MacRLC_ParamList = {CONFIG_STRING_MACRLC_LIST,NULL,0};
  config_getlist( &MacRLC_ParamList,MacRLC_Params,sizeof(MacRLC_Params)/sizeof(paramdef_t), NULL);
  config_getlist( &MacRLC_ParamList,MacRLC_Params,sizeof(MacRLC_Params)/sizeof(paramdef_t), NULL);

  if ( MacRLC_ParamList.numelt > 0) {
    RC.nb_macrlc_inst=MacRLC_ParamList.numelt;
    mac_top_init_eNB();
    RC.nb_mac_CC = (int *)malloc(RC.nb_macrlc_inst*sizeof(int));

    for (j = 0; j < RC.nb_macrlc_inst; j++) {
      RC.mac[j]->puSch10xSnr = *(MacRLC_ParamList.paramarray[j][MACRLC_PUSCH10xSNR_IDX ].iptr);
      RC.mac[j]->puCch10xSnr = *(MacRLC_ParamList.paramarray[j][MACRLC_PUCCH10xSNR_IDX ].iptr);
      RC.mac[j]->ue_multiple_max = *(MacRLC_ParamList.paramarray[j][MACRLC_UE_MULTIPLE_MAX_IDX ].iptr);
      RC.mac[j]->use_mcs_offset = *(MacRLC_ParamList.paramarray[j][MACRLC_USE_MCS_OFFSET_IDX ].iptr);
      RC.mac[j]->bler_lower = *(MacRLC_ParamList.paramarray[j][MACRLC_BLER_TARGET_LOWER_IDX ].dblptr);
      RC.mac[j]->bler_upper = *(MacRLC_ParamList.paramarray[j][MACRLC_BLER_TARGET_UPPER_IDX ].dblptr);
      RC.mac[j]->max_ul_rb_index = *(MacRLC_ParamList.paramarray[j][MACRLC_MAX_UL_RB_INDEX_IDX ].iptr);
      RC.nb_mac_CC[j] = *(MacRLC_ParamList.paramarray[j][MACRLC_CC_IDX].iptr);
      LOG_I(ENB_APP,"MAC instance %d parameters : pusch_snr %lf, pucch_snr %lf, ue_multiple_max %d, use_mcs_offset %d, bler_lower %lf, bler_upper %lf,max_ul_rb_index %d\n",
	j,
	RC.mac[j]->puSch10xSnr/10.0,
	RC.mac[j]->puCch10xSnr/10.0,
	RC.mac[j]->ue_multiple_max,
	RC.mac[j]->use_mcs_offset,
	RC.mac[j]->bler_lower,
	RC.mac[j]->bler_upper,
	RC.mac[j]->max_ul_rb_index);
      if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_N_PREFERENCE_IDX].strptr), "local_RRC") == 0) {
        // check number of instances is same as RRC/PDCP
        LOG_I(ENB_APP,"Configuring local RRC for MACRLC\n");
      } else if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_N_PREFERENCE_IDX].strptr), "f1") == 0) {
        LOG_I(ENB_APP,"Configuring F1 interfaces for MACRLC\n");
        RC.mac[j]->eth_params_n.local_if_name            = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_IF_NAME_IDX].strptr));
        RC.mac[j]->eth_params_n.my_addr                  = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_ADDRESS_IDX].strptr));
        RC.mac[j]->eth_params_n.remote_addr              = strdup(*(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_ADDRESS_IDX].strptr));
        RC.mac[j]->eth_params_n.my_portc                 = *(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_PORTC_IDX].iptr);
        RC.mac[j]->eth_params_n.remote_portc             = *(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_PORTC_IDX].iptr);
        RC.mac[j]->eth_params_n.my_portd                 = *(MacRLC_ParamList.paramarray[j][MACRLC_LOCAL_N_PORTD_IDX].iptr);
        RC.mac[j]->eth_params_n.remote_portd             = *(MacRLC_ParamList.paramarray[j][MACRLC_REMOTE_N_PORTD_IDX].iptr);;
        RC.mac[j]->eth_params_n.transp_preference        = ETH_UDP_MODE;
        macrlc_has_f1[j]                                 = 1;
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
        LOG_I(ENB_APP,"**************** vnf_port:%d\n", RC.mac[j]->eth_params_s.my_portc);
        configure_nfapi_vnf(RC.mac[j]->eth_params_s.my_addr, RC.mac[j]->eth_params_s.my_portc);
        LOG_I(ENB_APP,"**************** RETURNED FROM configure_nfapi_vnf() vnf_port:%d\n", RC.mac[j]->eth_params_s.my_portc);
      } else { // other midhaul
        AssertFatal(1==0,"MACRLC %d: %s unknown southbound midhaul\n",j,*(MacRLC_ParamList.paramarray[j][MACRLC_TRANSPORT_S_PREFERENCE_IDX].strptr));
      }

      if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_SCHED_MODE_IDX].strptr), "default") == 0) {
        global_scheduler_mode=SCHED_MODE_DEFAULT;
        LOG_I(ENB_APP,"sched mode = default %d [%s]\n",global_scheduler_mode,*(MacRLC_ParamList.paramarray[j][MACRLC_SCHED_MODE_IDX].strptr));
      } else if (strcmp(*(MacRLC_ParamList.paramarray[j][MACRLC_SCHED_MODE_IDX].strptr), "fairRR") == 0) {
        global_scheduler_mode=SCHED_MODE_FAIR_RR;
        printf("sched mode = fairRR %d [%s]\n",global_scheduler_mode,*(MacRLC_ParamList.paramarray[j][MACRLC_SCHED_MODE_IDX].strptr));
      } else {
        global_scheduler_mode=SCHED_MODE_DEFAULT;
        printf("sched mode = default %d [%s]\n",global_scheduler_mode,*(MacRLC_ParamList.paramarray[j][MACRLC_SCHED_MODE_IDX].strptr));
      }

      char *s = *MacRLC_ParamList.paramarray[j][MACRLC_DEFAULT_SCHED_DL_ALGO_IDX].strptr;
      void *d = dlsym(NULL, s);
      AssertFatal(d, "%s(): no default scheduler DL algo '%s' found\n", __func__, s);
      // release default, add new
      pp_impl_param_t *dl_pp = &RC.mac[j]->pre_processor_dl;
      dl_pp->dl_algo.unset(&dl_pp->dl_algo.data);
      dl_pp->dl_algo = *(default_sched_dl_algo_t *) d;
      dl_pp->dl_algo.data = dl_pp->dl_algo.setup();
      LOG_I(ENB_APP, "using default scheduler DL algo '%s'\n", dl_pp->dl_algo.name);
    }// j=0..num_inst
  } /*else {// MacRLC_ParamList.numelt > 0 // ignore it

    AssertFatal (0,
                 "No " CONFIG_STRING_MACRLC_LIST " configuration found");
  }*/
}

int RCconfig_RRC(uint32_t i, eNB_RRC_INST *rrc, int macrlc_has_f1) {
  int               num_enbs                      = 0;
  int               j,k                           = 0;
  int32_t           enb_id                        = 0;
  int               nb_cc                         = 0;
  int32_t           offsetMaxLimit                = 0;
  int32_t           cycleNb                       = 0;
   
  MessageDef *msg_p = itti_alloc_new_message(TASK_RRC_ENB, 0, RRC_CONFIGURATION_REQ);
  ccparams_lte_t ccparams_lte;
  ccparams_sidelink_t SLconfig;
  ccparams_eMTC_t eMTCconfig;
  memset((void *)&ccparams_lte,0,sizeof(ccparams_lte_t));
  memset((void *)&SLconfig,0,sizeof(ccparams_sidelink_t));
  memset((void *)&eMTCconfig,0,sizeof(ccparams_eMTC_t));
  paramdef_t ENBSParams[] = ENBSPARAMS_DESC;
  paramdef_t ENBParams[]  = ENBPARAMS_DESC;
  paramlist_def_t ENBParamList = {ENB_CONFIG_STRING_ENB_LIST,NULL,0};
  checkedparam_t config_check_CCparams[] = CCPARAMS_CHECK;
  paramdef_t CCsParams[] = CCPARAMS_DESC(ccparams_lte);
  paramlist_def_t CCsParamList = {ENB_CONFIG_STRING_COMPONENT_CARRIERS,NULL,0};
  paramdef_t eMTCParams[]              = EMTCPARAMS_DESC((&eMTCconfig));
  checkedparam_t config_check_eMTCparams[] = EMTCPARAMS_CHECK;
  srb1_params_t srb1_params;
  memset((void *)&srb1_params,0,sizeof(srb1_params_t));
  paramdef_t SRB1Params[] = SRB1PARAMS_DESC(srb1_params);
  paramdef_t SLParams[]              = CCPARAMS_SIDELINK_DESC(SLconfig);

  /* map parameter checking array instances to parameter definition array instances */
  for (int I=0; I< ( sizeof(CCsParams)/ sizeof(paramdef_t)  ) ; I++) {
    CCsParams[I].chkPptr = &(config_check_CCparams[I]);
  }

  for (int I = 0; I < (sizeof(CCsParams) / sizeof(paramdef_t)); I++) {
    eMTCParams[I].chkPptr = &(config_check_eMTCparams[I]);
  }

  /* get global parameters, defined outside any section in the config file */
  config_get( ENBSParams,sizeof(ENBSParams)/sizeof(paramdef_t),NULL);
  num_enbs = ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt;
  AssertFatal (i<num_enbs,
               "Failed to parse config file no %uth element in %s \n",i, ENB_CONFIG_STRING_ACTIVE_ENBS);

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

    LOG_I(RRC,"Instance %d: Southbound Transport %s\n",i,*(ENBParamList.paramarray[i][ENB_TRANSPORT_S_PREFERENCE_IDX].strptr));

    if (strcmp(*(ENBParamList.paramarray[i][ENB_TRANSPORT_S_PREFERENCE_IDX].strptr), "f1") == 0) {
      paramdef_t SCTPParams[]  = SCTPPARAMS_DESC;
      char aprefix[MAX_OPTNAME_SIZE*2 + 8];
      sprintf(aprefix,"%s.[%u].%s",ENB_CONFIG_STRING_ENB_LIST,i,ENB_CONFIG_STRING_SCTP_CONFIG);
      config_get( SCTPParams,sizeof(SCTPParams)/sizeof(paramdef_t),aprefix);
      rrc->node_id        = *(ENBParamList.paramarray[0][ENB_ENB_ID_IDX].uptr);
      LOG_I(ENB_APP,"F1AP: gNB_CU_id[%d] %d\n",k,rrc->node_id);
      rrc->node_name = strdup(*(ENBParamList.paramarray[0][ENB_ENB_NAME_IDX].strptr));
      LOG_I(ENB_APP,"F1AP: gNB_CU_name[%d] %s\n",k,rrc->node_name);
      rrc->eth_params_s.local_if_name            = strdup(*(ENBParamList.paramarray[i][ENB_LOCAL_S_IF_NAME_IDX].strptr));
      rrc->eth_params_s.my_addr                  = strdup(*(ENBParamList.paramarray[i][ENB_LOCAL_S_ADDRESS_IDX].strptr));
      rrc->eth_params_s.remote_addr              = strdup(*(ENBParamList.paramarray[i][ENB_REMOTE_S_ADDRESS_IDX].strptr));
      rrc->eth_params_s.my_portc                 = *(ENBParamList.paramarray[i][ENB_LOCAL_S_PORTC_IDX].uptr);
      rrc->eth_params_s.remote_portc             = *(ENBParamList.paramarray[i][ENB_REMOTE_S_PORTC_IDX].uptr);
      rrc->eth_params_s.my_portd                 = *(ENBParamList.paramarray[i][ENB_LOCAL_S_PORTD_IDX].uptr);
      rrc->eth_params_s.remote_portd             = *(ENBParamList.paramarray[i][ENB_REMOTE_S_PORTD_IDX].uptr);
      rrc->eth_params_s.transp_preference        = ETH_UDP_MODE;
      rrc->node_type                             = ngran_eNB_CU;
      rrc->sctp_in_streams                       = (uint16_t)*(SCTPParams[ENB_SCTP_INSTREAMS_IDX].uptr);
      rrc->sctp_out_streams                      = (uint16_t)*(SCTPParams[ENB_SCTP_OUTSTREAMS_IDX].uptr);
    } else {
      // set to ngran_eNB for now, it will get set to ngran_eNB_DU if macrlc entity which uses F1 is present
      // Note: we will have to handle the case of ngran_ng_eNB_DU
      if (macrlc_has_f1 == 0) {
        rrc->node_type = ngran_eNB;
        LOG_I(RRC,"Setting node_type to ngran_eNB\n");
      } else {
        rrc->node_type = ngran_eNB_DU;
        LOG_I(RRC,"Setting node_type to ngran_eNB_DU\n");
      }
    }

    rrc->nr_cellid        = (uint64_t)*(ENBParamList.paramarray[i][ENB_NRCELLID_IDX].u64ptr);

    // search if in active list

    for (k=0; k <num_enbs ; k++) {
      if (strcmp(ENBSParams[ENB_ACTIVE_ENBS_IDX].strlistptr[k], *(ENBParamList.paramarray[i][ENB_ENB_NAME_IDX].strptr)) == 0) {
        char enbpath[MAX_OPTNAME_SIZE + 8];
        sprintf(enbpath,"%s.[%i]",ENB_CONFIG_STRING_ENB_LIST,k);
        paramdef_t PLMNParams[] = PLMNPARAMS_DESC;
        paramlist_def_t PLMNParamList = {ENB_CONFIG_STRING_PLMN_LIST, NULL, 0};
        /* map parameter checking array instances to parameter definition array instances */
        checkedparam_t config_check_PLMNParams [] = PLMNPARAMS_CHECK;

        for (int I = 0; I < sizeof(PLMNParams) / sizeof(paramdef_t); ++I)
          PLMNParams[I].chkPptr = &(config_check_PLMNParams[I]);

        // In the configuration file it is in seconds. For RRC it has to be in milliseconds
        RRC_CONFIGURATION_REQ (msg_p).rrc_inactivity_timer_thres = (*ENBParamList.paramarray[i][ENB_RRC_INACTIVITY_THRES_IDX].uptr) * 1000;
        RRC_CONFIGURATION_REQ (msg_p).cell_identity = enb_id;
        RRC_CONFIGURATION_REQ (msg_p).tac = *ENBParamList.paramarray[i][ENB_TRACKING_AREA_CODE_IDX].uptr;
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

        /* measurement reports enabled? */
        if (ENBParamList.paramarray[i][ENB_ENABLE_MEASUREMENT_REPORTS].strptr != NULL &&
            *(ENBParamList.paramarray[i][ENB_ENABLE_MEASUREMENT_REPORTS].strptr) != NULL &&
            !strcmp(*(ENBParamList.paramarray[i][ENB_ENABLE_MEASUREMENT_REPORTS].strptr), "yes"))
          RRC_CONFIGURATION_REQ (msg_p).enable_measurement_reports = 1;
        else
          RRC_CONFIGURATION_REQ (msg_p).enable_measurement_reports = 0;

        /* x2 enabled? */
        if (ENBParamList.paramarray[i][ENB_ENABLE_X2].strptr != NULL &&
            *(ENBParamList.paramarray[i][ENB_ENABLE_X2].strptr) != NULL &&
            !strcmp(*(ENBParamList.paramarray[i][ENB_ENABLE_X2].strptr), "yes"))
          RRC_CONFIGURATION_REQ (msg_p).enable_x2 = 1;
        else
          RRC_CONFIGURATION_REQ (msg_p).enable_x2 = 0;

       /* m2 enabled */
       if (ENBParamList.paramarray[i][ENB_ENABLE_ENB_M2].strptr != NULL &&
            *(ENBParamList.paramarray[i][ENB_ENABLE_ENB_M2].strptr) != NULL &&
            !strcmp(*(ENBParamList.paramarray[i][ENB_ENABLE_ENB_M2].strptr), "yes"))
          RRC_CONFIGURATION_REQ (msg_p).eMBMS_M2_configured = 1;
        else
          RRC_CONFIGURATION_REQ (msg_p).eMBMS_M2_configured = 0;

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
            // Cell params, MIB/SIB1 in DU
            RRC_CONFIGURATION_REQ (msg_p).tdd_config[j] = ccparams_lte.tdd_config;
            AssertFatal (ccparams_lte.tdd_config <= LTE_TDD_Config__subframeAssignment_sa6,
                         "Failed to parse eNB configuration file %s, enb %u illegal tdd_config %d (should be 0-%d)!",
                         RC.config_file_name, i, ccparams_lte.tdd_config, LTE_TDD_Config__subframeAssignment_sa6);
            RRC_CONFIGURATION_REQ (msg_p).tdd_config_s[j] = ccparams_lte.tdd_config_s;
            AssertFatal (ccparams_lte.tdd_config_s <= LTE_TDD_Config__specialSubframePatterns_ssp8,
                         "Failed to parse eNB configuration file %s, enb %u illegal tdd_config_s %d (should be 0-%d)!",
                         RC.config_file_name, i, ccparams_lte.tdd_config_s, LTE_TDD_Config__specialSubframePatterns_ssp8);

            if (!ccparams_lte.prefix_type)
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %u define %s: NORMAL,EXTENDED!\n",
                           RC.config_file_name, i, ENB_CONFIG_STRING_PREFIX_TYPE);
            else if (strcmp(ccparams_lte.prefix_type, "NORMAL") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).prefix_type[j] = NORMAL;
            } else  if (strcmp(ccparams_lte.prefix_type, "EXTENDED") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).prefix_type[j] = EXTENDED;
            } else {
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for prefix_type choice: NORMAL or EXTENDED !\n",
                           RC.config_file_name, i, ccparams_lte.prefix_type);
            }

            if (!ccparams_lte.pbch_repetition)
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %u define %s: TRUE,FALSE!\n",
                           RC.config_file_name, i, ENB_CONFIG_STRING_PBCH_REPETITION);
            else if (strcmp(ccparams_lte.pbch_repetition, "TRUE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).pbch_repetition[j] = 1;
            } else  if (strcmp(ccparams_lte.pbch_repetition, "FALSE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).pbch_repetition[j] = 0;
            } else {
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for pbch_repetition choice: TRUE or FALSE !\n",
                           RC.config_file_name, i, ccparams_lte.pbch_repetition);
            }

            RRC_CONFIGURATION_REQ (msg_p).eutra_band[j] = ccparams_lte.eutra_band;
            RRC_CONFIGURATION_REQ (msg_p).downlink_frequency[j] = (uint32_t) ccparams_lte.downlink_frequency;
            RRC_CONFIGURATION_REQ (msg_p).uplink_frequency_offset[j] = (unsigned int) ccparams_lte.uplink_frequency_offset;
            RRC_CONFIGURATION_REQ (msg_p).Nid_cell[j]= ccparams_lte.Nid_cell;

            if (ccparams_lte.Nid_cell>503) {
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for Nid_cell choice: 0...503 !\n",
                           RC.config_file_name, i, ccparams_lte.Nid_cell);
            }

            RRC_CONFIGURATION_REQ (msg_p).N_RB_DL[j]= ccparams_lte.N_RB_DL;

            if ((ccparams_lte.N_RB_DL!=6) &&
                (ccparams_lte.N_RB_DL!=15) &&
                (ccparams_lte.N_RB_DL!=25) &&
                (ccparams_lte.N_RB_DL!=50) &&
                (ccparams_lte.N_RB_DL!=75) &&
                (ccparams_lte.N_RB_DL!=100)) {
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for N_RB_DL choice: 6,15,25,50,75,100 !\n",
                           RC.config_file_name, i, ccparams_lte.N_RB_DL);
            }

            if (strcmp(ccparams_lte.frame_type, "FDD") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).frame_type[j] = FDD;
            } else  if (strcmp(ccparams_lte.frame_type, "TDD") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).frame_type[j] = TDD;
            } else {
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for frame_type choice: FDD or TDD !\n",
                           RC.config_file_name, i, ccparams_lte.frame_type);
            }

            if (config_check_band_frequencies(j,
                                              RRC_CONFIGURATION_REQ (msg_p).eutra_band[j],
                                              RRC_CONFIGURATION_REQ (msg_p).downlink_frequency[j],
                                              RRC_CONFIGURATION_REQ (msg_p).uplink_frequency_offset[j],
                                              RRC_CONFIGURATION_REQ (msg_p).frame_type[j])) {
              AssertFatal(0, "error calling enb_check_band_frequencies\n");
            }

            if ((ccparams_lte.nb_antenna_ports <1) || (ccparams_lte.nb_antenna_ports > 2))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for nb_antenna_ports choice: 1..2 !\n",
                           RC.config_file_name, i, ccparams_lte.nb_antenna_ports);

            RRC_CONFIGURATION_REQ (msg_p).nb_antenna_ports[j] = ccparams_lte.nb_antenna_ports;

            if (!NODE_IS_DU(rrc->node_type)) { //this is CU or eNB, SIB2-20 in CU
              // Radio Resource Configuration (SIB2)
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_root =  ccparams_lte.prach_root;

              if ((ccparams_lte.prach_root <0) || (ccparams_lte.prach_root > 1023))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for prach_root choice: 0..1023 !\n",
                             RC.config_file_name, i, ccparams_lte.prach_root);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_config_index = ccparams_lte.prach_config_index;

              if ((ccparams_lte.prach_config_index <0) || (ccparams_lte.prach_config_index > 63))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for prach_config_index choice: 0..1023 !\n",
                             RC.config_file_name, i, ccparams_lte.prach_config_index);

              if (!ccparams_lte.prach_high_speed)
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u define %s: ENABLE,DISABLE!\n",
                             RC.config_file_name, i, ENB_CONFIG_STRING_PRACH_HIGH_SPEED);
              else if (strcmp(ccparams_lte.prach_high_speed, "ENABLE") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_high_speed = TRUE;
              } else if (strcmp(ccparams_lte.prach_high_speed, "DISABLE") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_high_speed = FALSE;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for prach_config choice: ENABLE,DISABLE !\n",
                             RC.config_file_name, i, ccparams_lte.prach_high_speed);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_zero_correlation = ccparams_lte.prach_zero_correlation;

              if ((ccparams_lte.prach_zero_correlation <0) ||
                  (ccparams_lte.prach_zero_correlation > 15))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for prach_zero_correlation choice: 0..15!\n",
                             RC.config_file_name, i, ccparams_lte.prach_zero_correlation);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_freq_offset = ccparams_lte.prach_freq_offset;

              if ((ccparams_lte.prach_freq_offset <0) ||
                  (ccparams_lte.prach_freq_offset > 94))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for prach_freq_offset choice: 0..94!\n",
                             RC.config_file_name, i, ccparams_lte.prach_freq_offset);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_delta_shift = ccparams_lte.pucch_delta_shift-1;

              if ((ccparams_lte.pucch_delta_shift <1) ||
                  (ccparams_lte.pucch_delta_shift > 3))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for pucch_delta_shift choice: 1..3!\n",
                             RC.config_file_name, i, ccparams_lte.pucch_delta_shift);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_nRB_CQI = ccparams_lte.pucch_nRB_CQI;

              if ((ccparams_lte.pucch_nRB_CQI <0) ||
                  (ccparams_lte.pucch_nRB_CQI > 98))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for pucch_nRB_CQI choice: 0..98!\n",
                             RC.config_file_name, i, ccparams_lte.pucch_nRB_CQI);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_nCS_AN = ccparams_lte.pucch_nCS_AN;

              if ((ccparams_lte.pucch_nCS_AN <0) ||
                  (ccparams_lte.pucch_nCS_AN > 7))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for pucch_nCS_AN choice: 0..7!\n",
                             RC.config_file_name, i, ccparams_lte.pucch_nCS_AN);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_n1_AN = ccparams_lte.pucch_n1_AN;

              if ((ccparams_lte.pucch_n1_AN <0) ||
                  (ccparams_lte.pucch_n1_AN > 2047))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for pucch_n1_AN choice: 0..2047!\n",
                             RC.config_file_name, i, ccparams_lte.pucch_n1_AN);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pdsch_referenceSignalPower = ccparams_lte.pdsch_referenceSignalPower;

              if ((ccparams_lte.pdsch_referenceSignalPower <-60) ||
                  (ccparams_lte.pdsch_referenceSignalPower > 50))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for pdsch_referenceSignalPower choice:-60..50!\n",
                             RC.config_file_name, i, ccparams_lte.pdsch_referenceSignalPower);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pdsch_p_b = ccparams_lte.pdsch_p_b;

              if ((ccparams_lte.pdsch_p_b <0) ||
                  (ccparams_lte.pdsch_p_b > 3))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for pdsch_p_b choice: 0..3!\n",
                             RC.config_file_name, i, ccparams_lte.pdsch_p_b);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_n_SB = ccparams_lte.pusch_n_SB;

              if ((ccparams_lte.pusch_n_SB <1) ||
                  (ccparams_lte.pusch_n_SB > 4))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for pusch_n_SB choice: 1..4!\n",
                             RC.config_file_name, i, ccparams_lte.pusch_n_SB);

              if (!ccparams_lte.pusch_hoppingMode)
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u define %s: interSubframe,intraAndInterSubframe!\n",
                             RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_HOPPINGMODE);
              else if (strcmp(ccparams_lte.pusch_hoppingMode,"interSubFrame")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_hoppingMode = LTE_PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_interSubFrame;
              } else if (strcmp(ccparams_lte.pusch_hoppingMode,"intraAndInterSubFrame")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_hoppingMode = LTE_PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_intraAndInterSubFrame;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for pusch_hoppingMode choice: interSubframe,intraAndInterSubframe!\n",
                             RC.config_file_name, i, ccparams_lte.pusch_hoppingMode);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_hoppingOffset = ccparams_lte.pusch_hoppingOffset;

              if ((ccparams_lte.pusch_hoppingOffset<0) ||
                  (ccparams_lte.pusch_hoppingOffset>98))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for pusch_hoppingOffset choice: 0..98!\n",
                             RC.config_file_name, i, ccparams_lte.pusch_hoppingMode);

              if (!ccparams_lte.pusch_enable64QAM)
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u define %s: ENABLE,DISABLE!\n",
                             RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_ENABLE64QAM);
              else if (strcmp(ccparams_lte.pusch_enable64QAM, "ENABLE") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_enable64QAM = TRUE;
              } else if (strcmp(ccparams_lte.pusch_enable64QAM, "DISABLE") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_enable64QAM = FALSE;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for pusch_enable64QAM choice: ENABLE,DISABLE!\n",
                             RC.config_file_name, i, ccparams_lte.pusch_enable64QAM);

              if (!ccparams_lte.pusch_groupHoppingEnabled)
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u define %s: ENABLE,DISABLE!\n",
                             RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_GROUP_HOPPING_EN);
              else if (strcmp(ccparams_lte.pusch_groupHoppingEnabled, "ENABLE") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_groupHoppingEnabled = TRUE;
              } else if (strcmp(ccparams_lte.pusch_groupHoppingEnabled, "DISABLE") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_groupHoppingEnabled= FALSE;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for pusch_groupHoppingEnabled choice: ENABLE,DISABLE!\n",
                             RC.config_file_name, i, ccparams_lte.pusch_groupHoppingEnabled);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_groupAssignment = ccparams_lte.pusch_groupAssignment;

              if ((ccparams_lte.pusch_groupAssignment<0)||
                  (ccparams_lte.pusch_groupAssignment>29))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for pusch_groupAssignment choice: 0..29!\n",
                             RC.config_file_name, i, ccparams_lte.pusch_groupAssignment);

              if (!ccparams_lte.pusch_sequenceHoppingEnabled)
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u define %s: ENABLE,DISABLE!\n",
                             RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_SEQUENCE_HOPPING_EN);
              else if (strcmp(ccparams_lte.pusch_sequenceHoppingEnabled, "ENABLE") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_sequenceHoppingEnabled = TRUE;
              } else if (strcmp(ccparams_lte.pusch_sequenceHoppingEnabled, "DISABLE") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_sequenceHoppingEnabled = FALSE;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for pusch_sequenceHoppingEnabled choice: ENABLE,DISABLE!\n",
                             RC.config_file_name, i, ccparams_lte.pusch_sequenceHoppingEnabled);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_nDMRS1= ccparams_lte.pusch_nDMRS1;  //cyclic_shift in RRC!

              if ((ccparams_lte.pusch_nDMRS1 <0) ||
                  (ccparams_lte.pusch_nDMRS1>7))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for pusch_nDMRS1 choice: 0..7!\n",
                             RC.config_file_name, i, ccparams_lte.pusch_nDMRS1);

              if (strcmp(ccparams_lte.phich_duration,"NORMAL")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_duration= LTE_PHICH_Config__phich_Duration_normal;
              } else if (strcmp(ccparams_lte.phich_duration,"EXTENDED")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_duration= LTE_PHICH_Config__phich_Duration_extended;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for phich_duration choice: NORMAL,EXTENDED!\n",
                             RC.config_file_name, i, ccparams_lte.phich_duration);

              if (strcmp(ccparams_lte.phich_resource,"ONESIXTH")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_resource= LTE_PHICH_Config__phich_Resource_oneSixth ;
              } else if (strcmp(ccparams_lte.phich_resource,"HALF")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_resource= LTE_PHICH_Config__phich_Resource_half;
              } else if (strcmp(ccparams_lte.phich_resource,"ONE")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_resource= LTE_PHICH_Config__phich_Resource_one;
              } else if (strcmp(ccparams_lte.phich_resource,"TWO")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_resource= LTE_PHICH_Config__phich_Resource_two;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for phich_resource choice: ONESIXTH,HALF,ONE,TWO!\n",
                             RC.config_file_name, i, ccparams_lte.phich_resource);

              printf("phich.resource %ld (%s), phich.duration %ld (%s)\n",
                     RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_resource,ccparams_lte.phich_resource,
                     RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_duration,ccparams_lte.phich_duration);

              if (strcmp(ccparams_lte.srs_enable, "ENABLE") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_enable= TRUE;
              } else if (strcmp(ccparams_lte.srs_enable, "DISABLE") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_enable= FALSE;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for srs_BandwidthConfig choice: ENABLE,DISABLE !\n",
                             RC.config_file_name, i, ccparams_lte.srs_enable);

              if (RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_enable== TRUE) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_BandwidthConfig= ccparams_lte.srs_BandwidthConfig;

                if ((ccparams_lte.srs_BandwidthConfig < 0) ||
                    (ccparams_lte.srs_BandwidthConfig >7))
                  AssertFatal (0, "Failed to parse eNB configuration file %s, enb %u unknown value %d for srs_BandwidthConfig choice: 0...7\n",
                               RC.config_file_name, i, ccparams_lte.srs_BandwidthConfig);

                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_SubframeConfig= ccparams_lte.srs_SubframeConfig;

                if ((ccparams_lte.srs_SubframeConfig<0) ||
                    (ccparams_lte.srs_SubframeConfig>15))
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for srs_SubframeConfig choice: 0..15 !\n",
                               RC.config_file_name, i, ccparams_lte.srs_SubframeConfig);

                if (strcmp(ccparams_lte.srs_ackNackST, "ENABLE") == 0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_ackNackST= TRUE;
                } else if (strcmp(ccparams_lte.srs_ackNackST, "DISABLE") == 0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_ackNackST= FALSE;
                } else
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for srs_BandwidthConfig choice: ENABLE,DISABLE !\n",
                               RC.config_file_name, i, ccparams_lte.srs_ackNackST);

                if (strcmp(ccparams_lte.srs_MaxUpPts, "ENABLE") == 0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_MaxUpPts= TRUE;
                } else if (strcmp(ccparams_lte.srs_MaxUpPts, "DISABLE") == 0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_MaxUpPts= FALSE;
                } else
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for srs_MaxUpPts choice: ENABLE,DISABLE !\n",
                               RC.config_file_name, i, ccparams_lte.srs_MaxUpPts);
              }

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_p0_Nominal= ccparams_lte.pusch_p0_Nominal;

              if ((ccparams_lte.pusch_p0_Nominal<-126) ||
                  (ccparams_lte.pusch_p0_Nominal>24))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for pusch_p0_Nominal choice: -126..24 !\n",
                             RC.config_file_name, i, ccparams_lte.pusch_p0_Nominal);

              if (strcmp(ccparams_lte.pusch_alpha,"AL0")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_Alpha_r12_al0;
              } else if (strcmp(ccparams_lte.pusch_alpha,"AL04")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_Alpha_r12_al04;
              } else if (strcmp(ccparams_lte.pusch_alpha,"AL05")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_Alpha_r12_al05;
              } else if (strcmp(ccparams_lte.pusch_alpha,"AL06")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_Alpha_r12_al06;
              } else if (strcmp(ccparams_lte.pusch_alpha,"AL07")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_Alpha_r12_al07;
              } else if (strcmp(ccparams_lte.pusch_alpha,"AL08")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_Alpha_r12_al08;
              } else if (strcmp(ccparams_lte.pusch_alpha,"AL09")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_Alpha_r12_al09;
              } else if (strcmp(ccparams_lte.pusch_alpha,"AL1")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_Alpha_r12_al1;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for pucch_Alpha choice: AL0,AL04,AL05,AL06,AL07,AL08,AL09,AL1!\n",
                             RC.config_file_name, i, ccparams_lte.pusch_alpha);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_p0_Nominal= ccparams_lte.pucch_p0_Nominal;

              if ((ccparams_lte.pucch_p0_Nominal<-127) ||
                  (ccparams_lte.pucch_p0_Nominal>-96))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for pucch_p0_Nominal choice: -127..-96 !\n",
                             RC.config_file_name, i, ccparams_lte.pucch_p0_Nominal);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].msg3_delta_Preamble= ccparams_lte.msg3_delta_Preamble;

              if ((ccparams_lte.msg3_delta_Preamble<-1) ||
                  (ccparams_lte.msg3_delta_Preamble>6))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for msg3_delta_Preamble choice: -1..6 !\n",
                             RC.config_file_name, i, ccparams_lte.msg3_delta_Preamble);

              if (strcmp(ccparams_lte.pucch_deltaF_Format1,"deltaF_2")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format1= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF_2;
              } else if (strcmp(ccparams_lte.pucch_deltaF_Format1,"deltaF0")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format1= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF0;
              } else if (strcmp(ccparams_lte.pucch_deltaF_Format1,"deltaF2")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format1= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF2;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for pucch_deltaF_Format1 choice: deltaF_2,dltaF0,deltaF2!\n",
                             RC.config_file_name, i, ccparams_lte.pucch_deltaF_Format1);

              if (strcmp(ccparams_lte.pucch_deltaF_Format1b,"deltaF1")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format1b= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF1;
              } else if (strcmp(ccparams_lte.pucch_deltaF_Format1b,"deltaF3")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format1b= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF3;
              } else if (strcmp(ccparams_lte.pucch_deltaF_Format1b,"deltaF5")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format1b= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF5;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for pucch_deltaF_Format1b choice: deltaF1,dltaF3,deltaF5!\n",
                             RC.config_file_name, i, ccparams_lte.pucch_deltaF_Format1b);

              if (strcmp(ccparams_lte.pucch_deltaF_Format2,"deltaF_2")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF_2;
              } else if (strcmp(ccparams_lte.pucch_deltaF_Format2,"deltaF0")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF0;
              } else if (strcmp(ccparams_lte.pucch_deltaF_Format2,"deltaF1")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF1;
              } else if (strcmp(ccparams_lte.pucch_deltaF_Format2,"deltaF2")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF2;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for pucch_deltaF_Format2 choice: deltaF_2,dltaF0,deltaF1,deltaF2!\n",
                             RC.config_file_name, i, ccparams_lte.pucch_deltaF_Format2);

              if (strcmp(ccparams_lte.pucch_deltaF_Format2a,"deltaF_2")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2a= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF_2;
              } else if (strcmp(ccparams_lte.pucch_deltaF_Format2a,"deltaF0")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2a= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF0;
              } else if (strcmp(ccparams_lte.pucch_deltaF_Format2a,"deltaF2")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2a= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF2;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for pucch_deltaF_Format2a choice: deltaF_2,dltaF0,deltaF2!\n",
                             RC.config_file_name, i, ccparams_lte.pucch_deltaF_Format2a);

              if (strcmp(ccparams_lte.pucch_deltaF_Format2b,"deltaF_2")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2b= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF_2;
              } else if (strcmp(ccparams_lte.pucch_deltaF_Format2b,"deltaF0")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2b= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF0;
              } else if (strcmp(ccparams_lte.pucch_deltaF_Format2b,"deltaF2")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2b= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF2;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for pucch_deltaF_Format2b choice: deltaF_2,dltaF0,deltaF2!\n",
                             RC.config_file_name, i, ccparams_lte.pucch_deltaF_Format2b);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_numberOfRA_Preambles= (ccparams_lte.rach_numberOfRA_Preambles/4)-1;

              if ((ccparams_lte.rach_numberOfRA_Preambles <4) ||
                  (ccparams_lte.rach_numberOfRA_Preambles>64) ||
                  ((ccparams_lte.rach_numberOfRA_Preambles&3)!=0))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for rach_numberOfRA_Preambles choice: 4,8,12,...,64!\n",
                             RC.config_file_name, i, ccparams_lte.rach_numberOfRA_Preambles);

              if (strcmp(ccparams_lte.rach_preamblesGroupAConfig, "ENABLE") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preamblesGroupAConfig= TRUE;
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_sizeOfRA_PreamblesGroupA= (ccparams_lte.rach_sizeOfRA_PreamblesGroupA/4)-1;

                if ((ccparams_lte.rach_numberOfRA_Preambles <4) ||
                    (ccparams_lte.rach_numberOfRA_Preambles>60) ||
                    ((ccparams_lte.rach_numberOfRA_Preambles&3)!=0))
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for rach_sizeOfRA_PreamblesGroupA choice: 4,8,12,...,60!\n",
                               RC.config_file_name, i, ccparams_lte.rach_sizeOfRA_PreamblesGroupA);

                switch (ccparams_lte.rach_messageSizeGroupA) {
                  case 56:
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messageSizeGroupA= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b56;
                    break;

                  case 144:
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messageSizeGroupA= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b144;
                    break;

                  case 208:
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messageSizeGroupA= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b208;
                    break;

                  case 256:
                    RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messageSizeGroupA= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messageSizeGroupA_b256;
                    break;

                  default:
                    AssertFatal (0,
                                 "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for rach_messageSizeGroupA choice: 56,144,208,256!\n",
                                 RC.config_file_name, i, ccparams_lte.rach_messageSizeGroupA);
                    break;
                }

                if (strcmp(ccparams_lte.rach_messagePowerOffsetGroupB,"minusinfinity")==0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_minusinfinity;
                } else if (strcmp(ccparams_lte.rach_messagePowerOffsetGroupB,"dB0")==0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB0;
                } else if (strcmp(ccparams_lte.rach_messagePowerOffsetGroupB,"dB5")==0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB5;
                } else if (strcmp(ccparams_lte.rach_messagePowerOffsetGroupB,"dB8")==0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB8;
                } else if (strcmp(ccparams_lte.rach_messagePowerOffsetGroupB,"dB10")==0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB10;
                } else if (strcmp(ccparams_lte.rach_messagePowerOffsetGroupB,"dB12")==0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB12;
                } else if (strcmp(ccparams_lte.rach_messagePowerOffsetGroupB,"dB15")==0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB15;
                } else if (strcmp(ccparams_lte.rach_messagePowerOffsetGroupB,"dB18")==0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB18;
                } else
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for rach_messagePowerOffsetGroupB choice: minusinfinity,dB0,dB5,dB8,dB10,dB12,dB15,dB18!\n",
                               RC.config_file_name, i, ccparams_lte.rach_messagePowerOffsetGroupB);
              } else if (strcmp(ccparams_lte.rach_preamblesGroupAConfig, "DISABLE") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preamblesGroupAConfig= FALSE;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for rach_preamblesGroupAConfig choice: ENABLE,DISABLE !\n",
                             RC.config_file_name, i, ccparams_lte.rach_preamblesGroupAConfig);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleInitialReceivedTargetPower= (ccparams_lte.rach_preambleInitialReceivedTargetPower+120)/2;

              if ((ccparams_lte.rach_preambleInitialReceivedTargetPower<-120) ||
                  (ccparams_lte.rach_preambleInitialReceivedTargetPower>-90) ||
                  ((ccparams_lte.rach_preambleInitialReceivedTargetPower&1)!=0))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for rach_preambleInitialReceivedTargetPower choice: -120,-118,...,-90 !\n",
                             RC.config_file_name, i, ccparams_lte.rach_preambleInitialReceivedTargetPower);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_powerRampingStep= ccparams_lte.rach_powerRampingStep/2;

              if ((ccparams_lte.rach_powerRampingStep<0) ||
                  (ccparams_lte.rach_powerRampingStep>6) ||
                  ((ccparams_lte.rach_powerRampingStep&1)!=0))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for rach_powerRampingStep choice: 0,2,4,6 !\n",
                             RC.config_file_name, i, ccparams_lte.rach_powerRampingStep);

              switch (ccparams_lte.rach_preambleTransMax) {
                case 3:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax= LTE_PreambleTransMax_n3;
                  break;

                case 4:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax= LTE_PreambleTransMax_n4;
                  break;

                case 5:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax= LTE_PreambleTransMax_n5;
                  break;

                case 6:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax= LTE_PreambleTransMax_n6;
                  break;

                case 7:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax= LTE_PreambleTransMax_n7;
                  break;

                case 8:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax= LTE_PreambleTransMax_n8;
                  break;

                case 10:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax= LTE_PreambleTransMax_n10;
                  break;

                case 20:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax= LTE_PreambleTransMax_n20;
                  break;

                case 50:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax= LTE_PreambleTransMax_n50;
                  break;

                case 100:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax= LTE_PreambleTransMax_n100;
                  break;

                case 200:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax= LTE_PreambleTransMax_n200;
                  break;

                default:
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for rach_preambleTransMax choice: 3,4,5,6,7,8,10,20,50,100,200!\n",
                               RC.config_file_name, i, ccparams_lte.rach_preambleTransMax);
                  break;
              }

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_raResponseWindowSize=  (ccparams_lte.rach_raResponseWindowSize==10)?7:ccparams_lte.rach_raResponseWindowSize-2;

              if ((ccparams_lte.rach_raResponseWindowSize<0)||
                  (ccparams_lte.rach_raResponseWindowSize==9)||
                  (ccparams_lte.rach_raResponseWindowSize>10))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for rach_raResponseWindowSize choice: 2,3,4,5,6,7,8,10!\n",
                             RC.config_file_name, i, ccparams_lte.rach_preambleTransMax);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_macContentionResolutionTimer= (ccparams_lte.rach_macContentionResolutionTimer/8)-1;

              if ((ccparams_lte.rach_macContentionResolutionTimer<8) ||
                  (ccparams_lte.rach_macContentionResolutionTimer>64) ||
                  ((ccparams_lte.rach_macContentionResolutionTimer&7)!=0))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for rach_macContentionResolutionTimer choice: 8,16,...,56,64!\n",
                             RC.config_file_name, i, ccparams_lte.rach_preambleTransMax);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_maxHARQ_Msg3Tx= ccparams_lte.rach_maxHARQ_Msg3Tx;

              if ((ccparams_lte.rach_maxHARQ_Msg3Tx<0) ||
                  (ccparams_lte.rach_maxHARQ_Msg3Tx>8))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for rach_maxHARQ_Msg3Tx choice: 1..8!\n",
                             RC.config_file_name, i, ccparams_lte.rach_preambleTransMax);

              switch (ccparams_lte.pcch_defaultPagingCycle) {
                case 32:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_defaultPagingCycle= LTE_PCCH_Config__defaultPagingCycle_rf32;
                  break;

                case 64:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_defaultPagingCycle= LTE_PCCH_Config__defaultPagingCycle_rf64;
                  break;

                case 128:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_defaultPagingCycle= LTE_PCCH_Config__defaultPagingCycle_rf128;
                  break;

                case 256:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_defaultPagingCycle= LTE_PCCH_Config__defaultPagingCycle_rf256;
                  break;

                default:
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for pcch_defaultPagingCycle choice: 32,64,128,256!\n",
                               RC.config_file_name, i, ccparams_lte.pcch_defaultPagingCycle);
                  break;
              }

              if (strcmp(ccparams_lte.pcch_nB, "fourT") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= LTE_PCCH_Config__nB_fourT;
              } else if (strcmp(ccparams_lte.pcch_nB, "twoT") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= LTE_PCCH_Config__nB_twoT;
              } else if (strcmp(ccparams_lte.pcch_nB, "oneT") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= LTE_PCCH_Config__nB_oneT;
              } else if (strcmp(ccparams_lte.pcch_nB, "halfT") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= LTE_PCCH_Config__nB_halfT;
              } else if (strcmp(ccparams_lte.pcch_nB, "quarterT") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= LTE_PCCH_Config__nB_quarterT;
              } else if (strcmp(ccparams_lte.pcch_nB, "oneEighthT") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= LTE_PCCH_Config__nB_oneEighthT;
              } else if (strcmp(ccparams_lte.pcch_nB, "oneSixteenthT") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= LTE_PCCH_Config__nB_oneSixteenthT;
              } else if (strcmp(ccparams_lte.pcch_nB, "oneThirtySecondT") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= LTE_PCCH_Config__nB_oneThirtySecondT;
              } else {
                AssertFatal (0, "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for pcch_nB choice: fourT,twoT,oneT,halfT,quarterT,oneighthT,oneSixteenthT,oneThirtySecondT !\n",
                             RC.config_file_name,
                             i,
                             ccparams_lte.pcch_nB);
              }

              if (strcmp(ccparams_lte.drx_Config_present, "prNothing") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_Config_present = LTE_DRX_Config_PR_NOTHING;
              } else if (strcmp(ccparams_lte.drx_Config_present, "prRelease") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_Config_present = LTE_DRX_Config_PR_release;
              } else if (strcmp(ccparams_lte.drx_Config_present, "prSetup") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_Config_present = LTE_DRX_Config_PR_setup;
              } else {
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for drx_Config_present choice: prNothing, prRelease, prSetup!\n",
                             RC.config_file_name, i, ccparams_lte.drx_Config_present);
              }

              if (strcmp(ccparams_lte.drx_onDurationTimer, "psf1") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_onDurationTimer = (long) LTE_DRX_Config__setup__onDurationTimer_psf1;
              } else if (strcmp(ccparams_lte.drx_onDurationTimer, "psf2") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_onDurationTimer = (long) LTE_DRX_Config__setup__onDurationTimer_psf2;
              } else if (strcmp(ccparams_lte.drx_onDurationTimer, "psf3") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_onDurationTimer = (long) LTE_DRX_Config__setup__onDurationTimer_psf3;
              } else if (strcmp(ccparams_lte.drx_onDurationTimer, "psf4") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_onDurationTimer = (long) LTE_DRX_Config__setup__onDurationTimer_psf4;
              } else if (strcmp(ccparams_lte.drx_onDurationTimer, "psf5") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_onDurationTimer = (long) LTE_DRX_Config__setup__onDurationTimer_psf5;
              } else if (strcmp(ccparams_lte.drx_onDurationTimer, "psf6") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_onDurationTimer = (long) LTE_DRX_Config__setup__onDurationTimer_psf6;
              } else if (strcmp(ccparams_lte.drx_onDurationTimer, "psf8") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_onDurationTimer = (long) LTE_DRX_Config__setup__onDurationTimer_psf8;
              } else if (strcmp(ccparams_lte.drx_onDurationTimer, "psf10") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_onDurationTimer = (long) LTE_DRX_Config__setup__onDurationTimer_psf10;
              } else if (strcmp(ccparams_lte.drx_onDurationTimer, "psf20") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_onDurationTimer = (long) LTE_DRX_Config__setup__onDurationTimer_psf20;
              } else if (strcmp(ccparams_lte.drx_onDurationTimer, "psf30") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_onDurationTimer = (long) LTE_DRX_Config__setup__onDurationTimer_psf30;
              } else if (strcmp(ccparams_lte.drx_onDurationTimer, "psf40") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_onDurationTimer = (long) LTE_DRX_Config__setup__onDurationTimer_psf40;
              } else if (strcmp(ccparams_lte.drx_onDurationTimer, "psf50") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_onDurationTimer = (long) LTE_DRX_Config__setup__onDurationTimer_psf50;
              } else if (strcmp(ccparams_lte.drx_onDurationTimer, "psf60") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_onDurationTimer = (long) LTE_DRX_Config__setup__onDurationTimer_psf60;
              } else if (strcmp(ccparams_lte.drx_onDurationTimer, "psf80") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_onDurationTimer = (long) LTE_DRX_Config__setup__onDurationTimer_psf80;
              } else if (strcmp(ccparams_lte.drx_onDurationTimer, "psf100") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_onDurationTimer = (long) LTE_DRX_Config__setup__onDurationTimer_psf100;
              } else if (strcmp(ccparams_lte.drx_onDurationTimer, "psf200") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_onDurationTimer = (long) LTE_DRX_Config__setup__onDurationTimer_psf200;
              } else {
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for drx_onDurationTimer choice !\n",
                             RC.config_file_name, i, ccparams_lte.drx_onDurationTimer);
                break;
              }

              if (strcmp(ccparams_lte.drx_InactivityTimer, "psf1") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_psf1;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "psf2") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_psf2;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "psf3") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_psf3;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "psf4") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_psf4;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "psf5") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_psf5;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "psf6") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_psf6;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "psf8") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_psf8;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "psf10") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_psf10;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "psf20") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_psf20;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "psf30") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_psf30;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "psf40") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_psf40;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "psf50") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_psf50;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "psf60") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_psf60;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "psf80") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_psf80;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "psf100") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_psf100;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "psf200") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_psf200;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "psf300") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_psf300;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "psf500") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_psf500;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "psf750") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_psf750;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "psf1280") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_psf1280;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "psf1920") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_psf1920;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "psf2560") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_psf2560;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "psf0-v1020") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_psf0_v1020;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "spare9") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_spare9;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "spare8") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_spare8;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "spare7") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_spare7;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "spare6") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_spare6;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "spare5") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_spare5;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "spare4") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_spare4;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "spare3") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_spare3;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "spare2") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_spare2;
              } else if (strcmp(ccparams_lte.drx_InactivityTimer, "spare1") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_InactivityTimer = (long) LTE_DRX_Config__setup__drx_InactivityTimer_spare1;
              } else {
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for drx_InactivityTimer choice !\n",
                             RC.config_file_name, i, ccparams_lte.drx_InactivityTimer);
                break;
              }

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_multiple_max= ccparams_lte.ue_multiple_max;

              if (!ccparams_lte.mbms_dedicated_serving_cell)
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u define %s: TRUE,FALSE!\n",
                             RC.config_file_name, i, ENB_CONFIG_STRING_MBMS_DEDICATED_SERVING_CELL);
              else if (strcmp(ccparams_lte.mbms_dedicated_serving_cell, "ENABLE") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].mbms_dedicated_serving_cell = TRUE;
              } else  if (strcmp(ccparams_lte.mbms_dedicated_serving_cell, "DISABLE") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].mbms_dedicated_serving_cell  = FALSE;
              } else {
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for mbms_dedicated_serving_cell choice: TRUE or FALSE !\n",
                             RC.config_file_name, i, ccparams_lte.mbms_dedicated_serving_cell);
              }

              switch (ccparams_lte.N_RB_DL) {
                case 25:
                  if ((ccparams_lte.ue_multiple_max < 1) ||
                      (ccparams_lte.ue_multiple_max > 4))
                    AssertFatal (0,
                                 "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for ue_multiple_max choice: 1..4!\n",
                                 RC.config_file_name, i, ccparams_lte.ue_multiple_max);

                  break;

                case 50:
                  if ((ccparams_lte.ue_multiple_max < 1) ||
                      (ccparams_lte.ue_multiple_max > 8))
                    AssertFatal (0,
                                 "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for ue_multiple_max choice: 1..8!\n",
                                 RC.config_file_name, i, ccparams_lte.ue_multiple_max);

                  break;

                case 100:
                  if ((ccparams_lte.ue_multiple_max < 1) ||
                      (ccparams_lte.ue_multiple_max > 16))
                    AssertFatal (0,
                                 "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for ue_multiple_max choice: 1..16!\n",
                                 RC.config_file_name, i, ccparams_lte.ue_multiple_max);

                  break;

                default:
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for N_RB_DL choice: 25,50,100 !\n",
                               RC.config_file_name, i, ccparams_lte.N_RB_DL);
                  break;
              }

              if (strcmp(ccparams_lte.drx_RetransmissionTimer, "psf1") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_RetransmissionTimer = (long) LTE_DRX_Config__setup__drx_RetransmissionTimer_psf1;
              } else if (strcmp(ccparams_lte.drx_RetransmissionTimer, "psf2") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_RetransmissionTimer = (long) LTE_DRX_Config__setup__drx_RetransmissionTimer_psf2;
              } else if (strcmp(ccparams_lte.drx_RetransmissionTimer, "psf4") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_RetransmissionTimer = (long) LTE_DRX_Config__setup__drx_RetransmissionTimer_psf4;
              } else if (strcmp(ccparams_lte.drx_RetransmissionTimer, "psf6") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_RetransmissionTimer = (long) LTE_DRX_Config__setup__drx_RetransmissionTimer_psf6;
              } else if (strcmp(ccparams_lte.drx_RetransmissionTimer, "psf8") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_RetransmissionTimer = (long) LTE_DRX_Config__setup__drx_RetransmissionTimer_psf8;
              } else if (strcmp(ccparams_lte.drx_RetransmissionTimer, "psf16") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_RetransmissionTimer = (long) LTE_DRX_Config__setup__drx_RetransmissionTimer_psf16;
              } else if (strcmp(ccparams_lte.drx_RetransmissionTimer, "psf24") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_RetransmissionTimer = (long) LTE_DRX_Config__setup__drx_RetransmissionTimer_psf24;
              } else if (strcmp(ccparams_lte.drx_RetransmissionTimer, "psf33") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_RetransmissionTimer = (long) LTE_DRX_Config__setup__drx_RetransmissionTimer_psf33;
              } else {
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for drx_RetransmissionTimer choice !\n",
                             RC.config_file_name, i, ccparams_lte.drx_RetransmissionTimer);
                break;
              }

              if (ccparams_lte.drx_longDrx_CycleStartOffset_present == NULL || strcmp(ccparams_lte.drx_longDrx_CycleStartOffset_present, "prNothing") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_longDrx_CycleStartOffset_present = LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_NOTHING;
              } else {
                if (strcmp(ccparams_lte.drx_longDrx_CycleStartOffset_present, "prSf10") == 0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_longDrx_CycleStartOffset_present = LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf10;
                  offsetMaxLimit = 10;
                } else if (strcmp(ccparams_lte.drx_longDrx_CycleStartOffset_present, "prSf20") == 0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_longDrx_CycleStartOffset_present = LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf20;
                  offsetMaxLimit = 20;
                } else if (strcmp(ccparams_lte.drx_longDrx_CycleStartOffset_present, "prSf32") == 0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_longDrx_CycleStartOffset_present = LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf32;
                  offsetMaxLimit = 32;
                } else if (strcmp(ccparams_lte.drx_longDrx_CycleStartOffset_present, "prSf40") == 0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_longDrx_CycleStartOffset_present = LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf40;
                  offsetMaxLimit = 40;
                } else if (strcmp(ccparams_lte.drx_longDrx_CycleStartOffset_present, "prSf64") == 0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_longDrx_CycleStartOffset_present = LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf64;
                  offsetMaxLimit = 64;
                } else if (strcmp(ccparams_lte.drx_longDrx_CycleStartOffset_present, "prSf80") == 0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_longDrx_CycleStartOffset_present = LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf80;
                  offsetMaxLimit = 80;
                } else if (strcmp(ccparams_lte.drx_longDrx_CycleStartOffset_present, "prSf128") == 0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_longDrx_CycleStartOffset_present = LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf128;
                  offsetMaxLimit = 128;
                } else if (strcmp(ccparams_lte.drx_longDrx_CycleStartOffset_present, "prSf160") == 0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_longDrx_CycleStartOffset_present = LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf160;
                  offsetMaxLimit = 160;
                } else if (strcmp(ccparams_lte.drx_longDrx_CycleStartOffset_present, "prSf256") == 0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_longDrx_CycleStartOffset_present = LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf256;
                  offsetMaxLimit = 256;
                } else if (strcmp(ccparams_lte.drx_longDrx_CycleStartOffset_present, "prSf320") == 0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_longDrx_CycleStartOffset_present = LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf320;
                  offsetMaxLimit = 320;
                } else if (strcmp(ccparams_lte.drx_longDrx_CycleStartOffset_present, "prSf512") == 0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_longDrx_CycleStartOffset_present = LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf512;
                  offsetMaxLimit = 512;
                } else if (strcmp(ccparams_lte.drx_longDrx_CycleStartOffset_present, "prSf640") == 0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_longDrx_CycleStartOffset_present = LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf640;
                  offsetMaxLimit = 640;
                } else if (strcmp(ccparams_lte.drx_longDrx_CycleStartOffset_present, "prSf1024") == 0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_longDrx_CycleStartOffset_present = LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf1024;
                  offsetMaxLimit = 1024;
                } else if (strcmp(ccparams_lte.drx_longDrx_CycleStartOffset_present, "prSf1280") == 0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_longDrx_CycleStartOffset_present = LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf1280;
                  offsetMaxLimit = 1280;
                } else if (strcmp(ccparams_lte.drx_longDrx_CycleStartOffset_present, "prSf2048") == 0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_longDrx_CycleStartOffset_present = LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf2048;
                  offsetMaxLimit = 2048;
                } else if (strcmp(ccparams_lte.drx_longDrx_CycleStartOffset_present, "prSf2560") == 0) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_longDrx_CycleStartOffset_present = LTE_DRX_Config__setup__longDRX_CycleStartOffset_PR_sf2560;
                  offsetMaxLimit = 2560;
                } else {
                  AssertFatal (0,
                               "Failed to parse eNB configuration file \"%s\", enb %u unknown string value \"%s\" for drx_longDrx_CycleStartOffset_present choice !\n",
                               RC.config_file_name, i, ccparams_lte.drx_longDrx_CycleStartOffset_present);
                }

                if (ccparams_lte.drx_longDrx_CycleStartOffset >= 0 && ccparams_lte.drx_longDrx_CycleStartOffset < offsetMaxLimit) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_longDrx_CycleStartOffset = ccparams_lte.drx_longDrx_CycleStartOffset;
                } else {
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %u incoherent value \"%d\" for drx_longDrx_CycleStartOffset !\n",
                               RC.config_file_name, i, ccparams_lte.drx_longDrx_CycleStartOffset);
                }
              }

              if  (strcmp(ccparams_lte.drx_shortDrx_Cycle, "") == 0 || ccparams_lte.drx_shortDrx_ShortCycleTimer == 0) {
                if  (strcmp(ccparams_lte.drx_shortDrx_Cycle, "") != 0 || ccparams_lte.drx_shortDrx_ShortCycleTimer != 0) {
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %u incoherent values \"%s\" -  \"%d\" for drx_shortDrx_Cycle or drx_shortDrx_ShortCycleTimer choice !\n",
                               RC.config_file_name, i, ccparams_lte.drx_shortDrx_Cycle, ccparams_lte.drx_shortDrx_ShortCycleTimer);
                } else {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_shortDrx_Cycle = -1;
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_shortDrx_ShortCycleTimer = 0;
                }
              } else {
                if (strcmp(ccparams_lte.drx_shortDrx_Cycle, "sf2") == 0) {
                  cycleNb = 2;
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_shortDrx_Cycle = LTE_DRX_Config__setup__shortDRX__shortDRX_Cycle_sf2;
                } else if (strcmp(ccparams_lte.drx_shortDrx_Cycle, "sf5") == 0) {
                  cycleNb = 5;
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_shortDrx_Cycle = LTE_DRX_Config__setup__shortDRX__shortDRX_Cycle_sf5;
                } else if (strcmp(ccparams_lte.drx_shortDrx_Cycle, "sf8") == 0) {
                  cycleNb = 8;
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_shortDrx_Cycle = LTE_DRX_Config__setup__shortDRX__shortDRX_Cycle_sf8;
                } else if (strcmp(ccparams_lte.drx_shortDrx_Cycle, "sf10") == 0) {
                  cycleNb = 10;
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_shortDrx_Cycle = LTE_DRX_Config__setup__shortDRX__shortDRX_Cycle_sf10;
                } else if (strcmp(ccparams_lte.drx_shortDrx_Cycle, "sf16") == 0) {
                  cycleNb = 16;
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_shortDrx_Cycle = LTE_DRX_Config__setup__shortDRX__shortDRX_Cycle_sf16;
                } else if (strcmp(ccparams_lte.drx_shortDrx_Cycle, "sf20") == 0) {
                  cycleNb = 20;
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_shortDrx_Cycle = LTE_DRX_Config__setup__shortDRX__shortDRX_Cycle_sf20;
                } else if (strcmp(ccparams_lte.drx_shortDrx_Cycle, "sf32") == 0) {
                  cycleNb = 32;
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_shortDrx_Cycle = LTE_DRX_Config__setup__shortDRX__shortDRX_Cycle_sf32;
                } else if (strcmp(ccparams_lte.drx_shortDrx_Cycle, "sf40") == 0) {
                  cycleNb = 40;
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_shortDrx_Cycle = LTE_DRX_Config__setup__shortDRX__shortDRX_Cycle_sf40;
                } else if (strcmp(ccparams_lte.drx_shortDrx_Cycle, "sf64") == 0) {
                  cycleNb = 64;
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_shortDrx_Cycle = LTE_DRX_Config__setup__shortDRX__shortDRX_Cycle_sf64;
                } else if (strcmp(ccparams_lte.drx_shortDrx_Cycle, "sf80") == 0) {
                  cycleNb = 80;
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_shortDrx_Cycle = LTE_DRX_Config__setup__shortDRX__shortDRX_Cycle_sf80;
                } else if (strcmp(ccparams_lte.drx_shortDrx_Cycle, "sf128") == 0) {
                  cycleNb = 128;
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_shortDrx_Cycle = LTE_DRX_Config__setup__shortDRX__shortDRX_Cycle_sf128;
                } else if (strcmp(ccparams_lte.drx_shortDrx_Cycle, "sf160") == 0) {
                  cycleNb = 160;
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_shortDrx_Cycle = LTE_DRX_Config__setup__shortDRX__shortDRX_Cycle_sf160;
                } else if (strcmp(ccparams_lte.drx_shortDrx_Cycle, "sf256") == 0) {
                  cycleNb = 256;
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_shortDrx_Cycle = LTE_DRX_Config__setup__shortDRX__shortDRX_Cycle_sf256;
                } else if (strcmp(ccparams_lte.drx_shortDrx_Cycle, "sf320") == 0) {
                  cycleNb = 320;
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_shortDrx_Cycle = LTE_DRX_Config__setup__shortDRX__shortDRX_Cycle_sf320;
                } else if (strcmp(ccparams_lte.drx_shortDrx_Cycle, "sf512") == 0) {
                  cycleNb = 512;
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_shortDrx_Cycle = LTE_DRX_Config__setup__shortDRX__shortDRX_Cycle_sf512;
                } else if (strcmp(ccparams_lte.drx_shortDrx_Cycle, "sf640") == 0) {
                  cycleNb = 640;
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_shortDrx_Cycle = LTE_DRX_Config__setup__shortDRX__shortDRX_Cycle_sf640;
                } else {
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %u incoherent value \"%s\" for drx_shortDrx_Cycle !\n",
                               RC.config_file_name, i, ccparams_lte.drx_shortDrx_Cycle);
                }

                if (cycleNb > 0 && (offsetMaxLimit % cycleNb != 0 || cycleNb == offsetMaxLimit)) {
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %u incompatible (not multiple) values \"%d\" -  \"%d\" for drx_shortDrx_Cycle and drx_longDrx_CycleStartOffset choice !\n",
                               RC.config_file_name, i, cycleNb, offsetMaxLimit);
                }

                if (ccparams_lte.drx_shortDrx_ShortCycleTimer >= 1 && ccparams_lte.drx_shortDrx_ShortCycleTimer <= 16 ) {
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].drx_shortDrx_ShortCycleTimer = ccparams_lte.drx_shortDrx_ShortCycleTimer;
                } else {
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for drx_shortDrx_ShortCycleTimer choice !\n",
                               RC.config_file_name, i, ccparams_lte.drx_shortDrx_ShortCycleTimer );
                }
              }

              switch (ccparams_lte.bcch_modificationPeriodCoeff) {
                case 2:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].bcch_modificationPeriodCoeff= LTE_BCCH_Config__modificationPeriodCoeff_n2;
                  break;

                case 4:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].bcch_modificationPeriodCoeff= LTE_BCCH_Config__modificationPeriodCoeff_n4;
                  break;

                case 8:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].bcch_modificationPeriodCoeff= LTE_BCCH_Config__modificationPeriodCoeff_n8;
                  break;

                case 16:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].bcch_modificationPeriodCoeff= LTE_BCCH_Config__modificationPeriodCoeff_n16;
                  break;

                default:
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for bcch_modificationPeriodCoeff choice: 2,4,8,16",
                               RC.config_file_name, i, ccparams_lte.bcch_modificationPeriodCoeff);
                  break;
              }

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t300= ccparams_lte.ue_TimersAndConstants_t300;
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t301= ccparams_lte.ue_TimersAndConstants_t301;
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t310= ccparams_lte.ue_TimersAndConstants_t310;
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_t311= ccparams_lte.ue_TimersAndConstants_t311;
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n310= ccparams_lte.ue_TimersAndConstants_n310;
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TimersAndConstants_n311= ccparams_lte.ue_TimersAndConstants_n311;

              switch (ccparams_lte.ue_TransmissionMode) {
                case 1:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode= LTE_AntennaInfoDedicated__transmissionMode_tm1;
                  break;

                case 2:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode= LTE_AntennaInfoDedicated__transmissionMode_tm2;
                  break;

                case 3:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode= LTE_AntennaInfoDedicated__transmissionMode_tm3;
                  break;

                case 4:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode= LTE_AntennaInfoDedicated__transmissionMode_tm4;
                  break;

                case 5:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode= LTE_AntennaInfoDedicated__transmissionMode_tm5;
                  break;

                case 6:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode= LTE_AntennaInfoDedicated__transmissionMode_tm6;
                  break;

                case 7:
                  RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_TransmissionMode= LTE_AntennaInfoDedicated__transmissionMode_tm7;
                  break;

                default:
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for ue_TransmissionMode choice: 1,2,3,4,5,6,7",
                               RC.config_file_name, i, ccparams_lte.ue_TransmissionMode);
                  break;
              }

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_multiple_max= ccparams_lte.ue_multiple_max;

              switch (ccparams_lte.N_RB_DL) {
                case 25:
                  if ((ccparams_lte.ue_multiple_max < 1) ||
                      (ccparams_lte.ue_multiple_max > 4))
                    AssertFatal (0,
                                 "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for ue_multiple_max choice: 1..4!\n",
                                 RC.config_file_name, i, ccparams_lte.ue_multiple_max);

                  break;

                case 50:
                  if ((ccparams_lte.ue_multiple_max < 1) ||
                      (ccparams_lte.ue_multiple_max > 8))
                    AssertFatal (0,
                                 "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for ue_multiple_max choice: 1..8!\n",
                                 RC.config_file_name, i, ccparams_lte.ue_multiple_max);

                  break;

                case 100:
                  if ((ccparams_lte.ue_multiple_max < 1) ||
                      (ccparams_lte.ue_multiple_max > 16))
                    AssertFatal (0,
                                 "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for ue_multiple_max choice: 1..16!\n",
                                 RC.config_file_name, i, ccparams_lte.ue_multiple_max);

                  break;

                default:
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %u unknown value \"%d\" for N_RB_DL choice: 25,50,100 !\n",
                               RC.config_file_name, i, ccparams_lte.N_RB_DL);
                  break;
              }

              // eMBMS configuration
              RRC_CONFIGURATION_REQ(msg_p).eMBMS_configured = 0;
              printf("No eMBMS configuration, skipping it\n");
              // eMTC configuration
              char brparamspath[MAX_OPTNAME_SIZE*2 + 160];
              sprintf(brparamspath,"%s.%s", ccspath, ENB_CONFIG_STRING_EMTC_PARAMETERS);
              config_get(eMTCParams, sizeof(eMTCParams)/sizeof(paramdef_t), brparamspath);
              RRC_CONFIGURATION_REQ(msg_p).eMTC_configured = eMTCconfig.eMTC_configured&1;

              if (eMTCconfig.eMTC_configured > 0) fill_eMTC_configuration(msg_p,&eMTCconfig, i,j,RC.config_file_name,brparamspath);
              else                            printf("No eMTC configuration, skipping it\n");

              // Sidelink configuration
              char SLparamspath[MAX_OPTNAME_SIZE*2 + 160];
              sprintf(SLparamspath,"%s.%s", ccspath, ENB_CONFIG_STRING_SL_PARAMETERS);
              config_get( SLParams, sizeof(SLParams)/sizeof(paramdef_t), SLparamspath);
              // Sidelink Resource pool information
              RRC_CONFIGURATION_REQ (msg_p).SL_configured=SLconfig.sidelink_configured&1;

              if (SLconfig.sidelink_configured==1) fill_SL_configuration(msg_p,&SLconfig,i,j,RC.config_file_name);
              else                                 printf("No SL configuration skipping it\n");
            } // !NODE_IS_DU(node_type)
          }

          RRC_CONFIGURATION_REQ (msg_p).nr_scg_ssb_freq = ccparams_lte.nr_scg_ssb_freq;

          if (!NODE_IS_DU(rrc->node_type)) {
            char srb1path[MAX_OPTNAME_SIZE*2 + 8];
            sprintf(srb1path,"%s.%s",enbpath,ENB_CONFIG_STRING_SRB1);
            config_get( SRB1Params,sizeof(SRB1Params)/sizeof(paramdef_t), srb1path);

            switch (srb1_params.srb1_max_retx_threshold) {
              case 1:
                rrc->srb1_max_retx_threshold = LTE_UL_AM_RLC__maxRetxThreshold_t1;
                break;

              case 2:
                rrc->srb1_max_retx_threshold = LTE_UL_AM_RLC__maxRetxThreshold_t2;
                break;

              case 3:
                rrc->srb1_max_retx_threshold = LTE_UL_AM_RLC__maxRetxThreshold_t3;
                break;

              case 4:
                rrc->srb1_max_retx_threshold = LTE_UL_AM_RLC__maxRetxThreshold_t4;
                break;

              case 6:
                rrc->srb1_max_retx_threshold = LTE_UL_AM_RLC__maxRetxThreshold_t6;
                break;

              case 8:
                rrc->srb1_max_retx_threshold = LTE_UL_AM_RLC__maxRetxThreshold_t8;
                break;

              case 16:
                rrc->srb1_max_retx_threshold = LTE_UL_AM_RLC__maxRetxThreshold_t16;
                break;

              case 32:
                rrc->srb1_max_retx_threshold = LTE_UL_AM_RLC__maxRetxThreshold_t32;
                break;

              default:
                AssertFatal (0,
                             "Bad config value when parsing eNB configuration file %s, enb %u  srb1_max_retx_threshold %d!\n",
                             RC.config_file_name, i, srb1_params.srb1_max_retx_threshold);
            }

            switch (srb1_params.srb1_poll_pdu) {
              case 4:
                rrc->srb1_poll_pdu = LTE_PollPDU_p4;
                break;

              case 8:
                rrc->srb1_poll_pdu = LTE_PollPDU_p8;
                break;

              case 16:
                rrc->srb1_poll_pdu = LTE_PollPDU_p16;
                break;

              case 32:
                rrc->srb1_poll_pdu = LTE_PollPDU_p32;
                break;

              case 64:
                rrc->srb1_poll_pdu = LTE_PollPDU_p64;
                break;

              case 128:
                rrc->srb1_poll_pdu = LTE_PollPDU_p128;
                break;

              case 256:
                rrc->srb1_poll_pdu = LTE_PollPDU_p256;
                break;

              default:
                if (srb1_params.srb1_poll_pdu >= 10000)
                  rrc->srb1_poll_pdu = LTE_PollPDU_pInfinity;
                else
                  AssertFatal (0,
                               "Bad config value when parsing eNB configuration file %s, enb %u  srb1_poll_pdu %d!\n",
                               RC.config_file_name, i, srb1_params.srb1_poll_pdu);
            }

            rrc->srb1_poll_byte             = srb1_params.srb1_poll_byte;

            switch (srb1_params.srb1_poll_byte) {
              case 25:
                rrc->srb1_poll_byte = LTE_PollByte_kB25;
                break;

              case 50:
                rrc->srb1_poll_byte = LTE_PollByte_kB50;
                break;

              case 75:
                rrc->srb1_poll_byte = LTE_PollByte_kB75;
                break;

              case 100:
                rrc->srb1_poll_byte = LTE_PollByte_kB100;
                break;

              case 125:
                rrc->srb1_poll_byte = LTE_PollByte_kB125;
                break;

              case 250:
                rrc->srb1_poll_byte = LTE_PollByte_kB250;
                break;

              case 375:
                rrc->srb1_poll_byte = LTE_PollByte_kB375;
                break;

              case 500:
                rrc->srb1_poll_byte = LTE_PollByte_kB500;
                break;

              case 750:
                rrc->srb1_poll_byte = LTE_PollByte_kB750;
                break;

              case 1000:
                rrc->srb1_poll_byte = LTE_PollByte_kB1000;
                break;

              case 1250:
                rrc->srb1_poll_byte = LTE_PollByte_kB1250;
                break;

              case 1500:
                rrc->srb1_poll_byte = LTE_PollByte_kB1500;
                break;

              case 2000:
                rrc->srb1_poll_byte = LTE_PollByte_kB2000;
                break;

              case 3000:
                rrc->srb1_poll_byte = LTE_PollByte_kB3000;
                break;

              default:
                if (srb1_params.srb1_poll_byte >= 10000)
                  rrc->srb1_poll_byte = LTE_PollByte_kBinfinity;
                else
                  AssertFatal (0,
                               "Bad config value when parsing eNB configuration file %s, enb %u  srb1_poll_byte %d!\n",
                               RC.config_file_name, i, srb1_params.srb1_poll_byte);
            }

            if (srb1_params.srb1_timer_poll_retransmit <= 250) {
              rrc->srb1_timer_poll_retransmit = (srb1_params.srb1_timer_poll_retransmit - 5)/5;
            } else if (srb1_params.srb1_timer_poll_retransmit <= 500) {
              rrc->srb1_timer_poll_retransmit = (srb1_params.srb1_timer_poll_retransmit - 300)/50 + 50;
            } else {
              AssertFatal (0,
                           "Bad config value when parsing eNB configuration file %s, enb %u  srb1_timer_poll_retransmit %d!\n",
                           RC.config_file_name, i, srb1_params.srb1_timer_poll_retransmit);
            }

            if (srb1_params.srb1_timer_status_prohibit <= 250) {
              rrc->srb1_timer_status_prohibit = srb1_params.srb1_timer_status_prohibit/5;
            } else if ((srb1_params.srb1_timer_poll_retransmit >= 300) && (srb1_params.srb1_timer_poll_retransmit <= 500)) {
              rrc->srb1_timer_status_prohibit = (srb1_params.srb1_timer_status_prohibit - 300)/50 + 51;
            } else {
              AssertFatal (0,
                           "Bad config value when parsing eNB configuration file %s, enb %u  srb1_timer_status_prohibit %d!\n",
                           RC.config_file_name, i, srb1_params.srb1_timer_status_prohibit);
            }

            switch (srb1_params.srb1_timer_reordering) {
              case 0:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms0;
                break;

              case 5:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms5;
                break;

              case 10:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms10;
                break;

              case 15:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms15;
                break;

              case 20:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms20;
                break;

              case 25:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms25;
                break;

              case 30:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms30;
                break;

              case 35:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms35;
                break;

              case 40:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms40;
                break;

              case 45:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms45;
                break;

              case 50:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms50;
                break;

              case 55:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms55;
                break;

              case 60:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms60;
                break;

              case 65:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms65;
                break;

              case 70:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms70;
                break;

              case 75:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms75;
                break;

              case 80:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms80;
                break;

              case 85:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms85;
                break;

              case 90:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms90;
                break;

              case 95:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms95;
                break;

              case 100:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms100;
                break;

              case 110:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms110;
                break;

              case 120:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms120;
                break;

              case 130:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms130;
                break;

              case 140:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms140;
                break;

              case 150:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms150;
                break;

              case 160:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms160;
                break;

              case 170:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms170;
                break;

              case 180:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms180;
                break;

              case 190:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms190;
                break;

              case 200:
                rrc->srb1_timer_reordering = LTE_T_Reordering_ms200;
                break;

              default:
                AssertFatal (0,
                             "Bad config value when parsing eNB configuration file %s, enb %u  srb1_timer_reordering %d!\n",
                             RC.config_file_name, i, srb1_params.srb1_timer_reordering);
            }
          }
        }
      }
    }

    memcpy(&rrc->configuration, &RRC_CONFIGURATION_REQ(msg_p), sizeof(RRC_CONFIGURATION_REQ(msg_p)));
  }

  LOG_I(RRC,"Node type %d \n ", rrc->node_type);
  return 0;
}

int RCconfig_DU_F1(MessageDef *msg_p, uint32_t i) {
  int k;
  paramdef_t ENBSParams[] = ENBSPARAMS_DESC;
  paramdef_t ENBParams[]  = ENBPARAMS_DESC;
  paramlist_def_t ENBParamList = {ENB_CONFIG_STRING_ENB_LIST,NULL,0};
  config_get( ENBSParams,sizeof(ENBSParams)/sizeof(paramdef_t),NULL);
  int num_enbs = ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt;
  AssertFatal (i<num_enbs,
               "Failed to parse config file no %uth element in %s \n",i, ENB_CONFIG_STRING_ACTIVE_ENBS);

  if (num_enbs>0) {
    // Output a list of all eNBs.
    config_getlist( &ENBParamList,ENBParams,sizeof(ENBParams)/sizeof(paramdef_t),NULL);
    AssertFatal(ENBParamList.paramarray[i][ENB_ENB_ID_IDX].uptr != NULL,
                "eNB id %u is not defined in configuration file\n",i);
    F1AP_SETUP_REQ (msg_p).num_cells_available = 0;

    for (k=0; k <num_enbs ; k++) {
      if (strcmp(ENBSParams[ENB_ACTIVE_ENBS_IDX].strlistptr[k], *(ENBParamList.paramarray[i][ENB_ENB_NAME_IDX].strptr) )== 0) {
        char aprefix[MAX_OPTNAME_SIZE*2 + 8];
        sprintf(aprefix,"%s.[%i]",ENB_CONFIG_STRING_ENB_LIST,k);
        paramdef_t PLMNParams[] = PLMNPARAMS_DESC;
        paramlist_def_t PLMNParamList = {ENB_CONFIG_STRING_PLMN_LIST, NULL, 0};
        /* map parameter checking array instances to parameter definition array instances */
        checkedparam_t config_check_PLMNParams [] = PLMNPARAMS_CHECK;

        for (int I = 0; I < sizeof(PLMNParams) / sizeof(paramdef_t); ++I)
          PLMNParams[I].chkPptr = &(config_check_PLMNParams[I]);

        config_getlist(&PLMNParamList, PLMNParams, sizeof(PLMNParams)/sizeof(paramdef_t), aprefix);
        paramdef_t SCTPParams[]  = SCTPPARAMS_DESC;
        F1AP_SETUP_REQ (msg_p).num_cells_available++;
        F1AP_SETUP_REQ (msg_p).gNB_DU_id        = *(ENBParamList.paramarray[0][ENB_ENB_ID_IDX].uptr);
        LOG_I(ENB_APP,"F1AP: gNB_DU_id[%d] %ld\n",k,F1AP_SETUP_REQ (msg_p).gNB_DU_id);
        F1AP_SETUP_REQ (msg_p).gNB_DU_name      = strdup(*(ENBParamList.paramarray[0][ENB_ENB_NAME_IDX].strptr));
        LOG_I(ENB_APP,"F1AP: gNB_DU_name[%d] %s\n",k,F1AP_SETUP_REQ (msg_p).gNB_DU_name);
        F1AP_SETUP_REQ (msg_p).tac[k]              = *ENBParamList.paramarray[i][ENB_TRACKING_AREA_CODE_IDX].uptr;
        LOG_I(ENB_APP,"F1AP: tac[%d] %d\n",k,F1AP_SETUP_REQ (msg_p).tac[k]);
        F1AP_SETUP_REQ (msg_p).mcc[k]              = *PLMNParamList.paramarray[0][ENB_MOBILE_COUNTRY_CODE_IDX].uptr;
        LOG_I(ENB_APP,"F1AP: mcc[%d] %d\n",k,F1AP_SETUP_REQ (msg_p).mcc[k]);
        F1AP_SETUP_REQ (msg_p).mnc[k]              = *PLMNParamList.paramarray[0][ENB_MOBILE_NETWORK_CODE_IDX].uptr;
        LOG_I(ENB_APP,"F1AP: mnc[%d] %d\n",k,F1AP_SETUP_REQ (msg_p).mnc[k]);
        F1AP_SETUP_REQ (msg_p).mnc_digit_length[k] = *PLMNParamList.paramarray[0][ENB_MNC_DIGIT_LENGTH].u8ptr;
        LOG_I(ENB_APP,"F1AP: mnc_digit_length[%d] %d\n",k,F1AP_SETUP_REQ (msg_p).mnc_digit_length[k]);
        AssertFatal((F1AP_SETUP_REQ (msg_p).mnc_digit_length[k] == 2) ||
                    (F1AP_SETUP_REQ (msg_p).mnc_digit_length[k] == 3),
                    "BAD MNC DIGIT LENGTH %d",
                    F1AP_SETUP_REQ (msg_p).mnc_digit_length[k]);
        F1AP_SETUP_REQ (msg_p).nr_cellid[k] = (uint64_t)*(ENBParamList.paramarray[i][ENB_NRCELLID_IDX].u64ptr);
        LOG_I(ENB_APP,"F1AP: nr_cellid[%d] %ld\n",k,F1AP_SETUP_REQ (msg_p).nr_cellid[k]);
        LOG_I(ENB_APP,"F1AP: CU_ip4_address in DU %s\n",RC.mac[k]->eth_params_n.remote_addr);
        LOG_I(ENB_APP,"FIAP: CU_ip4_address in DU %p, strlen %d\n",F1AP_SETUP_REQ (msg_p).CU_f1_ip_address.ipv4_address,(int)strlen(RC.mac[k]->eth_params_n.remote_addr));
        F1AP_SETUP_REQ (msg_p).CU_f1_ip_address.ipv6 = 0;
        F1AP_SETUP_REQ (msg_p).CU_f1_ip_address.ipv4 = 1;
        //strcpy(F1AP_SETUP_REQ (msg_p).CU_f1_ip_address.ipv6_address, "");
        strcpy(F1AP_SETUP_REQ (msg_p).CU_f1_ip_address.ipv4_address, RC.mac[k]->eth_params_n.remote_addr);
        LOG_I(ENB_APP,"F1AP: DU_ip4_address in DU %s\n",RC.mac[k]->eth_params_n.my_addr);
        LOG_I(ENB_APP,"FIAP: DU_ip4_address in DU %p, strlen %d\n",F1AP_SETUP_REQ (msg_p).DU_f1_ip_address.ipv4_address,(int)strlen(RC.mac[k]->eth_params_n.my_addr));
        F1AP_SETUP_REQ (msg_p).DU_f1_ip_address.ipv6 = 0;
        F1AP_SETUP_REQ (msg_p).DU_f1_ip_address.ipv4 = 1;
        //strcpy(F1AP_SETUP_REQ (msg_p).DU_f1_ip_address.ipv6_address, "");
        strcpy(F1AP_SETUP_REQ (msg_p).DU_f1_ip_address.ipv4_address, RC.mac[k]->eth_params_n.my_addr);
        //strcpy(F1AP_SETUP_REQ (msg_p).CU_ip_address[l].ipv6_address,*(F1ParamList.paramarray[l][ENB_CU_IPV6_ADDRESS_IDX].strptr));
        //F1AP_SETUP_REQ (msg_p).CU_port = RC.mac[k]->eth_params_n.remote_portc; // maybe we dont need it
        sprintf(aprefix,"%s.[%i].%s",ENB_CONFIG_STRING_ENB_LIST,k,ENB_CONFIG_STRING_SCTP_CONFIG);
        config_get( SCTPParams,sizeof(SCTPParams)/sizeof(paramdef_t),aprefix);
        F1AP_SETUP_REQ (msg_p).sctp_in_streams = (uint16_t)*(SCTPParams[ENB_SCTP_INSTREAMS_IDX].uptr);
        F1AP_SETUP_REQ (msg_p).sctp_out_streams = (uint16_t)*(SCTPParams[ENB_SCTP_OUTSTREAMS_IDX].uptr);
        eNB_RRC_INST *rrc = RC.rrc[k];
        // wait until RRC cell information is configured
        int cell_info_configured=0;

        do {
          LOG_I(ENB_APP,"ngran_eNB_DU: Waiting for basic cell configuration\n");
          usleep(100000);
          pthread_mutex_lock(&rrc->cell_info_mutex);
          cell_info_configured = rrc->cell_info_configured;
          pthread_mutex_unlock(&rrc->cell_info_mutex);
        } while (cell_info_configured ==0);

        rrc->configuration.mcc[0] = F1AP_SETUP_REQ (msg_p).mcc[k];
        rrc->configuration.mnc[0] = F1AP_SETUP_REQ (msg_p).mnc[k];
        rrc->configuration.tac    = F1AP_SETUP_REQ (msg_p).tac[k];
        rrc->nr_cellid = F1AP_SETUP_REQ (msg_p).nr_cellid[k];
        F1AP_SETUP_REQ (msg_p).nr_pci[k]    = rrc->carrier[0].physCellId;
        F1AP_SETUP_REQ (msg_p).num_ssi[k] = 0;

        if (rrc->carrier[0].sib1->tdd_Config) {
          LOG_I(ENB_APP,"ngran_DU: Configuring Cell %d for TDD\n",k);
          F1AP_SETUP_REQ (msg_p).fdd_flag = 0;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].tdd.nr_arfcn            = freq_to_arfcn10(rrc->carrier[0].sib1->freqBandIndicator,
              rrc->carrier[0].dl_CarrierFreq);
          // For LTE use scs field to carry prefix type and number of antennas
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].tdd.scs                 = (rrc->carrier[0].Ncp<<2)+rrc->carrier[0].p_eNB;;
          // use nrb field to hold LTE N_RB_DL (0...5)
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].tdd.nrb                 = rrc->carrier[0].mib.message.dl_Bandwidth;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].tdd.nrb                 = rrc->carrier[0].mib.message.dl_Bandwidth;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].tdd.num_frequency_bands = 1;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].tdd.nr_band[0]          = rrc->carrier[0].sib1->freqBandIndicator;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].tdd.sul_active          = 0;
        } else {
          LOG_I(ENB_APP,"ngran_DU: Configuring Cell %d for FDD\n",k);
          F1AP_SETUP_REQ (msg_p).fdd_flag = 1;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.dl_nr_arfcn             = freq_to_arfcn10(rrc->carrier[0].sib1->freqBandIndicator,
              rrc->carrier[0].dl_CarrierFreq);
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.ul_nr_arfcn             = F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.dl_nr_arfcn;
          // For LTE use scs field to carry prefix type and number of antennas
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.dl_scs                  = (rrc->carrier[0].Ncp<<2)+rrc->carrier[0].p_eNB;;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.ul_scs                  = rrc->carrier[0].Ncp;
          // use nrb field to hold LTE N_RB_DL (0...5)
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.ul_nrb                  = rrc->carrier[0].mib.message.dl_Bandwidth;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.ul_nrb                  = rrc->carrier[0].mib.message.dl_Bandwidth;
          // RK: we need to check there value for FDD's frequency_bands DL/UL
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.ul_num_frequency_bands  = 1;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.ul_nr_band[0]           = rrc->carrier[0].sib1->freqBandIndicator;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.dl_num_frequency_bands  = 1;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.dl_nr_band[0]           = rrc->carrier[0].sib1->freqBandIndicator;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.ul_num_sul_frequency_bands  = 0;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.ul_nr_sul_band[0]           = rrc->carrier[0].sib1->freqBandIndicator;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.dl_num_sul_frequency_bands  = 0;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.dl_nr_sul_band[0]           = rrc->carrier[0].sib1->freqBandIndicator;
          F1AP_SETUP_REQ (msg_p).nr_mode_info[k].fdd.sul_active              = 0;
        }

        F1AP_SETUP_REQ (msg_p).measurement_timing_information[k]             = "0";
        F1AP_SETUP_REQ (msg_p).ranac[k]                                      = 0;
        F1AP_SETUP_REQ (msg_p).mib[k]                                        = rrc->carrier[0].MIB;
        F1AP_SETUP_REQ (msg_p).sib1[k]                                       = rrc->carrier[0].SIB1;
        F1AP_SETUP_REQ (msg_p).mib_length[k]                                 = rrc->carrier[0].sizeof_MIB;
        F1AP_SETUP_REQ (msg_p).sib1_length[k]                                = rrc->carrier[0].sizeof_SIB1;
        break;
      } // if
    } // for
  } // if

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
    AssertFatal((message = itti_alloc_new_message(TASK_ENB_APP, 0, GTPV1U_ENB_S1_REQ))!=NULL,"");
    IPV4_STR_ADDR_TO_INT_NWBO ( address, GTPV1U_ENB_S1_REQ(message).enb_ip_address_for_S1u_S12_S4_up, "BAD IP ADDRESS FORMAT FOR eNB S1_U !\n" );
    LOG_I(GTPU,"Configuring GTPu address : %s -> %x\n",address,GTPV1U_ENB_S1_REQ(message).enb_ip_address_for_S1u_S12_S4_up);
    GTPV1U_ENB_S1_REQ(message).enb_port_for_S1u_S12_S4_up = enb_port_for_S1U;
    strcpy(GTPV1U_ENB_S1_REQ(message).addrStr,address);
    sprintf(GTPV1U_ENB_S1_REQ(message).portStr,"%d", enb_port_for_S1U);
    itti_send_msg_to_task (TASK_VARIABLE, 0, message); // data model is wrong: gtpu doesn't have enb_id (or module_id)
  } else
    LOG_E(GTPU,"invalid address for S1U\n");

  return 0;
}

int RCconfig_M2(MessageDef *msg_p, uint32_t i) {
  int   I, J, j, k, l;
  int   enb_id;
  char *address = NULL;
  char *cidr    = NULL;
  ccparams_lte_t ccparams_lte;
  memset((void *)&ccparams_lte,0,sizeof(ccparams_lte_t));
  paramdef_t ENBSParams[] = ENBSPARAMS_DESC;
  paramdef_t ENBParams[]  = ENBPARAMS_DESC;
  paramlist_def_t ENBParamList = {ENB_CONFIG_STRING_ENB_LIST,NULL,0};
  /* get global parameters, defined outside any section in the config file */
  config_get( ENBSParams,sizeof(ENBSParams)/sizeof(paramdef_t),NULL);
  checkedparam_t config_check_CCparams[] = CCPARAMS_CHECK;
  paramdef_t CCsParams[] = CCPARAMS_DESC(ccparams_lte);
  paramlist_def_t CCsParamList = {ENB_CONFIG_STRING_COMPONENT_CARRIERS, NULL, 0};

 // ccparams_MCE_t MCEconfig;
 // memset((void *)&MCEconfig,0,sizeof(ccparams_MCE_t));
 // paramdef_t MCEParams[]              = MCEPARAMS_DESC((&MCEconfig));
 // checkedparam_t config_check_MCEparams[] = MCEPARAMS_CHECK;


  /* map parameter checking array instances to parameter definition array instances */
  for (I = 0; I < (sizeof(CCsParams) / sizeof(paramdef_t)); I++) {
    CCsParams[I].chkPptr = &(config_check_CCparams[I]);
  }

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
        for (j = 0; j < ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt; j++) {
          if (strcmp(ENBSParams[ENB_ACTIVE_ENBS_IDX].strlistptr[j], *(ENBParamList.paramarray[k][ENB_ENB_NAME_IDX].strptr)) == 0) {
            paramdef_t PLMNParams[] = PLMNPARAMS_DESC;
            paramlist_def_t PLMNParamList = {ENB_CONFIG_STRING_PLMN_LIST, NULL, 0};
            /* map parameter checking array instances to parameter definition array instances */
            checkedparam_t config_check_PLMNParams [] = PLMNPARAMS_CHECK;

            for (int I = 0; I < sizeof(PLMNParams) / sizeof(paramdef_t); ++I)
              PLMNParams[I].chkPptr = &(config_check_PLMNParams[I]);

            paramdef_t M2Params[]  = M2PARAMS_DESC;
            paramlist_def_t M2ParamList = {ENB_CONFIG_STRING_TARGET_MCE_M2_IP_ADDRESS,NULL,0};
            paramdef_t SCTPParams[]  = SCTPPARAMS_DESC;
            paramdef_t NETParams[]  =  NETPARAMS_DESC;
            paramdef_t MBMSConfigParams[]  = MBMS_CONFIG_PARAMS_DESC;
            paramdef_t MBMSParams[]  = MBMSPARAMS_DESC;
           paramlist_def_t MBMSConfigParamList = {ENB_CONFIG_STRING_MBMS_CONFIGURATION_DATA_LIST,NULL,0};
           paramlist_def_t MBMSParamList = {ENB_CONFIG_STRING_MBMS_SERVICE_AREA_LIST,NULL,0};
            /* TODO: fix the size - if set lower we have a crash (MAX_OPTNAME_SIZE was 64 when this code was written) */
            /* this is most probably a problem with the config module */
            char aprefix[MAX_OPTNAME_SIZE*80 + 8];
            sprintf(aprefix,"%s.[%i]",ENB_CONFIG_STRING_ENB_LIST,k);
            /* Some default/random parameters */
            M2AP_REGISTER_ENB_REQ (msg_p).eNB_id = enb_id;

            if (strcmp(*(ENBParamList.paramarray[k][ENB_CELL_TYPE_IDX].strptr), "CELL_MACRO_ENB") == 0) {
              M2AP_REGISTER_ENB_REQ (msg_p).cell_type = CELL_MACRO_ENB;
            } else  if (strcmp(*(ENBParamList.paramarray[k][ENB_CELL_TYPE_IDX].strptr), "CELL_HOME_ENB") == 0) {
              M2AP_REGISTER_ENB_REQ (msg_p).cell_type = CELL_HOME_ENB;
            } else {
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for cell_type choice: CELL_MACRO_ENB or CELL_HOME_ENB !\n",
                           RC.config_file_name, i, *(ENBParamList.paramarray[k][ENB_CELL_TYPE_IDX].strptr));
            }

            M2AP_REGISTER_ENB_REQ (msg_p).eNB_name         = strdup(*(ENBParamList.paramarray[k][ENB_ENB_NAME_IDX].strptr));
            M2AP_REGISTER_ENB_REQ (msg_p).tac              = *ENBParamList.paramarray[k][ENB_TRACKING_AREA_CODE_IDX].uptr;
            config_getlist(&PLMNParamList, PLMNParams, sizeof(PLMNParams)/sizeof(paramdef_t), aprefix);



//            char aprefix2[MAX_OPTNAME_SIZE*80 + 8];
//            sprintf(aprefix2,"%s.[%i].%s.[0]",ENB_CONFIG_STRING_ENB_LIST,k,ENB_CONFIG_STRING_MBMS_CONFIGURATION_DATA_LIST);
//            config_getlist(&MBMSParamList, MBMSParams, sizeof(MBMSParams)/sizeof(paramdef_t), aprefix2);
//         if (MBMSParamList.numelt < 1 || MBMSParamList.numelt > 8)
//              AssertFatal(0, "The number of MBMS Areas must be in [1,8], but is %d\n",
//                          MBMSParamList.numelt);
//         M2AP_REGISTER_ENB_REQ (msg_p).num_mbms_service_area_list = MBMSParamList.numelt;
//         for(J=0; J<MBMSParamList.numelt;J++){
//             M2AP_REGISTER_ENB_REQ (msg_p).mbms_service_area_list[J] = *MBMSParamList.paramarray[J][ENB_MBMS_SERVICE_AREA_IDX].uptr;
//         }
//

            char aprefix2[MAX_OPTNAME_SIZE*80 + 8];
            sprintf(aprefix2,"%s.[%i]",ENB_CONFIG_STRING_ENB_LIST,k);
            config_getlist(&MBMSConfigParamList, MBMSConfigParams, sizeof(MBMSConfigParams)/sizeof(paramdef_t), aprefix2);
            if (MBMSConfigParamList.numelt < 1 || MBMSConfigParamList.numelt > 8)
              AssertFatal(0, "The number of MBMS Config Data must be in [1,8], but is %d\n",
                          MBMSConfigParamList.numelt);
            M2AP_REGISTER_ENB_REQ (msg_p).num_mbms_configuration_data_list = MBMSConfigParamList.numelt;
           for(int I=0; I < MBMSConfigParamList.numelt; I++){

                   sprintf(aprefix2,"%s.[%i].%s.[%i]",ENB_CONFIG_STRING_ENB_LIST,k,ENB_CONFIG_STRING_MBMS_CONFIGURATION_DATA_LIST,I);
                   config_getlist(&MBMSParamList, MBMSParams, sizeof(MBMSParams)/sizeof(paramdef_t), aprefix2);
                   if (MBMSParamList.numelt < 1 || MBMSParamList.numelt > 8)
                     AssertFatal(0, "The number of MBMS Areas must be in [1,8], but is %d\n",
                                 MBMSParamList.numelt);
                   M2AP_REGISTER_ENB_REQ (msg_p).mbms_configuration_data_list[I].num_mbms_service_area_list = MBMSParamList.numelt;
                   for(J=0; J<MBMSParamList.numelt;J++){
                       M2AP_REGISTER_ENB_REQ (msg_p).mbms_configuration_data_list[I].mbms_service_area_list[J] = *MBMSParamList.paramarray[J][ENB_MBMS_SERVICE_AREA_IDX].uptr;
                   }

          }


            if (PLMNParamList.numelt < 1 || PLMNParamList.numelt > 6)
              AssertFatal(0, "The number of PLMN IDs must be in [1,6], but is %d\n",
                          PLMNParamList.numelt);

            if (PLMNParamList.numelt > 1)
              LOG_W(M2AP, "M2AP currently handles only one PLMN, ignoring the others!\n");

            M2AP_REGISTER_ENB_REQ (msg_p).mcc = *PLMNParamList.paramarray[0][ENB_MOBILE_COUNTRY_CODE_IDX].uptr;
            M2AP_REGISTER_ENB_REQ (msg_p).mnc = *PLMNParamList.paramarray[0][ENB_MOBILE_NETWORK_CODE_IDX].uptr;
            M2AP_REGISTER_ENB_REQ (msg_p).mnc_digit_length = *PLMNParamList.paramarray[0][ENB_MNC_DIGIT_LENGTH].u8ptr;
            AssertFatal(M2AP_REGISTER_ENB_REQ(msg_p).mnc_digit_length == 3
                        || M2AP_REGISTER_ENB_REQ(msg_p).mnc < 100,
                        "MNC %d cannot be encoded in two digits as requested (change mnc_digit_length to 3)\n",
                        M2AP_REGISTER_ENB_REQ(msg_p).mnc);
            /* CC params */
            config_getlist(&CCsParamList, NULL, 0, aprefix);
            M2AP_REGISTER_ENB_REQ (msg_p).num_cc = CCsParamList.numelt;

            if (CCsParamList.numelt > 0) {
              //char ccspath[MAX_OPTNAME_SIZE*2 + 16];
              for (J = 0; J < CCsParamList.numelt ; J++) {
                sprintf(aprefix, "%s.[%i].%s.[%i]", ENB_CONFIG_STRING_ENB_LIST, k, ENB_CONFIG_STRING_COMPONENT_CARRIERS, J);
                config_get(CCsParams, sizeof(CCsParams)/sizeof(paramdef_t), aprefix);
                M2AP_REGISTER_ENB_REQ (msg_p).eutra_band[J] = ccparams_lte.eutra_band;
                M2AP_REGISTER_ENB_REQ (msg_p).downlink_frequency[J] = (uint32_t) ccparams_lte.downlink_frequency;
                M2AP_REGISTER_ENB_REQ (msg_p).uplink_frequency_offset[J] = (unsigned int) ccparams_lte.uplink_frequency_offset;
                M2AP_REGISTER_ENB_REQ (msg_p).Nid_cell[J]= ccparams_lte.Nid_cell;

                if (ccparams_lte.Nid_cell>503) {
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for Nid_cell choice: 0...503 !\n",
                               RC.config_file_name, k, ccparams_lte.Nid_cell);
                }

                M2AP_REGISTER_ENB_REQ (msg_p).N_RB_DL[J]= ccparams_lte.N_RB_DL;

                if ((ccparams_lte.N_RB_DL!=6) && (ccparams_lte.N_RB_DL!=15) && (ccparams_lte.N_RB_DL!=25) && (ccparams_lte.N_RB_DL!=50) && (ccparams_lte.N_RB_DL!=75) && (ccparams_lte.N_RB_DL!=100)) {
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for N_RB_DL choice: 6,15,25,50,75,100 !\n",
                               RC.config_file_name, k, ccparams_lte.N_RB_DL);
                }

                if (strcmp(ccparams_lte.frame_type, "FDD") == 0) {
                  M2AP_REGISTER_ENB_REQ (msg_p).frame_type[J] = FDD;
                } else  if (strcmp(ccparams_lte.frame_type, "TDD") == 0) {
                  M2AP_REGISTER_ENB_REQ (msg_p).frame_type[J] = TDD;
                } else {
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for frame_type choice: FDD or TDD !\n",
                               RC.config_file_name, k, ccparams_lte.frame_type);
                }

                M2AP_REGISTER_ENB_REQ (msg_p).fdd_earfcn_DL[J] = to_earfcn_DL(ccparams_lte.eutra_band, ccparams_lte.downlink_frequency, ccparams_lte.N_RB_DL);
                M2AP_REGISTER_ENB_REQ (msg_p).fdd_earfcn_UL[J] = to_earfcn_UL(ccparams_lte.eutra_band, ccparams_lte.downlink_frequency + ccparams_lte.uplink_frequency_offset, ccparams_lte.N_RB_DL);
              }
            }

            sprintf(aprefix,"%s.[%i]",ENB_CONFIG_STRING_ENB_LIST,k);
            config_getlist( &M2ParamList,M2Params,sizeof(M2Params)/sizeof(paramdef_t),aprefix);
            AssertFatal(M2ParamList.numelt <= M2AP_MAX_NB_ENB_IP_ADDRESS,
                        "value of M2ParamList.numelt %d must be lower than M2AP_MAX_NB_ENB_IP_ADDRESS %d value: reconsider to increase M2AP_MAX_NB_ENB_IP_ADDRESS\n",
                        M2ParamList.numelt,M2AP_MAX_NB_ENB_IP_ADDRESS);
            M2AP_REGISTER_ENB_REQ (msg_p).nb_m2 = 0;

            for (l = 0; l < M2ParamList.numelt; l++) {
              M2AP_REGISTER_ENB_REQ (msg_p).nb_m2 += 1;
              strcpy(M2AP_REGISTER_ENB_REQ (msg_p).target_mce_m2_ip_address[l].ipv4_address,*(M2ParamList.paramarray[l][ENB_M2_IPV4_ADDRESS_IDX].strptr));
              strcpy(M2AP_REGISTER_ENB_REQ (msg_p).target_mce_m2_ip_address[l].ipv6_address,*(M2ParamList.paramarray[l][ENB_M2_IPV6_ADDRESS_IDX].strptr));

              if (strcmp(*(M2ParamList.paramarray[l][ENB_M2_IP_ADDRESS_PREFERENCE_IDX].strptr), "ipv4") == 0) {
                M2AP_REGISTER_ENB_REQ (msg_p).target_mce_m2_ip_address[l].ipv4 = 1;
                M2AP_REGISTER_ENB_REQ (msg_p).target_mce_m2_ip_address[l].ipv6 = 0;
              } else if (strcmp(*(M2ParamList.paramarray[l][ENB_M2_IP_ADDRESS_PREFERENCE_IDX].strptr), "ipv6") == 0) {
                M2AP_REGISTER_ENB_REQ (msg_p).target_mce_m2_ip_address[l].ipv4 = 0;
                M2AP_REGISTER_ENB_REQ (msg_p).target_mce_m2_ip_address[l].ipv6 = 1;
              } else if (strcmp(*(M2ParamList.paramarray[l][ENB_M2_IP_ADDRESS_PREFERENCE_IDX].strptr), "no") == 0) {
                M2AP_REGISTER_ENB_REQ (msg_p).target_mce_m2_ip_address[l].ipv4 = 1;
                M2AP_REGISTER_ENB_REQ (msg_p).target_mce_m2_ip_address[l].ipv6 = 1;
              }
            }
            // timers
            //{
            //  int t_reloc_prep = 0;
            //  int tx2_reloc_overall = 0;
            //  paramdef_t p[] = {
            //    { "t_reloc_prep", "t_reloc_prep", 0, iptr:&t_reloc_prep, defintval:0, TYPE_INT, 0 },
            //    { "tx2_reloc_overall", "tx2_reloc_overall", 0, iptr:&tx2_reloc_overall, defintval:0, TYPE_INT, 0 }
            //  };
            //  config_get(p, sizeof(p)/sizeof(paramdef_t), aprefix);

            //  if (t_reloc_prep <= 0 || t_reloc_prep > 10000 ||
            //      tx2_reloc_overall <= 0 || tx2_reloc_overall > 20000) {
            //    LOG_E(M2AP, "timers in configuration file have wrong values. We must have [0 < t_reloc_prep <= 10000] and [0 < tx2_reloc_overall <= 20000]\n");
            //    exit(1);
            //  }

            //  M2AP_REGISTER_ENB_REQ (msg_p).t_reloc_prep = t_reloc_prep;
            //  M2AP_REGISTER_ENB_REQ (msg_p).tx2_reloc_overall = tx2_reloc_overall;
            //}
            // SCTP SETTING
            M2AP_REGISTER_ENB_REQ (msg_p).sctp_out_streams = SCTP_OUT_STREAMS;
            M2AP_REGISTER_ENB_REQ (msg_p).sctp_in_streams  = SCTP_IN_STREAMS;

            if (EPC_MODE_ENABLED) {
              sprintf(aprefix,"%s.[%i].%s",ENB_CONFIG_STRING_ENB_LIST,k,ENB_CONFIG_STRING_SCTP_CONFIG);
              config_get( SCTPParams,sizeof(SCTPParams)/sizeof(paramdef_t),aprefix);
              M2AP_REGISTER_ENB_REQ (msg_p).sctp_in_streams = (uint16_t)*(SCTPParams[ENB_SCTP_INSTREAMS_IDX].uptr);
              M2AP_REGISTER_ENB_REQ (msg_p).sctp_out_streams = (uint16_t)*(SCTPParams[ENB_SCTP_OUTSTREAMS_IDX].uptr);
            }

            sprintf(aprefix,"%s.[%i].%s",ENB_CONFIG_STRING_ENB_LIST,k,ENB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);
            // NETWORK_INTERFACES
            config_get( NETParams,sizeof(NETParams)/sizeof(paramdef_t),aprefix);
            M2AP_REGISTER_ENB_REQ (msg_p).enb_port_for_M2C = (uint32_t)*(NETParams[ENB_PORT_FOR_M2C_IDX].uptr);

            if ((NETParams[ENB_IPV4_ADDR_FOR_M2C_IDX].strptr == NULL) || (M2AP_REGISTER_ENB_REQ (msg_p).enb_port_for_M2C == 0)) {
              LOG_E(RRC,"Add eNB IPv4 address and/or port for M2C in the CONF file!\n");
              exit(1);
            }

            cidr = *(NETParams[ENB_IPV4_ADDR_FOR_M2C_IDX].strptr);
            address = strtok(cidr, "/");
            M2AP_REGISTER_ENB_REQ (msg_p).enb_m2_ip_address.ipv6 = 0;
            M2AP_REGISTER_ENB_REQ (msg_p).enb_m2_ip_address.ipv4 = 1;
            strcpy(M2AP_REGISTER_ENB_REQ (msg_p).enb_m2_ip_address.ipv4_address, address);
          }
        }
      }
    }
  }

  return 0;
}

//-----------------------------------------------------------------------------
/*
* Configure the s1ap_register_enb_req in itti message for future
* communications between eNB(s) and MME.
*/
int RCconfig_S1(
  MessageDef *msg_p,
  uint32_t i)
//-----------------------------------------------------------------------------
{
  int enb_id = 0;
  int32_t my_int = 0;
  const char *active_enb[MAX_ENB];
  char *address = NULL;
  char *cidr    = NULL;
  ccparams_lte_t ccparams_lte;
  memset((void *)&ccparams_lte,0,sizeof(ccparams_lte_t));
  // for no gcc warnings
  (void)my_int;
  memset((char *)active_enb, 0, MAX_ENB * sizeof(char *));
  paramdef_t ENBSParams[] = ENBSPARAMS_DESC;
  paramdef_t ENBParams[] = ENBPARAMS_DESC;
  paramlist_def_t ENBParamList = {ENB_CONFIG_STRING_ENB_LIST, NULL, 0};
  /* get global parameters, defined outside any section in the config file */
  config_get(ENBSParams, sizeof(ENBSParams)/sizeof(paramdef_t), NULL);
  AssertFatal (i < ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt,
               "Failed to parse config file %s, %uth attribute %s \n",
               RC.config_file_name, i, ENB_CONFIG_STRING_ACTIVE_ENBS);

  if (ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt > 0) {
    // Output a list of all eNBs.
    config_getlist(&ENBParamList, ENBParams, sizeof(ENBParams)/sizeof(paramdef_t), NULL);

    if (ENBParamList.numelt > 0) {
      for (int k = 0; k < ENBParamList.numelt; k++) {
        if (ENBParamList.paramarray[k][ENB_ENB_ID_IDX].uptr == NULL) {
          // Calculate a default eNB ID
          if (EPC_MODE_ENABLED) {
            uint32_t hash = 0;
            hash = s1ap_generate_eNB_id();
            enb_id = k + (hash & 0xFFFF8);
          } else {
            enb_id = k;
          }
        } else {
          enb_id = *(ENBParamList.paramarray[k][ENB_ENB_ID_IDX].uptr);
        }

        // search if in active list
        for (int j = 0; j < ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt; j++) {
          if (strcmp(ENBSParams[ENB_ACTIVE_ENBS_IDX].strlistptr[j], *(ENBParamList.paramarray[k][ENB_ENB_NAME_IDX].strptr)) == 0) {
            paramdef_t PLMNParams[] = PLMNPARAMS_DESC;
            paramlist_def_t PLMNParamList = {ENB_CONFIG_STRING_PLMN_LIST, NULL, 0};
            paramdef_t CCsParams[] = CCPARAMS_DESC(ccparams_lte);
            /* map parameter checking array instances to parameter definition array instances */
            checkedparam_t config_check_CCparams[] = CCPARAMS_CHECK;

            for (int I = 0; I < (sizeof(CCsParams) / sizeof(paramdef_t)); I++) {
              CCsParams[I].chkPptr = &(config_check_CCparams[I]);
            }

            /* map parameter checking array instances to parameter definition array instances */
            checkedparam_t config_check_PLMNParams [] = PLMNPARAMS_CHECK;

            for (int I = 0; I < sizeof(PLMNParams) / sizeof(paramdef_t); ++I) {
              PLMNParams[I].chkPptr = &(config_check_PLMNParams[I]);
            }

            paramdef_t S1Params[] = S1PARAMS_DESC;
            paramlist_def_t S1ParamList = {ENB_CONFIG_STRING_MME_IP_ADDRESS, NULL, 0};
            paramdef_t SCTPParams[] = SCTPPARAMS_DESC;
            paramdef_t NETParams[] =  NETPARAMS_DESC;
            char aprefix[MAX_OPTNAME_SIZE*2 + 8];
            sprintf(aprefix, "%s.[%i]", ENB_CONFIG_STRING_ENB_LIST, k);
            S1AP_REGISTER_ENB_REQ (msg_p).eNB_id = enb_id;

            if (strcmp(*(ENBParamList.paramarray[k][ENB_CELL_TYPE_IDX].strptr), "CELL_MACRO_ENB") == 0) {
              S1AP_REGISTER_ENB_REQ (msg_p).cell_type = CELL_MACRO_ENB;
            } else  if (strcmp(*(ENBParamList.paramarray[k][ENB_CELL_TYPE_IDX].strptr), "CELL_HOME_ENB") == 0) {
              S1AP_REGISTER_ENB_REQ (msg_p).cell_type = CELL_HOME_ENB;
              // Temporary option to be able to parse an eNB configuration file which is treated as gNB from
              // the X2AP layer and test the setup of an ENDC X2AP connection. To be removed when we are ready to
              // parse an actual gNB configuration file wrt. the X2AP parameters instead.
            } else  if (strcmp(*(ENBParamList.paramarray[k][ENB_CELL_TYPE_IDX].strptr), "CELL_MACRO_GNB") == 0) {
              S1AP_REGISTER_ENB_REQ (msg_p).cell_type = CELL_MACRO_GNB;
            } else {
              AssertFatal(0,
                          "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for cell_type choice: CELL_MACRO_ENB or CELL_HOME_ENB !\n",
                          RC.config_file_name,
                          i,
                          *(ENBParamList.paramarray[k][ENB_CELL_TYPE_IDX].strptr));
            }

            S1AP_REGISTER_ENB_REQ (msg_p).eNB_name = strdup(*(ENBParamList.paramarray[k][ENB_ENB_NAME_IDX].strptr));
            S1AP_REGISTER_ENB_REQ(msg_p).tac = *ENBParamList.paramarray[k][ENB_TRACKING_AREA_CODE_IDX].uptr;
            AssertFatal(!ENBParamList.paramarray[k][ENB_MOBILE_COUNTRY_CODE_IDX_OLD].strptr
                        && !ENBParamList.paramarray[k][ENB_MOBILE_NETWORK_CODE_IDX_OLD].strptr,
                        "It seems that you use an old configuration file. Please change the existing\n"
                        "    tracking_area_code  =  \"1\";\n"
                        "    mobile_country_code =  \"208\";\n"
                        "    mobile_network_code =  \"93\";\n"
                        "to\n"
                        "    tracking_area_code  =  1; // no string!!\n"
                        "    plmn_list = ( { mcc = 208; mnc = 93; mnc_length = 2; } )\n");
            if(*ENBParamList.paramarray[k][ENB_S1SETUP_RSP_TIMER_IDX].uptr <= 0xffff)
            {
              S1AP_REGISTER_ENB_REQ(msg_p).s1_setuprsp_wait_timer = *ENBParamList.paramarray[k][ENB_S1SETUP_RSP_TIMER_IDX].uptr;
            }
            else
            {
              LOG_E(S1AP, 
                    "s1setup_rsp_timer value in conf file is invalid (%d). Default value is set.\n",
                    *ENBParamList.paramarray[k][ENB_S1SETUP_RSP_TIMER_IDX].uptr);
              S1AP_REGISTER_ENB_REQ(msg_p).s1_setuprsp_wait_timer = 5;
            }

            if(*ENBParamList.paramarray[k][ENB_S1SETUP_REQ_TIMER_IDX].uptr <= 0xffff)
            {
              S1AP_REGISTER_ENB_REQ(msg_p).s1_setupreq_wait_timer = *ENBParamList.paramarray[k][ENB_S1SETUP_REQ_TIMER_IDX].uptr;
            }
            else
            {
              LOG_E(S1AP, 
                    "s1setup_req_timer value in conf file is invalid (%d). Default value is set.\n",
                    *ENBParamList.paramarray[k][ENB_S1SETUP_REQ_TIMER_IDX].uptr);
              S1AP_REGISTER_ENB_REQ(msg_p).s1_setupreq_wait_timer = 5;
            }

            if(*ENBParamList.paramarray[k][ENB_S1SETUP_REQ_COUNT_IDX].uptr <= 0xffff)
            {
              S1AP_REGISTER_ENB_REQ(msg_p).s1_setupreq_count = *ENBParamList.paramarray[k][ENB_S1SETUP_REQ_COUNT_IDX].uptr;
            }
            else
            {
              LOG_E(S1AP, 
                    "s1setup_req_count value in conf file is invalid (%d). Default value is set.\n",
                    *ENBParamList.paramarray[k][ENB_S1SETUP_REQ_COUNT_IDX].uptr);
              S1AP_REGISTER_ENB_REQ(msg_p).s1_setupreq_count = 0xffff;
            }

            if(*ENBParamList.paramarray[k][ENB_SCTP_REQ_TIMER_IDX].uptr <= 0xffff)
            {
              S1AP_REGISTER_ENB_REQ(msg_p).sctp_req_timer = *ENBParamList.paramarray[k][ENB_SCTP_REQ_TIMER_IDX].uptr;
            }
            else
            {
              LOG_E(S1AP, 
                    "sctp_req_timer value in conf file is invalid (%d). Default value is set.\n",
                    *ENBParamList.paramarray[k][ENB_SCTP_REQ_TIMER_IDX].uptr);
              S1AP_REGISTER_ENB_REQ(msg_p).sctp_req_timer = 180;
            }

            if(*ENBParamList.paramarray[k][ENB_SCTP_REQ_COUNT_IDX].uptr <= 0xffff)
            {
              S1AP_REGISTER_ENB_REQ(msg_p).sctp_req_count = *ENBParamList.paramarray[k][ENB_SCTP_REQ_COUNT_IDX].uptr;
            }
            else
            {
              LOG_E(S1AP, 
                    "sctp_req_count value in conf file is invalid (%d). Default value is set.\n",
                    *ENBParamList.paramarray[k][ENB_SCTP_REQ_COUNT_IDX].uptr);
              S1AP_REGISTER_ENB_REQ(msg_p).sctp_req_count = 0xffff;
            }
            config_getlist(&PLMNParamList, PLMNParams, sizeof(PLMNParams)/sizeof(paramdef_t), aprefix);

            if (PLMNParamList.numelt < 1 || PLMNParamList.numelt > 6) {
              AssertFatal(0, "The number of PLMN IDs must be in [1,6], but is %d\n",
                          PLMNParamList.numelt);
            }

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

            /* Default DRX param */
            /*
            * Here we get the config of the first CC, since the s1ap_register_enb_req_t doesn't support multiple CC.
            * There is a unique value of defaultPagingCycle per eNB (same for multiple cells).
            * Hence, it should be stated somewhere that the value should be the same for every CC, or put the value outside the CC
            * in the conf file.
            */
            sprintf(aprefix, "%s.[%i].%s.[%i]", ENB_CONFIG_STRING_ENB_LIST, k, ENB_CONFIG_STRING_COMPONENT_CARRIERS, 0);
            config_get(CCsParams, sizeof(CCsParams)/sizeof(paramdef_t), aprefix);

            switch (ccparams_lte.pcch_defaultPagingCycle) {
              case 32: {
                S1AP_REGISTER_ENB_REQ(msg_p).default_drx = 0;
                break;
              }

              case 64: {
                S1AP_REGISTER_ENB_REQ(msg_p).default_drx = 1;
                break;
              }

              case 128: {
                S1AP_REGISTER_ENB_REQ(msg_p).default_drx = 2;
                break;
              }

              case 256: {
                S1AP_REGISTER_ENB_REQ(msg_p).default_drx = 3;
                break;
              }

              default: {
                LOG_E(S1AP, "Default I-DRX value in conf file is invalid (%i). Should be 32, 64, 128 or 256. \
                       Default DRX set to 32 in MME configuration\n",
                      ccparams_lte.pcch_defaultPagingCycle);
                S1AP_REGISTER_ENB_REQ(msg_p).default_drx = 0;
              }
            }

            /* MME connection params */
            sprintf(aprefix, "%s.[%i]", ENB_CONFIG_STRING_ENB_LIST, k);
            config_getlist(&S1ParamList, S1Params, sizeof(S1Params)/sizeof(paramdef_t), aprefix);
            S1AP_REGISTER_ENB_REQ (msg_p).nb_mme = 0;

            for (int l = 0; l < S1ParamList.numelt; l++) {
              S1AP_REGISTER_ENB_REQ (msg_p).nb_mme += 1;
              strcpy(S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[l].ipv4_address,*(S1ParamList.paramarray[l][ENB_MME_IPV4_ADDRESS_IDX].strptr));
              strcpy(S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[l].ipv6_address,*(S1ParamList.paramarray[l][ENB_MME_IPV6_ADDRESS_IDX].strptr));

              if (strcmp(*(S1ParamList.paramarray[l][ENB_MME_IP_ADDRESS_PREFERENCE_IDX].strptr), "ipv4") == 0) {
                S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[l].ipv4 = 1;
                S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[l].ipv6 = 0;
              } else if (strcmp(*(S1ParamList.paramarray[l][ENB_MME_IP_ADDRESS_PREFERENCE_IDX].strptr), "ipv6") == 0) {
                S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[l].ipv4 = 0;
                S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[l].ipv6 = 1;
              } else if (strcmp(*(S1ParamList.paramarray[l][ENB_MME_IP_ADDRESS_PREFERENCE_IDX].strptr), "no") == 0) {
                S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[l].ipv4 = 1;
                S1AP_REGISTER_ENB_REQ (msg_p).mme_ip_address[l].ipv6 = 1;
              }

              if (S1ParamList.paramarray[l][ENB_MME_BROADCAST_PLMN_INDEX].iptr) {
                S1AP_REGISTER_ENB_REQ(msg_p).broadcast_plmn_num[l] = S1ParamList.paramarray[l][ENB_MME_BROADCAST_PLMN_INDEX].numelt;
              } else {
                S1AP_REGISTER_ENB_REQ(msg_p).broadcast_plmn_num[l] = 0;
              }

              /* set S1-mme port (sctp) */
              S1AP_REGISTER_ENB_REQ(msg_p).mme_port[l] = *S1ParamList.paramarray[l][ENB_MME_PORT_IDX].u16ptr;

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

                for (int el = 0; el < S1AP_REGISTER_ENB_REQ(msg_p).num_plmn; ++el) {
                  S1AP_REGISTER_ENB_REQ(msg_p).broadcast_plmn_index[l][el] = el;
                }
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
  ccparams_lte_t ccparams_lte;
  memset((void *)&ccparams_lte,0,sizeof(ccparams_lte_t));
  paramdef_t ENBSParams[] = ENBSPARAMS_DESC;
  paramdef_t ENBParams[]  = ENBPARAMS_DESC;
  paramlist_def_t ENBParamList = {ENB_CONFIG_STRING_ENB_LIST,NULL,0};
  /* get global parameters, defined outside any section in the config file */
  config_get( ENBSParams,sizeof(ENBSParams)/sizeof(paramdef_t),NULL);
  checkedparam_t config_check_CCparams[] = CCPARAMS_CHECK;
  paramdef_t CCsParams[] = CCPARAMS_DESC(ccparams_lte);
  paramlist_def_t CCsParamList = {ENB_CONFIG_STRING_COMPONENT_CARRIERS, NULL, 0};

  /* map parameter checking array instances to parameter definition array instances */
  for (I = 0; I < (sizeof(CCsParams) / sizeof(paramdef_t)); I++) {
    CCsParams[I].chkPptr = &(config_check_CCparams[I]);
  }

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
            }else {
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %u unknown value \"%s\" for cell_type choice: CELL_MACRO_ENB or CELL_HOME_ENB !\n",
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
                config_get(CCsParams, sizeof(CCsParams)/sizeof(paramdef_t), aprefix);
                X2AP_REGISTER_ENB_REQ (msg_p).eutra_band[J] = ccparams_lte.eutra_band;
                X2AP_REGISTER_ENB_REQ (msg_p).downlink_frequency[J] = (uint32_t) ccparams_lte.downlink_frequency;
                X2AP_REGISTER_ENB_REQ (msg_p).uplink_frequency_offset[J] = (unsigned int) ccparams_lte.uplink_frequency_offset;
                X2AP_REGISTER_ENB_REQ (msg_p).Nid_cell[J]= ccparams_lte.Nid_cell;

                if (ccparams_lte.Nid_cell>503) {
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for Nid_cell choice: 0...503 !\n",
                               RC.config_file_name, k, ccparams_lte.Nid_cell);
                }

                X2AP_REGISTER_ENB_REQ (msg_p).N_RB_DL[J]= ccparams_lte.N_RB_DL;

                if ((ccparams_lte.N_RB_DL!=6) && (ccparams_lte.N_RB_DL!=15) && (ccparams_lte.N_RB_DL!=25) && (ccparams_lte.N_RB_DL!=50) && (ccparams_lte.N_RB_DL!=75) && (ccparams_lte.N_RB_DL!=100)) {
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for N_RB_DL choice: 6,15,25,50,75,100 !\n",
                               RC.config_file_name, k, ccparams_lte.N_RB_DL);
                }

                if (strcmp(ccparams_lte.frame_type, "FDD") == 0) {
                  X2AP_REGISTER_ENB_REQ (msg_p).frame_type[J] = FDD;
                } else  if (strcmp(ccparams_lte.frame_type, "TDD") == 0) {
                  X2AP_REGISTER_ENB_REQ (msg_p).frame_type[J] = TDD;
                  X2AP_REGISTER_ENB_REQ (msg_p).subframeAssignment[J] = ccparams_lte.tdd_config;
                  X2AP_REGISTER_ENB_REQ (msg_p).specialSubframe[J] = ccparams_lte.tdd_config_s;
                } else {
                  AssertFatal (0,
                               "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for frame_type choice: FDD or TDD !\n",
                               RC.config_file_name, k, ccparams_lte.frame_type);
                }

                X2AP_REGISTER_ENB_REQ (msg_p).fdd_earfcn_DL[J] = to_earfcn_DL(ccparams_lte.eutra_band, ccparams_lte.downlink_frequency, ccparams_lte.N_RB_DL);
                X2AP_REGISTER_ENB_REQ (msg_p).fdd_earfcn_UL[J] = to_earfcn_UL(ccparams_lte.eutra_band, ccparams_lte.downlink_frequency + ccparams_lte.uplink_frequency_offset, ccparams_lte.N_RB_DL);
              }
            }

            sprintf(aprefix,"%s.[%i]",ENB_CONFIG_STRING_ENB_LIST,k);
            config_getlist( &X2ParamList,X2Params,sizeof(X2Params)/sizeof(paramdef_t),aprefix);
            AssertFatal(X2ParamList.numelt <= X2AP_MAX_NB_ENB_IP_ADDRESS,
                        "value of X2ParamList.numelt %d must be lower than X2AP_MAX_NB_ENB_IP_ADDRESS %d value: reconsider to increase X2AP_MAX_NB_ENB_IP_ADDRESS\n",
                        X2ParamList.numelt,X2AP_MAX_NB_ENB_IP_ADDRESS);
            X2AP_REGISTER_ENB_REQ (msg_p).nb_x2 = 0;
            LOG_I(X2AP,"X2ParamList.numelt %d\n",X2ParamList.numelt);
            for (l = 0; l < X2ParamList.numelt; l++) {
              X2AP_REGISTER_ENB_REQ (msg_p).nb_x2 += 1;
              strcpy(X2AP_REGISTER_ENB_REQ (msg_p).target_enb_x2_ip_address[l].ipv4_address,*(X2ParamList.paramarray[l][ENB_X2_IPV4_ADDRESS_IDX].strptr));
              strcpy(X2AP_REGISTER_ENB_REQ (msg_p).target_enb_x2_ip_address[l].ipv6_address,*(X2ParamList.paramarray[l][ENB_X2_IPV6_ADDRESS_IDX].strptr));

              LOG_I(X2AP,"registering with ip : %s\n",*(X2ParamList.paramarray[l][ENB_X2_IPV4_ADDRESS_IDX].strptr));
              if (strcmp(*(X2ParamList.paramarray[l][ENB_X2_IP_ADDRESS_PREFERENCE_IDX].strptr), "ipv4") == 0) {
                X2AP_REGISTER_ENB_REQ (msg_p).target_enb_x2_ip_address[l].ipv4 = 1;
                X2AP_REGISTER_ENB_REQ (msg_p).target_enb_x2_ip_address[l].ipv6 = 0;
              } else if (strcmp(*(X2ParamList.paramarray[l][ENB_X2_IP_ADDRESS_PREFERENCE_IDX].strptr), "ipv6") == 0) {
                X2AP_REGISTER_ENB_REQ (msg_p).target_enb_x2_ip_address[l].ipv4 = 0;
                X2AP_REGISTER_ENB_REQ (msg_p).target_enb_x2_ip_address[l].ipv6 = 1;
              } else if (strcmp(*(X2ParamList.paramarray[l][ENB_X2_IP_ADDRESS_PREFERENCE_IDX].strptr), "no") == 0) {
                X2AP_REGISTER_ENB_REQ (msg_p).target_enb_x2_ip_address[l].ipv4 = 1;
                X2AP_REGISTER_ENB_REQ (msg_p).target_enb_x2_ip_address[l].ipv6 = 1;
              }
            }

            // timers
            {
              int t_reloc_prep = 0;
              int tx2_reloc_overall = 0;
              int t_dc_prep = 0;
              int t_dc_overall = 0;
              paramdef_t p[] = {
                { "t_reloc_prep", "t_reloc_prep", 0, iptr:&t_reloc_prep, defintval:0, TYPE_INT, 0 },
                { "tx2_reloc_overall", "tx2_reloc_overall", 0, iptr:&tx2_reloc_overall, defintval:0, TYPE_INT, 0 },
                { "t_dc_prep", "t_dc_prep", 0, iptr:&t_dc_prep, defintval:0, TYPE_INT, 0 },
                { "t_dc_overall", "t_dc_overall", 0, iptr:&t_dc_overall, defintval:0, TYPE_INT, 0 }
              };
              config_get(p, sizeof(p)/sizeof(paramdef_t), aprefix);

              if (t_reloc_prep <= 0 || t_reloc_prep > 10000 ||
                  tx2_reloc_overall <= 0 || tx2_reloc_overall > 20000 ||
                  t_dc_prep <= 0 || t_dc_prep > 10000 ||
                  t_dc_overall <= 0 || t_dc_overall > 20000) {
                LOG_E(X2AP, "timers in configuration file have wrong values. We must have [0 < t_reloc_prep <= 10000] and [0 < tx2_reloc_overall <= 20000] and [0 < t_dc_prep <= 10000] and [0 < t_dc_overall <= 20000]\n");
                exit(1);
              }

              X2AP_REGISTER_ENB_REQ (msg_p).t_reloc_prep = t_reloc_prep;
              X2AP_REGISTER_ENB_REQ (msg_p).tx2_reloc_overall = tx2_reloc_overall;
              X2AP_REGISTER_ENB_REQ (msg_p).t_dc_prep = t_dc_prep;
              X2AP_REGISTER_ENB_REQ (msg_p).t_dc_overall = t_dc_overall;
            }
            // SCTP SETTING
            X2AP_REGISTER_ENB_REQ (msg_p).sctp_out_streams = SCTP_OUT_STREAMS;
            X2AP_REGISTER_ENB_REQ (msg_p).sctp_in_streams  = SCTP_IN_STREAMS;

            if (EPC_MODE_ENABLED) {
              sprintf(aprefix,"%s.[%i].%s",ENB_CONFIG_STRING_ENB_LIST,k,ENB_CONFIG_STRING_SCTP_CONFIG);
              config_get( SCTPParams,sizeof(SCTPParams)/sizeof(paramdef_t),aprefix);
              X2AP_REGISTER_ENB_REQ (msg_p).sctp_in_streams = (uint16_t)*(SCTPParams[ENB_SCTP_INSTREAMS_IDX].uptr);
              X2AP_REGISTER_ENB_REQ (msg_p).sctp_out_streams = (uint16_t)*(SCTPParams[ENB_SCTP_OUTSTREAMS_IDX].uptr);
            }

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
  paramdef_t ThreadParams[]  = THREAD_CONF_DESC;
  paramlist_def_t THREADParamList = {THREAD_CONFIG_STRING_THREAD_STRUCT,NULL,0};
  config_getlist( &THREADParamList,NULL,0,NULL);

  if(parallel_config == NULL) {
    if(THREADParamList.numelt>0) {
      config_getlist( &THREADParamList,ThreadParams,sizeof(ThreadParams)/sizeof(paramdef_t),NULL);
      parallel_conf = strdup(*(THREADParamList.paramarray[0][THREAD_PARALLEL_IDX].strptr));
    } else {
      parallel_conf = strdup("PARALLEL_RU_L1_TRX_SPLIT");
    }

    set_parallel_conf(parallel_conf);
  }

  if(worker_config == NULL) {
    if(THREADParamList.numelt>0) {
      config_getlist( &THREADParamList,ThreadParams,sizeof(ThreadParams)/sizeof(paramdef_t),NULL);
      worker_conf   = strdup(*(THREADParamList.paramarray[0][THREAD_WORKER_IDX].strptr));
    } else {
      worker_conf   = strdup("WORKER_ENABLE");
    }

    set_worker_conf(worker_conf);
  }

  free(worker_conf);
  free(parallel_conf);
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
  //EPC_MODE_ENABLED = ((*ENBSParams[ENB_NOS1_IDX].uptr) == 0);
  RC.nb_inst = ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt;

  if (RC.nb_inst > 0) {
    RC.nb_CC = (int *)malloc((1+RC.nb_inst)*sizeof(int));

    for (int i=0; i<RC.nb_inst; i++) {
      sprintf(aprefix,"%s.[%i]",ENB_CONFIG_STRING_ENB_LIST,i);
      config_getlist( &CCsParamList,NULL,0, aprefix);
      RC.nb_CC[i]    = CCsParamList.numelt;
    }
  }

  config_getlist( &MACRLCParamList,NULL,0, NULL);
  RC.nb_macrlc_inst  = MACRLCParamList.numelt;
  AssertFatal(RC.nb_macrlc_inst <= MAX_MAC_INST,
              "Too many macrlc instances %d\n",RC.nb_macrlc_inst);
  // Get num L1 instances
  config_getlist( &L1ParamList,NULL,0, NULL);
  RC.nb_L1_inst = L1ParamList.numelt;
  // Get num RU instances
  config_getlist( &RUParamList,NULL,0, NULL);
  RC.nb_RU     = RUParamList.numelt;
  RCconfig_parallel();
}

int check_plmn_identity(rrc_eNB_carrier_data_t *carrier,uint16_t mcc,uint16_t mnc,uint8_t mnc_digit_length) {
  AssertFatal(carrier->sib1->cellAccessRelatedInfo.plmn_IdentityList.list.count > 0,
              "plmn info isn't there\n");
  AssertFatal(mnc_digit_length ==2 || mnc_digit_length == 3,
              "impossible mnc_digit_length %d\n",mnc_digit_length);
  LTE_PLMN_IdentityInfo_t *plmn_Identity_info = carrier->sib1->cellAccessRelatedInfo.plmn_IdentityList.list.array[0];

  // check if mcc is different and return failure if so
  if (mcc !=
      (*plmn_Identity_info->plmn_Identity.mcc->list.array[0]*100)+
      (*plmn_Identity_info->plmn_Identity.mcc->list.array[1]*10) +
      (*plmn_Identity_info->plmn_Identity.mcc->list.array[2])) return(0);

  // check that mnc digit length is different and return failure if so
  if (mnc_digit_length != plmn_Identity_info->plmn_Identity.mnc.list.count) return 0;

  // check that 2 digit mnc is different and return failure if so
  if (mnc_digit_length == 2 &&
      (mnc !=
       (*plmn_Identity_info->plmn_Identity.mnc.list.array[0]*10) +
       (*plmn_Identity_info->plmn_Identity.mnc.list.array[1]))) return(0);
  else if (mnc_digit_length == 3 &&
           (mnc !=
            (*plmn_Identity_info->plmn_Identity.mnc.list.array[0]*100) +
            (*plmn_Identity_info->plmn_Identity.mnc.list.array[1]*10) +
            (*plmn_Identity_info->plmn_Identity.mnc.list.array[2]))) return(0);

  // if we're here, the mcc/mnc match so return success
  return(1);
}

void extract_and_decode_SI(int inst,int si_ind,uint8_t *si_container,int si_container_length) {
  eNB_RRC_INST *rrc = RC.rrc[inst];
  rrc_eNB_carrier_data_t *carrier = &rrc->carrier[0];
  LTE_BCCH_DL_SCH_Message_t *bcch_message ;
  AssertFatal(si_ind==2,"Can only handle a single SI block for now\n");
  LOG_I(ENB_APP, "rrc inst %d: Trying to decode SI block %d @ %p, length %d\n",inst,si_ind,si_container,si_container_length);
  // point to first SI block
  bcch_message = &carrier->systemInformation;
  asn_dec_rval_t dec_rval = uper_decode_complete( NULL,
                            &asn_DEF_LTE_BCCH_DL_SCH_Message,
                            (void **)&bcch_message,
                            (const void *)si_container,
                            si_container_length);

  if ((dec_rval.code != RC_OK) && (dec_rval.consumed == 0)) {
    AssertFatal(1==0, "[ENB_APP][RRC inst %"PRIu8"] Failed to decode BCCH_DLSCH_MESSAGE (%zu bits)\n",
                inst,
                dec_rval.consumed );
  }

  if (bcch_message->message.present == LTE_BCCH_DL_SCH_MessageType_PR_c1) {
    switch (bcch_message->message.choice.c1.present) {
      case LTE_BCCH_DL_SCH_MessageType__c1_PR_systemInformationBlockType1:
        AssertFatal(1==0,"Should have received SIB1 from CU\n");
        break;

      case LTE_BCCH_DL_SCH_MessageType__c1_PR_systemInformation: {
        LTE_SystemInformation_t *si = &bcch_message->message.choice.c1.choice.systemInformation;

        for (int i=0; i<si->criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list.count; i++) {
          LOG_I(ENB_APP,"Extracting SI %d/%d\n",i,si->criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list.count);
          struct LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member *typeandinfo;
          typeandinfo = si->criticalExtensions.choice.systemInformation_r8.sib_TypeAndInfo.list.array[i];

          switch(typeandinfo->present) {
            case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib2:
              carrier->sib2 = &typeandinfo->choice.sib2;
              carrier->SIB23 = (uint8_t *)malloc(64);
              memcpy((void *)carrier->SIB23,(void *)si_container,si_container_length);
              carrier->sizeof_SIB23 = si_container_length;
              LOG_I( ENB_APP, "[RRC %"PRIu8"] Found SIB2 in CU F1AP_SETUP_RESP message\n", inst);
              break;

            case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib3:
              carrier->sib3 = &typeandinfo->choice.sib3;
              LOG_I( ENB_APP, "[RRC %"PRIu8"] Found SIB3 in CU F1AP_SETUP_RESP message\n", inst);
              break;

            case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib4:
              //carrier->sib4 = &typeandinfo->choice.sib4;
              LOG_I( ENB_APP, "[RRC %"PRIu8"] Found SIB4 in CU F1AP_SETUP_RESP message\n", inst);
              break;

            case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib5:
              //carrier->sib5 = &typeandinfo->choice.sib5;
              LOG_I( ENB_APP, "[RRC %"PRIu8"] Found SIB5 in CU F1AP_SETUP_RESP message\n", inst);
              break;

            case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib6:
              //carrier->sib6 = &typeandinfo->choice.sib6;
              LOG_I( ENB_APP, "[RRC %"PRIu8"] Found SIB6 in CU F1AP_SETUP_RESP message\n", inst);
              break;

            case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib7:
              //carrier->sib7 = &typeandinfo->choice.sib7;
              LOG_I( ENB_APP, "[RRC %"PRIu8"] Found SIB7 in CU F1AP_SETUP_RESP message\n", inst);
              break;

            case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib8:
              //carrier->sib8 = &typeandinfo->choice.sib8;
              LOG_I( ENB_APP, "[RRC %"PRIu8"] Found SIB8 in CU F1AP_SETUP_RESP message\n", inst);
              break;

            case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib9:
              //carrier->sib9 = &typeandinfo->choice.sib9;
              LOG_I( ENB_APP, "[RRC %"PRIu8"] Found SIB9 in CU F1AP_SETUP_RESP message\n", inst);
              break;

            case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib10:
              //carrier->sib10 = &typeandinfo->choice.sib10;
              LOG_I( ENB_APP, "[RRC %"PRIu8"] Found SIB10 in CU F1AP_SETUP_RESP message\n", inst);
              break;

            case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib11:
              //carrier->sib11 = &typeandinfo->choice.sib11;
              LOG_I( ENB_APP, "[RRC %"PRIu8"] Found SIB11 in CU F1AP_SETUP_RESP message\n", inst);
              break;

            case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib12_v920:
              //carrier->sib12 = &typeandinfo->choice.sib12;
              LOG_I( ENB_APP, "[RRC %"PRIu8"] Found SIB12 in CU F1AP_SETUP_RESP message\n", inst);
              break;

            case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib13_v920:
              carrier->sib13 = &typeandinfo->choice.sib13_v920;
              LOG_I( ENB_APP, "[RRC %"PRIu8"] Found SIB13 in CU F1AP_SETUP_RESP message\n", inst);
              break;

            //SIB18
            case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib18_v1250:
              carrier->sib18 = &typeandinfo->choice.sib18_v1250;
              LOG_I( ENB_APP, "[RRC %"PRIu8"] Found SIB18 in CU F1AP_SETUP_RESP message\n", inst);
              break;

            //SIB19
            case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib19_v1250:
              carrier->sib19 = &typeandinfo->choice.sib19_v1250;
              LOG_I( ENB_APP, "[RRC %"PRIu8"] Found SIB19 in CU F1AP_SETUP_RESP message\n", inst);
              break;

            //SIB21
            case LTE_SystemInformation_r8_IEs__sib_TypeAndInfo__Member_PR_sib21_v1430:
              carrier->sib21 = &typeandinfo->choice.sib21_v1430;
              LOG_I( ENB_APP, "[RRC %"PRIu8"] Found SIB21 in CU F1AP_SETUP_RESP message\n", inst);
              break;

            default:
              AssertFatal(1==0,"Shouldn't have received this SI %d\n",typeandinfo->present);
              break;
          }
        }

        break;
      }

      case LTE_BCCH_DL_SCH_MessageType__c1_PR_NOTHING:
        AssertFatal(0, "Should have received SIB1 from CU\n");
        break;
    }
  } else AssertFatal(1==0,"No SI messages\n");
}

void configure_du_mac(int inst) {
  eNB_RRC_INST *rrc = RC.rrc[inst];
  rrc_eNB_carrier_data_t *carrier = &rrc->carrier[0];
  LOG_I(ENB_APP,"Configuring MAC/L1 %d, carrier->sib2 %p\n",inst,&carrier->sib2->radioResourceConfigCommon);
  rrc_mac_config_req_eNB(inst, 0,
                         carrier->physCellId,
                         carrier->p_eNB,
                         carrier->Ncp,
                         carrier->sib1->freqBandIndicator,
                         carrier->dl_CarrierFreq,
                         carrier->pbch_repetition,
                         0, // rnti
                         (LTE_BCCH_BCH_Message_t *) &carrier->mib,
                         (LTE_RadioResourceConfigCommonSIB_t *) &carrier->sib2->radioResourceConfigCommon,
                         (LTE_RadioResourceConfigCommonSIB_t *) &carrier->sib2_BR->radioResourceConfigCommon,
                         (struct LTE_PhysicalConfigDedicated *)NULL,
                         (LTE_SCellToAddMod_r10_t *)NULL,
                         //(struct PhysicalConfigDedicatedSCell_r10 *)NULL,
                         (LTE_MeasObjectToAddMod_t **) NULL,
                         (LTE_MAC_MainConfig_t *) NULL, 0,
                         (struct LTE_LogicalChannelConfig *)NULL,
                         (LTE_MeasGapConfig_t *) NULL,
                         carrier->sib1->tdd_Config,
                         NULL,
                         &carrier->sib1->schedulingInfoList,
                         carrier->ul_CarrierFreq,
                         carrier->sib2->freqInfo.ul_Bandwidth,
                         &carrier->sib2->freqInfo.additionalSpectrumEmission,
                         (LTE_MBSFN_SubframeConfigList_t *) carrier->sib2->mbsfn_SubframeConfigList,
                         carrier->MBMS_flag,
                         (LTE_MBSFN_AreaInfoList_r9_t *) & carrier->sib13->mbsfn_AreaInfoList_r9,
                         (LTE_PMCH_InfoList_r9_t *) NULL,
                         NULL,
                         0,
                         (LTE_BCCH_DL_SCH_Message_MBMS_t *) NULL,
                         (LTE_SchedulingInfo_MBMS_r14_t *) NULL,
                         (struct LTE_NonMBSFN_SubframeConfig_r14 *) NULL,
                         (LTE_SystemInformationBlockType1_MBMS_r14_t *) NULL,
                         (LTE_MBSFN_AreaInfoList_r9_t *) NULL,
			 (LTE_MBSFNAreaConfiguration_r9_t*) NULL
                        );
}

void handle_f1ap_setup_resp(f1ap_setup_resp_t *resp) {
  int i,j,si_ind;
  LOG_I(ENB_APP, "cells_to_activated %d, RRC instances %d\n",
        resp->num_cells_to_activate,RC.nb_inst);

  for (j=0; j<resp->num_cells_to_activate; j++) {
    for (i=0; i<RC.nb_inst; i++) {
      rrc_eNB_carrier_data_t *carrier =  &RC.rrc[i]->carrier[0];
      // identify local index of cell j by nr_cellid, plmn identity and physical cell ID
      LOG_I(ENB_APP, "Checking cell %d, rrc inst %d : rrc->nr_cellid %lx, resp->nr_cellid %lx\n",
            j,i,RC.rrc[i]->nr_cellid,resp->cells_to_activate[j].nr_cellid);

      if (RC.rrc[i]->nr_cellid == resp->cells_to_activate[j].nr_cellid &&
          (check_plmn_identity(carrier, resp->cells_to_activate[j].mcc, resp->cells_to_activate[j].mnc, resp->cells_to_activate[j].mnc_digit_length)>0 &&
           resp->cells_to_activate[j].nrpci == carrier->physCellId)) {
        // copy system information and decode it
        for (si_ind=2; si_ind<resp->cells_to_activate[j].num_SI + 2; si_ind++)  {
          //printf("SI %d size %d: ", si_ind, resp->cells_to_activate[j].SI_container_length[si_ind]);
          //for (int n=0;n<resp->cells_to_activate[j].SI_container_length[si_ind];n++)
          //  printf("%02x ",resp->cells_to_activate[j].SI_container[si_ind][n]);
          //printf("\n");
          if (si_ind==6) si_ind=9;
          if (resp->cells_to_activate[j].SI_container[si_ind] != NULL) {
            extract_and_decode_SI(i,
                                  si_ind,
                                  resp->cells_to_activate[j].SI_container[si_ind],
                                  resp->cells_to_activate[j].SI_container_length[si_ind]);
          }

        }

        // perform MAC/L1 common configuration
        configure_du_mac(i);
      } else {
        LOG_E(ENB_APP, "F1 Setup Response not matching\n");
      }
    }
  }
}

void read_config_and_init(void) {
  int macrlc_has_f1[MAX_MAC_INST];
  memset(macrlc_has_f1, 0, MAX_MAC_INST*sizeof(int));

  if (RC.nb_macrlc_inst > 0)
    AssertFatal(RC.nb_macrlc_inst == RC.nb_inst,
                "Number of MACRLC instances %d != number of RRC instances %d\n",
                RC.nb_macrlc_inst, RC.nb_inst);

  RCconfig_L1();
  LOG_I(PHY, "%s() RC.nb_L1_inst: %d\n", __FUNCTION__, RC.nb_L1_inst);
  RCconfig_macrlc(macrlc_has_f1);
  LOG_I(MAC, "%s() RC.nb_macrlc_inst: %d\n", __FUNCTION__, RC.nb_macrlc_inst);

  if (RC.nb_L1_inst > 0)
    AssertFatal(l1_north_init_eNB() == 0, "could not initialize L1 north interface\n");

  RC.rrc = malloc(RC.nb_inst * sizeof(eNB_RRC_INST *));
  AssertFatal(RC.rrc, "could not allocate memory for RC.rrc\n");

  for (uint32_t enb_id = 0; enb_id < RC.nb_inst; enb_id++) {
    RC.rrc[enb_id] = malloc(sizeof(eNB_RRC_INST));
    AssertFatal(RC.rrc[enb_id], "RRC context for eNB %u not allocated\n", enb_id);
    memset((void *)RC.rrc[enb_id], 0, sizeof(eNB_RRC_INST));
    RCconfig_RRC(enb_id, RC.rrc[enb_id],macrlc_has_f1[enb_id]);
  }

  RCconfig_flexran();
}
