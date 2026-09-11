/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "PHY/defs_nr_UE.h"
#include "PHY/TOOLS/tools_defs.h"
#include "PHY/NR_REFSIG/sss_nr.h"
#include "PHY/NR_UE_ESTIMATION/nr_estimation.h"
#include "PHY/MODULATION/modulation_UE.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "SCHED_NR_UE/defs.h"

// Number of symbols carrying SLSS signal  - PSS+SSS+PSBCH
#define SL_NR_NUMSYM_SLSS_NORMAL_CP 14
#define SL_NR_MAX_RX_ANTENNA 1
#define SL_NR_FIRST_PSS_SYMBOL 1
#define SL_NR_FIRST_SSS_SYMBOL 3
#define SL_NR_NUM_PSS_SSS_SYMBOLS 4

// #define SL_DEBUG

static int sl_nr_pss_correlation(PHY_VARS_NR_UE *UE, int frame_index)
{
  sl_nr_ue_phy_params_t *sl_ue = &UE->SL_UE_PHY_PARAMS;
  SL_NR_SYNC_PARAMS_t *sync_params = &sl_ue->sync_params;
  NR_DL_FRAME_PARMS *sl_fp = &UE->SL_UE_PHY_PARAMS.sl_frame_params;
  int16_t **pss_for_correlation = (int16_t **)sl_ue->init_params.sl_pss_for_correlation;

  uint32_t length = (frame_index == 0) ? sl_fp->samples_per_frame + (2 * sl_fp->ofdm_symbol_size) : sl_fp->samples_per_frame;
  int32_t **rxdata = (int32_t **)UE->common_vars.rxdata;

#ifdef SL_DEBUG
  char fname[50], sname[25];
  sprintf(fname, "rxdata_frame_%d.m", frame_index);
  sprintf(sname, "rxd_frame%d", frame_index);
  LOG_M(fname, sname, &rxdata[0][frame_index * sl_fp->samples_per_frame], sl_fp->samples_per_frame, 1, 1);
  LOG_M("pss_for_correlation0.m", "pss_id0", pss_for_correlation[0], 2048, 1, 1);
  LOG_M("pss_for_correlation1.m", "pss_id1", pss_for_correlation[1], 2048, 1, 1);

  int64_t *pss_corr_debug_values[SL_NR_NUM_IDs_IN_PSS];
#endif

  int maxval = 0;
  for (int i = 0; i < 2 * (sl_fp->ofdm_symbol_size); i++) {
    maxval = max(maxval, pss_for_correlation[0][i]);
    maxval = max(maxval, -pss_for_correlation[0][i]);
    maxval = max(maxval, pss_for_correlation[1][i]);
    maxval = max(maxval, -pss_for_correlation[1][i]);
  }

  int shift = log2_approx(maxval); //*(sl_fp->ofdm_symbol_size + sl_fp->nb_prefix_samples)*2);

#ifdef SL_DEBUG
  LOG_I(PHY, "SIDELINK SLSS SEARCH: Function:%s\n", __func__);
  LOG_I(PHY, "maxval:%d, shift:%d\n", maxval, shift);
#endif

  int64_t avg[SL_NR_NUM_IDs_IN_PSS] = {0};
  int64_t peak_value = 0, psss_corr_value = 0;
  unsigned int peak_position = 0, pss_source = 0;
  for (int pss_index = 0; pss_index < SL_NR_NUM_IDs_IN_PSS; pss_index++)
    avg[pss_index] = 0;

#ifdef SL_DEBUG
  int64_t *pss_corr_debug_values[SL_NR_NUM_IDs_IN_PSS];
  for (int pss_index = 0; pss_index < SL_NR_NUM_IDs_IN_PSS; pss_index++)
    pss_corr_debug_values[pss_index] = malloc16_clear(length * sizeof(int64_t));
#endif

  for (int n = 0; n < length - sl_fp->ofdm_symbol_size; n += 4) { //

    for (int pss_index = 0; pss_index < SL_NR_NUM_IDs_IN_PSS; pss_index++) {
      psss_corr_value = 0;

      // calculate dot product of primary_synchro_time_nr and rxdata[ar][n] (ar=0..nb_ant_rx) and store the sum in temp[n];
      for (int ar = 0; ar < sl_fp->nb_antennas_rx; ar++) {
        /* perform correlation of rx data and pss sequence ie it is a dot product */
        const c32_t result = dot_product((c16_t *)pss_for_correlation[pss_index],
                                         (c16_t *)&(rxdata[ar][n + frame_index * sl_fp->samples_per_frame]),
                                         sl_fp->ofdm_symbol_size,
                                         shift);

        const c64_t r64 = {.r = result.r, .i = result.i};
        psss_corr_value += squaredMod(r64);
      }

      // calculate the absolute value of sync_corr[n]
      avg[pss_index] += psss_corr_value;
      if (psss_corr_value > peak_value) {
        peak_value = psss_corr_value;
        peak_position = n;
        pss_source = pss_index;
      }
    }
  }

#ifdef SL_DEBUG
  LOG_M("pss_corr_debug_values_0.m", "pss_corr0", &pss_corr_debug_values[0][0], length, 1, 6);
  LOG_M("pss_corr_debug_values_1.m", "pss_corr1", &pss_corr_debug_values[1][0], length, 1, 6);

  for (int pss_index = 0; pss_index < SL_NR_NUM_IDs_IN_PSS; pss_index++) {
    free(pss_corr_debug_values[pss_index]);
  }
#endif

  double ffo_est = 0;
  if (UE->UE_fo_compensation) { // Not tested

    // fractional frequency offset computation according to Cross-correlation Synchronization Algorithm Using PSS
    // Shoujun Huang, Yongtao Su, Ying He and Shan Tang, "Joint time and frequency offset estimation in LTE downlink," 7th
    // International Conference on Communications and Networking in China, 2012.

    c16_t *pss = (c16_t *)pss_for_correlation[pss_source];
    c16_t *rxd = (c16_t *)&(rxdata[0][peak_position + frame_index * sl_fp->samples_per_frame]);
    int half_symbol = sl_fp->ofdm_symbol_size >> 1;

    // Computing cross-correlation at peak on half the symbol size for first half of data
    c32_t r1 = dot_product(pss, rxd, half_symbol, shift);

    // Computing cross-correlation at peak on half the symbol size for data shifted by half symbol size
    // as it is real and complex it is necessary to shift by a value equal to symbol size to obtain such shift
    c32_t r2 = dot_product(pss + half_symbol, rxd + half_symbol, half_symbol, shift);

    cd_t r1d = {r1.r, r1.i}, r2d = {r2.r, r2.i};
    // estimation of fractional frequency offset: angle[(result1)'*(result2)]/pi
    ffo_est = atan2(r1d.r * r2d.i - r2d.r * r1d.i, r1d.r * r2d.r + r1d.i * r2d.i) / M_PI;

#ifdef SL_DEBUG
    printf("ffo %lf\n", ffo_est);
#endif
  }

  // computing absolute value of frequency offset
  sync_params->freq_offset = ffo_est * sl_fp->subcarrier_spacing;

  for (int pss_index = 0; pss_index < SL_NR_NUM_IDs_IN_PSS; pss_index++)
    avg[pss_index] /= (length / 4);

  sync_params->N_sl_id2 = pss_source;

  LOG_I(PHY,
        "PSS Source = %d, Peak found at pos %d, val = %llu (%d dB) avg %d dB, ffo %lf, freq offset:%d Hz\n",
        pss_source,
        peak_position,
        (unsigned long long)peak_value,
        dB_fixed64(peak_value),
        dB_fixed64(avg[pss_source]),
        ffo_est,
        sync_params->freq_offset);

  if (peak_value < 5 * avg[pss_source])
    return (-1);

  return peak_position;
}

