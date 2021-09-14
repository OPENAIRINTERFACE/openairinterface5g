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

#include "tools_defs.h"
#include "PHY/impl_defs_top.h"
#include "PHY/sse_intrin.h"

// Compute Energy of a complex signal vector, removing the DC component!
// input  : points to vector
// length : length of vector in complex samples

#define shift 4
//#define shift_DC 0
#define SHRT_MIN -32768

#if defined(__x86_64__) || defined(__i386__)
#ifdef LOCALIZATION
int32_t subcarrier_energy(int32_t *input,uint32_t length, int32_t *subcarrier_energy, uint16_t rx_power_correction)
{

  int32_t i, subcarrier_pwr;
  register __m64 mm0,mm1, subcarrier;
  subcarrier = _mm_setzero_si64();//_m_pxor(subcarrier,subcarrier);
  __m64 *in = (__m64 *)input;

#ifdef MAIN
  int16_t *printb;
#endif

  mm0 = _mm_setzero_si64();//pxor(mm0,mm0);

  for (i=0; i<length>>1; i++) {

    mm1 = in[i];
    mm1 = _m_pmaddwd(mm1,mm1);
    mm1 = _m_psradi(mm1,shift);// shift any 32 bits blocs of the word by the value shift
    subcarrier = mm1;
    subcarrier = _m_psrlqi(subcarrier,32);
    subcarrier = _m_paddd(subcarrier,mm1);
    subcarrier_pwr = _m_to_int(subcarrier);
    subcarrier_pwr<<=shift;
    subcarrier_pwr = (unsigned short) dB_fixed(subcarrier_pwr);
    subcarrier_energy[i] = subcarrier_pwr*rx_power_correction;
  }

  _mm_empty();
  _m_empty();

  return i;
}
#endif

//-----------------------------------------------------------------
// Average Power calculation with DC removing
//-----------------------------------------------------------------
int32_t signal_energy(int32_t *input,uint32_t length)
{
  uint32_t i;
  int32_t temp;
  __m128i in, in_clp, i16_min, coe1;
  __m128 num0, num1, num2, num3, recp1;

  //init
  num0 = _mm_setzero_ps();
  num1 = _mm_setzero_ps();
  i16_min = _mm_set1_epi16(SHRT_MIN);
  coe1 = _mm_set1_epi16(1);
  recp1 = _mm_rcp_ps(_mm_cvtepi32_ps(_mm_set1_epi32(length)));

  //Acc
  for (i = 0; i < (length >> 2); i++) {
    in = _mm_loadu_si128((__m128i *)input);
    in_clp = _mm_subs_epi16(in, _mm_cmpeq_epi16(in, i16_min));//if in=SHRT_MIN in+1, else in
    num0 = _mm_add_ps(num0, _mm_cvtepi32_ps(_mm_madd_epi16(in_clp, in_clp)));
    num1 = _mm_add_ps(num1, _mm_cvtepi32_ps(_mm_madd_epi16(in, coe1)));//DC
    input += 4;
  }
  //Ave
  num2 = _mm_dp_ps(num0, recp1, 0xFF);//AC power
  num3 = _mm_dp_ps(num1, recp1, 0xFF);//DC
  num3 = _mm_mul_ps(num3, num3);      //DC power
  //remove DC
  temp = _mm_cvtsi128_si32(_mm_cvttps_epi32(_mm_sub_ps(num2, num3)));

  return temp;
}

int32_t signal_energy_amp_shift(int32_t *input,uint32_t length)
{

  int32_t i;
  int32_t temp,temp2;
  register __m64 mm0,mm1,mm2,mm3;
  __m64 *in = (__m64 *)input;

  mm0 = _mm_setzero_si64();
  mm3 = _mm_setzero_si64();

  for (i=0; i<length>>1; i++) {

    mm1 = in[i];
    mm2 = mm1;
    mm1 = _m_pmaddwd(mm1,mm1);
    mm1 = _m_psradi(mm1,AMP_SHIFT);// shift any 32 bits blocs of the word by the value shift_p9
    mm0 = _m_paddd(mm0,mm1);// add the two 64 bits words 4 bytes by 4 bytes
    mm3 = _m_paddw(mm3,mm2);// add the two 64 bits words 2 bytes by 2 bytes
  }

  mm1 = mm0;
  mm0 = _m_psrlqi(mm0,32);
  mm0 = _m_paddd(mm0,mm1);
  temp = _m_to_int(mm0);
  temp/=length; // this is the average of x^2


  // now remove the DC component


  mm2 = _m_psrlqi(mm3,32);
  mm2 = _m_paddw(mm2,mm3);
  mm2 = _m_pmaddwd(mm2,mm2);
  mm2 = _m_psradi(mm2,AMP_SHIFT); // fixed point representation of elements
  temp2 = _m_to_int(mm2);
  temp2/=(length*length);

  temp -= temp2;

  _mm_empty();
  _m_empty();

  return((temp>0)?temp:1);
}

