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

#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>
#include <arpa/inet.h>
#include <platform_types.h>
#include "config_userapi.h"
#include "../utils/LOG/log.h"

configmodule_interface_t *config_get_if(void) {
  if (cfgptr == NULL) {
  	if (isLogInitDone())
       LOG_W(ENB_APP,"[CONFIG] %s %d config module not initialized\n",__FILE__,__LINE__);
  }

  return cfgptr;
}

static int managed_ptr_sz(void* ptr) {
  configmodule_interface_t * cfg=config_get_if();
  AssertFatal(cfg->numptrs < CONFIG_MAX_ALLOCATEDPTRS,
              "This code use fixed size array as #define CONFIG_MAX_ALLOCATEDPTRS %d\n",
              CONFIG_MAX_ALLOCATEDPTRS);
  int i;
  pthread_mutex_lock(&cfg->memBlocks_mutex);
  int numptrs=cfg->numptrs;
  for (i=0; i<numptrs; i++ )
    if (cfg->oneBlock[i].ptrs == ptr)
      break;
  pthread_mutex_unlock(&cfg->memBlocks_mutex);  
  if ( i == numptrs )
    return -1;
  else
    return cfg->oneBlock[i].sz;
}

void *config_allocate_new(int sz, bool autoFree) {
  void *ptr = calloc(sz,1);  
  AssertFatal(ptr, "calloc fails\n");
  configmodule_interface_t * cfg=config_get_if();
  // add the memory piece in the managed memory pieces list
  pthread_mutex_lock(&cfg->memBlocks_mutex);
  int newBlockIdx=cfg->numptrs++;
  oneBlock_t* tmp=&cfg->oneBlock[newBlockIdx];
  tmp->ptrs = (char *)ptr;
  tmp->ptrsAllocated = true;
  tmp->sz=sz;
  tmp->toFree=autoFree;
  pthread_mutex_unlock(&cfg->memBlocks_mutex);    
  return ptr;
}

void config_check_valptr(paramdef_t *cfgoptions, int elt_sz, int nb_elt) {

  const bool toFree=!(cfgoptions->paramflags & PARAMFLAG_NOFREE);

  // let's see if the value has been read
  // the datamodel is more difficult than it should be
  // and it is not thread safe
  if (cfgoptions->voidptr) {
    int sz;
    if ( cfgoptions-> type == TYPE_STRING)
      sz=managed_ptr_sz(*cfgoptions->strptr);
    else
      sz=managed_ptr_sz(cfgoptions->voidptr);
    if ( sz != -1 ) // the same variable pointer has been used!
      cfgoptions->numelt=sz;
  }
        
  if (cfgoptions->voidptr != NULL
      && (cfgoptions->type == TYPE_INTARRAY
          || cfgoptions->type == TYPE_UINTARRAY
          ||  cfgoptions->type == TYPE_STRINGLIST )) {
    int sz;
    if ( cfgoptions->type == TYPE_STRING ||
          cfgoptions->type == TYPE_STRINGLIST ) 
      sz=managed_ptr_sz(*cfgoptions->strptr);
    else
      sz=managed_ptr_sz(cfgoptions->voidptr);
    if ( sz != -1) {
      CONFIG_PRINTF_ERROR("[CONFIG] %s NOT SUPPORTED not NULL pointer with array types", cfgoptions->optname);
      return ;
    }
  }

  if (cfgoptions->voidptr == NULL ) {
    if (cfgoptions->type == TYPE_STRING) {
      // difficult datamodel
      cfgoptions->strptr = config_allocate_new(sizeof(*cfgoptions->strptr), toFree);
    } else if ( cfgoptions->type == TYPE_STRINGLIST) {
      AssertFatal(nb_elt<MAX_LIST_SIZE, 
                  "This piece of code use fixed size arry of constant #define MAX_LIST_SIZE %d\n",
                  MAX_LIST_SIZE );
      cfgoptions->strlistptr= config_allocate_new(sizeof(char*)*MAX_LIST_SIZE, toFree);
      for (int  i=0; i<MAX_LIST_SIZE; i++)
        cfgoptions->strlistptr[i]= config_allocate_new(DEFAULT_EXTRA_SZ, toFree);
    } else {
      if ( cfgoptions->type == TYPE_INTARRAY || cfgoptions->type == TYPE_UINTARRAY )
        nb_elt=max(nb_elt, MAX_LIST_SIZE); // make room if the list is larger than the default one
      cfgoptions->voidptr = config_allocate_new(elt_sz*nb_elt, toFree);
    }
  }

  if (cfgoptions->type == TYPE_STRING && *cfgoptions->strptr == NULL) {
    *cfgoptions->strptr = config_allocate_new(nb_elt+DEFAULT_EXTRA_SZ, toFree);
    cfgoptions->numelt=nb_elt+DEFAULT_EXTRA_SZ;
  }

  printf_ptrs("[CONFIG] %s ptr: %p requested size: elt_sz: %d, nb_elt: %d\n",cfgoptions->optname, cfgoptions->voidptr, elt_sz, nb_elt);
}

