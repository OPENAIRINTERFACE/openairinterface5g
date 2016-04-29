/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include <nettle/nettle-meta.h>
#include <nettle/aes.h>
#include <nettle/ctr.h>

#include "assertions.h"
#include "conversions.h"
#include "secu_defs.h"
#include "snow3g.h"

// #define SECU_DEBUG


int nas_stream_encrypt_eea1(nas_stream_cipher_t *stream_cipher, uint8_t *out)
{
  snow_3g_context_t snow_3g_context;
  int       n ;
  int       i           = 0;
  uint32_t  zero_bit    = 0;
  //uint32_t  byte_length;
  uint32_t *KS;
  uint32_t  K[4],IV[4];

  DevAssert(stream_cipher != NULL);
  DevAssert(stream_cipher->key != NULL);
  DevAssert(stream_cipher->key_length == 16);
  DevAssert(out != NULL);

  n = ( stream_cipher->blength + 31 ) / 32;
  zero_bit = stream_cipher->blength & 0x7;
  //byte_length = stream_cipher->blength >> 3;

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
  snow3g_initialize(K, IV, &snow_3g_context);
  KS = (uint32_t *)malloc(4*n);
  snow3g_generate_key_stream(n,(uint32_t*)KS, &snow_3g_context);

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

  int ceil_index = 0;

  if (zero_bit > 0) {
    ceil_index = (stream_cipher->blength+7) >> 3;
    stream_cipher->message[ceil_index - 1] = stream_cipher->message[ceil_index - 1] & (uint8_t)(0xFF << (8 - zero_bit));
  }

  free(KS);
  memcpy(out, stream_cipher->message, n*4);

  if (zero_bit > 0) {
    out[ceil_index - 1] = stream_cipher->message[ceil_index - 1];
  }

  return 0;
}
