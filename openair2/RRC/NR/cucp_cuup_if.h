/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef CUCP_CUUP_IF_H
#define CUCP_CUUP_IF_H

#include <netinet/in.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/sctp.h>
#include "common/platform_types.h"

struct e1ap_bearer_setup_req_s;
struct e1ap_bearer_mod_req_s;
struct e1ap_bearer_mod_confirm_s;
struct e1ap_bearer_setup_resp_s;
struct e1ap_bearer_release_cmd_s;
typedef void (*cucp_cuup_bearer_context_setup_func_t)(sctp_assoc_t assoc_id, const struct e1ap_bearer_setup_req_s *req);
typedef void (*cucp_cuup_bearer_context_mod_func_t)(sctp_assoc_t assoc_id, const struct e1ap_bearer_mod_req_s *req);
typedef void (*cucp_cuup_bearer_context_mod_confirm_func_t)(sctp_assoc_t assoc_id, const struct e1ap_bearer_mod_confirm_s *conf);
typedef void (*cucp_cuup_bearer_context_release_func_t)(sctp_assoc_t assoc_id, const struct e1ap_bearer_release_cmd_s *cmd);

struct gNB_RRC_INST_s;
void cucp_cuup_message_transfer_direct_init(struct gNB_RRC_INST_s *rrc);
void cucp_cuup_message_transfer_e1ap_init(struct gNB_RRC_INST_s *rrc);

#endif
