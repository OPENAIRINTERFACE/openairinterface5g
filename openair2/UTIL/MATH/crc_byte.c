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

/*
  May 10, 2001
  Modified in June, 2001, to include  the length non multiple of 8
  crc_byte.c
  Byte oriented implementation of CRC's

  */
#include "rtos_header.h"
#include "platform_types.h"


/*ref 25.222 v4.0.0 , p12 */
/* the highest degree is set by default */
unsigned int             poly24 = 0x80006100;    // 1000 0000 0000 0000 0110 0001  D^24 + D^23 + D^6 + D^5 + D + 1
unsigned int             poly16 = 0x10210000;    // 0001 0000 0010 0001            D^16 + D^12 + D^5 + 1
unsigned int             poly12 = 0x80F00000;    // 1000 0000 1111                 D^12 + D^11 + D^3 + D^2 + D + 1
unsigned int             poly8 = 0x9B000000;     // 1001 1011                      D^8  + D^7  + D^4 + D^3 + D + 1
/*********************************************************

For initialization && verification purposes,
   bit by bit implementation with any polynomial

The first bit is in the MSB of each byte

*********************************************************/
static          unsigned int
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
static unsigned int      crc24Table[256];
static unsigned short      crc16Table[256];
static unsigned short      crc12Table[256];
static unsigned char       crc8Table[256];
void
crcTableInit ()
{
  unsigned char              c = 0;

  do {
    crc24Table[c] = crcbit (&c, 1, poly24);
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
crc24 (unsigned char * inptr, int bitlen)
{

  int             octetlen, resbit;
  unsigned int             crc = 0;
  octetlen = bitlen / 8;        /* Change in octets */
  resbit = (bitlen % 8);

  while (octetlen-- > 0) {
    crc = (crc << 8) ^ crc24Table[(*inptr++) ^ (crc >> 24)];
  }

  if (resbit > 0)
    crc = (crc << resbit) ^ crc24Table[((*inptr) >> (8 - resbit)) ^ (crc >> (32 - resbit))];

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

/*******************************************************************/
/**
   Test code
********************************************************************/

/* #ifdef MAIN
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
   #endif */
