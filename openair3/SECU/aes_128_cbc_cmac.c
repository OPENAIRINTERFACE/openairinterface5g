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

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "aes_128_cbc_cmac.h"

#include "common/utils/assertions.h"
#include <openssl/cmac.h>

#if OPENSSL_VERSION_MAJOR >= 3
// code for version 3.0 or greater
#include <openssl/core_names.h>

static 
const char *propq = NULL;

void aes_128_cbc_cmac(const aes_128_t* k_iv, byte_array_t msg, size_t len_out, uint8_t out[len_out])
{
  OSSL_LIB_CTX* library_context = OSSL_LIB_CTX_new();
  DevAssert(library_context != NULL);

  /* Fetch the CMAC implementation */
  EVP_MAC* mac = EVP_MAC_fetch(library_context, "CMAC", propq);
  DevAssert(mac != NULL);

  /* Create a context for the CMAC operation */
  EVP_MAC_CTX* mctx = EVP_MAC_CTX_new(mac);
  DevAssert(mctx != NULL);

  // The underlying cipher to be used 
  char cipher_name[] = "aes128";

  OSSL_PARAM params[2] = {0};
   params[0] = OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_CIPHER, cipher_name,
      sizeof(cipher_name));
   params[1] = OSSL_PARAM_construct_end();

  /* Initialise the CMAC operation */
  int rc = EVP_MAC_init(mctx, k_iv->key, sizeof(k_iv->key), params);
  DevAssert(rc != 0);

  size_t sz_iv = 0;
  uint8_t* iv = NULL;
  if(k_iv->type == AES_INITIALIZATION_VECTOR_8){
    sz_iv = 8;
    iv = (uint8_t*)k_iv->iv8.iv;
  } else if(k_iv->type == AES_INITIALIZATION_VECTOR_16) {
    sz_iv = 16;
    iv = (uint8_t*)k_iv->iv16.iv;
  } else {
    DevAssert(0!=0 && "Unknwon Initialization vector");
  }

  /* Make one or more calls to process the data to be authenticated */
  rc = EVP_MAC_update(mctx, iv, sz_iv);
  DevAssert(rc != 0);

  /* Make one or more calls to process the data to be authenticated */
  rc = EVP_MAC_update(mctx, msg.buf, msg.len);
  DevAssert(rc != 0);

  /* Make a call to the final with a NULL buffer to get the length of the MAC */
  size_t out_len = 0;
  rc = EVP_MAC_final(mctx, out, &out_len, len_out);
  DevAssert(rc != 0);

  EVP_MAC_CTX_free(mctx);
  EVP_MAC_free(mac);
  OSSL_LIB_CTX_free(library_context);
}

cbc_cmac_ctx_t init_aes_128_cbc_cmac(uint8_t key[16])
{
  DevAssert(key != NULL);

  OSSL_LIB_CTX* library_context = OSSL_LIB_CTX_new();
  DevAssert(library_context != NULL);

  /* Fetch the CMAC implementation */
  EVP_MAC* mac = EVP_MAC_fetch(library_context, "CMAC", propq);
  DevAssert(mac != NULL);

  cbc_cmac_ctx_t ctx = {.lib_ctx = library_context, .mac = mac }; 

  assert(16 == sizeof(ctx.key));
  memcpy(ctx.key, key, 16);

  return ctx; 
}

void cipher_aes_128_cbc_cmac(cbc_cmac_ctx_t const* ctx, const aes_128_t* k_iv, byte_array_t msg, size_t len_out, uint8_t out[len_out])
{
  DevAssert(ctx != NULL);
  DevAssert(k_iv != NULL);
  /* Create a context for the CMAC operation */
  EVP_MAC_CTX* mctx = EVP_MAC_CTX_new(ctx->mac);
  DevAssert(mctx != NULL);

  // The underlying cipher to be used 
  char cipher_name[] = "aes128";

  OSSL_PARAM params[2] = {0};
   params[0] = OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_CIPHER, cipher_name,
      sizeof(cipher_name));
   params[1] = OSSL_PARAM_construct_end();

  /* Initialise the CMAC operation */
  int rc = EVP_MAC_init(mctx, k_iv->key, sizeof(k_iv->key), params);
  DevAssert(rc != 0);

  size_t sz_iv = 0;
  uint8_t* iv = NULL;
  if(k_iv->type == AES_INITIALIZATION_VECTOR_8){
    sz_iv = 8;
    iv = (uint8_t*)k_iv->iv8.iv;
  } else if(k_iv->type == AES_INITIALIZATION_VECTOR_16) {
    sz_iv = 16;
    iv = (uint8_t*)k_iv->iv16.iv;
  } else {
    DevAssert(0!=0 && "Unknwon Initialization vector");
  }

  /* Make one or more calls to process the data to be authenticated */
  rc = EVP_MAC_update(mctx, iv, sz_iv);
  DevAssert(rc != 0);

  /* Make one or more calls to process the data to be authenticated */
  rc = EVP_MAC_update(mctx, msg.buf, msg.len);
  DevAssert(rc != 0);

  /* Make a call to the final with a NULL buffer to get the length of the MAC */
  size_t out_len = 0;
  rc = EVP_MAC_final(mctx, out, &out_len, len_out);
  DevAssert(rc != 0);

  EVP_MAC_CTX_free(mctx);
}

