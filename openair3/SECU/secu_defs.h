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

#ifndef SECU_DEFS_H_
#define SECU_DEFS_H_

#include "security_types.h"
#include <stdbool.h>

#define EIA0_ALG_ID     0x00
#define EIA1_128_ALG_ID 0x01
#define EIA2_128_ALG_ID 0x02

#define EEA0_ALG_ID     0x00
#define EEA1_128_ALG_ID 0x01
#define EEA2_128_ALG_ID 0x02

#define SECU_DIRECTION_UPLINK   0
#define SECU_DIRECTION_DOWNLINK 1

void kdf(const uint8_t *key,
         uint16_t key_len,
         uint8_t *s,
         uint16_t s_len,
         uint8_t *out,
         uint16_t out_len);

int derive_keNB(const uint8_t kasme[32], const uint32_t nas_count, uint8_t *keNB);

int derive_keNB_star(const uint8_t *kenb_32, const uint16_t pci, const uint32_t earfcn_dl,
                      const bool is_rel8_only, uint8_t * kenb_star);

int derive_key_nas(algorithm_type_dist_t nas_alg_type, uint8_t nas_enc_alg_id,
                   const uint8_t kasme[32], uint8_t *knas);

#define derive_key_nas_enc(aLGiD, kASME, kNAS)  \
    derive_key_nas(NAS_ENC_ALG, aLGiD, kASME, kNAS)

#define derive_key_nas_int(aLGiD, kASME, kNAS)  \
    derive_key_nas(NAS_INT_ALG, aLGiD, kASME, kNAS)

#define derive_key_rrc_enc(aLGiD, kASME, kNAS)  \
    derive_key_nas(RRC_ENC_ALG, aLGiD, kASME, kNAS)

#define derive_key_rrc_int(aLGiD, kASME, kNAS)  \
    derive_key_nas(RRC_INT_ALG, aLGiD, kASME, kNAS)

#define derive_key_up_enc(aLGiD, kASME, kNAS)  \
    derive_key_nas(UP_ENC_ALG, aLGiD, kASME, kNAS)

#define derive_key_up_int(aLGiD, kASME, kNAS)  \
    derive_key_nas(UP_INT_ALG, aLGiD, kASME, kNAS)

#define SECU_DIRECTION_UPLINK   0
#define SECU_DIRECTION_DOWNLINK 1

typedef struct {
  uint8_t *key;
  uint32_t key_length;
  uint32_t count;
  uint8_t  bearer;
  uint8_t  direction;
  uint8_t  *message;
  /* length in bits */
  uint32_t  blength;
} nas_stream_cipher_t;

int nas_stream_encrypt_eea1(nas_stream_cipher_t *stream_cipher, uint8_t *out);

int nas_stream_encrypt_eia1(nas_stream_cipher_t *stream_cipher, uint8_t out[4]);

int nas_stream_encrypt_eea2(nas_stream_cipher_t *stream_cipher, uint8_t *out);

int nas_stream_encrypt_eia2(nas_stream_cipher_t *stream_cipher, uint8_t out[4]);

#undef SECU_DEBUG

#endif /* SECU_DEFS_H_ */
