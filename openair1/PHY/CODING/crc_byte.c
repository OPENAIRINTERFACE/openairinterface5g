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

/* file: crc_byte.c
   purpose: generate 3GPP LTE CRCs. Byte-oriented implementation of CRC's
   author: raymond.knopp@eurecom.fr
   date: 21.10.2009

   Original UMTS version by
   P. Humblet
   May 10, 2001
   Modified in June, 2001, to include  the length non multiple of 8
*/

#ifndef __SSE4_1__
#define USE_INTEL_CRC 0
#else
#define USE_INTEL_CRC __SSE4_1__
#endif

#include "coding_defs.h"
#include "assertions.h"
#if USE_INTEL_CRC 
#include "crc.h"
#endif
/*ref 36-212 v8.6.0 , pp 8-9 */
/* the highest degree is set by default */

unsigned int             poly24a = 0x864cfb00;   // 1000 0110 0100 1100 1111 1011
												 // D^24 + D^23 + D^18 + D^17 + D^14 + D^11 + D^10 + D^7 + D^6 + D^5 + D^4 + D^3 + D + 1
unsigned int             poly24b = 0x80006300;   // 1000 0000 0000 0000 0110 0011
											     // D^24 + D^23 + D^6 + D^5 + D + 1
unsigned int             poly24c = 0xb2b11700;   // 1011 0010 1011 0001 0001 0111
												 // D^24+D^23+D^21+D^20+D^17+D^15+D^13+D^12+D^8+D^4+D^2+D+1

unsigned int             poly16 = 0x10210000;    // 0001 0000 0010 0001            D^16 + D^12 + D^5 + 1
unsigned int             poly12 = 0x80F00000;    // 1000 0000 1111                 D^12 + D^11 + D^3 + D^2 + D + 1
unsigned int             poly8 = 0x9B000000;     // 1001 1011                      D^8  + D^7  + D^4 + D^3 + D + 1
uint32_t poly6 = 0x84000000; // 10000100000... -> D^6+D^5+1
uint32_t poly11 = 0xc4200000; //11000100001000... -> D^11+D^10+D^9+D^5+1

/*********************************************************

For initialization && verification purposes,
   bit by bit implementation with any polynomial

The first bit is in the MSB of each byte

*********************************************************/
unsigned int crcbit (unsigned char * inputptr,
		     int octetlen,
		     unsigned int poly)
{
  unsigned int i, crc = 0, c;

  while (octetlen-- > 0) {
    c = (*inputptr++) << 24;

    for (i = 8; i != 0; i--) {
      if ((1 << 31) & (c ^ crc))
        crc = (crc << 1) ^ poly;
      else
        crc <<= 1;

      c <<= 1;
    }
  }

  return crc;
}

/*********************************************************

crc table initialization

*********************************************************/
static unsigned int        crc24aTable[256];
static unsigned int        crc24bTable[256];
static unsigned int        crc24cTable[256];
static unsigned short      crc16Table[256];
static unsigned short      crc12Table[256];
static unsigned short      crc11Table[256];
static unsigned char       crc8Table[256];
static unsigned char       crc6Table[256];

#if USE_INTEL_CRC
static DECLARE_ALIGNED(struct crc_pclmulqdq_ctx lte_crc24a_pclmulqdq, 16) = {
        0x64e4d700,     /**< k1 */
        0x2c8c9d00,     /**< k2 */
        0xd9fe8c00,     /**< k3 */
        0xf845fe24,     /**< q */
        0x864cfb00,     /**< p */
        0ULL            /**< res */
};
__m128i crc_xmm_be_le_swap128;

DECLARE_ALIGNED(const uint8_t crc_xmm_shift_tab[48], 16) = {
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
        0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
        0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff
};

#endif

void crcTableInit (void)
{
  unsigned char c = 0;

  do {
    crc24aTable[c] = crcbit (&c, 1, poly24a);
    crc24bTable[c] = crcbit (&c, 1, poly24b);
    crc24cTable[c] = crcbit (&c, 1, poly24c);
    crc16Table[c] = (unsigned short) (crcbit (&c, 1, poly16) >> 16);
    crc12Table[c] = (unsigned short) (crcbit (&c, 1, poly12) >> 16);
    crc11Table[c] = (unsigned short) (crcbit (&c, 1, poly11) >> 16);
    crc8Table[c] = (unsigned char) (crcbit (&c, 1, poly8) >> 24);
    crc6Table[c] = (unsigned char) (crcbit (&c, 1, poly6) >> 24);
  } while (++c);
#if USE_INTEL_CRC
    crc_xmm_be_le_swap128 = _mm_setr_epi32(0x0c0d0e0f, 0x08090a0b,
					   0x04050607, 0x00010203);

#endif
}

