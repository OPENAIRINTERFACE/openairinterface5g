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
/*****************************************************************************

Source      emm_cn.c

Version     0.1

Date        2013/12/05

Product     NAS stack

Subsystem   EPS Core Network

Author      Sebastien Roux, Lionel GAUTHIER

Description

*****************************************************************************/

#include <string.h>

#if defined(NAS_BUILT_IN_EPC)

#include "nas_log.h"
#include "commonDef.h"

#include "emm_cn.h"
#include "emm_sap.h"
#include "emm_proc.h"
#include "emm_cause.h"

#include "esm_send.h"  // LG
#include "esm_proc.h"  // LG
#include "esm_cause.h" // LG
#include "assertions.h"// LG
#include "emmData.h"   // LG
#include "esm_sap.h"   // LG
#include "EmmCommon.h"   // LG
extern int emm_cn_wrapper_attach_accept(emm_data_context_t *emm_ctx, void *data);

/*
 * Internal data used for attach procedure
 */
typedef struct {
  unsigned int ueid;          /* UE identifier        */
#define ATTACH_COUNTER_MAX  5
  unsigned int retransmission_count;  /* Retransmission counter   */
  OctetString esm_msg;        /* ESM message to be sent within
                     * the Attach Accept message    */
} attach_data_t;

/*
 * String representation of EMMCN-SAP primitives
 */
static const char *_emm_cn_primitive_str[] = {
  "EMM_CN_AUTHENTICATION_PARAM_RES",
  "EMM_CN_AUTHENTICATION_PARAM_FAIL",
  "EMM_CN_DEREGISTER_UE",
  "EMM_CN_PDN_CONNECTIVITY_RES",
  "EMM_CN_PDN_CONNECTIVITY_FAIL",
};

#define EMM_CN_SAP_BUFFER_SIZE 4096
static char _emm_cn_sap_buffer[EMM_CN_SAP_BUFFER_SIZE];

static int _emm_cn_authentication_res(const emm_cn_auth_res_t *msg)
{
  emm_data_context_t *emm_ctx = NULL;
  int rc = RETURNerror;
  OctetString loc_rand;
  OctetString autn;

  /* We received security vector from HSS. Try to setup security with UE */

  LOG_FUNC_IN;

  emm_ctx = emm_data_context_get(&_emm_data, msg->ue_id);

  if (emm_ctx == NULL) {
    LOG_TRACE(ERROR, "EMM-PROC  - "
              "Failed to find UE associated to id "NAS_UE_ID_FMT"...", msg->ue_id);
    LOG_FUNC_RETURN (rc);
  }

  /* Copy provided vector to user context */
  memcpy(emm_ctx->vector.kasme, msg->vector.kasme,     AUTH_KASME_SIZE);
  memcpy(emm_ctx->vector.autn,  msg->vector.autn,      AUTH_AUTN_SIZE);
  memcpy(emm_ctx->vector.rand,  msg->vector.rand,      AUTH_RAND_SIZE);
  memcpy(emm_ctx->vector.xres,  msg->vector.xres.data, msg->vector.xres.size);

  LOG_TRACE(INFO, "EMM-PROC  - Received RAND ..: "RAND_FORMAT"\n",
            RAND_DISPLAY(msg->vector.rand));
  LOG_TRACE(INFO, "EMM-PROC  - Received AUTN ..: "AUTN_FORMAT"\n",
            AUTN_DISPLAY(msg->vector.autn));
  LOG_TRACE(INFO, "EMM-PROC  - Received KASME .: "KASME_FORMAT" "KASME_FORMAT"\n",
            KASME_DISPLAY_1(msg->vector.kasme),
            KASME_DISPLAY_2(msg->vector.kasme));

  loc_rand.value  = emm_ctx->vector.rand;
  loc_rand.length = AUTH_RAND_SIZE;

  autn.value  = emm_ctx->vector.autn;
  autn.length = AUTH_AUTN_SIZE;

  emm_ctx->vector.xres_size = msg->vector.xres.size;

  /* 3GPP TS 24.401, Figure 5.3.2.1-1, point 5a
   * No EMM context exists for the UE in the network; authentication
   * and NAS security setup to activate integrity protection and NAS
   * ciphering are mandatory.
   */
  rc = emm_proc_authentication(emm_ctx,
                               emm_ctx->ueid,
                               0, // TODO: eksi != 0
                               &loc_rand,
                               &autn,
                               emm_attach_security,
                               NULL,
                               NULL);

  if (rc != RETURNok) {
    /* Failed to initiate the authentication procedure */
    LOG_TRACE(WARNING, "EMM-PROC  - "
              "Failed to initiate authentication procedure");
    emm_ctx->emm_cause = EMM_CAUSE_ILLEGAL_UE;
  }

  LOG_FUNC_RETURN (rc);
}

