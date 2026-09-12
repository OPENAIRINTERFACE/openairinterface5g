/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef __E1AP_H_
#define __E1AP_H_

#include "openair2/COMMON/e1ap_messages_types.h"
#include "e1ap_asnc.h"
#include "openair2/E1AP/e1ap_common.h"

int e1apCUCP_handle_SETUP_REQUEST(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *inst, const E1AP_E1AP_PDU_t *pdu);

int e1apCUUP_handle_SETUP_RESPONSE(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *inst, const E1AP_E1AP_PDU_t *pdu);

int e1apCUUP_handle_SETUP_FAILURE(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *inst, const E1AP_E1AP_PDU_t *pdu);

int e1apCUUP_handle_BEARER_CONTEXT_SETUP_REQUEST(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *inst, const E1AP_E1AP_PDU_t *pdu);

int e1apCUCP_handle_BEARER_CONTEXT_SETUP_RESPONSE(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *inst, const E1AP_E1AP_PDU_t *pdu);

int e1apCUCP_handle_BEARER_CONTEXT_SETUP_FAILURE(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *inst, const E1AP_E1AP_PDU_t *pdu);

int e1apCUUP_handle_BEARER_CONTEXT_MODIFICATION_REQUEST(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *inst, const E1AP_E1AP_PDU_t *pdu);

int e1apCUCP_handle_BEARER_CONTEXT_MODIFICATION_RESPONSE(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *e1_inst, const E1AP_E1AP_PDU_t *pdu);

int e1apCUCP_handle_BEARER_CONTEXT_MODIFICATION_FAILURE(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *inst, const E1AP_E1AP_PDU_t *pdu);

int e1apCUCP_handle_BEARER_CONTEXT_MODIFICATION_REQUIRED(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *inst, const E1AP_E1AP_PDU_t *pdu);

int e1apCUUP_handle_BEARER_CONTEXT_MODIFICATION_CONFIRM(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *inst, const E1AP_E1AP_PDU_t *pdu);

void e1apCUUP_send_BEARER_CONTEXT_SETUP_RESPONSE(sctp_assoc_t assoc_id, const e1ap_bearer_setup_resp_t *resp);

int e1apCUUP_handle_BEARER_CONTEXT_RELEASE_COMMAND(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *inst, const E1AP_E1AP_PDU_t *pdu);

int e1apCUCP_handle_BEARER_CONTEXT_RELEASE_COMPLETE(sctp_assoc_t assoc_id, e1ap_upcp_inst_t *inst, const E1AP_E1AP_PDU_t *pdu);

int e1apCUUP_send_BEARER_CONTEXT_RELEASE_COMPLETE(sctp_assoc_t assoc_id, const e1ap_bearer_release_cplt_t *cplt);

void *E1AP_CUUP_task(void *arg);

void *E1AP_CUCP_task(void *arg);
#endif
