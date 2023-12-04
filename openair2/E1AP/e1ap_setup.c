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
  config_get(config_get_if(), GNBSParams, sizeofArray(GNBSParams), NULL);
  num_gnbs = GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt;
  AssertFatal(num_gnbs > 0, "Failed to parse config file no active gNodeBs in %s \n", GNB_CONFIG_STRING_ACTIVE_GNBS);

  sprintf(gtpupath, "%s.[%i].%s", GNB_CONFIG_STRING_GNB_LIST, 0, GNB_CONFIG_STRING_NETWORK_INTERFACES_CONFIG);
  config_get(config_get_if(), NETParams, sizeofArray(NETParams), gtpupath);
  char *address;
  if (NETParams[1].strptr != NULL) {
    LOG_I(GTPU, "SA mode \n");
    AssertFatal(gnb_ipv4_address_for_NGU != NULL, "NG-U IPv4 address is NULL: could not read IPv4 address\n");
    address = strdup(gnb_ipv4_address_for_NGU);
    *port = gnb_port_for_NGU;
  } else {
    LOG_I(GTPU, "NSA mode \n");
    AssertFatal(gnb_ipv4_address_for_S1U != NULL, "S1U IPv4 address is NULL: could not read IPv4 address\n");
    address = strdup(gnb_ipv4_address_for_S1U);
    *port = gnb_port_for_S1U;
  }
  if (strchr(address, '/'))
    *strchr(address, '/') = 0;
  *addr = address;
  return;
}

