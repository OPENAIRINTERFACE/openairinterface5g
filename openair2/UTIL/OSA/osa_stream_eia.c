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
#include <math.h>

#include "assertions.h"

#include <openssl/aes.h>
#include <openssl/cmac.h>
#include <openssl/evp.h>

#include "UTIL/LOG/log.h"

#include "osa_defs.h"
#include "osa_snow3g.h"
#include "osa_internal.h"

// see spec 3GPP Confidentiality and Integrity Algorithms UEA2&UIA2. Document 1: UEA2 and UIA2 Specification. Version 1.1

/* OSA_MUL64x.
 * Input V: a 64-bit input.
 * Input c: a 64-bit input.
 * Output : a 64-bit output.
 * A 64-bit memory is allocated which is to be freed by the calling
 * function.
 * See section 4.3.2 for details.
 */
uint64_t OSA_MUL64x(uint64_t V, uint64_t c)
{
  if ( V & 0x8000000000000000 )
    return (V << 1) ^ c;
  else
    return V << 1;
}
/* OSA_MUL64xPOW.
 * Input V: a 64-bit input.
 * Input i: a positive integer.
 * Input c: a 64-bit input.
 * Output : a 64-bit output.
 * A 64-bit memory is allocated which is to be freed by the calling
function.
 * See section 4.3.3 for details.
 */
uint64_t OSA_MUL64xPOW(uint64_t V, uint32_t i, uint64_t c)
{
  if ( i == 0)
    return V;
  else
    return OSA_MUL64x( OSA_MUL64xPOW(V,i-1,c) , c);
}
/* OSA_MUL64.
 * Input V: a 64-bit input.
 * Input P: a 64-bit input.
 * Input c: a 64-bit input.
 * Output : a 64-bit output.
 * A 64-bit memory is allocated which is to be freed by the calling
 * function.
 * See section 4.3.4 for details.
 */
uint64_t OSA_MUL64(uint64_t V, uint64_t P, uint64_t c)
{
  uint64_t result = 0;
  int i = 0;

  for ( i=0; i<64; i++) {
    if( ( P>>i ) & 0x1 )
      result ^= OSA_MUL64xPOW(V,i,c);
  }

  return result;
}

/* osa_mask32bit.
* Input n: an integer in 1-32.
* Output : a 32 bit mask.
* Prepares a 32 bit mask with required number of 1 bits on the MSB side.
*/
uint32_t osa_mask32bit(int n)
{
  uint32_t mask=0x0;

  if ( n%32 == 0 )
    return 0xffffffff;

  while (n--)
    mask = (mask>>1) ^ 0x80000000;

  return mask;
}


/*!
 * @brief Create integrity cmac t for a given message.
 * @param[in] stream_cipher Structure containing various variables to setup encoding
 * @param[out] out For EIA2 the output string is 32 bits long
 */
