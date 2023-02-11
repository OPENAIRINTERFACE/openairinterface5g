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

#include <string.h>
#include <inttypes.h>
#include <arpa/inet.h>
#include "common/utils/LOG/log.h"
#include "common/utils/nr/nr_common.h"
#include "common/utils/LOG/log_extern.h"
#include "assertions.h"
#include "common/utils/ocp_itti/intertask_interface.h"
#include "openair2/GNB_APP/gnb_paramdef.h"
#include "openair3/ocp-gtpu/gtp_itf.h"

static void get_NGU_S1U_addr(char **addr, uint16_t *port)
{
  int num_gnbs = 0;
  char *gnb_ipv4_address_for_NGU = NULL;
  uint32_t gnb_port_for_NGU = 0;
  char *gnb_ipv4_address_for_S1U = NULL;
  uint32_t gnb_port_for_S1U = 0;
  char gtpupath[MAX_OPTNAME_SIZE * 2 + 8];

  paramdef_t GNBSParams[] = GNBSPARAMS_DESC;
  paramdef_t NETParams[] = GNBNETPARAMS_DESC;
  LOG_I(GTPU, "Configuring GTPu\n");

  /* get number of active eNodeBs */
  config_get(GNBSParams, sizeof(GNBSParams) / sizeof(paramdef_t), NULL);
  num_gnbs = GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt;
  AssertFatal(num_gnbs > 0, "Failed to parse config file no active gNodeBs in %s \n", GNB_CONFIG_STRING_ACTIVE_GNBS);

  sprintf(gtpupath, "%s.[%i].%s", GNB_CONFIG_STRING_GNB_LIST, 0, GNB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);
  config_get(NETParams, sizeof(NETParams) / sizeof(paramdef_t), gtpupath);
  char *address;
  if (NETParams[1].strptr != NULL) {
    LOG_I(GTPU, "SA mode \n");
    address = strdup(gnb_ipv4_address_for_NGU);
    *port = gnb_port_for_NGU;
  } else {
    LOG_I(GTPU, "NSA mode \n");
    address = strdup(gnb_ipv4_address_for_S1U);
    *port = gnb_port_for_S1U;
  }
  if (strchr(address, '/'))
    *strchr(address, '/') = 0;
  *addr = address;
  return;
}

MessageDef *RCconfig_NR_CU_E1(bool separate_CUUP_process)
{
  MessageDef *msgConfig = itti_alloc_new_message(TASK_GNB_APP, 0, E1AP_SETUP_REQ);
  if (!msgConfig)
    return NULL;

  paramdef_t GNBSParams[] = GNBSPARAMS_DESC;
  paramdef_t GNBParams[] = GNBPARAMS_DESC;
  paramlist_def_t GNBParamList = {GNB_CONFIG_STRING_GNB_LIST, NULL, 0};
  config_get(GNBSParams, sizeofArray(GNBSParams), NULL);
  char aprefix[MAX_OPTNAME_SIZE * 2 + 8];
  sprintf(aprefix, "%s.[%i]", GNB_CONFIG_STRING_GNB_LIST, 0);
  int num_gnbs = GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt;
  AssertFatal(num_gnbs == 1, "Support only one gNB per process\n");

  if (num_gnbs > 0) {
    config_getlist(&GNBParamList, GNBParams, sizeofArray(GNBParams), NULL);
    paramdef_t *gnbParms = GNBParamList.paramarray[0];
    AssertFatal(gnbParms[GNB_GNB_ID_IDX].uptr != NULL, "gNB id %u is not defined in configuration file\n", 0);
    e1ap_setup_req_t *e1Setup = &E1AP_SETUP_REQ(msgConfig);
    msgConfig->ittiMsgHeader.destinationInstance = 0;
    e1Setup->gNB_cu_up_id = *(gnbParms[GNB_GNB_ID_IDX].uptr);

    paramdef_t PLMNParams[] = GNBPLMNPARAMS_DESC;
    paramlist_def_t PLMNParamList = {GNB_CONFIG_STRING_PLMN_LIST, NULL, 0};
    /* map parameter checking array instances to parameter definition array instances */
    checkedparam_t config_check_PLMNParams[] = PLMNPARAMS_CHECK;

    for (int I = 0; I < sizeofArray(PLMNParams); ++I)
      PLMNParams[I].chkPptr = &(config_check_PLMNParams[I]);

    config_getlist(&PLMNParamList, PLMNParams, sizeofArray(PLMNParams), aprefix);
    int numPLMNs = PLMNParamList.numelt;
    e1Setup->supported_plmns = numPLMNs;

    for (int I = 0; I < numPLMNs; I++) {
      e1Setup->plmns[I].mcc = *PLMNParamList.paramarray[I][GNB_MOBILE_COUNTRY_CODE_IDX].uptr;
      e1Setup->plmns[I].mnc = *PLMNParamList.paramarray[I][GNB_MOBILE_NETWORK_CODE_IDX].uptr;
      e1Setup->plmns[I].mnc = *PLMNParamList.paramarray[I][GNB_MNC_DIGIT_LENGTH].u8ptr;
    }

    e1Setup->remotePortF1U = *(GNBParamList.paramarray[0][GNB_REMOTE_S_PORTD_IDX].uptr);
    e1Setup->localAddressF1U = strdup(*(GNBParamList.paramarray[0][GNB_LOCAL_S_ADDRESS_IDX].strptr));
    e1Setup->localPortF1U = *(GNBParamList.paramarray[0][GNB_LOCAL_S_PORTD_IDX].uptr);
    get_NGU_S1U_addr(&e1Setup->localAddressN3, &e1Setup->localPortN3);
    e1Setup->remotePortN3 = GTPV1_U_PORT_NUMBER;

    if (separate_CUUP_process) {
      paramlist_def_t GNBE1ParamList = {GNB_CONFIG_STRING_E1_PARAMETERS, NULL, 0};
      paramdef_t GNBE1Params[] = GNBE1PARAMS_DESC;
      config_getlist(&GNBE1ParamList, GNBE1Params, sizeofArray(GNBE1Params), aprefix);
      paramdef_t *e1Parms = GNBE1ParamList.paramarray[0];
      strcpy(e1Setup->CUCP_e1_ip_address.ipv4_address, *(e1Parms[GNB_CONFIG_E1_IPV4_ADDRESS_CUCP].strptr));
      e1Setup->CUCP_e1_ip_address.ipv4 = 1;
      strcpy(e1Setup->CUUP_e1_ip_address.ipv4_address, *(e1Parms[GNB_CONFIG_E1_IPV4_ADDRESS_CUUP].strptr));
      e1Setup->CUUP_e1_ip_address.ipv4 = 1;
      e1Setup->cn_support = *e1Parms[GNB_CONFIG_E1_CN_SUPPORT].uptr;
    }
  }
  return msgConfig;
}
