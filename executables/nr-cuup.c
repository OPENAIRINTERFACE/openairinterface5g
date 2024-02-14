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

#include "common/utils/simple_executable.h"
#include "executables/softmodem-common.h"
#include "common/utils/ocp_itti/intertask_interface.h"
#include "openair3/ocp-gtpu/gtp_itf.h"
#include "openair2/E1AP/e1ap.h"
#include "common/ran_context.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "openair2/F1AP/f1ap_common.h"
#include "openair2/F1AP/f1ap_ids.h"
#include "openair2/GNB_APP/gnb_config.h"
#include "nr_pdcp/nr_pdcp_oai_api.h"

RAN_CONTEXT_t RC;
THREAD_STRUCT thread_struct;
uint64_t downlink_frequency[MAX_NUM_CCs][4];
int32_t uplink_frequency_offset[MAX_NUM_CCs][4];
int asn1_xer_print;
int oai_exit = 0;
instance_t CUuniqInstance = 0;

#ifdef E2_AGENT
#include "openair2/E2AP/flexric/src/agent/e2_agent_api.h"
#include "openair2/E2AP/RAN_FUNCTION/init_ran_func.h"

static void initialize_agent(ngran_node_t node_type, e2_agent_args_t oai_args)
{
  AssertFatal(oai_args.sm_dir != NULL , "Please, specify the directory where the SMs are located in the config file, i.e., add in config file the next line: e2_agent = {near_ric_ip_addr = \"127.0.0.1\"; sm_dir = \"/usr/local/lib/flexric/\");} ");
  AssertFatal(oai_args.ip != NULL , "Please, specify the IP address of the nearRT-RIC in the config file, i.e., e2_agent = {near_ric_ip_addr = \"127.0.0.1\"; sm_dir = \"/usr/local/lib/flexric/\"");

  printf("After RCconfig_NR_E2agent %s %s \n",oai_args.sm_dir, oai_args.ip  );

  fr_args_t args = { .ip = oai_args.ip }; // init_fr_args(0, NULL);
  memcpy(args.libs_dir, oai_args.sm_dir, 128);

  sleep(1);

  // Only 1 instances is supported in one executable
  // Advice: run multiple executables to have multiple instances
  const instance_t e1_inst = 0;
  const e1ap_upcp_inst_t *e1inst = getCxtE1(e1_inst);

  const int nb_id = e1inst->gnb_id;
  const int cu_up_id = e1inst->cuup.setupReq.gNB_cu_up_id;
  const int mcc = e1inst->cuup.setupReq.plmn[0].id.mcc;
  const int mnc = e1inst->cuup.setupReq.plmn[0].id.mnc;
  const int mnc_digit_len = e1inst->cuup.setupReq.plmn[0].id.mnc_digit_length;

  printf("[E2 NODE]: mcc = %d mnc = %d mnc_digit = %d nb_id = %d \n", mcc, mnc, mnc_digit_len, nb_id);

  printf("[E2 NODE]: Args %s %s \n", args.ip, args.libs_dir);

  sm_io_ag_ran_t io = init_ran_func_ag();
  init_agent_api(mcc, mnc, mnc_digit_len, nb_id, cu_up_id, node_type, io, &args);
}
#endif  // E2_AGENT


void exit_function(const char *file, const char *function, const int line, const char *s, const int assert)
{
  if (assert) {
    abort();
  } else {
    sleep(1); // allow other threads to exit first
    exit(EXIT_SUCCESS);
  }
}

nfapi_mode_t nfapi_mod = -1;
void nfapi_setmode(nfapi_mode_t nfapi_mode)
{
  nfapi_mod = nfapi_mode;
}
nfapi_mode_t nfapi_getmode(void)
{
  return nfapi_mod;
}

ngran_node_t get_node_type()
{
  return ngran_gNB_CUUP;
}

