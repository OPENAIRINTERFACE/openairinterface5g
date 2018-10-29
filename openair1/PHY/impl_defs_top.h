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

/*! \file PHY/impl_defs_top.h
* \brief More defines and structure definitions
* \author R. Knopp, F. Kaltenberger
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr
* \note
* \warning
*/

#ifndef __PHY_IMPLEMENTATION_DEFS_H__
#define __PHY_IMPLEMENTATION_DEFS_H__

/** @defgroup _ref_implementation_ OpenAirInterface LTE Implementation
 * @{

 * @defgroup _PHY_RF_INTERFACE_ PHY - RF Interface
 * @ingroup _PHY_RF_INTERFACE_
 * @{
 * @defgroup _GENERIC_PHY_RF_INTERFACE_ Generic PHY - RF Interface
 * @defgroup _USRP_PHY_RF_INTERFACE_    PHY - USRP RF Interface
 * @defgroup _BLADERF_PHY_RF_INTERFACE_    PHY - BLADERF RF Interface
 * @defgroup _LMSSDR_PHY_RF_INTERFACE_    PHY - LMSSDR RF Interface
 * @}
 *
 * @ingroup _ref_implementation_
 * @{
 * This module is responsible for defining the generic interface between PHY and RF Target
 * @}
 
 * @defgroup _openair1_ openair1 Reference Implementation 
 * @ingroup _ref_implementation_
 * @{


 * @defgroup _physical_layer_ref_implementation_ Physical Layer Reference Implementation
 * @ingroup _openair1_
 * @{


 * @defgroup _PHY_STRUCTURES_ Basic Structures and Memory Initialization
 * @ingroup _physical_layer_ref_implementation_
 * @{
 * This module is responsible for defining and initializing the PHY variables during static configuration of OpenAirInterface.
 * @}

 * @defgroup _PHY_DSP_TOOLS_ DSP Tools
 * @ingroup _physical_layer_ref_implementation_
 * @{
 * This module is responsible for basic signal processing related to inner-MODEM processing.
 * @}

 * @defgroup _PHY_MODULATION_ Modulation and Demodulation
 * @ingroup _physical_layer_ref_implementation_
 * @{
 * This module is responsible for procedures related to OFDMA modulation and demodulation.
 * @}

 * @defgroup _PHY_PARAMETER_ESTIMATION_BLOCKS_ Parameter Estimation
 * @ingroup _physical_layer_ref_implementation_
 * @{
 * This module is responsible for procedures related to OFDMA frequency-domain channel estimation for LTE Downlink Channels.
 * @}

 * @defgroup _PHY_CODING_BLOCKS_ Channel Coding/Decoding Functions
 * @ingroup _physical_layer_ref_implementation_
 * @{
 * This module is responsible for procedures related to channel coding/decoding, rate-matching, segementation and interleaving.
 * @}

 * @defgroup _PHY_TRANSPORT_ Transport/Physical Channel Processing
 * @ingroup _physical_layer_ref_implementation_
 * @{
 * This module is responsible for defining and processing the PHY procedures (TX/RX) related to transport and physical channels.
 * @}

 * @defgroup _PHY_PROCEDURES_ Physical Layer Procedures
 * @ingroup _physical_layer_ref_implementation_
 * @{
 * This module is responsible for defining and processing the PHY procedures (TX/RX) related to transport and physical channels.
 * @}

 * @}
 * @}
 * @}
 */

#include "defs_eNB.h"
#include "types.h"



/** @addtogroup _PHY_STRUCTURES_
 * @{
*/
#define NUMBER_OF_OFDM_CARRIERS (frame_parms->ofdm_symbol_size)
#define NUMBER_OF_SYMBOLS_PER_FRAME (frame_parms->symbols_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME)
#define NUMBER_OF_USEFUL_CARRIERS (12*frame_parms->N_RB_DL)
#define NUMBER_OF_ZERO_CARRIERS (NUMBER_OF_OFDM_CARRIERS-NUMBER_OF_USEFUL_CARRIERS)
#define NUMBER_OF_USEFUL_CARRIERS_BYTES (NUMBER_OF_USEFUL_CARRIERS>>2)
#define HALF_NUMBER_OF_USEFUL_CARRIERS (NUMBER_OF_USEFUL_CARRIERS>>1)
#define HALF_NUMBER_OF_USEFUL_CARRIERS_BYTES (HALF_NUMBER_OF_USEFUL_CARRIERS>>2)
#define FIRST_CARRIER_OFFSET (HALF_NUMBER_OF_USEFUL_CARRIERS+NUMBER_OF_ZERO_CARRIERS)
#define NUMBER_OF_OFDM_SYMBOLS_PER_SLOT (NUMBER_OF_SYMBOLS_PER_FRAME/LTE_SLOTS_PER_FRAME)

#define EXTENSION_TYPE (PHY_config->PHY_framing.Extension_type)

#define NUMBER_OF_OFDM_CARRIERS_BYTES   NUMBER_OF_OFDM_CARRIERS*4
//#define NUMBER_OF_USEFUL_CARRIERS_BYTES NUMBER_OF_USEFUL_CARRIERS*4
#define HALF_NUMBER_OF_USER_CARRIERS_BYTES NUMBER_OF_USEFUL_CARRIERS/2

