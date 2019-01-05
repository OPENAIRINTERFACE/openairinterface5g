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
#include "LTE_SystemInformationBlockType2.h"
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

#define RRC_INACTIVITY_THRESH 0

extern uint16_t sf_ahead;
extern void set_parallel_conf(char *parallel_conf);
extern void set_worker_conf(char *worker_conf);
extern PARALLEL_CONF_t get_thread_parallel_conf(void);
extern WORKER_CONF_t   get_thread_worker_conf(void);
extern uint32_t to_earfcn_DL(int eutra_bandP, uint32_t dl_CarrierFreq, uint32_t bw);
extern uint32_t to_earfcn_UL(int eutra_bandP, uint32_t ul_CarrierFreq, uint32_t bw);
extern char *parallel_config;
extern char *worker_config;

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
  int32_t     srb1_timer_poll_retransmit    = 0;
  int32_t     srb1_timer_reordering         = 0;
  int32_t     srb1_timer_status_prohibit    = 0;
  int32_t     srb1_poll_pdu                 = 0;
  int32_t     srb1_poll_byte                = 0;
  int32_t     srb1_max_retx_threshold       = 0;
  */


  /* get number of eNBs */
  paramdef_t ENBSParams[] = ENBSPARAMS_DESC;
  config_get(ENBSParams, sizeof(ENBSParams)/sizeof(paramdef_t), NULL);
  num_enbs = ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt;
  /* for eNB ID */
  paramdef_t ENBParams[]  = ENBPARAMS_DESC;
  paramlist_def_t ENBParamList = {ENB_CONFIG_STRING_ENB_LIST, NULL, 0};
  /* for Nid_cell */
  checkedparam_t config_check_CCparams[] = CCPARAMS_CHECK;
  ccparams_lte_t ccparams_lte;
  memset((void*)&ccparams_lte,0,sizeof(ccparams_lte_t));
  paramdef_t CCsParams[] = CCPARAMS_DESC(ccparams_lte);
  paramlist_def_t CCsParamList = {ENB_CONFIG_STRING_COMPONENT_CARRIERS, NULL, 0};

  // Note: these should be turned on for EMTC support inside of FlexRAN
  // ccparams_emtc_t ccparams_emtc;
  //paramdef_t brParams[]              = BRPARAMS_DESC(ccparams_emtc);
  //paramdef_t schedulingInfoBrParams[] = SI_INFO_BR_DESC(ccparams_emtc);
  //paramlist_def_t schedulingInfoBrParamList = {ENB_CONFIG_STRING_SCHEDULING_INFO_BR, NULL, 0};
  //paramdef_t rachcelevelParams[]     = RACH_CE_LEVELINFOLIST_R13_DESC(ccparams_emtc);
  //paramlist_def_t rachcelevellist    = {ENB_CONFIG_STRING_RACH_CE_LEVELINFOLIST_R13, NULL, 0};
  //paramdef_t rsrprangeParams[]       = RSRP_RANGE_LIST_DESC(ccparams_emtc);
  //paramlist_def_t rsrprangelist      = {ENB_CONFIG_STRING_RSRP_RANGE_LIST, NULL, 0};
  //paramdef_t prachParams[]           = PRACH_PARAMS_CE_R13_DESC(ccparams_emtc);
  //paramlist_def_t prachParamslist    = {ENB_CONFIG_STRING_PRACH_PARAMETERS_CE_R13, NULL, 0};
  //paramdef_t n1PUCCH_ANR13Params[]   = N1PUCCH_AN_INFOLIST_R13_DESC(ccparams_emtc);
  //paramlist_def_t n1PUCCHInfoList    = {ENB_CONFIG_STRING_N1PUCCH_AN_INFOLIST_R13, NULL, 0};
  //paramdef_t pcchv1310Params[]       = PCCH_CONFIG_V1310_DESC(ccparams_emtc);
  //paramdef_t sib2freqhoppingParams[] = SIB2_FREQ_HOPPING_R13_DESC(ccparams_emtc);

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
    }// j=0..num_inst
  } else {// MacRLC_ParamList.numelt > 0
    AssertFatal (0,
                 "No " CONFIG_STRING_MACRLC_LIST " configuration found");
  }
}

