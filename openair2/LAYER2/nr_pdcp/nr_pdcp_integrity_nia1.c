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

#include "nr_pdcp_integrity_nia1.h"

#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "openair3/SECU/secu_defs.h"
#include "openair3/SECU/key_nas_deriver.h"

void *nr_pdcp_integrity_nia1_init(unsigned char *integrity_key)
{
  nas_stream_cipher_t *ret;

  ret = calloc(1, sizeof(*ret)); if (ret == NULL) abort();
  ret->key = malloc(16); if (ret->key == NULL) abort();
  memcpy(ret->key, integrity_key, 16);
  ret->key_length = 16;   /* unused */

  return ret;
}

void nr_pdcp_integrity_nia1_integrity(void *integrity_context,
                            unsigned char *out,
                            unsigned char *buffer, int length,
                            int bearer, int count, int direction)
{
  nas_stream_cipher_t *ctx = integrity_context;

  ctx->message = buffer;
  ctx->count = count;
  ctx->bearer = bearer-1;
  ctx->direction = direction;
  ctx->blength = length * 8;

  stream_compute_integrity(EIA1_128_ALG_ID, ctx, out);
}

void nr_pdcp_integrity_nia1_free_integrity(void *integrity_context)
{
  nas_stream_cipher_t *ctx = integrity_context;
  free(ctx->key);
  free(ctx);
}