#define CYCLIC_PREFIX_LENGTH (frame_parms->nb_prefix_samples)
#define CYCLIC_PREFIX_LENGTH_SAMPLES (CYCLIC_PREFIX_LENGTH*2)
#define CYCLIC_PREFIX_LENGTH_BYTES (CYCLIC_PREFIX_LENGTH*4)
#define CYCLIC_PREFIX_LENGTH0 (frame_parms->nb_prefix_samples0)
#define CYCLIC_PREFIX_LENGTH_SAMPLES0 (CYCLIC_PREFIX_LENGTH0*2)
#define CYCLIC_PREFIX_LENGTH_BYTES0 (CYCLIC_PREFIX_LENGTH0*4)

#define OFDM_SYMBOL_SIZE_SAMPLES ((NUMBER_OF_OFDM_CARRIERS + CYCLIC_PREFIX_LENGTH)*2)   // 16-bit units (i.e. real samples)
#define OFDM_SYMBOL_SIZE_SAMPLES0 ((NUMBER_OF_OFDM_CARRIERS + CYCLIC_PREFIX_LENGTH0)*2)   // 16-bit units (i.e. real samples)
#define OFDM_SYMBOL_SIZE_SAMPLES_MAX 4096   // 16-bit units (i.e. real samples)
#define OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES (OFDM_SYMBOL_SIZE_SAMPLES/2)                   // 32-bit units (i.e. complex samples)
#define OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES0 (OFDM_SYMBOL_SIZE_SAMPLES0/2)                   // 32-bit units (i.e. complex samples)
#define OFDM_SYMBOL_SIZE_SAMPLES_NO_PREFIX ((NUMBER_OF_OFDM_CARRIERS)*2)
#define OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES_NO_PREFIX (OFDM_SYMBOL_SIZE_SAMPLES_NO_PREFIX/2)
#define OFDM_SYMBOL_SIZE_BYTES (OFDM_SYMBOL_SIZE_SAMPLES*2)
#define OFDM_SYMBOL_SIZE_BYTES0 (OFDM_SYMBOL_SIZE_SAMPLES0*2)
#define OFDM_SYMBOL_SIZE_BYTES_NO_PREFIX (OFDM_SYMBOL_SIZE_SAMPLES_NO_PREFIX*2)

#define SLOT_LENGTH_BYTES (frame_parms->samples_per_tti<<1) // 4 bytes * samples_per_tti/2
#define SLOT_LENGTH_BYTES_NO_PREFIX (OFDM_SYMBOL_SIZE_BYTES_NO_PREFIX * NUMBER_OF_OFDM_SYMBOLS_PER_SLOT)

#define FRAME_LENGTH_COMPLEX_SAMPLES (frame_parms->samples_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME)
#define FRAME_LENGTH_SAMPLES (FRAME_LENGTH_COMPLEX_SAMPLES*2)
#define FRAME_LENGTH_SAMPLES_NO_PREFIX (NUMBER_OF_SYMBOLS_PER_FRAME*OFDM_SYMBOL_SIZE_SAMPLES_NO_PREFIX)
#define FRAME_LENGTH_COMPLEX_SAMPLES_NO_PREFIX (FRAME_LENGTH_SAMPLES_NO_PREFIX/2)

#define NUMBER_OF_CARRIERS_PER_GROUP (NUMBER_OF_USEFUL_CARRIERS/NUMBER_OF_FREQUENCY_GROUPS)

#define RX_PRECISION (16)
#define LOG2_RX_PRECISION (4)
#define RX_OUTPUT_SHIFT (4)


#define SAMPLE_SIZE_BYTES    2                                           // 2 bytes/real sample

#define FRAME_LENGTH_BYTES   (FRAME_LENGTH_SAMPLES * SAMPLE_SIZE_BYTES)  // frame size in bytes
#define FRAME_LENGTH_BYTES_NO_PREFIX   (FRAME_LENGTH_SAMPLES_NO_PREFIX * SAMPLE_SIZE_BYTES)  // frame size in bytes


#define FFT_SCALE_FACTOR     8                                           // Internal Scaling for FFT
#define DMA_BLKS_PER_SLOT    (SLOT_LENGTH_BYTES/2048)                    // Number of DMA blocks per slot
#define SLOT_TIME_NS         (SLOT_LENGTH_SAMPLES*(1e3)/7.68)            // slot time in ns

#define NB_ANTENNA_PORTS_ENB  6                                         // total number of eNB antenna ports

#ifdef EXMIMO
#define TARGET_RX_POWER 55    // Target digital power for the AGC
#define TARGET_RX_POWER_MAX 55    // Maximum digital power, such that signal does not saturate (value found by simulation)
#define TARGET_RX_POWER_MIN 50    // Minimum digital power, anything below will be discarded (value found by simulation)
#else
#define TARGET_RX_POWER 50    // Target digital power for the AGC
#define TARGET_RX_POWER_MAX 65    // Maximum digital power, such that signal does not saturate (value found by simulation)
#define TARGET_RX_POWER_MIN 35    // Minimum digital power, anything below will be discarded (value found by simulation)
#endif