int RCconfig_RRC(MessageDef *msg_p, uint32_t i, eNB_RRC_INST *rrc) {

  int               num_enbs                      = 0;
  int               j,k                           = 0;
  int32_t           enb_id                        = 0;
  int               nb_cc                         = 0;
  ccparams_lte_t ccparams_lte;
  ccparams_sidelink_t SLconfig;
  ccparams_eMTC_t eMTCconfig;

  memset((void*)&ccparams_lte,0,sizeof(ccparams_lte_t));
  memset((void*)&SLconfig,0,sizeof(ccparams_sidelink_t));
  memset((void*)&eMTCconfig,0,sizeof(ccparams_eMTC_t));


  paramdef_t ENBSParams[] = ENBSPARAMS_DESC;
  paramdef_t ENBParams[]  = ENBPARAMS_DESC;
  paramlist_def_t ENBParamList = {ENB_CONFIG_STRING_ENB_LIST,NULL,0};
  checkedparam_t config_check_CCparams[] = CCPARAMS_CHECK;
  paramdef_t CCsParams[] = CCPARAMS_DESC(ccparams_lte);
  paramlist_def_t CCsParamList = {ENB_CONFIG_STRING_COMPONENT_CARRIERS,NULL,0};

  paramdef_t eMTCParams[]              = EMTCPARAMS_DESC((&eMTCconfig));
  checkedparam_t config_check_eMTCparams[] = EMTCPARAMS_CHECK;

  srb1_params_t srb1_params;
  memset((void*)&srb1_params,0,sizeof(srb1_params_t));
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

    LOG_I(ENB_APP,"RRC %d: Southbound Transport %s\n",i,*(ENBParamList.paramarray[i][ENB_TRANSPORT_S_PREFERENCE_IDX].strptr));

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
      if (strcmp(ENBSParams[ENB_ACTIVE_ENBS_IDX].strlistptr[k], *(ENBParamList.paramarray[i][ENB_ENB_NAME_IDX].strptr)) == 0) {
        char enbpath[MAX_OPTNAME_SIZE + 8];
        sprintf(enbpath,"%s.[%i]",ENB_CONFIG_STRING_ENB_LIST,k);
        paramdef_t PLMNParams[] = PLMNPARAMS_DESC;
        paramlist_def_t PLMNParamList = {ENB_CONFIG_STRING_PLMN_LIST, NULL, 0};
        /* map parameter checking array instances to parameter definition array instances */
        checkedparam_t config_check_PLMNParams [] = PLMNPARAMS_CHECK;

        for (int I = 0; I < sizeof(PLMNParams) / sizeof(paramdef_t); ++I)
          PLMNParams[I].chkPptr = &(config_check_PLMNParams[I]);

        RRC_CONFIGURATION_REQ (msg_p).rrc_inactivity_timer_thres = RRC_INACTIVITY_THRESH; // set to 0 to deactivate
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

            RRC_CONFIGURATION_REQ (msg_p).tdd_config[j] = ccparams_lte.tdd_config;
            AssertFatal (ccparams_lte.tdd_config <= LTE_TDD_Config__subframeAssignment_sa6,
                         "Failed to parse eNB configuration file %s, enb %d illegal tdd_config %d (should be 0-%d)!",
                         RC.config_file_name, i, ccparams_lte.tdd_config, LTE_TDD_Config__subframeAssignment_sa6);
            RRC_CONFIGURATION_REQ (msg_p).tdd_config_s[j] = ccparams_lte.tdd_config_s;
            AssertFatal (ccparams_lte.tdd_config_s <= LTE_TDD_Config__specialSubframePatterns_ssp8,
                         "Failed to parse eNB configuration file %s, enb %d illegal tdd_config_s %d (should be 0-%d)!",
                         RC.config_file_name, i, ccparams_lte.tdd_config_s, LTE_TDD_Config__specialSubframePatterns_ssp8);
            if (!ccparams_lte.prefix_type)
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d define %s: NORMAL,EXTENDED!\n",
                           RC.config_file_name, i, ENB_CONFIG_STRING_PREFIX_TYPE);
            else if (strcmp(ccparams_lte.prefix_type, "NORMAL") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).prefix_type[j] = NORMAL;
            } else  if (strcmp(ccparams_lte.prefix_type, "EXTENDED") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).prefix_type[j] = EXTENDED;
            } else {
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for prefix_type choice: NORMAL or EXTENDED !\n",
                           RC.config_file_name, i, ccparams_lte.prefix_type);
            }

