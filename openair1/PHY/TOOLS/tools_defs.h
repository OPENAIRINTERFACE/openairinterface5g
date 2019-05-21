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

#ifndef __PHY_TOOLS_DEFS__H__
#define __PHY_TOOLS_DEFS__H__

/** @addtogroup _PHY_DSP_TOOLS_


* @{

*/

#include <stdint.h>
#include "PHY/sse_intrin.h"

#define CEILIDIV(a,b) ((a+b-1)/b)
#define ROUNDIDIV(a,b) (((a<<1)+b)/(b<<1))

struct complex {
  double x;
  double y;
};

struct complexf {
  float r;
  float i;
};

struct complex16 {
  int16_t r;
  int16_t i;
};

struct complex32 {
  int32_t r;
  int32_t i;
};

//cmult_sv.h

/*!\fn void multadd_real_vector_complex_scalar(int16_t *x,int16_t *alpha,int16_t *y,uint32_t N)
This function performs componentwise multiplication and accumulation of a complex scalar and a real vector.
@param x Vector input (Q1.15)
@param alpha Scalar input (Q1.15) in the format  |Re0 Im0|
@param y Output (Q1.15) in the format  |Re0  Im0 Re1 Im1|,......,|Re(N-1)  Im(N-1) Re(N-1) Im(N-1)|
@param N Length of x WARNING: N>=8

The function implemented is : \f$\mathbf{y} = y + \alpha\mathbf{x}\f$
*/
void multadd_real_vector_complex_scalar(int16_t *x,
                                        int16_t *alpha,
                                        int16_t *y,
                                        uint32_t N
                                       );

/*!\fn void multadd_complex_vector_real_scalar(int16_t *x,int16_t alpha,int16_t *y,uint8_t zero_flag,uint32_t N)
This function performs componentwise multiplication and accumulation of a real scalar and a complex vector.
@param x Vector input (Q1.15) in the format |Re0 Im0|Re1 Im 1| ...
@param alpha Scalar input (Q1.15) in the format  |Re0|
@param y Output (Q1.15) in the format  |Re0  Im0 Re1 Im1|,......,|Re(N-1)  Im(N-1) Re(N-1) Im(N-1)|
@param zero_flag Set output (y) to zero prior to accumulation
@param N Length of x WARNING: N>=8

The function implemented is : \f$\mathbf{y} = y + \alpha\mathbf{x}\f$
*/
void multadd_complex_vector_real_scalar(int16_t *x,
                                        int16_t alpha,
                                        int16_t *y,
                                        uint8_t zero_flag,
                                        uint32_t N);

int rotate_cpx_vector(int16_t *x,
                      int16_t *alpha,
                      int16_t *y,
                      uint32_t N,
                      uint16_t output_shift);




/*!\fn void init_fft(uint16_t size,uint8_t logsize,uint16_t *rev)
\brief Initialize the FFT engine for a given size
@param size Size of the FFT
@param logsize log2(size)
@param rev Pointer to bit-reversal permutation array
*/

//cmult_vv.c
/*!
  Multiply elementwise the complex conjugate of x1 with x2. 
  @param x1       - input 1    in the format  |Re0 Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
              We assume x1 with a dinamic of 15 bit maximum
  @param x2       - input 2    in the format  |Re0 Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
              We assume x2 with a dinamic of 14 bit maximum
  @param y        - output     in the format  |Re0 Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
  @param N        - the size f the vectors (this function does N cpx mpy. WARNING: N>=4;
  @param output_shift  - shift to be applied to generate output
  @param madd - if not zero result is added to output
*/

int mult_cpx_conj_vector(int16_t *x1,
                         int16_t *x2,
                         int16_t *y,
                         uint32_t N,
                         int output_shift,
			 int madd);

/*!
  Element-wise multiplication and accumulation of two complex vectors x1 and x2.
  @param x1       - input 1    in the format  |Re0 Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
              We assume x1 with a dinamic of 15 bit maximum
  @param x2       - input 2    in the format  |Re0 Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
              We assume x2 with a dinamic of 14 bit maximum
  @param y        - output     in the format  |Re0 Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
  @param zero_flag Set output (y) to zero prior to accumulation
  @param N        - the size f the vectors (this function does N cpx mpy. WARNING: N>=4;
  @param output_shift  - shift to be applied to generate output
*/

