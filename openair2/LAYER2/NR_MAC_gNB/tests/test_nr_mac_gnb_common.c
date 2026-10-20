/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/* globals and stubs needed to link the gNB MAC into a test */

#include <signal.h>
#include <stdlib.h>

#include "common/config/config_load_configmodule.h"
#include "common/ran_context.h"
#include "common/utils/LOG/log.h"
#include "e1ap_messages_types.h"
#include "executables/softmodem-common.h"
#include "PHY/defs_common.h"

RAN_CONTEXT_t RC;
int64_t uplink_frequency_offset[MAX_NUM_CCs][4];
uint64_t downlink_frequency[MAX_NUM_CCs][4];
instance_t DUuniqInstance = 0;
instance_t CUuniqInstance = 0;
unsigned int NTN_UE_Koffset = 0;
configmodule_interface_t *uniqCfg = NULL;
char *uecap_file;
THREAD_STRUCT thread_struct;

void e1_bearer_context_setup(const e1ap_bearer_setup_req_t *req) { (void)req; abort(); }
void e1_bearer_context_modif(const e1ap_bearer_mod_req_t *req) { (void)req; abort(); }
struct e1ap_bearer_mod_confirm_s;
void e1_bearer_context_mod_confirm(const struct e1ap_bearer_mod_confirm_s *conf) { (void)conf; abort(); }
struct e1ap_bearer_release_cmd_s;
void e1_bearer_release_cmd(const struct e1ap_bearer_release_cmd_s *cmd) { (void)cmd; abort(); }

// referenced by gnb_config.c, avoids linking the PHY
int l1_north_init_gNB(void) { abort(); }

// called from main() in test_nr_mac_gnb.cpp
void nr_mac_gnb_test_init(void)
{
  logInit();
  set_glog(OAILOG_ERR);
  get_softmodem_params()->phy_test = 0;
  get_softmodem_params()->do_ra = 0;
  get_softmodem_params()->nsa = 0;
}