#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))

            if (!ccparams_lte.pbch_repetition)
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d define %s: TRUE,FALSE!\n",
                           RC.config_file_name, i, ENB_CONFIG_STRING_PBCH_REPETITION);
            else if (strcmp(ccparams_lte.pbch_repetition, "TRUE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).pbch_repetition[j] = 1;
            } else  if (strcmp(ccparams_lte.pbch_repetition, "FALSE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).pbch_repetition[j] = 0;
            } else {
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pbch_repetition choice: TRUE or FALSE !\n",
                           RC.config_file_name, i, ccparams_lte.pbch_repetition);
            }

#endif
            RRC_CONFIGURATION_REQ (msg_p).eutra_band[j] = ccparams_lte.eutra_band;
            RRC_CONFIGURATION_REQ (msg_p).downlink_frequency[j] = (uint32_t) ccparams_lte.downlink_frequency;
            RRC_CONFIGURATION_REQ (msg_p).uplink_frequency_offset[j] = (unsigned int) ccparams_lte.uplink_frequency_offset;


            RRC_CONFIGURATION_REQ (msg_p).Nid_cell[j]= ccparams_lte.Nid_cell;

            if (ccparams_lte.Nid_cell>503) {
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for Nid_cell choice: 0...503 !\n",
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
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for N_RB_DL choice: 6,15,25,50,75,100 !\n",
                           RC.config_file_name, i, ccparams_lte.N_RB_DL);
            }

            if (strcmp(ccparams_lte.frame_type, "FDD") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).frame_type[j] = FDD;
            } else  if (strcmp(ccparams_lte.frame_type, "TDD") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).frame_type[j] = TDD;
            } else {
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for frame_type choice: FDD or TDD !\n",
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
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for nb_antenna_ports choice: 1..2 !\n",
                           RC.config_file_name, i, ccparams_lte.nb_antenna_ports);

            RRC_CONFIGURATION_REQ (msg_p).nb_antenna_ports[j] = ccparams_lte.nb_antenna_ports;

	    // Radio Resource Configuration (SIB2)

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_root =  ccparams_lte.prach_root;

            if ((ccparams_lte.prach_root <0) || (ccparams_lte.prach_root > 1023))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_root choice: 0..1023 !\n",
                           RC.config_file_name, i, ccparams_lte.prach_root);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_config_index = ccparams_lte.prach_config_index;

            if ((ccparams_lte.prach_config_index <0) || (ccparams_lte.prach_config_index > 63))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_config_index choice: 0..1023 !\n",
                           RC.config_file_name, i, ccparams_lte.prach_config_index);

            if (!ccparams_lte.prach_high_speed)
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                           RC.config_file_name, i, ENB_CONFIG_STRING_PRACH_HIGH_SPEED);
            else if (strcmp(ccparams_lte.prach_high_speed, "ENABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_high_speed = TRUE;
            } else if (strcmp(ccparams_lte.prach_high_speed, "DISABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_high_speed = FALSE;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for prach_config choice: ENABLE,DISABLE !\n",
                           RC.config_file_name, i, ccparams_lte.prach_high_speed);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_zero_correlation = ccparams_lte.prach_zero_correlation;

            if ((ccparams_lte.prach_zero_correlation <0) || 
		(ccparams_lte.prach_zero_correlation > 15))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_zero_correlation choice: 0..15!\n",
                           RC.config_file_name, i, ccparams_lte.prach_zero_correlation);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].prach_freq_offset = ccparams_lte.prach_freq_offset;

            if ((ccparams_lte.prach_freq_offset <0) || 
		(ccparams_lte.prach_freq_offset > 94))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for prach_freq_offset choice: 0..94!\n",
                           RC.config_file_name, i, ccparams_lte.prach_freq_offset);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_delta_shift = ccparams_lte.pucch_delta_shift-1;

            if ((ccparams_lte.pucch_delta_shift <1) || 
		(ccparams_lte.pucch_delta_shift > 3))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_delta_shift choice: 1..3!\n",
                           RC.config_file_name, i, ccparams_lte.pucch_delta_shift);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_nRB_CQI = ccparams_lte.pucch_nRB_CQI;

            if ((ccparams_lte.pucch_nRB_CQI <0) || 
		(ccparams_lte.pucch_nRB_CQI > 98))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_nRB_CQI choice: 0..98!\n",
                           RC.config_file_name, i, ccparams_lte.pucch_nRB_CQI);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_nCS_AN = ccparams_lte.pucch_nCS_AN;

            if ((ccparams_lte.pucch_nCS_AN <0) || 
		(ccparams_lte.pucch_nCS_AN > 7))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_nCS_AN choice: 0..7!\n",
                           RC.config_file_name, i, ccparams_lte.pucch_nCS_AN);


            //#if (LTE_RRC_VERSION < MAKE_VERSION(10, 0, 0))
            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_n1_AN = ccparams_lte.pucch_n1_AN;

            if ((ccparams_lte.pucch_n1_AN <0) || 
		(ccparams_lte.pucch_n1_AN > 2047))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_n1_AN choice: 0..2047!\n",
                           RC.config_file_name, i, ccparams_lte.pucch_n1_AN);

            //#endif
            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pdsch_referenceSignalPower = ccparams_lte.pdsch_referenceSignalPower;

            if ((ccparams_lte.pdsch_referenceSignalPower <-60) || 
		(ccparams_lte.pdsch_referenceSignalPower > 50))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pdsch_referenceSignalPower choice:-60..50!\n",
                           RC.config_file_name, i, ccparams_lte.pdsch_referenceSignalPower);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pdsch_p_b = ccparams_lte.pdsch_p_b;

            if ((ccparams_lte.pdsch_p_b <0) || 
		(ccparams_lte.pdsch_p_b > 3))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pdsch_p_b choice: 0..3!\n",
                           RC.config_file_name, i, ccparams_lte.pdsch_p_b);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_n_SB = ccparams_lte.pusch_n_SB;

            if ((ccparams_lte.pusch_n_SB <1) || 
		(ccparams_lte.pusch_n_SB > 4))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_n_SB choice: 1..4!\n",
                           RC.config_file_name, i, ccparams_lte.pusch_n_SB);

            if (!ccparams_lte.pusch_hoppingMode)
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d define %s: interSubframe,intraAndInterSubframe!\n",
                           RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_HOPPINGMODE);

            else if (strcmp(ccparams_lte.pusch_hoppingMode,"interSubFrame")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_hoppingMode = LTE_PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_interSubFrame;
            }  
	    else if (strcmp(ccparams_lte.pusch_hoppingMode,"intraAndInterSubFrame")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_hoppingMode = LTE_PUSCH_ConfigCommon__pusch_ConfigBasic__hoppingMode_intraAndInterSubFrame;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_hoppingMode choice: interSubframe,intraAndInterSubframe!\n",
                           RC.config_file_name, i, ccparams_lte.pusch_hoppingMode);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_hoppingOffset = ccparams_lte.pusch_hoppingOffset;

            if ((ccparams_lte.pusch_hoppingOffset<0) || 
		(ccparams_lte.pusch_hoppingOffset>98))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_hoppingOffset choice: 0..98!\n",
                           RC.config_file_name, i, ccparams_lte.pusch_hoppingMode);

            if (!ccparams_lte.pusch_enable64QAM)
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                           RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_ENABLE64QAM);
            else if (strcmp(ccparams_lte.pusch_enable64QAM, "ENABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_enable64QAM = TRUE;
            }  
	    else if (strcmp(ccparams_lte.pusch_enable64QAM, "DISABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_enable64QAM = FALSE;
            } 
	    else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_enable64QAM choice: ENABLE,DISABLE!\n",
                           RC.config_file_name, i, ccparams_lte.pusch_enable64QAM);

            if (!ccparams_lte.pusch_groupHoppingEnabled)
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                           RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_GROUP_HOPPING_EN);
            else if (strcmp(ccparams_lte.pusch_groupHoppingEnabled, "ENABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_groupHoppingEnabled = TRUE;
            }  
	    else if (strcmp(ccparams_lte.pusch_groupHoppingEnabled, "DISABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_groupHoppingEnabled= FALSE;
            } 
	    else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_groupHoppingEnabled choice: ENABLE,DISABLE!\n",
                           RC.config_file_name, i, ccparams_lte.pusch_groupHoppingEnabled);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_groupAssignment = ccparams_lte.pusch_groupAssignment;

            if ((ccparams_lte.pusch_groupAssignment<0)||
		(ccparams_lte.pusch_groupAssignment>29))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_groupAssignment choice: 0..29!\n",
                           RC.config_file_name, i, ccparams_lte.pusch_groupAssignment);

            if (!ccparams_lte.pusch_sequenceHoppingEnabled)
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d define %s: ENABLE,DISABLE!\n",
                           RC.config_file_name, i, ENB_CONFIG_STRING_PUSCH_SEQUENCE_HOPPING_EN);
            else if (strcmp(ccparams_lte.pusch_sequenceHoppingEnabled, "ENABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_sequenceHoppingEnabled = TRUE;
            }  
	    else if (strcmp(ccparams_lte.pusch_sequenceHoppingEnabled, "DISABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_sequenceHoppingEnabled = FALSE;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pusch_sequenceHoppingEnabled choice: ENABLE,DISABLE!\n",
                           RC.config_file_name, i, ccparams_lte.pusch_sequenceHoppingEnabled);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_nDMRS1= ccparams_lte.pusch_nDMRS1;  //cyclic_shift in RRC!

            if ((ccparams_lte.pusch_nDMRS1 <0) || 
		(ccparams_lte.pusch_nDMRS1>7))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_nDMRS1 choice: 0..7!\n",
                           RC.config_file_name, i, ccparams_lte.pusch_nDMRS1);


            if (strcmp(ccparams_lte.phich_duration,"NORMAL")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_duration= LTE_PHICH_Config__phich_Duration_normal;
            } else if (strcmp(ccparams_lte.phich_duration,"EXTENDED")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_duration= LTE_PHICH_Config__phich_Duration_extended;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for phich_duration choice: NORMAL,EXTENDED!\n",
                           RC.config_file_name, i, ccparams_lte.phich_duration);

            if (strcmp(ccparams_lte.phich_resource,"ONESIXTH")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_resource= LTE_PHICH_Config__phich_Resource_oneSixth ;
            } 
	    else if (strcmp(ccparams_lte.phich_resource,"HALF")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_resource= LTE_PHICH_Config__phich_Resource_half;
            } 
	    else if (strcmp(ccparams_lte.phich_resource,"ONE")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_resource= LTE_PHICH_Config__phich_Resource_one;
            } 
	    else if (strcmp(ccparams_lte.phich_resource,"TWO")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_resource= LTE_PHICH_Config__phich_Resource_two;
            } 
	    else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for phich_resource choice: ONESIXTH,HALF,ONE,TWO!\n",
                           RC.config_file_name, i, ccparams_lte.phich_resource);

            printf("phich.resource %ld (%s), phich.duration %ld (%s)\n",
                   RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_resource,ccparams_lte.phich_resource,
                   RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].phich_duration,ccparams_lte.phich_duration);

            if (strcmp(ccparams_lte.srs_enable, "ENABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_enable= TRUE;
            } 
	    else if (strcmp(ccparams_lte.srs_enable, "DISABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_enable= FALSE;
            } 
	    else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_BandwidthConfig choice: ENABLE,DISABLE !\n",
                           RC.config_file_name, i, ccparams_lte.srs_enable);

            if (RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_enable== TRUE) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_BandwidthConfig= ccparams_lte.srs_BandwidthConfig;

              if ((ccparams_lte.srs_BandwidthConfig < 0) || 
		  (ccparams_lte.srs_BandwidthConfig >7))
                AssertFatal (0, "Failed to parse eNB configuration file %s, enb %d unknown value %d for srs_BandwidthConfig choice: 0...7\n",
                             RC.config_file_name, i, ccparams_lte.srs_BandwidthConfig);

              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_SubframeConfig= ccparams_lte.srs_SubframeConfig;

              if ((ccparams_lte.srs_SubframeConfig<0) || 
		  (ccparams_lte.srs_SubframeConfig>15))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for srs_SubframeConfig choice: 0..15 !\n",
                             RC.config_file_name, i, ccparams_lte.srs_SubframeConfig);

              if (strcmp(ccparams_lte.srs_ackNackST, "ENABLE") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_ackNackST= TRUE;
              } 
	      else if (strcmp(ccparams_lte.srs_ackNackST, "DISABLE") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_ackNackST= FALSE;
              } else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_BandwidthConfig choice: ENABLE,DISABLE !\n",
                             RC.config_file_name, i, ccparams_lte.srs_ackNackST);

              if (strcmp(ccparams_lte.srs_MaxUpPts, "ENABLE") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_MaxUpPts= TRUE;
              } 
	      else if (strcmp(ccparams_lte.srs_MaxUpPts, "DISABLE") == 0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].srs_MaxUpPts= FALSE;
              } 
	      else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for srs_MaxUpPts choice: ENABLE,DISABLE !\n",
                             RC.config_file_name, i, ccparams_lte.srs_MaxUpPts);
            }

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_p0_Nominal= ccparams_lte.pusch_p0_Nominal;

            if ((ccparams_lte.pusch_p0_Nominal<-126) || 
		(ccparams_lte.pusch_p0_Nominal>24))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pusch_p0_Nominal choice: -126..24 !\n",
                           RC.config_file_name, i, ccparams_lte.pusch_p0_Nominal);

