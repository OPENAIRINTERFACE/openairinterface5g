/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/
/* file: 3gpplte_sse.c
   purpose: Encoding routines for implementing Turbo-coded (DLSCH) transport channels from 36-212, V8.6 2009-03
   author: Laurent Thomas
   maintainer: raymond.knopp@eurecom.fr
   date: 09.2012
*/
#ifndef TC_MAIN
#include "defs.h"
#include "extern_3GPPinterleaver.h"
#else
#include "vars.h"
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "PHY/sse_intrin.h"

#define print_bytes(s,x) printf("%s %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",s,(x)[0],(x)[1],(x)[2],(x)[3],(x)[4],(x)[5],(x)[6],(x)[7],(x)[8],(x)[9],(x)[10],(x)[11],(x)[12],(x)[13],(x)[14],(x)[15])
#define print_shorts(s,x) printf("%s %x,%x,%x,%x,%x,%x,%x,%x\n",s,(x)[0],(x)[1],(x)[2],(x)[3],(x)[4],(x)[5],(x)[6],(x)[7])
#define print_ints(s,x) printf("%s %x %x %x %x\n",s,(x)[0],(x)[1],(x)[2],(x)[3])


//#define DEBUG_TURBO_ENCODER 1
#define CALLGRIND 1
unsigned short threegpplte_interleaver_output;
unsigned long long threegpplte_interleaver_tmp;

#if defined(__x86_64__) || defined(__i386__)
struct treillis {
  union {
    __m64 systematic_64[3];
    char systematic_8[24];
  };
  union {
    __m64 parity1_64[3];
    char parity1_8[24];
  };
  union {
    __m64 parity2_64[3];
    char parity2_8[24];
  };
  int exit_state;
}  __attribute__ ((aligned(64)));

#elif defined(__arm__)

struct treillis {
  union {
    uint8x8_t systematic_64[3];
    char systematic_8[24];
  }__attribute__((aligned(64)));
  union {
    uint8x8_t parity1_64[3];
    char parity1_8[24];
  }__attribute__((aligned(64)));
  union {
    uint8x8_t parity2_64[3];
    char parity2_8[24];
  }__attribute__((aligned(64)));
  int exit_state;
};
#endif

struct treillis all_treillis[8][256];
int all_treillis_initialized=0;

static inline unsigned char threegpplte_rsc(unsigned char input,unsigned char *state)
{
  unsigned char output;
  output = (input ^ (*state>>2) ^ (*state>>1))&1;
  *state = (((input<<2)^(*state>>1))^((*state>>1)<<2)^((*state)<<2))&7;
  return(output);
}

static inline void threegpplte_rsc_termination(unsigned char *x,unsigned char *z,unsigned char *state)
{
  *z     = ((*state>>2) ^ (*state))   &1;
  *x     = ((*state)    ^ (*state>>1))   &1;
  *state = (*state)>>1;
}

void treillis_table_init(void)
{
  //struct treillis t[][]=all_treillis;
  //t=memalign(16,sizeof(struct treillis)*8*256);
  int i, j,b;
  unsigned char v, current_state;

  // clear all_treillis
  for (i=0; i<8; i++)
    bzero( all_treillis[i], sizeof(all_treillis[0]) );

  for (i=0; i<8; i++) { //all possible initial states
    for (j=0; j<=255; j++) { // all possible values of a byte
      current_state=i;

      for (b=0; b<8 ; b++ ) { // pre-compute the image of the byte j in _m128i vector right place
        all_treillis[i][j].systematic_8[b*3]= (j&(1<<(7-b)))>>(7-b);
        v=threegpplte_rsc( all_treillis[i][j].systematic_8[b*3] ,
                           &current_state);
        all_treillis[i][j].parity1_8[b*3+1]=v; // for the yparity1
        all_treillis[i][j].parity2_8[b*3+2]=v; // for the yparity2
      }

      all_treillis[i][j].exit_state=current_state;
    }
  }

  all_treillis_initialized=1;
  return ;
}


