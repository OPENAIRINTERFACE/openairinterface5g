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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#if !defined(HAVE_UINT128_T)
# include <gmp.h>
#endif

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>

#if 0
#include "queue.h"
#endif

#ifndef SECURITY_TYPES_H_
#define SECURITY_TYPES_H_

#define RAND_LENGTH_BITS       (128)
#define RAND_LENGTH_OCTETS     (RAND_LENGTH_BITS/8)
#define AUTH_KEY_LENGTH_BITS   (128)
#define AUTH_KEY_LENGTH_OCTETS (AUTH_KEY_LENGTH_BITS/8)
#define KASME_LENGTH_BITS      (256)
#define KASME_LENGTH_OCTETS    (KASME_LENGTH_BITS/8)
/* In OCTETS */
#define XRES_LENGTH_MIN        (4)
#define XRES_LENGTH_MAX        (16)
#define AUTN_LENGTH_BITS       (128)
#define AUTN_LENGTH_OCTETS     (AUTN_LENGTH_BITS/8)

/* Some methods to convert a string to an int64_t */
/*
#define STRING_TO_64BITS(sTRING, cONTAINER)    \
    sscanf(sTRING, "%" SCN64, cONTAINER)
#define STRING_TO_U64BITS(sTRING, cONTAINER)    \
    sscanf(sTRING, "%" SCNu64, cONTAINER)
*/

/* Converts a string to 128 bits gmplib integer holder */
# define STRING_TO_XBITS(sTRING, lENGTH, cONTAINER, rET)        \
do {                                                            \
    memcpy(cONTAINER, sTRING, lENGTH);                          \
    rET = 0;                                                    \
} while(0)

# define STRING_TO_128BITS(sTRING, cONTAINER, rET)  \
STRING_TO_XBITS(sTRING, 16, cONTAINER, rET)

# define STRING_TO_256BITS(sTRING, cONTAINER, rET)  \
STRING_TO_XBITS(sTRING, 32, cONTAINER, rET)

# define STRING_TO_RAND      STRING_TO_128BITS
# define STRING_TO_AUTH_KEY  STRING_TO_128BITS
# define STRING_TO_AUTH_RES  STRING_TO_128BITS
# define STRING_TO_AUTN      STRING_TO_128BITS
# define STRING_TO_KASME     STRING_TO_256BITS
# define STRING_TO_XRES(sTRING, lENGTH, cONTAINER, rET)         \
do {                                                            \
    STRING_TO_XBITS(sTRING, lENGTH, (cONTAINER)->data, rET);    \
    if (rET != -1)                                              \
        (cONTAINER)->size = lENGTH;                             \
} while(0)

/* RES amd XRES can have a variable length of 4-16 octets */
typedef struct {
  uint8_t size;
  uint8_t data[XRES_LENGTH_MAX];
} res_t;

#define FORMAT_128BITS "%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x,%02x"
#define RAND_FORMAT  FORMAT_128BITS
#define AUTN_FORMAT  FORMAT_128BITS
#define KASME_FORMAT FORMAT_128BITS

#define DISPLAY_128BITS(bUFFER) \
bUFFER[0], bUFFER[1], bUFFER[2], bUFFER[3], bUFFER[4], bUFFER[5], bUFFER[6], bUFFER[7],  \
bUFFER[8], bUFFER[9], bUFFER[10], bUFFER[11], bUFFER[12], bUFFER[13], bUFFER[14], bUFFER[15]
#define DISPLAY_128BITS_2(bUFFER) \
bUFFER[16], bUFFER[17], bUFFER[18], bUFFER[19], bUFFER[20], bUFFER[21], bUFFER[22], bUFFER[23],  \
bUFFER[24], bUFFER[25], bUFFER[26], bUFFER[27], bUFFER[28], bUFFER[29], bUFFER[30], bUFFER[31]

#define RAND_DISPLAY(bUFFER) DISPLAY_128BITS(bUFFER)
#define AUTN_DISPLAY(bUFFER) DISPLAY_128BITS(bUFFER)
/* Display only first 128 bits of KASME */
#define KASME_DISPLAY_1(bUFFER) DISPLAY_128BITS(bUFFER)
#define KASME_DISPLAY_2(bUFFER) DISPLAY_128BITS_2(bUFFER)

/* Holds an E-UTRAN authentication vector */
typedef struct eutran_vector_s {
  uint8_t rand[RAND_LENGTH_OCTETS];
  res_t   xres;
  uint8_t autn[AUTN_LENGTH_OCTETS];
  uint8_t kasme[KASME_LENGTH_OCTETS];

  /* one UE can have multiple vectors so use STAILQ lists for easy management */
#if 0
  STAILQ_ENTRY(eutran_vector_s) entries;
#endif
} eutran_vector_t;

#define FC_KASME        (0x10)
#define FC_KENB         (0x11)
#define FC_NH           (0x12)
#define FC_KENB_STAR    (0x13)
/* 33401 #A.7 Algorithm for key derivation function.
 * This FC should be used for:
 * - NAS Encryption algorithm
 * - NAS Integrity algorithm
 * - RRC Encryption algorithm
 * - RRC Integrity algorithm
 * - User Plane Encryption algorithm
 */
#define FC_ALG_KEY_DER  (0x15)
#define FC_KASME_TO_CK  (0x16)

typedef enum {
  NAS_ENC_ALG = 0x01,
  NAS_INT_ALG = 0x02,
  RRC_ENC_ALG = 0x03,
  RRC_INT_ALG = 0x04,
  UP_ENC_ALG  = 0x05,
  UP_INT_ALG  = 0x06
} algorithm_type_dist_t;

#endif /* SECURITY_TYPES_H_ */
