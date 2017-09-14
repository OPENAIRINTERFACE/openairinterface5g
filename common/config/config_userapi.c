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

/*! \file common/config/config_userapi.c
 * \brief configuration module, api implementation to access configuration parameters
 * \author Francois TABURET
 * \date 2017
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>
#include "config_userapi.h"


configmodule_interface_t *config_get_if(void)
{
    if (cfgptr == NULL) {
        fprintf(stderr,"[CONFIG] %s %d config module not initialized\n",__FILE__, __LINE__);
	exit(-1);
    }
    return cfgptr;
}

char * check_valptr(paramdef_t *cfgoptions, char **ptr, int length) 
{

     printf_ptrs("-- %s 0x%08lx %i\n",cfgoptions->optname,(uintptr_t)(*ptr),length);
     if (*ptr == NULL) {
        *ptr = malloc(length);
        if ( *ptr != NULL) {
             memset(*ptr,0,length);
             if ( (cfgoptions->paramflags & PARAMFLAG_NOFREE) != 0) {
                 config_get_if()->ptrs[config_get_if()->numptrs] = *ptr;
                 config_get_if()->numptrs++;
             }
        } else {
             fprintf (stderr,"[LIBCONFIG] %s %d malloc error\n",__FILE__, __LINE__);
             exit(-1);
        }       
     }
     return *ptr;
}

void config_printhelp(paramdef_t *params,int numparams)
{
   for (int i=0 ; i<numparams ; i++) {
       if ( params[i].helpstr != NULL) {
           printf("%s", params[i].helpstr);
       }
   }
}

int config_get(paramdef_t *params,int numparams, char *prefix)
{
int ret= -1;
configmodule_interface_t *cfgif = config_get_if();
if (cfgif != NULL) {
    ret = config_get_if()->get(params, numparams,prefix);
    if (ret >= 0) {
       config_process_cmdline(params,numparams,prefix);
   }
return ret;
}
return ret;
}
