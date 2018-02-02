/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*! \file common/utils/telnetsrv/telnetsrv_proccmd.h
 * \brief: Include file defining telnet commands related to this linux process
 * \author Francois TABURET
 * \date 2017
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#include <dlfcn.h>
#include "telnetsrv.h"


#ifdef TELNETSRV_PROCCMD_MAIN


extern int proccmd_show(char *buf, int debug, telnet_printfunc_t prnt);
extern int proccmd_thread(char *buf, int debug, telnet_printfunc_t prnt);
extern int proccmd_exit(char *buf, int debug, telnet_printfunc_t prnt);
extern int proccmd_log(char *buf, int debug, telnet_printfunc_t prnt);
telnetshell_vardef_t proc_vardef[] = {
{"",0,NULL}
};

telnetshell_cmddef_t proc_cmdarray[] = {
   {"show","loglvl|thread", proccmd_show},
   {"log","[enable,disable]", proccmd_log},
   {"thread","<id> aff|prio <aff|prio>", proccmd_thread},
   {"exit","", proccmd_exit},
   {"","",NULL},
};
#else
extern void add_proccmd_cmds();
#endif  /* TELNETSRV_PROCCMD_MAIN */

