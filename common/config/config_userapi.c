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
extern void exit_fun(const char* s);  // lte-softmodem clean exit function


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

     printf_ptrs("[CONFIG] %s ptr: 0x%08lx requested size: %i\n",cfgoptions->optname,(uintptr_t)(ptr),length);
     if(cfgoptions->numelt > 0) { /* already allocated */
          return *ptr;
     }

     if (*ptr == NULL) {
        *ptr = malloc(length);
        if ( *ptr != NULL) {
             memset(*ptr,0,length);
             if ( (cfgoptions->paramflags & PARAMFLAG_NOFREE) == 0) {
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
void config_assign_processedint(paramdef_t *cfgoption, int val) {
        cfgoption->processedvalue = malloc(sizeof(int));
        if (  cfgoption->processedvalue != NULL) {
             *(cfgoption->processedvalue) = val;
        } else {
             fprintf (stderr,"[CONFIG] %s %d malloc error\n",__FILE__, __LINE__);
             exit(-1);
        }            
}

int config_get_processedint(paramdef_t *cfgoption) {
   int ret;
        if (  cfgoption->processedvalue != NULL) {
             ret=*(cfgoption->processedvalue);
             free( cfgoption->processedvalue);
             cfgoption->processedvalue=NULL;
             printf_params("[CONFIG] %s:  set from %s to %i\n",cfgoption->optname, *(cfgoption->strptr), ret);
        } else {
             fprintf (stderr,"[CONFIG] %s %d %s has no processed integer availablle\n",__FILE__, __LINE__, cfgoption->optname);
             ret=0;
        }
return ret;            
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

int config_execcheck(paramdef_t *params,int numparams, char *prefix)
{
int st=0;

   for (int i=0 ; i<numparams ; i++) {
       if ( params[i].chkPptr == NULL) {
           continue;
       }
       if (params[i].chkPptr->s4.f4 != NULL) {
         st += params[i].chkPptr->s4.f4(&(params[i]));
       }
   }
   if (st != 0) {
      fprintf(stderr,"[CONFIG] config_execcheck: %i parameters with wrong value\n", -st); 
      if ( CONFIG_ISFLAGSET(CONFIG_NOABORTONCHKF) == 0) {
          exit_fun("exit because configuration failed\n");
      }
   }
return st;
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
         config_execcheck(params,numparams,prefix);
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

void print_intvalueerror(paramdef_t *param, char *fname, int *okval, int numokval) {
    fprintf(stderr,"[CONFIG] %s: %s: %i invalid value, authorized values:\n       ",
           fname,param->optname, (int)*(param->uptr));
    for ( int i=0; i<numokval ; i++) {
         fprintf(stderr, " %i",okval[i]);
         }
    fprintf(stderr, " \n");
}
 
int config_check_intval(paramdef_t *param)
{
    if ( param == NULL ){
       fprintf(stderr,"[CONFIG] config_check_intval: NULL param argument\n");
       return -1;
    }
    for ( int i=0; i<param->chkPptr->s1.num_okintval ; i++) {
         if( *(param->uptr) == param->chkPptr->s1.okintval[i] ) {
             return 0;
         }
    }
    print_intvalueerror(param,"config_check_intval", param->chkPptr->s1.okintval,param->chkPptr->s1.num_okintval);
    return -1;
}

int config_check_modify_integer(paramdef_t *param)
{
   
    for (int i=0; i < param->chkPptr->s1a.num_okintval ; i++) {
    	if (*(param->uptr) == param->chkPptr->s1a.okintval[i] ) {
            printf_params("[CONFIG] %s:  read value %i, set to %i\n",param->optname,*(param->uptr),param->chkPptr->s1a.setintval [i]);
    	    *(param->uptr) = param->chkPptr->s1a.setintval [i];
    	    return 0; 
    	}
   }
    print_intvalueerror(param,"config_check_modify_integer", param->chkPptr->s1a.okintval,param->chkPptr->s1a.num_okintval);
    return -1;
}

int config_check_intrange(paramdef_t *param)
{
   if( *(param->iptr) >= param->chkPptr->s2.okintrange[0]  && *(param->iptr) <= param->chkPptr->s2.okintrange[1]  ) {
       return 0;
   }
   fprintf(stderr,"[CONFIG] config_check_intrange: %s: %i invalid value, authorized range: %i %i\n",
           param->optname, (int)*(param->uptr), param->chkPptr->s2.okintrange[0], param->chkPptr->s2.okintrange[1]);
   return -1;
}

void print_strvalueerror(paramdef_t *param, char *fname, char **okval, int numokval) {
    fprintf(stderr,"[CONFIG] %s: %s: %s invalid value, authorized values:\n       ",
           fname,param->optname, *(param->strptr));
    for ( int i=0; i<numokval ; i++) {
         fprintf(stderr, " %s",okval[i]);
         }
    fprintf(stderr, " \n");
}
 
int config_check_strval(paramdef_t *param)
{
    if ( param == NULL ){
       fprintf(stderr,"[CONFIG] config_check_strval: NULL param argument\n");
       return -1;
    }
    for ( int i=0; i<param->chkPptr->s3.num_okstrval ; i++) {
         if( strcasecmp(*(param->strptr),param->chkPptr->s3.okstrval[i] ) == 0) {
             return 0;
         }
    }
    print_strvalueerror(param, "config_check_strval", param->chkPptr->s3.okstrval, param->chkPptr->s3.num_okstrval);
    return -1;
}

int config_checkstr_assign_integer(paramdef_t *param)
{


    for (int i=0; i < param->chkPptr->s3a.num_okstrval ; i++) {
    	if (strcasecmp(*(param->strptr),param->chkPptr->s3a.okstrval[i]  ) == 0) {
            config_assign_processedint(param, param->chkPptr->s3a.setintval[i]);
    	    return 0; 
    	}
   }
   print_strvalueerror(param, "config_check_strval", param->chkPptr->s3a.okstrval, param->chkPptr->s3a.num_okstrval);

    return -1;
}