int multadd_cpx_vector(int16_t *x1,
                    int16_t *x2,
                    int16_t *y,
                    uint8_t zero_flag,
                    uint32_t N,
		       int output_shift);

int mult_cpx_vector(int16_t *x1,
                    int16_t  *x2,
                    int16_t *y,
                    uint32_t N,
                    int output_shift);

// lte_dfts.c
void init_fft(uint16_t size,
              uint8_t logsize,
              uint16_t *rev);

/*!\fn void fft(int16_t *x,int16_t *y,int16_t *twiddle,uint16_t *rev,uint8_t log2size,uint8_t scale,uint8_t input_fmt)
This function performs optimized fixed-point radix-2 FFT/IFFT.
@param x Input
@param y Output in format: [Re0,Im0,Re0,Im0, Re1,Im1,Re1,Im1, ....., Re(N-1),Im(N-1),Re(N-1),Im(N-1)]
@param twiddle Twiddle factors
@param rev bit-reversed permutation
@param log2size Base-2 logarithm of FFT size
@param scale Total number of shifts (should be log2size/2 for normalized FFT)
@param input_fmt (0 - input is in complex Q1.15 format, 1 - input is in complex redundant Q1.15 format)
*/
/*void fft(int16_t *x,
         int16_t *y,
         int16_t *twiddle,
         uint16_t *rev,
         uint8_t log2size,
         uint8_t scale,
         uint8_t input_fmt
        );
*/

void idft1536(int16_t *sigF,int16_t *sig,int scale);

void idft6144(int16_t *sigF,int16_t *sig,int scale);

void idft12288(int16_t *sigF,int16_t *sig,int scale);

void idft18432(int16_t *sigF,int16_t *sig,int scale);

void idft3072(int16_t *sigF,int16_t *sig,int scale);

void idft24576(int16_t *sigF,int16_t *sig,int scale);

void dft1536(int16_t *sigF,int16_t *sig,int scale);

void dft6144(int16_t *sigF,int16_t *sig,int scale);

void dft12288(int16_t *sigF,int16_t *sig,int scale);

void dft18432(int16_t *sigF,int16_t *sig,int scale);


void dft3072(int16_t *sigF,int16_t *sig,int scale);

void dft24576(int16_t *sigF,int16_t *sig,int scale);


/*!\fn int32_t rotate_cpx_vector(int16_t *x,int16_t *alpha,int16_t *y,uint32_t N,uint16_t output_shift)
This function performs componentwise multiplication of a vector with a complex scalar.
@param x Vector input (Q1.15)  in the format  |Re0  Im0|,......,|Re(N-1) Im(N-1)|
@param alpha Scalar input (Q1.15) in the format  |Re0 Im0|
@param y Output (Q1.15) in the format  |Re0  Im0|,......,|Re(N-1) Im(N-1)|
@param N Length of x WARNING: N>=4
@param output_shift Number of bits to shift output down to Q1.15 (should be 15 for Q1.15 inputs) WARNING: log2_amp>0 can cause overflow!!

The function implemented is : \f$\mathbf{y} = \alpha\mathbf{x}\f$
*/
int32_t rotate_cpx_vector(int16_t *x,
                          int16_t *alpha,
                          int16_t *y,
                          uint32_t N,
                          uint16_t output_shift);


//cadd_sv.c

/*!\fn int32_t add_cpx_vector(int16_t *x,int16_t *alpha,int16_t *y,uint32_t N)
This function performs componentwise addition of a vector with a complex scalar.
@param x Vector input (Q1.15)  in the format  |Re0  Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
@param alpha Scalar input (Q1.15) in the format  |Re0 Im0|
@param y Output (Q1.15) in the format  |Re0  Im0 Re1 Im1|,......,|Re(N-2)  Im(N-2) Re(N-1) Im(N-1)|
@param N Length of x WARNING: N>=4

The function implemented is : \f$\mathbf{y} = \alpha + \mathbf{x}\f$
*/
int32_t add_cpx_vector(int16_t *x,
                       int16_t *alpha,
                       int16_t *y,
                       uint32_t N);

int32_t sub_cpx_vector16(int16_t *x,
			  int16_t *y,
			  int16_t *z,
			  uint32_t N);

int32_t add_cpx_vector32(int16_t *x,
                         int16_t *y,
                         int16_t *z,
                         uint32_t N);

