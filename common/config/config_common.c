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

#define _GNU_SOURCE

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <dlfcn.h>
#include <arpa/inet.h>
#include "common/platform_types.h"
#include "config_common.h"
#include "../utils/LOG/log.h"
static int managed_ptr_sz(configmodule_interface_t *cfg, void *ptr)
{
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

void config_check_valptr(configmodule_interface_t *cfg, paramdef_t *cfgoptions, int elt_sz, int nb_elt)
{
  const bool toFree=!(cfgoptions->paramflags & PARAMFLAG_NOFREE);

  // let's see if the value has been read
  // the datamodel is more difficult than it should be
  // and it is not thread safe
  if (cfgoptions->voidptr) {
    int sz;
    if ( cfgoptions-> type == TYPE_STRING)
      sz = managed_ptr_sz(cfg, *cfgoptions->strptr);
    else
      sz = managed_ptr_sz(cfg, cfgoptions->voidptr);
    if ( sz != -1 ) // the same variable pointer has been used!
      cfgoptions->numelt=sz;
  }
        
  if (cfgoptions->voidptr != NULL
      && (cfgoptions->type == TYPE_INTARRAY
          || cfgoptions->type == TYPE_UINTARRAY
          ||  cfgoptions->type == TYPE_STRINGLIST )) {
    int sz;
    if (cfgoptions->type == TYPE_STRING || cfgoptions->type == TYPE_STRINGLIST)
      sz = managed_ptr_sz(cfg, *cfgoptions->strptr);
    else
      sz = managed_ptr_sz(cfg, cfgoptions->voidptr);
    if ( sz == -1) {
       fprintf(stderr,"[CONFIG] %s NOT SUPPORTED not NULL pointer with array types", cfgoptions->optname);
      return ;
    }
  }

  if (cfgoptions->voidptr == NULL ) {
    if (cfgoptions->type == TYPE_STRING) {
      // difficult datamodel
      cfgoptions->strptr = config_allocate_new(cfg, sizeof(*cfgoptions->strptr), toFree);
    } else if ( cfgoptions->type == TYPE_STRINGLIST) {
      AssertFatal(nb_elt<MAX_LIST_SIZE, 
                  "This piece of code use fixed size arry of constant #define MAX_LIST_SIZE %d\n",
                  MAX_LIST_SIZE );
      cfgoptions->strlistptr = config_allocate_new(cfg, sizeof(char *) * MAX_LIST_SIZE, toFree);
      for (int  i=0; i<MAX_LIST_SIZE; i++)
        cfgoptions->strlistptr[i] = config_allocate_new(cfg, DEFAULT_EXTRA_SZ, toFree);
    } else {
      if ( cfgoptions->type == TYPE_INTARRAY || cfgoptions->type == TYPE_UINTARRAY )
        nb_elt=max(nb_elt, MAX_LIST_SIZE); // make room if the list is larger than the default one
      cfgoptions->voidptr = config_allocate_new(cfg, elt_sz * nb_elt, toFree);
    }
  }

  if (cfgoptions->type == TYPE_STRING && *cfgoptions->strptr == NULL) {
    *cfgoptions->strptr = config_allocate_new(cfg, nb_elt + DEFAULT_EXTRA_SZ, toFree);
    cfgoptions->numelt=nb_elt+DEFAULT_EXTRA_SZ;
  }
}

void *config_allocate_new(configmodule_interface_t *cfg, int sz, bool autoFree)
{
  void *ptr = calloc(sz, 1);
  AssertFatal(ptr, "calloc fails\n");
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

int config_setdefault_int(configmodule_interface_t *cfg, paramdef_t *cfgoptions, char *prefix)
{
  int status = 0;
  config_check_valptr(cfg, cfgoptions, sizeof(*cfgoptions->iptr), 1);

  if (((cfgoptions->paramflags & PARAMFLAG_MANDATORY) == 0)) {
    config_assign_int(cfg, cfgoptions, cfgoptions->optname, cfgoptions->defintval);
    status=1;
    printf_params(cfg, "[CONFIG] %s.%s set to default value\n", ((prefix == NULL) ? "" : prefix), cfgoptions->optname);
  }

  return status;
}

int config_setdefault_int64(configmodule_interface_t *cfg, paramdef_t *cfgoptions, char *prefix)
{
  int status = 0;
  config_check_valptr(cfg, cfgoptions, sizeof(*cfgoptions->i64ptr), 1);

  if (((cfgoptions->paramflags & PARAMFLAG_MANDATORY) == 0)) {
    *(cfgoptions->u64ptr) = cfgoptions->defuintval;
    status=1;
    printf_params(cfg,
                  "[CONFIG] %s.%s set to default value %llu\n",
                  ((prefix == NULL) ? "" : prefix),
                  cfgoptions->optname,
                  (long long unsigned)(*(cfgoptions->u64ptr)));
  }

  return status;
}

int config_setdefault_intlist(configmodule_interface_t *cfg, paramdef_t *cfgoptions, char *prefix)
{
  int status = 0;

  if (cfgoptions->defintarrayval != NULL) {
    config_check_valptr(cfg, cfgoptions, sizeof(cfgoptions->iptr), cfgoptions->numelt);
    cfgoptions->iptr = cfgoptions->defintarrayval;
    status=1;

    for (int j = 0; j < cfgoptions->numelt; j++) {
      printf_params(cfg, "[CONFIG] %s[%i] set to default value %i\n", cfgoptions->optname, j, (int)cfgoptions->iptr[j]);
    }
  }

  return status;
}

int config_setdefault_string(configmodule_interface_t *cfg, paramdef_t *cfgoptions, char *prefix)
{
  int status = 0;

  if (cfgoptions->defstrval != NULL) {
    status=1;

    if (cfgoptions->numelt == 0)
      config_check_valptr(cfg, cfgoptions, 1, strlen(cfgoptions->defstrval) + 1);
    if (cfgoptions->numelt < strlen(cfgoptions->defstrval) + 1) {
      fprintf(stderr,"[CONFIG] %s size too small\n", cfgoptions->optname);
    }
    snprintf(*cfgoptions->strptr, cfgoptions->numelt, "%s", cfgoptions->defstrval);
    printf_params(cfg,
                  "[CONFIG] %s.%s set to default value \"%s\"\n",
                  ((prefix == NULL) ? "" : prefix),
                  cfgoptions->optname,
                  *cfgoptions->strptr);
  }

  return status;
}

int config_setdefault_stringlist(configmodule_interface_t *cfg, paramdef_t *cfgoptions, char *prefix)
{
  int status = 0;

  if (cfgoptions->defstrlistval != NULL) {
    cfgoptions->strlistptr = cfgoptions->defstrlistval;
    status=1;

    for (int j = 0; j < cfgoptions->numelt; j++)
      printf_params(cfg,
                    "[CONFIG] %s.%s[%i] set to default value %s\n",
                    ((prefix == NULL) ? "" : prefix),
                    cfgoptions->optname,
                    j,
                    cfgoptions->strlistptr[j]);
  }

  return status;
}

int config_setdefault_double(configmodule_interface_t *cfg, paramdef_t *cfgoptions, char *prefix)
{
  int status = 0;
  config_check_valptr(cfg, cfgoptions, sizeof(*cfgoptions->dblptr), 1);

  if( ((cfgoptions->paramflags & PARAMFLAG_MANDATORY) == 0)) {
    *(cfgoptions->dblptr)=cfgoptions->defdblval;
    status=1;
    printf_params(cfg, "[CONFIG] %s set to default value %lf\n", cfgoptions->optname, *(cfgoptions->dblptr));
  }

  return status;
}

int config_assign_ipv4addr(configmodule_interface_t *cfg, paramdef_t *cfgoptions, char *ipv4addr)
{
  config_check_valptr(cfg, cfgoptions, sizeof(*cfgoptions->uptr), 1);
  int rst=inet_pton(AF_INET, ipv4addr,cfgoptions->uptr );

  if (rst == 1 && *(cfgoptions->uptr) > 0) {
    printf_params(cfg, "[CONFIG] %s: %s\n", cfgoptions->optname, ipv4addr);
    return 1;
  } else {
    if ( strncmp(ipv4addr,ANY_IPV4ADDR_STRING,sizeof(ANY_IPV4ADDR_STRING)) == 0) {
      printf_params(cfg, "[CONFIG] %s:%s (INADDR_ANY) \n", cfgoptions->optname, ipv4addr);
      *cfgoptions->uptr=INADDR_ANY;
      return 1;
    } else {
      fprintf(stderr,"[CONFIG] %s not valid for %s \n", ipv4addr, cfgoptions->optname);
      return -1;
    }
  }

  return 0;
}

int config_setdefault_ipv4addr(configmodule_interface_t *cfg, paramdef_t *cfgoptions, char *prefix)
{
  int status = 0;

  if (cfgoptions->defstrval != NULL) {
    status = config_assign_ipv4addr(cfg, cfgoptions, cfgoptions->defstrval);
  }

  return status;
}



void config_assign_int(configmodule_interface_t *cfg, paramdef_t *cfgoptions, char *fullname, int val)
{
  int tmpval=val;

  if ( ((cfgoptions->paramflags &PARAMFLAG_BOOL) != 0) && tmpval >0) {
    tmpval =1;
  }

  switch (cfgoptions->type) {
    case TYPE_UINT8:
      *(cfgoptions->u8ptr) = (uint8_t)tmpval;
      printf_params(cfg, "[CONFIG] %s: %u\n", fullname, (uint8_t)tmpval);
      break;

    case TYPE_INT8:
      *(cfgoptions->i8ptr) = (int8_t)tmpval;
      printf_params(cfg, "[CONFIG] %s: %i\n", fullname, (int8_t)tmpval);
      break;

    case TYPE_UINT16:
      *(cfgoptions->u16ptr) = (uint16_t)tmpval;
      printf_params(cfg, "[CONFIG] %s: %hu\n", fullname, (uint16_t)tmpval);
      break;

    case TYPE_INT16:
      *(cfgoptions->i16ptr) = (int16_t)tmpval;
      printf_params(cfg, "[CONFIG] %s: %hi\n", fullname, (int16_t)tmpval);
      break;

    case TYPE_UINT32:
      *(cfgoptions->uptr) = (uint32_t)tmpval;
      printf_params(cfg, "[CONFIG] %s: %u\n", fullname, (uint32_t)tmpval);
      break;

    case TYPE_MASK:
      *(cfgoptions->uptr) = *(cfgoptions->uptr) | (uint32_t)tmpval;
      printf_params(cfg, "[CONFIG] %s: 0x%08x\n", fullname, (uint32_t)tmpval);
      break;

    case TYPE_INT32:
      *(cfgoptions->iptr) = (int32_t)tmpval;
      printf_params(cfg, "[CONFIG] %s: %i\n", fullname, (int32_t)tmpval);
      break;

    default:
      fprintf (stderr,"[CONFIG] %s %i type %i non integer parameter %s not assigned\n",__FILE__, __LINE__,cfgoptions->type,fullname);
      break;
  }
}
