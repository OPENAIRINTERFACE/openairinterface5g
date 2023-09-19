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

/* file: 3gpplte_sse.c
   purpose: Encoding routines for implementing Turbo-coded (DLSCH) transport channels from 36-212, V8.6 2009-03
   author: Laurent Thomas
   maintainer: raymond.knopp@eurecom.fr
   date: 09.2012
*/
#ifndef TC_MAIN
  #include "coding_defs.h"
#else
  #include <stdint.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "PHY/sse_intrin.h"

#define print_bytes(s,x) printf("%s %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",s,(x)[0],(x)[1],(x)[2],(x)[3],(x)[4],(x)[5],(x)[6],(x)[7],(x)[8],(x)[9],(x)[10],(x)[11],(x)[12],(x)[13],(x)[14],(x)[15])
#define print_shorts(s,x) printf("%s %x,%x,%x,%x,%x,%x,%x,%x\n",s,(x)[0],(x)[1],(x)[2],(x)[3],(x)[4],(x)[5],(x)[6],(x)[7])
#define print_ints(s,x) printf("%s %x %x %x %x\n",s,(x)[0],(x)[1],(x)[2],(x)[3])

#define print_bytes2(s,x) printf("%s %x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x,%x\n",s,(x)[0],(x)[1],(x)[2],(x)[3],(x)[4],(x)[5],(x)[6],(x)[7],(x)[8],(x)[9],(x)[10],(x)[11],(x)[12],(x)[13],(x)[14],(x)[15],(x)[16],(x)[17],(x)[18],(x)[19],(x)[20],(x)[21],(x)[22],(x)[23],(x)[24],(x)[25],(x)[26],(x)[27],(x)[28],(x)[29],(x)[30],(x)[31])

//#define DEBUG_TURBO_ENCODER 1
//#define CALLGRIND 1

struct treillis {
  union {
    simde__m64 systematic_andp1_64[3];
    uint8_t systematic_andp1_8[24];
  };
  union {
    simde__m64 parity2_64[3];
    uint8_t parity2_8[24];
  };
  int exit_state;
}  __attribute__ ((aligned(64)));

struct treillis all_treillis[8][256];

int all_treillis_initialized=0;

static inline unsigned char threegpplte_rsc(unsigned char input,unsigned char *state) {
  unsigned char output;
  output = (input ^ (*state>>2) ^ (*state>>1))&1;
  *state = (((input<<2)^(*state>>1))^((*state>>1)<<2)^((*state)<<2))&7;
  return(output);
}

static inline void threegpplte_rsc_termination(unsigned char *x,unsigned char *z,unsigned char *state) {
  *z     = ((*state>>2) ^ (*state))   &1;
  *x     = ((*state)    ^ (*state>>1))   &1;
  *state = (*state)>>1;
}

static void treillis_table_init(void) {
  //struct treillis t[][]=all_treillis;
  //t=memalign(16,sizeof(struct treillis)*8*256);
  int i, j,b;
  unsigned char v, current_state;

  // clear all_treillis
  for (i=0; i<8; i++) {
    bzero( all_treillis[i], sizeof(all_treillis[0]) );
  }

  for (i=0; i<8; i++) { //all possible initial states
    for (j=0; j<=255; j++) { // all possible values of a byte
      current_state=i;

      for (b=0; b<8 ; b++ ) { // pre-compute the image of the byte j in _m128i vector right place
        all_treillis[i][j].systematic_andp1_8[b*3]= (j&(1<<(7-b)))>>(7-b);
        v=threegpplte_rsc( all_treillis[i][j].systematic_andp1_8[b*3] ,
                           &current_state);
        all_treillis[i][j].systematic_andp1_8[b*3+1]=v; // for the yparity1
        //        all_treillis[i][j].parity1_8[b*3+1]=v; // for the yparity1
        all_treillis[i][j].parity2_8[b*3+2]=v; // for the yparity2
      }

      all_treillis[i][j].exit_state=current_state;
    }
  }

  all_treillis_initialized=1;
  return ;
}