int32_t add_real_vector64(int16_t *x,
                          int16_t *y,
                          int16_t *z,
                          uint32_t N);

int32_t sub_real_vector64(int16_t *x,
                          int16_t* y,
                          int16_t *z,
                          uint32_t N);

int32_t add_real_vector64_scalar(int16_t *x,
                                 long long int a,
                                 int16_t *y,
                                 uint32_t N);

/*!\fn int32_t add_vector16(int16_t *x,int16_t *y,int16_t *z,uint32_t N)
This function performs componentwise addition of two vectors with Q1.15 components.
@param x Vector input (Q1.15)
@param y Scalar input (Q1.15)
@param z Scalar output (Q1.15)
@param N Length of x WARNING: N must be a multiple of 32

The function implemented is : \f$\mathbf{z} = \mathbf{x} + \mathbf{y}\f$
*/
int32_t add_vector16(int16_t *x,
                     int16_t *y,
                     int16_t *z,
                     uint32_t N);

int32_t add_vector16_64(int16_t *x,
                        int16_t *y,
                        int16_t *z,
                        uint32_t N);

int32_t complex_conjugate(int16_t *x1,
                          int16_t *y,
                          uint32_t N);

void bit8_txmux(int32_t length,int32_t offset);

void bit8_rxdemux(int32_t length,int32_t offset);

void Zero_Buffer(void *,uint32_t);
void Zero_Buffer_nommx(void *buf,uint32_t length);

void mmxcopy(void *dest,void *src,int size);

/*!\fn int32_t signal_energy(int *,uint32_t);
\brief Computes the signal energy per subcarrier
*/
int32_t signal_energy(int32_t *,uint32_t);

#ifdef LOCALIZATION
/*!\fn int32_t signal_energy(int *,uint32_t);
\brief Computes the signal energy per subcarrier
*/
int32_t subcarrier_energy(int32_t *,uint32_t, int32_t* subcarrier_energy, uint16_t rx_power_correction);
#endif

/*!\fn int32_t signal_energy_nodc(int32_t *,uint32_t);
\brief Computes the signal energy per subcarrier, without DC removal
*/
int32_t signal_energy_nodc(int32_t *,uint32_t);

/*!\fn double signal_energy_fp(double *s_re[2], double *s_im[2],uint32_t, uint32_t,uint32_t);
\brief Computes the signal energy per subcarrier
*/
double signal_energy_fp(double *s_re[2], double *s_im[2], uint32_t nb_antennas, uint32_t length,uint32_t offset);

/*!\fn double signal_energy_fp2(struct complex *, uint32_t);
\brief Computes the signal energy per subcarrier
*/
double signal_energy_fp2(struct complex *s, uint32_t length);


int32_t iSqrt(int32_t value);
uint8_t log2_approx(uint32_t);
uint8_t log2_approx64(unsigned long long int x);
int16_t invSqrt(int16_t x);
uint32_t angle(struct complex16 perrror);

/// computes the number of factors 2 in x
unsigned char factor2(unsigned int x);

/*!\fn int32_t phy_phase_compensation_top (uint32_t pilot_type, uint32_t initial_pilot,
        uint32_t last_pilot, int32_t ignore_prefix);
Compensate the phase rotation of the RF. WARNING: This function is currently unused. It has not been tested!
@param pilot_type indicates whether it is a CHBCH (=0) or a SCH (=1) pilot
@param initial_pilot index of the first pilot (which serves as reference)
@param last_pilot index of the last pilot in the range of pilots to correct the phase
@param ignore_prefix set to 1 if cyclic prefix has not been removed (by the hardware)

*/


int8_t dB_fixed(uint32_t x);

int8_t dB_fixed2(uint32_t x,uint32_t y);

int16_t dB_fixed_times10(uint32_t x);

uint8_t dB_fixed64(uint64_t x);

int32_t phy_phase_compensation_top (uint32_t pilot_type, uint32_t initial_pilot,
                                    uint32_t last_pilot, int32_t ignore_prefix);

int32_t dot_product(int16_t *x,
                    int16_t *y,
                    uint32_t N, //must be a multiple of 8
                    uint8_t output_shift);

