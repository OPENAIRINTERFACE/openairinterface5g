/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include <netinet/in.h>
#include <netinet/sctp.h>
#include "assertions.h"
#include "cucp_cuup_if.h"
#include "nr_rrc_defs.h"
#include "openair2/COMMON/e1ap_messages_types.h"
#include "openair2/LAYER2/nr_pdcp/cucp_cuup_handler.h"

static void cucp_cuup_bearer_context_setup_direct(sctp_assoc_t assoc_id, const e1ap_bearer_setup_req_t *req)
{
  AssertFatal(assoc_id == -1, "illegal assoc_id %d, impossible for integrated CU\n", assoc_id);
  e1_bearer_context_setup(req);
}

static void cucp_cuup_bearer_context_modif_direct(sctp_assoc_t assoc_id, const e1ap_bearer_mod_req_t *req)
{
  AssertFatal(assoc_id == -1, "illegal assoc_id %d, impossible for integrated CU\n", assoc_id);
  e1_bearer_context_modif(req);
}

static void cucp_cuup_bearer_context_mod_confirm_direct(sctp_assoc_t assoc_id, const e1ap_bearer_mod_confirm_t *conf)
{
  AssertFatal(assoc_id == -1, "illegal assoc_id %d, impossible for integrated CU\n", assoc_id);
  e1_bearer_context_mod_confirm(conf);
}

static void cucp_cuup_bearer_context_release_cmd_direct(sctp_assoc_t assoc_id, const e1ap_bearer_release_cmd_t *cmd)
{
  AssertFatal(assoc_id == -1, "illegal assoc_id %d, impossible for integrated CU\n", assoc_id);
  e1_bearer_release_cmd(cmd);
}

void cucp_cuup_message_transfer_direct_init(gNB_RRC_INST *rrc) {
  rrc->cucp_cuup.bearer_context_setup = cucp_cuup_bearer_context_setup_direct;
  rrc->cucp_cuup.bearer_context_mod = cucp_cuup_bearer_context_modif_direct;
  rrc->cucp_cuup.bearer_context_mod_confirm = cucp_cuup_bearer_context_mod_confirm_direct;
  rrc->cucp_cuup.bearer_context_release = cucp_cuup_bearer_context_release_cmd_direct;
}
