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

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "secu_defs.h"

#include <openssl/aes.h>
#include <openssl/cmac.h>
#include <openssl/evp.h>

// test
#include <openssl/ssl.h>
#include <openssl/err.h>
#include <openssl/bio.h>

#include "assertions.h"
#include "conversions.h"

#define SECU_DEBUG

/*!
 * @brief Create integrity cmac t for a given message.
 * @param[in] stream_cipher Structure containing various variables to setup encoding
 * @param[out] out For EIA2 the output string is 32 bits long
 */
int nas_stream_encrypt_eia2(nas_stream_cipher_t *stream_cipher, uint8_t out[4])
{
  uint8_t          *m            = NULL;
  uint32_t          local_count;
  size_t            size         = 4;
  uint8_t           data[16];
  CMAC_CTX         *cmac_ctx     = NULL;
  const EVP_CIPHER *cipher       = EVP_aes_128_cbc();
  uint32_t          zero_bit     = 0;
  uint32_t          m_length;
  int               ret;

  DevAssert(stream_cipher != NULL);
  DevAssert(stream_cipher->key != NULL);
  DevAssert(stream_cipher->key_length > 0);
  DevAssert(out != NULL);

  memset(data, 0, 16);

  zero_bit = stream_cipher->blength & 0x7;

  m_length = stream_cipher->blength >> 3;

  if (zero_bit > 0)
    m_length += 1;

  local_count = hton_int32(stream_cipher->count);

  m = calloc(m_length + 8, sizeof(uint8_t));

  memcpy(&m[0], &local_count, 4);
  m[4] = ((stream_cipher->bearer & 0x1F) << 3) | ((stream_cipher->direction & 0x01) << 2);

  memcpy(&m[8], stream_cipher->message, m_length);

#if defined(SECU_DEBUG)
  {
    int i;
    printf("Byte length: %u, Zero bits: %u\nm: ", m_length + 8, zero_bit);

    for (i = 0; i < m_length + 8; i++)
      printf("%02x", m[i]);

    printf("\nKey:");

    for (i = 0; i < stream_cipher->key_length; i++)
      printf("%02x", stream_cipher->key[i]);

    printf("\nMessage:");

    for (i = 0; i < m_length; i++)
      printf("%02x", stream_cipher->message[i]);

    printf("\n");
  }
#endif

  cmac_ctx = CMAC_CTX_new();
  ret = CMAC_Init(cmac_ctx, stream_cipher->key, stream_cipher->key_length, cipher, NULL);
  ret = CMAC_Update(cmac_ctx, m, m_length + 8);
  (void)ret; /* avoid gcc warning "set but not used" */
  CMAC_Final(cmac_ctx, data, &size);
  CMAC_CTX_free(cmac_ctx);

#if defined(SECU_DEBUG)
  {
    int i;
    printf("out: ");

    for (i = 0; i < size; i++)
      printf("%02x", data[i]);

    printf("\n");
  }
#endif

  memcpy(out, data, 4);
  free(m);

  return 0;
}
