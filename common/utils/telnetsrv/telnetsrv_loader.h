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

/*! \file common/utils/telnetsrv_proccmd.h
 * \brief: Include file defining telnet commands related to softmodem linux process
 * \author Francois TABURET
 * \date 2018
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */

#ifdef TELNETSRV_LOADER_MAIN

#include "UTIL/LOG/log.h"


#include "common/utils/load_module_shlib.h"


telnetshell_vardef_t loader_globalvardef[] = {
{"mainversion",TELNET_VARTYPE_STRING,&(loader_data.mainexec_buildversion)},
{"defpath",TELNET_VARTYPE_STRING,&(loader_data.shlibpath)},
{"maxshlibs",TELNET_VARTYPE_INT32,&(loader_data.maxshlibs)},
{"numshlibs",TELNET_VARTYPE_INT32,&(loader_data.numshlibs)},
{"",0,NULL}
};
telnetshell_vardef_t *loader_modulesvardef;

extern void add_loader_cmds(void);

#endif

/*-------------------------------------------------------------------------------------*/