char interleave_compact_byte(short * base_interleaver,unsigned char * input, unsigned char * output, int n)
{

  char expandInput[768*8] __attribute__((aligned(16)));
  int i,loop=n>>4;
#if defined(__x86_64__) || defined(__i386__)
  __m128i *i_128=(__m128i *)input, *o_128=(__m128i*)expandInput;
  __m128i tmp1, tmp2, tmp3, tmp4;
  __m128i BIT_MASK = _mm_set_epi8(  0b00000001,
                                    0b00000010,
                                    0b00000100,
                                    0b00001000,
                                    0b00010000,
                                    0b00100000,
                                    0b01000000,
                                    0b10000000,
                                    0b00000001,
                                    0b00000010,
                                    0b00000100,
                                    0b00001000,
                                    0b00010000,
                                    0b00100000,
                                    0b01000000,
                                    0b10000000);
#elif defined(__arm__)
  uint8x16_t *i_128=(uint8x16_t *)input, *o_128=(uint8x16_t *)expandInput;
  uint8x16_t tmp1,tmp2;
  uint16x8_t tmp3;
  uint32x4_t tmp4;
  uint8x16_t and_tmp;
  uint8x16_t BIT_MASK = {  	    0b10000000,
                                    0b01000000,
                                    0b00100000,
                                    0b00010000,
                                    0b00001000,
                                    0b00000100,
                                    0b00000010,
                                    0b00000001,
                                    0b10000000,
                                    0b01000000,
                                    0b00100000,
                                    0b00010000,
                                    0b00001000,
                                    0b00000100,
                                    0b00000010,
                                    0b00000001};
#endif
  if ((n&15) > 0)
    loop++;

  for (i=0; i<loop ; i++ ) {
    /* int cur_byte=i<<3; */
    /* for (b=0;b<8;b++) */
    /*   expandInput[cur_byte+b] = (input[i]&(1<<(7-b)))>>(7-b); */

#if defined(__x86_64__) || defined(__i386__)
    tmp1=_mm_load_si128(i_128++);
    tmp2=_mm_unpacklo_epi8(tmp1,tmp1);
    tmp3=_mm_unpacklo_epi16(tmp2,tmp2);
    tmp4=_mm_unpacklo_epi32(tmp3,tmp3);
    *o_128++=_mm_cmpeq_epi8(_mm_and_si128(tmp4,BIT_MASK),BIT_MASK);

    tmp4=_mm_unpackhi_epi32(tmp3,tmp3);
    *o_128++=_mm_cmpeq_epi8(_mm_and_si128(tmp4,BIT_MASK),BIT_MASK);;

    tmp3=_mm_unpackhi_epi16(tmp2,tmp2);
    tmp4=_mm_unpacklo_epi32(tmp3,tmp3);
    *o_128++=_mm_cmpeq_epi8(_mm_and_si128(tmp4,BIT_MASK),BIT_MASK);;

    tmp4=_mm_unpackhi_epi32(tmp3,tmp3);
    *o_128++=_mm_cmpeq_epi8(_mm_and_si128(tmp4,BIT_MASK),BIT_MASK);;

    tmp2=_mm_unpackhi_epi8(tmp1,tmp1);
    tmp3=_mm_unpacklo_epi16(tmp2,tmp2);
    tmp4=_mm_unpacklo_epi32(tmp3,tmp3);
    *o_128++=_mm_cmpeq_epi8(_mm_and_si128(tmp4,BIT_MASK),BIT_MASK);;

    tmp4=_mm_unpackhi_epi32(tmp3,tmp3);
    *o_128++=_mm_cmpeq_epi8(_mm_and_si128(tmp4,BIT_MASK),BIT_MASK);;

    tmp3=_mm_unpackhi_epi16(tmp2,tmp2);
    tmp4=_mm_unpacklo_epi32(tmp3,tmp3);
    *o_128++=_mm_cmpeq_epi8(_mm_and_si128(tmp4,BIT_MASK),BIT_MASK);;

    tmp4=_mm_unpackhi_epi32(tmp3,tmp3);
    *o_128++=_mm_cmpeq_epi8(_mm_and_si128(tmp4,BIT_MASK),BIT_MASK);;

#elif defined(__arm__)
    tmp1=vld1q_u8((uint8_t*)i_128);
    //print_bytes("tmp1:",(uint8_t*)&tmp1);

    uint8x16x2_t temp1 =  vzipq_u8(tmp1,tmp1);
    tmp2 = temp1.val[0];

    uint16x8x2_t temp2 =  vzipq_u16((uint16x8_t)tmp2,(uint16x8_t)tmp2);
    tmp3 = temp2.val[0];

    uint32x4x2_t temp3 =  vzipq_u32((uint32x4_t)tmp3,(uint32x4_t)tmp3);
    tmp4 = temp3.val[0];
    //print_bytes("tmp4:",(uint8_t*)&tmp4);

    *o_128++=vceqq_u8(vandq_u8((uint8x16_t)tmp4,BIT_MASK),BIT_MASK);    //1
    //print_bytes("o:",(uint8_t*)(o_128-1));

    tmp4 = temp3.val[1];
    //print_bytes("tmp4:",(uint8_t*)&tmp4);

    *o_128++=vceqq_u8(vandq_u8((uint8x16_t)tmp4,BIT_MASK),BIT_MASK);    //2
    //print_bytes("o:",(uint8_t*)(o_128-1));

    tmp3 = temp2.val[1];
    temp3 =  vzipq_u32((uint32x4_t)tmp3,(uint32x4_t)tmp3);
    tmp4 = temp3.val[0];
    //print_bytes("tmp4:",(uint8_t*)&tmp4);

    *o_128++=vceqq_u8(vandq_u8((uint8x16_t)tmp4,BIT_MASK),BIT_MASK);    //3
    //print_bytes("o:",(uint8_t*)(o_128-1));

    tmp4 = temp3.val[1];
    //print_bytes("tmp4:",(uint8_t*)&tmp4);

    *o_128++=vceqq_u8(vandq_u8((uint8x16_t)tmp4,BIT_MASK),BIT_MASK);    //4
    //and_tmp = vandq_u8((uint8x16_t)tmp4,BIT_MASK); print_bytes("and:",and_tmp); 
    //print_bytes("o:",(uint8_t*)(o_128-1));


    temp1 =  vzipq_u8(tmp1,tmp1);
    tmp2 = temp1.val[1];
    temp2 =  vzipq_u16((uint16x8_t)tmp2,(uint16x8_t)tmp2);
    tmp3 = temp2.val[0];
    temp3 =  vzipq_u32((uint32x4_t)tmp3,(uint32x4_t)tmp3);
    tmp4 = temp3.val[0];
    //print_bytes("tmp4:",(uint8_t*)&tmp4);

    *o_128++=vceqq_u8(vandq_u8((uint8x16_t)tmp4,BIT_MASK),BIT_MASK);    //5
    //print_bytes("o:",(uint8_t*)(o_128-1));

    tmp4 = temp3.val[1];
    //print_bytes("tmp4:",(uint8_t*)&tmp4);

    *o_128++=vceqq_u8(vandq_u8((uint8x16_t)tmp4,BIT_MASK),BIT_MASK);    //6
    //print_bytes("o:",(uint8_t*)(o_128-1));


    temp2 =  vzipq_u16((uint16x8_t)tmp2,(uint16x8_t)tmp2);
    tmp3 = temp2.val[1];
    temp3 =  vzipq_u32((uint32x4_t)tmp3,(uint32x4_t)tmp3);
    tmp4 = temp3.val[0];
    //print_bytes("tmp4:",(uint8_t*)&tmp4);

    *o_128++=vceqq_u8(vandq_u8((uint8x16_t)tmp4,BIT_MASK),BIT_MASK);    //7
    //print_bytes("o:",(uint8_t*)(o_128-1));

    tmp4 = temp3.val[1];
    //print_bytes("tmp4:",(uint8_t*)&tmp4);

    *o_128++=vceqq_u8(vandq_u8((uint8x16_t)tmp4,BIT_MASK),BIT_MASK);    //7
    //print_bytes("o:",(uint8_t*)(o_128-1));

    i_128++;
#endif
  }

  short * ptr_intl=base_interleaver;
#if defined(__x86_64) || defined(__i386__)
  __m128i tmp;
 uint16_t *systematic2_ptr=(unsigned short *) output;
#elif defined(__arm__)
  uint8x16_t tmp;
  const uint8_t __attribute__ ((aligned (16))) _Powers[16]= 
    { 1, 2, 4, 8, 16, 32, 64, 128, 1, 2, 4, 8, 16, 32, 64, 128 };

// Set the powers of 2 (do it once for all, if applicable)
  uint8x16_t Powers= vld1q_u8(_Powers);
  uint8_t *systematic2_ptr=(uint8_t *) output;
#endif
  int input_length_words=n>>1;

  for ( i=0; i<  input_length_words ; i ++ ) {

#if defined(__x86_64__) || defined(__i386__)
    tmp=_mm_insert_epi8(tmp,expandInput[*ptr_intl++],7);
    tmp=_mm_insert_epi8(tmp,expandInput[*ptr_intl++],6);
    tmp=_mm_insert_epi8(tmp,expandInput[*ptr_intl++],5);
    tmp=_mm_insert_epi8(tmp,expandInput[*ptr_intl++],4);
    tmp=_mm_insert_epi8(tmp,expandInput[*ptr_intl++],3);
    tmp=_mm_insert_epi8(tmp,expandInput[*ptr_intl++],2);
    tmp=_mm_insert_epi8(tmp,expandInput[*ptr_intl++],1);
    tmp=_mm_insert_epi8(tmp,expandInput[*ptr_intl++],0);
    tmp=_mm_insert_epi8(tmp,expandInput[*ptr_intl++],8+7);
    tmp=_mm_insert_epi8(tmp,expandInput[*ptr_intl++],8+6);
    tmp=_mm_insert_epi8(tmp,expandInput[*ptr_intl++],8+5);
    tmp=_mm_insert_epi8(tmp,expandInput[*ptr_intl++],8+4);
    tmp=_mm_insert_epi8(tmp,expandInput[*ptr_intl++],8+3);
    tmp=_mm_insert_epi8(tmp,expandInput[*ptr_intl++],8+2);
    tmp=_mm_insert_epi8(tmp,expandInput[*ptr_intl++],8+1);
    tmp=_mm_insert_epi8(tmp,expandInput[*ptr_intl++],8+0);
    *systematic2_ptr++=(unsigned short)_mm_movemask_epi8(tmp);
#elif defined(__arm__)
    tmp=vsetq_lane_u8(expandInput[*ptr_intl++],tmp,7);
    tmp=vsetq_lane_u8(expandInput[*ptr_intl++],tmp,6);
    tmp=vsetq_lane_u8(expandInput[*ptr_intl++],tmp,5);
    tmp=vsetq_lane_u8(expandInput[*ptr_intl++],tmp,4);
    tmp=vsetq_lane_u8(expandInput[*ptr_intl++],tmp,3);
    tmp=vsetq_lane_u8(expandInput[*ptr_intl++],tmp,2);
    tmp=vsetq_lane_u8(expandInput[*ptr_intl++],tmp,1);
    tmp=vsetq_lane_u8(expandInput[*ptr_intl++],tmp,0);
    tmp=vsetq_lane_u8(expandInput[*ptr_intl++],tmp,8+7);
    tmp=vsetq_lane_u8(expandInput[*ptr_intl++],tmp,8+6);
    tmp=vsetq_lane_u8(expandInput[*ptr_intl++],tmp,8+5);
    tmp=vsetq_lane_u8(expandInput[*ptr_intl++],tmp,8+4);
    tmp=vsetq_lane_u8(expandInput[*ptr_intl++],tmp,8+3);
    tmp=vsetq_lane_u8(expandInput[*ptr_intl++],tmp,8+2);
    tmp=vsetq_lane_u8(expandInput[*ptr_intl++],tmp,8+1);
    tmp=vsetq_lane_u8(expandInput[*ptr_intl++],tmp,8+0);
// Compute the mask from the input
    uint64x2_t Mask= vpaddlq_u32(vpaddlq_u16(vpaddlq_u8(vandq_u8(tmp, Powers))));
    vst1q_lane_u8(systematic2_ptr++, (uint8x16_t)Mask, 0);
    vst1q_lane_u8(systematic2_ptr++, (uint8x16_t)Mask, 8);

#endif
  }

  return n;
}


