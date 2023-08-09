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

/* Lookup tables for 3GPP scrambling/unscrambling */

/* Author R. Knopp / EURECOM / OpenAirInterface.org */
#ifndef __SCRAMBLING_LUTS__C__
#define __SCRAMBLING_LUTS__C__

#include "PHY/impl_defs_nr.h"
#include "PHY/sse_intrin.h"
#include <common/utils/LOG/log.h>

__m64 byte2m64_re[256];
__m64 byte2m64_im[256];

__m128i byte2m128i[256];

void init_byte2m64(void) {

  for (int s=0;s<256;s++) {
    byte2m64_re[s] = _mm_insert_pi16(byte2m64_re[s],(1-2*(s&1)),0);
    byte2m64_im[s] = _mm_insert_pi16(byte2m64_im[s],(1-2*((s>>1)&1)),0);
    byte2m64_re[s] = _mm_insert_pi16(byte2m64_re[s],(1-2*((s>>2)&1)),1);
    byte2m64_im[s] = _mm_insert_pi16(byte2m64_im[s],(1-2*((s>>3)&1)),1);
    byte2m64_re[s] = _mm_insert_pi16(byte2m64_re[s],(1-2*((s>>4)&1)),2);
    byte2m64_im[s] = _mm_insert_pi16(byte2m64_im[s],(1-2*((s>>5)&1)),2);
    byte2m64_re[s] = _mm_insert_pi16(byte2m64_re[s],(1-2*((s>>6)&1)),3);
    byte2m64_im[s] = _mm_insert_pi16(byte2m64_im[s],(1-2*((s>>7)&1)),3);
     LOG_T(PHY,"init_scrambling_luts: s %x (%d) ((%d,%d),(%d,%d),(%d,%d),(%d,%d))\n",
	    ((uint16_t*)&s)[0],
	    (1-2*(s&1)),
	    ((int16_t*)&byte2m64_re[s])[0],((int16_t*)&byte2m64_im[s])[0],    
	    ((int16_t*)&byte2m64_re[s])[1],((int16_t*)&byte2m64_im[s])[1],    
	    ((int16_t*)&byte2m64_re[s])[2],((int16_t*)&byte2m64_im[s])[2],    
	    ((int16_t*)&byte2m64_re[s])[3],((int16_t*)&byte2m64_im[s])[3]);  

  }
}

void init_byte2m128i(void) {

  for (int s=0;s<256;s++) {
    byte2m128i[s] = _mm_insert_epi16(byte2m128i[s],(1-2*(s&1)),0);
    byte2m128i[s] = _mm_insert_epi16(byte2m128i[s],(1-2*((s>>1)&1)),1);
    byte2m128i[s] = _mm_insert_epi16(byte2m128i[s],(1-2*((s>>2)&1)),2);
    byte2m128i[s] = _mm_insert_epi16(byte2m128i[s],(1-2*((s>>3)&1)),3);
    byte2m128i[s] = _mm_insert_epi16(byte2m128i[s],(1-2*((s>>4)&1)),4);
    byte2m128i[s] = _mm_insert_epi16(byte2m128i[s],(1-2*((s>>5)&1)),5);
    byte2m128i[s] = _mm_insert_epi16(byte2m128i[s],(1-2*((s>>6)&1)),6);
    byte2m128i[s] = _mm_insert_epi16(byte2m128i[s],(1-2*((s>>7)&1)),7);
  }
}

void init_scrambling_luts(void) {

  init_byte2m64();
  init_byte2m128i();
}

#endif
