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

#include "common/utils/assertions.h"
#include "sha_256_hmac.h"

#include <openssl/hmac.h>

#if OPENSSL_VERSION_NUMBER >= 0x30000000L

/* code for version 3.0 or greater */

#include <openssl/core_names.h>

void sha_256_hmac(const uint8_t key[32], byte_array_t data, size_t len, uint8_t out[len])
{
  DevAssert(key != NULL);
  DevAssert(data.buf != NULL);
  DevAssert(data.len != 0);
  DevAssert(len != 0);

  OSSL_LIB_CTX* library_context = OSSL_LIB_CTX_new();
  DevAssert(library_context != NULL);

  // A property query used for selecting the MAC implementation.
  const char* propq = NULL;
  // Fetch the HMAC implementation
  EVP_MAC* mac = EVP_MAC_fetch(library_context, "HMAC", propq);
  DevAssert(mac != NULL);

  // Create a context for the HMAC operation
  EVP_MAC_CTX* mctx = EVP_MAC_CTX_new(mac);
  DevAssert(mctx != NULL);

  // The underlying digest to be used
  OSSL_PARAM params[2] = {0};
  char digest_name[] = "SHA2-256";
  params[0] = OSSL_PARAM_construct_utf8_string(OSSL_MAC_PARAM_DIGEST, digest_name, sizeof(digest_name));
  params[1] = OSSL_PARAM_construct_end();

  // Initialise the HMAC operation
  int rc = EVP_MAC_init(mctx, key, 32, params);
  DevAssert(rc == 1);

  // Make one or more calls to process the data to be authenticated
  rc = EVP_MAC_update(mctx, data.buf, data.len);
  DevAssert(rc == 1);

  // Make one call to the final to get the MAC
  rc = EVP_MAC_final(mctx, out, &len, len);
  DevAssert(rc == 1);

  // OpenSSL free functions will ignore NULL arguments
  EVP_MAC_CTX_free(mctx);
  EVP_MAC_free(mac);
  OSSL_LIB_CTX_free(library_context);
}

#elif OPENSSL_VERSION_NUMBER >= 0x10100000L

/* code for version >= 1.1.0 and < 3.0.0 */

void sha_256_hmac(const uint8_t key[32], byte_array_t data, size_t len, uint8_t out[len])
{
  DevAssert(key != NULL);
  DevAssert(data.buf != NULL);
  DevAssert(data.len != 0);
  DevAssert(len != 0);

  HMAC_CTX* ctx = HMAC_CTX_new();

  HMAC_Init_ex(ctx, key, 32, EVP_sha256(), NULL);

  HMAC_Update(ctx, data.buf, data.len);

  HMAC_Final(ctx, out, (uint32_t*)&len);

  HMAC_CTX_free(ctx);
}

#else

/* code for version lower than 1.1.0 */

void sha_256_hmac(const uint8_t key[32], byte_array_t data, size_t len, uint8_t out[len])
{
  DevAssert(key != NULL);
  DevAssert(data.buf != NULL);
  DevAssert(data.len != 0);
  DevAssert(len != 0);

  HMAC_CTX ctx;
  HMAC_CTX_init(&ctx);

  HMAC_Init_ex(&ctx, key, 32, EVP_sha256(), NULL);

  HMAC_Update(&ctx, data.buf, data.len);

  HMAC_Final(&ctx, out, (uint32_t*)&len);

  HMAC_CTX_cleanup(&ctx);
}

#endif