/*
#define _mm_expand_si128(xmmx, out, bit_mask)   \
  {             \
   __m128i loc_mm;          \
   loc_mm=(xmmx);         \
   loc_mm=_mm_and_si128(loc_mm,bit_mask);   \
   out=_mm_cmpeq_epi8(loc_mm,bit_mask);   \
  }
*/

void threegpplte_turbo_encoder(unsigned char *input,
                               unsigned short input_length_bytes,
                               unsigned char *output,
                               unsigned char F,
                               unsigned short interleaver_f1,
                               unsigned short interleaver_f2)
{

  int i;
  unsigned char *x;
  unsigned char state0=0,state1=0;
  unsigned short input_length_bits = input_length_bytes<<3;
  short * base_interleaver;

  if (  all_treillis_initialized == 0 )
    treillis_table_init();

  // look for f1 and f2 precomputed interleaver values
  for (i=0; i < 188 && f1f2mat[i].nb_bits != input_length_bits; i++);

  if ( i == 188 ) {
    printf("Illegal frame length!\n");
    return;
  } else {
    base_interleaver=il_tb+f1f2mat[i].beg_index;
  }


  unsigned char systematic2[768];
  interleave_compact_byte(base_interleaver,input,systematic2,input_length_bytes);

#if defined(__x86_64__) || defined(__i386__)
  __m64 *ptr_output=(__m64*) output;
#elif defined(__arm__)
  uint8x8_t *ptr_output=(uint8x8_t*)output; 
#endif
  unsigned char cur_s1, cur_s2;
  int code_rate;

  for ( state0=state1=i=0 ; i<input_length_bytes; i++ ) {
    cur_s1=input[i];
    cur_s2=systematic2[i];

    for ( code_rate=0; code_rate<3; code_rate++) {
#if defined(__x86_64__) || defined(__i386__)
      *ptr_output++ = _mm_add_pi8(all_treillis[state0][cur_s1].systematic_64[code_rate],
                                  _mm_add_pi8(all_treillis[state0][cur_s1].parity1_64[code_rate],
                                              all_treillis[state1][cur_s2].parity2_64[code_rate]));
#elif defined(__arm__)
      uint8x8_t ptmp = vadd_u8(all_treillis[state0][cur_s1].parity1_64[code_rate],
                               all_treillis[state1][cur_s2].parity2_64[code_rate]);
      *ptr_output++ = vadd_u8(all_treillis[state0][cur_s1].systematic_64[code_rate],
                              ptmp);
#endif
    }

    state0=all_treillis[state0][cur_s1].exit_state;
    state1=all_treillis[state1][cur_s2].exit_state;
  }

  x=output+(input_length_bits*3);

  // Trellis termination
  threegpplte_rsc_termination(&x[0],&x[1],&state0);
#ifdef DEBUG_TURBO_ENCODER
  printf("term: x0 %d, x1 %d, state0 %d\n",x[0],x[1],state0);
#endif //DEBUG_TURBO_ENCODER

  threegpplte_rsc_termination(&x[2],&x[3],&state0);
#ifdef DEBUG_TURBO_ENCODER
  printf("term: x0 %d, x1 %d, state0 %d\n",x[2],x[3],state0);
#endif //DEBUG_TURBO_ENCODER

  threegpplte_rsc_termination(&x[4],&x[5],&state0);
#ifdef DEBUG_TURBO_ENCODER
  printf("term: x0 %d, x1 %d, state0 %d\n",x[4],x[5],state0);
#endif //DEBUG_TURBO_ENCODER

  threegpplte_rsc_termination(&x[6],&x[7],&state1);

#ifdef DEBUG_TURBO_ENCODER
  printf("term: x0 %d, x1 %d, state1 %d\n",x[6],x[7],state1);
#endif //DEBUG_TURBO_ENCODER
  threegpplte_rsc_termination(&x[8],&x[9],&state1);
#ifdef DEBUG_TURBO_ENCODER
  printf("term: x0 %d, x1 %d, state1 %d\n",x[8],x[9],state1);
#endif //DEBUG_TURBO_ENCODER
  threegpplte_rsc_termination(&x[10],&x[11],&state1);

#ifdef DEBUG_TURBO_ENCODER
  printf("term: x0 %d, x1 %d, state1 %d\n",x[10],x[11],state1);
#endif //DEBUG_TURBO_ENCODER
#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif
}



