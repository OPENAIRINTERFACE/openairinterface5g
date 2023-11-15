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
#define CONFIG_MAXOPTLENGTH 512 /* max full option length, full option name exemple: (prefix1.[<index>].prefix2.optname */


/* parameter flags definitions */
/*   Flags to be used by calling modules in their parameters definitions to modify  config module behavior*/
#define PARAMFLAG_MANDATORY               (1 << 0)         // parameter must be explicitely set, default value ignored
#define PARAMFLAG_DISABLECMDLINE          (1 << 1)         // parameter cannot bet set from comand line
#define PARAMFLAG_DONOTREAD               (1 << 2)         // parameter must be ignored in get function
#define PARAMFLAG_NOFREE                  (1 << 3)         // don't free parameter in end function
#define PARAMFLAG_BOOL                    (1 << 4)         // integer param can be 0 or 1
#define PARAMFLAG_CMDLINE_NOPREFIXENABLED (1 << 5)         // on the command line, allow a parameter to be specified without the prefix
#define PARAMFLAG_CMDLINEONLY (1 << 6) // this parameter cannot be specified in config file

/*   Flags used by config modules to return info to calling modules and/or to  for internal usage*/
#define PARAMFLAG_MALLOCINCONFIG          (1 << 15)        // parameter allocated in config module
#define PARAMFLAG_PARAMSET                (1 << 16)        // parameter has been explicitely set in get functions
#define PARAMFLAG_PARAMSETDEF             (1 << 17)        // parameter has been set to default value in get functions

/* checkedparam_t is possibly used in paramdef_t for specific parameter value validation */
#define CONFIG_MAX_NUMCHECKVAL            20
typedef struct paramdef paramdef_t;
typedef struct configmodule_interface configmodule_interface_t;
typedef union checkedparam {
  struct  {
    int (*f1)(configmodule_interface_t *cfg, paramdef_t *param); /* check an integer against a list of authorized values */
    int okintval[CONFIG_MAX_NUMCHECKVAL];                        /* integer array, store possible values  */
    int num_okintval;                                            /* number of valid values in the checkingval array */
  } s1;
  struct  {
    int (*f1a)(configmodule_interface_t *cfg,
               paramdef_t *param); /* check an integer against a list of authorized values and set param value */
    /* to the corresponding item in setintval array (mainly for RRC params)     */
    int okintval[CONFIG_MAX_NUMCHECKVAL];                        /* integer array, store possible values in config file */
    int setintval[CONFIG_MAX_NUMCHECKVAL];                        /* integer array, values set in the paramdef structure */
    int num_okintval;                                            /* number of valid values in the checkingval array */
  } s1a;
  struct {
    int (*f2)(configmodule_interface_t *cfg,
              paramdef_t *param); /* check an integer against an authorized range, defined by its min and max value */
    int okintrange[CONFIG_MAX_NUMCHECKVAL];  /* integer array, store  min and max values  */

  } s2;
  struct {
    int (*f3)(configmodule_interface_t *cfg, paramdef_t *param); /* check a string against a list of authorized values */
    char *okstrval[CONFIG_MAX_NUMCHECKVAL];                      /* string array, store possible values  */
    int  num_okstrval;                                           /* number of valid values in the checkingval array */
  } s3;
  struct {
    int (*f3a)(configmodule_interface_t *cfg,
               paramdef_t *param); /* check a string against a list of authorized values and set param value */
    /* to the corresponding item in setintval array (mainly for RRC params) */
    char *okstrval[CONFIG_MAX_NUMCHECKVAL];                      /* string array, store possible values  */
    int  setintval[CONFIG_MAX_NUMCHECKVAL];                      /* integer array, values set in the paramdef structure */
    int  num_okstrval;                                           /* number of valid values in the checkingval array */
  } s3a;
  struct {
    int (*f4)(configmodule_interface_t *cfg,
              paramdef_t *param); /* generic check function, no arguments but the param description */

  } s4;
  struct {
    void (*checkfunc)(configmodule_interface_t *cfg);
  } s5;
} checkedparam_t;

/* paramdef is used to describe a parameter, array of paramdef_t strustures is used as the main parameter in */
/* config apis used to retrieve parameters values  */
#define MAX_LIST_SIZE 32
#define DEFAULT_EXTRA_SZ 256
typedef struct paramdef {
  char         optname[MAX_OPTNAME_SIZE]; /* parameter name, can be used as long command line option */
  char         *helpstr;                  /* help string */
  unsigned int paramflags;                /* value is a "ored" combination of above PARAMFLAG_XXXX values */
  union { /* pointer to the parameter value, completed by the config module */
    char      **strptr;
    char      **strlistptr;
    uint8_t   *u8ptr;
    int8_t    *i8ptr;
    uint16_t  *u16ptr;
    int16_t   *i16ptr;
    uint32_t  *uptr;
    int32_t   *iptr;
    uint64_t  *u64ptr;
    int64_t   *i64ptr;
    double    *dblptr;
    void      *voidptr;
  } ;
  union {                                /* default parameter value, to be used when PARAMFLAG_MANDATORY is not specified */
    char      *defstrval;
    char      **defstrlistval;
    uint32_t  defuintval;
    int       defintval;
    uint64_t  defint64val;
    int       *defintarrayval;
    double    defdblval;
  } ;
  char type;                              /* parameter value type, as listed below as TYPE_XXXX macro */
  int numelt;                             /* number of elements in a list or array parameters or max size of string value */
  checkedparam_t   *chkPptr;              /* possible pointer to the structure containing the info used to check parameter values */
  int *processedvalue;                    /* used to store integer values computed from string original value */
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
#define TYPE_LASTSCALAR 25

#define PARAM_ISLISTORARRAY(P)  (P->type >  TYPE_LASTSCALAR )
#define PARAM_ISSCALAR(P)       (P->type <  TYPE_LASTSCALAR )

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

#endif  /* INCLUDE_CONFIG_PARAMDESC_H */
