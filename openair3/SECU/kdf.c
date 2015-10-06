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
#include <stdint.h>
#include <string.h>

#include <nettle/hmac.h>

#include "security_types.h"
#include "secu_defs.h"

void kdf(const uint8_t *key, uint16_t key_len, uint8_t *s, uint16_t s_len, uint8_t *out,
         uint16_t out_len)
{
  struct hmac_sha256_ctx ctx;

  memset(&ctx, 0, sizeof(ctx));

  hmac_sha256_set_key(&ctx, key_len, key);
  hmac_sha256_update(&ctx, s_len, s);
  hmac_sha256_digest(&ctx, out_len, out);
}

#ifndef NAS_UE
int derive_keNB(const uint8_t kasme[32], const uint32_t nas_count, uint8_t *keNB)
{
  uint8_t s[7];

  // FC
  s[0] = FC_KENB;
  // P0 = Uplink NAS count
  s[1] = (nas_count & 0xff000000) >> 24;
  s[2] = (nas_count & 0x00ff0000) >> 16;
  s[3] = (nas_count & 0x0000ff00) >> 8;
  s[4] = (nas_count & 0x000000ff);

  // Length of NAS count
  s[5] = 0x00;
  s[6] = 0x04;

  kdf(kasme, 32, s, 7, keNB, 32);

  return 0;
}
#endif

