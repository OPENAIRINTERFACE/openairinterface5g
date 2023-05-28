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
#ifndef SECURITY_TYPES_H_
#define SECURITY_TYPES_H_

#if !defined(HAVE_UINT128_T)
# include <gmp.h>
#endif

#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>


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
} eutran_vector_t;

#endif /* SECURITY_TYPES_H_ */
