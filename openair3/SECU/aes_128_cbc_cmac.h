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


#ifndef AES_128_CBC_CMAC_H
#define AES_128_CBC_CMAC_H 

#include "aes_128.h"
#include "common/utils/ds/byte_array.h"


#include <stdint.h>
#include <stdlib.h>

typedef struct {
  void* lib_ctx;
  void* mac;
  uint8_t key[16];
} cbc_cmac_ctx_t ;

void aes_128_cbc_cmac(const aes_128_t* k_iv, byte_array_t msg, size_t len_out, uint8_t out[len_out]);

cbc_cmac_ctx_t init_aes_128_cbc_cmac(uint8_t key[16]);

void cipher_aes_128_cbc_cmac(cbc_cmac_ctx_t const* ctx, const aes_128_t* k_iv, byte_array_t msg, size_t len_out, uint8_t out[len_out]);

void free_aes_128_cbc_cmac(cbc_cmac_ctx_t const* ctx);

#endif

