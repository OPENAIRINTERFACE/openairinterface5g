/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
    included in this distribution in the file called "COPYING". If not,
    see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

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
