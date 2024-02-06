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

static int get_single_ue_rnti(void)
{
  rrc_gNB_ue_context_t *ue_context_p = NULL;
  RB_FOREACH(ue_context_p, rrc_nr_ue_tree_s, &(RC.nrrrc[0]->rrc_ue_head)) {
    return ue_context_p->ue_context.rnti;
  }
  return -1;
}

int get_single_rnti(char *buf, int debug, telnet_printfunc_t prnt)
{
  if (buf)
    ERROR_MSG_RET("no parameter allowed\n");

  int rnti = get_single_ue_rnti();
  if (rnti < 1)
    ERROR_MSG_RET("different number of UEs\n");

  prnt("single UE RNTI %04x\n", rnti);
  return 0;
}

//void rrc_gNB_trigger_new_bearer(int rnti);
int add_bearer(char *buf, int debug, telnet_printfunc_t prnt)
{
  int rnti = -1;
  if (!buf) {
    rnti = get_single_ue_rnti();
    if (rnti < 1)
      ERROR_MSG_RET("no UE found\n");
  } else {
    rnti = strtol(buf, NULL, 16);
    if (rnti < 1 || rnti >= 0xfffe)
      ERROR_MSG_RET("RNTI needs to be [1,0xfffe]\n");
  }

  // verify it exists in RRC as well
  rrc_gNB_ue_context_t *rrcue = rrc_gNB_get_ue_context_by_rnti_any_du(RC.nrrrc[0], rnti);
  if (!rrcue)
    ERROR_MSG_RET("could not find UE with RNTI %04x\n", rnti);

  AssertFatal(false, "not implemented\n");
  //rrc_gNB_trigger_new_bearer(rnti);
  prnt("called rrc_gNB_trigger_new_bearer(%04x)\n", rnti);
  return 0;
}

//void rrc_gNB_trigger_release_bearer(int rnti);
int release_bearer(char *buf, int debug, telnet_printfunc_t prnt)
{
  int rnti = -1;
  if (!buf) {
    rnti = get_single_ue_rnti();
    if (rnti < 1)
      ERROR_MSG_RET("no UE found\n");
  } else {
    rnti = strtol(buf, NULL, 16);
    if (rnti < 1 || rnti >= 0xfffe)
      ERROR_MSG_RET("RNTI needs to be [1,0xfffe]\n");
  }

  // verify it exists in RRC as well
  rrc_gNB_ue_context_t *rrcue = rrc_gNB_get_ue_context_by_rnti_any_du(RC.nrrrc[0], rnti);
  if (!rrcue)
    ERROR_MSG_RET("could not find UE with RNTI %04x\n", rnti);

  AssertFatal(false, "not implemented\n");
  //rrc_gNB_trigger_release_bearer(rnti);
  prnt("called rrc_gNB_trigger_release_bearer(%04x)\n", rnti);
  return 0;
}

static telnetshell_cmddef_t bearercmds[] = {
  {"get_single_rnti", "", get_single_rnti},
  {"add_bearer", "[rnti(hex,opt)]", add_bearer},
  {"release_bearer", "[rnti(hex,opt)]", release_bearer},
  {"", "", NULL},
};

static telnetshell_vardef_t bearervars[] = {

  {"", 0, 0, NULL}
};

void add_bearer_cmds(void) {
  add_telnetcmd("bearer", bearervars, bearercmds);
}
