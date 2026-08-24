/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "PHY/defs_gNB.h"
#include "PHY/impl_defs_nr.h"
#include "phy_ofdm_mod.h"
#include "common/utils/LOG/log.h"
//#define DEBUG_OFDM_MOD

void nr_normal_prefix_mod(c16_t *txdataF,
                          c16_t *txdata,
                          uint8_t nsymb,
                          const NR_DL_FRAME_PARMS *frame_parms,
                          uint32_t slot,
                          bool was_symbol_used[NR_SYMBOLS_PER_SLOT])
{
  // This function works only slot wise. For more generic symbol generation refer nr_feptx0()
  if (frame_parms->numerology_index != 0) { // case where numerology != 0
    if (!(slot%(frame_parms->slots_per_subframe/2))) {
      if (was_symbol_used[0]) {
        PHY_ofdm_mod((int *)txdataF,
                    (int *)txdata,
                    frame_parms->ofdm_symbol_size,
                    1,
                    frame_parms->nb_prefix_samples0,
                    CYCLIC_PREFIX);
      } else {
        memset(txdata, 0, (frame_parms->nb_prefix_samples0 +  frame_parms->ofdm_symbol_size) * sizeof(c16_t));
      }
      for (int i = 1; i < nsymb; i++) {
        c16_t* tx_data_ptr = txdata + (i - 1) * (frame_parms->ofdm_symbol_size + frame_parms->nb_prefix_samples) +
                            frame_parms->ofdm_symbol_size + frame_parms->nb_prefix_samples0;
        if (was_symbol_used[i]) {
          PHY_ofdm_mod((int *)txdataF + frame_parms->ofdm_symbol_size * i,
                      (int *)tx_data_ptr,
                      frame_parms->ofdm_symbol_size,
                      1,
                      frame_parms->nb_prefix_samples,
                      CYCLIC_PREFIX);
        } else {
          memset(tx_data_ptr, 0, (frame_parms->nb_prefix_samples + frame_parms->ofdm_symbol_size) * sizeof(c16_t));
        }
      }
    }
    else {
      for (int i = 0; i < nsymb; i++) {
        c16_t* tx_data_ptr = txdata + i * (frame_parms->ofdm_symbol_size + frame_parms->nb_prefix_samples);
        if (was_symbol_used[i]) {
          PHY_ofdm_mod((int *)txdataF + frame_parms->ofdm_symbol_size * i,
                      (int *)tx_data_ptr,
                      frame_parms->ofdm_symbol_size,
                      1,
                      frame_parms->nb_prefix_samples,
                      CYCLIC_PREFIX);
        } else {
          memset(tx_data_ptr, 0, (frame_parms->nb_prefix_samples + frame_parms->ofdm_symbol_size) * sizeof(c16_t));
        }
      }
    }
  }
  else { // numerology = 0, longer CP for every 7th symbol
      PHY_ofdm_mod((int *)txdataF,
                   (int *)txdata,
                   frame_parms->ofdm_symbol_size,
                   1,
                   frame_parms->nb_prefix_samples0,
                   CYCLIC_PREFIX);
      PHY_ofdm_mod((int *)txdataF + frame_parms->ofdm_symbol_size,
                  (int *)txdata + frame_parms->ofdm_symbol_size + frame_parms->nb_prefix_samples0,
                  frame_parms->ofdm_symbol_size,
                  6,
                  frame_parms->nb_prefix_samples,
                  CYCLIC_PREFIX);
      PHY_ofdm_mod((int *)txdataF + 7*frame_parms->ofdm_symbol_size,
                   (int *)txdata + 6*(frame_parms->ofdm_symbol_size+frame_parms->nb_prefix_samples)
                                 + frame_parms->ofdm_symbol_size + frame_parms->nb_prefix_samples0,
                   frame_parms->ofdm_symbol_size,
                   1,
                   frame_parms->nb_prefix_samples0,
                   CYCLIC_PREFIX);
      PHY_ofdm_mod((int *)txdataF + 8 * frame_parms->ofdm_symbol_size,
                   (int *)txdata + 6 * (frame_parms->ofdm_symbol_size + frame_parms->nb_prefix_samples)
                                 + 2*(frame_parms->ofdm_symbol_size + frame_parms->nb_prefix_samples0),
                   frame_parms->ofdm_symbol_size,
                   6,
                   frame_parms->nb_prefix_samples,
                   CYCLIC_PREFIX);
  }
}

void apply_nr_rotation_TX(const NR_DL_FRAME_PARMS *fp,
                          c16_t *txdataF,
                          bool is_flat_buff,
                          const c16_t *symbol_rotation,
                          int slot,
                          int nb_rb,
                          int first_symbol,
                          int nsymb)
{
  int symb_offset = (slot % fp->slots_per_subframe) * fp->symbols_per_slot;

  symbol_rotation += symb_offset;

  for (int sidx = first_symbol; sidx < first_symbol + nsymb; sidx++) {
    const c16_t this_rotation = symbol_rotation[sidx];
    c16_t *this_symbol = txdataF + sidx * fp->ofdm_symbol_size;

    LOG_D(PHY,"Rotating symbol %d, slot %d, symbol_subframe_index %d (%d,%d)\n",
      sidx,
      slot,
      sidx + symb_offset,
      this_rotation.r,
      this_rotation.i);

    if (is_flat_buff)
      rotate_cpx_vector(this_symbol, this_rotation, this_symbol, nb_rb * NR_NB_SC_PER_RB, 15);
    else {
      c16_t *this_symbol_neg = this_symbol + fp->first_carrier_offset;
      if (nb_rb & 1) {
        this_symbol_neg -= 6;
        nb_rb += 1;
      }
      rotate_cpx_vector(this_symbol, this_rotation, this_symbol, nb_rb * 6, 15);
      rotate_cpx_vector(this_symbol_neg, this_rotation, this_symbol_neg, nb_rb * 6, 15);
    }
  }
}
                       
/* Do FFT-shift for symbols in the provided in buffer and writes to out buffer. */
void fft_shift(const c16_t *in,
               uint32_t in_symb_sz,
               uint16_t num_prb,
               c16_t *out,
               uint16_t fft_size_out,
               uint16_t start_symb,
               uint16_t num_symb)
{
  const int num_samp_half = num_prb * NR_NB_SC_PER_RB / 2;
  const int first_carrier_offset = fft_size_out - num_samp_half;
  for (int s = start_symb; s < start_symb + num_symb; s++) {
    // Copy negative freq component
    uint32_t out_offset = s * fft_size_out + first_carrier_offset;
    uint32_t in_offset = s * in_symb_sz;
    memcpy(out + out_offset, in + in_offset, num_samp_half * sizeof(int32_t));
    // Copy positive freq component
    out_offset = s * fft_size_out;
    in_offset = s * in_symb_sz + num_samp_half;
    memcpy(out + out_offset, in + in_offset, num_samp_half * sizeof(int32_t));
  }
}
