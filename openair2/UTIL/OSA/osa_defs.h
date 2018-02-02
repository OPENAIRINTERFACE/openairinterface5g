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

/*!
 * \file secu_defs.h
 * \brief Openair security algorithms functions implementing 3GPP TS 33.401
 * \note HMAC computations are done by nettle/openssl library
 * \author Sebastien ROUX
 * \date 2013
 * \version 0.1
 * @ingroup security
*/

#ifndef SECU_DEFS_H_
#define SECU_DEFS_H_

#define EIA0_ALG_ID     0x00
#define EIA1_128_ALG_ID 0x01
#define EIA2_128_ALG_ID 0x02

#define EEA0_ALG_ID     EIA0_ALG_ID
#define EEA1_128_ALG_ID EIA1_128_ALG_ID
#define EEA2_128_ALG_ID EIA2_128_ALG_ID

#define SECU_DIRECTION_UPLINK   0
#define SECU_DIRECTION_DOWNLINK 1

typedef enum {
  NAS_ENC_ALG = 0x01,
  NAS_INT_ALG = 0x02,
  RRC_ENC_ALG = 0x03,
  RRC_INT_ALG = 0x04,
  UP_ENC_ALG  = 0x05
} algorithm_type_dist_t;

//int derive_keNB(const uint8_t kasme[32], const uint32_t nas_count, uint8_t **keNB);

int derive_key(algorithm_type_dist_t nas_alg_type, uint8_t nas_enc_alg_id,
               const uint8_t key[32], uint8_t **out);

//#define derive_key_nas_enc(aLGiD, kEY, kNAS)    derive_key(NAS_ENC_ALG, aLGiD, kEY, kNAS)

//#define derive_key_nas_int(aLGiD, kEY, kNAS)    derive_key(NAS_INT_ALG, aLGiD, kEY, kNAS)

#define derive_key_rrc_enc(aLGiD, kEY, kNAS)  \
    derive_key(RRC_ENC_ALG, aLGiD, kEY, kNAS)

#define derive_key_rrc_int(aLGiD, kEY, kNAS)  \
    derive_key(RRC_INT_ALG, aLGiD, kEY, kNAS)

#define derive_key_up_enc(aLGiD, kEY, kNAS)  \
    derive_key(UP_ENC_ALG, aLGiD, kEY, kNAS)

#define derive_key_up_int(aLGiD, kEY, kNAS)  \
    derive_key(UP_INT_ALG, aLGiD, kEY, kNAS)

typedef struct {
  uint8_t  *key;
  uint32_t  key_length;
  uint32_t  count;
  uint8_t   bearer;
  uint8_t   direction;
  uint8_t  *message;
  /* length in bits */
  uint32_t  blength;
} stream_cipher_t;

/*!
 * @brief Encrypt/Decrypt a block of data based on the provided algorithm
 * @param[in] algorithm Algorithm used to encrypt the data
 *      Possible values are:
 *      - EIA0_ALG_ID for NULL encryption
 *      - EIA1_128_ALG_ID for SNOW-3G encryption (not avalaible right now)
 *      - EIA2_128_ALG_ID for 128 bits AES LTE encryption
 * @param[in] stream_cipher All parameters used to compute the encrypted block of data
 * @param[out] out The encrypted block of data dynamically allocated
 * @return 0 if everything OK, -1 if something failed
 */
int stream_encrypt(uint8_t algorithm, stream_cipher_t *stream_cipher, uint8_t **out);
#define stream_decrypt stream_encrypt

int stream_compute_integrity(uint8_t algorithm, stream_cipher_t *stream_cipher, uint8_t out[4]);

/*!
 * @brief Decrypt a block of data based on the provided algorithm
 * @param[in] algorithm Algorithm used to encrypt the data
 *      Possible values are:
 *      - EIA0_ALG_ID for NULL encryption
 *      - EIA1_128_ALG_ID for SNOW-3G encryption (not avalaible right now)
 *      - EIA2_128_ALG_ID for 128 bits AES LTE encryption
 * @param[in] stream_cipher All parameters used to compute the decrypted block of data
 * @param[out] out The decrypted block of data dynamically allocated
 * @return 0 if everything OK, -1 if something failed
 */
int stream_decrypt(uint8_t algorithm, stream_cipher_t *stream_cipher, uint8_t **out);

int stream_check_integrity(uint8_t algorithm, stream_cipher_t *stream_cipher, uint8_t *expected);
#undef SECU_DEBUG

#endif /* SECU_DEFS_H_ */
