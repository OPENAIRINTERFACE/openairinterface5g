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
  mce_config.c
  -------------------
  AUTHOR  : Javier Morgade
  COMPANY : Vicomtech, Spain
  EMAIL   : javier.morgade@ieee.org
*/

#include <string.h>
#include <inttypes.h>

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

#include "L1_paramdef.h"
#include "MACRLC_paramdef.h"
#include "common/config/config_userapi.h"
#include "RRC_config_tools.h"
#include "enb_paramdef.h"
#include "proto_agent.h"

#include "m3ap_MCE.h"



int RCconfig_MCE(void ) {
  //int               num_enbs                      = 0;
  //char             *enb_interface_name_for_S1U    = NULL;
  char               *mce_interface_name_for_m2_enb = NULL;
  char               *mce_interface_name_for_m3_mme = NULL;
  //char             *enb_ipv4_address_for_S1U      = NULL;
  char               *mce_ipv4_address_for_m2c      = NULL;
  char               *mce_ipv4_address_for_m3c      = NULL;
  //uint32_t          enb_port_for_S1U              = 0;
  uint32_t            mce_port_for_m2c              = 0;
  uint32_t            mce_port_for_m3c              = 0;
  char             *address                       = NULL;
  char             *cidr                          = NULL;
  //char gtpupath[MAX_OPTNAME_SIZE*2 + 8];
  char mcepath[MAX_OPTNAME_SIZE*2 + 8];
  //paramdef_t ENBSParams[] = ENBSPARAMS_DESC;
  //paramdef_t GTPUParams[]  = GTPUPARAMS_DESC;

  paramdef_t   MCEParams[] = MCE_NETPARAMS_DESC;

  ///* get number of active eNodeBs */
  //config_get( ENBSParams,sizeof(ENBSParams)/sizeof(paramdef_t),NULL);
  //num_enbs = ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt;
  //AssertFatal (num_enbs >0,
  //             "Failed to parse config file no active eNodeBs in %s \n", ENB_CONFIG_STRING_ACTIVE_ENBS);
  //sprintf(gtpupath,"%s.[%i].%s",ENB_CONFIG_STRING_ENB_LIST,0,ENB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);
  sprintf(mcepath,"%s.[%i].%s","MCEs",0,MCE_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);
  //config_get( GTPUParams,sizeof(GTPUParams)/sizeof(paramdef_t),gtpupath);
  config_get(MCEParams,sizeof(MCEParams)/sizeof(paramdef_t),mcepath);
  //cidr = enb_ipv4_address_for_S1U;
  cidr = mce_ipv4_address_for_m2c;
  address = strtok(cidr, "/");

  //LOG_W(MCE_APP,"cidr %s\n",cidr);
  //LOG_W(MCE_APP,"address %s\n",address);
  //LOG_W(MCE_APP,"mce_interface_name_for_m2_enb %s\n",mce_interface_name_for_m2_enb);
  //LOG_W(MCE_APP,"mce_ipv4_address_for_m2c %s\n",mce_ipv4_address_for_m2c);
  //LOG_W(MCE_APP,"mce_ipv4_address_for_m2c %s\n",*(MCEParams[1/*MCE_IPV4_ADDRESS_FOR_M2C_IDX*/].strptr));
  //LOG_W(MCE_APP,"mce_port_for_m2c %d\n",mce_port_for_m2c);
  //LOG_W(MCE_APP,"mce_interface_name_for_m3_mme %s\n",mce_interface_name_for_m3_mme);
  //LOG_W(MCE_APP,"mce_ipv4_address_for_m3c %s\n",mce_ipv4_address_for_m3c);
  //LOG_W(MCE_APP,"mce_port_for_m3c %d\n",mce_port_for_m3c);

//   strcpy(X2AP_REGISTER_ENB_REQ (msg_p).target_enb_x2_ip_address[l].ipv4_address,*(X2ParamList.paramarray[l][ENB_X2_IPV4_ADDRESS_IDX].strptr));


  if (address) {
    MessageDef *message; 
    AssertFatal((message = itti_alloc_new_message(TASK_MCE_APP, 0, M2AP_MCE_SCTP_REQ))!=NULL,"");
    //IPV4_STR_ADDR_TO_INT_NWBO ( address, M2AP_MCE_SCTP_REQ(message).mce_m2_ip_address, "BAD IP ADDRESS FORMAT FOR MCE M2_C !\n" );
    M2AP_MCE_SCTP_REQ (message).mce_m2_ip_address.ipv6 = 0;
    M2AP_MCE_SCTP_REQ (message).mce_m2_ip_address.ipv4 = 1;
    strcpy( M2AP_MCE_SCTP_REQ (message).mce_m2_ip_address.ipv4_address, address);
    //LOG_I(MCE_APP,"Configuring M2_C address : %s -> %x\n",address,M2AP_MCE_SCTP_REQ(message).mce_m2_ip_address);
    M2AP_MCE_SCTP_REQ(message).mce_port_for_M2C = mce_port_for_m2c;
    itti_send_msg_to_task (TASK_M2AP_MCE, 0, message); // data model is wrong: gtpu doesn't have enb_id (or module_id) 
  } else
    LOG_E(MCE_APP,"invalid address for M2AP\n");
  
  return 0;
}