static int _emm_cn_authentication_fail(const emm_cn_auth_fail_t *msg)
{
  int rc = RETURNerror;

  LOG_FUNC_IN;

  rc = emm_proc_attach_reject(msg->ue_id, msg->cause);

  LOG_FUNC_RETURN (rc);
}

static int _emm_cn_deregister_ue(const uint32_t ue_id)
{
  int rc = RETURNok;

  LOG_FUNC_IN;
  LOG_TRACE(WARNING, "EMM-PROC  - "
            "TODO deregister UE "NAS_UE_ID_FMT", following procedure is a test", ue_id);
  emm_proc_detach_request(ue_id, EMM_DETACH_TYPE_EPS /* ??? emm_proc_detach_type_t*/,
                              1 /*switch_off*/, 0 /*native_ksi*/, 0 /*ksi*/,
                              NULL /*guti*/, NULL /*imsi*/, NULL /*imei*/);
  LOG_FUNC_RETURN (rc);
}

static int _emm_cn_pdn_connectivity_res(const emm_cn_pdn_res_t *msg_pP)
{
  int                          rc                    = RETURNok;
  struct emm_data_context_s   *emm_ctx_p             = NULL;
  esm_proc_pdn_type_t          esm_pdn_type          = ESM_PDN_TYPE_IPV4;
  esm_proc_data_t             *esm_proc_data_p       = NULL;
  ESM_msg                      esm_msg;
  EpsQualityOfService          qos;
  ProtocolConfigurationOptions pco;
  unsigned int                 pco_in_index          = 0;
  signed int                   length_in_pco         = 0;
  uint16_t                     pi_or_ci              = 0; // protocol identifier or container identifier;
  uint8_t                      length_pi_or_ci       = 0;

  OctetString                  rsp                   = { 0, NULL};
  int                          is_standalone         = 0; // warning hardcoded
  int                          triggered_by_ue       = 1; // warning hardcoded
  attach_data_t               *data_p                = NULL;
  int                          esm_cause             = ESM_CAUSE_SUCCESS;
  int                          pid                   = 0;
  unsigned int                 new_ebi               = 0;


  LOG_FUNC_IN;
  emm_ctx_p = emm_data_context_get(&_emm_data, msg_pP->ue_id);

  if (emm_ctx_p == NULL) {
    LOG_TRACE(ERROR, "EMMCN-SAP  - "
              "Failed to find UE associated to id "NAS_UE_ID_FMT"...", msg_pP->ue_id);
    LOG_FUNC_RETURN (rc);
  }

  memset(&esm_msg, 0 , sizeof(ESM_msg));

  switch (msg_pP->pdn_type) {
  case IPv4:
    LOG_TRACE(INFO, "EMM  -  esm_pdn_type = ESM_PDN_TYPE_IPV4");
    esm_pdn_type = ESM_PDN_TYPE_IPV4;
    break;

  case IPv6:
    LOG_TRACE(INFO, "EMM  -  esm_pdn_type = ESM_PDN_TYPE_IPV6");
    esm_pdn_type = ESM_PDN_TYPE_IPV6;
    break;

  case IPv4_AND_v6:
    LOG_TRACE(INFO, "EMM  -  esm_pdn_type = ESM_PDN_TYPE_IPV4V6");
    esm_pdn_type = ESM_PDN_TYPE_IPV4V6;
    break;

  default:
    LOG_TRACE(INFO, "EMM  -  esm_pdn_type = ESM_PDN_TYPE_IPV4 (forced to default)");
    esm_pdn_type = ESM_PDN_TYPE_IPV4;
  }

  LOG_TRACE(INFO, "EMM  -  qci       = %u ", msg_pP->qci);
  LOG_TRACE(INFO, "EMM  -  qos.qci   = %u ", msg_pP->qos.qci);
  LOG_TRACE(INFO, "EMM  -  qos.mbrUL = %u ", msg_pP->qos.mbrUL);
  LOG_TRACE(INFO, "EMM  -  qos.mbrDL = %u ", msg_pP->qos.mbrDL);
  LOG_TRACE(INFO, "EMM  -  qos.gbrUL = %u ", msg_pP->qos.gbrUL);
  LOG_TRACE(INFO, "EMM  -  qos.gbrDL = %u ", msg_pP->qos.gbrDL);
  qos.bitRatesPresent           = 0;
  qos.bitRatesExtPresent        = 0;
#warning "Some work to do here about qos"
  qos.qci                       = msg_pP->qci;
  qos.bitRates.maxBitRateForUL  = 0;//msg_pP->qos.mbrUL;
  qos.bitRates.maxBitRateForDL  = 0;//msg_pP->qos.mbrDL;
  qos.bitRates.guarBitRateForUL = 0;//msg_pP->qos.gbrUL;
  qos.bitRates.guarBitRateForDL = 0;//msg_pP->qos.gbrDL;

  qos.bitRatesExt.maxBitRateForUL  = 0;
  qos.bitRatesExt.maxBitRateForDL  = 0;
  qos.bitRatesExt.guarBitRateForUL = 0;
  qos.bitRatesExt.guarBitRateForDL = 0;

  //--------------------------------------------------------------------------
  // PCO processing
  //--------------------------------------------------------------------------
  memset(&pco, 0,  sizeof(ProtocolConfigurationOptions));
  length_in_pco = msg_pP->pco.byte[1];
  if ((length_in_pco+1+1) != msg_pP->pco.length) {
	  LOG_TRACE(WARNING, "PCO: mismatch in lengths length_pco+1+1 %u != msg_pP->pco.length %u\n",
  		  length_in_pco+1+1, msg_pP->pco.length);
  }
  pco.configurationprotol = msg_pP->pco.byte[2] & 0x07;

  for (int i = 0; i < msg_pP->pco.length; i++) {
	  LOG_TRACE(WARNING, "EMMCN_PDN_CONNECTIVITY_RES.pco.byte[%u] = 0x%x", i, msg_pP->pco.byte[i]);
  }


  if ((length_in_pco > 0) && (msg_pP->pco.byte[2] & 0x80)) {
	pco_in_index = PCO_MIN_LENGTH;
    while (length_in_pco >= 3) {
      pi_or_ci = (((uint16_t)msg_pP->pco.byte[pco_in_index]) << 8) | (uint16_t)msg_pP->pco.byte[pco_in_index+1];
      pco_in_index += 2;
      length_pi_or_ci = msg_pP->pco.byte[pco_in_index++];
      pco.protocolid[pco.num_protocol_id_or_container_id]                = pi_or_ci;
      pco.lengthofprotocolid[pco.num_protocol_id_or_container_id]        = length_pi_or_ci;
      pco.protocolidcontents[pco.num_protocol_id_or_container_id].value  = malloc(length_pi_or_ci);
      pco.protocolidcontents[pco.num_protocol_id_or_container_id].length = length_pi_or_ci;
      memcpy(pco.protocolidcontents[pco.num_protocol_id_or_container_id].value,
    		&msg_pP->pco.byte[pco_in_index],
    		 length_pi_or_ci);

      LOG_TRACE(WARNING, "PCO: Found pi_or_ci 0x%x length %u content %s\n",
    		  pi_or_ci, length_pi_or_ci, dump_octet_string(&pco.protocolidcontents[pco.num_protocol_id_or_container_id]));
      pco.num_protocol_id_or_container_id++;
      pco_in_index += length_pi_or_ci;

      length_in_pco = length_in_pco - (length_pi_or_ci + 2 + 1);
    } // while (length_in_pco >= 3) {
  }  // if ((length_in_pco > 0) && (msg_pP->pco.byte[2] & 0x80)) {
  /*************************************************************************/
  /* CODE THAT WAS IN esm_recv.c/esm_recv_pdn_connectivity_request()       */
  /*************************************************************************/
  /* Execute the PDN connectivity procedure requested by the UE */
  pid = esm_proc_pdn_connectivity_request(
          emm_ctx_p,
          msg_pP->pti,
          msg_pP->request_type,
          &msg_pP->apn,
          esm_pdn_type,
          &msg_pP->pdn_addr,
          NULL,
          &esm_cause);
  LOG_TRACE(INFO, "EMM  -  APN = %s ", (char *)(msg_pP->apn.value));

  if (pid != RETURNerror) {
    /* Create local default EPS bearer context */
    rc = esm_proc_default_eps_bearer_context(
           emm_ctx_p,
           pid,
           &new_ebi,
           &msg_pP->qos,
           &esm_cause);

    if (rc != RETURNerror) {
      esm_cause = ESM_CAUSE_SUCCESS;
    }
  } else {
    LOG_FUNC_RETURN (rc);
  }

  /**************************************************************************/
  /* END OF CODE THAT WAS IN esm_recv.c/esm_recv_pdn_connectivity_request() */
  /**************************************************************************/
  LOG_TRACE(INFO, "EMM  -  APN = %s ", (char *)(msg_pP->apn.value));


  /*************************************************************************/
  /* CODE THAT WAS IN esm_sap.c/_esm_sap_recv()                            */
  /*************************************************************************/
  /* Return default EPS bearer context request message */
  rc = esm_send_activate_default_eps_bearer_context_request(
         msg_pP->pti,
         new_ebi, //msg_pP->ebi,
         &esm_msg.activate_default_eps_bearer_context_request,
         &msg_pP->apn,
         &pco,
         esm_pdn_type,
         &msg_pP->pdn_addr,
         &qos,
         ESM_CAUSE_SUCCESS);


  if  (rc != RETURNerror)  {
    /* Encode the returned ESM response message */
    int size = esm_msg_encode(&esm_msg, (uint8_t *)_emm_cn_sap_buffer,
                              EMM_CN_SAP_BUFFER_SIZE);
    LOG_TRACE(INFO, "ESM encoded MSG size %d ", size);

    if (size > 0) {
      rsp.length = size;
      rsp.value  = (uint8_t *)(_emm_cn_sap_buffer);
    }

    /* Complete the relevant ESM procedure */
    rc = esm_proc_default_eps_bearer_context_request(
           is_standalone,
           emm_ctx_p,
           new_ebi, //0, //ESM_EBI_UNASSIGNED, //msg->ebi,
           &rsp,
           triggered_by_ue);

    if (rc != RETURNok) {
      /* Return indication that ESM procedure failed */
      LOG_FUNC_RETURN (rc);
    }
  } else {
    LOG_TRACE(INFO, "ESM send activate_default_eps_bearer_context_request failed");
  }

  /*************************************************************************/
  /* END OF CODE THAT WAS IN esm_sap.c/_esm_sap_recv()                     */
  /*************************************************************************/

  LOG_TRACE(INFO, "EMM  -  APN = %s ", (char *)(msg_pP->apn.value));
  data_p = (attach_data_t *)emm_proc_common_get_args(msg_pP->ue_id);

  /* Setup the ESM message container */
  data_p->esm_msg.value = (uint8_t *)malloc(rsp.length);

  if (data_p->esm_msg.value) {
    data_p->esm_msg.length = rsp.length;
    LOG_TRACE(INFO, "EMM  - copy ESM MSG %d bytes", data_p->esm_msg.length);
    memcpy(data_p->esm_msg.value,
           rsp.value,
           rsp.length);
  } else {
    data_p->esm_msg.length = 0;
  }

  /* Send attach accept message to the UE */
  rc = emm_cn_wrapper_attach_accept(emm_ctx_p, data_p);

  if (rc != RETURNerror) {
    if (emm_ctx_p->guti_is_new && emm_ctx_p->old_guti) {
      /* Implicit GUTI reallocation;
       * Notify EMM that common procedure has been initiated
       */
      emm_sap_t emm_sap;
      emm_sap.primitive = EMMREG_COMMON_PROC_REQ;
      emm_sap.u.emm_reg.ueid = msg_pP->ue_id;
      rc = emm_sap_send(&emm_sap);
    }
  }

  LOG_TRACE(INFO, "EMM  -  APN = %s ", (char *)(msg_pP->apn.value));
  LOG_FUNC_RETURN (rc);
}

