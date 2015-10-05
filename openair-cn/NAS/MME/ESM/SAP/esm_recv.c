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
Source      esm_recv.c

Version     0.1

Date        2013/02/06

Product     NAS stack

Subsystem   EPS Session Management

Author      Frederic Maurel

Description Defines functions executed at the ESM Service Access
        Point upon receiving EPS Session Management messages
        from the EPS Mobility Management sublayer.

*****************************************************************************/

#include "esm_recv.h"
#include "commonDef.h"
#include "nas_log.h"

#include "esm_pt.h"
#include "esm_ebr.h"
#include "esm_proc.h"

#include "esm_cause.h"
#include "nas_itti_messaging.h"


/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 * Functions executed by both the UE and the MME upon receiving ESM messages
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_status()                                         **
 **                                                                        **
 ** Description: Processes ESM status message                              **
 **                                                                        **
 ** Inputs:  ueid:      UE local identifier                        **
 **      pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      msg:       The received ESM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    ESM cause code whenever the processing of  **
 **             the ESM message fails                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/

int esm_recv_status(emm_data_context_t *ctx, int pti, int ebi,
                    const esm_status_msg *msg)
{
  int esm_cause;
  int rc;

  LOG_FUNC_IN;

  LOG_TRACE(INFO, "ESM-SAP   - Received ESM status message (pti=%d, ebi=%d)",
            pti, ebi);

  /*
   * Message processing
   */
  /* Get the ESM cause */
  esm_cause = msg->esmcause;

  /* Execute the ESM status procedure */
  rc = esm_proc_status_ind(ctx, pti, ebi, &esm_cause);

  if (rc != RETURNerror) {
    esm_cause = ESM_CAUSE_SUCCESS;
  }

  /* Return the ESM cause value */
  LOG_FUNC_RETURN (esm_cause);
}


