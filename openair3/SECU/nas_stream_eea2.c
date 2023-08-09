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

#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#include "aes_128_ctr.h"
#include "assertions.h"
#include "conversions.h"
#include "nas_stream_eea2.h"

void nas_stream_encrypt_eea2(nas_stream_cipher_t const *stream_cipher, uint8_t *out)
{
  DevAssert(stream_cipher != NULL);
  DevAssert(stream_cipher->key != NULL);
  DevAssert(stream_cipher->key_length == 32);
  DevAssert(stream_cipher->bearer < 32);
  DevAssert(stream_cipher->direction < 2);
  DevAssert(stream_cipher->message != NULL);
  DevAssert(stream_cipher->blength > 7);

  aes_128_t p = {0};
  memcpy(p.key, stream_cipher->key, stream_cipher->key_length);
  p.type = AES_INITIALIZATION_VECTOR_16;
  p.iv16.d.count = htonl(stream_cipher->count);
  p.iv16.d.bearer = stream_cipher->bearer;
  p.iv16.d.direction = stream_cipher->direction;

  DevAssert((stream_cipher->blength & 0x07) == 0);
  const uint32_t byte_lenght = stream_cipher->blength >> 3;
  // Precondition: out must have enough space, at least as much as the input
  const size_t len_out = byte_lenght;
  byte_array_t msg = {.buf =  stream_cipher->message, .len = byte_lenght};
  aes_128_ctr(&p, msg, len_out, out);
}
