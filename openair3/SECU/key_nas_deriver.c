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

#include "aes_128.h"
#include "aes_128_cbc_cmac.h"

#include "common/utils/assertions.h"
#include "common/utils/LOG/log.h"
#include "kdf.h"
#include "key_nas_deriver.h"
#include "nas_stream_eea1.h"
#include "nas_stream_eea2.h"
#include "nas_stream_eia1.h"
#include "nas_stream_eia2.h"
#include "secu_defs.h"
#include "snow3g.h"
#include <arpa/inet.h>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>

#define FC_KENB (0x11)
#define FC_NH (0x12)
#define FC_KENB_STAR (0x13)
/* 33401 #A.7 Algorithm for key derivation function.
 * This FC should be used for:
 * - NAS Encryption algorithm
 * - NAS Integrity algorithm
 * - RRC Encryption algorithm
 * - RRC Integrity algorithm
 * - User Plane Encryption algorithm
 */
#define FC_ALG_KEY_DER (0x15)
#define FC_KASME_TO_CK (0x16)

#define NR_FC_ALG_KEY_DER (0x69)
#define NR_FC_ALG_KEY_NG_RAN_STAR_DER (0x70)

// #define SECU_DEBUG 1

static void derive_key_common(algorithm_type_dist_t alg_type, uint8_t alg_id, const uint8_t kasme[32], uint8_t FC, uint8_t out[32])
{
  uint8_t s[7];
#if defined(SECU_DEBUG)
  int i;
#endif

  /* FC */
  s[0] = FC;

  /* P0 = algorithm type distinguisher */
  s[1] = (uint8_t)(alg_type & 0xFF);

  /* L0 = length(P0) = 1 */
  s[2] = 0x00;
  s[3] = 0x01;

  /* P1 */
  s[4] = alg_id;

  /* L1 = length(P1) = 1 */
  s[5] = 0x00;
  s[6] = 0x01;

#if defined(SECU_DEBUG)
  printf("%s FC %d nas_alg_type distinguisher %d nas_enc_alg_identity %d\n", __FUNCTION__, FC_ALG_KEY_DER, alg_type, alg_id);

  for (i = 0; i < 7; i ++) {
    printf("0x%02x ", s[i]);
  }

  printf("\n");
#endif

  byte_array_t data = {.len = 7, .buf = s};
  kdf(kasme, data, 32, out);
}

/*!
 * @brief Derive the kNASenc from kasme and perform truncate on the generated key to
 * reduce his size to 128 bits. Definition of the derivation function can
 * be found in 3GPP TS.33401 #A.7
 * @param[in] nas_alg_type NAS algorithm distinguisher
 * @param[in] nas_enc_alg_id NAS encryption/integrity algorithm identifier.
 * Possible values are:
 *      - 0 for EIA0 algorithm (Null Integrity Protection algorithm)
 *      - 1 for 128-EIA1 SNOW 3G
 *      - 2 for 128-EIA2 AES
 * @param[in] kasme Key for MME as provided by AUC
 * @param[out] knas Pointer to reference where output of KDF will be stored.
 */
void derive_key_nas(algorithm_type_dist_t alg_type, uint8_t alg_id, const uint8_t kasme[32], uint8_t knas[32])
{
  derive_key_common(alg_type, alg_id, kasme, FC_ALG_KEY_DER, knas);
}

void nr_derive_key(algorithm_type_dist_t alg_type, uint8_t alg_id, const uint8_t key[32], uint8_t out[16])
{
  uint8_t tmp[32] = {0};
  derive_key_common(alg_type, alg_id, key, NR_FC_ALG_KEY_DER, tmp);

  // Only the last 16 bytes are needed in 5G
  memcpy(out, &tmp[16], 16);
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

#ifndef NAS_UE
void derive_keNB(const uint8_t kasme[32], const uint32_t nas_count, uint8_t *keNB)
{
  uint8_t s[7] = {0};

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

  byte_array_t data = {.buf = s, .len = 7};
  kdf(kasme, data, 32, keNB);
}
#endif

void derive_keNB_star(const uint8_t *kenb_32,
                      const uint16_t pci,
                      const uint32_t earfcn_dl,
                      const bool is_rel8_only,
                      uint8_t *kenb_star)
{
  // see 33.401 section A.5 KeNB* derivation function
  uint8_t s[10] = {0};
  byte_array_t data = {.buf = s};
  // FC = 0x13
  s[0] = FC_KENB_STAR;
  // P0 = PCI (target physical cell id)
  s[1] = (pci & 0x0000ff00) >> 8;
  s[2] = (pci & 0x000000ff);
  // L0 = length of PCI (i.e. 0x00 0x02)
  s[3] = 0x00;
  s[4] = 0x02;
  // P1 = EARFCN-DL (target physical cell downlink frequency)
  if (is_rel8_only) {
    s[5] = (earfcn_dl & 0x0000ff00) >> 8;
    s[6] = (earfcn_dl & 0x000000ff);
    s[7] = 0x00;
    s[8] = 0x02;
    data.len = 9;
  } else {
    s[5] = (earfcn_dl & 0x00ff0000) >> 16;
    s[6] = (earfcn_dl & 0x0000ff00) >> 8;
    s[7] = (earfcn_dl & 0x000000ff);
    s[8] = 0x00;
    s[9] = 0x03;
    data.len = 10;
  }

  kdf(kenb_32, data, 32, kenb_star);

  // L1 length of EARFCN-DL (i.e. L1 = 0x00 0x02 if EARFCN-DL is between 0 and 65535, and L1 = 0x00 0x03 if EARFCN-DL is between
  // 65536 and 262143) NOTE: The length of EARFCN-DL cannot be generally set to 3 bytes for backward compatibility reasons: A Rel-8
  // entity (UE or eNB) would always assume an input parameter length of 2 bytes for the EARFCN-DL. This
  // would lead to different derived keys if another entity assumed an input parameter length of 3 bytes for the
  // EARFCN-DL.
}

void nr_derive_key_ng_ran_star(uint16_t pci, uint64_t nr_arfcn_dl, const uint8_t key[32], uint8_t *key_ng_ran_star)
{
  uint8_t s[10] = {0};

  /* FC */
  s[0] = NR_FC_ALG_KEY_NG_RAN_STAR_DER;

  /* P0 = PCI */
  s[1] = (pci & 0x0000ff00) >> 8;
  s[2] = (pci & 0x000000ff);

  /* L0 = length(P0) = 2 */
  s[3] = 0x00;
  s[4] = 0x02;

  /* P1 = NR ARFCN */
  s[5] = (nr_arfcn_dl & 0x00ff0000) >> 16;
  s[6] = (nr_arfcn_dl & 0x0000ff00) >> 8;
  s[7] = (nr_arfcn_dl & 0x000000ff);

  /* L1 = length(P1) = 3 */
  s[8] = 0x00;
  s[9] = 0x03;

  byte_array_t data = {.buf = s, .len = 10};
  const uint32_t len_key = 32;
  kdf(key, data, len_key, key_ng_ran_star);
}

void derive_skgNB(const uint8_t *keNB, const uint16_t sk_counter, uint8_t *skgNB)
{
  uint8_t s[5] = {0};

  /* FC is 0x1c (see 3gpp 33.401 annex A.15) */
  s[0] = 0x1c;

  /* put sk_counter */
  s[1] = (sk_counter >> 8) & 0xff;
  s[2] = sk_counter & 0xff;

  /* put length of sk_counter (2) */
  s[3] = 0x00;
  s[4] = 0x02;

  byte_array_t data = {.buf = s, .len = 5};
  kdf(keNB, data, 32, skgNB);
}