#if (LTE_RRC_VERSION <= MAKE_VERSION(12, 0, 0))

            if (strcmp(ccparams_lte.pusch_alpha,"AL0")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_UplinkPowerControlCommon__alpha_al0;
            } 
	    else if (strcmp(ccparams_lte.pusch_alpha,"AL04")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_UplinkPowerControlCommon__alpha_al04;
            } 
	    else if (strcmp(ccparams_lte.pusch_alpha,"AL05")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_UplinkPowerControlCommon__alpha_al05;
            } 
	    else if (strcmp(ccparams_lte.pusch_alpha,"AL06")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_UplinkPowerControlCommon__alpha_al06;
            } 
	    else if (strcmp(ccparams_lte.pusch_alpha,"AL07")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_UplinkPowerControlCommon__alpha_al07;
            } 
	    else if (strcmp(ccparams_lte.pusch_alpha,"AL08")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_UplinkPowerControlCommon__alpha_al08;
            } 
	    else if (strcmp(ccparams_lte.pusch_alpha,"AL09")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_UplinkPowerControlCommon__alpha_al09;
            } 
	    else if (strcmp(ccparams_lte.pusch_alpha,"AL1")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_UplinkPowerControlCommon__alpha_al1;
            }

#endif
#if (LTE_RRC_VERSION >= MAKE_VERSION(12, 0, 0))

            if (strcmp(ccparams_lte.pusch_alpha,"AL0")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_Alpha_r12_al0;
            } 
	    else if (strcmp(ccparams_lte.pusch_alpha,"AL04")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_Alpha_r12_al04;
            } 
	    else if (strcmp(ccparams_lte.pusch_alpha,"AL05")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_Alpha_r12_al05;
            } 
	    else if (strcmp(ccparams_lte.pusch_alpha,"AL06")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_Alpha_r12_al06;
            } 
	    else if (strcmp(ccparams_lte.pusch_alpha,"AL07")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_Alpha_r12_al07;
            } 
	    else if (strcmp(ccparams_lte.pusch_alpha,"AL08")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_Alpha_r12_al08;
            } 
	    else if (strcmp(ccparams_lte.pusch_alpha,"AL09")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_Alpha_r12_al09;
            } 
	    else if (strcmp(ccparams_lte.pusch_alpha,"AL1")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pusch_alpha= LTE_Alpha_r12_al1;
            }