//the min and max gains have to match the calibrated gain table
//#define MAX_RF_GAIN 160
//#define MIN_RF_GAIN 96
#define MAX_RF_GAIN 200
#define MIN_RF_GAIN 80

#define PHY_SYNCH_OFFSET ((OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES)-1)  // OFFSET of BEACON SYNCH
#define PHY_SYNCH_MIN_POWER 1000
#define PHY_SYNCH_THRESHOLD 100



#define ONE_OVER_SQRT2_Q15 23170
#define ONE_OVER_2_Q15 16384

// QAM amplitude definitions

/// Amplitude for QPSK (\f$ 2^15 \times 1/\sqrt{2}\f$)
#define QPSK 23170

/// First Amplitude for QAM16 (\f$ 2^{15} \times 2/\sqrt{10}\f$)
#define QAM16_n1 20724
/// Second Amplitude for QAM16 (\f$ 2^{15} \times 1/\sqrt{10}\f$)
#define QAM16_n2 10362

///First Amplitude for QAM64 (\f$ 2^{15} \times 4/\sqrt{42}\f$)
#define QAM64_n1 20225
///Second Amplitude for QAM64 (\f$ 2^{15} \times 2/\sqrt{42}\f$)
#define QAM64_n2 10112
///Third Amplitude for QAM64 (\f$ 2^{15} \times 1/\sqrt{42}\f$)
#define QAM64_n3 5056

/// First Amplitude for QAM16 for TM5 (\f$ 2^{15} \times 2/sqrt(20)\f$)
#define QAM16_TM5_n1 14654
/// Second Amplitude for QAM16 for TM5 Receiver (\f$ 2^{15} \times 1/\sqrt{20}\f$)
#define QAM16_TM5_n2 7327

///First Amplitude for QAM64 (\f$ 2^{15} \times 4/\sqrt{84}\f$)
#define QAM64_TM5_n1 14301
///Second Amplitude for QAM64 (\f$ 2^{15} \times 2/\sqrt{84}\f$)
#define QAM64_TM5_n2 7150
///Third Amplitude for QAM64 for TM5 Receiver (\f$ 2^{15} \times 1/\sqrt{84}\f$)
#define QAM64_TM5_n3 3575


#ifdef BIT8_RXMUX
#define PERROR_SHIFT 0
#else
#define PERROR_SHIFT 10
#endif

#define BIT8_TX_SHIFT 2
#define BIT8_TX_SHIFT_DB 12

//#define CHBCH_RSSI_MIN -75

#ifdef BIT8_TX
#define AMP 128
#else
#define AMP 512//1024 //4096
#endif

#define AMP_OVER_SQRT2 ((AMP*ONE_OVER_SQRT2_Q15)>>15)
#define AMP_OVER_2 (AMP>>1)

/// Threshold for PUCCH Format 1 detection
#define PUCCH1_THRES 0
/// Threshold for PUCCH Format 1a/1b detection
#define PUCCH1a_THRES 4

/// Data structure for transmission.
typedef struct {
  /// RAW TX sample buffer
  char *TX_DMA_BUFFER[2];
} TX_VARS ;

/// Data structure for reception.
typedef struct {
  /// RAW TX sample buffer
  char *TX_DMA_BUFFER[2];
  /// RAW RX sample buffer
  int *RX_DMA_BUFFER[2];
} TX_RX_VARS;

/// Measurement Variables

//#define NUMBER_OF_SUBBANDS_MAX 13
#define NUMBER_OF_HARQ_PID_MAX 8

#define MAX_FRAME_NUMBER 0x400
#include "openairinterface5g_limits.h"
#include "assertions.h"

#define cmax(a,b)  ((a>b) ? (a) : (b))
#define cmax3(a,b,c) ((cmax(a,b)>c) ? (cmax(a,b)) : (c))
#define cmin(a,b)  ((a<b) ? (a) : (b))
#define max(a,b) cmax(a,b)
#define min(a,b) cmin(a,b)

#ifndef malloc16
#  ifdef __AVX2__
#    define malloc16(x) memalign(32,x+32)
#  else
#    define malloc16(x) memalign(16,x+16)
#  endif
#endif
#define free16(y,x) free(y)
#define bigmalloc malloc
#define bigmalloc16 malloc16
#define openair_free(y,x) free((y))
#define PAGE_SIZE 4096
#define free_and_zero(PtR) do { \
      if (PtR) {           \
        free(PtR);         \
        PtR = NULL;        \
      }                    \
    } while (0)
static inline void* malloc16_clear( size_t size )
{
#ifdef __AVX2__
  void* ptr = memalign(32, size+32);
#else
  void* ptr = memalign(16, size+16);
#endif
  DevAssert(ptr);
  memset( ptr, 0, size );
  return ptr;
}

#endif //__PHY_IMPLEMENTATION_DEFS_H__ 
/**@} 
*/
