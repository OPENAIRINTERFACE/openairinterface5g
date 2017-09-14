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

/*! \file common/config/config_paramdesc.h
 * \brief configuration module, include file describing parameters, common to all implementations
 * \author Francois TABURET
 * \date 2017
 * \version 0.1
 * \company NOKIA BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#include <stdint.h>
#ifndef INCLUDE_CONFIG_PARAMDESC_H
#define INCLUDE_CONFIG_PARAMDESC_H





#define MAX_OPTNAME_SIZE 64
#define MAX_SHORTOPT_SIZE 8


/* parameter flags definitions */
/*   Flags to be used by calling modules in their parameters definitions: */
#define PARAMFLAG_DISABLECMDLINE          (1 << 0)         // parameter can bet set from comand line

/*   Flags used by config modules: */
/* flags to be used by caller modules when defining parameters */
#define PARAMFLAG_MANDATORY               (1 << 1)               // parameter must be explicitely set, default value ignored

/* flags used by config modules, at runtime to manage memory allocations */
#define PARAMFLAG_MALLOCINCONFIG         (1 << 15)        // parameter allocated in config module
#define PARAMFLAG_NOFREE                 (1 << 14)        // don't free parameter in end function

/* flags to be used by caller modules to modify get behavior */
#define PARAMFLAG_DONOTREAD              (1 << 20)        // parameter must be ignored in get function

typedef struct paramdef
{
   char optname[MAX_OPTNAME_SIZE];
   char shortopt[MAX_SHORTOPT_SIZE];
   char *helpstr;
   unsigned int paramflags;
   union {
     char **strptr;
     char **strlistptr;
     uint32_t *uptr;
     int32_t *iptr;
     uint64_t *u64ptr;
     int64_t *i64ptr;
     } ;
   union {
     char *defstrval;
     char **defstrlistval;
     uint32_t defuintval;
     int defintval;
     uint64_t defint64val;
     int *defintarrayval;
     } ;   
   char type;
   int numelt;
} paramdef_t;

#define TYPE_STRING     1
#define TYPE_INT        2
#define TYPE_UINT       3
#define TYPE_INT64      4
#define TYPE_UINT64     5
#define TYPE_IPV4ADDR   20


#define TYPE_STRINGLIST 50
#define TYPE_INTARRAY   51
#define TYPE_UINTARRAY  52
#define TYPE_LIST       55
#define NO_UINTDEFAULT ((int)(-1))
#define ANY_IPV4ADDR_STRING "0.0.0.0"

typedef struct paramlist_def {
    char listname[MAX_OPTNAME_SIZE];
    paramdef_t **paramarray;
    int numelt ;
} paramlist_def_t;

/* macro helpers for module users */
#define CONFIG_PARAMLENGTH(A)    (sizeof(A)/sizeof(paramdef_t))
#endif  /* INCLUDE_CONFIG_PARAMDESC_H */
