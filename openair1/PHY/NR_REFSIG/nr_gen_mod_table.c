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

#include "nr_refsig.h"
#include "nr_mod_table.h"
short nr_qpsk_mod_table[8];

int32_t nr_16qam_mod_table[16];
#if defined(__SSE2__)
__m128i nr_qpsk_byte_mod_table[2048];
#endif

int64_t nr_16qam_byte_mod_table[1024];

int64_t nr_64qam_mod_table[4096];

int32_t nr_256qam_mod_table[512];

void nr_generate_modulation_table() {
  float sqrt2 = 0.70711;
  float sqrt10 = 0.31623;
  float sqrt42 = 0.15430;
  float sqrt170 = 0.076696;
  float val = 32768.0;
  uint32_t i,j;
  short* table;

  // QPSK
  for (i=0; i<4; i++) {
    nr_qpsk_mod_table[i*2]   = (short)(1-2*(i&1))*val*sqrt2*sqrt2;
    nr_qpsk_mod_table[i*2+1] = (short)(1-2*((i>>1)&1))*val*sqrt2*sqrt2;
    //printf("%d j%d\n",nr_qpsk_mod_table[i*2],nr_qpsk_mod_table[i*2+1]);
  }

#if defined(__SSE2__)
  //QPSK m128
  table = (short*) nr_qpsk_byte_mod_table;
  for (i=0; i<256; i++) {
    for (j=0; j<4; j++) {
      table[i*8+(j*2)]   = (short)(1-2*((i>>(j*2))&1))*val*sqrt2*sqrt2;
      table[i*8+(j*2)+1] = (short)(1-2*((i>>(j*2+1))&1))*val*sqrt2*sqrt2;
      //printf("%d j%d\n",nr_qpsk_byte_mod_table[i*8+(j*2)],nr_qpsk_byte_mod_table[i*8+(j*2)+1]);
    }
  }
#endif

  //16QAM
  table = (short*) nr_16qam_byte_mod_table;
  for (i=0; i<256; i++) {
    for (j=0; j<2; j++) {
      table[i*4+(j*2)]   = (short)((1-2*((i>>(j*4))&1))*(2-(1-2*((i>>(j*4+2))&1))))*val*sqrt10*sqrt2;
      table[i*4+(j*2)+1] = (short)((1-2*((i>>(j*4+1))&1))*(2-(1-2*((i>>(j*4+3))&1))))*val*sqrt10*sqrt2;
     //printf("%d j%d\n",nr_16qam_byte_mod_table[i*4+(j*2)],nr_16qam_byte_mod_table[i*4+(j*2)+1]);
    }
  }

  table = (short*) nr_16qam_mod_table;
  for (i=0; i<16; i++) {
    table[i*2]   = (short)((1-2*(i&1))*(2-(1-2*((i>>2)&1))))*val*sqrt10*sqrt2;
    table[i*2+1] = (short)((1-2*((i>>1)&1))*(2-(1-2*((i>>3)&1))))*val*sqrt10*sqrt2;
    //printf("%d j%d\n",table[i*2],table[i*2+1]);
  }

  //64QAM
  table = (short*) nr_64qam_mod_table;
  for (i=0; i<4096; i++) {
    for (j=0; j<2; j++) {
      table[i*4+(j*2)]   = (short)((1-2*((i>>(j*6))&1))*(4-(1-2*((i>>(j*6+2))&1))*(2-(1-2*((i>>(j*6+4))&1)))))*val*sqrt42*sqrt2;
      table[i*4+(j*2)+1] = (short)((1-2*((i>>(j*6+1))&1))*(4-(1-2*((i>>(j*6+3))&1))*(2-(1-2*((i>>(j*6+5))&1)))))*val*sqrt42*sqrt2;
      //printf("%d j%d\n",table[i*4+(j*2)],table[i*4+(j*2)+1]);
    }
  }

  //256QAM
  table = (short*) nr_256qam_mod_table;
  for (i=0; i<256; i++) {
    table[i*2]   = (short)((1-2*(i&1))*(8-(1-2*((i>>2)&1))*(4-(1-2*((i>>4)&1))*(2-(1-2*((i>>6)&1))))))*val*sqrt170*sqrt2;
    table[i*2+1] = (short)((1-2*((i>>1)&1))*(8-(1-2*((i>>3)&1))*(4-(1-2*((i>>5)&1))*(2-(1-2*((i>>7)&1))))))*val*sqrt170*sqrt2;
  }
}

