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

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include "PHY/sse_intrin.h"

#define CEILIDIV(a,b) ((a+b-1)/b)
#define ROUNDIDIV(a,b) (((a<<1)+b)/(b<<1))

struct complexd {
  double r;
  double i;
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
                                        uint32_t N);

void multadd_real_four_symbols_vector_complex_scalar(int16_t *x,
                                                     int16_t *alpha,
                                                     int16_t *y);

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



#ifdef OAIDFTS_MAIN
typedef  void(*adftfunc_t)(int16_t *sigF,int16_t *sig,unsigned char scale_flag);  
typedef  void(*aidftfunc_t)(int16_t *sigF,int16_t *sig,unsigned char scale_flag);     

void dft12(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft24(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft36(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft48(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft60(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft64(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft72(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft96(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft108(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft120(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft128(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft144(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft180(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft192(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft216(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft240(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft256(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft288(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft300(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft324(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft360(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft384(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft432(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft480(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft512(int16_t *x,int16_t *y,uint8_t scale_flag);
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
void dft1024(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft1080(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft1152(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft1200(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft1296(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft1440(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft1500(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft1536(int16_t *sigF,int16_t *sig,uint8_t scale_flag);
void dft1620(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft1728(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft1800(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft1920(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft1944(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft2048(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft2160(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft2304(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft2400(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft2592(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft2700(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft2880(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft2916(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft3000(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft3072(int16_t *sigF,int16_t *sig,uint8_t scale_flag);
void dft3240(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft4096(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft6144(int16_t *sigF,int16_t *sig,uint8_t scale_flag);
void dft8192(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft9216(int16_t *x,int16_t *y,uint8_t scale_flag);
void dft12288(int16_t *x,int16_t *y,uint8_t scale_flag);  
void dft18432(int16_t *x,int16_t *y,uint8_t scale_flag); 
void dft24576(int16_t *x,int16_t *y,uint8_t scale_flag); 
void dft36864(int16_t *x,int16_t *y,uint8_t scale_flag); 
void dft49152(int16_t *x,int16_t *y,uint8_t scale_flag); 
void dft73728(int16_t *x,int16_t *y,uint8_t scale_flag); 
void dft98304(int16_t *x,int16_t *y,uint8_t scale_flag);


void idft64(int16_t *x,int16_t *y,uint8_t scale_flag);
void idft128(int16_t *x,int16_t *y,uint8_t scale_flag);
void idft256(int16_t *x,int16_t *y,uint8_t scale_flag);
void idft512(int16_t *x,int16_t *y,uint8_t scale_flag);
void idft1024(int16_t *x,int16_t *y,uint8_t scale_flag);
void idft1536(int16_t *sigF,int16_t *sig,uint8_t scale_flag);
void idft2048(int16_t *x,int16_t *y,uint8_t scale_flag);
void idft3072(int16_t *sigF,int16_t *sig,uint8_t scale_flag);
void idft4096(int16_t *x,int16_t *y,uint8_t scale_flag);
void idft6144(int16_t *sigF,int16_t *sig,uint8_t scale_flag);
void idft8192(int16_t *x,int16_t *y,uint8_t scale_flag);
void idft9216(int16_t *x,int16_t *y,uint8_t scale_flag);
void idft12288(int16_t *sigF,int16_t *sig,uint8_t scale_flag);
void idft18432(int16_t *sigF,int16_t *sig,uint8_t scale_flag);
void idft24576(int16_t *sigF,int16_t *sig,uint8_t scale_flag);
void idft36864(int16_t *sigF,int16_t *sig,uint8_t scale_flag);
void idft49152(int16_t *sigF,int16_t *sig,uint8_t scale_flag); 
void idft73728(int16_t *sigF,int16_t *sig,uint8_t scale_flag);
void idft98304(int16_t *sigF,int16_t *sig,uint8_t scale_flag);




#else
  typedef  void(*dftfunc_t)(uint8_t sizeidx,int16_t *sigF,int16_t *sig,unsigned char scale_flag);  
  typedef  void(*idftfunc_t)(uint8_t sizeidx,int16_t *sigF,int16_t *sig,unsigned char scale_flag);  
#  ifdef OAIDFTS_LOADER
  dftfunc_t dft;
  idftfunc_t idft;
#  else
  extern dftfunc_t dft;
  extern idftfunc_t idft;
  extern int load_dftslib(void);
#  endif
#endif

typedef enum DFT_size_idx {
	DFT_12,    DFT_24,    DFT_36,   DFT_48,     DFT_60,   DFT_72,   DFT_96,
	DFT_108,   DFT_120,   DFT_128,  DFT_144,    DFT_180,  DFT_192,  DFT_216,   DFT_240,
	DFT_256,   DFT_288,   DFT_300,  DFT_324,    DFT_360,  DFT_384,  DFT_432,   DFT_480,
	DFT_512,   DFT_540,   DFT_576,  DFT_600,    DFT_648,  DFT_720,  DFT_768,   DFT_864,
	DFT_900,   DFT_960,   DFT_972,  DFT_1024,   DFT_1080, DFT_1152, DFT_1200,  DFT_1296,
	DFT_1440,  DFT_1500,  DFT_1536, DFT_1620,   DFT_1728, DFT_1800, DFT_1920,  DFT_1944,
	DFT_2048,  DFT_2160,  DFT_2304, DFT_2400,   DFT_2592, DFT_2700, DFT_2880,  DFT_2916,
	DFT_3000,  DFT_3072,  DFT_3240, DFT_4096,   DFT_6144, DFT_8192, DFT_9216,  DFT_12288,
	DFT_18432, DFT_24576, DFT_36864, DFT_49152, DFT_73728, DFT_98304,
	DFT_SIZE_IDXTABLESIZE
} dft_size_idx_t;

#ifdef OAIDFTS_MAIN
adftfunc_t dft_ftab[]={
	dft12,    dft24,    dft36,    dft48,    dft60,   dft72,   dft96,
	dft108,   dft120,   dft128,   dft144,   dft180,  dft192,  dft216,   dft240,
	dft256,   dft288,   dft300,   dft324,   dft360,  dft384,  dft432,   dft480,
	dft512,   dft540,   dft576,   dft600,   dft648,  dft720,  dft768,   dft864,
	dft900,   dft960,   dft972,   dft1024,  dft1080, dft1152, dft1200,  dft1296,
	dft1440,  dft1500,  dft1536,  dft1620,  dft1728, dft1800, dft1920,  dft1944,
	dft2048,  dft2160,  dft2304,  dft2400,  dft2592, dft2700, dft2880,  dft2916,
	dft3000,  dft3072,  dft3240,  dft4096,  dft6144, dft8192, dft9216,  dft12288,
	dft18432, dft24576, dft36864, dft49152, dft73728, dft98304
};
#endif

typedef enum idft_size_idx {
	IDFT_128,   IDFT_256,  IDFT_512,   IDFT_1024,  IDFT_1536,  IDFT_2048,  IDFT_3072,  IDFT_4096,
	IDFT_6144,  IDFT_8192, IDFT_9216,  IDFT_12288, IDFT_18432, IDFT_24576, IDFT_36864, IDFT_49152, 
	IDFT_73728, IDFT_98304, 
	IDFT_SIZE_IDXTABLESIZE
} idft_size_idx_t;
#ifdef OAIDFTS_MAIN
aidftfunc_t idft_ftab[]={
	idft128,   idft256,  idft512,   idft1024,  idft1536,  idft2048,  idft3072,  idft4096,
	idft6144,  idft8192, idft9216,  idft12288, idft18432, idft24576, idft36864, idft49152,
	idft73728, idft98304
};
#endif



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

/*!\fn int32_t signal_energy_fixed_p9(int *input, uint32_t length);
\brief Computes the signal energy per subcarrier
\ the input signal has a fixed point representation of AMP_SHIFT bits
\ the ouput energy has a fixed point representation of AMP_SHIFT bits
*/
int32_t signal_energy_amp_shift(int32_t *input, uint32_t length);

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

int32_t signal_power(int32_t *,uint32_t);
int32_t interference_power(int32_t *,uint32_t);

/*!\fn double signal_energy_fp(double *s_re[2], double *s_im[2],uint32_t, uint32_t,uint32_t);
\brief Computes the signal energy per subcarrier
*/
double signal_energy_fp(double *s_re[2], double *s_im[2], uint32_t nb_antennas, uint32_t length,uint32_t offset);

/*!\fn double signal_energy_fp2(struct complex *, uint32_t);
\brief Computes the signal energy per subcarrier
*/
double signal_energy_fp2(struct complexd *s, uint32_t length);


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
int16_t dB_fixed_x10(uint32_t x);

int32_t phy_phase_compensation_top(uint32_t pilot_type,
                                   uint32_t initial_pilot,
                                   uint32_t last_pilot,
                                   int32_t ignore_prefix);

int32_t dot_product(int16_t *x,
                    int16_t *y,
                    uint32_t N, //must be a multiple of 8
                    uint8_t output_shift);

int64_t dot_product64(int16_t *x,
                      int16_t *y,
                      uint32_t N, //must be a multiple of 8
                      uint8_t output_shift);


/** @} */


double interp(double x, double *xs, double *ys, int count);

int write_output(const char *fname,const char *vname,void *data,int length,int dec,char format);

#ifdef __cplusplus
}
#endif

#endif //__PHY_TOOLS_DEFS__H__
