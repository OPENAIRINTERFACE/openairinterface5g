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

#include <stdlib.h> // malloc, free
#include <string.h> // memset

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
 ** Inputs:  **
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
int esm_recv_status(int pti, int ebi, const esm_status_msg *msg)
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
  rc = esm_proc_status_ind(pti, ebi, &esm_cause);

  if (rc != RETURNerror) {
    esm_cause = ESM_CAUSE_SUCCESS;
  }

  /* Return the ESM cause value */
  LOG_FUNC_RETURN (esm_cause);
}

/*
 * --------------------------------------------------------------------------
 * Functions executed by the UE upon receiving ESM message from the network
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_pdn_connectivity_reject()                        **
 **                                                                        **
 ** Description: Processes PDN Connectivity Reject message                 **
 **                                                                        **
 ** Inputs:  pti:       Procedure transaction identity             **
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
int esm_recv_pdn_connectivity_reject(nas_user_t *user, int pti, int ebi,
                                     const pdn_connectivity_reject_msg *msg)
{
  LOG_FUNC_IN;

  int esm_cause;

  LOG_TRACE(INFO, "ESM-SAP   - Received PDN Connectivity Reject message "
            "(pti=%d, ebi=%d, cause=%d)", pti, ebi, msg->esmcause);

  /*
   * Procedure transaction identity checking
   */
  if ( (pti == ESM_PT_UNASSIGNED) || esm_pt_is_reserved(pti) ) {
    /* 3GPP TS 24.301, section 7.3.1, case a
     * Reserved or unassigned PTI value
     */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid PTI value (pti=%d)", pti);
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_PTI_VALUE);
  } else if ( esm_pt_is_not_in_use(user->esm_pt_data, pti) ) {
    /* 3GPP TS 24.301, section 7.3.1, case a
     * Assigned value that does not match any PTI in use
     */
    LOG_TRACE(WARNING, "ESM-SAP   - PTI mismatch (pti=%d)", pti);
    LOG_FUNC_RETURN (ESM_CAUSE_PTI_MISMATCH);
  }
  /*
   * EPS bearer identity checking
   */
  else if ( (ebi != ESM_EBI_UNASSIGNED) || esm_ebr_is_reserved(user->esm_ebr_data, ebi) ) {
    /* 3GPP TS 24.301, section 7.3.2, case a
     * Assigned or reserved EPS bearer identity value */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid EPS bearer identity (ebi=%d)",
              ebi);
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }

  /*
   * Message processing
   */
  /* Get the ESM cause */
  esm_cause = msg->esmcause;

  /* Execute the PDN connectivity procedure not accepted by the network */
  int rc = esm_proc_pdn_connectivity_reject(user, pti, &esm_cause);

  if (rc != RETURNerror) {
    esm_cause = ESM_CAUSE_SUCCESS;
  }

  /* Return the ESM cause value */
  LOG_FUNC_RETURN (esm_cause);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_pdn_disconnect_reject()                          **
 **                                                                        **
 ** Description: Processes PDN Disconnect Reject message                   **
 **                                                                        **
 ** Inputs:  pti:       Procedure transaction identity             **
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
int esm_recv_pdn_disconnect_reject(nas_user_t *user, int pti, int ebi,
                                   const pdn_disconnect_reject_msg *msg)
{
  LOG_FUNC_IN;

  int esm_cause;

  LOG_TRACE(INFO, "ESM-SAP   - Received PDN Disconnect Reject message "
            "(pti=%d, ebi=%d, cause=%d)", pti, ebi, msg->esmcause);

  /*
   * Procedure transaction identity checking
   */
  if ( (pti == ESM_PT_UNASSIGNED) || esm_pt_is_reserved(pti) ) {
    /* 3GPP TS 24.301, section 7.3.1, case b
     * Reserved or unassigned PTI value
     */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid PTI value (pti=%d)", pti);
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_PTI_VALUE);
  } else if ( esm_pt_is_not_in_use(user->esm_pt_data, pti) ) {
    /* 3GPP TS 24.301, section 7.3.1, case b
     * Assigned value that does not match any PTI in use
     */
    LOG_TRACE(WARNING, "ESM-SAP   - PTI mismatch (pti=%d)", pti);
    LOG_FUNC_RETURN (ESM_CAUSE_PTI_MISMATCH);
  }
  /*
   * EPS bearer identity checking
   */
  else if ( (ebi != ESM_EBI_UNASSIGNED) || esm_ebr_is_reserved(user->esm_ebr_data, ebi) ) {
    /* 3GPP TS 24.301, section 7.3.2, case b
     * Assigned or reserved EPS bearer identity value */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid EPS bearer identity (ebi=%d)",
              ebi);
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }

  /*
   * Message processing
   */
  /* Get the ESM cause */
  esm_cause = msg->esmcause;

  /* Execute the PDN disconnect procedure not accepted by the network */
  int rc = esm_proc_pdn_disconnect_reject(user, pti, &esm_cause);

  if (rc != RETURNerror) {
    esm_cause = ESM_CAUSE_SUCCESS;
  }

  /* Return the ESM cause value */
  LOG_FUNC_RETURN (esm_cause);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_activate_default_eps_bearer_context_request()    **
 **                                                                        **
 ** Description: Processes Activate Default EPS Bearer Context Request     **
 **      message                                                   **
 **                                                                        **
 ** Inputs:  pti:       Procedure transaction identity             **
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
int esm_recv_activate_default_eps_bearer_context_request(nas_user_t *user, int pti, int ebi,
    const activate_default_eps_bearer_context_request_msg *msg)
{
  LOG_FUNC_IN;

  int esm_cause = ESM_CAUSE_SUCCESS;
  esm_pt_data_t *esm_pt_data = user->esm_pt_data;

  LOG_TRACE(INFO, "ESM-SAP   - Received Activate Default EPS Bearer Context "
            "Request message (pti=%d, ebi=%d)", pti, ebi);

  /*
   * Procedure transaction identity checking
   */
  if ( (pti == ESM_PT_UNASSIGNED) || esm_pt_is_reserved(pti) ) {
    /* 3GPP TS 24.301, section 7.3.1, case h
     * Reserved or unassigned PTI value
     */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid PTI value (pti=%d)", pti);
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_PTI_VALUE);
  } else if ( esm_pt_is_not_in_use(esm_pt_data, pti) ) {
    /* 3GPP TS 24.301, section 7.3.1, case g
     * Assigned value that does not match any PTI in use
     */
    LOG_TRACE(WARNING, "ESM-SAP   - PTI mismatch (pti=%d)", pti);
    LOG_FUNC_RETURN (ESM_CAUSE_PTI_MISMATCH);
  }
  /*
   * EPS bearer identity checking
   */
  else if ( (ebi == ESM_EBI_UNASSIGNED) || esm_ebr_is_reserved(user->esm_ebr_data, ebi) ) {
    /* 3GPP TS 24.301, section 7.3.2, case g
     * Reserved or unassigned EPS bearer identity value
     */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid EPS bearer identity (ebi=%d)",
              ebi);
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }

  /*
   * Message processing
   */
  esm_proc_qos_t qos = {-1, -1, -1, -1, -1};

  /* Get the maximum bit rate for uplink and downlink */
  if (msg->epsqos.bitRatesExtPresent) {
    qos.mbrUL =
      eps_qos_bit_rate_ext_value(msg->epsqos.bitRatesExt.maxBitRateForUL);
    qos.mbrDL =
      eps_qos_bit_rate_ext_value(msg->epsqos.bitRatesExt.maxBitRateForDL);
  } else if (msg->epsqos.bitRatesPresent) {
    qos.mbrUL =
      eps_qos_bit_rate_value(msg->epsqos.bitRates.maxBitRateForUL);
    qos.mbrDL =
      eps_qos_bit_rate_value(msg->epsqos.bitRates.maxBitRateForDL);
  }

  /* Get the guaranteed bit rate for uplink and downlink */
  if (msg->epsqos.bitRatesExtPresent) {
    qos.gbrUL =
      eps_qos_bit_rate_ext_value(msg->epsqos.bitRatesExt.guarBitRateForUL);
    qos.gbrDL =
      eps_qos_bit_rate_ext_value(msg->epsqos.bitRatesExt.guarBitRateForDL);
  } else if (msg->epsqos.bitRatesPresent) {
    qos.gbrUL =
      eps_qos_bit_rate_value(msg->epsqos.bitRates.guarBitRateForUL);
    qos.gbrDL =
      eps_qos_bit_rate_value(msg->epsqos.bitRates.guarBitRateForDL);
  }

  /* Get the QoS Class Identifier */
  qos.qci = msg->epsqos.qci;
  /* Get the value of the PDN type indicator */
  int pdn_type = -1;

  if (msg->pdnaddress.pdntypevalue == PDN_VALUE_TYPE_IPV4) {
    pdn_type = ESM_PDN_TYPE_IPV4;
  } else if (msg->pdnaddress.pdntypevalue == PDN_VALUE_TYPE_IPV6) {
    pdn_type = ESM_PDN_TYPE_IPV6;
  } else if (msg->pdnaddress.pdntypevalue == PDN_VALUE_TYPE_IPV4V6) {
    pdn_type = ESM_PDN_TYPE_IPV4V6;
  }

  /* Get the ESM cause */
  if (msg->presencemask &
      ACTIVATE_DEFAULT_EPS_BEARER_CONTEXT_REQUEST_ESM_CAUSE_PRESENT) {
    /* The network allocated a PDN address of a PDN type which is different
     * from the requested PDN type */
    esm_cause = msg->esmcause;
  }

  /* Execute the PDN connectivity procedure accepted by the network */
  int pid = esm_proc_pdn_connectivity_accept(user, pti, pdn_type,
            &msg->pdnaddress.pdnaddressinformation,
            &msg->accesspointname.accesspointnamevalue,
            &esm_cause);

  if (pid != RETURNerror) {
    /* Create local default EPS bearer context */
    int rc = esm_proc_default_eps_bearer_context_request(user, pid, ebi, &qos,
             &esm_cause);

    if (rc != RETURNerror) {
      esm_cause = ESM_CAUSE_SUCCESS;
    }
  }

  /* Return the ESM cause value */
  LOG_FUNC_RETURN (esm_cause);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_activate_dedicated_eps_bearer_context_request()  **
 **                                                                        **
 ** Description: Processes Activate Dedicated EPS Bearer Context Request   **
 **      message                                                   **
 **                                                                        **
 ** Inputs:  pti:       Procedure transaction identity             **
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
int esm_recv_activate_dedicated_eps_bearer_context_request(nas_user_t *user, int pti, int ebi,
    const activate_dedicated_eps_bearer_context_request_msg *msg)
{
  LOG_FUNC_IN;

  int esm_cause = ESM_CAUSE_SUCCESS;
  int i;
  int j;
  esm_pt_data_t *esm_pt_data = user->esm_pt_data;

  LOG_TRACE(INFO, "ESM-SAP   - Received Activate Dedicated EPS Bearer "
            "Context Request message (pti=%d, ebi=%d)", pti, ebi);

  /*
   * Procedure transaction identity checking
   */
  if ( esm_pt_is_reserved(pti) ) {
    /* 3GPP TS 24.301, section 7.3.1, case j
     * Reserved PTI value
     */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid PTI value (pti=%d)", pti);
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_PTI_VALUE);
  } else if ( (pti != ESM_PT_UNASSIGNED) && esm_pt_is_not_in_use(esm_pt_data, pti) ) {
    /* 3GPP TS 24.301, section 7.3.1, case i
     * Assigned value that does not match any PTI in use
     */
    LOG_TRACE(WARNING, "ESM-SAP   - PTI mismatch (pti=%d)", pti);
    LOG_FUNC_RETURN (ESM_CAUSE_PTI_MISMATCH);
  }
  /*
   * EPS bearer identity checking
   */
  else if ( (ebi == ESM_EBI_UNASSIGNED) || esm_ebr_is_reserved(user->esm_ebr_data, ebi) ) {
    /* 3GPP TS 24.301, section 7.3.2, case h
     * Reserved or unassigned EPS bearer identity value
     */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid EPS bearer identity (ebi=%d)",
              ebi);
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }
  /*
   * TFT checking
   */
  else if (msg->tft.tftoperationcode != TRAFFIC_FLOW_TEMPLATE_OPCODE_CREATE) {
    /* 3GPP TS 24.301, section 6.4.2.4, case a1
     * Semantic errors in TFT operations
     */
    LOG_FUNC_RETURN (ESM_CAUSE_SEMANTIC_ERROR_IN_THE_TFT_OPERATION);
  } else if (msg->tft.numberofpacketfilters == 0) {
    /* 3GPP TS 24.301, section 6.4.2.4, case b1
     * Syntactical errors in TFT operations
     */
    LOG_FUNC_RETURN (ESM_CAUSE_SYNTACTICAL_ERROR_IN_THE_TFT_OPERATION);
  }

  /*
   * Message processing
   */
  /* Processing of the EPS bearer level QoS parameters */
  esm_proc_qos_t qos = {-1, -1, -1, -1, -1};

  /* Get the maximum bit rate for uplink and downlink */
  if (msg->epsqos.bitRatesExtPresent) {
    qos.mbrUL =
      eps_qos_bit_rate_ext_value(msg->epsqos.bitRatesExt.maxBitRateForUL);
    qos.mbrDL =
      eps_qos_bit_rate_ext_value(msg->epsqos.bitRatesExt.maxBitRateForDL);
  } else if (msg->epsqos.bitRatesPresent) {
    qos.mbrUL =
      eps_qos_bit_rate_value(msg->epsqos.bitRates.maxBitRateForUL);
    qos.mbrDL =
      eps_qos_bit_rate_value(msg->epsqos.bitRates.maxBitRateForDL);
  }

  /* Get the guaranteed bit rate for uplink and downlink */
  if (msg->epsqos.bitRatesExtPresent) {
    qos.gbrUL =
      eps_qos_bit_rate_ext_value(msg->epsqos.bitRatesExt.guarBitRateForUL);
    qos.gbrDL =
      eps_qos_bit_rate_ext_value(msg->epsqos.bitRatesExt.guarBitRateForDL);
  } else if (msg->epsqos.bitRatesPresent) {
    qos.gbrUL =
      eps_qos_bit_rate_value(msg->epsqos.bitRates.guarBitRateForUL);
    qos.gbrDL =
      eps_qos_bit_rate_value(msg->epsqos.bitRates.guarBitRateForDL);
  }

  /* Get the QoS Class Identifier */
  qos.qci = msg->epsqos.qci;

  /* Processing of the traffic flow template parameters */
  esm_proc_tft_t tft = {0, {NULL}};
  /* Get the list of packet filters */
  const PacketFilters *pkfs = &(msg->tft.packetfilterlist.createtft);

  for (i = 0; i < msg->tft.numberofpacketfilters; i++) {
    /* Create new temporary packet filter */
    tft.pkf[i] = (network_pkf_t *)malloc(sizeof(network_pkf_t));

    if (tft.pkf[i] != NULL) {
      /* Initialize the temporary packet filter */
      memset(tft.pkf[i], 0, sizeof(network_pkf_t));
      /* Increment the number of packet filters contained in the TFT */
      tft.n_pkfs += 1;
      /* Packet filter identifier */
      tft.pkf[i]->id = pkfs[i]->identifier;
      /* Packet filter direction */
      tft.pkf[i]->dir = pkfs[i]->direction;
      /* Evaluation precedence */
      tft.pkf[i]->precedence = pkfs[i]->eval_precedence;

      /* Get the packet filter components */
      const PacketFilter *pkf = &(pkfs[i]->packetfilter);

      if (pkf->flags & TRAFFIC_FLOW_TEMPLATE_IPV4_REMOTE_ADDR_FLAG) {
        /* IPv4 remote address component */
        for (j = 0;
             (j < TRAFFIC_FLOW_TEMPLATE_IPV4_ADDR_SIZE)
             && (j < NET_PACKET_FILTER_IPV4_ADDR_SIZE); j++) {
          tft.pkf[i]->data.ipv4.addr[j] = pkf->ipv4remoteaddr[j].addr;
          tft.pkf[i]->data.ipv4.mask[j] = pkf->ipv4remoteaddr[j].mask;
        }
      } else if (pkf->flags & TRAFFIC_FLOW_TEMPLATE_IPV6_REMOTE_ADDR_FLAG) {
        /* IPv6 remote address component */
        for (j = 0;
             (j < TRAFFIC_FLOW_TEMPLATE_IPV6_ADDR_SIZE)
             && (j < NET_PACKET_FILTER_IPV6_ADDR_SIZE); j++) {
          tft.pkf[i]->data.ipv6.addr[j] = pkf->ipv6remoteaddr[j].addr;
          tft.pkf[i]->data.ipv6.mask[j] = pkf->ipv6remoteaddr[j].mask;
        }
      }

      if (pkf->flags & TRAFFIC_FLOW_TEMPLATE_PROTOCOL_NEXT_HEADER_FLAG) {
        /* Protocol identifier/Next header component */
        tft.pkf[i]->data.ipv4.protocol =
          pkf->protocolidentifier_nextheader;
      }

      if (pkf->flags & TRAFFIC_FLOW_TEMPLATE_SINGLE_LOCAL_PORT_FLAG) {
        /* Single local port component */
        tft.pkf[i]->lport = pkf->singlelocalport;
      } else if (pkf->flags & TRAFFIC_FLOW_TEMPLATE_LOCAL_PORT_RANGE_FLAG) {
        /* Local port range component */
        /* TODO: Add port range type to network_pkf_t in networkDef.h */
        tft.pkf[i]->lport = pkf->localportrange.lowlimit;
      }

      if (pkf->flags & TRAFFIC_FLOW_TEMPLATE_SINGLE_REMOTE_PORT_FLAG) {
        /* Single remote port component */
        tft.pkf[i]->rport = pkf->singleremoteport;
      } else if (pkf->flags & TRAFFIC_FLOW_TEMPLATE_REMOTE_PORT_RANGE_FLAG) {
        /* Remote port range component */
        /* TODO: Add port range type to network_pkf_t in networkDef.h */
        tft.pkf[i]->rport = pkf->remoteportrange.lowlimit;
      }

      if (pkf->flags & TRAFFIC_FLOW_TEMPLATE_SECURITY_PARAMETER_INDEX) {
        /* Security parameter index component */
        tft.pkf[i]->data.ipv6.ipsec = pkf->securityparameterindex;
      }

      if (pkf->flags & TRAFFIC_FLOW_TEMPLATE_TYPE_OF_SERVICE_TRAFFIC_CLASS) {
        /* Type of service/Traffic class component */
        tft.pkf[i]->data.ipv4.tos =
          pkf->typdeofservice_trafficclass.value;
      }

      if (pkf->flags & TRAFFIC_FLOW_TEMPLATE_FLOW_LABEL) {
        /* Flow label component */
        tft.pkf[i]->data.ipv6.fl = pkf->flowlabel;
      }
    }
  }

  /* Execute the dedicated EPS bearer context activation procedure */
  int rc = esm_proc_dedicated_eps_bearer_context_request(user, ebi,
           msg->linkedepsbeareridentity,
           &qos, &tft, &esm_cause);

  if (rc != RETURNerror) {
    esm_cause = ESM_CAUSE_SUCCESS;
  }

  /* Release temporary traffic flow template data */
  for (i = 0; i < tft.n_pkfs; i++) {
    free(tft.pkf[i]);
  }

  /* Return the ESM cause value */
  LOG_FUNC_RETURN (esm_cause);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_recv_deactivate_eps_bearer_context_request()          **
 **                                                                        **
 ** Description: Processes Deactivate EPS Bearer Context Request message   **
 **                                                                        **
 ** Inputs:  pti:       Procedure transaction identity             **
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
int esm_recv_deactivate_eps_bearer_context_request(nas_user_t *user, int pti, int ebi,
    const deactivate_eps_bearer_context_request_msg *msg)
{
  LOG_FUNC_IN;

  int rc = RETURNok;
  int esm_cause;
  esm_pt_data_t *esm_pt_data = user->esm_pt_data;

  LOG_TRACE(INFO, "ESM-SAP   - Received Deactivate EPS Bearer Context "
            "Request message (pti=%d, ebi=%d)", pti, ebi);

  /*
   * Procedure transaction identity checking
   */
  if ( esm_pt_is_reserved(pti) ) {
    /* 3GPP TS 24.301, section 7.3.1, case m
     * Reserved PTI value
     */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid PTI value (pti=%d)", pti);
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_PTI_VALUE);
  } else if ( esm_pt_is_not_in_use(esm_pt_data, pti) ) {
    /* 3GPP TS 24.301, section 7.3.1, case m
     * Assigned value does not match any PTI in use
     */
    LOG_TRACE(WARNING, "ESM-SAP   - PTI mismatch (pti=%d)", pti);
    LOG_FUNC_RETURN (ESM_CAUSE_PTI_MISMATCH);
  }
  /*
   * EPS bearer identity checking
   */
  else if ( (ebi == ESM_EBI_UNASSIGNED) || esm_ebr_is_reserved(user->esm_ebr_data, ebi) ||
            esm_ebr_is_not_in_use(user->esm_ebr_data, ebi) ) {
    /* 3GPP TS 24.301, section 7.3.2, case j
     * Reserved or unassigned EPS bearer identity value or,
     * assigned value that does not match an existing EPS bearer context
     */
    LOG_TRACE(WARNING, "ESM-SAP   - Invalid EPS bearer identity (ebi=%d)",
              ebi);
    /* Respond with a DEACTIVATE EPS BEARER CONTEXT ACCEPT message with
     * the EPS bearer identity set to the received EPS bearer identity */
    LOG_FUNC_RETURN (ESM_CAUSE_INVALID_EPS_BEARER_IDENTITY);
  }

  /*
   * Message processing
   */
  /* Get the ESM cause */
  esm_cause = msg->esmcause;

  /* Execute the PDN disconnect procedure accepted by the network */
  if (pti != ESM_PT_UNASSIGNED) {
    rc = esm_proc_pdn_disconnect_accept(esm_pt_data, pti, &esm_cause);
  }

  if (rc != RETURNerror) {
    /* Execute the EPS bearer context deactivation procedure */
    rc = esm_proc_eps_bearer_context_deactivate_request(user, ebi, &esm_cause);

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
