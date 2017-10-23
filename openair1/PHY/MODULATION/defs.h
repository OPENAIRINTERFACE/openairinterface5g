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

#ifndef __MODULATION_DEFS__H__
#define __MODULATION_DEFS__H__
#include "PHY/defs.h"
/** @addtogroup _PHY_MODULATION_
 * @{
*/

/**
\fn void PHY_ofdm_mod(int *input,int *output,int fftsize,unsigned char nb_symbols,unsigned short nb_prefix_samples,Extension_t etype)
This function performs OFDM modulation with cyclic extension or zero-padding.

@param input The sequence input samples in the frequency-domain.  This is a concatenation of the input symbols in SIMD redundant format
@param output The time-domain output signal
@param fftsize size of OFDM symbol size (\f$N_d\f$)
@param nb_symbols The number of OFDM symbols in the block
@param nb_prefix_samples The number of prefix/suffix/zero samples
@param etype Type of extension (CYCLIC_PREFIX,CYCLIC_SUFFIX,ZEROS)

*/
void PHY_ofdm_mod(int *input,
                  int *output,
                  int fftsize,
                  unsigned char nb_symbols,
                  unsigned short nb_prefix_samples,
                  Extension_t etype
                 );

#ifdef OPENAIR_LTE

/*!
\brief This function implements the OFDM front end processor on reception (FEP)
\param phy_vars_ue Pointer to PHY variables
\param l symbol within slot (0..6/7)
\param Ns Slot number (0..19)
\param sample_offset offset within rxdata (points to beginning of subframe)
\param no_prefix if 1 prefix is removed by HW
\param reset_freq_est if non-zero it resets the frequency offset estimation loop
*/

int slot_fep(PHY_VARS_UE *phy_vars_ue,
             unsigned char l,
             unsigned char Ns,
             int sample_offset,
             int no_prefix,
	     int reset_freq_est);

int slot_fep_mbsfn(PHY_VARS_UE *phy_vars_ue,
                   unsigned char l,
                   int subframe,
                   int sample_offset,
                   int no_prefix);

int slot_fep_ul(RU_t *ru,
                unsigned char l,
                unsigned char Ns,
                int no_prefix);

int front_end_fft(PHY_VARS_UE *ue,
             unsigned char l,
             unsigned char Ns,
             int sample_offset,
             int no_prefix);

int front_end_chanEst(PHY_VARS_UE *ue,
             unsigned char l,
             unsigned char Ns,
            int reset_freq_est);

void normal_prefix_mod(int32_t *txdataF,int32_t *txdata,uint8_t nsymb,LTE_DL_FRAME_PARMS *frame_parms);

void do_OFDM_mod(int32_t **txdataF, int32_t **txdata, uint32_t frame,uint16_t next_slot, LTE_DL_FRAME_PARMS *frame_parms);

void do_OFDM_mod_symbol(LTE_eNB_COMMON *eNB_common_vars, RU_COMMON *common, uint16_t next_slot, LTE_DL_FRAME_PARMS *frame_parms,int do_precoding);

void remove_7_5_kHz(RU_t *ru,uint8_t subframe);

void apply_7_5_kHz(PHY_VARS_UE *phy_vars_ue,int32_t*txdata,uint8_t subframe);

void init_prach625(LTE_DL_FRAME_PARMS *frame_parms);

void remove_625_Hz(PHY_VARS_eNB *phy_vars_eNB,int16_t *prach);

void apply_625_Hz(PHY_VARS_UE *phy_vars_ue,int16_t *prach);

/** \brief This function performs beamforming precoding for common
 * data
    @param txdataF Table of pointers for frequency-domain TX signals
    @param txdataF_BF Table of pointers for frequency-domain TX signals
    @param frame_parms Frame descriptor structure
after beamforming
    @param beam_weights Beamforming weights applied on each
antenna element and each carrier
    @param slot Slot number
    @param symbol Symbol index on which to act
    @param aa physical antenna index*/
int beam_precoding(int32_t **txdataF,
	           int32_t **txdataF_BF,
                   LTE_DL_FRAME_PARMS *frame_parms,
	           int32_t ***beam_weights,
                   int slot,
                   int symbol,
                   int aa);

int f_read(char *calibF_fname, int nb_ant, int nb_freq, int32_t **tdd_calib_coeffs);

int estimate_DLCSI_from_ULCSI(int32_t **calib_dl_ch_estimates, int32_t **ul_ch_estimates, int32_t **tdd_calib_coeffs, int nb_ant, int nb_freq);

int compute_BF_weights(int32_t **beam_weights, int32_t **calib_dl_ch_estimates, PRECODE_TYPE_t precode_type, int nb_ant, int nb_freq);


#endif
/** @}*/
#endif
