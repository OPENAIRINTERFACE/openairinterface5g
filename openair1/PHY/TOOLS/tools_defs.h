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
uint8_t dB_fixed64(uint64_t x);

int8_t dB_fixed2(uint32_t x,uint32_t y);

int16_t dB_fixed_times10(uint32_t x);

int32_t phy_phase_compensation_top (uint32_t pilot_type, uint32_t initial_pilot,
                                    uint32_t last_pilot, int32_t ignore_prefix);

int32_t dot_product(int16_t *x,
                    int16_t *y,
                    uint32_t N, //must be a multiple of 8
                    uint8_t output_shift);

int64_t dot_product64(int16_t *x,
                      int16_t *y,
                      uint32_t N, //must be a multiple of 8
                      uint8_t output_shift);

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

#endif //__PHY_TOOLS_DEFS__H__