rlc_op_status_t rlc_data_req(const protocol_ctxt_t *const pc,
                             const srb_flag_t sf,
                             const MBMS_flag_t mf,
                             const rb_id_t rb_id,
                             const mui_t mui,
                             const confirm_t c,
                             const sdu_size_t size,
                             uint8_t *const buf,
                             const uint32_t *const a,
                             const uint32_t *const b)
{
  abort();
  return 0;
}

int nr_rlc_get_available_tx_space(const rnti_t rntiP, const logical_chan_id_t channel_idP)
{
  abort();
  return 0;
}

void rrc_gNB_generate_dedicatedRRCReconfiguration(const protocol_ctxt_t *const ctxt_pP, rrc_gNB_ue_context_t *ue_context_pP)
{
  abort();
}

void nr_rlc_add_drb(int rnti, int drb_id, const NR_RLC_BearerConfig_t *rlc_BearerConfig)
{
  abort();
}

void prepare_and_send_ue_context_modification_f1(rrc_gNB_ue_context_t *ue_context_p, e1ap_bearer_setup_resp_t *e1ap_resp)
{
  abort();
}

f1ap_cudu_inst_t *getCxt(instance_t instanceP)
{
  // the E1 module uses F1's getCxt() to decide whether there is F1-U and if
  // so, what is the GTP instance. In the CU-UP, we don't start the F1 module,
  // and instead, E1 handles the GTP-U endpoint. In the following, we fake the
  // instance and put the right GTP-U instance number in.
  const e1ap_upcp_inst_t *e1inst = getCxtE1(instanceP);
  static f1ap_cudu_inst_t fake = {0};
  fake.gtpInst = e1inst->gtpInstF1U;
  return &fake;
}
configmodule_interface_t *uniqCfg = NULL;
int main(int argc, char **argv)
{
  /// static configuration for NR at the moment
  if ((uniqCfg = load_configmodule(argc, argv, CONFIG_ENABLECMDLINEONLY)) == NULL) {
    exit_fun("[SOFTMODEM] Error, configuration module init failed\n");
  }
  logInit();
#ifndef PACKAGE_VERSION
#  define PACKAGE_VERSION "UNKNOWN-EXPERIMENTAL"
#endif
  LOG_I(HW, "Version: %s\n", PACKAGE_VERSION);
  set_softmodem_sighandler();
  itti_init(TASK_MAX, tasks_info);
  int rc;
  rc = itti_create_task(TASK_SCTP, sctp_eNB_task, NULL);
  AssertFatal(rc >= 0, "Create task for SCTP failed\n");
  rc = itti_create_task(TASK_GTPV1_U, gtpv1uTask, NULL);
  AssertFatal(rc >= 0, "Create task for GTPV1U failed\n");
  rc = itti_create_task(TASK_CUUP_E1, E1AP_CUUP_task, NULL);
  AssertFatal(rc >= 0, "Create task for CUUP E1 failed\n");
  nr_pdcp_layer_init(true);
  cu_init_f1_ue_data(); // for CU-UP/CP mapping: we use the same
  E1_t e1type = UPtype;
  MessageDef *msg = RCconfig_NR_CU_E1(&e1type);
  AssertFatal(msg != NULL, "Send init to task for E1AP UP failed\n");
  itti_send_msg_to_task(TASK_CUUP_E1, 0, msg);

  #ifdef E2_AGENT
  //////////////////////////////////
  //////////////////////////////////
  //// Init the E2 Agent

  // OAI Wrapper 
  e2_agent_args_t oai_args = RCconfig_NR_E2agent();

  if (oai_args.enabled) {
    const ngran_node_t node_type = get_node_type();
    assert(node_type == ngran_gNB_CUUP);
    initialize_agent(node_type, oai_args);
  }

  #endif // E2_AGENT

  printf("TYPE <CTRL-C> TO TERMINATE\n");
  itti_wait_tasks_end(NULL);

  logClean();
  printf("Bye.\n");
  return 0;
}
