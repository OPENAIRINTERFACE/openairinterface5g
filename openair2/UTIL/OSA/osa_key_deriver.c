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

#include <stdint.h>

#include <nettle/hmac.h>

#include "osa_defs.h"
#include "osa_internal.h"
#include "common/utils/LOG/log.h"

static inline
void kdf(const uint8_t *s, const uint32_t s_length, const uint8_t *key,
         const uint32_t key_length, uint8_t **out, uint32_t out_length)
{
  struct hmac_sha256_ctx ctx;

  uint8_t *buffer;

  buffer = malloc(sizeof(uint8_t) * out_length);

  hmac_sha256_set_key(&ctx, key_length, key);
  hmac_sha256_update(&ctx, s_length, s);
  hmac_sha256_digest(&ctx, out_length, buffer);

  *out = buffer;
}

/*!
 * @brief Derive the keys from key and perform truncate on the generated key to
 * reduce his size to 128 bits. Definition of the derivation function can
 * be found in 3GPP TS.33401 #A.7
 * @param[in] alg_type Algorithm distinguisher
 * @param[in] alg_id Algorithm identifier.
 * Possible values are:
 *      - 0 for EIA0 algorithm (Null Integrity Protection algorithm)
 *      - 1 for 128-EIA1 SNOW 3G
 *      - 2 for 128-EIA2 AES
 * @param[in] key The top key used to derive other subkeys
 * @param[out] out Pointer to reference where output of KDF will be stored.
 * NOTE: knas is dynamically allocated by the KDF function
 */
int derive_key(algorithm_type_dist_t alg_type, uint8_t alg_id,
               const uint8_t key[32], uint8_t **out)
{
  uint8_t string[7];

  /* FC */
  string[0] = FC_ALG_KEY_DER;

  /* P0 = algorithm type distinguisher */
  string[1] = (uint8_t)(alg_type & 0xFF);

  /* L0 = length(P0) = 1 */
  string[2] = 0x00;
  string[3] = 0x01;

  /* P1 */
  string[4] = alg_id;

  /* L1 = length(P1) = 1 */
  string[5] = 0x00;
  string[6] = 0x01;

#if defined(SECU_DEBUG)
  {
    int i;
    char payload[6 * sizeof(string) + 1];
    int  index = 0;

    for (i = 0; i < sizeof(string); i++)
      index += sprintf(&payload[index], "0x%02x ", string[i]);

    LOG_D(OSA, "Key deriver input string: %s\n", payload);
  }
#endif

  kdf(string, 7, key, 32, out, 32);

  return 0;
}
/*
int derive_keNB(const uint8_t key[32], const uint32_t nas_count, uint8_t **keNB)
{
    uint8_t string[7];

    // FC
    string[0] = FC_KENB;
    // P0 = Uplink NAS count
    string[1] = (nas_count & 0xff000000) >> 24;
    string[2] = (nas_count & 0x00ff0000) >> 16;
    string[3] = (nas_count & 0x0000ff00) >> 8;
    string[4] = (nas_count & 0x000000ff);

    // Length of NAS count
    string[5] = 0x00;
    string[6] = 0x04;

#if defined(SECU_DEBUG)
    {
        int i;
        char payload[6 * sizeof(string) + 1];
        int  index = 0;

        for (i = 0; i < sizeof(string); i++)
            index += sprintf(&payload[index], "0x%02x ", string[i]);
        LOG_D(OSA, "KeNB deriver input string: %s\n", payload);
    }
#endif

    kdf(string, 7, key, 32, keNB, 32);

    return 0;
}
*/
