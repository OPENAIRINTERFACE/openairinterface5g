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
Source      PdnConnectivity.c

Version     0.1

Date        2013/01/02

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines the PDN connectivity ESM procedure executed by the
        Non-Access Stratum.

        The PDN connectivity procedure is used by the UE to request
        the setup of a default EPS bearer to a PDN.

        The procedure is used either to establish the 1st default
        bearer by including the PDN CONNECTIVITY REQUEST message
        into the initial attach message, or to establish subsequent
        default bearers to additional PDNs in order to allow the UE
        simultaneous access to multiple PDNs by sending the message
        stand-alone.

*****************************************************************************/

#include <stdlib.h> // malloc, free
#include <string.h> // memset, memcpy, memcmp
#include <ctype.h>  // isprint

#include "esm_proc.h"
#include "commonDef.h"
#include "nas_log.h"

#include "esmData.h"
#include "esm_cause.h"
#include "esm_pt.h"

# include "mme_api.h"

#include "emm_sap.h"

#if defined(ENABLE_ITTI)
# include "assertions.h"
#endif

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/



/*
 * --------------------------------------------------------------------------
 *  Internal data handled by the PDN connectivity procedure in the MME
 * --------------------------------------------------------------------------
 */
/*
 * PDN connection handlers
 */
static int _pdn_connectivity_create(emm_data_context_t *ctx,
                                    int pti,
                                    const OctetString *apn,
                                    esm_proc_pdn_type_t pdn_type,
                                    const OctetString *pdn_addr,
                                    int is_emergency);
int _pdn_connectivity_delete(emm_data_context_t *ctx, int pid);


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/


/*
 * --------------------------------------------------------------------------
 *      PDN connectivity procedure executed by the MME
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_pdn_connectivity_request()                       **
 **                                                                        **
 ** Description: Performs PDN connectivity procedure requested by the UE.  **
 **                                                                        **
 **              3GPP TS 24.301, section 6.5.1.3                           **
 **      Upon receipt of the PDN CONNECTIVITY REQUEST message, the **
 **      MME checks if connectivity with the requested PDN can be  **
 **      established. If no requested  APN  is provided  the  MME  **
 **      shall use the default APN as the  requested  APN if the   **
 **      request type is different from "emergency", or the APN    **
 **      configured for emergency bearer services if the request   **
 **      type is "emergency".                                      **
 **      If connectivity with the requested PDN is accepted by the **
 **      network, the MME shall initiate the default EPS bearer    **
 **      context activation procedure.                             **
 **                                                                        **
 ** Inputs:  ueid:      UE local identifier                        **
 **      pti:       Identifies the PDN connectivity procedure  **
 **             requested by the UE                        **
 **      request_type:  Type of the PDN request                    **
 **      pdn_type:  PDN type value (IPv4, IPv6, IPv4v6)        **
 **      apn:       Requested Access Point Name                **
 **      Others:    _esm_data                                  **
 **                                                                        **
 ** Outputs:     apn:       Default Access Point Name                  **
 **      pdn_addr:  Assigned IPv4 address and/or IPv6 suffix   **
 **      esm_qos:   EPS bearer level QoS parameters            **
 **      esm_cause: Cause code returned upon ESM procedure     **
 **             failure                                    **
 **      Return:    The identifier of the PDN connection if    **
 **             successfully created;                      **
 **             RETURNerror otherwise.                     **
 **      Others:    _esm_data                                  **
 **                                                                        **
 ***************************************************************************/