/*********************************************************

Byte by byte LUT implementations,
assuming initial byte is 0 padded (in MSB) if necessary
can use SIMD optimized Intel CRC for LTE/NR 24a/24b variants
*********************************************************/

unsigned int crc24a (unsigned char * inptr,
					 int bitlen)
{
  int octetlen = bitlen / 8;  /* Change in octets */

  if ( bitlen % 8 || !USE_INTEL_CRC ) {
  unsigned int      crc = 0;
  int resbit= (bitlen % 8);
    
  while (octetlen-- > 0) {
    //   printf("crc24a: in %x => crc %x\n",crc,*inptr);
    crc = (crc << 8) ^ crc24aTable[(*inptr++) ^ (crc >> 24)];
  }

  if (resbit > 0)
    crc = (crc << resbit) ^ crc24aTable[((*inptr) >> (8 - resbit)) ^ (crc >> (32 - resbit))];
  return crc;
  }
  #if USE_INTEL_CRC
  else {
  return crc32_calc_pclmulqdq(inptr, octetlen, 0,
                              &lte_crc24a_pclmulqdq);
  }
  #endif

}

#if USE_INTEL_CRC
static DECLARE_ALIGNED(struct crc_pclmulqdq_ctx lte_crc24b_pclmulqdq, 16) = {
        0x80140500,     /**< k1 */
        0x42000100,     /**< k2 */
        0x90042100,     /**< k3 */
        0xffff83ff,     /**< q */
        0x80006300,     /**< p */
        0ULL            /**< res */
};
#endif
unsigned int crc24b (unsigned char * inptr,
	   	     int bitlen)
{
  int octetlen = bitlen / 8;  /* Change in octets */
  
  if ( bitlen % 8 || !USE_INTEL_CRC ) {
  unsigned int crc = 0;
  int resbit = (bitlen % 8);

  while (octetlen-- > 0) {
    //    printf("crc24b: in %x => crc %x (%x)\n",crc,*inptr,crc24bTable[(*inptr) ^ (crc >> 24)]);
    crc = (crc << 8) ^ crc24bTable[(*inptr++) ^ (crc >> 24)];
  }

  if (resbit > 0)
    crc = (crc << resbit) ^ crc24bTable[((*inptr) >> (8 - resbit)) ^ (crc >> (32 - resbit))];

  return crc;
  }
#if USE_INTEL_CRC
  else {
  return crc32_calc_pclmulqdq(inptr, octetlen, 0,
                              &lte_crc24b_pclmulqdq);
  }
#endif
}

unsigned int crc24c (unsigned char * inptr,
					 int bitlen)
{
  int octetlen, resbit;
  unsigned int crc = 0;
  octetlen = bitlen / 8;        /* Change in octets */
  resbit = (bitlen % 8);

  while (octetlen-- > 0) {
    crc = (crc << 8) ^ crc24cTable[(*inptr++) ^ (crc >> 24)];
  }

  if (resbit > 0) {
    crc = (crc << resbit) ^ crc24cTable[((*inptr) >> (8 - resbit)) ^ (crc >> (32 - resbit))];
  }

  return crc;
}

unsigned int
crc16 (unsigned char * inptr, int bitlen)
{
  int             octetlen, resbit;
  unsigned int             crc = 0;
  octetlen = bitlen / 8;        /* Change in octets */
  resbit = (bitlen % 8);

  while (octetlen-- > 0) {

    crc = (crc << 8) ^ (crc16Table[(*inptr++) ^ (crc >> 24)] << 16);
  }

  if (resbit > 0)
    crc = (crc << resbit) ^ (crc16Table[((*inptr) >> (8 - resbit)) ^ (crc >> (32 - resbit))] << 16);

  return crc;
}