#ifdef TC_MAIN
#define INPUT_LENGTH 20 
#define F1 21
#define F2 120

int main(int argc,char **argv)
{

  unsigned char input[INPUT_LENGTH+16],state,state2;
  unsigned char output[12+(3*(INPUT_LENGTH<<3))],x,z;
  int i;
  unsigned char out;

  for (state=0; state<8; state++) {
    for (i=0; i<2; i++) {
      state2=state;
      out = threegpplte_rsc(i,&state2);
      printf("State (%d->%d) : (%d,%d)\n",state,state2,i,out);
    }
  }

  printf("\n");

  for (state=0; state<8; state++) {

    state2=state;
    threegpplte_rsc_termination(&x,&z,&state2);
    printf("Termination: (%d->%d) : (%d,%d)\n",state,state2,x,z);
  }

  memset((void*)input,0,INPUT_LENGTH+16);
  for (i=0; i<INPUT_LENGTH; i++) {
    input[i] = i*219;
    printf("Input %d : %x\n",i,input[i]);
  }

  threegpplte_turbo_encoder(&input[0],
                            INPUT_LENGTH,
                            &output[0],
                            0,
                            F1,
                            F2);


  for (i=0;i<12+(INPUT_LENGTH*24);i++)
    printf("%d",output[i]);
  printf("\n");

  return(0);
}

#endif // MAIN