/*
 * --------------------------------------------------------------------------
 * Functions executed by the MME upon receiving ESM message from the UE
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_pdn_connectivity_request()                       **
 **                                                                        **
 ** Description: Processes PDN connectivity request message                **
 **                                                                        **
 ** Inputs:  ueid:      UE local identifier                        **
 **      pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      msg:       The received ESM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     new_ebi:   New assigned EPS bearer identity           **
 **      data:      PDN connection and EPS bearer context data **
 **      Return:    ESM cause code whenever the processing of  **
 **             the ESM message fails                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_recv_pdn_connectivity_request(emm_data_context_t *ctx, int pti, int ebi,
                                      const pdn_connectivity_request_msg *msg,
                                      unsigned int *new_ebi, void *data)
{
  int esm_cause = ESM_CAUSE_SUCCESS;
  uint8_t i;

  LOG_FUNC_IN;

  LOG_TRACE(INFO, "ESM-SAP   - Received PDN Connectivity Request message "
            "(ueid=%u, pti=%d, ebi=%d)", ctx->ueid, pti, ebi);

  /*
   * Procedure transaction identity checking
   */
  if ( (pti == ESM_PT_UNASSIGNED) || esm_pt_is_reserved(pti) ) {
    /* 3GPP TS 24.301, section 7.3.1, case a
     * Reserved or unassigned PTI value
     */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid PTI value (pti=%d)", pti);
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_PTI_VALUE);
  }
  /*
   * EPS bearer identity checking
   */
  else if ( ebi != ESM_EBI_UNASSIGNED ) {
    /* 3GPP TS 24.301, section 7.3.2, case a
     * Reserved or assigned EPS bearer identity value */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid EPS bearer identity (ebi=%d)",
              ebi);
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }

  /*
   * Message processing
   */
  /* Get PDN connection and EPS bearer context data structure to setup */
  esm_proc_data_t *esm_data = (esm_proc_data_t *)(data);

  if (data == NULL) {
    LOG_TRACE(ERROR, "ESM-SAP   - Invalid ESM data structure");
    LOG_FUNC_RETURN (ESM_CAUSE_PROTOCOL_ERROR);
  }

  /* Get the PDN connectivity request type */
  int request_type = -1;

  if (msg->requesttype == REQUEST_TYPE_INITIAL_REQUEST) {
    request_type = ESM_PDN_REQUEST_INITIAL;
  } else if (msg->requesttype == REQUEST_TYPE_HANDOVER) {
    request_type = ESM_PDN_REQUEST_HANDOVER;
  } else if (msg->requesttype == REQUEST_TYPE_EMERGENCY) {
    request_type = ESM_PDN_REQUEST_EMERGENCY;
  } else {
    /* Unkown PDN request type */
    LOG_TRACE(ERROR, "ESM-SAP   - Invalid PDN request type");
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_MANDATORY_INFO);
  }

  /* Get the value of the PDN type indicator */
  if (msg->pdntype == PDN_TYPE_IPV4) {
    esm_data->pdn_type = ESM_PDN_TYPE_IPV4;
  } else if (msg->pdntype == PDN_TYPE_IPV6) {
    esm_data->pdn_type = ESM_PDN_TYPE_IPV6;
  } else if (msg->pdntype == PDN_TYPE_IPV4V6) {
    esm_data->pdn_type = ESM_PDN_TYPE_IPV4V6;
  } else {
    /* Unkown PDN type */
    LOG_TRACE(ERROR, "ESM-SAP   - Invalid PDN type");
    LOG_FUNC_RETURN (ESM_CAUSE_UNKNOWN_PDN_TYPE);
  }

  /* Get the Access Point Name, if provided */
  if (msg->presencemask & PDN_CONNECTIVITY_REQUEST_ACCESS_POINT_NAME_PRESENT) {
    esm_data->apn = msg->accesspointname.accesspointnamevalue;
  }

  /* Get the ESM information transfer flag */
  if (msg->presencemask &
      PDN_CONNECTIVITY_REQUEST_ESM_INFORMATION_TRANSFER_FLAG_PRESENT) {
    /* 3GPP TS 24.301, sections 6.5.1.2, 6.5.1.3
     * ESM information, i.e. protocol configuration options, APN, or both,
     * has to be sent after the NAS signalling security has been activated
     * between the UE and the MME.
     *
     * The MME then at a later stage in the PDN connectivity procedure
     * initiates the ESM information request procedure in which the UE
     * can provide the MME with protocol configuration options or APN
     * or both.
     * The MME waits for completion of the ESM information request
     * procedure before proceeding with the PDN connectivity procedure.
     */
    //TODO: rc = esm_proc_information_request();
  }
  esm_data->pco = msg->protocolconfigurationoptions;
  for (i = 0; i < msg->protocolconfigurationoptions.num_protocol_id_or_container_id; i++) {
    DUP_OCTET_STRING(msg->protocolconfigurationoptions.protocolidcontents[i], esm_data->pco.protocolidcontents[i]);
    esm_data->pco.protocolid[i]         = msg->protocolconfigurationoptions.protocolid[i];
    esm_data->pco.lengthofprotocolid[i] = msg->protocolconfigurationoptions.lengthofprotocolid[i];
  }


#if defined(ORIGINAL_CODE)
  /* Execute the PDN connectivity procedure requested by the UE */
  int pid = esm_proc_pdn_connectivity_request(ctx, pti, request_type,
            &esm_data->apn,
            esm_data->pdn_type,
            &esm_data->pdn_addr,
            &esm_data->qos,
            &esm_cause);

  if (pid != RETURNerror) {
    /* Create local default EPS bearer context */
    int rc = esm_proc_default_eps_bearer_context(ctx, pid, new_ebi,
             &esm_data->qos, &esm_cause);

    if (rc != RETURNerror) {
      esm_cause = ESM_CAUSE_SUCCESS;
    }
  }

#else
  int is_emergency = (request_type == ESM_PDN_REQUEST_EMERGENCY);

  nas_itti_pdn_connectivity_req(
    pti,
    ctx->ueid,
    ctx->imsi,
    esm_data,
    request_type);
  esm_cause = ESM_CAUSE_SUCCESS;
