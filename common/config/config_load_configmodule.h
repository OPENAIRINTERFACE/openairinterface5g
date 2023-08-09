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

/*! \file common/config/config_load_configmodule.h
 * \brief: configuration module, include file to be used by the source code calling the
 *  configuration module initialization
 * \author Francois TABURET
 * \date 2017
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#ifndef INCLUDE_CONFIG_LOADCONFIGMODULE_H
#define INCLUDE_CONFIG_LOADCONFIGMODULE_H


#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "common/config/config_paramdesc.h"
#include "common/utils/T/T.h"
#define CONFIG_MAX_OOPT_PARAMS    10     // maximum number of parameters in the -O option (-O <cfgmode>:P1:P2...
#define CONFIG_MAX_ALLOCATEDPTRS  2048   // maximum number of parameters that can be dynamicaly allocated in the config module

/* default values for configuration module parameters */
#define CONFIG_LIBCONFIGFILE        "libconfig"  // use libconfig file
#define CONFIG_CMDLINEONLY          "cmdline"    // use only command line options
#define DEFAULT_CFGFILENAME         "oai.conf"   // default config file
/*   bit position definition for the argv_info mask of the configmodule_interface_t structure */
#define CONFIG_CMDLINEOPT_PROCESSED    (1<<0)   // command line option has been processed
/*  bit position definitions for the rtflags mask of the configmodule_interface_t structure*/
#define CONFIG_PRINTPARAMS    1                 // print parameters values while processing
#define CONFIG_DEBUGPTR       (1<<1)            // print memory allocation/free debug messages
#define CONFIG_DEBUGCMDLINE   (1<<2)            // print command line processing messages
#define CONFIG_NOABORTONCHKF  (1<<4)            // disable abort execution when parameter checking function fails
#define CONFIG_SAVERUNCFG (1 << 5) // create config file with running values, as after cfg file + command line processing
#define CONFIG_NOEXITONHELP   (1<<19)           // do not exit after printing help
#define CONFIG_HELP           (1<<20)           // print help message
#define CONFIG_ABORT          (1<<21)           // config failed,abort execution 
#define CONFIG_NOOOPT         (1<<22)           // no -O option found when parsing command line
struct configmodule_interface;
typedef int (*configmodule_getfunc_t)(struct configmodule_interface *, paramdef_t *, int numparams, char *prefix);
typedef int (
    *configmodule_getlistfunc_t)(struct configmodule_interface *, paramlist_def_t *, paramdef_t *, int numparams, char *prefix);
typedef int (*configmodule_setfunc_t)(paramdef_t *cfgoptions, int numoptions, char *prefix);
typedef void (*configmodule_endfunc_t)(struct configmodule_interface *cfg);

typedef int (*configmodule_initfunc_t)(struct configmodule_interface *cfg);

typedef struct configmodule_status {
  int num_paramgroups;
  char **paramgroups_names;
  int num_err_nullvalue;
  int emptyla;
  int num_err_read;
  int num_err_write;
  int num_read;
  int num_write;
  char *debug_cfgname;
} configmodule_status_t;

typedef struct oneBlock_s{
    void *ptrs;
    int sz;
    bool toFree;
    bool ptrsAllocated;
} oneBlock_t;

typedef struct configmodule_interface {
  int      argc;
  char     **argv;
  uint32_t *argv_info;
  char     *cfgmode;
  int      num_cfgP;
  char     *cfgP[CONFIG_MAX_OOPT_PARAMS];
  configmodule_initfunc_t         init;
  configmodule_getfunc_t          get;
  configmodule_getlistfunc_t      getlist;
  configmodule_setfunc_t set;
  configmodule_endfunc_t          end;
  pthread_mutex_t  memBlocks_mutex;
  configmodule_endfunc_t write_parsedcfg;
  uint32_t numptrs;
  uint32_t rtflags;
  oneBlock_t oneBlock[CONFIG_MAX_ALLOCATEDPTRS];
  char *tmpdir;
  configmodule_status_t *status; // allocated in debug mode only
} configmodule_interface_t;

#define CONFIG_HELP_TMPDIR "<Repository to create temporary file>"

#define CONFIG_SECTIONNAME "config"
#define CONFIGP_DEBUGFLAGS "debugflags"
#define CONFIGP_TMPDIR "tmpdir"

// extern configmodule_interface_t *cfgptr;

#define printf_params(cfg, ...)                     \
  if ((cfg->rtflags & (CONFIG_PRINTPARAMS)) != 0) { \
    printf(__VA_ARGS__);                            \
  }

#define printf_cmdl(cfg, ...)                        \
  if ((cfg->rtflags & (CONFIG_DEBUGCMDLINE)) != 0) { \
    printf(__VA_ARGS__);                             \
  }

#define CONFIG_ENABLECMDLINEONLY  (1<<1)
extern configmodule_interface_t *load_configmodule(int argc, char **argv, uint32_t initflags);
/* free ressources used to read parameters, keep memory 
 * allocated for parameters values which has been defined with the PARAMFLAG_NOFREE flag
 * should be used as soon as there is no need to read parameters but doesn't prevent
 * a new config module init
*/
void end_configmodule(configmodule_interface_t *cfg);
void write_parsedcfg(configmodule_interface_t *cfg);

/* free all config module memory, to be used at end of program as
 * it will free parameters values even those specified with the PARAMFLAG_NOFREE flag */
extern void free_configmodule(void);
#define CONFIG_PRINTF_ERROR(f, x... ) if (isLogInitDone ()) { LOG_E(ENB_APP,f,x);} else {printf(f,x);}; if ( !CONFIG_ISFLAGSET(CONFIG_NOABORTONCHKF) ) exit_fun("exit because configuration failed\n");


#endif  /* INCLUDE_CONFIG_LOADCONFIGMODULE_H */
