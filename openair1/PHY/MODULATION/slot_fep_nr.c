/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "PHY/defs_gNB.h"
#include "PHY/defs_nr_common.h"
#include "modulation_UE.h"
#include "nr_modulation.h"
#include "PHY/NR_UE_ESTIMATION/nr_estimation.h"
#include "PHY/nr_phy_common/inc/nr_phy_common.h"
#include <common/utils/LOG/log.h>

/* rxdataF should be 16 bytes aligned */
void nr_symbol_fep(const NR_DL_FRAME_PARMS *frame_parms,
                   const int slot,
                   const unsigned char symbol,
                   const int link_type,
                   c16_t *rxdata[frame_parms->nb_antennas_rx],
                   c16_t *rxdataF[frame_parms->nb_antennas_rx],
                   time_stats_t* dft_stats)
{
  AssertFatal(symbol < frame_parms->symbols_per_slot,
              "slot_fep: symbol must be between 0 and %d\n",
              frame_parms->symbols_per_slot - 1);
  AssertFatal(slot < frame_parms->slots_per_frame, "slot_fep: Ns must be between 0 and %d\n", frame_parms->slots_per_frame - 1);

  dft_size_idx_t dftsize = get_dft(frame_parms->ofdm_symbol_size);
  for (unsigned char aa = 0; aa < frame_parms->nb_antennas_rx; aa++) {
    if (dft_stats) start_meas(dft_stats);
    dft(dftsize, (int16_t *)rxdata[aa], (int16_t *)rxdataF[aa], 1);
    if (dft_stats) stop_meas(dft_stats);

    const bool is_sl = (link_type == link_type_sl);
    apply_nr_rotation_symbol_RX(frame_parms->symbols_per_slot,
                                frame_parms->slots_per_subframe,
                                frame_parms->timeshift_symbol_rotation,
                                frame_parms->first_carrier_offset,
                                rxdataF[aa],
                                frame_parms->symbol_rotation[link_type],
                                is_sl ? frame_parms->N_RB_SL : frame_parms->N_RB_DL,
                                slot,
                                symbol);
  }
}

