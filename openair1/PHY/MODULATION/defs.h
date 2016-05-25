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
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/
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

int slot_fep_ul(LTE_DL_FRAME_PARMS *frame_parms,
                LTE_eNB_COMMON *eNb_common_vars,
                unsigned char l,
                unsigned char Ns,
                unsigned char eNb_id,
                int no_prefix);

void normal_prefix_mod(int32_t *txdataF,int32_t *txdata,uint8_t nsymb,LTE_DL_FRAME_PARMS *frame_parms);

void do_OFDM_mod(int32_t **txdataF, int32_t **txdata, uint32_t frame,uint16_t next_slot, LTE_DL_FRAME_PARMS *frame_parms);

void remove_7_5_kHz(PHY_VARS_eNB *phy_vars_eNB,uint8_t subframe);

void apply_7_5_kHz(PHY_VARS_UE *phy_vars_ue,int32_t*txdata,uint8_t subframe);

void init_prach625(LTE_DL_FRAME_PARMS *frame_parms);

void remove_625_Hz(PHY_VARS_eNB *phy_vars_eNB,int16_t *prach);

void apply_625_Hz(PHY_VARS_UE *phy_vars_ue,int16_t *prach);

#endif
/** @}*/
#endif