int32_t signal_energy_nodc(int32_t *input,uint32_t length)
{
  int32_t i;
  int32_t temp;

  __m128i in;
  __m128  mm0;

//init
  mm0 = _mm_setzero_ps();
//Acc
  for (i=0; i<(length>>2); i++) {
    in = _mm_loadu_si128((__m128i *)input);
    mm0 = _mm_add_ps(mm0,_mm_cvtepi32_ps(_mm_madd_epi16(in,in)));
    input += 4;
  }
  //Ave
  temp = (int)((((float*)&mm0)[0] +
                 ((float*)&mm0)[1] +
                 ((float*)&mm0)[2] +
                 ((float*)&mm0)[3])/(float)length);

  return temp;
}

#elif defined(__arm__)

int32_t signal_energy(int32_t *input,uint32_t length)
{

  int32_t i;
  int32_t temp,temp2;
  register int32x4_t tmpE,tmpDC;
  int32x2_t tmpE2,tmpDC2;
  int16x4_t *in = (int16x4_t *)input;

  tmpE  = vdupq_n_s32(0);
  tmpDC = vdupq_n_s32(0);

  for (i=0; i<length>>1; i++) {

    tmpE = vqaddq_s32(tmpE,vshrq_n_s32(vmull_s16(*in,*in),shift));
    //tmpDC = vaddw_s16(tmpDC,vshr_n_s16(*in++,shift_DC));

  }

  tmpE2 = vpadd_s32(vget_low_s32(tmpE),vget_high_s32(tmpE));

  temp=(vget_lane_s32(tmpE2,0)+vget_lane_s32(tmpE2,1))/length;
  temp<<=shift;   // this is the average of x^2

  // now remove the DC component


  tmpDC2 = vpadd_s32(vget_low_s32(tmpDC),vget_high_s32(tmpDC));

  temp2=(vget_lane_s32(tmpDC2,0)+vget_lane_s32(tmpDC2,1))/(length*length);

  //  temp2<<=(2*shift_DC);
#ifdef MAIN
  printf("E x^2 = %d\n",temp);
#endif
  temp -= temp2;
#ifdef MAIN
  printf("(E x)^2=%d\n",temp2);
#endif

  return((temp>0)?temp:1);
}

int32_t signal_energy_nodc(int32_t *input,uint32_t length)
{

  int32_t i;
  int32_t temp;
  register int32x4_t tmpE;
  int32x2_t tmpE2;
  int16x4_t *in = (int16x4_t *)input;

  tmpE = vdupq_n_s32(0);

  for (i=0; i<length>>1; i++) {

    tmpE = vqaddq_s32(tmpE,vshrq_n_s32(vmull_s16(*in,*in),shift));

  }

  tmpE2 = vpadd_s32(vget_low_s32(tmpE),vget_high_s32(tmpE));

  temp=(vget_lane_s32(tmpE2,0)+vget_lane_s32(tmpE2,1))/length;
  temp<<=shift;   // this is the average of x^2

#ifdef MAIN
  printf("E x^2 = %d\n",temp);
#endif

  return((temp>0)?temp:1);
}

#endif
double signal_energy_fp(double *s_re[2],double *s_im[2],uint32_t nb_antennas,uint32_t length,uint32_t offset)
{

  int32_t aa,i;
  double V=0.0;

  for (i=0; i<length; i++) {
    for (aa=0; aa<nb_antennas; aa++) {
      V= V + (s_re[aa][i+offset]*s_re[aa][i+offset]) + (s_im[aa][i+offset]*s_im[aa][i+offset]);
    }
  }

  return(V/length/nb_antennas);
}

double signal_energy_fp2(struct complexd *s,uint32_t length)
{

  int32_t i;
  double V=0.0;

  for (i=0; i<length; i++) {
    //    printf("signal_energy_fp2 : %f,%f => %f\n",s[i].x,s[i].y,V);
    //      V= V + (s[i].y*s[i].x) + (s[i].y*s[i].x);
    V= V + (s[i].r*s[i].r) + (s[i].i*s[i].i);
  }

  return(V/length);
}

