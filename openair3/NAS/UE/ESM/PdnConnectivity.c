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

#include "emm_sap.h"

# include "assertions.h"

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *  Internal data handled by the PDN connectivity procedure in the UE
 * --------------------------------------------------------------------------
 */
/*
 * PDN connection handlers
 */
static int _pdn_connectivity_create(esm_data_t *esm_data, int pid, const OctetString *apn,
                                    esm_proc_pdn_type_t pdn_type, bool is_emergency);
static int _pdn_connectivity_update(esm_data_t *esm_data, int pid, const OctetString *apn,
                                    esm_proc_pdn_type_t pdn_type, const OctetString *pdn_addr, int esm_cause);
static int _pdn_connectivity_delete(esm_data_t *esm_data, int pid);

static int _pdn_connectivity_set_pti(esm_data_t *esm_data, int pid, int pti);
static int _pdn_connectivity_find_apn(esm_data_t *esm_data, const OctetString *apn);
static int _pdn_connectivity_find_pdn(esm_data_t * esm_data, const OctetString *apn,
                                      esm_proc_pdn_type_t pdn_type);

/*
 * Timer handlers
 */
static void *_pdn_connectivity_t3482_handler(void *);

/* Maximum value of the PDN connectivity request retransmission counter */
#define ESM_PDN_CONNECTIVITY_COUNTER_MAX 5


/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/*
 * --------------------------------------------------------------------------
 *      PDN connectivity procedure executed by the UE
 * --------------------------------------------------------------------------
 */
/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_pdn_connectivity()                               **
 **                                                                        **
 ** Description: Defines a new PDN connection for the specified Access     **
 **      Point Name or undefines the PDN connection with the given **
 **      identifier                                                **
 **                                                                        **
 ** Inputs:  cid:       PDN connection identifier                  **
 **      is_to_define:  Indicates whether the PDN connection has   **
 **             to be defined or undefined                 **
 **      pdn_type:  PDN connection type (IPv4, IPv6, IPv4v6)   **
 **      apn:       Access Point logical Name to be used       **
 **      is_emergency:  true if the PDN connection has to be esta- **
 **             blished for emergency bearer services      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     pti:       Procedure transaction identity assigned to **
 **             the new PDN connection or the released PDN **
 **             connection                                 **
 **      Return:    RETURNok, RETURNerror                      **
 **                                                                        **
 ***************************************************************************/