char interleave_compact_byte(short *base_interleaver,unsigned char *input, unsigned char *output, int n) {
  char expandInput[768*8] __attribute__((aligned(32)));
  int i, loop = n >> 4;
  simde__m256i *i_256 = (simde__m256i *)input, *o_256 = (simde__m256i *)expandInput;
  simde__m256i tmp1, tmp2, tmp3, tmp4, tmp5, tmp6, tmp7;
  simde__m256i BIT_MASK = simde_mm256_set_epi8(0b00000001,
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
                                               0b10000000,
                                               0b00000001,
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
  loop=n>>5;

  if ((n&31) > 0)
    loop++;

  for (i=0; i<loop ; i++ ) {
    // int cur_byte=i<<3;
    // for (b=0;b<8;b++)
    //   expandInput[cur_byte+b] = (input[i]&(1<<(7-b)))>>(7-b);
    tmp1=simde_mm256_load_si256(i_256++);       // tmp1 = B0,B1,...,B15,...,B31
    //print_bytes2("in",(uint8_t*)&tmp1);
    tmp2=simde_mm256_unpacklo_epi8(tmp1,tmp1);  // tmp2 = B0,B0,B1,B1,...,B7,B7,B16,B16,B17,B17,...,B23,B23
    tmp3=simde_mm256_unpacklo_epi16(tmp2,tmp2); // tmp3 = B0,B0,B0,B0,B1,B1,B1,B1,B2,B2,B2,B2,B3,B3,B3,B3,B16,B16,B16,B16,...,B19,B19,B19,B19
    tmp4=simde_mm256_unpacklo_epi32(tmp3,tmp3); // tmp4 - B0,B0,B0,B0,B0,B0,B0,B0,B1,B1,B1,B1,B1,B1,B1,B1,B16,B16...,B17..,B17
    tmp5=simde_mm256_unpackhi_epi32(tmp3,tmp3); // tmp5 - B2,B2,B2,B2,B2,B2,B2,B2,B3,B3,B3,B3,B3,B3,B3,B3,B18...,B18,B19,...,B19
    tmp6=simde_mm256_insertf128_si256(tmp4,simde_mm256_extracti128_si256(tmp5,0),1);  // tmp6 = B0 B1 B2 B3
    tmp7=simde_mm256_insertf128_si256(tmp5,simde_mm256_extracti128_si256(tmp4,1),0);  // tmp7 = B16 B17 B18 B19
    //print_bytes2("tmp2",(uint8_t*)&tmp2);
    //print_bytes2("tmp3",(uint8_t*)&tmp3);
    //print_bytes2("tmp4",(uint8_t*)&tmp4);
    //print_bytes2("tmp5",(uint8_t*)&tmp4);
    //print_bytes2("tmp6",(uint8_t*)&tmp6);
    //print_bytes2("tmp7",(uint8_t*)&tmp7);
    o_256[0]=simde_mm256_cmpeq_epi8(simde_mm256_and_si256(tmp6,BIT_MASK),BIT_MASK);
    //print_bytes2("out",(uint8_t*)o_256);
    o_256[4]=simde_mm256_cmpeq_epi8(simde_mm256_and_si256(tmp7,BIT_MASK),BIT_MASK);;
    //print_bytes2("out",(uint8_t*)(o_256+4));
    tmp3=simde_mm256_unpackhi_epi16(tmp2,tmp2); // tmp3 = B4,B4,B4,B4,B5,B5,B5,B5,B6,B6,B6,B6,B7,B7,B7,B7,B20,B20,B20,B20,...,B23,B23,B23,B23
    tmp4=simde_mm256_unpacklo_epi32(tmp3,tmp3); // tmp4 - B4,B4,B4,B4,B4,B4,B4,B4,B5,B5,B5,B5,B5,B5,B5,B5,B20,B20...,B21..,B21
    tmp5=simde_mm256_unpackhi_epi32(tmp3,tmp3); // tmp5 - B6,B6,B6,B6,B6,B6,B6,B6,B7,B7,B7,B7,B7,B7,B7,B7,B22...,B22,B23,...,B23
    tmp6=simde_mm256_insertf128_si256(tmp4,simde_mm256_extracti128_si256(tmp5,0),1);  // tmp6 = B4 B5 B6 B7
    tmp7=simde_mm256_insertf128_si256(tmp5,simde_mm256_extracti128_si256(tmp4,1),0);  // tmp7 = B20 B21 B22 B23
    //print_bytes2("tmp2",(uint8_t*)&tmp2);
    //print_bytes2("tmp3",(uint8_t*)&tmp3);
    //print_bytes2("tmp4",(uint8_t*)&tmp4);
    //print_bytes2("tmp5",(uint8_t*)&tmp4);
    //print_bytes2("tmp6",(uint8_t*)&tmp6);
    //print_bytes2("tmp7",(uint8_t*)&tmp7);
    o_256[1]=simde_mm256_cmpeq_epi8(simde_mm256_and_si256(tmp6,BIT_MASK),BIT_MASK);
    //print_bytes2("out",(uint8_t*)(o_256+1));
    o_256[5]=simde_mm256_cmpeq_epi8(simde_mm256_and_si256(tmp7,BIT_MASK),BIT_MASK);;
    //print_bytes2("out",(uint8_t*)(o_256+4));
    tmp2=simde_mm256_unpackhi_epi8(tmp1,tmp1);  // tmp2 = B8 B9 B10 B11 B12 B13 B14 B15 B25 B26 B27 B28 B29 B30 B31
    tmp3=simde_mm256_unpacklo_epi16(tmp2,tmp2); // tmp3 = B8,B9,B10,B11,B26,B27,B28,B29
    tmp4=simde_mm256_unpacklo_epi32(tmp3,tmp3); // tmp4 - B8,B9,B26,B27
    tmp5=simde_mm256_unpackhi_epi32(tmp3,tmp3); // tmp5 - B10,B11,B28,B29
    tmp6=simde_mm256_insertf128_si256(tmp4,simde_mm256_extracti128_si256(tmp5,0),1);  // tmp6 = B8 B9 B10 B11
    tmp7=simde_mm256_insertf128_si256(tmp5,simde_mm256_extracti128_si256(tmp4,1),0);  // tmp7 = B26 B27 B28 B29
    //print_bytes2("tmp2",(uint8_t*)&tmp2);
    //print_bytes2("tmp3",(uint8_t*)&tmp3);
    //print_bytes2("tmp4",(uint8_t*)&tmp4);
    //print_bytes2("tmp5",(uint8_t*)&tmp4);
    //print_bytes2("tmp6",(uint8_t*)&tmp6);
    //print_bytes2("tmp7",(uint8_t*)&tmp7);
    o_256[2]=simde_mm256_cmpeq_epi8(simde_mm256_and_si256(tmp6,BIT_MASK),BIT_MASK);
    //print_bytes2("out",(uint8_t*)(o_256+2));
    o_256[6]=simde_mm256_cmpeq_epi8(simde_mm256_and_si256(tmp7,BIT_MASK),BIT_MASK);;
    //print_bytes2("out",(uint8_t*)(o_256+4));
    tmp3=simde_mm256_unpackhi_epi16(tmp2,tmp2); // tmp3 = B12 B13 B14 B15 B28 B29 B30 B31
    tmp4=simde_mm256_unpacklo_epi32(tmp3,tmp3); // tmp4 = B12 B13 B28 B29
    tmp5=simde_mm256_unpackhi_epi32(tmp3,tmp3); // tmp5 = B14 B15 B30 B31
    tmp6=simde_mm256_insertf128_si256(tmp4,simde_mm256_extracti128_si256(tmp5,0),1);  // tmp6 = B12 B13 B14 B15
    tmp7=simde_mm256_insertf128_si256(tmp5,simde_mm256_extracti128_si256(tmp4,1),0);  // tmp7 = B28 B29 B30 B31
    //print_bytes2("tmp2",(uint8_t*)&tmp2);
    //print_bytes2("tmp3",(uint8_t*)&tmp3);
    //print_bytes2("tmp4",(uint8_t*)&tmp4);
    //print_bytes2("tmp5",(uint8_t*)&tmp4);
    //print_bytes2("tmp6",(uint8_t*)&tmp6);
    //print_bytes2("tmp7",(uint8_t*)&tmp7);
    o_256[3]=simde_mm256_cmpeq_epi8(simde_mm256_and_si256(tmp6,BIT_MASK),BIT_MASK);
    //print_bytes2("out",(uint8_t*)(o_256+3));
    o_256[7]=simde_mm256_cmpeq_epi8(simde_mm256_and_si256(tmp7,BIT_MASK),BIT_MASK);;
    //print_bytes2("out",(uint8_t*)(o_256+7));
    o_256 += 8;
  }

  short *ptr_intl = base_interleaver;
  simde__m256i tmp = {0};
  uint32_t *systematic2_ptr = (uint32_t *)output;
  int input_length_words=1+((n-1)>>2);

  for (i = 0; i < input_length_words; i++) {
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],7);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],6);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],5);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],4);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],3);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],2);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],1);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],0);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],8+7);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],8+6);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],8+5);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],8+4);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],8+3);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],8+2);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],8+1);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],8+0);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],16+7);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],16+6);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],16+5);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],16+4);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],16+3);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],16+2);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],16+1);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],16+0);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],24+7);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],24+6);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],24+5);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],24+4);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],24+3);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],24+2);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],24+1);
    tmp=simde_mm256_insert_epi8(tmp,expandInput[*ptr_intl++],24+0);
    *systematic2_ptr++ = (unsigned int)simde_mm256_movemask_epi8(tmp);
  }

  return n;
}

