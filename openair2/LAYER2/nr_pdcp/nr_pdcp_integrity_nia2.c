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

#include "nr_pdcp_integrity_nia2.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <openssl/cmac.h>

void *nr_pdcp_integrity_nia2_init(unsigned char *integrity_key)
{
  CMAC_CTX *ctx;

  ctx = CMAC_CTX_new();
  if (ctx == NULL) abort();

  CMAC_Init(ctx, integrity_key, 16, EVP_aes_128_cbc(), NULL);

  return ctx;
}

static void compute_t(unsigned char *t, uint32_t count, int bearer,
                      int direction)
{
  t[0] = (count >> 24) & 255;
  t[1] = (count >> 16) & 255;
  t[2] = (count >>  8) & 255;
  t[3] = (count      ) & 255;
  t[4] = ((bearer-1) << 3) | (direction << 2);
  memset(&t[5], 0, 8-5);
}

void nr_pdcp_integrity_nia2_integrity(void *integrity_context,
                            unsigned char *out,
                            unsigned char *buffer, int length,
                            int bearer, int count, int direction)
{
  CMAC_CTX *ctx = integrity_context;
  unsigned char t[8];
  unsigned char mac[16];
  size_t maclen;

  /* see 33.401 B.2.3 for the input to 128-EIA2
   * (which is identical to 128-NIA2, see 33.501 D.3.1.3) */
  compute_t(t, count, bearer, direction);

  CMAC_Init(ctx, NULL, 0, NULL, NULL);
  CMAC_Update(ctx, t, 8);
  CMAC_Update(ctx, buffer, length);
  CMAC_Final(ctx, mac, &maclen);

  /* AES CMAC (RFC 4493) outputs 128 bits but NR PDCP PDUs have a MAC-I of
   * 32 bits (see 38.323 6.2). RFC 4493 2.1 says to truncate most significant
   * bit first (so seems to say 33.401 B.2.3)
   */
  memcpy(out, mac, 4);
}

void nr_pdcp_integrity_nia2_free_integrity(void *integrity_context)
{
  CMAC_CTX *ctx = integrity_context;
  CMAC_CTX_free(ctx);
}