static void sl_nr_extract_sss(PHY_VARS_NR_UE *ue,
                              int32_t *tot_metric,
                              uint8_t *phase_max,
                              c16_t rxdataF[][ue->SL_UE_PHY_PARAMS.sl_frame_params.samples_per_slot_wCP])
{
  c16_t pss_ext[SL_NR_MAX_RX_ANTENNA][SL_NR_NUM_PSS_SYMBOLS][SL_NR_PSS_SEQUENCE_LENGTH];
  c16_t sss_ext[SL_NR_MAX_RX_ANTENNA][SL_NR_NUM_SSS_SYMBOLS][SL_NR_PSS_SEQUENCE_LENGTH];
  uint8_t Nid2 = ue->SL_UE_PHY_PARAMS.sync_params.N_sl_id2;
  NR_DL_FRAME_PARMS *sl_fp = &ue->SL_UE_PHY_PARAMS.sl_frame_params;
  int16_t *d;
  uint16_t Nid1 = 0;
  c16_t *rxF_ext;

  for (int aarx = 0; aarx < sl_fp->nb_antennas_rx; aarx++) {
    unsigned int ofdm_symbol_size = sl_fp->ofdm_symbol_size;

    // pss, sss extraction
    for (int sym = SL_NR_FIRST_PSS_SYMBOL; sym < SL_NR_FIRST_PSS_SYMBOL + SL_NR_NUM_PSS_SSS_SYMBOLS; sym++) {
      if (sym < SL_NR_FIRST_PSS_SYMBOL + SL_NR_NUM_PSS_SYMBOLS) {
        rxF_ext = &pss_ext[aarx][sym - SL_NR_FIRST_PSS_SYMBOL][0];
      } else {
        rxF_ext = &sss_ext[aarx][sym - SL_NR_FIRST_SSS_SYMBOL][0];
      }

      unsigned int k = CIRCULAR_INC(sl_fp->first_carrier_offset, sl_fp->ssb_start_subcarrier + 2, ofdm_symbol_size);
      LOG_D(PHY,
            "firstcarrieroffset:%d, ssb_sc:%d, k:%d, symbol:%d\n",
            sl_fp->first_carrier_offset,
            sl_fp->ssb_start_subcarrier,
            k,
            sym);

      for (int i = 0; i < SL_NR_PSS_SEQUENCE_LENGTH; i++) {
        rxF_ext[i] = rxdataF[aarx][sym * ofdm_symbol_size + k];
        k = CIRCULAR_INC(k, 1, ofdm_symbol_size);
      }
    }

    LOG_D(PHY, "SIDELINK SLSS SEARCH: EXTRACTION OF PSS, SSS done\n");

#ifdef SL_DEBUG
    LOG_M("pss_ext_sym1.m", "pss_ext1", &pss_ext[aarx][0][0], SL_NR_PSS_SEQUENCE_LENGTH, 1, 1);
    LOG_M("pss_ext_sym2.m", "pss_ext2", &pss_ext[aarx][1][0], SL_NR_PSS_SEQUENCE_LENGTH, 1, 1);
    LOG_M("sss_ext_sym3.m", "sss_ext3", &sss_ext[aarx][0][0], SL_NR_PSS_SEQUENCE_LENGTH, 1, 1);
    LOG_M("sss_ext_sym4.m", "sss_ext4", &sss_ext[aarx][1][0], SL_NR_PSS_SEQUENCE_LENGTH, 1, 1);
#endif

    // get conjugated channel estimate from PSS, H* = R* \cdot PSS
    // and do channel estimation and compensation based on PSS
    int16_t *pss = ue->SL_UE_PHY_PARAMS.init_params.sl_pss_for_sync[Nid2];
    c16_t *pss_ext2, *sss_ext2;

    // 2 Symbols each for PSS and SSS
    for (int j = 0; j < SL_NR_NUM_PSS_OR_SSS_SYMBOLS; j++) {
      sss_ext2 = &sss_ext[aarx][j][0];
      pss_ext2 = &pss_ext[aarx][j][0];

      for (int i = 0; i < SL_NR_PSS_SEQUENCE_LENGTH; i++) {
        // This is H*(PSS) = R* \cdot PSS
        c16_t tmp = {.r = (pss_ext2[i].r * pss[i]), .i = (-pss_ext2[i].i * pss[i])};

        int amp = c16amp2(tmp);
        int shift = log2_approx(amp) / 2;

        // This is R(SSS) \cdot H*(PSS)
        c16_t tmp2 = c16mulShift(tmp, sss_ext2[i], shift);

        // MRC on RX antennas
        // sss_ext now contains the compensated SSS
        if (aarx == 0) {
          sss_ext2[i].r = tmp2.r;
          sss_ext2[i].i = tmp2.i;
        } else {
          AssertFatal(1 == 0, "SIDELINK MORE THAN 1 RX ANTENNA NOT YET SUPPORTED\n");
        }
      }
    }

    LOG_D(PHY, "SIDELINK SLSS SEARCH: Ch. estimation SSS done\n");
  }

  /*

  #ifdef SL_DEBUG

    write_output("rxsig0.m","rxs0",&ue->common_vars.rxdata[0][0],ue->frame_parms.samples_per_subframe,1,1);
    write_output("rxdataF0_pss.m","rxF0_pss",&ue->common_vars.rxdataF[0][0],frame_parms->ofdm_symbol_size,1,1);
    write_output("rxdataF0_sss.m","rxF0_sss",&ue->common_vars.rxdataF[0][(SSS_SYMBOL_NB-PSS_SYMBOL_NB)*frame_parms->ofdm_symbol_size],frame_parms->ofdm_symbol_size,1,1);
    write_output("pss_ext.m","pss_ext",pss_ext,LENGTH_PSS_NR,1,1);

  #endif
  */

  /* for phase evaluation, one uses an array of possible phase shifts */
  /* then a correlation is done between received signal with a shift pĥase and the reference signal */
  /* Computation of signal with shift phase is based on below formula */
  /* cosinus cos(x + y) = cos(x)cos(y) - sin(x)sin(y) */
  /* sinus   sin(x + y) = sin(x)cos(y) + cos(x)sin(y) */

  // now do the SSS detection based on the pre computed SSS sequences
  *tot_metric = INT_MIN;
  c16_t *sss = &sss_ext[0][0][0];

  for (uint16_t id1 = 0; id1 < SL_NR_NUM_IDs_IN_SSS; id1++) { // all possible SSS Nid1 values
    for (int phase = -15; phase < 16; phase += 2) { // phase offset between PSS and SSS

      int32_t metric = 0, metric_re = 0;
      const c64_t rot = (c64_t){round(cos(M_PI / 3 / 15 * (phase)) * 32767), round(sin(M_PI / 3 / 15 * (phase)) * 32767)};
      d = (int16_t *)&ue->SL_UE_PHY_PARAMS.init_params.sl_sss_for_sync[Nid2 * SL_NR_NUM_IDs_IN_SSS + id1];

      // This is the inner product using one particular value of each unknown parameter
      for (int i = 0; i < SL_NR_SSS_SEQUENCE_LENGTH; i++) {
        // metric is only real part because sss is a pure real signal (imaginary is 0)
        metric_re += d[i] * ((rot.r * sss[i].r - rot.i * sss[i].i) >> 15);
      }

      metric = metric_re;

      // if the current metric is better than the last save it
      if (metric > *tot_metric) {
        *tot_metric = metric;
        Nid1 = id1;
        *phase_max = phase;

        LOG_D(PHY,
              "(phase,Nid1) (%d,%d), metric_phase %d tot_metric %d, phase_max %d \n",
              phase,
              Nid1,
              metric,
              *tot_metric,
              *phase_max);
      }
    }
  }

  ue->SL_UE_PHY_PARAMS.sync_params.N_sl_id1 = Nid1;
  ue->SL_UE_PHY_PARAMS.sync_params.N_sl_id =
      ue->SL_UE_PHY_PARAMS.sync_params.N_sl_id1 + SL_NR_NUM_IDs_IN_SSS * ue->SL_UE_PHY_PARAMS.sync_params.N_sl_id2;
  LOG_I(PHY,
        "UE[%d]NR-SL SLSS SEARCH: SSS Processing over. id2 from SSS:%d, id1 from PSS:%d, SLSS id:%d\n",
        ue->Mod_id,
        ue->SL_UE_PHY_PARAMS.sync_params.N_sl_id1,
        ue->SL_UE_PHY_PARAMS.sync_params.N_sl_id2,
        ue->SL_UE_PHY_PARAMS.sync_params.N_sl_id);

#ifdef SL_DEBUG
#define SSS_METRIC_FLOOR_NR_SL (30000)
  if (*tot_metric > SSS_METRIC_FLOOR_NR_SL) {
    Nid2 = ue->SL_UE_PHY_PARAMS.sync_params.N_sl_id2;
    Nid1 = ue->SL_UE_PHY_PARAMS.sync_params.N_sl_id1;
    printf("Nid2 %d Nid1 %d tot_metric %d, phase_max %d \n", Nid2, Nid1, *tot_metric, *phase_max);
  }
#endif

  return;
}

