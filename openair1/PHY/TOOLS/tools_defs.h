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

#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include "PHY/sse_intrin.h"
#include "common/utils/assertions.h"

#define CEILIDIV(a,b) ((a+b-1)/b)
#define ROUNDIDIV(a,b) (((a<<1)+b)/(b<<1))

typedef struct complexd {
  double r;
  double i;
} cd_t;

typedef struct complexf {
  float r;
  float i;
} cf_t;

typedef struct complex16 {
  int16_t r;
  int16_t i;
} c16_t;

typedef struct complex32 {
  int32_t r;
  int32_t i;
} c32_t;
  
typedef struct complex64 {
  int64_t r;
  int64_t i;
} c64_t;

#define squaredMod(a) ((a).r*(a).r+(a).i*(a).i)
#define csum(res, i1, i2) (res).r=(i1).r+(i2).r ; (res).i=(i1).i+(i2).i  
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

#define FOREACH_DFTSZ(SZ_DEF) \
  SZ_DEF(12) \
  SZ_DEF(24) \
  SZ_DEF(36) \
  SZ_DEF(48) \
  SZ_DEF(60) \
  SZ_DEF(64) \
  SZ_DEF(72) \
  SZ_DEF(96) \
  SZ_DEF(108) \
  SZ_DEF(120) \
  SZ_DEF(128) \
  SZ_DEF(144) \
  SZ_DEF(180) \
  SZ_DEF(192) \
  SZ_DEF(216) \
  SZ_DEF(240) \
  SZ_DEF(256) \
  SZ_DEF(288) \
  SZ_DEF(300) \
  SZ_DEF(324) \
  SZ_DEF(360) \
  SZ_DEF(384) \
  SZ_DEF(432) \
  SZ_DEF(480) \
  SZ_DEF(512) \
  SZ_DEF(540) \
  SZ_DEF(576) \
  SZ_DEF(600) \
  SZ_DEF(648) \
  SZ_DEF(720) \
  SZ_DEF(768) \
  SZ_DEF(864) \
  SZ_DEF(900) \
  SZ_DEF(960) \
  SZ_DEF(972) \
  SZ_DEF(1024) \
  SZ_DEF(1080) \
  SZ_DEF(1152) \
  SZ_DEF(1200) \
  SZ_DEF(1296) \
  SZ_DEF(1440) \
  SZ_DEF(1500) \
  SZ_DEF(1536) \
  SZ_DEF(1620) \
  SZ_DEF(1728) \
  SZ_DEF(1800) \
  SZ_DEF(1920) \
  SZ_DEF(1944) \
  SZ_DEF(2048) \
  SZ_DEF(2160) \
  SZ_DEF(2304) \
  SZ_DEF(2400) \
  SZ_DEF(2592) \
  SZ_DEF(2700) \
  SZ_DEF(2880) \
  SZ_DEF(2916) \
  SZ_DEF(3000) \
  SZ_DEF(3072) \
  SZ_DEF(3240) \
  SZ_DEF(4096) \
  SZ_DEF(6144) \
  SZ_DEF(8192) \
  SZ_DEF(9216) \
  SZ_DEF(12288) \
  SZ_DEF(18432) \
  SZ_DEF(24576) \
  SZ_DEF(36864) \
  SZ_DEF(49152) \
  SZ_DEF(73728) \
  SZ_DEF(98304)

#define FOREACH_IDFTSZ(SZ_DEF)\
  SZ_DEF(64) \
  SZ_DEF(128) \
  SZ_DEF(256) \
  SZ_DEF(512) \
  SZ_DEF(1024) \
  SZ_DEF(1536) \
  SZ_DEF(2048) \
  SZ_DEF(3072) \
  SZ_DEF(4096) \
  SZ_DEF(6144) \
  SZ_DEF(8192) \
  SZ_DEF(9216) \
  SZ_DEF(12288) \
  SZ_DEF(18432) \
  SZ_DEF(24576) \
  SZ_DEF(36864) \
  SZ_DEF(49152) \
  SZ_DEF(73728) \
  SZ_DEF(98304)

#ifdef OAIDFTS_MAIN
typedef  void(*adftfunc_t)(int16_t *sigF,int16_t *sig,unsigned char scale_flag);
typedef  void(*aidftfunc_t)(int16_t *sigF,int16_t *sig,unsigned char scale_flag);

#define SZ_FUNC(Sz) void dft ## Sz(int16_t *x,int16_t *y,uint8_t scale_flag);

FOREACH_DFTSZ(SZ_FUNC)

#define SZ_iFUNC(Sz) void idft ## Sz(int16_t *x,int16_t *y,uint8_t scale_flag);

FOREACH_IDFTSZ(SZ_iFUNC)

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

#define SZ_ENUM(Sz) DFT_ ## Sz,

