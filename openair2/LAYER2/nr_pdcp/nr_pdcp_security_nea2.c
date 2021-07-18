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

#include "nr_pdcp_security_nea2.h"

#include <stdlib.h>
#include <string.h>
#include <nettle/nettle-meta.h>
#include <nettle/aes.h>
#include <nettle/ctr.h>

#ifndef NETTLE_VERSION_MAJOR
/* hack: include bignum.h, not version.h because version.h does not exist
 *       in old versions and bignum.h includes version.h (as of today).
 *       May completely fail to work... maybe we should skip support of old
 *       versions of nettle.
 */
#include <nettle/bignum.h>
#endif

void *nr_pdcp_security_nea2_init(unsigned char *ciphering_key)
{
  void *ctx = calloc(1, nettle_aes128.context_size);
  if (ctx == NULL) exit(1);

#if !defined(NETTLE_VERSION_MAJOR) || NETTLE_VERSION_MAJOR < 3
  nettle_aes128.set_encrypt_key(ctx, 16, ciphering_key);
#else
  nettle_aes128.set_encrypt_key(ctx, ciphering_key);
#endif

  return ctx;
}

void nr_pdcp_security_nea2_cipher(void *security_context,
                                  unsigned char *buffer, int length,
                                  int bearer, int count, int direction)
{
  unsigned char t[16];

  t[0] = (count >> 24) & 255;
  t[1] = (count >> 16) & 255;
  t[2] = (count >>  8) & 255;
  t[3] = (count      ) & 255;
  t[4] = ((bearer-1) << 3) | (direction << 2);
  memset(&t[5], 0, 16-5);

  nettle_ctr_crypt(security_context, nettle_aes128.encrypt,
                   nettle_aes128.block_size, t,
                   length, buffer, buffer);
}

void nr_pdcp_security_nea2_free_security(void *security_context)
{
  free(security_context);
}