void free_aes_128_cbc_cmac(cbc_cmac_ctx_t const* ctx)
{
  DevAssert(ctx != NULL);
  EVP_MAC_free(ctx->mac);
  OSSL_LIB_CTX_free(ctx->lib_ctx);
}

#else
// code for 1.1.x or lower

void aes_128_cbc_cmac(const aes_128_t* k_iv, byte_array_t msg, size_t len_out, uint8_t out[len_out])
{
  DevAssert(k_iv != NULL);

  CMAC_CTX *ctx = CMAC_CTX_new();
  DevAssert(ctx != NULL);

  CMAC_Init(ctx, k_iv->key, sizeof(k_iv->key), EVP_aes_128_cbc(), NULL);

  size_t sz_iv = 0;
  uint8_t* iv = NULL;
  if(k_iv->type == AES_INITIALIZATION_VECTOR_8){
    sz_iv = 8;
    iv = (uint8_t*)k_iv->iv8.iv;
  } else if(k_iv->type == AES_INITIALIZATION_VECTOR_16) {
    sz_iv = 16;
    iv = (uint8_t*)k_iv->iv16.iv;
  } else {
    AssertFatal(0 != 0, "Unknwon Initialization vector");
  }

  CMAC_Update(ctx, iv, sz_iv);

  CMAC_Update(ctx, msg.buf, msg.len);
  size_t len_res = 0;
  CMAC_Final(ctx, out, &len_res);
  DevAssert(len_res <= len_out);

  CMAC_CTX_free(ctx);
}

cbc_cmac_ctx_t init_aes_128_cbc_cmac(uint8_t key[16])
{
  DevAssert(key != NULL);
  cbc_cmac_ctx_t ctx = {.lib_ctx = NULL, .mac = NULL }; 

  ctx.mac = CMAC_CTX_new();
  DevAssert(ctx.mac != NULL);

  //assert(16 == sizeof(ctx.key));
  memcpy(ctx.key, key, 16); //sizeof(ctx.key));

  CMAC_Init(ctx.mac, ctx.key, 16, EVP_aes_128_cbc(), NULL);
  return ctx;
}

void cipher_aes_128_cbc_cmac(cbc_cmac_ctx_t const* ctx, const aes_128_t* k_iv, byte_array_t msg, size_t len_out, uint8_t out[len_out])
{
  DevAssert(ctx != NULL);
  DevAssert(k_iv != NULL);

  //From https://man.openbsd.org/CMAC_Init.3
  //If ctx is already initialized, CMAC_Init() can be called again with key, 
  //cipher, and impl all set to NULL and key_len set to 0. In that case, any 
  //data already processed is discarded and ctx is re-initialized to start 
  //reading data anew.
  CMAC_Init(ctx->mac, NULL, 0, NULL, NULL);

  size_t sz_iv = 0;
  uint8_t* iv = NULL;
  if(k_iv->type == AES_INITIALIZATION_VECTOR_8){
    sz_iv = 8;
    iv = (uint8_t*)k_iv->iv8.iv;
  } else if(k_iv->type == AES_INITIALIZATION_VECTOR_16) {
    sz_iv = 16;
    iv = (uint8_t*)k_iv->iv16.iv;
  } else {
    AssertFatal(0 != 0, "Unknwon Initialization vector");
  }

  CMAC_Update(ctx->mac, iv, sz_iv);

  CMAC_Update(ctx->mac, msg.buf, msg.len);
  size_t len_res = 0;
  CMAC_Final(ctx->mac, out, &len_res);
  DevAssert(len_res <= len_out);
}

void free_aes_128_cbc_cmac(cbc_cmac_ctx_t const* ctx)
{
  DevAssert(ctx != NULL);
  CMAC_CTX_free(ctx->mac);
}

#endif

