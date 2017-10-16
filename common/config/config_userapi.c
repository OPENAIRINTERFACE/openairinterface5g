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

char * config_check_valptr(paramdef_t *cfgoptions, char **ptr, int length) 
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
             fprintf (stderr,"[CONFIG] %s %d malloc error\n",__FILE__, __LINE__);
             exit(-1);
        }       
     }
     return *ptr;
}

void config_assign_int(paramdef_t *cfgoptions, char *fullname, int val)
{
int tmpval=val;
  if ( ((cfgoptions->paramflags &PARAMFLAG_BOOL) != 0) && tmpval >1) {
      tmpval =1;
  }
  switch (cfgoptions->type) {
       	case TYPE_UINT8:
	     *(cfgoptions->u8ptr) = (uint8_t)tmpval;
	     printf_params("[CONFIG] %s: %u\n", fullname, (uint8_t)tmpval);
	break;
       	case TYPE_INT8:
	     *(cfgoptions->i8ptr) = (int8_t)tmpval;
	     printf_params("[CONFIG] %s: %i\n", fullname, (int8_t)tmpval);
	break;		
       	case TYPE_UINT16:
	     *(cfgoptions->u16ptr) = (uint16_t)tmpval;
	     printf_params("[CONFIG] %s: %hu\n", fullname, (uint16_t)tmpval);     
	break;
       	case TYPE_INT16:
	     *(cfgoptions->i16ptr) = (int16_t)tmpval;
	     printf_params("[CONFIG] %s: %hi\n", fullname, (int16_t)tmpval);
	break;	
       	case TYPE_UINT32:
	     *(cfgoptions->uptr) = (uint32_t)tmpval;
	     printf_params("[CONFIG] %s: %u\n", fullname, (uint32_t)tmpval);
	break;
       	case TYPE_MASK:
	     *(cfgoptions->uptr) = *(cfgoptions->uptr) | (uint32_t)tmpval;
	     printf_params("[CONFIG] %s: 0x%08x\n", fullname, (uint32_t)tmpval);
	break;
       	case TYPE_INT32:
	     *(cfgoptions->iptr) = (int32_t)tmpval;
	     printf_params("[CONFIG] %s: %i\n", fullname, (int32_t)tmpval);
	break;	
	default:
	     fprintf (stderr,"[CONFIG] %s %i type %i non integer parameter %s not assigned\n",__FILE__, __LINE__,cfgoptions->type,fullname);
	break;
  }
}

void config_printhelp(paramdef_t *params,int numparams)
{
   for (int i=0 ; i<numparams ; i++) {
       if ( params[i].helpstr != NULL) {
           printf("%s%s: %s",
	           (strlen(params[i].optname) <= 1) ? "-" : "--", 
	           params[i].optname,
	           params[i].helpstr);
       }
   }
}

int config_get(paramdef_t *params,int numparams, char *prefix)
{
int ret= -1;

if (CONFIG_ISFLAGSET(CONFIG_ABORT)) {
    fprintf(stderr,"[CONFIG] config_get skipped, config module not properly initialized\n");
    return ret;
}
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

int config_isparamset(paramdef_t *params,int paramidx)
{
  if ((params[paramidx].paramflags & PARAMFLAG_PARAMSET) != 0) {
      return 1;
  } else {
      return 0;
  }
}
