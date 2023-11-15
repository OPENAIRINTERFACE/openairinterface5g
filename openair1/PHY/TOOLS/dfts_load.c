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

/*! \file dfts_load.c
 * \brief: load library implementing coding/decoding algorithms
 * \author Francois TABURET
 * \date 2020
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#define _GNU_SOURCE 
#include <sys/types.h>
#include <stdlib.h>
#include <malloc.h>
#include "assertions.h"
#include "common/utils/LOG/log.h"
#define OAIDFTS_LOADER
#include "tools_defs.h"
#include "common/config/config_userapi.h" 
#include "common/utils/load_module_shlib.h" 


/* function description array, to be used when loading the dfts/idfts lib */
static loader_shlibfunc_t shlib_fdesc[2];
static char *arg[64] = {"phytest", "-O", "cmdlineonly::dbgl0"};
int load_dftslib(void)
{
  char *ptr = (char *)config_get_if();
  if (ptr == NULL) { // phy simulators, config module possibly not loaded
    uniqCfg = load_configmodule(3, (char **)arg, CONFIG_ENABLECMDLINEONLY);
    logInit();
  }
  shlib_fdesc[0].fname = "dft";
  shlib_fdesc[1].fname = "idft";
  int ret = load_module_shlib("dfts", shlib_fdesc, sizeof(shlib_fdesc) / sizeof(loader_shlibfunc_t), NULL);
  AssertFatal((ret >= 0), "Error loading dftsc decoder");
  dft = (dftfunc_t)shlib_fdesc[0].fptr;
  idft = (idftfunc_t)shlib_fdesc[1].fptr;
  return 0;
}
