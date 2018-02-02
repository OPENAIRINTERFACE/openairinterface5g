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

#include <nettle/nettle-meta.h>
#include <nettle/aes.h>
#include <nettle/ctr.h>

#include "UTIL/LOG/log.h"

#include "assertions.h"
#include "osa_defs.h"
#include "osa_snow3g.h"
#include "osa_internal.h"

int stream_encrypt_eea0(stream_cipher_t *stream_cipher, uint8_t **out)
{
  uint8_t *data = NULL;

  uint32_t byte_length;

  DevAssert(stream_cipher != NULL);
  DevAssert(out != NULL);

  LOG_D(OSA, "Entering stream_encrypt_eea0, bits length %u, bearer %u, "
        "count %u, direction %s\n", stream_cipher->blength,
        stream_cipher->bearer, stream_cipher->count, stream_cipher->direction == SECU_DIRECTION_DOWNLINK ?
        "Downlink" : "Uplink");

  byte_length = (stream_cipher->blength + 7) >> 3;

  if (*out == NULL) {
    /* User did not provide output buffer */
    data = malloc(byte_length);
    *out = data;
  } else {
    data = *out;
  }

  memcpy (data, stream_cipher->message, byte_length);

  return 0;
}

int stream_encrypt_eea1(stream_cipher_t *stream_cipher, uint8_t **out)
{
  osa_snow_3g_context_t snow_3g_context;
  int       n ;
  int       i           = 0;
  uint32_t  zero_bit    = 0;
  uint32_t *KS;
  uint32_t  K[4],IV[4];

  DevAssert(stream_cipher != NULL);
  DevAssert(stream_cipher->key != NULL);
  DevAssert(stream_cipher->key_length == 16);
  DevAssert(out != NULL);

  n = ( stream_cipher->blength + 31 ) / 32;
  zero_bit = stream_cipher->blength & 0x7;

  memset(&snow_3g_context, 0, sizeof(snow_3g_context));
  /*Initialisation*/
  /* Load the confidentiality key for SNOW 3G initialization as in section
  3.4. */
  memcpy(K+3,stream_cipher->key+0,4); /*K[3] = key[0]; we assume
  K[3]=key[0]||key[1]||...||key[31] , with key[0] the
  * most important bit of key*/
  memcpy(K+2,stream_cipher->key+4,4); /*K[2] = key[1];*/
  memcpy(K+1,stream_cipher->key+8,4); /*K[1] = key[2];*/
  memcpy(K+0,stream_cipher->key+12,4); /*K[0] = key[3]; we assume
  K[0]=key[96]||key[97]||...||key[127] , with key[127] the
  * least important bit of key*/
  K[3] = hton_int32(K[3]);
  K[2] = hton_int32(K[2]);
  K[1] = hton_int32(K[1]);
  K[0] = hton_int32(K[0]);
  /* Prepare the initialization vector (IV) for SNOW 3G initialization as in
  section 3.4. */
  IV[3] = stream_cipher->count;
  IV[2] = ((((uint32_t)stream_cipher->bearer) << 3) | ((((uint32_t)stream_cipher->direction) & 0x1) << 2)) << 24;
  IV[1] = IV[3];
  IV[0] = IV[2];

  /* Run SNOW 3G algorithm to generate sequence of key stream bits KS*/
  osa_snow3g_initialize(K, IV, &snow_3g_context);
  KS = (uint32_t *)malloc(4*n);
  osa_snow3g_generate_key_stream(n,(uint32_t*)KS, &snow_3g_context);

  if (zero_bit > 0) {

    KS[n - 1] = KS[n - 1] & (uint32_t)(0xFFFFFFFF << (8 - zero_bit));
  }

  for (i=0; i<n; i++) {
    KS[i] = hton_int32(KS[i]);
  }

  /* Exclusive-OR the input data with keystream to generate the output bit
  stream */
  for (i=0; i<n*4; i++) {
    stream_cipher->message[i] ^= *(((uint8_t*)KS)+i);
  }

  if (zero_bit > 0) {
    int ceil_index = (stream_cipher->blength+7) >> 3;
    stream_cipher->message[ceil_index - 1] = stream_cipher->message[ceil_index - 1] & (uint8_t)(0xFF << (8 - zero_bit));
  }

  free(KS);
  *out = stream_cipher->message;
  return 0;
}

int stream_encrypt_eea2(stream_cipher_t *stream_cipher, uint8_t **out)
{
  uint8_t m[16];
  uint32_t local_count;

  void *ctx;
  uint8_t *data = NULL;

  uint32_t zero_bit = 0;
  uint32_t byte_length;

  DevAssert(stream_cipher != NULL);
  DevAssert(out != NULL);

  LOG_D(OSA, "Entering stream_encrypt_eea2, bits length %u, bearer %u, "
        "count %u, direction %s\n", stream_cipher->blength,
        stream_cipher->bearer, stream_cipher->count, stream_cipher->direction == SECU_DIRECTION_DOWNLINK ?
        "Downlink" : "Uplink");

  zero_bit = stream_cipher->blength & 0x7;

  byte_length = stream_cipher->blength >> 3;

  if (zero_bit > 0)
    byte_length += 1;

  ctx = malloc(nettle_aes128.context_size);

  if (*out == NULL) {
    /* User provided output buffer */
    data = malloc(byte_length);
    *out = data;
  } else {
    data = *out;
  }

  local_count = hton_int32(stream_cipher->count);

  memset(m, 0, sizeof(m));
  memcpy(&m[0], &local_count, 4);
  m[4] = ((stream_cipher->bearer & 0x1F) << 3) |
         ((stream_cipher->direction & 0x01) << 2);
  /* Other bits are 0 */

#if defined(SECU_DEBUG)
  {
    int i;
    char payload[6 * byte_length + 1];
    int  index = 0;

    for (i = 0; i < byte_length; i++)
      index += sprintf(&payload[index], "0x%02x ", stream_cipher->message[i]);

    LOG_D(OSA, "Original message: %s\n", payload);
  }
#endif

#if NETTLE_VERSION_MAJOR < 3
  nettle_aes128.set_encrypt_key(ctx, stream_cipher->key_length,
                                stream_cipher->key);
#else
  nettle_aes128.set_encrypt_key(ctx, 
                                stream_cipher->key);
#endif

  nettle_ctr_crypt(ctx, nettle_aes128.encrypt,
                   nettle_aes128.block_size, m,
                   byte_length, data, stream_cipher->message);

  if (zero_bit > 0)
    data[byte_length - 1] = data[byte_length - 1] & (uint8_t)(0xFF << (8 - zero_bit));

  free(ctx);

#if defined(SECU_DEBUG)
  {
    int i;
    char payload[6 * byte_length + 1];
    int  index = 0;

    for (i = 0; i < byte_length; i++)
      index += sprintf(&payload[index], "0x%02x ", data[i]);

    LOG_D(OSA, "Encrypted message: %s\n", payload);
  }
#endif

  return 0;
}

int stream_encrypt(uint8_t algorithm, stream_cipher_t *stream_cipher, uint8_t **out)
{
  if (algorithm == EEA0_ALG_ID) {
    return stream_encrypt_eea0(stream_cipher, out);
  } else if (algorithm == EEA1_128_ALG_ID) {
    return stream_encrypt_eea1(stream_cipher, out);
  } else if (algorithm == EEA2_128_ALG_ID) {
    return stream_encrypt_eea2(stream_cipher, out);
  }

  LOG_E(OSA, "Provided encryption algorithm is currently not supported = %u\n", algorithm);
  return -1;
}
