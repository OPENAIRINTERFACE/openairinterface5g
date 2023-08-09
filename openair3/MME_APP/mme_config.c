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
  mme_config.c
  -------------------
  AUTHOR  : Javier Morgade
  COMPANY : VICOMTECH, Spain
  EMAIL   : javier.morgade@ieee.org
*/

#include <string.h>
#include <inttypes.h>

#include "common/utils/LOG/log.h"
#include "assertions.h"
#include "mme_config.h"
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
#include "radio/ETHERNET/ethernet_lib.h"
#include "nfapi_vnf.h"
#include "nfapi_pnf.h"

#include "L1_paramdef.h"
#include "MACRLC_paramdef.h"
#include "common/config/config_userapi.h"
#include "RRC_config_tools.h"
#include "enb_paramdef.h"

int RCconfig_MME(void ) {
  //int               num_enbs                      = 0;
  //char             *enb_interface_name_for_S1U    = NULL;
  char               *mme_interface_name_for_m3_mce = NULL;
  //char             *enb_ipv4_address_for_S1U      = NULL;
  char               *mme_ipv4_address_for_m3c      = NULL;
  //uint32_t          enb_port_for_S1U              = 0;
  uint32_t            mme_port_for_m3c              = 0;
  char             *address                       = NULL;
  char             *cidr                          = NULL;
  //char gtpupath[MAX_OPTNAME_SIZE*2 + 8];
  char mmepath[MAX_OPTNAME_SIZE*2 + 8];
  //paramdef_t ENBSParams[] = ENBSPARAMS_DESC;
  //paramdef_t GTPUParams[]  = GTPUPARAMS_DESC;

  paramdef_t   MMEParams[] = MME_NETPARAMS_DESC;

  ///* get number of active eNodeBs */
  // config_get( config_get_if(),ENBSParams,sizeofArray(ENBSParams),NULL);
  // num_enbs = ENBSParams[ENB_ACTIVE_ENBS_IDX].numelt;
  // AssertFatal (num_enbs >0,
  //              "Failed to parse config file no active eNodeBs in %s \n", ENB_CONFIG_STRING_ACTIVE_ENBS);
  // sprintf(gtpupath,"%s.[%i].%s",ENB_CONFIG_STRING_ENB_LIST,0,ENB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);
  sprintf(mmepath,"%s.[%i].%s","MMEs",0,MME_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);
  // config_get(config_get_if(), GTPUParams,sizeofArray(GTPUParams),gtpupath);
  config_get(config_get_if(), MMEParams, sizeofArray(MMEParams), mmepath);
  //cidr = enb_ipv4_address_for_S1U;
  cidr = mme_ipv4_address_for_m3c;
  address = strtok(cidr, "/");

  //LOG_W(MME_APP,"cidr %s\n",cidr);
  //LOG_W(MME_APP,"address %s\n",address);
  //LOG_W(MME_APP,"mme_interface_name_for_m3_mce %s\n",mme_interface_name_for_m3_mce);
  //LOG_W(MME_APP,"mme_ipv4_address_for_m3c %s\n",mme_ipv4_address_for_m3c);
  //LOG_W(MME_APP,"mme_port_for_m3c %d\n",mme_port_for_m3c);

  if (address) {
    MessageDef *message;
    AssertFatal((message = itti_alloc_new_message(TASK_MME_APP, 0, M3AP_MME_SCTP_REQ))!=NULL,"");
    M3AP_MME_SCTP_REQ (message).mme_m3_ip_address.ipv6 = 0;
    M3AP_MME_SCTP_REQ (message).mme_m3_ip_address.ipv4 = 1;
    strcpy( M3AP_MME_SCTP_REQ (message).mme_m3_ip_address.ipv4_address, address);
    LOG_I(MME_APP,"Configuring M3_C address : %s\n",address/*,M3AP_MME_SCTP_REQ(message).mme_m3_ip_address*/);
    M3AP_MME_SCTP_REQ(message).mme_port_for_M3C = mme_port_for_m3c;
    itti_send_msg_to_task (TASK_M3AP_MME, 0, message); // data model is wrong: gtpu doesn't have enb_id (or module_id)
  } else
    LOG_E(MCE_APP,"invalid address for M2AP\n");



  return 0;
}