int RCconfig_M3(MessageDef *msg_p, uint32_t i) {
  int l;
  //int               num_enbs                      = 0;
  //char             *enb_interface_name_for_S1U    = NULL;
  char               *mce_interface_name_for_m2_enb = NULL;
  char               *mce_interface_name_for_m3_mme = NULL;
  //char             *enb_ipv4_address_for_S1U      = NULL;
  char               *mce_ipv4_address_for_m2c      = NULL;
  char               *mce_ipv4_address_for_m3c      = NULL;
  //uint32_t          enb_port_for_S1U              = 0;
  uint32_t            mce_port_for_m2c              = 0;
  uint32_t            mce_port_for_m3c              = 0;
  char             *address                       = NULL;
  char             *cidr                          = NULL;


  char mcepath[MAX_OPTNAME_SIZE*2 + 8];
  paramdef_t   MCEParams[] = MCE_NETPARAMS_DESC;
  paramdef_t M3Params[]  = M3PARAMS_DESC;
  paramdef_t MCCHParams[]  = MCCH_PARAMS_DESC;

  sprintf(mcepath,"%s.[%i].%s","MCEs",0,MCE_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);
  config_get(MCEParams,sizeof(MCEParams)/sizeof(paramdef_t),mcepath);

  paramlist_def_t M3ParamList = {MCE_CONFIG_STRING_TARGET_MME_M3_IP_ADDRESS,NULL,0};
  paramlist_def_t MCCHParamList = {MCE_CONFIG_STRING_MCCH_CONFIG_PER_MBSFN_AREA,NULL,0};

  char aprefix[MAX_OPTNAME_SIZE*80 + 8];
  sprintf(aprefix,"%s.[%i]","MCEs",0);
  /* Some default/random parameters */
  M3AP_REGISTER_MCE_REQ (msg_p).MCE_id = i;
  //M3AP_REGISTER_MCE_REQ (msg_p).MME_name = "kk";
  sprintf(aprefix,"%s.[%i]","MCEs",0);
  config_getlist( &M3ParamList,M3Params,sizeof(M3Params)/sizeof(paramdef_t),aprefix);
  //printf("M3ParamList.numelt %d\n",M3ParamList.numelt);
  M3AP_REGISTER_MCE_REQ (msg_p).nb_m3 = 0;
  for (l = 0; l < M3ParamList.numelt; l++) {
	M3AP_REGISTER_MCE_REQ (msg_p).nb_m3 += 1;
	M3AP_REGISTER_MCE_REQ (msg_p).MCE_name         = strdup(*(M3ParamList.paramarray[l][MCE_MCE_NAME_IDX].strptr));

        strcpy(M3AP_REGISTER_MCE_REQ (msg_p).target_mme_m3_ip_address[l].ipv4_address,*(M3ParamList.paramarray[l][MCE2_M3_IPV4_ADDRESS_IDX].strptr));
        strcpy(M3AP_REGISTER_MCE_REQ (msg_p).target_mme_m3_ip_address[l].ipv6_address,*(M3ParamList.paramarray[l][MCE2_M3_IPV6_ADDRESS_IDX].strptr));

        if (strcmp(*(M3ParamList.paramarray[l][MCE2_M3_IP_ADDRESS_PREFERENCE_IDX].strptr), "ipv4") == 0) {
		M3AP_REGISTER_MCE_REQ (msg_p).target_mme_m3_ip_address[l].ipv4 = 1;
		M3AP_REGISTER_MCE_REQ (msg_p).target_mme_m3_ip_address[l].ipv6 = 0;
	} else if (strcmp(*(M3ParamList.paramarray[l][MCE2_M3_IP_ADDRESS_PREFERENCE_IDX].strptr), "ipv6") == 0) {
		M3AP_REGISTER_MCE_REQ (msg_p).target_mme_m3_ip_address[l].ipv4 = 0;
		M3AP_REGISTER_MCE_REQ (msg_p).target_mme_m3_ip_address[l].ipv6 = 1;
	} else if (strcmp(*(M3ParamList.paramarray[l][MCE2_M3_IP_ADDRESS_PREFERENCE_IDX].strptr), "no") == 0) {
		M3AP_REGISTER_MCE_REQ (msg_p).target_mme_m3_ip_address[l].ipv4 = 1;
		M3AP_REGISTER_MCE_REQ (msg_p).target_mme_m3_ip_address[l].ipv6 = 1;
        }
	M3AP_REGISTER_MCE_REQ (msg_p).sctp_out_streams = 2;
	M3AP_REGISTER_MCE_REQ (msg_p).sctp_in_streams  = 2;
  }

  sprintf(aprefix,"%s.[%i].%s","MCEs",0,MCE_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);
  config_get( MCEParams,sizeof(MCEParams)/sizeof(paramdef_t),aprefix);
  M3AP_REGISTER_MCE_REQ (msg_p).mme_port_for_M3C = (uint32_t)*(MCEParams[MCE2_PORT_FOR_M3C_IDX].uptr);

  if ((MCEParams[MCE2_IPV4_ADDR_FOR_M3C_IDX].strptr == NULL) || (M3AP_REGISTER_MCE_REQ (msg_p).mme_port_for_M3C == 0)) {
      LOG_E(RRC,"Add eNB IPv4 address and/or port for M3C in the CONF file!\n");
      exit(1);
  }

  cidr = *(MCEParams[MCE2_IPV4_ADDR_FOR_M3C_IDX].strptr);
  address = strtok(cidr, "/");
  M3AP_REGISTER_MCE_REQ (msg_p).mme_m3_ip_address.ipv6 = 0;
  M3AP_REGISTER_MCE_REQ (msg_p).mme_m3_ip_address.ipv4 = 1;
  strcpy(M3AP_REGISTER_MCE_REQ (msg_p).mme_m3_ip_address.ipv4_address, address);

  sprintf(aprefix,"%s.[%i]","MCEs",0);
  config_getlist( &MCCHParamList,MCCHParams,sizeof(MCCHParams)/sizeof(paramdef_t),aprefix);
  //printf("MCCHParamList.numelt %d\n",MCCHParamList.numelt);
  for (l = 0; l < MCCHParamList.numelt; l++) {
        //M2AP_SETUP_RESP(msg_p).mcch_config_per_mbsfn[l].mbsfn_area = *(MCCHParamList.paramarray[l][MCCH_MBSFN_AREA_IDX].uptr);
        //M2AP_SETUP_RESP(msg_p).mcch_config_per_mbsfn[l].pdcch_length = *(MCCHParamList.paramarray[l][MCCH_PDCCH_LENGTH_IDX].uptr);
        //M2AP_SETUP_RESP(msg_p).mcch_config_per_mbsfn[l].repetition_period = *(MCCHParamList.paramarray[l][MCCH_REPETITION_PERIOD_IDX].uptr);
        //M2AP_SETUP_RESP(msg_p).mcch_config_per_mbsfn[l].offset = *(MCCHParamList.paramarray[l][MCCH_OFFSET_IDX].uptr);
        //M2AP_SETUP_RESP(msg_p).mcch_config_per_mbsfn[l].modification_period = *(MCCHParamList.paramarray[l][MCCH_MODIFICATION_PERIOD_IDX].uptr);
        //M2AP_SETUP_RESP(msg_p).mcch_config_per_mbsfn[l].subframe_allocation_info = *(MCCHParamList.paramarray[l][MCCH_SF_ALLOCATION_INFO_IDX].uptr);
        //M2AP_SETUP_RESP(msg_p).mcch_config_per_mbsfn[l].mcs = strdup(*(MCCHParamList.paramarray[l][MCCH_MCS_IDX].strptr));
  }


  


    return 0;
}
int RCconfig_m2_mcch(m2ap_setup_resp_t * m2ap_setup_resp, uint32_t i) {
  int l;

  char mcepath[MAX_OPTNAME_SIZE*2 + 8];
  paramdef_t MCCHParams[]  = MCCH_PARAMS_DESC;
  paramdef_t   MCEParams[] = MCE_PARAMS_DESC;
  paramdef_t PLNMParams[]  = MCE_PLMN_PARAMS_DESC;
  paramlist_def_t MCCHParamList = {MCE_CONFIG_STRING_MCCH_CONFIG_PER_MBSFN_AREA,NULL,0};
  char aprefix[MAX_OPTNAME_SIZE*80 + 8];
  sprintf(mcepath,"%s.[%i]","MCEs",0);
  config_get(MCEParams,sizeof(MCEParams)/sizeof(paramdef_t),mcepath);
  m2ap_setup_resp->MCE_id = *(MCEParams[MCE_MCE_ID_IDX].uptr);
  m2ap_setup_resp->MCE_name = strdup(*(MCEParams[MCE_MCE_NAME_IDX].strptr));;
  sprintf(mcepath,"%s.[%i].%s","MCEs",0,MCE_CONFIG_STRING_PLMN);
  config_get(PLNMParams,sizeof(PLNMParams)/sizeof(paramdef_t),mcepath);
  m2ap_setup_resp->mcc = *(PLNMParams[MCE_CONFIG_STRING_MCC_IDX].uptr);
  m2ap_setup_resp->mnc = *(PLNMParams[MCE_CONFIG_STRING_MNC_IDX].uptr);
  m2ap_setup_resp->mnc_digit_length = *(PLNMParams[MCE_CONFIG_STRING_MNC_LENGTH_IDX].uptr);
  //LOG_E(MCE_APP,"PLNM %d\n",*(PLNMParams[MCE_CONFIG_STRING_MCC_IDX].uptr));
  //LOG_E(MCE_APP,"PLNM %d\n",*(PLNMParams[MCE_CONFIG_STRING_MNC_LENGTH_IDX].uptr));


  sprintf(aprefix,"%s.[%i]","MCEs",0);
  config_getlist( &MCCHParamList,MCCHParams,sizeof(MCCHParams)/sizeof(paramdef_t),aprefix);
  //printf("MCCHParamList.numelt %d\n",MCCHParamList.numelt);
  AssertFatal(MCCHParamList.numelt <= 8, "File wrong parsed\n");
  for (l = 0; l < MCCHParamList.numelt; l++) {
        m2ap_setup_resp->mcch_config_per_mbsfn[l].mbsfn_area = *(MCCHParamList.paramarray[l][MCCH_MBSFN_AREA_IDX].uptr);
        m2ap_setup_resp->mcch_config_per_mbsfn[l].pdcch_length = *(MCCHParamList.paramarray[l][MCCH_PDCCH_LENGTH_IDX].uptr);
        m2ap_setup_resp->mcch_config_per_mbsfn[l].repetition_period = *(MCCHParamList.paramarray[l][MCCH_REPETITION_PERIOD_IDX].uptr);
        m2ap_setup_resp->mcch_config_per_mbsfn[l].offset = *(MCCHParamList.paramarray[l][MCCH_OFFSET_IDX].uptr);
        m2ap_setup_resp->mcch_config_per_mbsfn[l].modification_period = *(MCCHParamList.paramarray[l][MCCH_MODIFICATION_PERIOD_IDX].uptr);
        m2ap_setup_resp->mcch_config_per_mbsfn[l].subframe_allocation_info = *(MCCHParamList.paramarray[l][MCCH_SF_ALLOCATION_INFO_IDX].uptr);
        m2ap_setup_resp->mcch_config_per_mbsfn[l].mcs = *(MCCHParamList.paramarray[l][MCCH_MCS_IDX].uptr);
  }
  m2ap_setup_resp->num_mcch_config_per_mbsfn = MCCHParamList.numelt;
	return 0;
}
int RCconfig_M2_MCCH(MessageDef *msg_p, uint32_t i) {
  int l;

  char mcepath[MAX_OPTNAME_SIZE*2 + 8];
  paramdef_t MCCHParams[]  = MCCH_PARAMS_DESC;
  paramdef_t   MCEParams[] = MCE_PARAMS_DESC;
  paramdef_t PLNMParams[]  = MCE_PLMN_PARAMS_DESC;
  paramlist_def_t MCCHParamList = {MCE_CONFIG_STRING_MCCH_CONFIG_PER_MBSFN_AREA,NULL,0};
  char aprefix[MAX_OPTNAME_SIZE*80 + 8];
  sprintf(mcepath,"%s.[%i]","MCEs",0);
  config_get(MCEParams,sizeof(MCEParams)/sizeof(paramdef_t),mcepath);
  M2AP_SETUP_RESP (msg_p).MCE_id = *(MCEParams[MCE_MCE_ID_IDX].uptr);
  M2AP_SETUP_RESP (msg_p).MCE_name = strdup(*(MCEParams[MCE_MCE_NAME_IDX].strptr));;
  sprintf(mcepath,"%s.[%i].%s","MCEs",0,MCE_CONFIG_STRING_PLMN);
  config_get(PLNMParams,sizeof(PLNMParams)/sizeof(paramdef_t),mcepath);
  M2AP_SETUP_RESP (msg_p).mcc = *(PLNMParams[MCE_CONFIG_STRING_MCC_IDX].uptr);
  M2AP_SETUP_RESP (msg_p).mnc = *(PLNMParams[MCE_CONFIG_STRING_MNC_IDX].uptr);
  M2AP_SETUP_RESP (msg_p).mnc_digit_length = *(PLNMParams[MCE_CONFIG_STRING_MNC_LENGTH_IDX].uptr);
  //LOG_E(MCE_APP,"PLNM %d\n",*(PLNMParams[MCE_CONFIG_STRING_MCC_IDX].uptr));
  //LOG_E(MCE_APP,"PLNM %d\n",*(PLNMParams[MCE_CONFIG_STRING_MNC_LENGTH_IDX].uptr));


  sprintf(aprefix,"%s.[%i]","MCEs",0);
  config_getlist( &MCCHParamList,MCCHParams,sizeof(MCCHParams)/sizeof(paramdef_t),aprefix);
  //printf("MCCHParamList.numelt %d\n",MCCHParamList.numelt);
  AssertFatal(MCCHParamList.numelt <= 8, "File wrong parsed\n");
  for (l = 0; l < MCCHParamList.numelt; l++) {
        M2AP_SETUP_RESP(msg_p).mcch_config_per_mbsfn[l].mbsfn_area = *(MCCHParamList.paramarray[l][MCCH_MBSFN_AREA_IDX].uptr);
        M2AP_SETUP_RESP(msg_p).mcch_config_per_mbsfn[l].pdcch_length = *(MCCHParamList.paramarray[l][MCCH_PDCCH_LENGTH_IDX].uptr);
        M2AP_SETUP_RESP(msg_p).mcch_config_per_mbsfn[l].repetition_period = *(MCCHParamList.paramarray[l][MCCH_REPETITION_PERIOD_IDX].uptr);
        M2AP_SETUP_RESP(msg_p).mcch_config_per_mbsfn[l].offset = *(MCCHParamList.paramarray[l][MCCH_OFFSET_IDX].uptr);
        M2AP_SETUP_RESP(msg_p).mcch_config_per_mbsfn[l].modification_period = *(MCCHParamList.paramarray[l][MCCH_MODIFICATION_PERIOD_IDX].uptr);
        M2AP_SETUP_RESP(msg_p).mcch_config_per_mbsfn[l].subframe_allocation_info = *(MCCHParamList.paramarray[l][MCCH_SF_ALLOCATION_INFO_IDX].uptr);
        M2AP_SETUP_RESP(msg_p).mcch_config_per_mbsfn[l].mcs = *(MCCHParamList.paramarray[l][MCCH_MCS_IDX].uptr);
  }
  M2AP_SETUP_RESP(msg_p).num_mcch_config_per_mbsfn = MCCHParamList.numelt;


	return 0;
}
int RCconfig_m2_scheduling(  m2ap_mbms_scheduling_information_t * m2ap_mbms_scheduling_information, uint32_t i) {
  int l,j,k/*,m*/;
  char mcepath[MAX_OPTNAME_SIZE*2 + 8];
  paramdef_t MBMS_SCHE_Params[]  = MCE_MBMS_SCHEDULING_INFO_PARAMS_DESC;
  sprintf(mcepath,"%s.[%i].%s","MCEs",0,MCE_CONFIG_STRING_MBMS_SCHEDULING_INFO);
  //LOG_E(MCE_APP,"%s\n",mcepath);
  config_get(MBMS_SCHE_Params,sizeof(MBMS_SCHE_Params)/sizeof(paramdef_t),mcepath);
  //LOG_E(MCE_APP,"%s %d\n",mcepath, *(MBMS_SCHE_Params[MCE_CONFIG_STRING_MCCH_UPDATE_TIME_IDX].uptr));
  m2ap_mbms_scheduling_information->mcch_update_time=*(MBMS_SCHE_Params[MCE_CONFIG_STRING_MCCH_UPDATE_TIME_IDX].uptr);

  paramdef_t MBMS_CONFIGURATION_Params[] = MCE_MBMS_AREA_CONFIGURATION_LIST_PARAMS_DESC;
  paramlist_def_t MBMS_AREA_CONFIGURATION_ParamList = {MCE_CONFIG_STRING_MBMS_AREA_CONFIGURATION_LIST,NULL,0};
  sprintf(mcepath,"%s.[%i].%s","MCEs",0,MCE_CONFIG_STRING_MBMS_SCHEDULING_INFO);
  config_getlist(&MBMS_AREA_CONFIGURATION_ParamList,MBMS_CONFIGURATION_Params,sizeof(MBMS_CONFIGURATION_Params)/sizeof(paramdef_t),mcepath);
  AssertFatal(MBMS_AREA_CONFIGURATION_ParamList.numelt <= 8, "File wrong parsed\n");
  //LOG_E(MCE_APP,"%s\n",mcepath);
  //LOG_E(MCE_APP,"MBMS_AREA_CONFIGURATION_ParamList.numelt %d\n",MBMS_AREA_CONFIGURATION_ParamList.numelt);

  m2ap_mbms_scheduling_information->num_mbms_area_config_list = MBMS_AREA_CONFIGURATION_ParamList.numelt;
  for (l = 0; l < MBMS_AREA_CONFIGURATION_ParamList.numelt; l++) {
	m2ap_mbms_scheduling_information->mbms_area_config_list[l].common_sf_allocation_period = *(MBMS_AREA_CONFIGURATION_ParamList.paramarray[l][MCE_CONFIG_STRING_COMMON_SF_ALLOCATION_PERIOD_IDX].uptr);
	m2ap_mbms_scheduling_information->mbms_area_config_list[l].mbms_area_id=*(MBMS_AREA_CONFIGURATION_ParamList.paramarray[l][MCE_CONFIG_STRING_MBMS_AREA_ID_IDX].uptr);
  	char mcepath2[MAX_OPTNAME_SIZE*2 + 8];
  	paramdef_t PMCH_Params[]  = MCE_MBMS_PMCH_CONFIGURATION_LIST_PARAMS_DESC;
  	paramlist_def_t PMCH_ParamList = {MCE_CONFIG_STRING_PMCH_CONFIGURATION_LIST,NULL,0};
  	sprintf(mcepath2,"%s.[%i].%s.%s.[%i]","MCEs",i,MCE_CONFIG_STRING_MBMS_SCHEDULING_INFO,MCE_CONFIG_STRING_MBMS_AREA_CONFIGURATION_LIST,l);
	//LOG_E(MCE_APP,"%s\n",mcepath2);
  	config_getlist(&PMCH_ParamList,PMCH_Params,sizeof(PMCH_Params)/sizeof(paramdef_t),mcepath2);
  	AssertFatal(PMCH_ParamList.numelt <= 8, "File wrong parsed\n");
	m2ap_mbms_scheduling_information->mbms_area_config_list[l].num_pmch_config_list=PMCH_ParamList.numelt;
	for(j = 0; j < PMCH_ParamList.numelt; j++){
	m2ap_mbms_scheduling_information->mbms_area_config_list[l].pmch_config_list[j].allocated_sf_end =	*(PMCH_ParamList.paramarray[j][MCE_CONFIG_STRING_ALLOCATED_SF_END_IDX].uptr);
		m2ap_mbms_scheduling_information->mbms_area_config_list[l].pmch_config_list[j].data_mcs = *(PMCH_ParamList.paramarray[j][MCE_CONFIG_STRING_DATA_MCS_IDX].uptr);
	m2ap_mbms_scheduling_information->mbms_area_config_list[l].pmch_config_list[j].mch_scheduling_period= *(PMCH_ParamList.paramarray[j][MCE_CONFIG_STRING_MCH_SCHEDULING_PERIOD_IDX].uptr);
  		char mcepath3[MAX_OPTNAME_SIZE*2 + 8];
		paramdef_t MBMS_SESSION_LIST_Params[] = MCE_MBMS_MBMS_SESSION_LIST_DESC;
		paramlist_def_t MBMS_SESSION_LIST_ParamList = {MCE_CONFIG_STRING_MBMS_SESSION_LIST,NULL,0};
  		sprintf(mcepath3,"%s.[%i].%s.%s.[%i].%s.[%i]","MCEs",i,MCE_CONFIG_STRING_MBMS_SCHEDULING_INFO,MCE_CONFIG_STRING_MBMS_AREA_CONFIGURATION_LIST,l,MCE_CONFIG_STRING_PMCH_CONFIGURATION_LIST,j);
  		config_getlist(&MBMS_SESSION_LIST_ParamList,MBMS_SESSION_LIST_Params,sizeof(MBMS_SESSION_LIST_Params)/sizeof(paramdef_t),mcepath3);
		//LOG_E(MCE_APP,"%s ---- %d\n",mcepath3, MBMS_SESSION_LIST_ParamList.numelt);
  		AssertFatal(MBMS_SESSION_LIST_ParamList.numelt <= 8, "File wrong parsed\n");
		m2ap_mbms_scheduling_information->mbms_area_config_list[l].pmch_config_list[j].num_mbms_session_list = MBMS_SESSION_LIST_ParamList.numelt;
		for(k = 0; k < MBMS_SESSION_LIST_ParamList.numelt; k++ ){

  			//char mcepath4[MAX_OPTNAME_SIZE*8 + 8];
			//paramdef_t MBMS_SESSION_LIST_PER_PMCH_Params[] = MCE_MBMS_MBMS_SESSION_LIST_PER_PMCH_PARAMS_DESC;
			//paramlist_def_t MBMS_SESSION_LIST_PER_PMCH_ParamList = {MCE_CONFIG_STRING_MBMS_SESSION_LIST_PER_PMCH,NULL,0};
	        	//sprintf(mcepath4,"%s.[%i].%s.%s.[%i].%s.[%i].%s.[%i]","MCEs",i,MCE_CONFIG_STRING_MBMS_SCHEDULING_INFO,MCE_CONFIG_STRING_MBMS_AREA_CONFIGURATION_LIST,l,MCE_CONFIG_STRING_PMCH_CONFIGURATION_LIST,j,MCE_CONFIG_STRING_MBMS_SESSION_LIST,k);
			//LOG_E(MCE_APP,"%s\n",mcepath4);
			//config_getlist(&MBMS_SESSION_LIST_PER_PMCH_ParamList,MBMS_SESSION_LIST_PER_PMCH_Params,sizeof(MBMS_SESSION_LIST_PER_PMCH_Params)/sizeof(paramdef_t),mcepath4);
			//m2ap_mbms_scheduling_information->mbms_area_config_list[l].pmch_config_list[j].mbms_session_list[k].num_mbms_session_per_pmch = MBMS_SESSION_LIST_PER_PMCH_ParamList.numelt;
			//for(m = 0; m < MBMS_SESSION_LIST_PER_PMCH_ParamList.numelt; m++){

			m2ap_mbms_scheduling_information->mbms_area_config_list[l].pmch_config_list[j].mbms_session_list[k].service_id = 	
			*(MBMS_SESSION_LIST_ParamList.paramarray[k][MCE_CONFIG_STRING_MBMS_SERVICE_ID_IDX].uptr);
			uint32_t lcid =*(MBMS_SESSION_LIST_ParamList.paramarray[k][MCE_CONFIG_STRING_MBMS_LCID_IDX].uptr);
			m2ap_mbms_scheduling_information->mbms_area_config_list[l].pmch_config_list[j].mbms_session_list[k].lcid =lcid;	
			//uint32_t service_id2=*(MBMS_SESSION_LIST_ParamList.paramarray[k][MCE_CONFIG_STRING_MBMS_LCID_IDX].uptr);
			//m2ap_mbms_scheduling_information->mbms_area_config_list[l].pmch_config_list[j].mbms_session_list[k].service_id2 =service_id2;
			//LOG_E(MCE_APP,"lcid %ld\n",*(MBMS_SESSION_LIST_ParamList.paramarray[k][MCE_CONFIG_STRING_MBMS_LCID_IDX].uptr));
			//LOG_E(MCE_APP,"service_id %d\n",*(MBMS_SESSION_LIST_ParamList.paramarray[k][MCE_CONFIG_STRING_MBMS_SERVICE_ID_IDX].uptr));

			char mcepath5[MAX_OPTNAME_SIZE*8 + 8];
			paramdef_t PLNMParams[]  = MCE_PLMN_PARAMS_DESC;
			sprintf(mcepath5,"%s.[%i].%s.%s.[%i].%s.[%i].%s.[%i].%s","MCEs",i,MCE_CONFIG_STRING_MBMS_SCHEDULING_INFO,MCE_CONFIG_STRING_MBMS_AREA_CONFIGURATION_LIST,l,MCE_CONFIG_STRING_PMCH_CONFIGURATION_LIST,j,MCE_CONFIG_STRING_MBMS_SESSION_LIST,k,MCE_CONFIG_STRING_PLMN);
			config_get(PLNMParams,sizeof(PLNMParams)/sizeof(paramdef_t),mcepath5);
			//LOG_E(MCE_APP,"PLNM %d\n",*(PLNMParams[MCE_CONFIG_STRING_MNC_LENGTH_IDX].uptr));
			m2ap_mbms_scheduling_information->mbms_area_config_list[l].pmch_config_list[j].mbms_session_list[k].mcc = 	*(PLNMParams[MCE_CONFIG_STRING_MCC_IDX].uptr);
			m2ap_mbms_scheduling_information->mbms_area_config_list[l].pmch_config_list[j].mbms_session_list[k].mnc = 	*(PLNMParams[MCE_CONFIG_STRING_MNC_IDX].uptr);
			m2ap_mbms_scheduling_information->mbms_area_config_list[l].pmch_config_list[j].mbms_session_list[k].mnc_length = 	*(PLNMParams[MCE_CONFIG_STRING_MNC_LENGTH_IDX].uptr);
			
			//}
			//LOG_E(MCE_APP,"MBMS_SESSION_LIST_PER_PMCH_ParamList.numelt %d\n",MBMS_SESSION_LIST_PER_PMCH_ParamList.numelt);
			//AssertFatal(MBMS_SESSION_LIST_PER_PMCH_ParamList.numelt <= 8, "File wrong parsed\n");

			//printf("%d\n",*(MBMS_SESSION_LIST_ParamList.paramarray[k][MCE_CONFIG_STRING_PLNM_IDENTITY_IDX].uptr));
			//printf("%d\n",*(MBMS_SESSION_LIST_ParamList.paramarray[k][MCE_CONFIG_STRING_SERVICE_ID_IDX].uptr));
			//printf("%d\n",*(MBMS_SESSION_LIST_ParamList.paramarray[k][MCE_CONFIG_STRING_LCID_IDX].uptr));
			//*((MBMS_SESSION_LIST_ParamList[k][].uptr);
			//*((MBMS_SESSION_LIST_ParamList[k][].uptr);
			//*((MBMS_SESSION_LIST_ParamList[k][].uptr);
			//*((MBMS_SESSION_LIST_ParamList[k][].uptr);
			//*((MBMS_SESSION_LIST_ParamList[k][].uptr);
		}
	}
	paramdef_t MBSFN_SF_Params[]  = MCE_MBMS_MBMS_SF_CONFIGURATION_LIST_PARAMS_DESC;
  	paramlist_def_t MBSFN_SF_ParamList = {MCE_CONFIG_STRING_MBMS_SF_CONFIGURATION_LIST,NULL,0};
  	sprintf(mcepath2,"%s.[%i].%s.%s.[%i]","MCEs",i,MCE_CONFIG_STRING_MBMS_SCHEDULING_INFO,MCE_CONFIG_STRING_MBMS_AREA_CONFIGURATION_LIST,l);
	//LOG_E(MCE_APP,"%s\n",mcepath2);
  	config_getlist(&MBSFN_SF_ParamList,MBSFN_SF_Params,sizeof(MBSFN_SF_Params)/sizeof(paramdef_t),mcepath2);
  	AssertFatal(MBSFN_SF_ParamList.numelt <= 8, "File wrong parsed\n");

	m2ap_mbms_scheduling_information->mbms_area_config_list[l].num_mbms_sf_config_list=MBSFN_SF_ParamList.numelt;
	for(j = 0; j < MBSFN_SF_ParamList.numelt; j++){
		m2ap_mbms_scheduling_information->mbms_area_config_list[l].mbms_sf_config_list[j].radioframe_allocation_period=*(MBSFN_SF_ParamList.paramarray[j][MCE_CONFIG_STRING_RADIOFRAME_ALLOCATION_PERIOD_IDX].uptr);
		m2ap_mbms_scheduling_information->mbms_area_config_list[l].mbms_sf_config_list[j].radioframe_allocation_offset = *(MBSFN_SF_ParamList.paramarray[j][MCE_CONFIG_STRING_RADIOFRAME_ALLOOCATION_OFFSET_IDX].uptr);
		if(strcmp(*(MBSFN_SF_ParamList.paramarray[j][MCE_CONFIG_STRING_NUM_FRAME_IDX].strptr),"oneFrame")==0){
			m2ap_mbms_scheduling_information->mbms_area_config_list[l].mbms_sf_config_list[j].is_four_sf = 0;
		}else{
			m2ap_mbms_scheduling_information->mbms_area_config_list[l].mbms_sf_config_list[j].is_four_sf = 1;
		}
		m2ap_mbms_scheduling_information->mbms_area_config_list[l].mbms_sf_config_list[j].subframe_allocation = *(MBSFN_SF_ParamList.paramarray[j][MCE_CONFIG_STRING_SUBFRAME_ALLOCATION_IDX].uptr);
	}


  }
	return 0;
}
int RCconfig_M2_SCHEDULING(MessageDef *msg_p, uint32_t i) {
  int l,j,k/*,m*/;
  char mcepath[MAX_OPTNAME_SIZE*2 + 8];
  paramdef_t MBMS_SCHE_Params[]  = MCE_MBMS_SCHEDULING_INFO_PARAMS_DESC;
  sprintf(mcepath,"%s.[%i].%s","MCEs",0,MCE_CONFIG_STRING_MBMS_SCHEDULING_INFO);
  //LOG_E(MCE_APP,"%s\n",mcepath);
  config_get(MBMS_SCHE_Params,sizeof(MBMS_SCHE_Params)/sizeof(paramdef_t),mcepath);
  //LOG_E(MCE_APP,"%s %d\n",mcepath, *(MBMS_SCHE_Params[MCE_CONFIG_STRING_MCCH_UPDATE_TIME_IDX].uptr));
  M2AP_MBMS_SCHEDULING_INFORMATION(msg_p).mcch_update_time=*(MBMS_SCHE_Params[MCE_CONFIG_STRING_MCCH_UPDATE_TIME_IDX].uptr);

  paramdef_t MBMS_CONFIGURATION_Params[] = MCE_MBMS_AREA_CONFIGURATION_LIST_PARAMS_DESC;
  paramlist_def_t MBMS_AREA_CONFIGURATION_ParamList = {MCE_CONFIG_STRING_MBMS_AREA_CONFIGURATION_LIST,NULL,0};
  sprintf(mcepath,"%s.[%i].%s","MCEs",0,MCE_CONFIG_STRING_MBMS_SCHEDULING_INFO);
  config_getlist(&MBMS_AREA_CONFIGURATION_ParamList,MBMS_CONFIGURATION_Params,sizeof(MBMS_CONFIGURATION_Params)/sizeof(paramdef_t),mcepath);
  AssertFatal(MBMS_AREA_CONFIGURATION_ParamList.numelt <= 8, "File wrong parsed\n");
  //LOG_E(MCE_APP,"%s\n",mcepath);
  //LOG_E(MCE_APP,"MBMS_AREA_CONFIGURATION_ParamList.numelt %d\n",MBMS_AREA_CONFIGURATION_ParamList.numelt);

  M2AP_MBMS_SCHEDULING_INFORMATION(msg_p).num_mbms_area_config_list = MBMS_AREA_CONFIGURATION_ParamList.numelt;
  for (l = 0; l < MBMS_AREA_CONFIGURATION_ParamList.numelt; l++) {
	M2AP_MBMS_SCHEDULING_INFORMATION(msg_p).mbms_area_config_list[l].common_sf_allocation_period = *(MBMS_AREA_CONFIGURATION_ParamList.paramarray[l][MCE_CONFIG_STRING_COMMON_SF_ALLOCATION_PERIOD_IDX].uptr);
	M2AP_MBMS_SCHEDULING_INFORMATION(msg_p).mbms_area_config_list[l].mbms_area_id=*(MBMS_AREA_CONFIGURATION_ParamList.paramarray[l][MCE_CONFIG_STRING_MBMS_AREA_ID_IDX].uptr);
  	char mcepath2[MAX_OPTNAME_SIZE*2 + 8];
  	paramdef_t PMCH_Params[]  = MCE_MBMS_PMCH_CONFIGURATION_LIST_PARAMS_DESC;
  	paramlist_def_t PMCH_ParamList = {MCE_CONFIG_STRING_PMCH_CONFIGURATION_LIST,NULL,0};
  	sprintf(mcepath2,"%s.[%i].%s.%s.[%i]","MCEs",i,MCE_CONFIG_STRING_MBMS_SCHEDULING_INFO,MCE_CONFIG_STRING_MBMS_AREA_CONFIGURATION_LIST,l);
	//LOG_E(MCE_APP,"%s\n",mcepath2);
  	config_getlist(&PMCH_ParamList,PMCH_Params,sizeof(PMCH_Params)/sizeof(paramdef_t),mcepath2);
  	AssertFatal(PMCH_ParamList.numelt <= 8, "File wrong parsed\n");
	M2AP_MBMS_SCHEDULING_INFORMATION(msg_p).mbms_area_config_list[l].num_pmch_config_list=PMCH_ParamList.numelt;
	for(j = 0; j < PMCH_ParamList.numelt; j++){
	M2AP_MBMS_SCHEDULING_INFORMATION(msg_p).mbms_area_config_list[l].pmch_config_list[j].allocated_sf_end =	*(PMCH_ParamList.paramarray[j][MCE_CONFIG_STRING_ALLOCATED_SF_END_IDX].uptr);
		M2AP_MBMS_SCHEDULING_INFORMATION(msg_p).mbms_area_config_list[l].pmch_config_list[j].data_mcs = *(PMCH_ParamList.paramarray[j][MCE_CONFIG_STRING_DATA_MCS_IDX].uptr);
	M2AP_MBMS_SCHEDULING_INFORMATION(msg_p).mbms_area_config_list[l].pmch_config_list[j].mch_scheduling_period= *(PMCH_ParamList.paramarray[j][MCE_CONFIG_STRING_MCH_SCHEDULING_PERIOD_IDX].uptr);
  		char mcepath3[MAX_OPTNAME_SIZE*2 + 8];
		paramdef_t MBMS_SESSION_LIST_Params[] = MCE_MBMS_MBMS_SESSION_LIST_DESC;
		paramlist_def_t MBMS_SESSION_LIST_ParamList = {MCE_CONFIG_STRING_MBMS_SESSION_LIST,NULL,0};
  		sprintf(mcepath3,"%s.[%i].%s.%s.[%i].%s.[%i]","MCEs",i,MCE_CONFIG_STRING_MBMS_SCHEDULING_INFO,MCE_CONFIG_STRING_MBMS_AREA_CONFIGURATION_LIST,l,MCE_CONFIG_STRING_PMCH_CONFIGURATION_LIST,j);
  		config_getlist(&MBMS_SESSION_LIST_ParamList,MBMS_SESSION_LIST_Params,sizeof(MBMS_SESSION_LIST_Params)/sizeof(paramdef_t),mcepath3);
		//LOG_E(MCE_APP,"%s ---- %d\n",mcepath3, MBMS_SESSION_LIST_ParamList.numelt);
  		AssertFatal(MBMS_SESSION_LIST_ParamList.numelt <= 8, "File wrong parsed\n");
		M2AP_MBMS_SCHEDULING_INFORMATION(msg_p).mbms_area_config_list[l].pmch_config_list[j].num_mbms_session_list = MBMS_SESSION_LIST_ParamList.numelt;
		for(k = 0; k < MBMS_SESSION_LIST_ParamList.numelt; k++ ){

  			//char mcepath4[MAX_OPTNAME_SIZE*8 + 8];
			//paramdef_t MBMS_SESSION_LIST_PER_PMCH_Params[] = MCE_MBMS_MBMS_SESSION_LIST_PER_PMCH_PARAMS_DESC;
			//paramlist_def_t MBMS_SESSION_LIST_PER_PMCH_ParamList = {MCE_CONFIG_STRING_MBMS_SESSION_LIST_PER_PMCH,NULL,0};
	        	//sprintf(mcepath4,"%s.[%i].%s.%s.[%i].%s.[%i].%s.[%i]","MCEs",i,MCE_CONFIG_STRING_MBMS_SCHEDULING_INFO,MCE_CONFIG_STRING_MBMS_AREA_CONFIGURATION_LIST,l,MCE_CONFIG_STRING_PMCH_CONFIGURATION_LIST,j,MCE_CONFIG_STRING_MBMS_SESSION_LIST,k);
			//LOG_E(MCE_APP,"%s\n",mcepath4);
			//config_getlist(&MBMS_SESSION_LIST_PER_PMCH_ParamList,MBMS_SESSION_LIST_PER_PMCH_Params,sizeof(MBMS_SESSION_LIST_PER_PMCH_Params)/sizeof(paramdef_t),mcepath4);
			//M2AP_MBMS_SCHEDULING_INFORMATION(msg_p).mbms_area_config_list[l].pmch_config_list[j].mbms_session_list[k].num_mbms_session_per_pmch = MBMS_SESSION_LIST_PER_PMCH_ParamList.numelt;
			//for(m = 0; m < MBMS_SESSION_LIST_PER_PMCH_ParamList.numelt; m++){

			M2AP_MBMS_SCHEDULING_INFORMATION(msg_p).mbms_area_config_list[l].pmch_config_list[j].mbms_session_list[k].service_id = 	
			*(MBMS_SESSION_LIST_ParamList.paramarray[k][MCE_CONFIG_STRING_MBMS_SERVICE_ID_IDX].uptr);
			uint32_t lcid =*(MBMS_SESSION_LIST_ParamList.paramarray[k][MCE_CONFIG_STRING_MBMS_LCID_IDX].uptr);
			M2AP_MBMS_SCHEDULING_INFORMATION(msg_p).mbms_area_config_list[l].pmch_config_list[j].mbms_session_list[k].lcid =lcid;	
			//uint32_t service_id2=*(MBMS_SESSION_LIST_ParamList.paramarray[k][MCE_CONFIG_STRING_MBMS_LCID_IDX].uptr);
			//M2AP_MBMS_SCHEDULING_INFORMATION(msg_p).mbms_area_config_list[l].pmch_config_list[j].mbms_session_list[k].service_id2 =service_id2;
			//LOG_E(MCE_APP,"lcid %ld\n",*(MBMS_SESSION_LIST_ParamList.paramarray[k][MCE_CONFIG_STRING_MBMS_LCID_IDX].uptr));
			//LOG_E(MCE_APP,"service_id %d\n",*(MBMS_SESSION_LIST_ParamList.paramarray[k][MCE_CONFIG_STRING_MBMS_SERVICE_ID_IDX].uptr));

			char mcepath5[MAX_OPTNAME_SIZE*8 + 8];
			paramdef_t PLNMParams[]  = MCE_PLMN_PARAMS_DESC;
			sprintf(mcepath5,"%s.[%i].%s.%s.[%i].%s.[%i].%s.[%i].%s","MCEs",i,MCE_CONFIG_STRING_MBMS_SCHEDULING_INFO,MCE_CONFIG_STRING_MBMS_AREA_CONFIGURATION_LIST,l,MCE_CONFIG_STRING_PMCH_CONFIGURATION_LIST,j,MCE_CONFIG_STRING_MBMS_SESSION_LIST,k,MCE_CONFIG_STRING_PLMN);
			config_get(PLNMParams,sizeof(PLNMParams)/sizeof(paramdef_t),mcepath5);
			//LOG_E(MCE_APP,"PLNM %d\n",*(PLNMParams[MCE_CONFIG_STRING_MNC_LENGTH_IDX].uptr));
			M2AP_MBMS_SCHEDULING_INFORMATION(msg_p).mbms_area_config_list[l].pmch_config_list[j].mbms_session_list[k].mcc = 	*(PLNMParams[MCE_CONFIG_STRING_MCC_IDX].uptr);
			M2AP_MBMS_SCHEDULING_INFORMATION(msg_p).mbms_area_config_list[l].pmch_config_list[j].mbms_session_list[k].mnc = 	*(PLNMParams[MCE_CONFIG_STRING_MNC_IDX].uptr);
			M2AP_MBMS_SCHEDULING_INFORMATION(msg_p).mbms_area_config_list[l].pmch_config_list[j].mbms_session_list[k].mnc_length = 	*(PLNMParams[MCE_CONFIG_STRING_MNC_LENGTH_IDX].uptr);
			
			//}
			//LOG_E(MCE_APP,"MBMS_SESSION_LIST_PER_PMCH_ParamList.numelt %d\n",MBMS_SESSION_LIST_PER_PMCH_ParamList.numelt);
			//AssertFatal(MBMS_SESSION_LIST_PER_PMCH_ParamList.numelt <= 8, "File wrong parsed\n");

			//printf("%d\n",*(MBMS_SESSION_LIST_ParamList.paramarray[k][MCE_CONFIG_STRING_PLNM_IDENTITY_IDX].uptr));
			//printf("%d\n",*(MBMS_SESSION_LIST_ParamList.paramarray[k][MCE_CONFIG_STRING_SERVICE_ID_IDX].uptr));
			//printf("%d\n",*(MBMS_SESSION_LIST_ParamList.paramarray[k][MCE_CONFIG_STRING_LCID_IDX].uptr));
			//*((MBMS_SESSION_LIST_ParamList[k][].uptr);
			//*((MBMS_SESSION_LIST_ParamList[k][].uptr);
			//*((MBMS_SESSION_LIST_ParamList[k][].uptr);
			//*((MBMS_SESSION_LIST_ParamList[k][].uptr);
			//*((MBMS_SESSION_LIST_ParamList[k][].uptr);
		}
	}
	paramdef_t MBSFN_SF_Params[]  = MCE_MBMS_MBMS_SF_CONFIGURATION_LIST_PARAMS_DESC;
  	paramlist_def_t MBSFN_SF_ParamList = {MCE_CONFIG_STRING_MBMS_SF_CONFIGURATION_LIST,NULL,0};
  	sprintf(mcepath2,"%s.[%i].%s.%s.[%i]","MCEs",i,MCE_CONFIG_STRING_MBMS_SCHEDULING_INFO,MCE_CONFIG_STRING_MBMS_AREA_CONFIGURATION_LIST,l);
	//LOG_E(MCE_APP,"%s\n",mcepath2);
  	config_getlist(&MBSFN_SF_ParamList,MBSFN_SF_Params,sizeof(MBSFN_SF_Params)/sizeof(paramdef_t),mcepath2);
  	AssertFatal(MBSFN_SF_ParamList.numelt <= 8, "File wrong parsed\n");

	M2AP_MBMS_SCHEDULING_INFORMATION(msg_p).mbms_area_config_list[l].num_mbms_sf_config_list=MBSFN_SF_ParamList.numelt;
	for(j = 0; j < MBSFN_SF_ParamList.numelt; j++){
		M2AP_MBMS_SCHEDULING_INFORMATION(msg_p).mbms_area_config_list[l].mbms_sf_config_list[j].radioframe_allocation_period=*(MBSFN_SF_ParamList.paramarray[j][MCE_CONFIG_STRING_RADIOFRAME_ALLOCATION_PERIOD_IDX].uptr);
		M2AP_MBMS_SCHEDULING_INFORMATION(msg_p).mbms_area_config_list[l].mbms_sf_config_list[j].radioframe_allocation_offset = *(MBSFN_SF_ParamList.paramarray[j][MCE_CONFIG_STRING_RADIOFRAME_ALLOOCATION_OFFSET_IDX].uptr);
		if(strcmp(*(MBSFN_SF_ParamList.paramarray[j][MCE_CONFIG_STRING_NUM_FRAME_IDX].strptr),"oneFrame")==0){
			M2AP_MBMS_SCHEDULING_INFORMATION(msg_p).mbms_area_config_list[l].mbms_sf_config_list[j].is_four_sf = 0;
		}else{
			M2AP_MBMS_SCHEDULING_INFORMATION(msg_p).mbms_area_config_list[l].mbms_sf_config_list[j].is_four_sf = 1;
		}
		M2AP_MBMS_SCHEDULING_INFORMATION(msg_p).mbms_area_config_list[l].mbms_sf_config_list[j].subframe_allocation = *(MBSFN_SF_ParamList.paramarray[j][MCE_CONFIG_STRING_SUBFRAME_ALLOCATION_IDX].uptr);
	}


  }

  return 0;
}