int nr_slot_fep(PHY_VARS_NR_UE *ue,
                const NR_DL_FRAME_PARMS *frame_parms,
                unsigned int slot,
                unsigned int symbol,
                c16_t rxdataF[][frame_parms->samples_per_slot_wCP],
                enum nr_Link linktype,
                uint32_t sample_offset,
                c16_t **rxdata)
{
  AssertFatal(symbol < frame_parms->symbols_per_slot,
              "slot_fep: symbol must be between 0 and %d\n",
              frame_parms->symbols_per_slot - 1);
  AssertFatal(slot < frame_parms->slots_per_frame, "slot_fep: Ns must be between 0 and %d\n", frame_parms->slots_per_frame - 1);

  bool is_sl = (linktype == link_type_sl);
  bool is_synchronized = (ue) ? ue->is_synchronized : false;
  unsigned int nb_prefix_samples = frame_parms->nb_prefix_samples;
  unsigned int nb_prefix_samples0 = (is_synchronized || is_sl) ? frame_parms->nb_prefix_samples0 : nb_prefix_samples;

  // For Sidelink 16 frames worth of samples is processed to find SSB, for 5G-NR 2.
  const unsigned int total_samples = (is_sl) ? 16 * frame_parms->samples_per_frame : 2 * frame_parms->samples_per_frame;

  unsigned int rx_offset = get_samples_slot_timestamp(frame_parms, slot);
  const unsigned int abs_symbol = slot * frame_parms->symbols_per_slot + symbol;
  for (int idx_symb = slot * frame_parms->symbols_per_slot; idx_symb <= abs_symbol; idx_symb++)
    rx_offset += (idx_symb % (0x7 << frame_parms->numerology_index)) ? nb_prefix_samples : nb_prefix_samples0;
  rx_offset += frame_parms->ofdm_symbol_size * symbol;

  rx_offset += sample_offset;

  // use OFDM symbol from within 1/8th of the CP to avoid ISI
  rx_offset -= (nb_prefix_samples / frame_parms->ofdm_offset_divisor);

  LOG_D(PHY,
        "slot_fep: slot %d, symbol %d, nb_prefix_samples %u, nb_prefix_samples0 %u, rx_offset %u energy %d\n",
        slot,
        symbol,
        nb_prefix_samples,
        nb_prefix_samples0,
        rx_offset,
        dB_fixed(signal_energy((int32_t *)&rxdata[0][rx_offset], frame_parms->ofdm_symbol_size)));

  c16_t tmp_dft_in[frame_parms->nb_antennas_rx][frame_parms->ofdm_symbol_size] __attribute__((aligned(32)));
  c16_t *rxdata_symb_ptr[frame_parms->nb_antennas_rx];
  c16_t *rxdataF_symb_ptr[frame_parms->nb_antennas_rx];
  for (unsigned char aa = 0; aa < frame_parms->nb_antennas_rx; aa++) {
    rxdataF_symb_ptr[aa] = &rxdataF[aa][frame_parms->ofdm_symbol_size * symbol];
    // This happens only during initial sync
    if (rx_offset + frame_parms->ofdm_symbol_size > total_samples) {
      // we have to wrap on the end
      memcpy(&tmp_dft_in[aa][0], &rxdata[aa][rx_offset], (total_samples - rx_offset) * sizeof(int32_t));
      memcpy(&tmp_dft_in[aa][total_samples - rx_offset],
             &rxdata[aa][0],
             (frame_parms->ofdm_symbol_size - (total_samples - rx_offset)) * sizeof(int32_t));
      rxdata_symb_ptr[aa] = tmp_dft_in[aa];
    } else {
      rxdata_symb_ptr[aa] = &rxdata[aa][rx_offset];
    }

    if (ue && ue->cont_fo_comp) {
      start_meas_nr_ue_phy(ue, RX_FO_COMPENSATION_STATS);
      nr_fo_compensation(ue->dl_Doppler_shift + ue->freq_offset,
                         frame_parms->samples_per_subframe,
                         rx_offset,
                         rxdata_symb_ptr[aa],
                         tmp_dft_in[aa],
                         frame_parms->ofdm_symbol_size);
      stop_meas_nr_ue_phy(ue, RX_FO_COMPENSATION_STATS);
      rxdata_symb_ptr[aa] = tmp_dft_in[aa];
    }
  }
  time_stats_t* dft_stats = NULL;
  if (ue) dft_stats = &ue->phy_cpu_stats.cpu_time_stats[RX_DFT_STATS];
  nr_symbol_fep(frame_parms, slot, symbol, linktype, rxdata_symb_ptr, rxdataF_symb_ptr, dft_stats);
  return 0;
}

int nr_symbol_fep_ul(const NR_DL_FRAME_PARMS *fp,
                     const c16_t *rxdata,
                     c16_t *rxdataF,
                     unsigned char symbol,
                     unsigned char slot,
                     int sample_offset)
{
  dft_size_idx_t dftsize = get_dft(fp->ofdm_symbol_size);
  // This is for misalignment issues
  int32_t tmp_dft_in[fp->ofdm_symbol_size] __attribute__((aligned(32)));

  // offset of first OFDM symbol
  uint32_t prefix_length = get_samples_symbol_duration(fp, slot, symbol, 1) - fp->ofdm_symbol_size;
  unsigned int rxdata_offset =
      get_samples_slot_timestamp(fp, slot) + get_samples_symbol_timestamp(fp, slot, symbol) + prefix_length;
  // use OFDM symbol from within 1/8th of the CP to avoid ISI
  rxdata_offset -= (fp->nb_prefix_samples / fp->ofdm_offset_divisor);

  int16_t *rxdata_ptr;
  if (rxdata_offset >= sample_offset)
    rxdata_offset -= sample_offset;
  else
    rxdata_offset += fp->samples_per_frame - sample_offset;

  if (rxdata_offset + fp->ofdm_symbol_size > fp->samples_per_frame) {
    memcpy(&tmp_dft_in[0],
           &rxdata[rxdata_offset],
           (fp->samples_per_frame - rxdata_offset) * sizeof(int32_t));
    memcpy(&tmp_dft_in[fp->samples_per_frame - rxdata_offset],
           &rxdata[0],
           (fp->ofdm_symbol_size - fp->samples_per_frame + rxdata_offset) * sizeof(int32_t));
    rxdata_ptr = (int16_t *)tmp_dft_in;
  } else {
    // use dft input from RX buffer directly
    rxdata_ptr = (int16_t *)&rxdata[rxdata_offset];
  }

  dft(dftsize, rxdata_ptr, (int16_t *)rxdataF, 1);

  return 0;
}