MessageDef *RCconfig_NR_CU_E1(const E1_t *entity)
{
  MessageDef *msgConfig = itti_alloc_new_message(TASK_GNB_APP, 0, E1AP_REGISTER_REQ);
  if (!msgConfig)
    return NULL;

  paramdef_t GNBSParams[] = GNBSPARAMS_DESC;
  paramdef_t GNBParams[] = GNBPARAMS_DESC;
  paramlist_def_t GNBParamList = {GNB_CONFIG_STRING_GNB_LIST, NULL, 0};
  config_get(config_get_if(), GNBSParams, sizeofArray(GNBSParams), NULL);
  char aprefix[MAX_OPTNAME_SIZE * 2 + 8];
  sprintf(aprefix, "%s.[%i]", GNB_CONFIG_STRING_GNB_LIST, 0);
  int num_gnbs = GNBSParams[GNB_ACTIVE_GNBS_IDX].numelt;
  AssertFatal(num_gnbs == 1, "Support only one gNB per process\n");
  config_getlist(config_get_if(), &GNBParamList, GNBParams, sizeofArray(GNBParams), NULL);
  paramdef_t *gnbParms = GNBParamList.paramarray[0];
  e1ap_setup_req_t *e1Setup = &E1AP_REGISTER_REQ(msgConfig).setup_req;

  if (num_gnbs > 0) {
    msgConfig->ittiMsgHeader.destinationInstance = 0;
    if (*gnbParms[GNB_GNB_NAME_IDX].strptr)
      e1Setup->gNB_cu_up_name = *(gnbParms[GNB_GNB_NAME_IDX].strptr);

    paramdef_t PLMNParams[] = GNBPLMNPARAMS_DESC;
    paramlist_def_t PLMNParamList = {GNB_CONFIG_STRING_PLMN_LIST, NULL, 0};
    /* map parameter checking array instances to parameter definition array instances */
    checkedparam_t config_check_PLMNParams[] = PLMNPARAMS_CHECK;

    for (int I = 0; I < sizeofArray(PLMNParams); ++I)
      PLMNParams[I].chkPptr = &(config_check_PLMNParams[I]);

    config_getlist(config_get_if(), &PLMNParamList, PLMNParams, sizeofArray(PLMNParams), aprefix);
    int numPLMNs = PLMNParamList.numelt;
    e1Setup->supported_plmns = numPLMNs;

    for (int I = 0; I < numPLMNs; I++) {
      e1Setup->plmn[I].id.mcc = *PLMNParamList.paramarray[I][GNB_MOBILE_COUNTRY_CODE_IDX].uptr;
      e1Setup->plmn[I].id.mnc = *PLMNParamList.paramarray[I][GNB_MOBILE_NETWORK_CODE_IDX].uptr;
      e1Setup->plmn[I].id.mnc_digit_length = *PLMNParamList.paramarray[I][GNB_MNC_DIGIT_LENGTH].uptr;

      char snssaistr[MAX_OPTNAME_SIZE*2 + 8];
      sprintf(snssaistr, "%s.[%i].%s.[%i]", GNB_CONFIG_STRING_GNB_LIST, 0, GNB_CONFIG_STRING_PLMN_LIST, I);
      paramlist_def_t SNSSAIParamList = {GNB_CONFIG_STRING_SNSSAI_LIST, NULL, 0};
      paramdef_t SNSSAIParams[] = GNBSNSSAIPARAMS_DESC;
      config_getlist(config_get_if(), &SNSSAIParamList, SNSSAIParams, sizeof(SNSSAIParams) / sizeof(paramdef_t), snssaistr);
      e1Setup->plmn[I].supported_slices = SNSSAIParamList.numelt;
      e1Setup->plmn[I].slice = calloc(SNSSAIParamList.numelt, sizeof(*e1Setup->plmn[I].slice));
      AssertFatal(e1Setup->plmn[I].slice != NULL, "out of memory\n");
      for (int s = 0; s < SNSSAIParamList.numelt; ++s) {
        e1ap_nssai_t *slice = &e1Setup->plmn[I].slice[s];
        slice->sst = *SNSSAIParamList.paramarray[s][GNB_SLICE_SERVICE_TYPE_IDX].uptr;
        slice->sd = *SNSSAIParamList.paramarray[s][GNB_SLICE_DIFFERENTIATOR_IDX].uptr;
      }
    }

    e1ap_net_config_t *e1ap_nc = &E1AP_REGISTER_REQ(msgConfig).net_config;
    e1ap_nc->remotePortF1U = *(GNBParamList.paramarray[0][GNB_REMOTE_S_PORTD_IDX].uptr);
    e1ap_nc->localAddressF1U = strdup(*(GNBParamList.paramarray[0][GNB_LOCAL_S_ADDRESS_IDX].strptr));
    e1ap_nc->localPortF1U = *(GNBParamList.paramarray[0][GNB_LOCAL_S_PORTD_IDX].uptr);
    get_NGU_S1U_addr(&e1ap_nc->localAddressN3, &e1ap_nc->localPortN3);
    e1ap_nc->remotePortN3 = e1ap_nc->localPortN3 ;

    AssertFatal(config_isparamset(gnbParms, GNB_GNB_ID_IDX), "%s is not defined in configuration file\n", GNB_CONFIG_STRING_GNB_ID);
    uint32_t gnb_id = *gnbParms[GNB_GNB_ID_IDX].uptr;
    E1AP_REGISTER_REQ(msgConfig).gnb_id = gnb_id;

    if (entity != NULL) {
      paramlist_def_t GNBE1ParamList = {GNB_CONFIG_STRING_E1_PARAMETERS, NULL, 0};
      paramdef_t GNBE1Params[] = GNBE1PARAMS_DESC;
      config_getlist(config_get_if(), &GNBE1ParamList, GNBE1Params, sizeofArray(GNBE1Params), aprefix);
      paramdef_t *e1Parms = GNBE1ParamList.paramarray[0];
      strcpy(e1ap_nc->CUCP_e1_ip_address.ipv4_address, *(e1Parms[GNB_CONFIG_E1_IPV4_ADDRESS_CUCP].strptr));
      e1ap_nc->CUCP_e1_ip_address.ipv4 = 1;
      strcpy(e1ap_nc->CUUP_e1_ip_address.ipv4_address, *(e1Parms[GNB_CONFIG_E1_IPV4_ADDRESS_CUUP].strptr));
      e1ap_nc->CUUP_e1_ip_address.ipv4 = 1;
      if (*entity == CPtype) {
        // CP needs gNB_ID (although not used, but other parts check it as
        // well), and gNB-CU-UP ID should NOT be present as it comes through E1!
        AssertFatal(!config_isparamset(gnbParms, GNB_GNB_CU_UP_ID_IDX), "%s must not be defined in configuration file\n", GNB_CONFIG_STRING_GNB_CU_UP_ID);
      } else { // UPtype
        AssertFatal(config_isparamset(gnbParms, GNB_GNB_CU_UP_ID_IDX), "%s is not be defined in configuration file\n", GNB_CONFIG_STRING_GNB_CU_UP_ID);
        e1Setup->gNB_cu_up_id = *gnbParms[GNB_GNB_CU_UP_ID_IDX].u64ptr;
      }
    } else {
      // integrated CU-CP/UP. We don't care about the gNB-CU-UP ID that much,
      // but if it's there, check it's the same as gNB ID
      uint64_t *gnb_cu_up_id = gnbParms[GNB_GNB_CU_UP_ID_IDX].u64ptr;
      AssertFatal(!config_isparamset(gnbParms, GNB_GNB_CU_UP_ID_IDX) || *gnb_cu_up_id == gnb_id,
                  "%s is different of %s: they need to match or remove %s from config\n",
                  GNB_CONFIG_STRING_GNB_CU_UP_ID,
                  GNB_CONFIG_STRING_GNB_ID,
                  GNB_CONFIG_STRING_GNB_CU_UP_ID);
      e1Setup->gNB_cu_up_id = gnb_id;
    }
  }
  return msgConfig;
}