#ifdef MAIN
#define LENGTH 256
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
main(int argc,char **argv)
{

  int input[LENGTH];
  int energy=0,dc_r=0,dc_i=0;
  int16_t s=1,i;
  int amp;

  amp = atoi(argv[1]);// arguments to integer

  if (argc>1)
    printf("Amp = %d\n",amp);

  for (i=0; i<LENGTH; i++) {
    s = -s;
    ((int16_t*)input)[2*i]     = 31 + (int16_t)(amp*sin(2*M_PI*i/LENGTH));
    ((int16_t*)input)[1+(2*i)] = 30 + (int16_t)(amp*cos(2*M_PI*i/LENGTH));
    energy += (((int16_t*)input)[2*i]*((int16_t*)input)[2*i]) + (((int16_t*)input)[1+(2*i)]*((int16_t*)input)[1+(2*i)]);
    dc_r += ((int16_t*)input)[2*i];
    dc_i += ((int16_t*)input)[1+(2*i)];


  }

  energy/=LENGTH;
  dc_r/=LENGTH;
  dc_i/=LENGTH;

  printf("signal_energy = %d dB(%d,%d,%d,%d)\n",dB_fixed(signal_energy(input,LENGTH)),signal_energy(input,LENGTH),energy-(dc_r*dc_r+dc_i*dc_i),energy,(dc_r*dc_r+dc_i*dc_i));
  printf("dc = (%d,%d)\n",dc_r,dc_i);
}
#endif

#define SHRT_MIN -32768
int32_t signal_power(int32_t *input, uint32_t length)
{

  uint32_t i;
  int32_t temp;

  __m128i in, in_clp, i16_min;
  __m128  num0, num1;
  __m128  recp1;

  //init
  num0 = _mm_setzero_ps();
  i16_min = _mm_set1_epi16(SHRT_MIN);
  recp1 = _mm_rcp_ps(_mm_cvtepi32_ps(_mm_set1_epi32(length)));
  //Acc
  for (i = 0; i < (length >> 2); i++) {
    in = _mm_loadu_si128((__m128i *)input);
    in_clp = _mm_subs_epi16(in, _mm_cmpeq_epi16(in, i16_min));//if in=SHRT_MIN in+1, else in
    num0 = _mm_add_ps(num0, _mm_cvtepi32_ps(_mm_madd_epi16(in_clp, in_clp)));
    input += 4;
  }
  //Ave
  num1 = _mm_dp_ps(num0, recp1, 0xFF);
  temp = _mm_cvtsi128_si32(_mm_cvttps_epi32(num1));

  return temp;
}

int32_t interference_power(int32_t *input, uint32_t length)
{

  uint32_t i;
  int32_t temp;

  __m128i in, in_clp, i16_min;
  __m128i num0, num1, num2, num3;
  __m128  num4, num5, num6;
  __m128  recp1;

  //init
  i16_min = _mm_set1_epi16(SHRT_MIN);
  num5 = _mm_setzero_ps();
  recp1 = _mm_rcp_ps(_mm_cvtepi32_ps(_mm_set1_epi32(length>>2)));// 1/n, n= length/4
  //Acc
  for (i = 0; i < (length >> 2); i++) {
    in = _mm_loadu_si128((__m128i *)input);
    in_clp = _mm_subs_epi16(in, _mm_cmpeq_epi16(in, i16_min));           //if in=SHRT_MIN, in+1, else in
    num0 = _mm_cvtepi16_epi32(in_clp);                                   //lower 2 complex [0], [1]
    num1 = _mm_cvtepi16_epi32(_mm_shuffle_epi32(in_clp, 0x4E));          //upper 2 complex [2], [3]
    num2 = _mm_srai_epi32(_mm_add_epi32(num0, num1), 0x01);              //average A=complex( [0] + [2] ) / 2, B=complex( [1] + [3] ) / 2 
    num3 = _mm_sub_epi32(num2, _mm_shuffle_epi32(num2, 0x4E));           //complexA-complexB, B-A
    num4 = _mm_dp_ps(_mm_cvtepi32_ps(num3), _mm_cvtepi32_ps(num3), 0x3F);//C = num3 lower complex power, C, C, C
    num5 = _mm_add_ps(num5, num4);                                       //Acc Cn, Cn, Cn, Cn, 
    input += 4;
  }
  //Interference ve
  num6 = _mm_mul_ps(num5, recp1); //Cn / n
  temp = _mm_cvtsi128_si32(_mm_cvttps_epi32(num6));

  return temp;
}