typedef enum dft_size_idx {
  FOREACH_DFTSZ(SZ_ENUM)
  DFT_SIZE_IDXTABLESIZE
}  dft_size_idx_t;

#define SZ_iENUM(Sz) IDFT_ ## Sz,

/*******************************************************************
*
* NAME :         get_dft
*
* PARAMETERS :   size of ofdm symbol
*
* RETURN :       function for discrete fourier transform
*
* DESCRIPTION :  get dft function depending of ofdm size
*
*********************************************************************/
static inline
dft_size_idx_t get_dft(int ofdm_symbol_size)
{
  switch (ofdm_symbol_size) {
    case 128:
      return DFT_128;
    case 256:
      return DFT_256;
    case 512:
      return DFT_512;
    case 1024:
      return DFT_1024;
    case 1536:
      return DFT_1536;
    case 2048:
      return DFT_2048;
    case 3072:
      return DFT_3072;
    case 4096:
      return DFT_4096;
    case 6144:
      return DFT_6144;
    case 8192:
      return DFT_8192;
    case 9216:
      return DFT_9216;
    case 12288:
      return DFT_12288;
    case 18432:
      return DFT_18432;
    case 24576:
      return DFT_24576;
    case 36864:
      return DFT_36864;
    case 49152:
      return DFT_49152;
    case 73728:
      return DFT_73728;
    case 98304:
      return DFT_98304;
    default:
      printf("function get_dft : unsupported ofdm symbol size \n");
      assert(0);
      break;
 }
 return DFT_SIZE_IDXTABLESIZE; // never reached and will trigger assertion in idft function;
}

typedef enum idft_size_idx {
  FOREACH_IDFTSZ(SZ_iENUM)
  IDFT_SIZE_IDXTABLESIZE
}  idft_size_idx_t;

#ifdef OAIDFTS_MAIN

#define SZ_PTR(Sz) {dft ## Sz,Sz},
struct {
  adftfunc_t func;
  int size;
} dft_ftab[]= {
  FOREACH_DFTSZ(SZ_PTR)
};

#define SZ_iPTR(Sz)  {idft ## Sz,Sz},
struct {
  adftfunc_t func;
  int size;
} idft_ftab[]= {
  FOREACH_IDFTSZ(SZ_iPTR)
};

#endif

/*******************************************************************
*
* NAME :         get_idft
*
* PARAMETERS :   size of ofdm symbol
*
* RETURN :       index pointing to the dft func in the dft library
*
* DESCRIPTION :  get idft function depending of ofdm size
*
*********************************************************************/
static inline
idft_size_idx_t get_idft(int ofdm_symbol_size)
{
  switch (ofdm_symbol_size) {
    case 128:
      return IDFT_128;
    case 256:
      return IDFT_256;
    case 512:
      return IDFT_512;
    case 1024:
      return IDFT_1024;
    case 1536:
      return IDFT_1536;
    case 2048:
      return IDFT_2048;
    case 3072:
      return IDFT_3072;
    case 4096:
      return IDFT_4096;
    case 6144:
      return IDFT_6144;
    case 8192:
      return IDFT_8192;
    case 9216:
      return IDFT_9216;
    case 12288:
      return IDFT_12288;
    case 18432:
      return IDFT_18432;
    case 24576:
      return IDFT_24576;
    case 36864:
      return IDFT_36864;
    case 49152:
      return IDFT_49152;
    case 73728:
      return IDFT_73728;
    case 98304:
      return IDFT_98304;
    default:
      printf("function get_idft : unsupported ofdm symbol size \n");
      assert(0);
      break;
 }
 return IDFT_SIZE_IDXTABLESIZE; // never reached and will trigger assertion in idft function
}


/*!\fn int32_t rotate_cpx_vector(c16_t *x,c16_t *alpha,c16_t *y,uint32_t N,uint16_t output_shift)
This function performs componentwise multiplication of a vector with a complex scalar.
@param x Vector input (Q1.15)  in the format  |Re0  Im0|,......,|Re(N-1) Im(N-1)|
@param alpha Scalar input (Q1.15) in the format  |Re0 Im0|
@param y Output (Q1.15) in the format  |Re0  Im0|,......,|Re(N-1) Im(N-1)|
@param N Length of x WARNING: N>=4
@param output_shift Number of bits to shift output down to Q1.15 (should be 15 for Q1.15 inputs) WARNING: log2_amp>0 can cause overflow!!

The function implemented is : \f$\mathbf{y} = \alpha\mathbf{x}\f$
*/
void rotate_cpx_vector(c16_t *x,
                       c16_t *alpha,
                       c16_t *y,
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
                          int16_t *y,
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
int32_t subcarrier_energy(int32_t *,uint32_t, int32_t *subcarrier_energy, uint16_t rx_power_correction);
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
