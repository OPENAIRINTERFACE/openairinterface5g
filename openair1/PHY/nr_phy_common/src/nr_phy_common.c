/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "nr_phy_common.h"
#include <complex.h>
#include "PHY/sse_intrin.h"
#include "PHY/impl_defs_top.h"
#include "PHY/TOOLS/tools_defs.h"
#include "log.h"
#ifdef __aarch64__
#define USE_128BIT
#endif

#define PEAK_DETECT_THRESHOLD 15
simde__m128i byte2m128i[256];
void init_byte2m128i(void)
{
  for (int s = 0; s < 256; s++) {
    byte2m128i[s] = simde_mm_insert_epi16(byte2m128i[s], (1 - 2 * (s & 1)), 0);
    byte2m128i[s] = simde_mm_insert_epi16(byte2m128i[s], (1 - 2 * ((s >> 1) & 1)), 1);
    byte2m128i[s] = simde_mm_insert_epi16(byte2m128i[s], (1 - 2 * ((s >> 2) & 1)), 2);
    byte2m128i[s] = simde_mm_insert_epi16(byte2m128i[s], (1 - 2 * ((s >> 3) & 1)), 3);
    byte2m128i[s] = simde_mm_insert_epi16(byte2m128i[s], (1 - 2 * ((s >> 4) & 1)), 4);
    byte2m128i[s] = simde_mm_insert_epi16(byte2m128i[s], (1 - 2 * ((s >> 5) & 1)), 5);
    byte2m128i[s] = simde_mm_insert_epi16(byte2m128i[s], (1 - 2 * ((s >> 6) & 1)), 6);
    byte2m128i[s] = simde_mm_insert_epi16(byte2m128i[s], (1 - 2 * ((s >> 7) & 1)), 7);
  }
}

void init_delay_table(uint16_t ofdm_symbol_size,
                      int max_delay_comp,
                      int max_ofdm_symbol_size,
                      c16_t delay_table[][max_ofdm_symbol_size])
{
  for (int delay = -max_delay_comp; delay <= max_delay_comp; delay++) {
    for (int k = 0; k < ofdm_symbol_size; k++) {
      double complex delay_cexp = cexp(I * (2.0 * M_PI * k * delay / ofdm_symbol_size));
      delay_table[max_delay_comp + delay][k].r = (int16_t)round(256 * creal(delay_cexp));
      delay_table[max_delay_comp + delay][k].i = (int16_t)round(256 * cimag(delay_cexp));
    }
  }
}

void freq2time(uint16_t ofdm_symbol_size, int16_t *freq_signal, int16_t *time_signal)
{
  const idft_size_idx_t idft_size = get_idft(ofdm_symbol_size);
  idft(idft_size, freq_signal, time_signal, 1);
}

void nr_est_delay(int ofdm_symbol_size, const c16_t *ls_est, c16_t *ch_estimates_time, delay_t *delay)
{
  idft(get_idft(ofdm_symbol_size), (int16_t *)ls_est, (int16_t *)ch_estimates_time, 1);

  int max_pos = delay->delay_max_pos;
  int max_val = delay->delay_max_val;
  const int sync_pos = 0;

  uint64_t mean_val = 0;
  for (int i = 0; i < ofdm_symbol_size; i++) {
    int temp = c16amp2(ch_estimates_time[i]) >> 1;
    mean_val += temp;
    if (temp > max_val) {
      max_pos = i;
      max_val = temp;
    }
  }
  mean_val /= ofdm_symbol_size;

  if (max_pos > ofdm_symbol_size / 2)
    max_pos = max_pos - ofdm_symbol_size;

  delay->delay_max_pos = max_pos;
  delay->delay_max_val = max_val;

  // The peak in general is quite clear. It only gives a small peak when the noise is high, generally obtaining an incorrect
  // estimated delay, and causing the delay compensation to worsen the result instead of improving it. After analyzing several
  // peaks, and doing many tests, a PEAK_DETECT_THRESHOLD = 15 is an adequate value, to apply delay compensation only when there is
  // clearly a peak
  delay->valid = mean_val > 0 && max_val / mean_val > PEAK_DETECT_THRESHOLD;
  delay->est_delay = delay->valid ? max_pos - sync_pos : 0;
}

