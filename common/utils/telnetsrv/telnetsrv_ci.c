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
#include "openair2/LAYER2/NR_MAC_gNB/nr_mac_gNB.h"
#include "openair2/LAYER2/nr_rlc/nr_rlc_oai_api.h"
#include "openair2/LAYER2/nr_rlc/nr_rlc_ue_manager.h"
#include "openair2/LAYER2/nr_rlc/nr_rlc_entity_am.h"

#define TELNETSERVERCODE
#include "telnetsrv.h"

#define ERROR_MSG_RET(mSG, aRGS...) do { prnt(mSG, ##aRGS); return 1; } while (0)

static int get_single_ue_rnti_mac(void)
{
  NR_UE_info_t *ue = NULL;
  UE_iterator(RC.nrmac[0]->UE_info.list, it) {
    if (it && ue)
      return -1;
    if (it)
      ue = it;
  }
  if (!ue)
    return -1;

  return ue->rnti;
}

int get_single_rnti(char *buf, int debug, telnet_printfunc_t prnt)
{
  if (buf)
    ERROR_MSG_RET("no parameter allowed\n");

  int rnti = get_single_ue_rnti_mac();
  if (rnti < 1)
    ERROR_MSG_RET("different number of UEs\n");

  prnt("single UE RNTI %04x\n", rnti);
  return 0;
}

int get_reestab_count(char *buf, int debug, telnet_printfunc_t prnt)
{
  if (!RC.nrrrc)
    ERROR_MSG_RET("no RRC present, cannot list counts\n");
  rrc_gNB_ue_context_t *ue = NULL;
  int rnti = -1;
  if (!buf) {
    rrc_gNB_ue_context_t *l = NULL;
    int n = 0;
    RB_FOREACH(l, rrc_nr_ue_tree_s, &RC.nrrrc[0]->rrc_ue_head) {
      if (ue == NULL) ue = l;
      n++;
    }
    if (!ue)
      ERROR_MSG_RET("could not find any UE in RRC\n");
    if (n > 1)
      ERROR_MSG_RET("more than one UE in RRC present\n");
  } else {
    rnti = strtol(buf, NULL, 16);
    if (rnti < 1 || rnti >= 0xfffe)
      ERROR_MSG_RET("RNTI needs to be [1,0xfffe]\n");
    ue = rrc_gNB_get_ue_context_by_rnti_any_du(RC.nrrrc[0], rnti);
    if (!ue)
      ERROR_MSG_RET("could not find UE with RNTI %04x in RRC\n");
  }

  prnt("UE RNTI %04x reestab %d reconfig %d\n",
       ue->ue_context.rnti,
       ue->ue_context.ue_reestablishment_counter,
       ue->ue_context.ue_reconfiguration_counter);
  return 0;
}

int trigger_reestab(char *buf, int debug, telnet_printfunc_t prnt)
{
  if (!RC.nrmac)
    ERROR_MSG_RET("no MAC/RLC present, cannot trigger reestablishment\n");
  int rnti = -1;
  if (!buf) {
    rnti = get_single_ue_rnti_mac();
    if (rnti < 1)
      ERROR_MSG_RET("no UE found\n");
  } else {
    rnti = strtol(buf, NULL, 16);
    if (rnti < 1 || rnti >= 0xfffe)
      ERROR_MSG_RET("RNTI needs to be [1,0xfffe]\n");
  }

  nr_rlc_test_trigger_reestablishment(rnti);

  prnt("Reset RLC counters of UE RNTI %04x to trigger reestablishment\n", rnti);
  return 0;
}

static telnetshell_cmddef_t cicmds[] = {
  {"get_single_rnti", "", get_single_rnti},
  {"force_reestab", "[rnti(hex,opt)]", trigger_reestab},
  {"get_reestab_count", "[rnti(hex,opt)]", get_reestab_count},
  {"", "", NULL},
};

static telnetshell_vardef_t civars[] = {

  {"", 0, 0, NULL}
};

void add_ci_cmds(void) {
  add_telnetcmd("ci", civars, cicmds);
}
