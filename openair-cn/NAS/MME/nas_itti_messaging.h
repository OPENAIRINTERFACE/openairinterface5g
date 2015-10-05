/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
#include <string.h>
#include <stdint.h>
#include <ctype.h>

#include "assertions.h"
#include "intertask_interface.h"
#include "esm_proc.h"
#include "msc.h"

#ifndef NAS_ITTI_MESSAGING_H_
#define NAS_ITTI_MESSAGING_H_

# if defined(NAS_BUILT_IN_EPC)
int nas_itti_plain_msg(
  const char *buffer,
  const nas_message_t *msg,
  const int lengthP,
  const int instance);

int nas_itti_protected_msg(
  const char *buffer,
  const nas_message_t *msg,
  const int lengthP,
  const int instance);
# endif

# if defined(NAS_BUILT_IN_EPC)
#include "conversions.h"

int nas_itti_dl_data_req(
  const uint32_t ue_idP,
  void *const    data_pP,
  const uint32_t lengthP);

static inline void nas_itti_pdn_connectivity_req(
  int                     ptiP,
  unsigned int            ueidP,
  const imsi_t           *const imsi_pP,
  esm_proc_data_t        *proc_data_pP,
  esm_proc_pdn_request_t  request_typeP)
{
  MessageDef *message_p = NULL;
  uint8_t     i;
  uint8_t     index;

  AssertFatal(imsi_pP       != NULL, "imsi_pP param is NULL");
  AssertFatal(proc_data_pP  != NULL, "proc_data_pP param is NULL");


  message_p = itti_alloc_new_message(TASK_NAS_MME, NAS_PDN_CONNECTIVITY_REQ);
  memset(&message_p->ittiMsg.nas_pdn_connectivity_req,
         0,
         sizeof(nas_pdn_connectivity_req_t));

  hexa_to_ascii((uint8_t *)imsi_pP->u.value,
                NAS_PDN_CONNECTIVITY_REQ(message_p).imsi,
                8);

  NAS_PDN_CONNECTIVITY_REQ(message_p).pti             = ptiP;
  NAS_PDN_CONNECTIVITY_REQ(message_p).ue_id           = ueidP;
  NAS_PDN_CONNECTIVITY_REQ(message_p).imsi[15]        = '\0';

  if (isdigit(NAS_PDN_CONNECTIVITY_REQ(message_p).imsi[14])) {
    NAS_PDN_CONNECTIVITY_REQ(message_p).imsi_length = 15;
  } else {
    NAS_PDN_CONNECTIVITY_REQ(message_p).imsi_length = 14;
    NAS_PDN_CONNECTIVITY_REQ(message_p).imsi[14] = '\0';
  }

  DUP_OCTET_STRING(proc_data_pP->apn,      NAS_PDN_CONNECTIVITY_REQ(message_p).apn);
  DUP_OCTET_STRING(proc_data_pP->pdn_addr, NAS_PDN_CONNECTIVITY_REQ(message_p).pdn_addr);

  switch (proc_data_pP->pdn_type) {
  case ESM_PDN_TYPE_IPV4:
    NAS_PDN_CONNECTIVITY_REQ(message_p).pdn_type = IPv4;
    break;

  case ESM_PDN_TYPE_IPV6:
    NAS_PDN_CONNECTIVITY_REQ(message_p).pdn_type = IPv6;
    break;

  case ESM_PDN_TYPE_IPV4V6:
    NAS_PDN_CONNECTIVITY_REQ(message_p).pdn_type = IPv4_AND_v6;
    break;

  default:
    NAS_PDN_CONNECTIVITY_REQ(message_p).pdn_type = IPv4;
    break;
  }

  // not efficient but be careful about "typedef network_qos_t esm_proc_qos_t;"
  NAS_PDN_CONNECTIVITY_REQ(message_p).qos.gbrUL = proc_data_pP->qos.gbrUL;
  NAS_PDN_CONNECTIVITY_REQ(message_p).qos.gbrDL = proc_data_pP->qos.gbrDL;
  NAS_PDN_CONNECTIVITY_REQ(message_p).qos.mbrUL = proc_data_pP->qos.mbrUL;
  NAS_PDN_CONNECTIVITY_REQ(message_p).qos.mbrDL = proc_data_pP->qos.mbrDL;
  NAS_PDN_CONNECTIVITY_REQ(message_p).qos.qci   = proc_data_pP->qos.qci;

  NAS_PDN_CONNECTIVITY_REQ(message_p).proc_data = proc_data_pP;

  NAS_PDN_CONNECTIVITY_REQ(message_p).request_type  = request_typeP;

  if (proc_data_pP->pco.num_protocol_id_or_container_id <= PROTOCOL_CONFIGURATION_OPTIONS_MAXIMUM_PROTOCOL_ID_OR_CONTAINER_ID) {
    NAS_PDN_CONNECTIVITY_REQ(message_p).pco.byte[0] = ACTIVATE_DEDICATED_EPS_BEARER_CONTEXT_ACCEPT_PROTOCOL_CONFIGURATION_OPTIONS_IEI;
    NAS_PDN_CONNECTIVITY_REQ(message_p).pco.byte[1] = 1 + 3 * proc_data_pP->pco.num_protocol_id_or_container_id;
    NAS_PDN_CONNECTIVITY_REQ(message_p).pco.byte[2] = 0x80; // do it fast
    i = 0;
    index = 3;
    while (( i < proc_data_pP->pco.num_protocol_id_or_container_id) &&
    		((index + proc_data_pP->pco.protocolidcontents[i].length) <= PROTOCOL_CONFIGURATION_OPTIONS_MAXIMUM_LENGTH)){
      NAS_PDN_CONNECTIVITY_REQ(message_p).pco.byte[1] += proc_data_pP->pco.lengthofprotocolid[i];
      NAS_PDN_CONNECTIVITY_REQ(message_p).pco.byte[index++] = (proc_data_pP->pco.protocolid[i] >> 8);
      NAS_PDN_CONNECTIVITY_REQ(message_p).pco.byte[index++] = (proc_data_pP->pco.protocolid[i] & 0x00FF);
      NAS_PDN_CONNECTIVITY_REQ(message_p).pco.byte[index++] = proc_data_pP->pco.lengthofprotocolid[i];
      if (proc_data_pP->pco.lengthofprotocolid[i] > 0) {
        memcpy( &NAS_PDN_CONNECTIVITY_REQ(message_p).pco.byte[index],
    		proc_data_pP->pco.protocolidcontents[i].value,
    		proc_data_pP->pco.lengthofprotocolid[i]);
        index += proc_data_pP->pco.lengthofprotocolid[i];
      }
      i++;
    }
    NAS_PDN_CONNECTIVITY_REQ(message_p).pco.length = index;
  }


  MSC_LOG_TX_MESSAGE(
  	  	MSC_NAS_MME,
  	  	MSC_MMEAPP_MME,
  	  	NULL,0,
  	  	"0 NAS_PDN_CONNECTIVITY_REQ ue id %06"PRIX32" IMSI %X",
  	  	ueidP, NAS_PDN_CONNECTIVITY_REQ(message_p).imsi);

  itti_send_msg_to_task(TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}



static inline void nas_itti_establish_cnf(
  const uint32_t         ue_idP,
  const nas_error_code_t error_codeP,
  void            *const data_pP,
  const uint32_t         lengthP,
  const uint16_t         selected_encryption_algorithmP,
  const uint16_t         selected_integrity_algorithmP)
{
  MessageDef *message_p = NULL;

  message_p = itti_alloc_new_message(TASK_NAS_MME, NAS_CONNECTION_ESTABLISHMENT_CNF);
  memset(&message_p->ittiMsg.nas_conn_est_cnf,
         0,
         sizeof(nas_conn_est_cnf_t));
  NAS_CONNECTION_ESTABLISHMENT_CNF(message_p).UEid            = ue_idP;
  NAS_CONNECTION_ESTABLISHMENT_CNF(message_p).errCode         = error_codeP;
  NAS_CONNECTION_ESTABLISHMENT_CNF(message_p).nasMsg.data     = data_pP;
  NAS_CONNECTION_ESTABLISHMENT_CNF(message_p).nasMsg.length   = lengthP;
  NAS_CONNECTION_ESTABLISHMENT_CNF(message_p).selected_encryption_algorithm   = selected_encryption_algorithmP;
  NAS_CONNECTION_ESTABLISHMENT_CNF(message_p).selected_integrity_algorithm    = selected_integrity_algorithmP;

  MSC_LOG_TX_MESSAGE(
  	  	MSC_NAS_MME,
  	  	MSC_MMEAPP_MME,
  	  	NULL,0,
  	  	"0 NAS_CONNECTION_ESTABLISHMENT_CNF ue id %06"PRIX32" len %u sea %x sia %x ",
  	  ue_idP, lengthP, selected_encryption_algorithmP, selected_integrity_algorithmP);

  itti_send_msg_to_task(TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}

static inline void nas_itti_auth_info_req(
  const uint32_t      ue_idP,
  const imsi_t *const imsi_pP,
  uint8_t             initial_reqP,
  const uint8_t      *auts_pP)
{
  MessageDef *message_p;

  message_p = itti_alloc_new_message(TASK_NAS_MME, NAS_AUTHENTICATION_PARAM_REQ);
  memset(&message_p->ittiMsg.nas_auth_param_req,
         0,
         sizeof(nas_auth_param_req_t));

  hexa_to_ascii((uint8_t *)imsi_pP->u.value,
                NAS_AUTHENTICATION_PARAM_REQ(message_p).imsi, 8);

  NAS_AUTHENTICATION_PARAM_REQ(message_p).imsi[15] = '\0';

  if (isdigit(NAS_AUTHENTICATION_PARAM_REQ(message_p).imsi[14])) {
    NAS_AUTHENTICATION_PARAM_REQ(message_p).imsi_length = 15;
  } else {
    NAS_AUTHENTICATION_PARAM_REQ(message_p).imsi_length = 14;
    NAS_AUTHENTICATION_PARAM_REQ(message_p).imsi[14] = '\0';
  }

  NAS_AUTHENTICATION_PARAM_REQ(message_p).initial_req = initial_reqP;
  NAS_AUTHENTICATION_PARAM_REQ(message_p).ue_id = ue_idP;

  /* Re-synchronisation */
  if (auts_pP != NULL) {
    NAS_AUTHENTICATION_PARAM_REQ(message_p).re_synchronization = 1;
    memcpy(NAS_AUTHENTICATION_PARAM_REQ(message_p).auts, auts_pP, AUTS_LENGTH);
  } else {
    NAS_AUTHENTICATION_PARAM_REQ(message_p).re_synchronization = 0;
    memset(NAS_AUTHENTICATION_PARAM_REQ(message_p).auts, 0, AUTS_LENGTH);
  }

  MSC_LOG_TX_MESSAGE(
  	  	MSC_NAS_MME,
  	  	MSC_MMEAPP_MME,
  	  	NULL,0,
  	  	"0 NAS_AUTHENTICATION_PARAM_REQ ue id %06"PRIX32" IMSI %s ",
  	  ue_idP, NAS_AUTHENTICATION_PARAM_REQ(message_p).imsi);

  itti_send_msg_to_task(TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}

static inline void nas_itti_establish_rej(
  const uint32_t      ue_idP,
  const imsi_t *const imsi_pP
  , uint8_t           initial_reqP)
{
  MessageDef *message_p;

  message_p = itti_alloc_new_message(TASK_NAS_MME, NAS_AUTHENTICATION_PARAM_REQ);
  memset(&message_p->ittiMsg.nas_auth_param_req,
         0,
         sizeof(nas_auth_param_req_t));

  hexa_to_ascii((uint8_t *)imsi_pP->u.value,
                NAS_AUTHENTICATION_PARAM_REQ(message_p).imsi, 8);

  NAS_AUTHENTICATION_PARAM_REQ(message_p).imsi[15] = '\0';

  if (isdigit(NAS_AUTHENTICATION_PARAM_REQ(message_p).imsi[14])) {
    NAS_AUTHENTICATION_PARAM_REQ(message_p).imsi_length = 15;
  } else {
    NAS_AUTHENTICATION_PARAM_REQ(message_p).imsi_length = 14;
    NAS_AUTHENTICATION_PARAM_REQ(message_p).imsi[14]    = '\0';
  }

  NAS_AUTHENTICATION_PARAM_REQ(message_p).initial_req = initial_reqP;
  NAS_AUTHENTICATION_PARAM_REQ(message_p).ue_id       = ue_idP;

  MSC_LOG_TX_MESSAGE(
  	  	MSC_NAS_MME,
  	  	MSC_MMEAPP_MME,
  	  	NULL,0,
  	  	"0 NAS_AUTHENTICATION_PARAM_REQ ue id %06"PRIX32" IMSI %s (establish reject)",
  	  ue_idP, NAS_AUTHENTICATION_PARAM_REQ(message_p).imsi);

  itti_send_msg_to_task(TASK_MME_APP, INSTANCE_DEFAULT, message_p);
}
# endif


#endif /* NAS_ITTI_MESSAGING_H_ */
