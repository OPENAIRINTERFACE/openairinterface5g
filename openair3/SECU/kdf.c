/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