int esm_proc_pdn_connectivity(nas_user_t *user, int cid, int is_to_define,
                              esm_proc_pdn_type_t pdn_type,
                              const OctetString *apn, bool is_emergency,
                              unsigned int *pti)
{
    LOG_FUNC_IN;

    int rc = RETURNerror;
    int pid = cid - 1;
    esm_data_t *esm_data = user-> esm_data;
    esm_pt_data_t *esm_pt_data = user-> esm_pt_data;

    if (!is_to_define) {
        LOG_TRACE(INFO, "ESM-PROC  - Undefine PDN connection (cid=%d)", cid);
        /* Delete the PDN connection entry */
        int pti = _pdn_connectivity_delete(esm_data, pid);

        if (pti != ESM_PT_UNASSIGNED) {
            /* Release the procedure transaction data */
            rc = esm_pt_release(esm_pt_data, pti);
        }

        LOG_FUNC_RETURN(rc);
    } else if (pti != NULL) {
        LOG_TRACE(INFO, "ESM-PROC  - Assign new procedure transaction identity "
                  "(cid=%d)", cid);
        /* Assign new procedure transaction identity */
        *pti = esm_pt_assign(esm_pt_data);

        if (*pti == ESM_PT_UNASSIGNED) {
            LOG_TRACE(WARNING, "ESM-PROC  - Failed to assign new procedure "
                      "transaction identity");
            LOG_FUNC_RETURN (RETURNerror);
        }

        /* Update the PDN connection data */
        rc = _pdn_connectivity_set_pti(esm_data, pid, *pti);

        if (rc != RETURNok) {
            LOG_TRACE(WARNING, "ESM-PROC  - Failed to update PDN connection");
        }

        LOG_FUNC_RETURN (rc);
    }

    LOG_TRACE(INFO,"ESM-PROC  - Define new %s PDN connection to APN %s (cid=%d)",
              (pdn_type == ESM_PDN_TYPE_IPV4)? "IPv4" :
              (pdn_type == ESM_PDN_TYPE_IPV6)? "IPv6" : "IPv4v6",
              (apn!=NULL)?apn->value:0, cid);

    if (is_emergency && esm_data->emergency) {
        /* The UE shall not request additional PDN connection for
         * emergency bearer services */
        LOG_TRACE(WARNING, "ESM-PROC  - PDN connection for emergency bearer "
                  "services is already active");
        LOG_FUNC_RETURN (RETURNerror);
    } else if (pid < ESM_DATA_PDN_MAX) {
        if ((pid == esm_data->pdn[pid].pid) && (esm_data->pdn[pid].is_active)) {
            /* PDN connection with the specified identifier is active */
            LOG_TRACE(WARNING, "ESM-PROC  - PDN connection is active");
            LOG_FUNC_RETURN (RETURNerror);
        }
    } else {
        LOG_TRACE(WARNING, "ESM-PROC  - PDN connection identifier is not valid");
        LOG_FUNC_RETURN (RETURNerror);
    }

    if (apn && apn->length > 0) {
        /* The UE requested subsequent connectivity to additionnal PDNs */
        int pid = _pdn_connectivity_find_apn(esm_data, apn);

        if ( (pid >= 0) && esm_data->pdn[pid].is_active ) {
            /* An active PDN connection to this APN already exists */
            if ( (esm_data->pdn[pid].data->type != ESM_PDN_TYPE_IPV4V6) &&
                    (esm_data->pdn[pid].data->type != pdn_type) ) {
                /* The UE is requesting PDN connection for other IP version
                 * than the one already activated */
                if (!esm_data->pdn[pid].data->addr_realloc) {
                    /* The network does not allow PDN connectivity using
                     * IPv4 and IPv6 address versions to the same APN */
                    if (pdn_type != ESM_PDN_TYPE_IPV4V6) {
                        LOG_TRACE(WARNING, "ESM-PROC  - %s PDN connectivity to "
                                  "%s is not allowed by the network",
                                  (pdn_type != ESM_PDN_TYPE_IPV4)? "IPv6" :
                                  "IPv4", apn->value);
                    } else {
                        LOG_TRACE(WARNING, "ESM-PROC  - %s PDN connection to %s "
                                  "already exists",
                                  (esm_data->pdn[pid].data->type !=
                                   ESM_PDN_TYPE_IPV4)? "IPv6" : "IPv4",
                                  apn->value);
                    }

                    LOG_FUNC_RETURN (RETURNerror);
                }
            } else {
                /* The UE is requesting PDN connection to this APN using the
                 * same IP version than the one already activated */
                LOG_TRACE(WARNING, "ESM-PROC  - %s PDN connection to %s "
                          "already exists",
                          (esm_data->pdn[pid].data->type != ESM_PDN_TYPE_IPV4)?
                          (esm_data->pdn[pid].data->type != ESM_PDN_TYPE_IPV6)?
                          "IPv4v6" : "IPv6" : "IPv4", apn->value);
                LOG_FUNC_RETURN (RETURNerror);
            }
        }
    }

    /*
     * New PDN context has to be defined to allow connectivity to an APN:
     * The UE may attempt to attach to the network using the default APN,
     * or request PDN connectivity to emergency bearer services. The UE
     * may also subsequently request connectivity to additional PDNs if
     * not already established, or may have been allowed to request PDN
     * connectivity for other IP version than the one already activated
     */
    rc = _pdn_connectivity_create(esm_data, pid, apn, pdn_type, is_emergency);
    LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_pdn_connectivity_request()                       **
 **                                                                        **
 ** Description: Initiates PDN connectivity procedure to request setup of  **
 **      a default EPS bearer to a PDN.                            **
 **                                                                        **
 **              3GPP TS 24.301, section 6.5.1.2                           **
 **      The UE requests connectivity to an additional PDN by sen- **
 **      ding a PDN CONNECTIVITY REQUEST message to the MME, star- **
 **      ting timer T3482 and entering state PROCEDURE TRANSACTION **
 **      PENDING.                                                  **
 **                                                                        **
 ** Inputs:  is_standalone: Indicates whether the PDN connectivity     **
 **             procedure is initiated as part of the at-  **
 **             tach procedure                             **
 **      pti:       Procedure transaction identity             **
 **      msg:       Encoded PDN connectivity request message   **
 **             to be sent                                 **
 **      sent_by_ue:    Not used - Always true                     **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_pdn_connectivity_request(nas_user_t *user, bool is_standalone, int pti,
                                      OctetString *msg, bool sent_by_ue)
{
    LOG_FUNC_IN;
    esm_pt_data_t *esm_pt_data = user->esm_pt_data;
    int rc = RETURNok;

    LOG_TRACE(INFO, "ESM-PROC  - Initiate PDN connectivity (pti=%d)", pti);

    if (is_standalone) {
        emm_sap_t emm_sap;
        emm_esm_data_t *emm_esm = &emm_sap.u.emm_esm.u.data;
        /*
         * Notity EMM that ESM PDU has to be forwarded to lower layers
         */
        emm_sap.primitive = EMMESM_UNITDATA_REQ;
        emm_sap.u.emm_esm.ueid = user->ueid;
        emm_esm->msg.length = msg->length;
        emm_esm->msg.value = msg->value;
        rc = emm_sap_send(user, &emm_sap);

        if (rc != RETURNerror) {
            /* Start T3482 retransmission timer */
            rc = esm_pt_start_timer(user, pti, msg, T3482_DEFAULT_VALUE,
                                    _pdn_connectivity_t3482_handler);
        }
    }

    if (rc != RETURNerror) {
        /* Set the procedure transaction state to PENDING */
        rc = esm_pt_set_status(esm_pt_data, pti, ESM_PT_PENDING);

        if (rc != RETURNok) {
            /* The procedure transaction was already in PENDING state */
            LOG_TRACE(WARNING, "ESM-PROC  - PTI %d was already PENDING", pti);
        }
    }

    LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_pdn_connectivity_accept()                        **
 **                                                                        **
 ** Description: Performs PDN connectivity procedure accepted by the net-  **
 **      work.                                                     **
 **                                                                        **
 **              3GPP TS 24.301, section 6.5.1.3                           **
 **      The UE shall stop timer T3482 and enter the state PROCE-  **
 **      DURE TRANSACTION INACTIVE.                                **
 **                                                                        **
 ** Inputs:  pti:       Identifies the UE requested PDN connecti-  **
 **             vity procedure accepted by the network     **
 **      pdn_type:  PDN type value (IPv4, IPv6, IPv4v6)        **
 **      pdn_addr:  PDN address                                **
 **      apn:       Access Point Name of the PDN connection    **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     esm_cause: Cause code returned upon ESM procedure     **
 **             failure                                    **
 **      Return:    The identifier of the PDN connection when  **
 **             successfully updated;                      **
 **             RETURNerror otherwise.                     **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_pdn_connectivity_accept(nas_user_t *user, int pti, esm_proc_pdn_type_t pdn_type,
                                     const OctetString *pdn_addr,
                                     const OctetString *apn, int *esm_cause)
{
    LOG_FUNC_IN;
    esm_data_t *esm_data  = user->esm_data;
    esm_pt_data_t *esm_pt_data = user->esm_pt_data;
    int     rc;
    int     pid = RETURNerror;
    char    apn_first_char[4];

    char str[128] __attribute__((unused));

    if (isprint(apn->value[0])) {
        apn_first_char[0] = '\0';
    } else {
        sprintf (apn_first_char, "%02X", apn->value[0]);
    }

    LOG_TRACE(INFO, "ESM-PROC  - PDN connectivity accepted by the network "
              "(pti=%d) APN = %s\"%s\", IP address = %s", pti, apn_first_char, isprint(apn->value[0]) ? &apn->value[0] : &apn->value[1],
              (pdn_type == ESM_PDN_TYPE_IPV4)? esm_data_get_ipv4_addr(pdn_addr, str) :
              (pdn_type == ESM_PDN_TYPE_IPV6)? esm_data_get_ipv6_addr(pdn_addr, str) :
              esm_data_get_ipv4v6_addr(pdn_addr, str));

    /* Stop T3482 timer if running */
    esm_pt_stop_timer(esm_pt_data, pti);
    /* Set the procedure transaction state to INACTIVE */
    rc = esm_pt_set_status(esm_pt_data, pti, ESM_PT_INACTIVE);

    if (rc != RETURNok) {
        /* The procedure transaction was already in INACTIVE state
         * as the request may have already been accepted; consider
         * this request message with same PTI as a network re-
         * transmission */
        LOG_TRACE(WARNING, "ESM-PROC  - PTI %d network retransmission", pti);
        *esm_cause = ESM_CAUSE_PTI_ALREADY_IN_USE;
    } else {
        /* XXX - 3GPP TS 24.301, section 6.5.1.3 and 7.3.1
         * The UE should ensure that the procedure transaction identity
         * (PTI) assigned to this procedure is not released immediately.
         * While the PTI value is not released, the UE regards any received
         * ACTIVATE DEFAULT EPS BEARER CONTEXT REQUEST message with the same
         * PTI value as a network retransmission.
         * The way to achieve this is implementation dependent.
         */

        /* Check whether a PDN connection exists to this APN */
        pid = _pdn_connectivity_find_pdn(esm_data, apn, pdn_type);

        if (pid < 0) {
            /* No any PDN connection has been defined to establish connectivity
             * to this APN */
            LOG_TRACE(WARNING, "ESM-PROC  - PDN connection entry for "
                      "APN \"%s\" (type=%d) not found", apn->value, pdn_type);
            *esm_cause = ESM_CAUSE_UNKNOWN_ACCESS_POINT_NAME;
            LOG_FUNC_RETURN(RETURNerror);
        }

        /* Update the PDN connection */
        rc = _pdn_connectivity_update(esm_data, pid, apn, pdn_type, pdn_addr, *esm_cause);

        if (rc != RETURNok) {
            LOG_TRACE(WARNING, "ESM-PROC  - Failed to update PDN connection "
                      "(pid=%d)", pid);
            *esm_cause = ESM_CAUSE_REQUEST_REJECTED_UNSPECIFIED;
            LOG_FUNC_RETURN(RETURNerror);
        }
    }

    LOG_FUNC_RETURN (pid);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_pdn_connectivity_reject()                        **
 **                                                                        **
 ** Description: Performs PDN connectivity procedure not accepted by the   **
 **      network.                                                  **
 **                                                                        **
 **              3GPP TS 24.301, section 6.5.1.4                           **
 **      Upon receipt of the PDN CONNECTIVITY REJECT message, the  **
 **      UE shall stop timer T3482 and enter the state PROCEDURE   **
 **      TRANSACTION INACTIVE.                                     **
 **                                                                        **
 ** Inputs:  pti:       Identifies the UE requested PDN connecti-  **
 **             vity procedure rejected by the network     **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     esm_cause: Cause code returned upon ESM procedure     **
 **             failure                                    **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_pdn_connectivity_reject(nas_user_t *user, int pti, int *esm_cause)
{
    LOG_FUNC_IN;
    esm_pt_data_t *esm_pt_data = user->esm_pt_data;
    int rc;

    LOG_TRACE(WARNING, "ESM-PROC  - PDN connectivity rejected by "
              "the network (pti=%d), ESM cause = %d", pti, *esm_cause);

    /* Stop T3482 timer if running */
    (void) esm_pt_stop_timer(esm_pt_data, pti);
    /* Set the procedure transaction state to INACTIVE */
    rc = esm_pt_set_status(esm_pt_data, pti, ESM_PT_INACTIVE);

    if (rc != RETURNok) {
        /* The procedure transaction was already in INACTIVE state */
        LOG_TRACE(WARNING, "ESM-PROC  - PTI %d was already INACTIVE", pti);
        *esm_cause = ESM_CAUSE_MESSAGE_TYPE_NOT_COMPATIBLE;
    } else {
        /* Release the procedure transaction identity */
        rc = esm_pt_release(user->esm_pt_data, pti);

        if (rc != RETURNok) {
            LOG_TRACE(WARNING, "ESM-PROC  - Failed to release PTI %d", pti);
            *esm_cause = ESM_CAUSE_REQUEST_REJECTED_UNSPECIFIED;
        }
    }

    LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_pdn_connectivity_complete()                      **
 **                                                                        **
 ** Description: Terminates the PDN connectivity procedure upon receiving  **
 **      indication from the EPS Mobility Management sublayer that **
 **      the ACTIVATE DEFAULT EPS BEARER CONTEXT ACCEPT message    **
 **      has been successfully delivered to the MME.               **
 **                                                                        **
 **      The UE releases the transaction identity assigned to this **
 **      PDN connectivity procedure.                               **
 **                                                                        **
 ** Inputs:  None                                                      **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_pdn_connectivity_complete(nas_user_t *user)
{
    LOG_FUNC_IN;
    esm_pt_data_t *esm_pt_data = user->esm_pt_data;
    int rc = RETURNerror;

    LOG_TRACE(INFO, "ESM-PROC  - PDN connectivity complete");

    /* Get the procedure transaction identity assigned to the PDN connection
     * entry which is still pending in the inactive state */
    int pti = esm_pt_get_pending_pti(esm_pt_data, ESM_PT_INACTIVE);

    if (pti != ESM_PT_UNASSIGNED) {
        /* Release the procedure transaction identity */
        rc = esm_pt_release(esm_pt_data, pti);
    }

    LOG_FUNC_RETURN(rc);
}

/****************************************************************************
 **                                                                        **
 ** Name:    esm_proc_pdn_connectivity_failure()                       **
 **                                                                        **
 ** Description: Performs PDN connectivity procedure upon receiving trans- **
 **      mission failure of ESM message indication from the EPS    **
 **      Mobility Management sublayer                              **
 **                                                                        **
 **      The UE releases the transaction identity allocated to the **
 **      PDN connectivity procedure which is still pending in the  **
 **      PROCEDURE TRANSACTION INACTIVE or PENDING state.          **
 **                                                                        **
 ** Inputs:  is_pending:    true if this PDN connectivity procedure    **
 **             transaction is in the PENDING state        **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
int esm_proc_pdn_connectivity_failure(nas_user_t *user, int is_pending)
{
    LOG_FUNC_IN;
    esm_pt_data_t *esm_pt_data = user->esm_pt_data;
    int rc;
    int pti;

    LOG_TRACE(WARNING, "ESM-PROC  - PDN connectivity failure in state %s",
              (is_pending)? "PENDING" : "INACTIVE");

    if (is_pending) {
        /* Get the procedure transaction identity assigned to the pending PDN
         * connection entry */
        pti = esm_pt_get_pending_pti(esm_pt_data, ESM_PT_PENDING);

        if (pti == ESM_PT_UNASSIGNED) {
            LOG_TRACE(ERROR, "ESM-PROC  - No procedure transaction is PENDING");
            return (RETURNerror);
        }

        /* Set the procedure transaction state to INACTIVE */
        (void) esm_pt_set_status(esm_pt_data, pti, ESM_PT_INACTIVE);
    } else {
        /* Get the procedure transaction identity assigned to the PDN
         * connection entry which is still pending in the inactive state */
        pti = esm_pt_get_pending_pti(esm_pt_data, ESM_PT_INACTIVE);
    }

    /* Release the procedure transaction identity */
    rc = esm_pt_release(esm_pt_data, pti);

    if (rc != RETURNok) {
        LOG_TRACE(WARNING, "ESM-PROC  - Failed to release PTI %d", pti);
    }

    LOG_FUNC_RETURN(rc);
}



/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/*
 *---------------------------------------------------------------------------
 *              Timer handlers
 *---------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    _pdn_connectivity_t3482_handler()                         **
 **                                                                        **
 ** Description: T3482 timeout handler                                     **
 **                                                                        **
 **              3GPP TS 24.301, section 6.5.1.5, case a                   **
 **      On the first expiry of the timer T3482, the UE shall re-  **
 **      send the PDN CONNECTIVITY REQUEST and shall reset and re- **
 **      start timer T3482. This retransmission is repeated four   **
 **      times, i.e. on the fifth expiry of timer T3482, the UE    **
 **      shall abort the procedure, release the PTI allocated for  **
 **      this invocation and enter the state PROCEDURE TRANSACTION **
 **      INACTIVE.                                                 **
 **                                                                        **
 ** Inputs:  args:      handler parameters                         **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    None                                       **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
// FIXME
static void *_pdn_connectivity_t3482_handler(void *args)
{
    LOG_FUNC_IN;

    int rc;

    /* Get retransmission timer parameters data */
    esm_pt_timer_data_t *data = args;
    nas_user_t *user = data->user;
    esm_pt_data_t *esm_pt_data = user->esm_pt_data;

    /* Increment the retransmission counter */
    data->count += 1;

    LOG_TRACE(WARNING, "ESM-PROC  - T3482 timer expired (pti=%d), "
              "retransmission counter = %d", data->pti, data->count);

    if (data->count < ESM_PDN_CONNECTIVITY_COUNTER_MAX) {
        emm_sap_t emm_sap;
        emm_esm_data_t *emm_esm = &emm_sap.u.emm_esm.u.data;
        /*
         * Notify EMM that the PDN connectivity request message
         * has to be sent again
         */
        emm_sap.primitive = EMMESM_UNITDATA_REQ;
        emm_sap.u.emm_esm.ueid = user->ueid;
        emm_esm->msg.length = data->msg.length;
        emm_esm->msg.value = data->msg.value;
        rc = emm_sap_send(user, &emm_sap);

        if (rc != RETURNerror) {
            /* Restart the timer T3482 */
            rc = esm_pt_start_timer(user, data->pti, &data->msg, T3482_DEFAULT_VALUE,
                                    _pdn_connectivity_t3482_handler);
        }
    } else {
        /* Set the procedure transaction state to INACTIVE */
        rc = esm_pt_set_status(esm_pt_data, data->pti, ESM_PT_INACTIVE);

        if (rc != RETURNok) {
            /* The procedure transaction was already in INACTIVE state */
            LOG_TRACE(WARNING, "ESM-PROC  - PTI %d was already INACTIVE",
                      data->pti);
        } else {
            /* Release the transaction identity assigned to this procedure */
            rc = esm_pt_release(esm_pt_data, data->pti);

            if (rc != RETURNok) {
                LOG_TRACE(WARNING, "ESM-PROC  - Failed to release PTI %d",
                          data->pti);
            }
        }
    }

    LOG_FUNC_RETURN(NULL);
}

/*
 *---------------------------------------------------------------------------
 *              PDN connection handlers
 *---------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:    _pdn_connectivity_create()                                **
 **                                                                        **
 ** Description: Creates a new PDN connection entry or updates existing    **
 **      non-active PDN connection entry                           **
 **                                                                        **
 ** Inputs:  pid:       Identifier of the PDN connection entry     **
 **      apn:       Access Point Name of the PDN connection    **
 **      pdn_type:  PDN type (IPv4, IPv6, IPv4v6)              **
 **      is_emergency:  true if the PDN connection has to be esta- **
 **             blished for emergency bearer services      **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **                                                                        **
 ***************************************************************************/
static int _pdn_connectivity_create(esm_data_t *esm_data, int pid, const OctetString *apn,
                                    esm_proc_pdn_type_t pdn_type,
                                    bool is_emergency)
{
    esm_pdn_t *pdn = NULL;

    LOG_TRACE(INFO, "ESM-PROC  - Create new PDN connection (pid=%d)", pid);

    if (pid >= ESM_DATA_PDN_MAX) {
        return (RETURNerror);
    } else if (esm_data->pdn[pid].is_active) {
        LOG_TRACE(ERROR, "ESM-PROC  - PDN connection is active");
        return (RETURNerror);
    }

    if (esm_data->pdn[pid].data != NULL) {
        /* Update existing non-active PDN connection */
        pdn = esm_data->pdn[pid].data;
    } else {
        /* Create new PDN connection */
        pdn = (esm_pdn_t *)malloc(sizeof(esm_pdn_t));

        if (pdn == NULL) {
            LOG_TRACE(WARNING, "ESM-PROC  - "
                      "Failed to create new PDN connection");
            return (RETURNerror);
        }

        memset(pdn, 0, sizeof(esm_pdn_t));
        /* Increment the number of PDN connections */
        esm_data->n_pdns += 1;
        /* Set the PDN connection identifier */
        esm_data->pdn[pid].pid = pid;
        /* Reset the PDN connection active indicator */
        esm_data->pdn[pid].is_active = false;
        /* Setup the PDN connection data */
        esm_data->pdn[pid].data = pdn;
    }

    /* Update the PDN connection data */
    pdn->is_emergency = is_emergency;

    if ( apn && (apn->length > 0) ) {
        if (pdn->apn.length > 0) {
            free(pdn->apn.value);
            pdn->apn.length = 0;
        }

        pdn->apn.value = (uint8_t *)malloc(apn->length + 1);

        if (pdn->apn.value) {
            pdn->apn.length = apn->length;
            memcpy(pdn->apn.value, apn->value, apn->length);
            pdn->apn.value[pdn->apn.length] = '\0';
        }
    }

    pdn->type = pdn_type;
    pdn->addr_realloc = false;

    return (RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _pdn_connectivity_update()                                **
 **                                                                        **
 ** Description: Updates PDN connection entry with the given identifier    **
 **                                                                        **
 ** Inputs:  pid:       Identifier of the PDN connection entry     **
 **      pdn_type:  PDN type (IPv4, IPv6, IPv4v6)              **
 **      pdn_addr:  Network allocated PDN IPv4 or IPv6 address **
 **      esm_cause: ESM cause code                             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **                                                                        **
 ***************************************************************************/
static int _pdn_connectivity_update(esm_data_t *esm_data, int pid, const OctetString *apn,
                                    esm_proc_pdn_type_t pdn_type,
                                    const OctetString *pdn_addr,
                                    int esm_cause)
{
    LOG_TRACE(INFO, "ESM-PROC  - Update PDN connection (pid=%d)", pid);

    if (pid >= ESM_DATA_PDN_MAX) {
        return (RETURNerror);
    } else if (pid != esm_data->pdn[pid].pid) {
        LOG_TRACE(ERROR, "ESM-PROC  - PDN connection identifier is not valid");
        return (RETURNerror);
    } else if (esm_data->pdn[pid].data == NULL) {
        LOG_TRACE(ERROR, "ESM-PROC  - PDN connection has not been allocated");
        return (RETURNerror);
    } else if (esm_data->pdn[pid].is_active) {
        LOG_TRACE(WARNING, "ESM-PROC  - Active %s PDN connection to %s already "
                  "exists", (esm_data->pdn[pid].data->type != ESM_PDN_TYPE_IPV4)?
                  "IPv6" : "IPv4", esm_data->pdn[pid].data->apn.value);
        return (RETURNerror);
    }

    /* Get the PDN connection */
    esm_pdn_t *pdn = esm_data->pdn[pid].data;

    /* Setup the Access Point Name value */
    if ( apn && (apn->length > 0) ) {
        if (pdn->apn.length > 0) {
            free(pdn->apn.value);
            pdn->apn.length = 0;
        }

        pdn->apn.value = (uint8_t *)malloc(apn->length + 1);

        if (pdn->apn.value) {
            pdn->apn.length = apn->length;
            memcpy(pdn->apn.value, apn->value, apn->length);
            pdn->apn.value[pdn->apn.length] = '\0';
        }
    }

    /* Setup the IP address allocated by the network */
    if ( pdn_addr && (pdn_addr->length > 0) ) {
        int length = ((pdn_addr->length < ESM_DATA_IP_ADDRESS_SIZE) ?
                      pdn_addr->length : ESM_DATA_IP_ADDRESS_SIZE);
        memcpy(pdn->ip_addr, pdn_addr->value, length);
        pdn->type = pdn_type;
    }

    /*
     * 3GPP TS 24.301, section 6.2.2
     * Update the address re-allocation indicator
     */
    if (esm_cause == ESM_CAUSE_SINGLE_ADDRESS_BEARERS_ONLY_ALLOWED) {
        /* The UE requested IPv4 or IPv6 address and the network allows
         * single addressing per bearer:
         * The UE should subsequently request another PDN connection for
         * the other IP version using the UE requested PDN connectivity
         * procedure to the same APN with a single address PDN type
         * (IPv4 or IPv6) other than the one already activated */
        pdn->addr_realloc = true;
    } else if ( (esm_cause == ESM_CAUSE_PDN_TYPE_IPV4_ONLY_ALLOWED) ||
                (esm_cause == ESM_CAUSE_PDN_TYPE_IPV6_ONLY_ALLOWED) ) {
        /* The UE requested IPv4 or IPv6 address and the network allows
         * IPv4 or IPv6 PDN address only:
         * The UE shall not subsequently initiate another UE requested
         * PDN connectivity procedure to the same APN to obtain a PDN
         * type different from the one allowed by the network */
        pdn->addr_realloc = false;
    } else if (pdn_type != ESM_PDN_TYPE_IPV4V6) {
        pdn->addr_realloc = true;
    }

    return (RETURNok);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _pdn_connectivity_delete()                                **
 **                                                                        **
 ** Description: Deletes the PDN connection entry with given identifier    **
 **                                                                        **
 ** Inputs:  pid:       Identifier of the PDN connection to be     **
 **             released                                   **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The identity of the procedure transaction  **
 **             assigned to the PDN connection when suc-   **
 **             cessfully released;                        **
 **             UNASSIGNED value otherwise.                **
 **                                                                        **
 ***************************************************************************/
static int _pdn_connectivity_delete(esm_data_t *esm_data, int pid)
{
    int pti = ESM_PT_UNASSIGNED;

    if (pid < ESM_DATA_PDN_MAX) {
        if (pid != esm_data->pdn[pid].pid) {
            LOG_TRACE(ERROR,
                      "ESM-PROC  - PDN connection identifier is not valid");
        } else if (esm_data->pdn[pid].data == NULL) {
            LOG_TRACE(ERROR,
                      "ESM-PROC  - PDN connection has not been allocated");
        } else if (esm_data->pdn[pid].is_active) {
            LOG_TRACE(ERROR, "ESM-PROC  - PDN connection is active");
        } else {
            /* Get the identity of the procedure transaction that created
             * the PDN connection */
            pti = esm_data->pdn[pid].data->pti;
        }
    }

    if (pti != ESM_PT_UNASSIGNED) {
        /* Decrement the number of PDN connections */
        esm_data->n_pdns -= 1;
        /* Set the PDN connection as available */
        esm_data->pdn[pid].pid = -1;

        /* Release allocated PDN connection data */
        if (esm_data->pdn[pid].data->apn.length > 0) {
            free(esm_data->pdn[pid].data->apn.value);
        }

        free(esm_data->pdn[pid].data);
        esm_data->pdn[pid].data = NULL;
        LOG_TRACE(WARNING, "ESM-PROC  - PDN connection %d released", pid);
    }

    /* Return the procedure transaction identity */
    return (pti);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _pdn_connectivity_set_pti()                               **
 **                                                                        **
 ** Description: Update the procedure transaction identity assigned to the **
 **      PDN connection entry with the given identifier            **
 **                                                                        **
 ** Inputs:  pid:       PDN connection identifier                  **
 **      pti:       Procedure transaction identity             **
 **      Others:    None                                       **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    RETURNok, RETURNerror                      **
 **                                                                        **
 ***************************************************************************/
static int _pdn_connectivity_set_pti(esm_data_t *esm_data, int pid, int pti)
{
    if (pid < ESM_DATA_PDN_MAX) {
        if (pid != esm_data->pdn[pid].pid) {
            LOG_TRACE(ERROR,
                      "ESM-PROC  - PDN connection identifier is not valid");
        } else if (esm_data->pdn[pid].data == NULL) {
            LOG_TRACE(ERROR,
                      "ESM-PROC  - PDN connection has not been allocated");
        } else if (esm_data->pdn[pid].is_active) {
            LOG_TRACE(ERROR, "ESM-PROC  - PDN connection is active");
        } else {
            /* Update the identity of the procedure transaction assigned to
             * the PDN connection */
            esm_data->pdn[pid].data->pti = pti;
            return (RETURNok);
        }
    }

    return (RETURNerror);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _pdn_connectivity_find_apn()                              **
 **                                                                        **
 ** Description: Search the list of PDN connections for an entry defined   **
 **      for the specified APN                                     **
 **                                                                        **
 ** Inputs:  apn:       Access Point Name of the PDN connection    **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The identifier of the PDN connection if    **
 **             found in the list; -1 otherwise.           **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _pdn_connectivity_find_apn(esm_data_t *esm_data, const OctetString *apn)
{
    int i;

    for (i = 0; i < ESM_DATA_PDN_MAX; i++) {
        if ( (esm_data->pdn[i].pid != -1) && esm_data->pdn[i].data ) {
            if (esm_data->pdn[i].data->apn.length != apn->length) {
                continue;
            }

            if (memcmp(esm_data->pdn[i].data->apn.value,
                       apn->value, apn->length) != 0) {
                continue;
            }

            /* PDN entry found */
            break;
        }
    }

    /* Return the identifier of the PDN connection */
    return (esm_data->pdn[i].pid);
}

/****************************************************************************
 **                                                                        **
 ** Name:    _pdn_connectivity_find_pdn()                              **
 **                                                                        **
 ** Description: Search the list of PDN connections for an entry defined   **
 **      for the specified APN with the same PDN type              **
 **                                                                        **
 ** Inputs:  apn:       Access Point Name of the PDN connection    **
 **      pdn_type:  PDN address type                           **
 **                                                                        **
 ** Outputs:     None                                                      **
 **      Return:    The identifier of the PDN connection if    **
 **             found in the list; -1 otherwise.           **
 **      Others:    None                                       **
 **                                                                        **
 ***************************************************************************/
static int _pdn_connectivity_find_pdn(esm_data_t *esm_data, const OctetString *apn,
                                      const esm_proc_pdn_type_t pdn_type)
{
    int i;

    for (i = 0; i < ESM_DATA_PDN_MAX; i++) {
        if ( (esm_data->pdn[i].pid != -1) && esm_data->pdn[i].data ) {
            /* PDN connection established during initial network attachment */
            if (esm_data->pdn[i].data->apn.length == 0) {
                break;
            }

            /* Subsequent PDN connection established for the specified APN */
            if (esm_data->pdn[i].data->apn.length != apn->length) {
                continue;
            }

            if (memcmp(esm_data->pdn[i].data->apn.value,
                       apn->value, apn->length) != 0) {
                continue;
            }

            if (esm_data->pdn[i].data->type == ESM_PDN_TYPE_IPV4V6) {
                break;
            }

            if (esm_data->pdn[i].data->type == pdn_type) {
                break;
            }
        }
    }

    /* Return the identifier of the PDN connection */
    return (esm_data->pdn[i].pid);
}
