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

/*! \file openair1/PHY/TOOLS/phy_scope_interface.c
 * \brief soft scope API interface implementation
 * \author Nokia BellLabs France, francois Taburet
 * \date 2019
 * \version 0.1
 * \company Nokia BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#include <stdio.h>
#include "common/config/config_userapi.h"
#include "common/utils/load_module_shlib.h"
#include "phy_scope_interface.h"

#define SOFTSCOPE_ENDFUNC_IDX 0

static  loader_shlibfunc_t scope_fdesc[]= {{"end_forms",NULL}};

int load_softscope(char *exectype, void *initarg) {
  char libname[64];
  sprintf(libname,"%.10sscope",exectype);
  return load_module_shlib(libname,scope_fdesc,1,initarg);
}

int end_forms(void) {
  if (scope_fdesc[SOFTSCOPE_ENDFUNC_IDX].fptr) {
    scope_fdesc[SOFTSCOPE_ENDFUNC_IDX].fptr();
    return 0;
  }

  return -1;
}
