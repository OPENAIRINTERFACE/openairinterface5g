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

#ifndef __TELNETSRV_PROCCMD__H__
#define __TELNETSRV_PROCCMD__H__

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
#define PROCCMD_LOG_HELP_STRING " log sub commands: \n\
 show:  		     display current log configuration \n\
 online, noonline:	     enable or disable console logs \n\
 enable, disable id1-id2:    enable or disable logs for components index id1 to id2 \n\
 file, nofile id1-id2:       enable or disable redirecting logs to file for components index id1 to id2 \n\
                             logfile name depends on component name and is printed in the show command \n\
 level_<level> id1-id2:      set log level to <level> for components index id1 to id2 \n\
                             use the show command to get the authorized values for \n\
                             <level> and the list of component indexes that can be used for id1 and id2 \n\
 print_<opt> <0|1>           disable or enable the \"opt\" log option, use the show command to get \
                             the available options\n\
 dump_<mod> debug_<mod >     disable or enable the debug file generation or debug code\
                             for \"mod\" module. use the show command to get the available modules\n"  

#define PROCCMD_THREAD_HELP_STRING " thread sub commands: \n\
 <thread id> aff <core>  :    set affinity of thread <thread id> to core <core> \n\
 <thread id> prio <prio> :    set scheduling parameters for thread <thread id>  \n\
                   if prio < -20: linux scheduling policy set to FIFO, \n\
                                  with priority = -20 - prio \n\
                   if prio > 19: linux scheduling policy set to OTHER, \n\
                                  with priority (nice value) =  prio \n\
  use \"softmodem show thread\" to get <thread id> \n"
 

telnetshell_cmddef_t proc_cmdarray[] = {
   {"show","loglvl|thread|config", proccmd_show},
   {"log","(enter help for details)", proccmd_log},
   {"thread","(enter help for details)", proccmd_thread},
   {"exit","", proccmd_exit},
   {"","",NULL},
};
#else
extern void add_proccmd_cmds(void);
#endif  /* TELNETSRV_PROCCMD_MAIN */

#endif