static inline int64_t dot_product64(int16_t *x,
		      int16_t *y,
		      uint32_t N, //must be a multiple of 8
		      uint8_t output_shift)
{

#if defined(__x86_64__) || defined(__i386__)
  __m128i *x128,*y128,mmtmp1,mmtmp2,mmtmp3,mmcumul,mmcumul_re,mmcumul_im;
  __m128i minus_i = _mm_set_epi16(-1,1,-1,1,-1,1,-1,1);
  int64_t result;

  x128 = (__m128i*) x;
  y128 = (__m128i*) y;

  mmcumul_re = _mm_setzero_si128();
  mmcumul_im = _mm_setzero_si128();

  __m128i*end=x128+(N>>2);
  for (__m128i* inPtr=x128; inPtr < end ; inPtr++) {

//    printf("n=%d, x128=%p, y128=%p\n",n,x128,y128);
       // print_shorts("x",&x128[0]);
       // print_shorts("y",&y128[0]);

    // this computes Re(z) = Re(x)*Re(y) + Im(x)*Im(y)
    mmtmp1 = _mm_madd_epi16(*inPtr,*y128);
      //  print_ints("retmp",&mmtmp1);
    // mmtmp1 contains real part of 4 consecutive outputs (32-bit)
    // shift and accumulate results
    mmtmp1 = _mm_srai_epi32(mmtmp1,output_shift);
    mmcumul_re = _mm_add_epi32(mmcumul_re,mmtmp1);
        //print_ints("re",&mmcumul_re);


    // this computes Im(z) = Re(x)*Im(y) - Re(y)*Im(x)
    mmtmp2 = _mm_shufflelo_epi16(*y128,_MM_SHUFFLE(2,3,0,1));
        //print_shorts("y",&mmtmp2);
    mmtmp2 = _mm_shufflehi_epi16(mmtmp2,_MM_SHUFFLE(2,3,0,1));
        //print_shorts("y",&mmtmp2);
    mmtmp2 = _mm_sign_epi16(mmtmp2,minus_i);
          //  print_shorts("y",&mmtmp2);

    mmtmp3 = _mm_madd_epi16(*inPtr,mmtmp2);
            //print_ints("imtmp",&mmtmp3);
    // mmtmp3 contains imag part of 4 consecutive outputs (32-bit)
    // shift and accumulate results
    mmtmp3 = _mm_srai_epi32(mmtmp3,output_shift);
    mmcumul_im = _mm_add_epi32(mmcumul_im,mmtmp3);
        //print_ints("im",&mmcumul_im);
    y128++;
  }

  // this gives Re Re Im Im
  mmcumul = _mm_hadd_epi32(mmcumul_re,mmcumul_im);
    //print_ints("cumul1",&mmcumul);
  // this gives Re Im Re Im
  mmcumul = _mm_hadd_epi32(mmcumul,mmcumul);
    //print_ints("cumul2",&mmcumul);

  //mmcumul = _mm_srai_epi32(mmcumul,output_shift);
  // extract the lower half
  result = _mm_extract_epi64(mmcumul,0);
  //printf("result: (%d,%d)\n",((int32_t*)&result)[0],((int32_t*)&result)[1]); 
 
  return(result);

#elif defined(__arm__)
  int16x4_t *x_128=(int16x4_t*)x;
  int16x4_t *y_128=(int16x4_t*)y;
  int32x4_t tmp_re,tmp_im;
  int32x4_t tmp_re1,tmp_im1;
  int32x4_t re_cumul,im_cumul;
  int32x2_t re_cumul2,im_cumul2;
  int32x4_t shift = vdupq_n_s32(-output_shift); 
  int32x2x2_t result2;
  int16_t conjug[4]__attribute__((aligned(16))) = {-1,1,-1,1} ;

  re_cumul = vdupq_n_s32(0);
  im_cumul = vdupq_n_s32(0); 

  for (n=0; n<(N>>2); n++) {

    tmp_re  = vmull_s16(*x_128++, *y_128++);
    //tmp_re = [Re(x[0])Re(y[0]) Im(x[0])Im(y[0]) Re(x[1])Re(y[1]) Im(x[1])Im(y[1])] 
    tmp_re1 = vmull_s16(*x_128++, *y_128++);
    //tmp_re1 = [Re(x1[1])Re(x2[1]) Im(x1[1])Im(x2[1]) Re(x1[1])Re(x2[2]) Im(x1[1])Im(x2[2])] 
    tmp_re  = vcombine_s32(vpadd_s32(vget_low_s32(tmp_re),vget_high_s32(tmp_re)),
                           vpadd_s32(vget_low_s32(tmp_re1),vget_high_s32(tmp_re1)));
    //tmp_re = [Re(ch[0])Re(rx[0])+Im(ch[0])Im(ch[0]) Re(ch[1])Re(rx[1])+Im(ch[1])Im(ch[1]) Re(ch[2])Re(rx[2])+Im(ch[2]) Im(ch[2]) Re(ch[3])Re(rx[3])+Im(ch[3])Im(ch[3])] 

    tmp_im  = vmull_s16(vrev32_s16(vmul_s16(*x_128++,*(int16x4_t*)conjug)),*y_128++);
    //tmp_im = [-Im(ch[0])Re(rx[0]) Re(ch[0])Im(rx[0]) -Im(ch[1])Re(rx[1]) Re(ch[1])Im(rx[1])]
    tmp_im1 = vmull_s16(vrev32_s16(vmul_s16(*x_128++,*(int16x4_t*)conjug)),*y_128++);
    //tmp_im1 = [-Im(ch[2])Re(rx[2]) Re(ch[2])Im(rx[2]) -Im(ch[3])Re(rx[3]) Re(ch[3])Im(rx[3])]
    tmp_im  = vcombine_s32(vpadd_s32(vget_low_s32(tmp_im),vget_high_s32(tmp_im)),
                           vpadd_s32(vget_low_s32(tmp_im1),vget_high_s32(tmp_im1)));
    //tmp_im = [-Im(ch[0])Re(rx[0])+Re(ch[0])Im(rx[0]) -Im(ch[1])Re(rx[1])+Re(ch[1])Im(rx[1]) -Im(ch[2])Re(rx[2])+Re(ch[2])Im(rx[2]) -Im(ch[3])Re(rx[3])+Re(ch[3])Im(rx[3])]

    re_cumul = vqaddq_s32(re_cumul,vqshlq_s32(tmp_re,shift));
    im_cumul = vqaddq_s32(im_cumul,vqshlq_s32(tmp_im,shift));
  }
  
  re_cumul2 = vpadd_s32(vget_low_s32(re_cumul),vget_high_s32(re_cumul));
  im_cumul2 = vpadd_s32(vget_low_s32(im_cumul),vget_high_s32(im_cumul));
  re_cumul2 = vpadd_s32(re_cumul2,re_cumul2);
  im_cumul2 = vpadd_s32(im_cumul2,im_cumul2);
  result2   = vzip_s32(re_cumul2,im_cumul2);
  return(vget_lane_s32(result2.val[0],0));
#endif
}