void config_assign_int(paramdef_t *cfgoptions, char *fullname, int val) {
  int tmpval=val;

  if ( ((cfgoptions->paramflags &PARAMFLAG_BOOL) != 0) && tmpval >0) {
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
  cfgoption->processedvalue = config_allocate_new(sizeof(int),!(cfgoption->paramflags & PARAMFLAG_NOFREE));

  if (  cfgoption->processedvalue != NULL) {
    *(cfgoption->processedvalue) = val;
  } else {
    CONFIG_PRINTF_ERROR("[CONFIG] %s %d malloc error\n",__FILE__, __LINE__);
  }
}

int config_get_processedint(paramdef_t *cfgoption) {
  int ret;

  if (  cfgoption->processedvalue != NULL) {
    ret=*(cfgoption->processedvalue);
    free( cfgoption->processedvalue);
    cfgoption->processedvalue=NULL;
    printf_params("[CONFIG] %s:  set from %s to %i\n",cfgoption->optname, *cfgoption->strptr, ret);
  } else {
    fprintf (stderr,"[CONFIG] %s %d %s has no processed integer availablle\n",__FILE__, __LINE__, cfgoption->optname);
    ret=0;
  }

  return ret;
}
void config_printhelp(paramdef_t *params,int numparams, char *prefix) {
  printf("\n-----Help for section %-26s: %03i entries------\n",(prefix==NULL)?"(root section)":prefix,numparams);

  for (int i=0 ; i<numparams ; i++) {
    printf("    %s%s: %s",
           (strlen(params[i].optname) <= 1) ? "-" : "--",
           params[i].optname,
           (params[i].helpstr != NULL)?params[i].helpstr:"Help string not specified\n");
  }   /* for on params entries */

  printf("--------------------------------------------------------------------\n\n");
}

int config_execcheck(paramdef_t *params, int numparams, char *prefix) {
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
    CONFIG_PRINTF_ERROR("[CONFIG] config_execcheck: section %s %i parameters with wrong value\n", prefix, -st);
  }

  return st;
}

int config_paramidx_fromname(paramdef_t *params, int numparams, char *name) {
  for (int i=0; i<numparams ; i++) {
    if (strcmp(name,params[i].optname) == 0)
      return i;
  }

  fprintf(stderr,"[CONFIG]config_paramidx_fromname , %s is not a valid parameter name\n",name);
  return -1;
}

int config_get(paramdef_t *params, int numparams, char *prefix) {
  int ret= -1;

  if (CONFIG_ISFLAGSET(CONFIG_ABORT)) {
    fprintf(stderr,"[CONFIG] config_get, section %s skipped, config module not properly initialized\n",prefix);
    return ret;
  }

  configmodule_interface_t *cfgif = config_get_if();
  if (cfgif != NULL) {
    ret = config_get_if()->get(params, numparams, prefix);

    if (ret >= 0) {
      config_process_cmdline(params, numparams, prefix);
      config_execcheck(params, numparams, prefix);
    }

    return ret;
  }

  return ret;
}

