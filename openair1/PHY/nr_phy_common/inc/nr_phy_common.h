/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef __NR_PHY_COMMON__H__
#define __NR_PHY_COMMON__H__

#include "common/platform_types.h"
#include "common/utils/nr/nr_common.h"

void init_byte2m128i(void);
void freq2time(uint16_t ofdm_symbol_size, int16_t *freq_signal, int16_t *time_signal);
void nr_est_delay(int ofdm_symbol_size, const c16_t *ls_est, c16_t *ch_estimates_time, delay_t *delay);
unsigned int nr_get_tx_amp(int power_dBm, int power_max_dBm, int total_nb_rb, int nb_rb);
void nr_fo_compensation(double fo_Hz, int samples_per_ms, int sample_offset, const c16_t *rxdata_in, c16_t *rxdata_out, int size);
void nr_channel_level(const int symbol,
                      const int size_est,
                      const c16_t ch_estimates_ext[][size_est],
                      const int nb_rx,
                      int32_t avg[nb_rx],
                      const uint32_t len);
void nr_scale_channel(int size, int ch_estimates_ext[][size], int symb, uint32_t len, int nrOfLayers, int nb_rx, int shift_ch_ext);
int nr_get_ssb_start_sc(int scs,
                        int ssb_offset_point_a,
                        int ssb_sco,
                        frequency_range_t freq_range);

#endif
