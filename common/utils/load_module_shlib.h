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

/*! \file common/utils/load_module_shlib.h
 * \brief include file for users of the shared lib loader
 * \author Francois TABURET
 * \date 2017
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#ifndef LOAD_SHLIB_H
#define LOAD_SHLIB_H


typedef int(*initfunc_t)(void);

typedef struct {
   char *shlibpath;
}loader_data_t;

typedef struct {
   char *fname;
   int (*fptr)(void);
}loader_shlibfunc_t;
#ifdef LOAD_MODULE_SHLIB_MAIN
#define LOADER_CONFIG_PREFIX  "loader"
#define DEFAULT_PATH ""
loader_data_t loader_data;

/*--------------------------------------------------------------------------------------------------------------------------------------*/
/*                                       LOADER parameters                                                                              */
/*   optname               helpstr   paramflags    XXXptr	                           defXXXval	            type       numelt   */
/*--------------------------------------------------------------------------------------------------------------------------------------*/
#define LOADER_PARAMS_DESC { \
{"shlibpath",                NULL,    0,          strptr:(char **)&(loader_data.shlibpath), defstrval:DEFAULT_PATH, TYPE_STRING, 0} \
}

/*-------------------------------------------------------------------------------------------------------------*/
#else
extern int load_module_shlib(char *modname, loader_shlibfunc_t *farray, int numf);
#endif

#endif

