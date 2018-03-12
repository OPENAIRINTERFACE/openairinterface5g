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
#include "common/config/config_paramdesc.h"
#define CONFIG_MAX_OOPT_PARAMS    10     // maximum number of parameters in the -O option (-O <cfgmode>:P1:P2...
#define CONFIG_MAX_ALLOCATEDPTRS  1024   // maximum number of parameters that can be dynamicaly allocated in the config module

/* default values for configuration module parameters */
#define DEFAULT_CFGMODE           "libconfig"  // use libconfig file
#define DEFAULT_CFGFILENAME       "oai.conf"   // default config file

/* rtflags bit position definitions */
#define CONFIG_PRINTPARAMS    1               // print parameters values while processing
#define CONFIG_DEBUGPTR       1<<1            // print memory allocation/free debug messages
#define CONFIG_DEBUGCMDLINE   1<<2            // print command line processing messages
#define CONFIG_NOABORTONCHKF  1<<3            // disable abort execution when parameter checking function fails
#define CONFIG_HELP           1<<20           // print help message
#define CONFIG_ABORT          1<<21           // config failed,abort execution 
#define CONFIG_NOOOPT         1<<22           // no -O option found when parsing command line
typedef int(*configmodule_initfunc_t)(char *cfgP[],int numP);
typedef int(*configmodule_getfunc_t)(paramdef_t *,int numparams, char *prefix);
typedef int(*configmodule_getlistfunc_t)(paramlist_def_t *, paramdef_t *,int numparams, char *prefix);
typedef void(*configmodule_endfunc_t)(void);
typedef struct configmodule_interface
{
  int      argc;
  char     **argv;
  char     *cfgmode;
  int      num_cfgP;
  char     *cfgP[CONFIG_MAX_OOPT_PARAMS];
  configmodule_initfunc_t         init;
  configmodule_getfunc_t          get;
  configmodule_getlistfunc_t      getlist;
  configmodule_endfunc_t          end;
  uint32_t numptrs;
  uint32_t rtflags;
  char     *ptrs[CONFIG_MAX_ALLOCATEDPTRS];  
} configmodule_interface_t;

#ifdef CONFIG_LOADCONFIG_MAIN
configmodule_interface_t *cfgptr=NULL;

static char config_helpstr [] = "\n lte-softmodem -O [config mode]<:dbg[debugflags]> \n \
          debugflags can also be defined in the config_libconfig section of the config file\n \
          debugflags: mask,    1->print parameters, 2->print memory allocations debug messages\n \
                               4->print command line processing debug messages\n ";
			       
#define CONFIG_SECTIONNAME "config"
#define CONFIGPARAM_DEBUGFLAGS_IDX        0


static paramdef_t Config_Params[] = {
/*-----------------------------------------------------------------------------------------------------------------------*/
/*                                            config parameters for config module                                        */
/*   optname              helpstr           paramflags     XXXptr       defXXXval            type       numelt           */
/*-----------------------------------------------------------------------------------------------------------------------*/
{"debugflags",            config_helpstr,   0,             uptr:NULL,   defintval:0,        TYPE_MASK,  0}, 
};

#else
extern configmodule_interface_t *cfgptr;
#endif


#define printf_params(...) if ( (cfgptr->rtflags & CONFIG_PRINTPARAMS) != 0 )  { printf ( __VA_ARGS__ ); }
#define printf_ptrs(...)   if ( (cfgptr->rtflags & CONFIG_DEBUGPTR) != 0 )     { printf ( __VA_ARGS__ ); }     
#define printf_cmdl(...)   if ( (cfgptr->rtflags & CONFIG_DEBUGCMDLINE) != 0 ) { printf ( __VA_ARGS__ ); }
 
extern configmodule_interface_t *load_configmodule(int argc, char **argv);
extern void end_configmodule(void);

#endif  /* INCLUDE_CONFIG_LOADCONFIGMODULE_H */
