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

#include "aes_128_ecb.h"
#include "common/utils/assertions.h"
#include <openssl/aes.h>
#include <openssl/err.h>
#include <openssl/evp.h>

void aes_128_ecb(const aes_128_t* k_iv, byte_array_t msg, size_t len_out, uint8_t out[len_out])
{
  DevAssert(k_iv != NULL);
  DevAssert(k_iv->type == NONE_INITIALIZATION_VECTOR);

  // Create and initialise the context
  EVP_CIPHER_CTX* ctx = EVP_CIPHER_CTX_new();
  DevAssert(ctx != NULL);

  // Initialise the encryption operation.
  int rc = EVP_EncryptInit_ex(ctx, EVP_aes_128_ecb(), NULL, k_iv->key, NULL);
  DevAssert(rc == 1);

  int len_ev = 0;
  rc = EVP_EncryptUpdate(ctx, out, &len_ev, msg.buf, msg.len);
  AssertFatal(!(len_ev > len_out), "Buffer overflow");

  // Finalise the encryption. Normally ciphertext bytes may be written at
  // this stage, but this does not occur in GCM mode
  rc = EVP_EncryptFinal_ex(ctx, out + len_ev, &len_ev);
  DevAssert(rc == 1);

  // Get the tag
  // rc == EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag))
  EVP_CIPHER_CTX_free(ctx);
}