void apply_nr_rotation_symbol_fftshifted_RX(const int symbols_per_slot,
                                            const int slots_per_subframe,
                                            const c16_t *shift_rot,
                                            c16_t *rxdataF,
                                            const c16_t *rot,
                                            const int nb_rb,
                                            const int slot,
                                            const int symbol)
{
  const int symb_offset = (slot % slots_per_subframe) * symbols_per_slot;
  c16_t rot2 = rot[symbol + symb_offset];
  rot2.i = -rot2.i;
  rotate_cpx_vector(rxdataF, rot2, rxdataF, nb_rb * NR_NB_SC_PER_RB, 15);
  mult_cpx_vector(rxdataF, shift_rot, rxdataF, nb_rb * NR_NB_SC_PER_RB, 15);
}

void apply_nr_rotation_symbol_RX(const int symbols_per_slot,
                                 const int slots_per_subframe,
                                 const c16_t *shift_rot,
                                 const int first_carrier_offset,
                                 c16_t *rxdataF,
                                 const c16_t *rot,
                                 int nb_rb,
                                 int slot,
                                 int symbol)
{
  const int symb_offset = (slot % slots_per_subframe) * symbols_per_slot;

  c16_t rot2 = rot[symbol + symb_offset];
  rot2.i = -rot2.i;
  LOG_D(PHY, "slot %d, symb_offset %d rotating by %d.%d\n", slot, symb_offset, rot2.r, rot2.i);
  c16_t *this_symbol = rxdataF;

  if (nb_rb & 1) {
    rotate_cpx_vector(this_symbol, rot2, this_symbol, (nb_rb + 1) * 6, 15);
    rotate_cpx_vector(this_symbol + first_carrier_offset - 6, rot2, this_symbol + first_carrier_offset - 6, (nb_rb + 1) * 6, 15);
    mult_cpx_vector(this_symbol, shift_rot, this_symbol, (nb_rb + 1) * 6, 15);
    mult_cpx_vector(this_symbol + first_carrier_offset - 6,
                    shift_rot + first_carrier_offset - 6,
                    this_symbol + first_carrier_offset - 6,
                    (nb_rb + 1) * 6,
                    15);
  } else {
    rotate_cpx_vector(this_symbol, rot2, this_symbol, nb_rb * 6, 15);
    rotate_cpx_vector(this_symbol + first_carrier_offset, rot2, this_symbol + first_carrier_offset, nb_rb * 6, 15);
    mult_cpx_vector(this_symbol, shift_rot, this_symbol, nb_rb * 6, 15);
    mult_cpx_vector(this_symbol + first_carrier_offset,
                    shift_rot + first_carrier_offset,
                    this_symbol + first_carrier_offset,
                    nb_rb * 6,
                    15);
  }
}

void nr_ofdm_demod_and_rx_rotation(c16_t **rxdata,
                                   c16_t **rxdataF,
                                   const NR_DL_FRAME_PARMS *fp,
                                   int nb_antennas,
                                   int slot,
                                   int slot_offsetF,
                                   enum nr_Link linktype,
                                   bool was_symbol_used[NR_SYMBOLS_PER_SLOT])
{
  for (int aa = 0; aa < nb_antennas; aa++) {
    for (uint8_t symbol = 0; symbol < fp->symbols_per_slot; symbol++) {
      if (was_symbol_used[symbol] == true) {
        // OFDM Demod
        nr_symbol_fep_ul(fp, &rxdata[aa][0], &rxdataF[aa][slot_offsetF + symbol * fp->ofdm_symbol_size], symbol, slot, 0);
        // FFT-shift
        fftshift_inplace(&rxdataF[aa][slot_offsetF + symbol * fp->ofdm_symbol_size],
                         fp->N_RB_UL * NR_NB_SC_PER_RB,
                         fp->ofdm_symbol_size);
        // Phase compensation
        apply_nr_rotation_symbol_fftshifted_RX(fp->symbols_per_slot,
                                               fp->slots_per_subframe,
                                               fp->timeshift_symbol_rotation,
                                               &rxdataF[aa][slot_offsetF + symbol * fp->ofdm_symbol_size],
                                               fp->symbol_rotation[linktype],
                                               fp->N_RB_UL,
                                               slot,
                                               symbol);
      }
    }
  }
}
