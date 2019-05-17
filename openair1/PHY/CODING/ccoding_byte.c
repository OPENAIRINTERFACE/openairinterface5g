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

/* file: ccoding_byte.c
   purpose: Encoding routines for implementing 802.11 convolutionally-coded waveforms
   author: raymond.knopp@eurecom.fr, based on similar code for 3GPP convolutional code (UMTS) by P. Humblet (2000)
   date: 10.2004
*/
#include "coding_defs.h"


unsigned short gdot11[] = { 0133, 0171 }; // {A,B}
unsigned short gdot11_rev[] = { 0155, 0117 }; // {A,B}
unsigned char  ccodedot11_table[128];
unsigned char  ccodedot11_table_rev[128];


/*************************************************************************

  Encodes for an arbitrary convolutional code of rate 1/2
  with a constraint length of 7 bits.
  The inputs are bit packed in octets (from MSB to LSB).
  Trellis termination is included here
*************************************************************************/



void
ccodedot11_encode (unsigned int numbytes,
                   unsigned char *inPtr,
                   unsigned char *outPtr,
                   unsigned char puncturing) {
  unsigned int             state;
  unsigned char              c, out, shiftbit =0;
  //  printf("In ccodedot11_encode (%d,%p,%p,%d)\n",numbytes,inPtr,outPtr,puncturing);
#ifdef DEBUG_CCODE
  unsigned int  dummy;
#endif //DEBUG_CCODE
  int bit_index;
  /* The input bit is shifted in position 8 of the state.
     Shiftbit will take values between 1 and 8 */
  state = 0;
#ifdef DEBUG_CCODE
  dummy = 0;
#endif //DEBUG_CCODE
  /* Do not increment inPtr until we read the next octet */
  bit_index=0;

  while (numbytes-- > 0) {
    c = *inPtr++;
#ifdef DEBUG_CCODE
    printf("** %d **\n",c);
#endif //DEBUG_CCODE

    switch (puncturing) {
      case 0:  //rate 1/2
        for (shiftbit = 0; shiftbit<8; shiftbit++) {
          state >>= 1;

          if ((c&(1<<shiftbit)) != 0) {
            state |= 64;
          }

          out = ccodedot11_table[state];
          *outPtr++ = out  & 1;
          *outPtr++ = (out>>1)&1;
#ifdef DEBUG_CCODE
          printf("%u: %u -> %d (%u)\n",dummy,state,out,ccodedot11_table[state]);
          dummy+=2;
#endif //DEBUG_CCODE      
        }

        break;

      case 1: // rate 3/4
        for (shiftbit = 0; shiftbit<8; shiftbit++) {
          state >>= 1;

          if ((c&(1<<shiftbit)) != 0) {
            state |= 64;
          }

          out = ccodedot11_table[state];

          if (bit_index<2)
            *outPtr++ = out  & 1;

          if (bit_index!=1)
            *outPtr++ = (out>>1)&1;

#ifdef DEBUG_CCODE
          printf("%u: %u -> %d (%u)\n",dummy,state,out,ccodedot11_table[state]);
          dummy+=2;
#endif //DEBUG_CCODE      
          bit_index=(bit_index==2)?0:(bit_index+1);
        }

        break;

      case 2: // rate 2/3
        for (shiftbit = 0; shiftbit<8; shiftbit++) {
          state >>= 1;

          if ((c&(1<<shiftbit)) != 0) {
            state |= 64;
          }

          out = ccodedot11_table[state];
          *outPtr++ = out  & 1;

          if (bit_index==0)
            *outPtr++ = (out>>1)&1;

#ifdef DEBUG_CCODE
          printf("%u: %u -> %d (%u)\n",dummy,state,out,ccodedot11_table[state]);
          dummy+=2;
#endif //DEBUG_CCODE      
          bit_index=(bit_index==0)?1:0;
        }

        break;

      default:
        break;
    }
  }

  /*
  // Termination - Add one NULL byte to terminate trellis
  #ifdef DEBUG_CCODE
      printf("Termination\n");
  #endif //DEBUG_CCODE
  for (shiftbit = 0; shiftbit<8;shiftbit++) {
    state >>= 1;
    out = ccodedot11_table[state];

    *outPtr++ = out  & 1;
    *outPtr++ = (out>>1)&1;

  #ifdef DEBUG_CCODE
    printf("%d: %d -> %d (%d)\n",dummy,state,out,ccodedot11_table[state]);
    dummy+=2;
  #endif //DEBUG_CCODE
    printf("%d -> %d (%d)\n",state,out,ccodedot11_table[state]);

  }

  */
}



/*************************************************************************

  Functions to initialize the code tables

*************************************************************************/


/* Basic code table initialization for constraint length 7 */
/* Input in MSB, followed by state in 6 LSBs */

void ccodedot11_init(void) {
  unsigned int  i, j, k, sum;

  for (i = 0; i < 128; i++) {
    ccodedot11_table[i] = 0;

    /* Compute R output bits */
    for (j = 0; j < 2; j++) {
      sum = 0;

      for (k = 0; k < 7; k++)
        if ((i & gdot11[j]) & (1 << k))
          sum++;

      /* Write the sum modulo 2 in bit j */
      ccodedot11_table[i] |= (sum & 1) << j;
    }
  }
}

/* Input in LSB, followed by state in 6 MSBs */
void ccodedot11_init_inv(void) {
  unsigned int  i, j, k, sum;

  for (i = 0; i < 128; i++) {
    ccodedot11_table_rev[i] = 0;

    /* Compute R output bits */
    for (j = 0; j < 2; j++) {
      sum = 0;

      for (k = 0; k < 7; k++)
        if ((i & gdot11_rev[j]) & (1 << k))
          sum++;

      /* Write the sum modulo 2 in bit j */
      ccodedot11_table_rev[i] |= (sum & 1) << j;
    }
  }
}



/*****************************************************************/
/**
  Test program

******************************************************************/

#ifdef DEBUG_CCODE
#include <stdio.h>

main() {
  unsigned char test[] = "0Thebigredfox";
  unsigned char output[512], *inPtr, *outPtr;
  unsigned int i;
  test[0] = 128;
  test[1] = 0;
  ccodedot11_init();
  inPtr = test;
  outPtr = output;
  ccodedot11_encode(16, inPtr, outPtr,0);

  for (i = 0; i < 32; i++) printf("%x ", output[i]);

  printf("\n");
}
#endif

/** @}*/
