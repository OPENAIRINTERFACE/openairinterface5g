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
#include "openair1/PHY/CODING/nrLDPC_defs.h"

#ifdef LDPC_LOADER
nrLDPC_decoderfunc_t nrLDPC_decoder;
nrLDPC_encoderfunc_t nrLDPC_encoder;
#else
/* functions to load the LDPC shared lib, implemented in openair1/PHY/CODING/nrLDPC_load.c */
extern int load_nrLDPClib(void) ;
extern int load_nrLDPClib_ref(char *libversion, nrLDPC_encoderfunc_t * nrLDPC_encoder_ptr); // for ldpctest
/* ldpc coder/decoder functions, as loaded by load_nrLDPClib(). */
extern nrLDPC_decoderfunc_t nrLDPC_decoder;
extern nrLDPC_encoderfunc_t nrLDPC_encoder;
// inline functions:
#include "openair1/PHY/CODING/nrLDPC_decoder/nrLDPC_init_mem.h"
#endif