/*
#define simde_mm_expand_si128(xmmx, out, bit_mask)   \
  {             \
   simde__m128i loc_mm;          \
   loc_mm=(xmmx);         \
   loc_mm=simde_mm_and_si128(loc_mm,bit_mask);   \
   out=simde_mm_cmpeq_epi8(loc_mm,bit_mask);   \
  }
*/

void threegpplte_turbo_encoder_sse(unsigned char *input,
                                   unsigned short input_length_bytes,
                                   unsigned char *output,
                                   unsigned char F) {
  int i;
  unsigned char *x;
  unsigned char state0=0,state1=0;
  unsigned short input_length_bits = input_length_bytes<<3;
  short *base_interleaver;

  if (  all_treillis_initialized == 0 ) {
    treillis_table_init();
  }

  // look for f1 and f2 precomputed interleaver values
  for (i=0; i < 188 && f1f2mat[i].nb_bits != input_length_bits; i++);

  if ( i == 188 ) {
    printf("Illegal frame length!\n");
    return;
  } else {
    base_interleaver=il_tb+f1f2mat[i].beg_index;
  }

  unsigned char systematic2[768] __attribute__((aligned(32)));
  interleave_compact_byte(base_interleaver, input, systematic2, input_length_bytes);
  simde__m64 *ptr_output = (simde__m64 *)output;
  unsigned char cur_s1, cur_s2;
  int code_rate;

  for ( state0=state1=i=0 ; i<input_length_bytes; i++ ) {
    cur_s1=input[i];
    cur_s2=systematic2[i];

    for (code_rate = 0; code_rate < 3; code_rate++) {
      /*
       *ptr_output++ = simde_mm_add_pi8(all_treillis[state0][cur_s1].systematic_64[code_rate],
       simde_mm_add_pi8(all_treillis[state0][cur_s1].parity1_64[code_rate],
      all_treillis[state1][cur_s2].parity2_64[code_rate]));
      */
      *ptr_output++ = simde_mm_add_pi8(all_treillis[state0][cur_s1].systematic_andp1_64[code_rate],
                                       all_treillis[state1][cur_s2].parity2_64[code_rate]);
    }

    state0=all_treillis[state0][cur_s1].exit_state;
    state1=all_treillis[state1][cur_s2].exit_state;
  }

  x=output+(input_length_bits*3);
  // Trellis termination
  threegpplte_rsc_termination(&x[0],&x[1],&state0);
#ifdef DEBUG_TURBO_ENCODER
  printf("term: x0 %u, x1 %u, state0 %d\n",x[0],x[1],state0);
#endif //DEBUG_TURBO_ENCODER
  threegpplte_rsc_termination(&x[2],&x[3],&state0);
#ifdef DEBUG_TURBO_ENCODER
  printf("term: x0 %u, x1 %u, state0 %d\n",x[2],x[3],state0);
#endif //DEBUG_TURBO_ENCODER
  threegpplte_rsc_termination(&x[4],&x[5],&state0);
#ifdef DEBUG_TURBO_ENCODER
  printf("term: x0 %u, x1 %u, state0 %d\n",x[4],x[5],state0);
#endif //DEBUG_TURBO_ENCODER
  threegpplte_rsc_termination(&x[6],&x[7],&state1);
#ifdef DEBUG_TURBO_ENCODER
  printf("term: x0 %u, x1 %u, state1 %d\n",x[6],x[7],state1);
#endif //DEBUG_TURBO_ENCODER
  threegpplte_rsc_termination(&x[8],&x[9],&state1);
#ifdef DEBUG_TURBO_ENCODER
  printf("term: x0 %u, x1 %u, state1 %d\n",x[8],x[9],state1);
#endif //DEBUG_TURBO_ENCODER
  threegpplte_rsc_termination(&x[10],&x[11],&state1);
#ifdef DEBUG_TURBO_ENCODER
  printf("term: x0 %u, x1 %u, state1 %d\n",x[10],x[11],state1);
#endif // DEBUG_TURBO_ENCODER
  simde_mm_empty();
  simde_m_empty();
}