int esm_proc_pdn_connectivity_request(emm_data_context_t *ctx, int pti,
                                      esm_proc_pdn_request_t request_type,
                                      OctetString *apn,
                                      esm_proc_pdn_type_t pdn_type,
                                      OctetString *pdn_addr,
                                      esm_proc_qos_t *esm_qos,
                                      int *esm_cause)
{
  int rc = RETURNerror;
  int pid = RETURNerror;

  LOG_FUNC_IN;

  LOG_TRACE(INFO, "ESM-PROC  - PDN connectivity requested by the UE "
            "(ueid="NAS_UE_ID_FMT", pti=%d) PDN type = %s, APN = %s pdn addr = %s", ctx->ueid, pti,
            (pdn_type == ESM_PDN_TYPE_IPV4)? "IPv4" :
            (pdn_type == ESM_PDN_TYPE_IPV6)? "IPv6" : "IPv4v6",
            (apn) ? (char *)(apn->value) : "null",
            (pdn_addr) ? (char *)(pdn_addr->value) : "null");

#if !defined(NAS_BUILT_IN_EPC)

  /* UE identifier sanity check */
  if (ctx->ueid >= ESM_DATA_NB_UE_MAX) {
    LOG_TRACE(WARNING, "ESM-PROC  - Number of connected UEs exceeded");
    LOG_FUNC_RETURN (RETURNerror);
  }

#endif

  /*
   * Check network IP capabilities
   */
  *esm_cause = ESM_CAUSE_SUCCESS;
  LOG_TRACE(INFO, "ESM-PROC  - _esm_data.conf.features %08x", _esm_data.conf.features);
#warning "Uncomment code about _esm_data.conf.features & (MME_API_IPV4 | MME_API_IPV6) later"
#if defined(ORIGINAL_CODE)

  switch (_esm_data.conf.features & (MME_API_IPV4 | MME_API_IPV6)) {
  case (MME_API_IPV4 | MME_API_IPV6):

    /* The network supports both IPv4 and IPv6 connection */
    if ( (pdn_type == ESM_PDN_TYPE_IPV4V6) &&
         (_esm_data.conf.features & MME_API_SINGLE_ADDR_BEARERS) ) {
      /* The network supports single IP version bearers only */
      *esm_cause = ESM_CAUSE_SINGLE_ADDRESS_BEARERS_ONLY_ALLOWED;
    }

    rc = RETURNok;
    break;

  case MME_API_IPV6:
    /* The network supports connection to IPv6 only */
    *esm_cause = ESM_CAUSE_PDN_TYPE_IPV6_ONLY_ALLOWED;

    if (pdn_type != ESM_PDN_TYPE_IPV4) {
      rc = RETURNok;
    }

    break;

  case MME_API_IPV4:
    /* The network supports connection to IPv4 only */
    *esm_cause = ESM_CAUSE_PDN_TYPE_IPV4_ONLY_ALLOWED;

    if (pdn_type != ESM_PDN_TYPE_IPV6) {
      rc = RETURNok;
    }

    break;

  default:
    LOG_TRACE(ERROR,
              "ESM-PROC  - _esm_data.conf.features incorrect value (no IPV4 or IPV6 ) %X",
              _esm_data.conf.features);
  }

#else
  rc = RETURNok;
#endif

  if (rc != RETURNerror) {
    int is_emergency = (request_type == ESM_PDN_REQUEST_EMERGENCY);
#if defined(ORIGINAL_CODE)
    mme_api_ip_version_t mme_pdn_index;
    mme_api_qos_t qos;

    switch (pdn_type) {
    case ESM_PDN_TYPE_IPV4:
      mme_pdn_index = MME_API_IPV4_ADDR;
      break;

    case ESM_PDN_TYPE_IPV6:
      mme_pdn_index = MME_API_IPV6_ADDR;
      break;

    case ESM_PDN_TYPE_IPV4V6:
    default:
      mme_pdn_index = MME_API_IPV4V6_ADDR;
      break;
    }

    /* Check if connectivity with the requested PDN can be established */
    rc = mme_api_subscribe(apn, mme_pdn_index, pdn_addr, is_emergency, &qos);

    if (rc != RETURNok) {
      LOG_TRACE(WARNING, "ESM-PROC  - Connectivity to the requested PDN "
                "cannot be established");
      *esm_cause = ESM_CAUSE_REQUEST_REJECTED_UNSPECIFIED;
      LOG_FUNC_RETURN (RETURNerror);
    }

#endif

    /* Create new PDN connection */
    pid = _pdn_connectivity_create(ctx, pti, apn, pdn_type,
                                   pdn_addr, is_emergency);
#if defined(ORIGINAL_CODE)

    /* Setup ESM QoS parameters */
    if (esm_qos) {
      esm_qos->gbrUL = qos.gbr[MME_API_UPLINK];
      esm_qos->gbrDL = qos.gbr[MME_API_DOWNLINK];
      esm_qos->mbrUL = qos.mbr[MME_API_UPLINK];
      esm_qos->mbrDL = qos.mbr[MME_API_DOWNLINK];
      esm_qos->qci   = qos.qci;
    }

#endif

    if (pid < 0) {
      LOG_TRACE(WARNING, "ESM-PROC  - Failed to create PDN connection");
      *esm_cause = ESM_CAUSE_INSUFFICIENT_RESOURCES;
      LOG_FUNC_RETURN(RETURNerror);
    }
  }

  LOG_FUNC_RETURN(pid);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_pdn_connectivity_reject()                        **
 **                                                                        **
 ** Description: Performs PDN connectivity procedure not accepted by the   **
 **      network.                                                  **
 **                                                                        **
 **              3GPP TS 24.301, section 6.5.1.4                           **
 **      If connectivity with the requested PDN cannot be accepted **
 **      by the network, the MME shall send a PDN CONNECTIVITY RE- **
 **      JECT message to the UE.                                   **
 **                                                                        **
 ** Inputs:  is_standalone: Indicates whether the PDN connectivity     **
 **             procedure was initiated as part of the at- **
 **             tach procedure                             **
 **      ueid:      UE lower layer identifier                  **
 **      ebi:       Not used                                   **
 **      msg:       Encoded PDN connectivity reject message to **
 **             be sent                                    **
 **      ue_triggered:  Not used                                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_pdn_connectivity_reject(int is_standalone, emm_data_context_t *ctx,
                                     int ebi, OctetString *msg, int ue_triggered)
{
  LOG_FUNC_IN;

  int rc = RETURNerror;

  LOG_TRACE(WARNING, "ESM-PROC  - PDN connectivity not accepted by the "
            "network (ueid="NAS_UE_ID_FMT")", ctx->ueid);

  if (is_standalone) {
    emm_sap_t emm_sap;
    /*
     * Notity EMM that ESM PDU has to be forwarded to lower layers
     */
    emm_sap.primitive = EMMESM_UNITDATA_REQ;
    emm_sap.u.emm_esm.ctx = ctx;
    emm_sap.u.emm_esm.u.data.msg.length = msg->length;
    emm_sap.u.emm_esm.u.data.msg.value = msg->value;
    rc = emm_sap_send(&emm_sap);
  }

  /* If the PDN connectivity procedure initiated as part of the initial
   * attach procedure has failed, an error is returned to notify EMM that
   * the ESM sublayer did not accept UE requested PDN connectivity */
  LOG_FUNC_RETURN (rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:        esm_proc_pdn_connectivity_failure()                       **
 **                                                                        **
 ** Description: Performs PDN connectivity procedure upon receiving noti-  **
 **              fication from the EPS Mobility Management sublayer that   **
 **              EMM procedure that initiated PDN connectivity activation  **
 **              locally failed.                                           **
 **                                                                        **
 **              The MME releases the PDN connection entry allocated when  **
 **              the PDN connectivity procedure was requested by the UE.   **
 **                                                                        **
 **         Inputs:  ueid:      UE local identifier                        **
 **                  pid:       Identifier of the PDN connection to be     **
 **                             released                                   **
 **                  Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **                  Return:    RETURNok, RETURNerror                      **
 **                  Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_pdn_connectivity_failure(emm_data_context_t *ctx, int pid)
{
  int pti;

  LOG_FUNC_IN;

  LOG_TRACE(WARNING, "ESM-PROC  - PDN connectivity failure (ueid="NAS_UE_ID_FMT", pid=%d)",
            ctx->ueid, pid);

  /* Delete the PDN connection entry */
  pti = _pdn_connectivity_delete(ctx, pid);

  if (pti != ESM_PT_UNASSIGNED) {
    LOG_FUNC_RETURN (RETURNok);
  }

  LOG_FUNC_RETURN (RETURNerror);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/


/*
 *---------------------------------------------------------------------------
 *              PDN connection handlers
 *---------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:        _pdn_connectivity_create()                                **
 **                                                                        **
 ** Description: Creates a new PDN connection entry for the specified UE   **
 **                                                                        **
 ** Inputs:          ueid:      UE local identifier                        **
 **                  ctx:       UE context                                 **
 **                  pti:       Procedure transaction identity             **
 **                  apn:       Access Point Name of the PDN connection    **
 **                  pdn_type:  PDN type (IPv4, IPv6, IPv4v6)              **
 **                  pdn_addr:  Network allocated PDN IPv4 or IPv6 address **
 **              is_emergency:  TRUE if the PDN connection has to be esta- **
 **                             blished for emergency bearer services      **
 **                  Others:    _esm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **                  Return:    The identifier of the PDN connection if    **
 **                             successfully created; -1 otherwise.        **
 **                  Others:    _esm_data                                  **
 **                                                                        **
 ***************************************************************************/
static int _pdn_connectivity_create(emm_data_context_t *ctx,
                                    int pti,
                                    const OctetString *apn,
                                    esm_proc_pdn_type_t pdn_type,
                                    const OctetString *pdn_addr,
                                    int is_emergency)
{
  int pid = ESM_DATA_PDN_MAX;

  LOG_TRACE(INFO, "ESM-PROC  - Create new PDN connection "
            "(pti=%d) APN = %s, IP address = %s (ueid="NAS_UE_ID_FMT")", pti, apn->value,
            (pdn_type == ESM_PDN_TYPE_IPV4)? esm_data_get_ipv4_addr(pdn_addr) :
            (pdn_type == ESM_PDN_TYPE_IPV6)? esm_data_get_ipv6_addr(pdn_addr) :
            esm_data_get_ipv4v6_addr(pdn_addr), ctx->ueid);

  /* Search for an available PDN connection entry */
  for (pid = 0; pid < ESM_DATA_PDN_MAX; pid++) {
    if (ctx->esm_data_ctx.pdn[pid].data != NULL) {
      continue;
    }

    break;
  }

  if (pid < ESM_DATA_PDN_MAX) {
    /* Create new PDN connection */
    esm_pdn_t *pdn = (esm_pdn_t *)malloc(sizeof(esm_pdn_t));

    if (pdn != NULL) {
      memset(pdn, 0, sizeof(esm_pdn_t));
      /* Increment the number of PDN connections */
      ctx->esm_data_ctx.n_pdns += 1;
      /* Set the PDN connection identifier */
      ctx->esm_data_ctx.pdn[pid].pid = pid;
      /* Reset the PDN connection active indicator */
      ctx->esm_data_ctx.pdn[pid].is_active = FALSE;
      /* Setup the PDN connection data */
      ctx->esm_data_ctx.pdn[pid].data = pdn;

      /* Set the procedure transaction identity */
      pdn->pti = pti;
      /* Set the emergency bearer services indicator */
      pdn->is_emergency = is_emergency;

      /* Setup the Access Point Name */
      if ( apn && (apn->length > 0) ) {
        pdn->apn.value = (uint8_t *)malloc(apn->length + 1);

        if (pdn->apn.value) {
          pdn->apn.length = apn->length;
          memcpy(pdn->apn.value, apn->value, apn->length);
          pdn->apn.value[pdn->apn.length] = '\0';
        }
      }

      /* Setup the IP address allocated by the network */
      if ( pdn_addr && (pdn_addr->length > 0) ) {
        int length =
          ((pdn_addr->length < ESM_DATA_IP_ADDRESS_SIZE) ?
           pdn_addr->length : ESM_DATA_IP_ADDRESS_SIZE);
        memcpy(pdn->ip_addr, pdn_addr->value, length);
        pdn->type = pdn_type;
      }

      /* Return the identifier of the new PDN connection */
      return (ctx->esm_data_ctx.pdn[pid].pid);
    }

    LOG_TRACE(WARNING, "ESM-PROC  - Failed to create new PDN connection "
              "(pid=%d)", pid);
  }

  return (-1);
}

/****************************************************************************
 **                                                                        **
 ** Name:        _pdn_connectivity_delete()                                **
 **                                                                        **
 ** Description: Deletes PDN connection to the specified UE associated to  **
 **              PDN connection entry with given identifier                **
 **                                                                        **
 ** Inputs:          ueid:      UE local identifier                        **
 **                  pid:       Identifier of the PDN connection to be     **
 **                             released                                   **
 **                  Others:    _esm_data                                  **
 **                                                                        **
 ** Outputs:     None                                                      **
 **                  Return:    The identity of the procedure transaction  **
 **                             assigned to the PDN connection when suc-   **
 **                             cessfully released;                        **
 **                             UNASSIGNED value otherwise.                **
 **                  Others:    _esm_data                                  **
 **                                                                        **
 ***************************************************************************/
int _pdn_connectivity_delete(emm_data_context_t *ctx, int pid)
{
  int pti = ESM_PT_UNASSIGNED;

  if (ctx == NULL) {
    return pti;
  }

  if (pid < ESM_DATA_PDN_MAX) {
    if (pid != ctx->esm_data_ctx.pdn[pid].pid) {
      LOG_TRACE(ERROR,
                "ESM-PROC  - PDN connection identifier is not valid");
    } else if (ctx->esm_data_ctx.pdn[pid].data == NULL) {
      LOG_TRACE(ERROR,
                "ESM-PROC  - PDN connection has not been allocated");
    } else if (ctx->esm_data_ctx.pdn[pid].is_active) {
      LOG_TRACE(ERROR, "ESM-PROC  - PDN connection is active");
    } else {
      /* Get the identity of the procedure transaction that created
       * the PDN connection */
      pti = ctx->esm_data_ctx.pdn[pid].data->pti;
    }
  }

  if (pti != ESM_PT_UNASSIGNED) {
    /* Decrement the number of PDN connections */
    ctx->esm_data_ctx.n_pdns -= 1;
    /* Set the PDN connection as available */
    ctx->esm_data_ctx.pdn[pid].pid = -1;

    /* Release allocated PDN connection data */
    if (ctx->esm_data_ctx.pdn[pid].data->apn.length > 0) {
      free(ctx->esm_data_ctx.pdn[pid].data->apn.value);
    }

    free(ctx->esm_data_ctx.pdn[pid].data);
    ctx->esm_data_ctx.pdn[pid].data = NULL;
    LOG_TRACE(WARNING, "ESM-PROC  - PDN connection %d released", pid);
  }

  /* Return the procedure transaction identity */
  return (pti);
}
