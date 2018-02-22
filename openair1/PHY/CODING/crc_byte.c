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


#include "defs.h"


/*ref 36-212 v8.6.0 , pp 8-9 */
/* the highest degree is set by default */
unsigned int             poly24a = 0x864cfb00;   //1000 0110 0100 1100 1111 1011  D^24 + D^23 + D^18 + D^17 + D^14 + D^11 + D^10 + D^7 + D^6 + D^5 + D^4 + D^3 + D + 1
unsigned int             poly24b = 0x80006300;    // 1000 0000 0000 0000 0110 0011  D^24 + D^23 + D^6 + D^5 + D + 1
unsigned int             poly16 = 0x10210000;    // 0001 0000 0010 0001            D^16 + D^12 + D^5 + 1
unsigned int             poly12 = 0x80F00000;    // 1000 0000 1111                 D^12 + D^11 + D^3 + D^2 + D + 1
unsigned int             poly8 = 0x9B000000;     // 1001 1011                      D^8  + D^7  + D^4 + D^3 + D + 1
/*********************************************************

For initialization && verification purposes,
   bit by bit implementation with any polynomial

The first bit is in the MSB of each byte

*********************************************************/
unsigned int
crcbit (unsigned char * inputptr, int octetlen, unsigned int poly)
{
  unsigned int             i, crc = 0, c;

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
static unsigned int      crc24aTable[256];
static unsigned int      crc24bTable[256];
static unsigned short      crc16Table[256];
static unsigned short      crc12Table[256];
static unsigned char       crc8Table[256];

void crcTableInit (void)
{
  unsigned char              c = 0;

  do {
    crc24aTable[c] = crcbit (&c, 1, poly24a);
    crc24bTable[c] = crcbit (&c, 1, poly24b);
    crc16Table[c] = (unsigned short) (crcbit (&c, 1, poly16) >> 16);
    crc12Table[c] = (unsigned short) (crcbit (&c, 1, poly12) >> 16);
    crc8Table[c] = (unsigned char) (crcbit (&c, 1, poly8) >> 24);
  } while (++c);
}
/*********************************************************

Byte by byte implementations,
assuming initial byte is 0 padded (in MSB) if necessary

*********************************************************/
unsigned int
crc24a (unsigned char * inptr, int bitlen)
{

  int             octetlen, resbit;
  unsigned int             crc = 0;
  octetlen = bitlen / 8;        /* Change in octets */
  resbit = (bitlen % 8);

  while (octetlen-- > 0) {
    //   printf("crc24a: in %x => crc %x\n",crc,*inptr);
    crc = (crc << 8) ^ crc24aTable[(*inptr++) ^ (crc >> 24)];
  }

  if (resbit > 0)
    crc = (crc << resbit) ^ crc24aTable[((*inptr) >> (8 - resbit)) ^ (crc >> (32 - resbit))];

  return crc;
}

unsigned int crc24b (unsigned char * inptr, int bitlen)
{

  int             octetlen, resbit;
  unsigned int             crc = 0;
  octetlen = bitlen / 8;        /* Change in octets */
  resbit = (bitlen % 8);

  while (octetlen-- > 0) {
    //    printf("crc24b: in %x => crc %x (%x)\n",crc,*inptr,crc24bTable[(*inptr) ^ (crc >> 24)]);
    crc = (crc << 8) ^ crc24bTable[(*inptr++) ^ (crc >> 24)];
  }

  if (resbit > 0)
    crc = (crc << resbit) ^ crc24bTable[((*inptr) >> (8 - resbit)) ^ (crc >> (32 - resbit))];

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
  printf("%x\n", crcbit(test, sizeof(test) - 1, poly24));
  printf("%x\n", crc24(test, (sizeof(test) - 1)*8));
  printf("%x\n", crcbit(test, sizeof(test) - 1, poly8));
  printf("%x\n", crc8(test, (sizeof(test) - 1)*8));
}
#endif