int config_getlist(paramlist_def_t *ParamList, paramdef_t *params, int numparams, char *prefix) {
  if (CONFIG_ISFLAGSET(CONFIG_ABORT)) {
    fprintf(stderr,"[CONFIG] config_get skipped, config module not properly initialized\n");
    return -1;
  }

  if (!config_get_if())
    return -1;

  const int ret = config_get_if()->getlist(ParamList, params, numparams, prefix);

  if (ret >= 0 && params) {
    char *newprefix;

    if (prefix) {
      int rc = asprintf(&newprefix, "%s.%s", prefix, ParamList->listname);

      if (rc < 0) newprefix = NULL;
    } else {
      newprefix = ParamList->listname;
    }

    char cfgpath[MAX_OPTNAME_SIZE*2 + 6]; /* prefix.listname.[listindex] */

    for (int i = 0; i < ParamList->numelt; ++i) {
      // TODO config_process_cmdline?
      sprintf(cfgpath, "%s.[%i]", newprefix, i);
      config_process_cmdline(ParamList->paramarray[i],numparams,cfgpath);
      config_execcheck(ParamList->paramarray[i], numparams, cfgpath);
    }

    if (prefix)
      free(newprefix);
  }

  return ret;
}

int config_isparamset(paramdef_t *params,int paramidx) {
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

int config_check_intval(paramdef_t *param) {
  if ( param == NULL ) {
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

int config_check_modify_integer(paramdef_t *param) {
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

int config_check_intrange(paramdef_t *param) {
  if( *(param->iptr) >= param->chkPptr->s2.okintrange[0]  && *(param->iptr) <= param->chkPptr->s2.okintrange[1]  ) {
    return 0;
  }

  fprintf(stderr,"[CONFIG] config_check_intrange: %s: %i invalid value, authorized range: %i %i\n",
          param->optname, (int)*(param->uptr), param->chkPptr->s2.okintrange[0], param->chkPptr->s2.okintrange[1]);
  return -1;
}

void print_strvalueerror(paramdef_t *param, char *fname, char **okval, int numokval) {
  fprintf(stderr,"[CONFIG] %s: %s: %s invalid value, authorized values:\n       ",
          fname,param->optname, *param->strptr);

  for ( int i=0; i<numokval ; i++) {
    fprintf(stderr, " %s",okval[i]);
  }

  fprintf(stderr, " \n");
}

int config_check_strval(paramdef_t *param) {
  if ( param == NULL ) {
    fprintf(stderr,"[CONFIG] config_check_strval: NULL param argument\n");
    return -1;
  }

  for ( int i=0; i<param->chkPptr->s3.num_okstrval ; i++) {
    if( strcasecmp(*param->strptr,param->chkPptr->s3.okstrval[i] ) == 0) {
      return 0;
    }
  }

  print_strvalueerror(param, "config_check_strval", param->chkPptr->s3.okstrval, param->chkPptr->s3.num_okstrval);
  return -1;
}

int config_checkstr_assign_integer(paramdef_t *param) {
  for (int i=0; i < param->chkPptr->s3a.num_okstrval ; i++) {
    if (strcasecmp(*param->strptr,param->chkPptr->s3a.okstrval[i]  ) == 0) {
      config_assign_processedint(param, param->chkPptr->s3a.setintval[i]);
      return 0;
    }
  }

  print_strvalueerror(param, "config_check_strval", param->chkPptr->s3a.okstrval, param->chkPptr->s3a.num_okstrval);
  return -1;
}

void config_set_checkfunctions(paramdef_t *params, checkedparam_t *checkfunctions, int numparams) {
  for (int i=0; i< numparams ; i++ ) {
    params[i].chkPptr = &(checkfunctions[i]);
  }
}

int config_setdefault_string(paramdef_t *cfgoptions, char *prefix) {
  int status = 0;

  if( cfgoptions->defstrval != NULL) {
    status=1;

    if (cfgoptions->numelt == 0 ) 
      config_check_valptr(cfgoptions, 1, strlen(cfgoptions->defstrval)+1);
    if ( cfgoptions->numelt < strlen(cfgoptions->defstrval)+1 ) {
      CONFIG_PRINTF_ERROR("[CONFIG] %s size too small\n", cfgoptions->optname);
    }
    snprintf(*cfgoptions->strptr, cfgoptions->numelt, "%s",cfgoptions->defstrval);
    printf_params("[CONFIG] %s.%s set to default value \"%s\"\n", ((prefix == NULL) ? "" : prefix), cfgoptions->optname, *cfgoptions->strptr);
    
  }

  return status;
}

int config_setdefault_stringlist(paramdef_t *cfgoptions, char *prefix) {
  int status = 0;

  if( cfgoptions->defstrlistval != NULL) {
    cfgoptions->strlistptr=cfgoptions->defstrlistval;
    status=1;

    for(int j=0; j<cfgoptions->numelt; j++)
      printf_params("[CONFIG] %s.%s[%i] set to default value %s\n", ((prefix == NULL) ? "" : prefix), cfgoptions->optname,j, cfgoptions->strlistptr[j]);
  }

  return status;
}

int config_setdefault_int(paramdef_t *cfgoptions, char *prefix) {
  int status = 0;
  config_check_valptr(cfgoptions, sizeof(*cfgoptions->iptr), 1);

  if( ((cfgoptions->paramflags & PARAMFLAG_MANDATORY) == 0)) {
    config_assign_int(cfgoptions,cfgoptions->optname,cfgoptions->defintval);
    status=1;
    printf_params("[CONFIG] %s.%s set to default value\n", ((prefix == NULL) ? "" : prefix), cfgoptions->optname);
  }

  return status;
}

int config_setdefault_int64(paramdef_t *cfgoptions, char *prefix) {
  int status = 0;
  config_check_valptr(cfgoptions, sizeof(*cfgoptions->i64ptr), 1);

  if( ((cfgoptions->paramflags & PARAMFLAG_MANDATORY) == 0)) {
    *(cfgoptions->u64ptr)=cfgoptions->defuintval;
    status=1;
    printf_params("[CONFIG] %s.%s set to default value %llu\n", ((prefix == NULL) ? "" : prefix), cfgoptions->optname, (long long unsigned)(*(cfgoptions->u64ptr)));
  }

  return status;
}

int config_setdefault_intlist(paramdef_t *cfgoptions, char *prefix) {
  int status = 0;

  if( cfgoptions->defintarrayval != NULL) {
    config_check_valptr(cfgoptions, sizeof(cfgoptions->iptr), cfgoptions->numelt);
    cfgoptions->iptr=cfgoptions->defintarrayval;
    status=1;

    for (int j=0; j<cfgoptions->numelt ; j++) {
      printf_params("[CONFIG] %s[%i] set to default value %i\n",cfgoptions->optname,j,(int)cfgoptions->iptr[j]);
    }
  }

  return status;
}

int config_setdefault_double(paramdef_t *cfgoptions, char *prefix) {
  int status = 0;
  config_check_valptr(cfgoptions, sizeof(*cfgoptions->dblptr), 1);

  if( ((cfgoptions->paramflags & PARAMFLAG_MANDATORY) == 0)) {
    *(cfgoptions->dblptr)=cfgoptions->defdblval;
    status=1;
    printf_params("[CONFIG] %s set to default value %lf\n",cfgoptions->optname, *(cfgoptions->dblptr));
  }

  return status;
}

int config_assign_ipv4addr(paramdef_t *cfgoptions, char *ipv4addr) {
  config_check_valptr(cfgoptions, sizeof(*cfgoptions->uptr), 1);
  int rst=inet_pton(AF_INET, ipv4addr,cfgoptions->uptr );

  if (rst == 1 && *(cfgoptions->uptr) > 0) {
    printf_params("[CONFIG] %s: %s\n",cfgoptions->optname, ipv4addr);
    return 1;
  } else {
    if ( strncmp(ipv4addr,ANY_IPV4ADDR_STRING,sizeof(ANY_IPV4ADDR_STRING)) == 0) {
      printf_params("[CONFIG] %s:%s (INADDR_ANY) \n",cfgoptions->optname,ipv4addr);
      *cfgoptions->uptr=INADDR_ANY;
      return 1;
    } else {
      fprintf(stderr,"[CONFIG] %s not valid for %s \n", ipv4addr, cfgoptions->optname);
      return -1;
    }
  }

  return 0;
}


int config_setdefault_ipv4addr(paramdef_t *cfgoptions,  char *prefix) {
  int status = 0;

  if (cfgoptions->defstrval != NULL) {
    status = config_assign_ipv4addr(cfgoptions, cfgoptions->defstrval);
  }

  return status;
}