#endif
  /* Return the ESM cause value */
  LOG_FUNC_RETURN (esm_cause);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_pdn_disconnect_request()                         **
 **                                                                        **
 ** Description: Processes PDN disconnect request message                  **
 **                                                                        **
 ** Inputs:  ueid:      UE local identifier                        **
 **      pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      msg:       The received ESM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     linked_ebi:    Linked EPS bearer identity of the default  **
 **             bearer associated with the PDN to discon-  **
 **             nect from                                  **
 **      Return:    ESM cause code whenever the processing of  **
 **             the ESM message fails                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_recv_pdn_disconnect_request(emm_data_context_t *ctx, int pti, int ebi,
                                    const pdn_disconnect_request_msg *msg,
                                    unsigned int *linked_ebi)
{
  LOG_FUNC_IN;

  int esm_cause = ESM_CAUSE_SUCCESS;

  LOG_TRACE(INFO, "ESM-SAP   - Received PDN Disconnect Request message "
            "(ueid=%d, pti=%d, ebi=%d)", ctx->ueid, pti, ebi);

  /*
   * Procedure transaction identity checking
   */
  if ( (pti == ESM_PT_UNASSIGNED) || esm_pt_is_reserved(pti) ) {
    /* 3GPP TS 24.301, section 7.3.1, case b
     * Reserved or unassigned PTI value
     */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid PTI value (pti=%d)", pti);
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_PTI_VALUE);
  }
  /*
   * EPS bearer identity checking
   */
  else if ( ebi != ESM_EBI_UNASSIGNED ) {
    /* 3GPP TS 24.301, section 7.3.2, case b
     * Reserved or assigned EPS bearer identity value */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid EPS bearer identity (ebi=%d)",
              ebi);
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }

  /*
   * Message processing
   */
  /* Execute the PDN disconnect procedure requested by the UE */
  int pid = esm_proc_pdn_disconnect_request(ctx, pti, &esm_cause);

  if (pid != RETURNerror) {
    /* Get the identity of the default EPS bearer context assigned to
     * the PDN connection to disconnect from */
    *linked_ebi = msg->linkedepsbeareridentity;
    /* Release the associated default EPS bearer context */
    int bid = 0;
    int rc = esm_proc_eps_bearer_context_deactivate(ctx, FALSE,
             *linked_ebi,
             &pid, &bid, &esm_cause);

    if (rc != RETURNerror) {
      esm_cause = ESM_CAUSE_SUCCESS;
    }
  }

  /* Return the ESM cause value */
  LOG_FUNC_RETURN (esm_cause);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_activate_default_eps_bearer_context_accept()     **
 **                                                                        **
 ** Description: Processes Activate Default EPS Bearer Context Accept      **
 **      message                                                   **
 **                                                                        **
 ** Inputs:  ueid:      UE local identifier                        **
 **          pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      msg:       The received ESM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    ESM cause code whenever the processing of  **
 **             the ESM message fails                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_recv_activate_default_eps_bearer_context_accept(emm_data_context_t *ctx,
    int pti, int ebi,
    const activate_default_eps_bearer_context_accept_msg *msg)
{
  LOG_FUNC_IN;

  int esm_cause = ESM_CAUSE_SUCCESS;

  LOG_TRACE(INFO, "ESM-SAP   - Received Activate Default EPS Bearer Context "
            "Accept message (ueid=%d, pti=%d, ebi=%d)", ctx->ueid, pti, ebi);

  /*
   * Procedure transaction identity checking
   */
  if (esm_pt_is_reserved(pti)) {
    /* 3GPP TS 24.301, section 7.3.1, case f
     * Reserved PTI value
     */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid PTI value (pti=%d)", pti);
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_PTI_VALUE);
  }
  /*
   * EPS bearer identity checking
   */
  else if ( esm_ebr_is_reserved(ebi) || esm_ebr_is_not_in_use(ctx, ebi) ) {
    /* 3GPP TS 24.301, section 7.3.2, case f
     * Reserved or assigned value that does not match an existing EPS
     * bearer context
     */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid EPS bearer identity (ebi=%d)",
              ebi);
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }

  /*
   * Message processing
   */
  /* Execute the default EPS bearer context activation procedure accepted
   * by the UE */
  int rc = esm_proc_default_eps_bearer_context_accept(ctx, ebi, &esm_cause);

  if (rc != RETURNerror) {
    esm_cause = ESM_CAUSE_SUCCESS;
  }

  /* Return the ESM cause value */
  LOG_FUNC_RETURN (esm_cause);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_activate_default_eps_bearer_context_reject()     **
 **                                                                        **
 ** Description: Processes Activate Default EPS Bearer Context Reject      **
 **      message                                                   **
 **                                                                        **
 ** Inputs:  ueid:      UE local identifier                        **
 **          pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      msg:       The received ESM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    ESM cause code whenever the processing of  **
 **             the ESM message fail                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_recv_activate_default_eps_bearer_context_reject(emm_data_context_t *ctx,
    int pti, int ebi,
    const activate_default_eps_bearer_context_reject_msg *msg)
{
  LOG_FUNC_IN;

  int esm_cause = ESM_CAUSE_SUCCESS;

  LOG_TRACE(INFO, "ESM-SAP   - Received Activate Default EPS Bearer Context "
            "Reject message (ueid=%d, pti=%d, ebi=%d)", ctx->ueid, pti, ebi);

  /*
   * Procedure transaction identity checking
   */
  if (esm_pt_is_reserved(pti)) {
    /* 3GPP TS 24.301, section 7.3.1, case f
     * Reserved PTI value
     */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid PTI value (pti=%d)", pti);
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_PTI_VALUE);
  }
  /*
   * EPS bearer identity checking
   */
  else if ( esm_ebr_is_reserved(ebi) || esm_ebr_is_not_in_use(ctx, ebi) ) {
    /* 3GPP TS 24.301, section 7.3.2, case f
     * Reserved or assigned value that does not match an existing EPS
     * bearer context
     */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid EPS bearer identity (ebi=%d)",
              ebi);
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }

  /*
   * Message processing
   */
  /* Execute the default EPS bearer context activation procedure not accepted
   * by the UE */
  int rc = esm_proc_default_eps_bearer_context_reject(ctx, ebi, &esm_cause);

  if (rc != RETURNerror) {
    esm_cause = ESM_CAUSE_SUCCESS;
  }

  /* Return the ESM cause value */
  LOG_FUNC_RETURN (esm_cause);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_activate_dedicated_eps_bearer_context_accept()   **
 **                                                                        **
 ** Description: Processes Activate Dedicated EPS Bearer Context Accept    **
 **      message                                                   **
 **                                                                        **
 ** Inputs:  ueid:      UE local identifier                        **
 **          pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      msg:       The received ESM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    ESM cause code whenever the processing of  **
 **             the ESM message fails                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_recv_activate_dedicated_eps_bearer_context_accept(emm_data_context_t *ctx,
    int pti, int ebi,
    const activate_dedicated_eps_bearer_context_accept_msg *msg)
{
  LOG_FUNC_IN;

  int esm_cause = ESM_CAUSE_SUCCESS;

  LOG_TRACE(INFO, "ESM-SAP   - Received Activate Dedicated EPS Bearer "
            "Context Accept message (ueid=%d, pti=%d, ebi=%d)",
            ctx->ueid, pti, ebi);

  /*
   * Procedure transaction identity checking
   */
  if (esm_pt_is_reserved(pti)) {
    /* 3GPP TS 24.301, section 7.3.1, case f
     * Reserved PTI value
     */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid PTI value (pti=%d)", pti);
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_PTI_VALUE);
  }
  /*
   * EPS bearer identity checking
   */
  else if ( esm_ebr_is_reserved(ebi) || esm_ebr_is_not_in_use(ctx, ebi) ) {
    /* 3GPP TS 24.301, section 7.3.2, case f
     * Reserved or assigned value that does not match an existing EPS
     * bearer context
     */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid EPS bearer identity (ebi=%d)",
              ebi);
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }

  /*
   * Message processing
   */
  /* Execute the dedicated EPS bearer context activation procedure accepted
   * by the UE */
  int rc = esm_proc_dedicated_eps_bearer_context_accept(ctx, ebi,
           &esm_cause);

  if (rc != RETURNerror) {
    esm_cause = ESM_CAUSE_SUCCESS;
  }

  /* Return the ESM cause value */
  LOG_FUNC_RETURN (esm_cause);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_activate_dedicated_eps_bearer_context_reject()   **
 **                                                                        **
 ** Description: Processes Activate Dedicated EPS Bearer Context Reject    **
 **      message                                                   **
 **                                                                        **
 ** Inputs:  ueid:      UE local identifier                        **
 **          pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      msg:       The received ESM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    ESM cause code whenever the processing of  **
 **             the ESM message fail                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_recv_activate_dedicated_eps_bearer_context_reject(emm_data_context_t *ctx,
    int pti, int ebi,
    const activate_dedicated_eps_bearer_context_reject_msg *msg)
{
  LOG_FUNC_IN;

  int esm_cause = ESM_CAUSE_SUCCESS;

  LOG_TRACE(INFO, "ESM-SAP   - Received Activate Dedicated EPS Bearer "
            "Context Reject message (ueid=%d, pti=%d, ebi=%d)",
            ctx->ueid, pti, ebi);

  /*
   * Procedure transaction identity checking
   */
  if (esm_pt_is_reserved(pti)) {
    /* 3GPP TS 24.301, section 7.3.1, case f
     * Reserved PTI value
     */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid PTI value (pti=%d)", pti);
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_PTI_VALUE);
  }
  /*
   * EPS bearer identity checking
   */
  else if ( esm_ebr_is_reserved(ebi) || esm_ebr_is_not_in_use(ctx, ebi) ) {
    /* 3GPP TS 24.301, section 7.3.2, case f
     * Reserved or assigned value that does not match an existing EPS
     * bearer context
     */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid EPS bearer identity (ebi=%d)",
              ebi);
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }

  /*
   * Message processing
   */
  /* Execute the dedicated EPS bearer context activation procedure not
   *  accepted by the UE */
  int rc = esm_proc_dedicated_eps_bearer_context_reject(ctx, ebi,
           &esm_cause);

  if (rc != RETURNerror) {
    esm_cause = ESM_CAUSE_SUCCESS;
  }

  /* Return the ESM cause value */
  LOG_FUNC_RETURN (esm_cause);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_deactivate_eps_bearer_context_accept()           **
 **                                                                        **
 ** Description: Processes Deactivate EPS Bearer Context Accept message    **
 **                                                                        **
 ** Inputs:  ueid:      UE local identifier                        **
 **          pti:       Procedure transaction identity             **
 **      ebi:       EPS bearer identity                        **
 **      msg:       The received ESM message                   **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    ESM cause code whenever the processing of  **
 **             the ESM message fails                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_recv_deactivate_eps_bearer_context_accept(emm_data_context_t *ctx,
    int pti, int ebi,
    const deactivate_eps_bearer_context_accept_msg *msg)
{
  LOG_FUNC_IN;

  int esm_cause = ESM_CAUSE_SUCCESS;

  LOG_TRACE(INFO, "ESM-SAP   - Received Deactivate EPS Bearer Context "
            "Accept message (ueid=%d, pti=%d, ebi=%d)", ctx->ueid, pti, ebi);

  /*
   * Procedure transaction identity checking
   */
  if (esm_pt_is_reserved(pti)) {
    /* 3GPP TS 24.301, section 7.3.1, case f
     * Reserved PTI value
     */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid PTI value (pti=%d)", pti);
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_PTI_VALUE);
  }
  /*
   * EPS bearer identity checking
   */
  else if ( esm_ebr_is_reserved(ebi) || esm_ebr_is_not_in_use(ctx, ebi) ) {
    /* 3GPP TS 24.301, section 7.3.2, case f
     * Reserved or assigned value that does not match an existing EPS
     * bearer context
     */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid EPS bearer identity (ebi=%d)",
              ebi);
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }

  /*
   * Message processing
   */
  /* Execute the default EPS bearer context activation procedure accepted
   * by the UE */
  int pid = esm_proc_eps_bearer_context_deactivate_accept(ctx, ebi,
            &esm_cause);

  if (pid != RETURNerror) {
    /* Release all the resources reserved for the PDN */
    int rc = esm_proc_pdn_disconnect_accept(ctx, pid, &esm_cause);

    if (rc != RETURNerror) {
      esm_cause = ESM_CAUSE_SUCCESS;
    }
  }

  /* Return the ESM cause value */
  LOG_FUNC_RETURN (esm_cause);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/
