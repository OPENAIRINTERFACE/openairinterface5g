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

#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>

#include "openair2/RRC/NR/rrc_gNB_UE_context.h"

#define TELNETSERVERCODE
#include "telnetsrv.h"

#define ERROR_MSG_RET(mSG, aRGS...) do { prnt(mSG, ##aRGS); return 1; } while (0)

/**
 * Module brief:
 * This module is used to add RRCRelease commands to the telnet server in the
 * absence of full support for E2SM RAN Control (RC).
 * This provides similar functionality to the ORAN.WG3.E2SM-RC-R003-v05.00 
 * 8.4.5.4 RRC Connection Release Control which is initiated by the RIC. 
 * 
 * Implementation notes:
 * We refer to the method call rrc_gNB_generate_RRCRelease at rrc_gNB_NGAP.c 
 * during rrc_gNB_process_NGAP_UE_CONTEXT_RELEASE_COMMAND message generation. 
 * 
 * Building the telnetsrv and module:
 * ./build_oai --build-lib telnetsrv
 * 
 * Loading the module:
 * sudo ./nr-softmodem --sa -E --rfsim --log_config.global_log_options level,nocolor,time -O ~/gnb.sa.band78.106prb.rfsim.conf --telnetsrv --telnetsrv.shrmod rrc
*/

static int get_single_ue_id(void)
{
  rrc_gNB_ue_context_t *ue_context_p = NULL;
  RB_FOREACH(ue_context_p, rrc_nr_ue_tree_s, &(RC.nrrrc[0]->rrc_ue_head)) {
    return ue_context_p->ue_context.rrc_ue_id;
  }
  return -1;
}

/**
 * @brief Trigger RRC Release for a specific UE
 * @param buf: RRC UE ID
 * @param debug: Debug flag
 * @param prnt: Print function
 * @return 0 on success, -1 on failure
*/
int rrc_gNB_trigger_release(char *buf, int debug, telnet_printfunc_t prnt) {
  ue_id_t ue_id = -1;
  protocol_ctxt_t ctxt;

  if (!buf) {
    ue_id = get_single_ue_id();
    if (ue_id < 1) {
      prnt("No UE found!\n");
      ERROR_MSG_RET("No UE found!\n");
    }
  } else {
    ue_id = strtol(buf, NULL, 10);
    if (ue_id < 1 || ue_id >= 0xfffffe) {
      prnt("UE ID needs to be [1,0xfffffe]\n");
      ERROR_MSG_RET("UE ID needs to be [1,0xfffffe]\n");
    }
  }

  /* get RRC and UE */
  gNB_RRC_INST *rrc = RC.nrrrc[0];
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(rrc, ue_id);
  if (!ue_context_p) {
    prnt("Could not find UE context associated with UE ID %lu\n", ue_id);
    LOG_E(RRC, "Could not find UE context associated with UE ID %lu\n", ue_id);
    return -1;
  }
  
  gNB_RRC_UE_t *UE = &ue_context_p->ue_context;
  PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, 0, GNB_FLAG_YES, UE->rrc_ue_id, 0, 0);
  ctxt.eNB_index = 0;

  rrc_gNB_generate_RRCRelease(&ctxt, ue_context_p);
  prnt("RRC Release triggered for UE %u\n", ue_id);
  
  return 0;
}

/**
 * @brief Trigger RRC Release for all UEs
*/
int rrc_gNB_trigger_release_all(char *buf, int debug, telnet_printfunc_t prnt) {
  rrc_gNB_ue_context_t *ue_context_p = NULL;
  protocol_ctxt_t ctxt; /* Not sure what exactly is this */

  RB_FOREACH(ue_context_p, rrc_nr_ue_tree_s, &(RC.nrrrc[0]->rrc_ue_head)) {
    gNB_RRC_UE_t *UE = &ue_context_p->ue_context;
    PROTOCOL_CTXT_SET_BY_INSTANCE(&ctxt, 0, GNB_FLAG_YES, UE->rrc_ue_id, 0, 0);
    ctxt.eNB_index = 0;

    rrc_gNB_generate_RRCRelease(&ctxt, ue_context_p);
    prnt("RRC Release triggered for UE %u\n", UE->rrc_ue_id);
  }

  return 0;
}

static telnetshell_cmddef_t rrc_cmds[] = {
  {"release_rrc", "[rrc_ue_id(int,opt)]", rrc_gNB_trigger_release},
  {"release_rrc_all", "", rrc_gNB_trigger_release_all},
  {"", "", NULL},
};

static telnetshell_vardef_t rrc_vars[] = {
  {"", 0, 0, NULL}
};

void add_rrc_cmds(void) {
  add_telnetcmd("rrc", rrc_vars, rrc_cmds);
}
