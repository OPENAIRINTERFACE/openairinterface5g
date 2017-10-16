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



/* parameter flags definitions */
/*   Flags to be used by calling modules in their parameters definitions to modify  config module behavior*/
#define PARAMFLAG_MANDATORY               (1 << 0)         // parameter must be explicitely set, default value ignored
#define PARAMFLAG_DISABLECMDLINE          (1 << 1)         // parameter cannot bet set from comand line
#define PARAMFLAG_DONOTREAD               (1 << 2)         // parameter must be ignored in get function
#define PARAMFLAG_NOFREE                  (1 << 3)         // don't free parameter in end function
#define PARAMFLAG_BOOL                    (1 << 4)         // integer param can be 0 or 1


/*   Flags used by config modules to return info to calling modules and/or to  for internal usage*/
#define PARAMFLAG_MALLOCINCONFIG          (1 << 15)        // parameter allocated in config module
#define PARAMFLAG_PARAMSET                (1 << 16)        // parameter has been explicitely set in get functions
#define PARAMFLAG_PARAMSETDEF             (1 << 17)        // parameter has been set to default value in get functions

typedef struct paramdef
{
   char optname[MAX_OPTNAME_SIZE];        /* parameter name, can be used as long command line option */
   char *helpstr;                         /* help string */
   unsigned int paramflags;               /* value is a "ored" combination of above PARAMFLAG_XXXX values */
   union {                                /* pointer to the parameter value, completed by the config module */
     char **strptr;
     char **strlistptr;
     uint8_t   *u8ptr;
     char      *i8ptr;     
     uint16_t  *u16ptr;
     int16_t   *i16ptr;
     uint32_t  *uptr;
     int32_t   *iptr;
     uint64_t  *u64ptr;
     int64_t   *i64ptr;
     double    *dblptr;
     } ;
   union {                                /* default parameter value, to be used when PARAMFLAG_MANDATORY is not specified */
     char *defstrval;
     char **defstrlistval;
     uint32_t defuintval;
     int defintval;
     uint64_t defint64val;
     int *defintarrayval;
     double defdblval;
     } ;   
   char type;                              /* parameter value type, as listed below as TYPE_XXXX macro */
   int numelt;                             /* number of elements in a list or array parameters or max size of string value */ 
} paramdef_t;

#define TYPE_INT        TYPE_INT32
#define TYPE_UINT       TYPE_UINT32
#define TYPE_STRING     1
#define TYPE_INT8       2
#define TYPE_UINT8      3
#define TYPE_INT16      4
#define TYPE_UINT16     5
#define TYPE_INT32      6
#define TYPE_UINT32     7
#define TYPE_INT64      8
#define TYPE_UINT64     9
#define TYPE_MASK       10
#define TYPE_DOUBLE     16
#define TYPE_IPV4ADDR   20


#define TYPE_STRINGLIST 50
#define TYPE_INTARRAY   51
#define TYPE_UINTARRAY  52
#define TYPE_LIST       55

#define ANY_IPV4ADDR_STRING "0.0.0.0"

typedef struct paramlist_def {
    char listname[MAX_OPTNAME_SIZE];
    paramdef_t **paramarray;
    int numelt ;
} paramlist_def_t;

/* macro helpers for module users */
#define CONFIG_PARAMLENGTH(A)    (sizeof(A)/sizeof(paramdef_t))
#endif  /* INCLUDE_CONFIG_PARAMDESC_H */
