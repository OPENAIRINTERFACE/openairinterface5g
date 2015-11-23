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

// #define SECU_DEBUG

int nas_stream_encrypt_eea2(nas_stream_cipher_t *stream_cipher, uint8_t *out)
{
  uint8_t m[16];
  uint32_t local_count;

  void *ctx;
  uint8_t *data;

  uint32_t zero_bit = 0;
  uint32_t byte_length;

  DevAssert(stream_cipher != NULL);
  DevAssert(out != NULL);

  zero_bit = stream_cipher->blength & 0x7;

  byte_length = stream_cipher->blength >> 3;

  if (zero_bit > 0)
    byte_length += 1;

  ctx = malloc(nettle_aes128.context_size);
  data = malloc(byte_length);

  local_count = hton_int32(stream_cipher->count);

  memset(m, 0, sizeof(m));
  memcpy(&m[0], &local_count, 4);
  m[4] = ((stream_cipher->bearer & 0x1F) << 3) |
         ((stream_cipher->direction & 0x01) << 2);
  /* Other bits are 0 */

#if defined(SECU_DEBUG)
  {
    int i;
    printf("Blength: %u, Zero bits: %u\n", stream_cipher->blength, zero_bit);

    for (i = 0; i < sizeof(m); i++)
      printf("0x%02x ", m[i]);

    printf("\n");
  }
#endif

  nettle_aes128.set_encrypt_key(ctx, stream_cipher->key_length,
                                stream_cipher->key);

  nettle_ctr_crypt(ctx, nettle_aes128.encrypt,
                   nettle_aes128.block_size, m,
                   byte_length, data, stream_cipher->message);

  if (zero_bit > 0)
    data[byte_length - 1] = data[byte_length - 1] & (uint8_t)(0xFF << (8 - zero_bit));


  memcpy(out, data, byte_length);
  free(data);
  free(ctx);

  return 0;
}