// Right now 2 frames worth of samples get processed for PSS in OAI.
// For PSS in Sidelink, worst case 1 SSB in 16 frames can be present
// Hence 16 frames worth of samples needs to be correlated to find the PSS.
nr_initial_sync_t sl_nr_slss_search(PHY_VARS_NR_UE *UE, UE_nr_rxtx_proc_t *proc, int num_frames)
{
  sl_nr_ue_phy_params_t *sl_ue = &UE->SL_UE_PHY_PARAMS;
  SL_NR_SYNC_PARAMS_t *sync_params = &sl_ue->sync_params;
  NR_DL_FRAME_PARMS *sl_fp = &UE->SL_UE_PHY_PARAMS.sl_frame_params;

  int32_t sync_pos = -1; // sync_pos_frame = -1;
  int32_t metric_tdd_ncp = 0;
  uint8_t phase_tdd_ncp = 0;
  double im, re;
  int ret = -1;
  uint16_t rx_slss_id = 65535;

  nr_initial_sync_t result = {false, 0};

#ifdef SL_DEBUG_SEARCH_SLSS
  LOG_D(PHY, "SIDELINK SEARCH SLSS: Function:%s\n", __func__);
#endif

  /*   Initial synchronisation
   *
   *                                 1 radio frame = 10 ms
   *     <--------------------------------------------------------------------------->
   *     |                                 Received UE data buffer                    |
   *     ----------------------------------------------------------------------------
   *     <-------------->|psbch|pss|pss|sss|sss|psbch sym5-sym 12|sym13 - guard|
   *          sync_pos            SS/PSBCH block
   */

  // initial sync performed on 16 successive frames. Worst case - one PSBCH can be sent in 16 frames.
  // If psbch passes on first frame, no need to process second frame
  // Problem with the frame approach is that
  // --------- SSB can be on the boundary between frames. In this case if only 1 SSB is sent we will miss it.
  // rxdata will hold 16 frames + slot worth of samples. This needs to be processed to find the best SSB
  for (int frame_index = 0; frame_index < num_frames; frame_index++) {
    /* process pss search on received buffer */
    sync_pos = sl_nr_pss_correlation(UE, frame_index);

    if (sync_pos == -1) {
      LOG_I(PHY, "SIDELINK SEARCH SLSS: No PSSS found in this frame\n");
      continue;
    }

    sync_pos += frame_index * sl_fp->samples_per_frame; // position in the num_frames frame samples

    for (int pss_sym = 1; pss_sym < 3; pss_sym++) {
      // Now Sync pos can point to PSS 1st symbol or 2nd symbol.
      // Right now implemented the strategy to try both locations for FFT
      // Think about a better correlation strategy
      if (pss_sym == 1) { // Check if sync pos points to SYMBOL1 - first symbol of PSS location
        if (sync_pos > sl_fp->nb_prefix_samples0 + sl_fp->ofdm_symbol_size + sl_fp->nb_prefix_samples)
          sync_params->ssb_offset = sync_pos - (sl_fp->nb_prefix_samples0 + sl_fp->ofdm_symbol_size + sl_fp->nb_prefix_samples);
        else
          sync_params->ssb_offset = sync_pos + sl_fp->samples_per_frame
                                    - (sl_fp->nb_prefix_samples0 + sl_fp->ofdm_symbol_size + sl_fp->nb_prefix_samples);
      } else { // Check if sync pos points to SYMBOL2 - second symbol of PSS location
        if (sync_pos >= sl_fp->nb_prefix_samples0 + 2 * (sl_fp->ofdm_symbol_size + sl_fp->nb_prefix_samples))
          sync_params->ssb_offset =
              sync_pos - (sl_fp->nb_prefix_samples0 + 2 * (sl_fp->ofdm_symbol_size + sl_fp->nb_prefix_samples));
        else
          sync_params->ssb_offset = sync_pos + sl_fp->samples_per_frame
                                    - (sl_fp->nb_prefix_samples0 + 2 * (sl_fp->ofdm_symbol_size + sl_fp->nb_prefix_samples));
      }

      LOG_I(PHY,
            "UE[%d]SIDELINK SEARCH SLSS: PSS Peak at %d, PSS sym:%d, Estimated PSS position %d\n",
            UE->Mod_id,
            sync_pos,
            pss_sym,
            sync_params->ssb_offset);

      int slss_block_samples = (SL_NR_NUMSYM_SLSS_NORMAL_CP * sl_fp->ofdm_symbol_size)
                               + (SL_NR_NUMSYM_SLSS_NORMAL_CP - 1) * sl_fp->nb_prefix_samples + sl_fp->nb_prefix_samples0;

      int ssb_end_position = sync_params->ssb_offset + slss_block_samples;

      LOG_D(PHY,
            "ssb_end:%d ssb block samples:%d total samples: %d\n",
            ssb_end_position,
            slss_block_samples,
            num_frames * sl_fp->samples_per_frame);

      /* check that SSS/PBCH block is continuous inside the received buffer */
      if (ssb_end_position < num_frames * sl_fp->samples_per_frame) {
        // digital compensation of FFO for SSB symbols
        if (UE->UE_fo_compensation) { // This code to be checked. Why do we do this before PSS detection is successful?
          double s_time = 1 / (1.0e3 * sl_fp->samples_per_subframe); // sampling time
          double off_angle = -2 * M_PI * s_time * (sync_params->freq_offset); // offset rotation angle compensation per sample

          int start = sync_params->ssb_offset; // start for offset correction is at ssb_offset (pss time position)
          // Adapt this for other numerologies number of symbols with larger cp increases TBD
          int end = ssb_end_position; // loop over samples in all symbols (ssb size), including prefix

          LOG_I(PHY,
                "SLSS SEARCH: FREQ comp of SLSS samples. Freq_OFSET:%d, startpos:%d, end_pos:%d\n",
                sync_params->freq_offset,
                start,
                end);
          for (int n = start; n < end; n++) {
            for (int ar = 0; ar < sl_fp->nb_antennas_rx; ar++) {
              re = ((double)(((short *)UE->common_vars.rxdata[ar]))[2 * n]);
              im = ((double)(((short *)UE->common_vars.rxdata[ar]))[2 * n + 1]);
              ((short *)UE->common_vars.rxdata[ar])[2 * n] = (short)(round(re * cos(n * off_angle) - im * sin(n * off_angle)));
              ((short *)UE->common_vars.rxdata[ar])[2 * n + 1] = (short)(round(re * sin(n * off_angle) + im * cos(n * off_angle)));
            }
          }
        }

        NR_DL_FRAME_PARMS *frame_parms = &UE->SL_UE_PHY_PARAMS.sl_frame_params;
        const uint32_t rxdataF_sz = frame_parms->samples_per_slot_wCP;
        __attribute__((aligned(32))) c16_t rxdataF[frame_parms->nb_antennas_rx][rxdataF_sz];

        /* In order to achieve correct processing for NR prefix samples is forced to 0 and then restored after function call */
        int16_t psbch_e_rx[SL_NR_POLAR_PSBCH_E_NORMAL_CP + 2] = {0};
        int16_t psbch_unClipped[SL_NR_POLAR_PSBCH_E_NORMAL_CP + 2] = {0};
        int psbch_e_rx_offset = 0;

        for (int symbol = 0; symbol < SL_NR_NUMSYM_SLSS_NORMAL_CP; symbol++) {
          nr_slot_fep(UE,
                      frame_parms,
                      proc->nr_slot_rx,
                      symbol,
                      rxdataF,
                      link_type_sl,
                      sync_params->ssb_offset,
                      UE->common_vars.rxdata);
        }

        /* TODO: change this function to use new rxdataF format */
        sl_nr_extract_sss(UE, &metric_tdd_ncp, &phase_tdd_ncp, rxdataF);

        // save detected cell id to psbch
        rx_slss_id = UE->SL_UE_PHY_PARAMS.sync_params.N_sl_id;

        uint8_t decoded_output[4];

        for (int symbol = 0; symbol < SL_NR_NUMSYM_SLSS_NORMAL_CP - 1;) {
          __attribute__((aligned(32))) struct complex16 dl_ch_estimates[frame_parms->nb_antennas_rx][frame_parms->ofdm_symbol_size];
          __attribute__((aligned(32))) c16_t rxdataF_symb[frame_parms->nb_antennas_rx][frame_parms->ofdm_symbol_size];
          for (int aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
            /* TODO: Change sl sss extract to follow the new rxdataF format */
            memcpy(rxdataF_symb[aarx],
                   &rxdataF[aarx][symbol * frame_parms->ofdm_symbol_size],
                   sizeof(c16_t) * frame_parms->ofdm_symbol_size);
            nr_pbch_channel_estimation(frame_parms,
                                       &UE->SL_UE_PHY_PARAMS,
                                       dl_ch_estimates[aarx],
                                       proc,
                                       symbol,
                                       0,
                                       0,
                                       frame_parms->ssb_start_subcarrier,
                                       rxdataF_symb[aarx],
                                       true,
                                       rx_slss_id);
          }
          nr_generate_psbch_llr(frame_parms,
                                rxdataF_symb,
                                dl_ch_estimates,
                                symbol,
                                &psbch_e_rx_offset,
                                psbch_e_rx,
                                psbch_unClipped);

          UE->adjust_rxgain = nr_sl_psbch_rsrp_measurements(UE, sl_ue, frame_parms, symbol, rxdataF, false);

          symbol = (symbol == 0) ? 5 : symbol + 1;
        }

        ret = nr_psbch_decode(UE, psbch_e_rx, proc, psbch_e_rx_offset, rx_slss_id, NULL, decoded_output);

        result.cell_detected = (ret == 0);

        if (result.cell_detected) { // Check this later TBD
          // sync at symbol ue->symbol_offset
          // computing the offset wrt the beginning of the frame
          // SSB located at symbol 0
          sync_params->remaining_frames =
              (num_frames * sl_fp->samples_per_frame - sync_params->ssb_offset) / sl_fp->samples_per_frame;
          // ssb_offset points to start of sl-ssb
          // rx_offset points to remaining samples needed to fill a frame
          sync_params->rx_offset = sync_params->ssb_offset % sl_fp->samples_per_frame;

          LOG_I(PHY,
                "UE[%d]SIDELINK SLSS SEARCH: PSBCH RX OK. Remainingframes:%d, rx_offset:%d\n",
                UE->Mod_id,
                sync_params->remaining_frames,
                sync_params->rx_offset);

          uint32_t psbch_payload = (*(uint32_t *)decoded_output);
          // retrieve DFN and slot number from SL-MIB
          sync_params->DFN = (((psbch_payload & 0x0700) >> 1) | ((psbch_payload & 0xFE0000) >> 17));
          sync_params->slot_offset = (((psbch_payload & 0x010000) >> 10) | ((psbch_payload & 0xFC000000) >> 26));

          LOG_I(PHY,
                "UE[%d]SIDELINK SLSS SEARCH: SL-MIB: DFN:%d, slot:%d.\n",
                UE->Mod_id,
                sync_params->DFN,
                sync_params->slot_offset);

          UE->init_sync_frame = sync_params->remaining_frames;
          result.rx_offset = sync_params->rx_offset;

          nr_sidelink_indication_t sl_indication;
          sl_nr_rx_indication_t rx_ind = {0};
          uint16_t number_pdus = 1;
          nr_fill_sl_indication(&sl_indication, &rx_ind, NULL, proc, UE, NULL);
          nr_fill_sl_rx_indication(&rx_ind, SL_NR_RX_PDU_TYPE_SSB, UE, number_pdus, (void *)decoded_output, rx_slss_id);

          LOG_D(PHY, "Sidelink SLSS SEARCH PSBCH RX OK. Send SL-SSB TO MAC\n");

          if (UE->if_inst && UE->if_inst->sl_indication)
            UE->if_inst->sl_indication(&sl_indication);

          break;
        }

        LOG_I(PHY,
              "SIDELINK SLSS SEARCH: SLSS ID: %d metric %d, phase %d, psbch CRC %s\n",
              sl_ue->sync_params.N_sl_id,
              metric_tdd_ncp,
              phase_tdd_ncp,
              (ret == 0) ? "OK" : "NOT OK");

      } else {
        LOG_W(PHY, "SIDELINK SLSS SEARCH: Error: Not enough samples to process PSBCH. sync_pos %d\n", sync_pos);
      }
    }
    if (result.cell_detected)
      break;
  }

  if (!result.cell_detected) { // PSBCH not found so indicate sync to higher layers and configure frame parameters
    LOG_E(PHY, "SIDELINK SLSS SEARCH: PSBCH not received. Estimated PSS position:%d\n", sync_pos);
  }

  return result;
}
