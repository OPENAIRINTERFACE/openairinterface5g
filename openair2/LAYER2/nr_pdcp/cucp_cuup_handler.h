/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef CUCP_CUUP_HANDLER_H
#define CUCP_CUUP_HANDLER_H

#include <stdbool.h>

void nr_pdcp_e1_if_init(bool uses_e1);

struct e1ap_bearer_setup_req_s;
struct e1ap_bearer_mod_req_s;
struct e1ap_bearer_mod_confirm_s;
struct e1ap_bearer_release_cmd_s;
void e1_bearer_context_setup(const struct e1ap_bearer_setup_req_s *req);
void e1_bearer_context_modif(const struct e1ap_bearer_mod_req_s *req);
void e1_bearer_context_mod_confirm(const struct e1ap_bearer_mod_confirm_s *conf);
void e1_bearer_release_cmd(const struct e1ap_bearer_release_cmd_s *cmd);
void e1_reset(void);

#endif /* CUCP_CUUP_HANDLER_H */