int stream_compute_integrity_eia1(stream_cipher_t *stream_cipher, uint8_t out[4])
{
  osa_snow_3g_context_t snow_3g_context;
  uint32_t        K[4],IV[4], z[5];
  int             i=0,D;
  uint32_t        MAC_I = 0;
  uint64_t        EVAL;
  uint64_t        V;
  uint64_t        P;
  uint64_t        Q;
  uint64_t        c;
  uint64_t        M_D_2;
  int             rem_bits;
  uint32_t        mask = 0;
  uint32_t       *message;

  message = (uint32_t*)stream_cipher->message; /* To operate 32 bit message internally. */
  /* Load the Integrity Key for SNOW3G initialization as in section 4.4. */
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
  /* Prepare the Initialization Vector (IV) for SNOW3G initialization as in
    section 4.4. */
  IV[3] = (uint32_t)stream_cipher->count;
  IV[2] = ((((uint32_t)stream_cipher->bearer) & 0x0000001F) << 27);
  IV[1] = (uint32_t)(stream_cipher->count) ^ ( (uint32_t)(stream_cipher->direction) << 31 ) ;
  IV[0] = ((((uint32_t)stream_cipher->bearer) & 0x0000001F) << 27) ^ ((uint32_t)(stream_cipher->direction & 0x00000001) << 15);
  //printf ("K:\n");
  //hexprint(K, 16);
  //printf ("K[0]:%08X\n",K[0]);
  //printf ("K[1]:%08X\n",K[1]);
  //printf ("K[2]:%08X\n",K[2]);
  //printf ("K[3]:%08X\n",K[3]);

  //printf ("IV:\n");
  //hexprint(IV, 16);
  //printf ("IV[0]:%08X\n",IV[0]);
  //printf ("IV[1]:%08X\n",IV[1]);
  //printf ("IV[2]:%08X\n",IV[2]);
  //printf ("IV[3]:%08X\n",IV[3]);
  z[0] = z[1] = z[2] = z[3] = z[4] = 0;
  /* Run SNOW 3G to produce 5 keystream words z_1, z_2, z_3, z_4 and z_5. */
  osa_snow3g_initialize(K, IV, &snow_3g_context);
  osa_snow3g_generate_key_stream(5, z, &snow_3g_context);
  //printf ("z[0]:%08X\n",z[0]);
  //printf ("z[1]:%08X\n",z[1]);
  //printf ("z[2]:%08X\n",z[2]);
  //printf ("z[3]:%08X\n",z[3]);
  //printf ("z[4]:%08X\n",z[4]);
  P = ((uint64_t)z[0] << 32) | (uint64_t)z[1];
  Q = ((uint64_t)z[2] << 32) | (uint64_t)z[3];
  //printf ("P:%16lX\n",P);
  //printf ("Q:%16lX\n",Q);
  /* Calculation */
  D = ceil( stream_cipher->blength / 64.0 ) + 1;
  //printf ("D:%d\n",D);
  EVAL = 0;
  c = 0x1b;

  /* for 0 <= i <= D-3 */
  for (i=0; i<D-2; i++) {
    V = EVAL ^ ( (uint64_t)hton_int32(message[2*i]) << 32 | (uint64_t)hton_int32(message[2*i+1]) );
    EVAL = OSA_MUL64(V,P,c);
    //printf ("Mi: %16X %16X\tEVAL: %16lX\n",hton_int32(message[2*i]),hton_int32(message[2*i+1]), EVAL);
  }

  /* for D-2 */
  rem_bits = stream_cipher->blength % 64;

  if (rem_bits == 0)
    rem_bits = 64;

  mask = osa_mask32bit(rem_bits%32);

  if (rem_bits > 32) {
    M_D_2 = ( (uint64_t) hton_int32(message[2*(D-2)]) << 32 ) |
            (uint64_t) (hton_int32(message[2*(D-2)+1]) &  mask);
  } else {
    M_D_2 = ( (uint64_t) hton_int32(message[2*(D-2)]) & mask) << 32 ;
  }

  V = EVAL ^ M_D_2;
  EVAL = OSA_MUL64(V,P,c);
  /* for D-1 */
  EVAL ^= stream_cipher->blength;
  /* Multiply by Q */
  EVAL = OSA_MUL64(EVAL,Q,c);
  MAC_I = (uint32_t)(EVAL >> 32) ^ z[4];
  //printf ("MAC_I:%16X\n",MAC_I);
  MAC_I = hton_int32(MAC_I);
  memcpy(out, &MAC_I, 4);
  return 0;
}

int stream_compute_integrity_eia2(stream_cipher_t *stream_cipher, uint8_t out[4])
{
  uint8_t  *m;
  uint32_t local_count;
  size_t size = 4;

  uint8_t  data[16];

  CMAC_CTX *cmac_ctx;

  uint32_t zero_bit = 0;
  uint32_t m_length;

  DevAssert(stream_cipher != NULL);
  DevAssert(stream_cipher->key != NULL);

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
    char payload[6 * sizeof(m) + 1];
    int  index = 0;
    LOG_T(OSA, "Blength: %u, Zero bits: %u\n",
          stream_cipher->blength, zero_bit);

    for (i = 0; i < sizeof(m); i++)
      index += sprintf(&payload[index], "0x%02x ", m[i]);

    LOG_D(OSA, "Payload: %s\n", payload);
  }
#endif

  cmac_ctx = CMAC_CTX_new();

  CMAC_Init(cmac_ctx, stream_cipher->key, stream_cipher->key_length, EVP_aes_128_cbc(), NULL);
  CMAC_Update(cmac_ctx, m, m_length + 8);

  CMAC_Final(cmac_ctx, data, &size);

  CMAC_CTX_free(cmac_ctx);

  memcpy(out, data, 4);
  free(m);

  return 0;
}

int stream_compute_integrity(uint8_t algorithm, stream_cipher_t *stream_cipher, uint8_t out[4])
{
  if (algorithm == EIA1_128_ALG_ID) {
    LOG_D(OSA, "EIA1 algorithm applied for integrity\n");
    return stream_compute_integrity_eia1(stream_cipher, out);
  } else if (algorithm == EIA2_128_ALG_ID) {
    LOG_D(OSA, "EIA2 algorithm applied for integrity\n");
    return stream_compute_integrity_eia2(stream_cipher, out);
  }

  LOG_E(OSA, "Provided integrity algorithm is currently not supported = %u\n", algorithm);
  return -1;
}

int stream_check_integrity(uint8_t algorithm, stream_cipher_t *stream_cipher, uint8_t *expected)
{
  uint8_t result[4];

  if (algorithm != EIA0_ALG_ID) {
    if (stream_compute_integrity(algorithm, stream_cipher, result) != 0) {
      return -1;
    }

    if (memcmp(result, expected, 4) != 0) {
      LOG_E(OSA, "Mismatch found in integrity for algorithm %u,\n"
            "\tgot %02x.%02x.%02x.%02x, expecting %02x.%02x.%02x.%02x\n",
            algorithm, result[0], result[1], result[2], result[3], expected[0],
            expected[1], expected[2], expected[3]);
      return -1;
    }
  }

  /* Integrity verification succeeded */
  return 0;
}