#endif
            else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_Alpha choice: AL0,AL04,AL05,AL06,AL07,AL08,AL09,AL1!\n",
                           RC.config_file_name, i, ccparams_lte.pusch_alpha);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_p0_Nominal= ccparams_lte.pucch_p0_Nominal;

            if ((ccparams_lte.pucch_p0_Nominal<-127) || 
		(ccparams_lte.pucch_p0_Nominal>-96))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pucch_p0_Nominal choice: -127..-96 !\n",
                           RC.config_file_name, i, ccparams_lte.pucch_p0_Nominal);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].msg3_delta_Preamble= ccparams_lte.msg3_delta_Preamble;

            if ((ccparams_lte.msg3_delta_Preamble<-1) || 
		(ccparams_lte.msg3_delta_Preamble>6))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for msg3_delta_Preamble choice: -1..6 !\n",
                           RC.config_file_name, i, ccparams_lte.msg3_delta_Preamble);

            if (strcmp(ccparams_lte.pucch_deltaF_Format1,"deltaF_2")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format1= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF_2;
            } 
	    else if (strcmp(ccparams_lte.pucch_deltaF_Format1,"deltaF0")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format1= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF0;
            } 
	    else if (strcmp(ccparams_lte.pucch_deltaF_Format1,"deltaF2")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format1= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format1_deltaF2;
            } 
	    else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format1 choice: deltaF_2,dltaF0,deltaF2!\n",
                           RC.config_file_name, i, ccparams_lte.pucch_deltaF_Format1);

            if (strcmp(ccparams_lte.pucch_deltaF_Format1b,"deltaF1")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format1b= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF1;
            } 
	    else if (strcmp(ccparams_lte.pucch_deltaF_Format1b,"deltaF3")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format1b= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF3;
            } 
	    else if (strcmp(ccparams_lte.pucch_deltaF_Format1b,"deltaF5")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format1b= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format1b_deltaF5;
            } 
	    else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format1b choice: deltaF1,dltaF3,deltaF5!\n",
                           RC.config_file_name, i, ccparams_lte.pucch_deltaF_Format1b);

            if (strcmp(ccparams_lte.pucch_deltaF_Format2,"deltaF_2")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF_2;
            } 
	    else if (strcmp(ccparams_lte.pucch_deltaF_Format2,"deltaF0")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF0;
            } 
	    else if (strcmp(ccparams_lte.pucch_deltaF_Format2,"deltaF1")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF1;
            } 
	    else if (strcmp(ccparams_lte.pucch_deltaF_Format2,"deltaF2")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2_deltaF2;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2 choice: deltaF_2,dltaF0,deltaF1,deltaF2!\n",
                           RC.config_file_name, i, ccparams_lte.pucch_deltaF_Format2);

            if (strcmp(ccparams_lte.pucch_deltaF_Format2a,"deltaF_2")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2a= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF_2;
            }
	    else if (strcmp(ccparams_lte.pucch_deltaF_Format2a,"deltaF0")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2a= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF0;
            } 
	    else if (strcmp(ccparams_lte.pucch_deltaF_Format2a,"deltaF2")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2a= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2a_deltaF2;
            } 
	    else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2a choice: deltaF_2,dltaF0,deltaF2!\n",
                           RC.config_file_name, i, ccparams_lte.pucch_deltaF_Format2a);

            if (strcmp(ccparams_lte.pucch_deltaF_Format2b,"deltaF_2")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2b= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF_2;
            } 
	    else if (strcmp(ccparams_lte.pucch_deltaF_Format2b,"deltaF0")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2b= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF0;
            } 
	    else if (strcmp(ccparams_lte.pucch_deltaF_Format2b,"deltaF2")==0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pucch_deltaF_Format2b= LTE_DeltaFList_PUCCH__deltaF_PUCCH_Format2b_deltaF2;
            } 
	    else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pucch_deltaF_Format2b choice: deltaF_2,dltaF0,deltaF2!\n",
                           RC.config_file_name, i, ccparams_lte.pucch_deltaF_Format2b);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_numberOfRA_Preambles= (ccparams_lte.rach_numberOfRA_Preambles/4)-1;

            if ((ccparams_lte.rach_numberOfRA_Preambles <4) || 
		(ccparams_lte.rach_numberOfRA_Preambles>64) || 
		((ccparams_lte.rach_numberOfRA_Preambles&3)!=0))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_numberOfRA_Preambles choice: 4,8,12,...,64!\n",
                           RC.config_file_name, i, ccparams_lte.rach_numberOfRA_Preambles);

            if (strcmp(ccparams_lte.rach_preamblesGroupAConfig, "ENABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preamblesGroupAConfig= TRUE;
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_sizeOfRA_PreamblesGroupA= (ccparams_lte.rach_sizeOfRA_PreamblesGroupA/4)-1;

              if ((ccparams_lte.rach_numberOfRA_Preambles <4) || 
		  (ccparams_lte.rach_numberOfRA_Preambles>60) || 
		  ((ccparams_lte.rach_numberOfRA_Preambles&3)!=0))
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_sizeOfRA_PreamblesGroupA choice: 4,8,12,...,60!\n",
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
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_messageSizeGroupA choice: 56,144,208,256!\n",
			     RC.config_file_name, i, ccparams_lte.rach_messageSizeGroupA);
		break;
              }

              if (strcmp(ccparams_lte.rach_messagePowerOffsetGroupB,"minusinfinity")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_minusinfinity;
              } 
	      else if (strcmp(ccparams_lte.rach_messagePowerOffsetGroupB,"dB0")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB0;
              } 
	      else if (strcmp(ccparams_lte.rach_messagePowerOffsetGroupB,"dB5")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB5;
              } 
	      else if (strcmp(ccparams_lte.rach_messagePowerOffsetGroupB,"dB8")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB8;
              } 
	      else if (strcmp(ccparams_lte.rach_messagePowerOffsetGroupB,"dB10")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB10;
              } 
	      else if (strcmp(ccparams_lte.rach_messagePowerOffsetGroupB,"dB12")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB12;
              } 
	      else if (strcmp(ccparams_lte.rach_messagePowerOffsetGroupB,"dB15")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB15;
              } 
	      else if (strcmp(ccparams_lte.rach_messagePowerOffsetGroupB,"dB18")==0) {
                RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_messagePowerOffsetGroupB= LTE_RACH_ConfigCommon__preambleInfo__preamblesGroupAConfig__messagePowerOffsetGroupB_dB18;
              } 
	      else
                AssertFatal (0,
                             "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rach_messagePowerOffsetGroupB choice: minusinfinity,dB0,dB5,dB8,dB10,dB12,dB15,dB18!\n",
                             RC.config_file_name, i, ccparams_lte.rach_messagePowerOffsetGroupB);
            } else if (strcmp(ccparams_lte.rach_preamblesGroupAConfig, "DISABLE") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preamblesGroupAConfig= FALSE;
            } else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for rach_preamblesGroupAConfig choice: ENABLE,DISABLE !\n",
                           RC.config_file_name, i, ccparams_lte.rach_preamblesGroupAConfig);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleInitialReceivedTargetPower= (ccparams_lte.rach_preambleInitialReceivedTargetPower+120)/2;

            if ((ccparams_lte.rach_preambleInitialReceivedTargetPower<-120) || 
		(ccparams_lte.rach_preambleInitialReceivedTargetPower>-90) || 
		((ccparams_lte.rach_preambleInitialReceivedTargetPower&1)!=0))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_preambleInitialReceivedTargetPower choice: -120,-118,...,-90 !\n",
                           RC.config_file_name, i, ccparams_lte.rach_preambleInitialReceivedTargetPower);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_powerRampingStep= ccparams_lte.rach_powerRampingStep/2;

            if ((ccparams_lte.rach_powerRampingStep<0) || 
		(ccparams_lte.rach_powerRampingStep>6) || 
		((ccparams_lte.rach_powerRampingStep&1)!=0))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_powerRampingStep choice: 0,2,4,6 !\n",
                           RC.config_file_name, i, ccparams_lte.rach_powerRampingStep);

            switch (ccparams_lte.rach_preambleTransMax) {
#if (LTE_RRC_VERSION < MAKE_VERSION(14, 0, 0))

	    case 3:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax= LTE_RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n3;
	      break;

	    case 4:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax= LTE_RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n4;
	      break;

	    case 5:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax= LTE_RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n5;
	      break;

	    case 6:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax= LTE_RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n6;
	      break;

	    case 7:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax= LTE_RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n7;
	      break;

	    case 8:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax= LTE_RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n8;
	      break;

	    case 10:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax= LTE_RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n10;
	      break;

	    case 20:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax= LTE_RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n20;
	      break;

	    case 50:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax= LTE_RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n50;
	      break;

	    case 100:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax= LTE_RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n100;
	      break;

	    case 200:
	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_preambleTransMax= LTE_RACH_ConfigCommon__ra_SupervisionInfo__preambleTransMax_n200;
	      break;
#else

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

#endif

	    default:
	      AssertFatal (0,
			   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_preambleTransMax choice: 3,4,5,6,7,8,10,20,50,100,200!\n",
			   RC.config_file_name, i, ccparams_lte.rach_preambleTransMax);
	      break;
            }

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_raResponseWindowSize=  (ccparams_lte.rach_raResponseWindowSize==10)?7:ccparams_lte.rach_raResponseWindowSize-2;

            if ((ccparams_lte.rach_raResponseWindowSize<0)||
		(ccparams_lte.rach_raResponseWindowSize==9)||
		(ccparams_lte.rach_raResponseWindowSize>10))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_raResponseWindowSize choice: 2,3,4,5,6,7,8,10!\n",
                           RC.config_file_name, i, ccparams_lte.rach_preambleTransMax);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_macContentionResolutionTimer= (ccparams_lte.rach_macContentionResolutionTimer/8)-1;

            if ((ccparams_lte.rach_macContentionResolutionTimer<8) || 
		(ccparams_lte.rach_macContentionResolutionTimer>64) || 
		((ccparams_lte.rach_macContentionResolutionTimer&7)!=0))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_macContentionResolutionTimer choice: 8,16,...,56,64!\n",
                           RC.config_file_name, i, ccparams_lte.rach_preambleTransMax);

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].rach_maxHARQ_Msg3Tx= ccparams_lte.rach_maxHARQ_Msg3Tx;

            if ((ccparams_lte.rach_maxHARQ_Msg3Tx<0) || 
		(ccparams_lte.rach_maxHARQ_Msg3Tx>8))
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for rach_maxHARQ_Msg3Tx choice: 1..8!\n",
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
			   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for pcch_defaultPagingCycle choice: 32,64,128,256!\n",
			   RC.config_file_name, i, ccparams_lte.pcch_defaultPagingCycle);
	      break;
            }

            if (strcmp(ccparams_lte.pcch_nB, "fourT") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= LTE_PCCH_Config__nB_fourT;
            } 
	    else if (strcmp(ccparams_lte.pcch_nB, "twoT") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= LTE_PCCH_Config__nB_twoT;
            } 
	    else if (strcmp(ccparams_lte.pcch_nB, "oneT") == 0) {
 	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= LTE_PCCH_Config__nB_oneT;
            } 
	    else if (strcmp(ccparams_lte.pcch_nB, "halfT") == 0) {
 	      RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= LTE_PCCH_Config__nB_halfT;
            } 
	    else if (strcmp(ccparams_lte.pcch_nB, "quarterT") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= LTE_PCCH_Config__nB_quarterT;
            } 
	    else if (strcmp(ccparams_lte.pcch_nB, "oneEighthT") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= LTE_PCCH_Config__nB_oneEighthT;
            } 
	    else if (strcmp(ccparams_lte.pcch_nB, "oneSixteenthT") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= LTE_PCCH_Config__nB_oneSixteenthT;
            } 
	    else if (strcmp(ccparams_lte.pcch_nB, "oneThirtySecondT") == 0) {
              RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].pcch_nB= LTE_PCCH_Config__nB_oneThirtySecondT;
            } 
	    else
              AssertFatal (0,
                           "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for pcch_nB choice: fourT,twoT,oneT,halfT,quarterT,oneighthT,oneSixteenthT,oneThirtySecondT !\n",
                           RC.config_file_name, i, ccparams_lte.pcch_nB);

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
			   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for bcch_modificationPeriodCoeff choice: 2,4,8,16",
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
			   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_TransmissionMode choice: 1,2,3,4,5,6,7",
			   RC.config_file_name, i, ccparams_lte.ue_TransmissionMode);
	      break;
            }

            RRC_CONFIGURATION_REQ (msg_p).radioresourceconfig[j].ue_multiple_max= ccparams_lte.ue_multiple_max;

            switch (ccparams_lte.N_RB_DL) {
	    case 25:
	      if ((ccparams_lte.ue_multiple_max < 1) || 
		  (ccparams_lte.ue_multiple_max > 4))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_multiple_max choice: 1..4!\n",
			     RC.config_file_name, i, ccparams_lte.ue_multiple_max);

	      break;

	    case 50:
	      if ((ccparams_lte.ue_multiple_max < 1) || 
		  (ccparams_lte.ue_multiple_max > 8))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_multiple_max choice: 1..8!\n",
			     RC.config_file_name, i, ccparams_lte.ue_multiple_max);

	      break;

	    case 100:
	      if ((ccparams_lte.ue_multiple_max < 1) || 
		  (ccparams_lte.ue_multiple_max > 16))
		AssertFatal (0,
			     "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for ue_multiple_max choice: 1..16!\n",
			     RC.config_file_name, i, ccparams_lte.ue_multiple_max);

	      break;

	    default:
	      AssertFatal (0,
			   "Failed to parse eNB configuration file %s, enb %d unknown value \"%d\" for N_RB_DL choice: 25,50,100 !\n",
			   RC.config_file_name, i, ccparams_lte.N_RB_DL);
	      break;
            }

	    // eMBMS configuration
	    RRC_CONFIGURATION_REQ(msg_p).eMBMS_configured = 0;
	    printf("No eMBMS configuration, skipping it\n");

	    // eMTC configuration
	    char brparamspath[MAX_OPTNAME_SIZE*2 + 16];
	    sprintf(brparamspath,"%s.%s", ccspath, ENB_CONFIG_STRING_EMTC_PARAMETERS);
	    config_get(eMTCParams, sizeof(eMTCParams)/sizeof(paramdef_t), brparamspath);
	    RRC_CONFIGURATION_REQ(msg_p).eMTC_configured = eMTCconfig.eMTC_configured&1;

	    if (eMTCconfig.eMTC_configured > 0) fill_eMTC_configuration(msg_p,&eMTCconfig, i,j,RC.config_file_name,brparamspath); 
	    else   	                        printf("No eMTC configuration, skipping it\n");
	    

	    // Sidelink configuration
	    char SLparamspath[MAX_OPTNAME_SIZE*2 + 16];
	    sprintf(SLparamspath,"%s.%s", ccspath, ENB_CONFIG_STRING_SL_PARAMETERS);
	    config_get( SLParams, sizeof(SLParams)/sizeof(paramdef_t), SLparamspath);

	    // Sidelink Resource pool information
	    RRC_CONFIGURATION_REQ (msg_p).SL_configured=SLconfig.sidelink_configured&1;
	    if (SLconfig.sidelink_configured==1) fill_SL_configuration(msg_p,&SLconfig,i,j,RC.config_file_name);
	    else                                 printf("No SL configuration skipping it\n");
	  }
	}

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
		       "Bad config value when parsing eNB configuration file %s, enb %d  srb1_max_retx_threshold %u!\n",
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
			 "Bad config value when parsing eNB configuration file %s, enb %d  srb1_poll_pdu %u!\n",
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
			 "Bad config value when parsing eNB configuration file %s, enb %d  srb1_poll_byte %u!\n",
			 RC.config_file_name, i, srb1_params.srb1_poll_byte);
	}
	
	if (srb1_params.srb1_timer_poll_retransmit <= 250) {
	  rrc->srb1_timer_poll_retransmit = (srb1_params.srb1_timer_poll_retransmit - 5)/5;
	} else if (srb1_params.srb1_timer_poll_retransmit <= 500) {
	  rrc->srb1_timer_poll_retransmit = (srb1_params.srb1_timer_poll_retransmit - 300)/50 + 50;
	} else {
	  AssertFatal (0,
		       "Bad config value when parsing eNB configuration file %s, enb %d  srb1_timer_poll_retransmit %u!\n",
		       RC.config_file_name, i, srb1_params.srb1_timer_poll_retransmit);
	}
	
	if (srb1_params.srb1_timer_status_prohibit <= 250) {
	  rrc->srb1_timer_status_prohibit = srb1_params.srb1_timer_status_prohibit/5;
	} else if ((srb1_params.srb1_timer_poll_retransmit >= 300) && (srb1_params.srb1_timer_poll_retransmit <= 500)) {
	  rrc->srb1_timer_status_prohibit = (srb1_params.srb1_timer_status_prohibit - 300)/50 + 51;
	} else {
	  AssertFatal (0,
		       "Bad config value when parsing eNB configuration file %s, enb %d  srb1_timer_status_prohibit %u!\n",
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
		       "Bad config value when parsing eNB configuration file %s, enb %d  srb1_timer_reordering %u!\n",
		       RC.config_file_name, i, srb1_params.srb1_timer_reordering);
	}
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

  memset((void*)&ccparams_lte,0,sizeof(ccparams_lte_t));


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
            } else {
              AssertFatal(0,
                          "Failed to parse eNB configuration file %s, enb %d unknown value \"%s\" for cell_type choice: CELL_MACRO_ENB or CELL_HOME_ENB !\n",
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
              } else if (strcmp(*(S1ParamList.paramarray[l][ENB_MME_IP_ADDRESS_PREFERENCE_IDX].strptr), "ipv6") == 0) {
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

  memset((void*)&ccparams_lte,0,sizeof(ccparams_lte_t));


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
  

  paramdef_t ThreadParams[]  = THREAD_CONF_DESC;
  paramlist_def_t THREADParamList = {THREAD_CONFIG_STRING_THREAD_STRUCT,NULL,0};
  config_getlist( &THREADParamList,NULL,0,NULL);

  if(parallel_config == NULL){
    if(THREADParamList.numelt>0) {
      config_getlist( &THREADParamList,ThreadParams,sizeof(ThreadParams)/sizeof(paramdef_t),NULL);
      parallel_conf = strdup(*(THREADParamList.paramarray[0][THREAD_PARALLEL_IDX].strptr));
    } else {
      parallel_conf = strdup("PARALLEL_RU_L1_TRX_SPLIT");
    }
    set_parallel_conf(parallel_conf);
  }

  if(worker_config == NULL){
    if(THREADParamList.numelt>0) {
      config_getlist( &THREADParamList,ThreadParams,sizeof(ThreadParams)/sizeof(paramdef_t),NULL);
      worker_conf   = strdup(*(THREADParamList.paramarray[0][THREAD_WORKER_IDX].strptr));
    } else {
      worker_conf   = strdup("WORKER_ENABLE");
    }
    set_worker_conf(worker_conf);
  }



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
