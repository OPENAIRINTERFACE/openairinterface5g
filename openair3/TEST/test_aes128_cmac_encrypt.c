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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <unistd.h>

#include "test_util.h"

#include "secu_defs.h"

#include <openssl/aes.h>
#include <openssl/cmac.h>
#include <openssl/evp.h>

static CMAC_CTX *ctx;

static
void test_cmac(const uint8_t *key, unsigned key_length, const uint8_t *message,
               unsigned length, const uint8_t *expect, unsigned expected_length)
{
  size_t size;
  uint8_t result[16];

  ctx = CMAC_CTX_new();

  CMAC_Init(ctx, key, key_length, EVP_aes_128_cbc(), NULL);
  CMAC_Update(ctx, message, length);
  CMAC_Final(ctx, result, &size);

  CMAC_CTX_free(ctx);

  if (compare_buffer(result, size, expect, expected_length) != 0) {
    fail("test_cmac");
  }
}

void doit (void)
{
  /* SP300-38B #D.1 */

  /* Example 1 */
  test_cmac(
    HL("2b7e151628aed2a6abf7158809cf4f3c"),
    HL(""),
    HL("bb1d6929e95937287fa37d129b756746"));
  /* Example 2 */
  test_cmac(
    HL("2b7e151628aed2a6abf7158809cf4f3c"),
    HL("6bc1bee22e409f96e93d7e117393172a"),
    HL("070a16b46b4d4144f79bdd9dd04a287c"));
  /* Example 3 */
  test_cmac(
    HL("2b7e151628aed2a6abf7158809cf4f3c"),
    HL("6bc1bee22e409f96e93d7e117393172a"
       "ae2d8a571e03ac9c9eb76fac45af8e51"
       "30c81c46a35ce411"),
    HL("dfa66747de9ae63030ca32611497c827"));
  /* Example 4 */
  test_cmac(
    HL("2b7e151628aed2a6abf7158809cf4f3c"),
    HL("6bc1bee22e409f96e93d7e117393172a"
       "ae2d8a571e03ac9c9eb76fac45af8e51"
       "30c81c46a35ce411e5fbc1191a0a52ef"
       "f69f2445df4f9b17ad2b417be66c3710"),
    HL("51f0bebf7e3b9d92fc49741779363cfe"));
}
