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

/* file: ccoding_byte_lte.c
   purpose: Tail-biting convolutional code from 36-212, V8.6 2009-03.  Includes CRC (8-UCI,16-DCI)and RNTI scrambling (DCI)
   author: raymond.knopp@eurecom.fr
   date: 21.10.2009
*/
#include "coding_defs.h"

//#define DEBUG_CCODE 1

unsigned short glte[] = { 0133, 0171, 0165 }; // {A,B}
unsigned short glte_rev[] = { 0155, 0117, 0127 }; // {A,B}
unsigned short gdab[] = { 0133, 0171, 0145 }; // {A,B}
unsigned short gdab_rev[] = { 0155, 0117, 0123 }; // {A,B}
unsigned char  ccodelte_table[128];      // for transmitter
unsigned char  ccodelte_table_rev[128];  // for receiver


/*************************************************************************

  Encodes for an arbitrary convolutional code of rate 1/3
  with a constraint length of 7 bits.
  The inputs are bit packed in octets (from MSB to LSB).
  An optional 8-bit CRC (3GPP) can be added.
  Trellis tail-biting is included here
*************************************************************************/



void
ccodelte_encode (int32_t numbits,
                 uint8_t add_crc,
                 uint8_t *inPtr,
                 uint8_t *outPtr,
                 uint16_t rnti) {
  uint32_t             state;
  uint8_t              c, out, first_bit;
  int8_t shiftbit=0;
  uint16_t c16;
  uint16_t next_last_byte=0;
  uint32_t crc=0;
#ifdef DEBUG_CCODE
  uint32_t  dummy=0;
#endif //DEBUG_CCODE
  /* The input bit is shifted in position 8 of the state.
     Shiftbit will take values between 1 and 8 */
  state = 0;

  if (add_crc == 1) {
    crc = crc8(inPtr,numbits);
    first_bit      = 2;
    c = (uint8_t)(crc>>24);
  } else if (add_crc == 2) {
    crc = crc16(inPtr,numbits);
#ifdef DEBUG_CCODE
    printf("ccode_lte : crc %x\n",crc);
#endif
    // scramble with RNTI
    crc ^= (((uint32_t)rnti)<<16);
#ifdef DEBUG_CCODE
    printf("ccode_lte : crc %x (rnti %x)\n",crc,rnti);
#endif
    first_bit      = 2;
    c = (uint8_t)((crc>>16)&0xff);
  } else {
    next_last_byte = numbits>>3;
    first_bit      = (numbits-6)&7;
    c = inPtr[next_last_byte-1];
  }

#ifdef DEBUG_CCODE
  printf("next_last_byte %x (numbits %d, %d)\n",c,numbits,next_last_byte);
#endif

  // Perform Tail-biting

  // get bits from last byte of input (or crc)
  //  for (shiftbit = first_bit; shiftbit<8; shiftbit++) {
  for (shiftbit = 0 ; shiftbit <(8-first_bit) ; shiftbit++) {
    if ((c&(1<<(7-first_bit-shiftbit))) != 0)
      state |= (1<<shiftbit);

#ifdef DEBUG_CCODE
    printf("shiftbit %d, %d -> %d \n",shiftbit,c&(1<<(7-first_bit-shiftbit)),state);
    dummy+=3;
#endif
  }

  // get bits from next to last byte of input (not when crc is added)
  if ((add_crc==0)&&((numbits&7)>0)) {
    c = inPtr[next_last_byte];

    //    printf("last_byte %x\n",c);
    //    for (shiftbit = 0 ; shiftbit < (numbits&7) ; shiftbit++) {
    for (shiftbit = (numbits&7)-1 ; shiftbit>=0 ; shiftbit--) {
      state >>= 1;

      if ((c&(1<<shiftbit)) != 0)
        state |= 64;

#ifdef DEBUG_CCODE
      printf("shiftbit %d, %d: %d -> %d \n",shiftbit,state>>6,dummy,state);
      dummy+=3;
#endif
    }
  }

  state = state & 0x3f;   // true initial state of Tail-biting CCode
  state<<=1;              // because of loop structure in CCode
#ifdef DEBUG_CCODE
  printf("Initial state %d\n",state);
  dummy = 0;
#endif //DEBUG_CCODE
  /* Do not increment inPtr until we read the next octet */

  while (numbits > 0) {
    c = *inPtr++;
#ifdef DEBUG_CCODE
    printf("** %x **\n",c);
#endif //DEBUG_CCODE

    //    for (shiftbit = 0; (shiftbit<8) && (numbits>0);shiftbit++,numbits--) {
    for (shiftbit = 7; (shiftbit>=0) && (numbits>0); shiftbit--,numbits--) {
      state >>= 1;

      if ((c&(1<<shiftbit)) != 0) {
        state |= 64;
      }

      out = ccodelte_table[state];
      *outPtr++ = out  & 1;
      *outPtr++ = (out>>1)&1;
      *outPtr++ = (out>>2)&1;
#ifdef DEBUG_CCODE
      printf("numbits %d, input %d, outbit %d: %d -> %d (%d%d%d)\n",numbits,state>>6,dummy,state,out,out&1,(out>>1)&1,(out>>2)&1);
      dummy+=3;
#endif //DEBUG_CCODE      
    }
  }

  // now code 8-bit CRC for UCI
  if (add_crc == 1) {
    c = (uint8_t)(crc>>24);

    //    for (shiftbit = 0; (shiftbit<8);shiftbit++) {
    for (shiftbit = 7; (shiftbit>=0); shiftbit--) {
      state >>= 1;

      if ((c&(1<<shiftbit)) != 0) {
        state |= 64;
      }

      out = ccodelte_table[state];
      *outPtr++ = out  & 1;
      *outPtr++ = (out>>1)&1;
      *outPtr++ = (out>>2)&1;
#ifdef DEBUG_CCODE
      printf("crc bit %d input %d, outbit %d: %d -> %d (%u)\n",shiftbit,state>>6,dummy,state,out,ccodelte_table[state]);
      dummy+=3;
#endif //DEBUG_CCODE      
    }
  }

  // now code 16-bit CRC for DCI
  if (add_crc == 2) {
    c16 = (uint16_t)(crc>>16);

    //    for (shiftbit = 0; (shiftbit<16);shiftbit++) {
    for (shiftbit = 15; (shiftbit>=0); shiftbit--) {
      state >>= 1;

      if ((c16&(1<<shiftbit)) != 0) {
        state |= 64;
      }

      out = ccodelte_table[state];
      *outPtr++ = out  & 1;
      *outPtr++ = (out>>1)&1;
      *outPtr++ = (out>>2)&1;
#ifdef DEBUG_CCODE
      printf("crc bit %d input %d, outbit %d: %d -> %d (%u)\n",shiftbit,state>>6,dummy,state,out,ccodelte_table[state]);
      dummy+=3;
#endif //DEBUG_CCODE      
    }
  }
}