unsigned int
crc12 (unsigned char * inptr, int bitlen)
{
  int             octetlen, resbit;
  unsigned int             crc = 0;
  octetlen = bitlen / 8;        /* Change in octets */
  resbit = (bitlen % 8);

  while (octetlen-- > 0) {
    crc = (crc << 8) ^ (crc12Table[(*inptr++) ^ (crc >> 24)] << 16);
  }

  if (resbit > 0)
    crc = (crc << resbit) ^ (crc12Table[((*inptr) >> (8 - resbit)) ^ (crc >> (32 - resbit))] << 16);

  return crc;
}

unsigned int
crc11 (unsigned char * inptr, int bitlen)
{
  int             octetlen, resbit;
  unsigned int             crc = 0;
  octetlen = bitlen / 8;        /* Change in octets */
  resbit = (bitlen % 8);

  while (octetlen-- > 0) {
    crc = (crc << 8) ^ (crc11Table[(*inptr++) ^ (crc >> 24)] << 16);
  }

  if (resbit > 0)
    crc = (crc << resbit) ^ (crc11Table[((*inptr) >> (8 - resbit)) ^ (crc >> (32 - resbit))] << 16);

  return crc;
}

unsigned int
crc8 (unsigned char * inptr, int bitlen)
{
  int             octetlen, resbit;
  unsigned int             crc = 0;
  octetlen = bitlen / 8;        /* Change in octets */
  resbit = (bitlen % 8);

  while (octetlen-- > 0) {
    crc = crc8Table[(*inptr++) ^ (crc >> 24)] << 24;
  }

  if (resbit > 0)
    crc = (crc << resbit) ^ (crc8Table[((*inptr) >> (8 - resbit)) ^ (crc >> (32 - resbit))] << 24);

  return crc;
}

unsigned int
crc6 (unsigned char * inptr, int bitlen)
{
  int             octetlen, resbit;
  unsigned int             crc = 0;
  octetlen = bitlen / 8;        /* Change in octets */
  resbit = (bitlen % 8);

  while (octetlen-- > 0) {
    crc = crc6Table[(*inptr++) ^ (crc >> 24)] << 24;
  }

  if (resbit > 0)
    crc = (crc << resbit) ^ (crc6Table[((*inptr) >> (8 - resbit)) ^ (crc >> (32 - resbit))] << 24);

  return crc;
}

int check_crc(uint8_t* decoded_bytes, uint32_t n, uint32_t F, uint8_t crc_type)
{
  uint32_t crc=0,oldcrc=0;
  uint8_t crc_len=0;

  switch (crc_type) {
  case CRC24_A:
  case CRC24_B:
    crc_len=3;
    break;

  case CRC16:
    crc_len=2;
    break;

  case CRC8:
    crc_len=1;
    break;

  default:
    AssertFatal(1,"Invalid crc_type \n");
  }

  for (int i=0; i<crc_len; i++)
    oldcrc |= (decoded_bytes[(n>>3)-crc_len+i])<<((crc_len-1-i)<<3);

  switch (crc_type) {
    
  case CRC24_A:
    oldcrc&=0x00ffffff;
    crc = crc24a(decoded_bytes,
		 n-24)>>8;
    
    break;
    
  case CRC24_B:
      oldcrc&=0x00ffffff;
      crc = crc24b(decoded_bytes,
                   n-24)>>8;
      
      break;

    case CRC16:
      oldcrc&=0x0000ffff;
      crc = crc16(decoded_bytes,
                  n-16)>>16;
      
      break;

    case CRC8:
      oldcrc&=0x000000ff;
      crc = crc8(decoded_bytes,
                 n-8)>>24;
      break;

    default:
      AssertFatal(1,"Invalid crc_type \n");
    }


    if (crc == oldcrc)
      return(1);
    else
      return(0);

}


#ifdef DEBUG_CRC
/*******************************************************************/
/**
   Test code
********************************************************************/

#include <stdio.h>

main()
{
  unsigned char test[] = "Thebigredfox";
  crcTableInit();
  printf("%x\n", crcbit(test, sizeof(test) - 1, poly24a));
  printf("%x\n", crc24(test, (sizeof(test) - 1)*8));
  printf("%x\n", crcbit(test, sizeof(test) - 1, poly8));
  printf("%x\n", crc8(test, (sizeof(test) - 1)*8));
}
#endif