void init_encoder_sse (void) {
  treillis_table_init();
}
/* function which will be called by the shared lib loader, to check shared lib version
   against main exec version. version mismatch no considered as fatal (interfaces not supposed to change)
*/
int  coding_checkbuildver(char *mainexec_buildversion, char **shlib_buildversion) {
#ifndef PACKAGE_VERSION
#define PACKAGE_VERSION "standalone built: " __DATE__ __TIME__
#endif
  *shlib_buildversion = PACKAGE_VERSION;

  if (strcmp(mainexec_buildversion, *shlib_buildversion) != 0) {
    fprintf(stderr,"[CODING] shared lib version %s, doesn't match main version %s, compatibility should be checked\n",
            mainexec_buildversion,*shlib_buildversion);
  }

  return 0;
}

#ifdef TC_MAIN
#define INPUT_LENGTH 20
#define F1 21
#define F2 120

int main(int argc,char **argv) {
  unsigned char input[INPUT_LENGTH+32],state,state2;
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

  memset((void *)input,0,INPUT_LENGTH+16);

  for (i=0; i<INPUT_LENGTH; i++) {
    input[i] = i*219;
    printf("Input %d : %u\n",i,input[i]);
  }

  threegpplte_turbo_encoder_sse(&input[0],
                                INPUT_LENGTH,
                                &output[0],
                                0);

  for (i=0; i<12+(INPUT_LENGTH*24); i++)
    printf("%u",output[i]);

  printf("\n");
  return(0);
}

#endif // MAIN