/*************************************************************************

  Functions to initialize the code tables

*************************************************************************/


/* Basic code table initialization for constraint length 7 */
/* Input in MSB, followed by state in 6 LSBs */

void ccodelte_init(void) {
  unsigned int  i, j, k, sum;

  for (i = 0; i < 128; i++) {
    ccodelte_table[i] = 0;

    /* Compute 3 output bits */
    for (j = 0; j < 3; j++) {
      sum = 0;

      for (k = 0; k < 7; k++)
        if ((i & glte[j]) & (1 << k))
          sum++;

      /* Write the sum modulo 2 in bit j */
      ccodelte_table[i] |= (sum & 1) << j;
    }
  }
}

/* Input in LSB, followed by state in 6 MSBs */
void ccodelte_init_inv(void) {
  unsigned int  i, j, k, sum;

  for (i = 0; i < 128; i++) {
    ccodelte_table_rev[i] = 0;

    /* Compute R output bits */
    for (j = 0; j < 3; j++) {
      sum = 0;

      for (k = 0; k < 7; k++)
        if ((i & glte_rev[j]) & (1 << k))
          sum++;

      /* Write the sum modulo 2 in bit j */
      ccodelte_table_rev[i] |= (sum & 1) << j;
    }
  }
}

void ccodedab_init(void) {
  unsigned int  i, j, k, sum;

  for (i = 0; i < 128; i++) {
    ccodelte_table[i] = 0;

    /* Compute 3 output bits */
    for (j = 0; j < 3; j++) {
      sum = 0;

      for (k = 0; k < 7; k++)
        if ((i & gdab[j]) & (1 << k))
          sum++;

      /* Write the sum modulo 2 in bit j */
      ccodelte_table[i] |= (sum & 1) << j;
    }
  }
}

/* Input in LSB, followed by state in 6 MSBs */
void ccodedab_init_inv(void) {
  unsigned int  i, j, k, sum;

  for (i = 0; i < 128; i++) {
    ccodelte_table_rev[i] = 0;

    /* Compute R output bits */
    for (j = 0; j < 3; j++) {
      sum = 0;

      for (k = 0; k < 7; k++)
        if ((i & gdab_rev[j]) & (1 << k))
          sum++;

      /* Write the sum modulo 2 in bit j */
      ccodelte_table_rev[i] |= (sum & 1) << j;
    }
  }
}


/*****************************************************************/
/**
  Test program

******************************************************************/

#ifdef CCODE_MAIN
#include <stdio.h>

main() {
  unsigned char test[] = "Thebigredfox";
  unsigned char output[512], *inPtr, *outPtr;
  unsigned int i;
  test[0] = 128;
  test[1] = 0;
  ccodelte_init();
  inPtr = test;
  outPtr = output;
  ccodelte_encode(21, inPtr, outPtr);

  for (i = 0; i < 21*3; i++) printf("%x ", output[i]);

  printf("\n");
}
#endif

/** @}*/
