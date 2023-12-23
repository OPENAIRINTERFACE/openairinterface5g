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
#ifndef _NRLDPC_EXTERN_H__
#define _NRLDPC_EXTERN_H__
#include "openair1/PHY/CODING/nrLDPC_defs.h"
/* LDPC maximum code block size - maximum E */
#define LDPC_MAX_CB_SIZE 32768
/* ldpc coder/decoder API*/
typedef struct ldpc_interface_s {
  LDPC_initfunc_t *LDPCinit;
  LDPC_shutdownfunc_t *LDPCshutdown;
  LDPC_decoderfunc_t *LDPCdecoder;
  LDPC_encoderfunc_t *LDPCencoder;
} ldpc_interface_t;

// Global var to limit the rework of the dirty legacy code
extern ldpc_interface_t ldpc_interface, ldpc_interface_offload;

/* functions to load the LDPC shared lib, implemented in openair1/PHY/CODING/nrLDPC_load.c */
int load_LDPClib(char *version, ldpc_interface_t *);
int free_LDPClib(ldpc_interface_t *ldpc_interface);

LDPC_initfunc_t LDPCinit;
LDPC_shutdownfunc_t LDPCshutdown;
LDPC_decoderfunc_t LDPCdecoder;
LDPC_encoderfunc_t LDPCencoder;

// inline functions:
#endif
