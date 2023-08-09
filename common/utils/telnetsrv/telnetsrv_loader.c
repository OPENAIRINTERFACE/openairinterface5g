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

/*! \file common/utils/telnetsrv/telnetsrv_loader.c
 * \brief: implementation of telnet commands related to softmodem linux process
 * \author Francois TABURET
 * \date 2018
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#define _GNU_SOURCE 
#include <string.h>
#include <pthread.h>


#define TELNETSERVERCODE
#include "telnetsrv.h"
#define TELNETSRV_LOADER_MAIN
#include "telnetsrv_loader.h"


int loader_show_cmd(char *buff, int debug, telnet_printfunc_t prnt);
telnetshell_cmddef_t loader_cmdarray[] = {
    {"show", "[params,modules]", loader_show_cmd, {(webfunc_t)loader_show_cmd}, 0, NULL},
    {"", "", NULL, {NULL}, 0, NULL},
};

/*-------------------------------------------------------------------------------------*/
int loader_show_cmd(char *buff, int debug, telnet_printfunc_t prnt)
{
  if (buff == NULL) {
    prnt("ERROR wrong loader SHOW command...\n");
    return 0;
  }
   if (debug > 0)
       prnt( "loader_show_cmd received %s\n",buff);

      if (strcasestr(buff,"params") != NULL) {
          prnt( "loader parameters:\n");
          prnt( "   Main executable build version: \"%s\"\n", loader_data.mainexec_buildversion);
          prnt( "   Default shared lib path: \"%s\"\n", loader_data.shlibpath);
          prnt( "   Max number of shared lib : %i\n", loader_data.maxshlibs);
      } else if (strcasestr(buff, "modules") != NULL || buff[0] == 0 || strcasestr(buff, "show") != NULL) {
        prnt("%i shared lib have been dynamicaly loaded by the oai loader\n", loader_data.numshlibs);
        for (int i = 0; i < loader_data.numshlibs; i++) {
          prnt("   Module %i: %s\n", i, loader_data.shlibs[i].name);
          prnt("       Shared library build version: \"%s\"\n", ((loader_data.shlibs[i].shlib_buildversion == NULL) ? "" : loader_data.shlibs[i].shlib_buildversion));
          prnt("       Shared library path: \"%s\"\n", loader_data.shlibs[i].thisshlib_path);
          prnt("       %i function pointers registered:\n", loader_data.shlibs[i].numfunc);
          for (int j = 0; j < loader_data.shlibs[i].numfunc; j++) {
            prnt("          function %i %s at %p\n", j, loader_data.shlibs[i].funcarray[j].fname, loader_data.shlibs[i].funcarray[j].fptr);
          }
        }
      } else {
        prnt("%s: wrong loader command...\n", buff);
      }
   return 0;
}

void add_loader_cmds(void)
{

   add_telnetcmd("loader", loader_globalvardef, loader_cmdarray);
}
