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

#include <stdbool.h>
#include <stdint.h>

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

/*!
 * @brief Encrypt/Decrypt a block of data based on the provided algorithm
 * @param[in] algorithm Algorithm used to encrypt the data
 *      Possible values are:
 *      - EIA0_ALG_ID for NULL encryption
 *      - EIA1_128_ALG_ID for SNOW-3G encryption (not avalaible right now)
 *      - EIA2_128_ALG_ID for 128 bits AES LTE encryption
 * @param[in] stream_cipher All parameters used to compute the encrypted block of data
 * @param[out] out The encrypted block of data
 */

typedef enum {
  EIA0_ALG_ID = 0x00,
  EIA1_128_ALG_ID = 0x01,
  EIA2_128_ALG_ID = 0x02,
  END_EIA0_ALG_ID,
} eia_alg_id_e;

void stream_compute_integrity(eia_alg_id_e alg, nas_stream_cipher_t const *stream_cipher, uint8_t out[4]);

typedef enum {
  EEA0_ALG_ID = 0x00,
  EEA1_128_ALG_ID = 0x01,
  EEA2_128_ALG_ID = 0x02,

  END_EEA_ALG_ID
} eea_alg_id_e;

void stream_compute_encrypt(eea_alg_id_e alg, nas_stream_cipher_t const *stream_cipher, uint8_t *out);

#endif /* SECU_DEFS_H_ */
