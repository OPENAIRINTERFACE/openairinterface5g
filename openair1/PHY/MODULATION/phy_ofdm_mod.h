/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef __PHY_OFDM_MOD__H__
#define __PHY_OFDM_MOD__H__

/*! \brief Extension Type */
typedef enum {
  CYCLIC_PREFIX,
  CYCLIC_SUFFIX,
  ZEROS,
  NONE
} Extension_t;

/**
   \fn void PHY_ofdm_mod(int *input,int *output,int fftsize,unsigned char nb_symbols,unsigned short nb_prefix_samples,Extension_t
   etype) This function performs OFDM modulation with cyclic extension or zero-padding
   @param input The sequence input samples in the frequency-domain  This is a concatenation of the input symbols in SIMD redundant
   format
   @param output The time-domain output signal
   @param fftsize size of OFDM symbol size (\f$N_d\f$)
   @param nb_symbols The number of OFDM symbols in the block
   @param nb_prefix_samples The number of prefix/suffix/zero samples
   @param etype Type of extension (CYCLIC_PREFIX,CYCLIC_SUFFIX,ZEROS)
*/
void PHY_ofdm_mod(const int *input,
                  int *output,
                  int fftsize,
                  unsigned char nb_symbols,
                  unsigned short nb_prefix_samples,
                  Extension_t etype);
#endif
