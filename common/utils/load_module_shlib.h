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




typedef struct {
   char *fname;
   int (*fptr)(void);
}loader_shlibfunc_t;

typedef struct {
   char               *name;
   char               *shlib_version;    // 
   char               *shlib_buildversion;
   char               *thisshlib_path;
   uint32_t           numfunc;
   loader_shlibfunc_t *funcarray;
}loader_shlibdesc_t;

typedef struct {
   char               *mainexec_buildversion;
   char               *shlibpath;
   uint32_t           maxshlibs;
   uint32_t           numshlibs;
   loader_shlibdesc_t *shlibs;
}loader_data_t;

/* function type of functions which may be implemented by a module */
/* 1: init function, called when loading, if found in the shared lib */
typedef int(*initfunc_t)(void);
/* 2: version checking function, called when loading, if it returns -1, trigger main exec abort  */
typedef int(*checkverfunc_t)(char * mainexec_version, char ** shlib_version);
/* 3: get function array function, called when loading when a module doesn't provide */
/* the function array when calling load_module_shlib (farray param NULL)             */
typedef int(*getfarrayfunc_t)(loader_shlibfunc_t **funcarray);

#ifdef LOAD_MODULE_SHLIB_MAIN
#define LOADER_CONFIG_PREFIX  "loader"
#define DEFAULT_PATH      ""
#define DEFAULT_MAXSHLIBS 10
loader_data_t loader_data;

/*------------------------------------------------------------------------------------------------------------------------------------------------*/
/*                                       LOADER parameters                                                                                        */
/*   optname               helpstr   paramflags           XXXptr	                           defXXXval	              type       numelt   */
/*------------------------------------------------------------------------------------------------------------------------------------------------*/
#define LOADER_PARAMS_DESC { \
{"shlibpath",                NULL,    PARAMFLAG_NOFREE, strptr:(char **)&(loader_data.shlibpath), defstrval:DEFAULT_PATH,      TYPE_STRING, 0}, \
{"maxshlibs",                NULL,    0,                uptr:&(loader_data.maxshlibs),            defintval:DEFAULT_MAXSHLIBS, TYPE_UINT32, 0}, \
}

/*-------------------------------------------------------------------------------------------------------------*/
#else  /* LOAD_MODULE_SHLIB_MAIN */
extern int load_module_shlib(char *modname, loader_shlibfunc_t *farray, int numf);
extern void * get_shlibmodule_fptr(char *modname, char *fname);
extern loader_data_t loader_data;
#endif /* LOAD_MODULE_SHLIB_MAIN */

#endif

