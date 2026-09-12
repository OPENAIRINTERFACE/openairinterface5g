/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef E1AP_BEARER_CONTEXT_MANAGEMENT_H_
#define E1AP_BEARER_CONTEXT_MANAGEMENT_H_

#include <stdbool.h>
#include "openair2/COMMON/e1ap_messages_types.h"
#include "common/platform_types.h"

struct E1AP_E1AP_PDU;

struct E1AP_E1AP_PDU *encode_E1_bearer_context_setup_request(const e1ap_bearer_setup_req_t *msg);
bool decode_E1_bearer_context_setup_request(const struct E1AP_E1AP_PDU *pdu, e1ap_bearer_setup_req_t *out);
e1ap_bearer_setup_req_t cp_bearer_context_setup_request(const e1ap_bearer_setup_req_t *msg);
void free_e1ap_context_setup_request(e1ap_bearer_setup_req_t *msg);
bool eq_bearer_context_setup_request(const e1ap_bearer_setup_req_t *a, const e1ap_bearer_setup_req_t *b);

struct E1AP_E1AP_PDU *encode_E1_bearer_context_setup_response(const e1ap_bearer_setup_resp_t *msg);
bool decode_E1_bearer_context_setup_response(const struct E1AP_E1AP_PDU *pdu, e1ap_bearer_setup_resp_t *out);
void free_e1ap_context_setup_response(const e1ap_bearer_setup_resp_t *msg);
e1ap_bearer_setup_resp_t cp_bearer_context_setup_response(const e1ap_bearer_setup_resp_t *msg);
bool eq_bearer_context_setup_response(const e1ap_bearer_setup_resp_t *a, const e1ap_bearer_setup_resp_t *b);

struct E1AP_E1AP_PDU *encode_E1_bearer_context_setup_failure(const e1ap_bearer_context_setup_failure_t *msg);
bool decode_E1_bearer_context_setup_failure(e1ap_bearer_context_setup_failure_t *out, const struct E1AP_E1AP_PDU *pdu);
e1ap_bearer_context_setup_failure_t cp_bearer_context_setup_failure(const e1ap_bearer_context_setup_failure_t *msg);
bool eq_bearer_context_setup_failure(const e1ap_bearer_context_setup_failure_t *a, const e1ap_bearer_context_setup_failure_t *b);
void free_e1_bearer_context_setup_failure(const e1ap_bearer_context_setup_failure_t *msg);

struct E1AP_E1AP_PDU *encode_e1_bearer_context_release_command(const e1ap_bearer_release_cmd_t *msg);
bool decode_e1_bearer_context_release_command(e1ap_bearer_release_cmd_t *out, const struct E1AP_E1AP_PDU *pdu);
bool eq_bearer_context_release_command(const e1ap_bearer_release_cmd_t *a, const e1ap_bearer_release_cmd_t *b);
e1ap_bearer_release_cmd_t cp_bearer_context_release_command(const e1ap_bearer_release_cmd_t *msg);
void free_e1_bearer_context_release_command(const e1ap_bearer_release_cmd_t *msg);

struct E1AP_E1AP_PDU *encode_e1_bearer_context_release_complete(const e1ap_bearer_release_cplt_t *cplt);
bool decode_e1_bearer_context_release_complete(e1ap_bearer_release_cplt_t *out, const struct E1AP_E1AP_PDU *pdu);
bool eq_bearer_context_release_complete(const e1ap_bearer_release_cplt_t *a, const e1ap_bearer_release_cplt_t *b);
e1ap_bearer_release_cplt_t cp_bearer_context_release_complete(const e1ap_bearer_release_cplt_t *src);
void free_e1_bearer_context_release_complete(const e1ap_bearer_release_cplt_t *msg);

struct E1AP_E1AP_PDU *encode_E1_bearer_context_mod_request(const e1ap_bearer_mod_req_t *msg);
bool decode_E1_bearer_context_mod_request(const struct E1AP_E1AP_PDU *pdu, e1ap_bearer_mod_req_t *out);
void free_e1ap_context_mod_request(const e1ap_bearer_mod_req_t *msg);
e1ap_bearer_mod_req_t cp_bearer_context_mod_request(const e1ap_bearer_mod_req_t *msg);
bool eq_bearer_context_mod_request(const e1ap_bearer_mod_req_t *a, const e1ap_bearer_mod_req_t *b);

struct E1AP_E1AP_PDU *encode_E1_bearer_context_mod_response(const e1ap_bearer_modif_resp_t *msg);
bool decode_E1_bearer_context_mod_response(e1ap_bearer_modif_resp_t *out, const struct E1AP_E1AP_PDU *pdu);
void free_e1ap_context_mod_response(const e1ap_bearer_modif_resp_t *msg);
e1ap_bearer_modif_resp_t cp_bearer_context_mod_response(const e1ap_bearer_modif_resp_t *msg);
bool eq_bearer_context_mod_response(const e1ap_bearer_modif_resp_t *a, const e1ap_bearer_modif_resp_t *b);

struct E1AP_E1AP_PDU *encode_E1_bearer_context_mod_failure(const e1ap_bearer_context_mod_failure_t *msg);
bool decode_E1_bearer_context_mod_failure(e1ap_bearer_context_mod_failure_t *out, const struct E1AP_E1AP_PDU *pdu);
void free_E1_bearer_context_mod_failure(const e1ap_bearer_context_mod_failure_t *msg);
e1ap_bearer_context_mod_failure_t cp_E1_bearer_context_mod_failure(const e1ap_bearer_context_mod_failure_t *msg);
bool eq_E1_bearer_context_mod_failure(const e1ap_bearer_context_mod_failure_t *a, const e1ap_bearer_context_mod_failure_t *b);

struct E1AP_E1AP_PDU *encode_E1_bearer_context_mod_required(const e1ap_bearer_mod_required_t *msg);
bool decode_E1_bearer_context_mod_required(const struct E1AP_E1AP_PDU *pdu, e1ap_bearer_mod_required_t *out);
void free_e1ap_context_mod_required(const e1ap_bearer_mod_required_t *msg);
e1ap_bearer_mod_required_t cp_bearer_context_mod_required(const e1ap_bearer_mod_required_t *msg);
bool eq_bearer_context_mod_required(const e1ap_bearer_mod_required_t *a, const e1ap_bearer_mod_required_t *b);

struct E1AP_E1AP_PDU *encode_E1_bearer_context_mod_confirm(const e1ap_bearer_mod_confirm_t *msg);
bool decode_E1_bearer_context_mod_confirm(e1ap_bearer_mod_confirm_t *out, const struct E1AP_E1AP_PDU *pdu);
bool eq_bearer_context_mod_confirm(const e1ap_bearer_mod_confirm_t *a, const e1ap_bearer_mod_confirm_t *b);
e1ap_bearer_mod_confirm_t cp_bearer_context_mod_confirm(const e1ap_bearer_mod_confirm_t *msg);
void free_e1ap_context_mod_confirm(const e1ap_bearer_mod_confirm_t *msg);

#endif /* E1AP_BEARER_CONTEXT_MANAGEMENT_H_ */