unsigned int nr_get_tx_amp(int power_dBm, int power_max_dBm, int total_nb_rb, int nb_rb)
{
  // assume power at AMP is 20dBm
  // if gain = 20 (power == 40)
  int gain_dB = power_dBm - power_max_dBm;
  double gain_lin;

  gain_lin = pow(10, .1 * gain_dB);
  if ((nb_rb > 0) && (nb_rb <= total_nb_rb)) {
    return ((int)(AMP * sqrt(gain_lin * total_nb_rb / (double)nb_rb)));
  } else {
    LOG_E(PHY, "Illegal nb_rb/N_RB_UL combination (%d/%d)\n", nb_rb, total_nb_rb);
    // mac_xface->macphy_exit("");
  }
  return (0);
}

// compute average channel_level on each antenna
void nr_channel_level(const int symbol,
                      const int size_est,
                      const c16_t ch_estimates_ext[][size_est],
                      const int nb_rx,
                      int32_t avg[nb_rx],
                      const uint32_t len)
{
  for (int aarx = 0; aarx < nb_rx; aarx++) {
    // compute average squared module
    avg[aarx] = signal_energy_nodc(ch_estimates_ext[aarx] + symbol * len, len);
    LOG_D(PHY, "Channel level: %d\n", avg[aarx]);
  }
}

void nr_fo_compensation(double fo_Hz, int samples_per_ms, int sample_offset, const c16_t *rxdata_in, c16_t *rxdata_out, int size)
{
  const double phase_inc = -fo_Hz / (samples_per_ms * 1000);
  double phase = sample_offset * phase_inc;
  phase -= (int)phase;
#if 1
  // The bottleneck is the calculation of the complex rotation values using get_sin_cos().
  // This code path does not compute these values for the complete OFDM symbol, but only for a smaller CHUNK size.
  // After applying the rotation to a CHUNK size of the output, these rotation values are efficiently rotated further by `rot_vec`.
  // Unfortunately, this propagates small errors from one chunk to the next.
  // Therefore, there is a tradeoff between speed (better with small CHUNK sizes) and accuracy (better with large CHUNK sizes).
#define CHUNK 128
  c16_t rot[CHUNK] __attribute__((aligned(32)));
  for (int i = 0; i < CHUNK; i++) {
    rot[i] = get_sin_cos(phase);
    phase += phase_inc;
  }
  const c16_t rot_vec = get_sin_cos(CHUNK * phase_inc);
  while (size > CHUNK) {
    mult_complex_vectors(rxdata_in, rot, rxdata_out, CHUNK, 14);
    rotate_cpx_vector(rot, rot_vec, rot, CHUNK, 14);
    rxdata_in += CHUNK;
    rxdata_out += CHUNK;
    size -= CHUNK;
  }
  mult_complex_vectors(rxdata_in, rot, rxdata_out, size, 14);
#else
  // This code path computes the complex rotation values for the complete OFDM symbol using get_sin_cos().
  // This is more accurate, but also slower than the code path above.
  c16_t rot[size] __attribute__((aligned(32)));
  for (int i = 0; i < size; i++) {
    rot[i] = get_sin_cos(phase);
    phase += phase_inc;
  }
  mult_complex_vectors(rxdata_in, rot, rxdata_out, size, 14);
#endif
}

/*!
* Setting the first subcarrier
* 3GPP TS 38.211 sections 7.4.3.1 and 4.4.4.2
* for FR1 offsetToPointA and k_SSB are expressed in terms of 15 kHz SCS
* for FR2 offsetToPointA is expressed in terms of 60 kHz SCS and k_SSB expressed in terms of the SCS provided
* by the higher-layer parameter subCarrierSpacingCommon
*/
int nr_get_ssb_start_sc(int scs, int ssb_offset_point_a, int ssb_sco, frequency_range_t freq_range)
{
  const int prb_offset =
      (freq_range == FR1) ? ssb_offset_point_a >> scs : ssb_offset_point_a >> (scs - 2);
  const int sc_offset =
      (freq_range == FR1) ? ssb_sco >> scs : ssb_sco;

  int ssb_start_subcarrier = (12 * prb_offset + sc_offset);

  LOG_D(NR_PHY, "prb_offset:%d, ssb_subcarrier_offset:%d,scs :%d, Fr:%d, ssb_start_subcarrier:%d\n",
                        prb_offset, ssb_sco, scs, freq_range, ssb_start_subcarrier);

  return ssb_start_subcarrier;
}
