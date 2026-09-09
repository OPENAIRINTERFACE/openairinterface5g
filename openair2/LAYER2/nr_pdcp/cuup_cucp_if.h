/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef CUUP_CUCP_IF_H
#define CUUP_CUCP_IF_H

#include <stdbool.h>

struct e1ap_bearer_setup_resp_s;
struct e1ap_bearer_modif_resp_s;
struct e1ap_bearer_mod_required_s;
struct e1ap_bearer_release_cplt_s;
typedef void (*e1_bearer_setup_response_func_t)(const struct e1ap_bearer_setup_resp_s *resp);
typedef void (*e1_bearer_modif_response_func_t)(const struct e1ap_bearer_modif_resp_s *resp);
typedef void (*e1_bearer_mod_required_func_t)(const struct e1ap_bearer_mod_required_s *req);
typedef void (*e1_bearer_release_complete_func_t)(const struct e1ap_bearer_release_cplt_s *cplt);

typedef struct e1_if_t {
  e1_bearer_setup_response_func_t bearer_setup_response;
  e1_bearer_modif_response_func_t bearer_modif_response;
  e1_bearer_mod_required_func_t bearer_mod_required;
  e1_bearer_release_complete_func_t bearer_release_complete;
} e1_if_t;

e1_if_t *get_e1_if(void);
bool e1_used(void);
void nr_pdcp_e1_if_init(bool uses_e1);

void cuup_cucp_init_direct(e1_if_t *iface);
void cuup_cucp_init_e1ap(e1_if_t *iface);

#endif /* CUUP_CUCP_IF_H */
