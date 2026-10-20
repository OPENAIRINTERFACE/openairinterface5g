/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "common/oai_version.h"
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
#include "LAYER2/nr_pdcp/nr_pdcp_oai_api.h"
#include "common/utils/time_manager/time_manager.h"

RAN_CONTEXT_t RC;
THREAD_STRUCT thread_struct;
uint64_t downlink_frequency[MAX_NUM_CCs][4];
int64_t uplink_frequency_offset[MAX_NUM_CCs][4];
int oai_exit = 0;

#ifdef E2_AGENT
#include "openair2/E2AP/flexric/src/agent/e2_agent_api.h"
#include "openair2/E2AP/RAN_FUNCTION/init_ran_func.h"

static void initialize_agent(ngran_node_t node_type, e2_agent_args_t oai_args)
{
  AssertFatal(oai_args.sm_dir != NULL , "Please, specify the directory where the SMs are located in the config file, i.e., add in config file the next line: e2_agent = {near_ric_ip_addr = \"127.0.0.1\"; sm_dir = \"/usr/local/lib/flexric/\");} ");
  AssertFatal(oai_args.ip != NULL , "Please, specify the IP address of the nearRT-RIC in the config file, i.e., e2_agent = {near_ric_ip_addr = \"127.0.0.1\"; sm_dir = \"/usr/local/lib/flexric/\"");

  printf("After RCconfig_NR_E2agent %s %s \n",oai_args.sm_dir, oai_args.ip  );

  fr_args_t args = {0};
  memcpy(args.ip, oai_args.ip, FR_IP_ADDRESS_LEN);
  memcpy(args.libs_dir, oai_args.sm_dir, FR_CONF_FILE_LEN);

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

rlc_op_status_t nr_rlc_data_req(const protocol_ctxt_t *const ctxt_pP,
                                const srb_flag_t srb_flagP,
                                const rb_id_t rb_idP,
                                const mui_t muiP,
                                sdu_size_t sdu_sizeP,
                                uint8_t *sdu_pP)
{
  abort();
  return 0;
}

int nr_rlc_get_available_tx_space(const rnti_t rntiP, const logical_chan_id_t channel_idP)
{
  abort();
  return 0;
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

  // start time manager with some reasonable default for the running mode
  // (may be overwritten in configuration file or command line)
  void nr_pdcp_ms_tick(void);
  time_manager_tick_function_t tick_functions[] = {
    nr_pdcp_ms_tick
  };
  int tick_functions_count = 1;
  time_manager_start(tick_functions, tick_functions_count, TIME_SOURCE_REALTIME);

  // strdup to put the sring in the core file for post mortem identification
  char *v = strdup(OAI_PACKAGE_VERSION);
  LOG_I(HW, "Version: %s\n", v);
  set_softmodem_sighandler();
  itti_init(TASK_MAX, tasks_info);
  int rc;
  rc = itti_create_task(TASK_SCTP, sctp_eNB_task, NULL);
  AssertFatal(rc >= 0, "Create task for SCTP failed\n");
  rc = itti_create_task(TASK_GTPV1_U, gtpv1uTask, NULL);
  AssertFatal(rc >= 0, "Create task for GTPV1U failed\n");
  rc = itti_create_task(TASK_CUUP_E1, E1AP_CUUP_task, NULL);
  AssertFatal(rc >= 0, "Create task for CUUP E1 failed\n");
  nr_pdcp_layer_init();
  cu_init_f1_ue_data(); // for CU-UP/CP mapping: we use the same
  E1_t e1type = UPtype;
  MessageDef *msg = RCconfig_NR_CU_E1(&e1type);
  AssertFatal(msg != NULL, "Send init to task for E1AP UP failed\n");
  itti_send_msg_to_task(TASK_CUUP_E1, 0, msg);
  LOG_D(E1AP, "Send E1AP REGISTER REQ to TASK_CUUP_E1\n");

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

  time_manager_finish();

  free(v);

  logClean();
  printf("Bye.\n");
  return 0;
}