static int _emm_cn_pdn_connectivity_fail(const emm_cn_pdn_fail_t *msg)
{
  int rc = RETURNok;

  LOG_FUNC_IN;

  LOG_FUNC_RETURN (rc);
}

int emm_cn_send(const emm_cn_t *msg)
{
  int rc = RETURNerror;
  emm_cn_primitive_t primitive = msg->primitive;

  LOG_FUNC_IN;

  LOG_TRACE(INFO, "EMMCN-SAP - Received primitive %s (%d)",
            _emm_cn_primitive_str[primitive - _EMMCN_START - 1], primitive);

  switch (primitive) {
  case _EMMCN_AUTHENTICATION_PARAM_RES:
    rc = _emm_cn_authentication_res(msg->u.auth_res);
    break;

  case _EMMCN_AUTHENTICATION_PARAM_FAIL:
    rc = _emm_cn_authentication_fail(msg->u.auth_fail);
    break;

  case EMMCN_DEREGISTER_UE:
    rc = _emm_cn_deregister_ue(msg->u.deregister.UEid);
    break;

  case EMMCN_PDN_CONNECTIVITY_RES:
    rc = _emm_cn_pdn_connectivity_res(msg->u.emm_cn_pdn_res);
    break;

  case EMMCN_PDN_CONNECTIVITY_FAIL:
    rc = _emm_cn_pdn_connectivity_fail(msg->u.emm_cn_pdn_fail);
    break;

  default:
    /* Other primitives are forwarded to the Access Stratum */
    rc = RETURNerror;
    break;
  }

  if (rc != RETURNok) {
    LOG_TRACE(ERROR, "EMMCN-SAP - Failed to process primitive %s (%d)",
              _emm_cn_primitive_str[primitive - _EMMCN_START - 1],
              primitive);
  }

  LOG_FUNC_RETURN (rc);
}
#endif