void dft12(int16_t *x,int16_t *y);
void dft24(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft36(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft48(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft60(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft72(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft96(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft108(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft120(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft144(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft180(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft192(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft216(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft240(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft288(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft300(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft324(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft360(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft384(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft432(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft480(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft540(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft576(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft600(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft648(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft720(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft768(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft864(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft900(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft960(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft972(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft1080(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft1152(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft1200(int16_t *x,int16_t *y,uint8_t scale_flag);

void dft64(int16_t *x,int16_t *y,int scale);
void dft128(int16_t *x,int16_t *y,int scale);
void dft256(int16_t *x,int16_t *y,int scale);
void dft512(int16_t *x,int16_t *y,int scale);
void dft1024(int16_t *x,int16_t *y,int scale);
void dft2048(int16_t *x,int16_t *y,int scale);
void dft4096(int16_t *x,int16_t *y,int scale);
void dft8192(int16_t *x,int16_t *y,int scale);
void idft64(int16_t *x,int16_t *y,int scale);
void idft128(int16_t *x,int16_t *y,int scale);
void idft256(int16_t *x,int16_t *y,int scale);
void idft512(int16_t *x,int16_t *y,int scale);
void idft1024(int16_t *x,int16_t *y,int scale);
void idft2048(int16_t *x,int16_t *y,int scale);
void idft4096(int16_t *x,int16_t *y,int scale);
void idft8192(int16_t *x,int16_t *y,int scale);
/** @} */


double interp(double x, double *xs, double *ys, int count);

int write_output(const char *fname,const char *vname,void *data,int length,int dec,char format);

#endif //__PHY_TOOLS_DEFS__H__
