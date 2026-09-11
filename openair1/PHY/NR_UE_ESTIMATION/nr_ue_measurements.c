/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*!
 * \brief UE measurements routines
 */

#include "executables/softmodem-common.h"
#include "executables/nr-softmodem-common.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/INIT/nr_phy_init.h"
#include "common/utils/LOG/log.h"
#include "PHY/sse_intrin.h"
#include "SCHED_NR_UE/defs.h"
#include "PHY/NR_REFSIG/sss_nr.h"
#include "PHY/NR_REFSIG/pss_nr.h"
#include "PHY/NR_REFSIG/ss_pbch_nr.h"
#include "PHY/MODULATION/modulation_UE.h"
#include "PHY/NR_REFSIG/nr_refsig.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "PHY/NR_UE_ESTIMATION/nr_estimation.h"

#define K1 ((long long int) 512)
#define K2 ((long long int) (1024-K1))

//#define DEBUG_MEAS_RRC
//#define DEBUG_MEAS_UE
//#define DEBUG_RANK_EST

void nr_ue_measurements(PHY_VARS_NR_UE *ue,
                        const UE_nr_rxtx_proc_t *proc,
                        int number_rbs,
                        uint16_t l,
                        uint32_t pdsch_est_size,
                        int32_t dl_ch_estimates[][pdsch_est_size])
{
  int slot = proc->nr_slot_rx;
  int aarx, aatx, gNB_id = 0;
  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  int ch_offset = frame_parms->ofdm_symbol_size * l;
  ue->measurements.nb_antennas_rx = frame_parms->nb_antennas_rx;

  allocCast3D(rx_spatial_power,
              int,
              ue->measurements.rx_spatial_power,
              NUMBER_OF_CONNECTED_gNB_MAX,
              cmax(frame_parms->nb_antenna_ports_gNB, 1),
              cmax(frame_parms->nb_antennas_rx, 1),
              false);
  allocCast3D(rx_spatial_power_dB,
              short,
              ue->measurements.rx_spatial_power_dB,
              NUMBER_OF_CONNECTED_gNB_MAX,
              cmax(frame_parms->nb_antenna_ports_gNB, 1),
              cmax(frame_parms->nb_antennas_rx, 1),
              false);

  // signal measurements
  for (gNB_id = 0; gNB_id < ue->n_connected_gNB; gNB_id++) {
    ue->measurements.rx_power_tot[gNB_id] = 0;
    for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
      int rx_power = 0;
      for (aatx = 0; aatx < frame_parms->nb_antenna_ports_gNB; aatx++) {
        const int z = signal_energy_nodc((c16_t*)&dl_ch_estimates[gNB_id][ch_offset], number_rbs * NR_NB_SC_PER_RB);
        rx_spatial_power[gNB_id][aatx][aarx] = z;
        if (rx_spatial_power[gNB_id][aatx][aarx] < 0)
          rx_spatial_power[gNB_id][aatx][aarx] = 0;
        rx_spatial_power_dB[gNB_id][aatx][aarx] = dB_fixed(rx_spatial_power[gNB_id][aatx][aarx]);
        rx_power += rx_spatial_power[gNB_id][aatx][aarx];
      }
      ue->measurements.rx_power_tot[gNB_id] += rx_power;
    }
    ue->measurements.rx_power_tot_dB[gNB_id] =  dB_fixed(ue->measurements.rx_power_tot[gNB_id]);
  }

  // filter to remove jitter
  if (ue->init_averaging == 0) {

    for (gNB_id = 0; gNB_id < ue->n_connected_gNB; gNB_id++)
      ue->measurements.rx_power_avg[gNB_id] = (int)((K1 * ue->measurements.rx_power_avg[gNB_id] + K2 * ue->measurements.rx_power_tot[gNB_id]) >> 10);

    ue->measurements.n0_power_avg = (int)((K1 * ue->measurements.n0_power_avg + K2 * ue->measurements.n0_power_tot) >> 10);

    LOG_D(PHY, "Noise Power Computation: K1 %lld K2 %lld n0 avg %u n0 tot %u\n", K1, K2, ue->measurements.n0_power_avg, ue->measurements.n0_power_tot);

  } else {

    for (gNB_id = 0; gNB_id < ue->n_connected_gNB; gNB_id++)
      ue->measurements.rx_power_avg[gNB_id] = ue->measurements.rx_power_tot[gNB_id];

    ue->measurements.n0_power_avg = ue->measurements.n0_power_tot;
    ue->init_averaging = 0;

  }

  for (gNB_id = 0; gNB_id < ue->n_connected_gNB; gNB_id++) {

    ue->measurements.rx_power_avg_dB[gNB_id] = dB_fixed( ue->measurements.rx_power_avg[gNB_id]);
    ue->measurements.n0_power_avg_dB = dB_fixed(ue->measurements.n0_power_avg);
    ue->measurements.wideband_cqi_tot[gNB_id] = ue->measurements.rx_power_tot_dB[gNB_id] - ue->measurements.n0_power_tot_dB;
    ue->measurements.wideband_cqi_avg[gNB_id] = ue->measurements.rx_power_avg_dB[gNB_id] - ue->measurements.n0_power_avg_dB;
    ue->measurements.rx_rssi_dBm[gNB_id] =
        ue->measurements.rx_power_avg_dB[gNB_id] + 30 - SQ15_SQUARED_NORM_FACTOR_DB
        - ((int)openair0_cfg_g[ue->rf_map.card].rx_gain[0] - (int)openair0_cfg_g[ue->rf_map.card].rx_gain_offset[0])
        - dB_fixed(ue->frame_parms.ofdm_symbol_size);

    LOG_D(PHY, "[gNB %d] Slot %d, RSSI %d dB (%d dBm/RE), WBandCQI %d dB, rxPwrAvg %d, n0PwrAvg %d\n",
      gNB_id,
      slot,
      ue->measurements.rx_power_avg_dB[gNB_id],
      ue->measurements.rx_rssi_dBm[gNB_id],
      ue->measurements.wideband_cqi_avg[gNB_id],
      ue->measurements.rx_power_avg[gNB_id],
      ue->measurements.n0_power_tot);
  }
}

// This function calculates:
// - SS reference signal received digital power in dB/RE
uint32_t nr_ue_calculate_ssb_rsrp(const NR_DL_FRAME_PARMS *fp,
                                  const c16_t rxdataF[][fp->ofdm_symbol_size],
                                  int ssb_start_subcarrier)
{
  const int k_start = 56;
  const int k_end = 183;
  uint64_t rsrp = 0;

  for (int aarx = 0; aarx < fp->nb_antennas_rx; aarx++) {
    const c16_t *rxF_sss = rxdataF[aarx];
    unsigned int re = CIRCULAR_INC(fp->first_carrier_offset, ssb_start_subcarrier + k_start, fp->ofdm_symbol_size);
    for (int k = k_start; k < k_end; k++) {
      rsrp += squaredMod(rxF_sss[re]);
      re = CIRCULAR_INC(re, 1, fp->ofdm_symbol_size);
    }
  }

  rsrp /= fp->nb_antennas_rx * (k_end - k_start);
  LOG_D(PHY, "RSRP/nb_re: %ld\n", rsrp);
  return rsrp;
}

// Send SSB RSRP measurement to MAC
static void send_ssb_rsrp_meas(PHY_VARS_NR_UE *ue,
                               const UE_nr_rxtx_proc_t *proc,
                               uint16_t Nid_cell,
                               int rsrp_dBm,
                               int ssb_index,
                               float sinr_dB)
{
  if (!ue->if_inst || !ue->if_inst->dl_indication)
    return;

  fapi_nr_l1_measurements_t l1_measurements = {
      .gNB_index = proc->gNB_id,
      .meas_type = NFAPI_NR_SS_MEAS,
      .Nid_cell = Nid_cell,
      .is_neighboring_cell = false,
      .rsrp_dBm = rsrp_dBm,
      .ssb_index = ssb_index,
      .sinr_dB = sinr_dB,
  };

  fapi_nr_rx_indication_t rx_ind;
  rx_ind.number_pdus = 0;
  nr_fill_rx_indication(&rx_ind, FAPI_NR_MEAS_IND, ue, 0, 0, NULL, proc, &l1_measurements);
  nr_downlink_indication_t dl_indication = (nr_downlink_indication_t){.gNB_index = proc->gNB_id,
                                                                      .module_id = ue->Mod_id,
                                                                      .cc_id = ue->CC_id,
                                                                      .hfn = proc->hfn_rx,
                                                                      .frame = proc->frame_rx,
                                                                      .slot = proc->nr_slot_rx,
                                                                      .rx_ind = &rx_ind};
  ue->if_inst->dl_indication(&dl_indication);
}

// Send neighboring-cell SSB RSRP measurement directly to RRC via ITTI
static void send_neighbor_cell_meas(PHY_VARS_NR_UE *ue, const UE_nr_rxtx_proc_t *proc, uint16_t Nid_cell, int rsrp_dBm)
{
  if (ue->if_inst && ue->if_inst->meas_ind)
    ue->if_inst->meas_ind(ue->Mod_id, proc->gNB_id, Nid_cell, false, true, rsrp_dBm);
}

// This function implements:
// - SS reference signal received power (SS-RSRP) as per clause 5.1.1 of 3GPP TS 38.215 version 16.3.0 Release 16
// - no Layer 3 filtering implemented (no filterCoefficient provided from RRC)
// Todo:
// - Layer 3 filtering according to clause 5.5.3.2 of 3GPP TS 38.331 version 16.2.0 Release 16
// Measurement units:
// - RSRP:    W (dBW)
// - RX Gain  dB
void nr_ue_ssb_rsrp_measurements(PHY_VARS_NR_UE *ue,
                                 int ssb_index,
                                 const UE_nr_rxtx_proc_t *proc,
                                 const c16_t rxdataF[ue->frame_parms.nb_antennas_rx][ue->frame_parms.ofdm_symbol_size])
{
  const NR_DL_FRAME_PARMS *fp = &ue->frame_parms;

  uint32_t rsrp_avg = nr_ue_calculate_ssb_rsrp(fp, rxdataF, fp->ssb_start_subcarrier);
  float rsrp_db_per_re = 10 * log10(rsrp_avg);

  openair0_config_t *cfg = &openair0_cfg_g[ue->rf_map.card];

  if (rsrp_avg == 0)
    ue->measurements.ssb_rsrp_dBm[ssb_index] = -200; // lower than any value to be reported per Table 10.1.6.1-1 of 38.133
  else
    ue->measurements.ssb_rsrp_dBm[ssb_index] = rsrp_db_per_re + 30 - SQ15_SQUARED_NORM_FACTOR_DB
                                               - ((int)cfg->rx_gain[0] - (int)cfg->rx_gain_offset[0])
                                               - dB_fixed(fp->ofdm_symbol_size);

  // to obtain non-integer dB value with a resoluion of 0.5dB
  uint32_t signal_pwr = rsrp_avg > ue->measurements.n0_power_avg ? rsrp_avg - ue->measurements.n0_power_avg : 0;
  int SNRtimes10 = dB_fixed_x10(signal_pwr) - dB_fixed_x10(ue->measurements.n0_power_avg);
  ue->measurements.ssb_sinr_dB[ssb_index] = SNRtimes10 / 10.0;

  LOG_D(PHY,
        "[UE %d] ssb %d SS-RSRP: %d dBm/RE (%f dB/RE), SS-SINR: %f dB\n",
        ue->Mod_id,
        ssb_index,
        ue->measurements.ssb_rsrp_dBm[ssb_index],
        rsrp_db_per_re,
        ue->measurements.ssb_sinr_dB[ssb_index]);

  // Send SS measurements to MAC
  send_ssb_rsrp_meas(ue,
                     proc,
                     ue->frame_parms.Nid_cell,
                     ue->measurements.ssb_rsrp_dBm[ssb_index],
                     ssb_index,
                     ue->measurements.ssb_sinr_dB[ssb_index]);
}

static void reset_neighboring_cell_info(fapi_nr_neighboring_cell_t *neighbor_cell,
                                        neighboring_cell_info_t *neighboring_cell_info,
                                        uint32_t samples_per_slot_wCP)
{
  neighboring_cell_info->pss_search_start = 0;
  neighboring_cell_info->pss_search_length = samples_per_slot_wCP;
  neighboring_cell_info->ssb_slot = -1;
  neighboring_cell_info->valid_meas = false;
  neighboring_cell_info->consec_fail = 0;
  neighbor_cell->is_candidate = false;
  if (!neighbor_cell->Nid_cell_was_configured)
    neighbor_cell->Nid_cell = -1;
}

static bool search_neighboring_cell(UE_nr_rxtx_proc_t *proc,
                                    NR_DL_FRAME_PARMS *frame_parms,
                                    fapi_nr_neighboring_cell_t *nr_neighboring_cell,
                                    neighboring_cell_info_t *neighboring_cell_info,
                                    c16_t **rxdata,
                                    uint32_t rxdata_size,
                                    c16_t rxdataF[][frame_parms->nb_antennas_rx][frame_parms->ofdm_symbol_size],
                                    c16_t pssTime[][frame_parms->ofdm_symbol_size],
                                    const uint16_t *exclude_nid_cells,
                                    int num_exclude_nid_cells)
{
  nr_ssb_search_params_t search_params = {
      .dl_CarrierFreq = frame_parms->dl_CarrierFreq,
      .sampling_rate = frame_parms->samples_per_subframe * 1000,
      .slots_per_frame = frame_parms->slots_per_frame,
      .slots_per_subframe = frame_parms->slots_per_subframe,
      .numerology_index = frame_parms->numerology_index,
      .ofdm_symbol_size = frame_parms->ofdm_symbol_size,
      .ofdm_offset_divisor = frame_parms->ofdm_offset_divisor,
      .nb_antennas_rx = frame_parms->nb_antennas_rx,
      .symbols_per_slot = frame_parms->symbols_per_slot,
      .first_carrier_offset = frame_parms->first_carrier_offset,
      .N_RB_DL = frame_parms->N_RB_DL,
      .rxdata_size = frame_parms->samples_per_slot_wCP,
      .rxdata = rxdata,
      .nb_prefix_samples = frame_parms->nb_prefix_samples,
      .nb_prefix_samples0 = frame_parms->nb_prefix_samples0,
      .ssb_start_subcarrier = frame_parms->ssb_start_subcarrier,
      .subcarrier_spacing = frame_parms->subcarrier_spacing,
      .samples_per_slot_wCP = frame_parms->samples_per_slot_wCP,
      .target_nid_cell = -1, // Blind search
      .exclude_nid_cells = exclude_nid_cells,
      .num_exclude_nid_cells = num_exclude_nid_cells,
      .apply_freq_offset = false,
      .fo_flag = false,
      .rxdataF = rxdataF,
      .pssTime = pssTime,
  };

  bool cell_detected = false;
  if (nr_search_ssb_common(&search_params)) {
    int pbch_initial_symbol = 1;
    const int N_L = (frame_parms->Lmax == 4) ? 4 : 8;
    const int N_hf = (frame_parms->Lmax == 4) ? 2 : 1;
    double metric = 0;
    // loops over possible pbch dmrs cases to retrieve best estimated i_ssb (and n_hf for Lmax=4) for multiple ssb detection
    for (int hf = 0; hf < N_hf; hf++) {
      for (int l = 0; l < N_L; l++) {
        // computing correlation between received DMRS symbols and transmitted sequence for current i_ssb and n_hf
        cd_t cumul = {0};
        for (int i = pbch_initial_symbol; i < pbch_initial_symbol + 3; i++) {
          c32_t meas = nr_pbch_dmrs_correlation(frame_parms,
                                                i,
                                                i - pbch_initial_symbol,
                                                search_params.sss_res.nid_cell,
                                                search_params.ssb_start_subcarrier,
                                                nr_gold_pbch(frame_parms->Lmax, search_params.sss_res.nid_cell, hf, l),
                                                rxdataF[i]);
          csum(cumul, cumul, meas);
        }
        double tmp = squaredMod(cumul);
        if (metric < tmp) {
          metric = tmp;
        }
      }
    }
    cell_detected = metric > NR_PBCH_DMRS_METRIC_FLOOR ? true : false;
  }

  if (cell_detected) {
    nr_neighboring_cell->Nid_cell = search_params.sss_res.nid_cell;
    nr_neighboring_cell->is_candidate = true;
    LOG_D(NR_PHY,
          "Found neighbor cell PCI=%d (pss peak pos =%d, pss_peak=%d dB, pss_avg=%d dB)\n",
          search_params.sss_res.nid_cell,
          search_params.pss_res.pos,
          search_params.pss_res.peak,
          search_params.pss_res.avg);

    // Update search window
    neighboring_cell_info->pss_search_start = search_params.pss_res.pos - 16;
    neighboring_cell_info->pss_search_length = 32;
    neighboring_cell_info->ssb_slot = proc->nr_slot_rx;
  }

  return cell_detected;
}

static bool validate_known_pci(NR_DL_FRAME_PARMS *frame_parms,
                               fapi_nr_neighboring_cell_t *nr_neighboring_cell,
                               neighboring_cell_info_t *neighboring_cell_info,
                               c16_t **rxdata,
                               c16_t rxdataF[][frame_parms->nb_antennas_rx][frame_parms->ofdm_symbol_size],
                               c16_t pssTime[][frame_parms->ofdm_symbol_size],
                               int slot)
{
  int known_pci = nr_neighboring_cell->Nid_cell;

  int length = neighboring_cell_info->pss_search_length;
  c16_t *rx[frame_parms->nb_antennas_rx];
  for (int i = 0; i < frame_parms->nb_antennas_rx; i++)
    rx[i] = rxdata[i] + neighboring_cell_info->pss_search_start;

  pss_search_t p_pss = (pss_search_t){.rxdata = rx,
                                      .nb_antennas_rx = frame_parms->nb_antennas_rx,
                                      .rxdata_length = length,
                                      .ofdm_symbol_size = frame_parms->ofdm_symbol_size,
                                      .nb_prefix_samples = 0,
                                      .subcarrier_spacing = frame_parms->subcarrier_spacing,
                                      .fo_flag = false,
                                      .target_Nid_cell = known_pci,
                                      .pssTime = (c16_t *)pssTime};
  nr_pss_info_t pss_info = pss_search_time_nr(&p_pss);
  pss_detection_result_t pss_res = pss_info.pss_elem_info[0];

  if (!pss_res.success) {
    if (neighboring_cell_info->valid_meas)
      neighboring_cell_info->consec_fail++;
    LOG_D(NR_PHY,
          "PSS validation failed for PCI=%d (slot=%d, search window: start=%d, length=%d, peak=%d dB, avg=%d dB), consec_fail=%d\n",
          known_pci,
          slot,
          neighboring_cell_info->pss_search_start,
          length,
          pss_res.peak,
          pss_res.avg,
          neighboring_cell_info->consec_fail);
    return false;
  }

  int ssb_time_offset = neighboring_cell_info->pss_search_start + pss_res.pos - frame_parms->nb_prefix_samples;
  if (ssb_time_offset < 0) {
    if (neighboring_cell_info->valid_meas)
      neighboring_cell_info->consec_fail++;
    return false; // pss position is too close to buffer beginning
  }

  __attribute__((aligned(32))) c16_t rxdataF_tmp[frame_parms->nb_antennas_rx][frame_parms->samples_per_slot_wCP];
  uint8_t sss_symbol = SSS_SYMBOL_NB - PSS_SYMBOL_NB;
  nr_slot_fep(NULL, frame_parms, 0, 0, rxdataF_tmp, link_type_dl, ssb_time_offset, (c16_t **)rxdata);
  nr_slot_fep(NULL, frame_parms, 0, sss_symbol, rxdataF_tmp, link_type_dl, ssb_time_offset, (c16_t **)rxdata);
  /* TODO: Once symbol based PDSCH proc is imeplemented, nr_slot_fep() will use
  the new rxdataF buffer format so the following memcpy can be removed. */
  for (int aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
    memcpy(rxdataF[0][aarx], &rxdataF_tmp[aarx][0], sizeof(c16_t) * frame_parms->ofdm_symbol_size);
    memcpy(rxdataF[sss_symbol][aarx],
           &rxdataF_tmp[aarx][sss_symbol * frame_parms->ofdm_symbol_size],
           sizeof(c16_t) * frame_parms->ofdm_symbol_size);
  }

  nr_sss_params_t p_sss = (nr_sss_params_t){.nb_antennas_rx = frame_parms->nb_antennas_rx,
                                            .samples_per_slot_wCP = frame_parms->samples_per_slot_wCP,
                                            .ofdm_symbol_size = frame_parms->ofdm_symbol_size,
                                            .first_carrier_offset = frame_parms->first_carrier_offset,
                                            .ssb_start_subcarrier = frame_parms->ssb_start_subcarrier,
                                            .subcarrier_spacing = frame_parms->subcarrier_spacing,
                                            .exclude_nid_cells = NULL,
                                            .num_exclude_nid_cells = 0};
  sss_detection_result_t res = rx_sss_nr(&p_sss, &pss_res, known_pci, rxdataF);

  if (!res.success) {
    if (neighboring_cell_info->valid_meas)
      neighboring_cell_info->consec_fail++;
    LOG_D(NR_PHY, "Known PCI validation failed for PCI=%d, consec_fail=%d\n", known_pci, neighboring_cell_info->consec_fail);
    return false;
  }

  LOG_D(NR_PHY, "Known PCI validation completed for PCI=%d\n", known_pci);
  nr_neighboring_cell->is_candidate = false;
  neighboring_cell_info->consec_fail = 0;
  neighboring_cell_info->valid_meas = true;
  neighboring_cell_info->pss_search_start += pss_res.pos - 16;
  neighboring_cell_info->pss_search_length = 32;
  neighboring_cell_info->ssb_slot = slot;

  return true;
}

static void handle_blind_search(fapi_nr_neighboring_cell_t *nr_neighboring_cell, uint32_t ssb_freq)
{
  bool found = false;
  for (int n = 0; n < NUMBER_OF_NEIGHBORING_CELLS_MAX; n++) {
    fapi_nr_neighboring_cell_t *cell = &nr_neighboring_cell[n];
    if (found && cell->active == 1 && cell->ssb_freq == ssb_freq && cell->Nid_cell == (uint16_t)-1) {
      cell->active = 0;
      cell->ssb_freq = 0;
    }
    if (cell->active == 1 && cell->ssb_freq == ssb_freq && cell->Nid_cell == (uint16_t)-1) {
      found = true;
    }
  }

  if (found)
    return;

  for (int n = 0; n < NUMBER_OF_NEIGHBORING_CELLS_MAX; n++) {
    fapi_nr_neighboring_cell_t *cell = &nr_neighboring_cell[n];
    if (cell->active == 0) {
      cell->active = true;
      cell->ssb_freq = ssb_freq;
      cell->Nid_cell = -1;
      cell->Nid_cell_was_configured = false;
      return;
    }
  }
}

static void search_new_neighboring_cell(UE_nr_rxtx_proc_t *proc, PHY_VARS_NR_UE *ue, c16_t **rxdata, uint32_t rxdata_size)
{
  // Generate PSS time-domain sequences once for all neighbor cells
  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  __attribute__((aligned(32))) c16_t pssTime[NUMBER_PSS_SEQUENCE][frame_parms->ofdm_symbol_size];
  for (int nid2_idx = 0; nid2_idx < NUMBER_PSS_SEQUENCE; nid2_idx++) {
    generate_pss_nr_time(frame_parms->ofdm_symbol_size,
                         frame_parms->first_carrier_offset,
                         nid2_idx,
                         frame_parms->ssb_start_subcarrier,
                         pssTime[nid2_idx]);
  }

  // Build list of already discovered PCIs (serving cell + neighbor cells) for exclusion during blind search
  uint16_t exclude_nid_cells[NUMBER_OF_NEIGHBORING_CELLS_MAX + 1];
  exclude_nid_cells[0] = frame_parms->Nid_cell;
  int num_exclude_nid_cells = 1;
  for (int i = 0; i < NUMBER_OF_NEIGHBORING_CELLS_MAX; i++) {
    fapi_nr_neighboring_cell_t *cell = &ue->nrUE_config.meas_config.nr_neighboring_cell[i];
    if (cell->active && cell->Nid_cell != (uint16_t)-1 && cell->Nid_cell != frame_parms->Nid_cell) {
      exclude_nid_cells[num_exclude_nid_cells++] = cell->Nid_cell;
    }
  }

  const uint32_t rxdataF_sz = frame_parms->ofdm_symbol_size;
  __attribute__((aligned(32))) c16_t rxdataF[NR_N_SYMBOLS_SSB][frame_parms->nb_antennas_rx][rxdataF_sz];

  for (int cell_idx = 0; cell_idx < NUMBER_OF_NEIGHBORING_CELLS_MAX; cell_idx++) {
    fapi_nr_neighboring_cell_t *neighbor_cell = &ue->nrUE_config.meas_config.nr_neighboring_cell[cell_idx];
    if (neighbor_cell->active == 0 || neighbor_cell->Nid_cell != (uint16_t)-1) {
      continue;
    }

    neighboring_cell_info_t *neighboring_cell_info = &ue->measurements.neighboring_cell_info[cell_idx];

    bool neighbor_found = search_neighboring_cell(proc,
                                                  frame_parms,
                                                  neighbor_cell,
                                                  neighboring_cell_info,
                                                  rxdata,
                                                  rxdata_size,
                                                  rxdataF,
                                                  pssTime,
                                                  exclude_nid_cells,
                                                  num_exclude_nid_cells);
    if (neighbor_found) {
      // Add it to the exclusion list so that the same cell is not found again during the loop.
      exclude_nid_cells[num_exclude_nid_cells++] = neighbor_cell->Nid_cell;
      // The same frequency may contain other Nid_cells. Since no Nid_cell has been configured,
      // we have to continue searching for other Nid_cells.
      handle_blind_search(ue->nrUE_config.meas_config.nr_neighboring_cell, neighbor_cell->ssb_freq);
    }
  }
}

static void do_neighboring_cell_measurements(UE_nr_rxtx_proc_t *proc, PHY_VARS_NR_UE *ue, c16_t **rxdata)
{
  // Generate PSS time-domain sequences once for all neighbor cells
  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  __attribute__((aligned(32))) c16_t pssTime[NUMBER_PSS_SEQUENCE][frame_parms->ofdm_symbol_size];
  for (int nid2_idx = 0; nid2_idx < NUMBER_PSS_SEQUENCE; nid2_idx++) {
    generate_pss_nr_time(frame_parms->ofdm_symbol_size,
                         frame_parms->first_carrier_offset,
                         nid2_idx,
                         frame_parms->ssb_start_subcarrier,
                         pssTime[nid2_idx]);
  }

  const uint32_t rxdataF_sz = frame_parms->ofdm_symbol_size;
  __attribute__((aligned(32))) c16_t rxdataF[NR_N_SYMBOLS_SSB][frame_parms->nb_antennas_rx][rxdataF_sz];

  for (int cell_idx = 0; cell_idx < NUMBER_OF_NEIGHBORING_CELLS_MAX; cell_idx++) {
    fapi_nr_neighboring_cell_t *neighbor_cell = &ue->nrUE_config.meas_config.nr_neighboring_cell[cell_idx];
    if (neighbor_cell->active == 0 || neighbor_cell->Nid_cell == (uint16_t)-1 || neighbor_cell->Nid_cell == frame_parms->Nid_cell) {
      continue;
    }

    neighboring_cell_info_t *neighboring_cell_info = &ue->measurements.neighboring_cell_info[cell_idx];
    if (neighboring_cell_info->pss_search_length == 0) {
      neighboring_cell_info->pss_search_length = frame_parms->samples_per_slot_wCP;
      neighboring_cell_info->ssb_slot = -1;
    }

    if (neighboring_cell_info->ssb_slot != -1 && neighboring_cell_info->ssb_slot != proc->nr_slot_rx)
      continue;

    if (!validate_known_pci(frame_parms, neighbor_cell, neighboring_cell_info, rxdata, rxdataF, pssTime, proc->nr_slot_rx)) {
      if (neighboring_cell_info->consec_fail >= NEIGHBOR_CELL_MAX_CONSECUTIVE_FAILURES) {
        if (neighbor_cell->is_candidate) {
          LOG_D(NR_PHY, "Neighbor cell confirmation failed for candidate with PCI=%d\n", neighbor_cell->Nid_cell);
        } else {
          LOG_D(NR_PHY, "Max consecutive failures reached for PCI=%d, resetting to full search\n", neighbor_cell->Nid_cell);
          send_neighbor_cell_meas(ue, proc, neighbor_cell->Nid_cell, INT_MAX);
        }
        reset_neighboring_cell_info(neighbor_cell, neighboring_cell_info, frame_parms->samples_per_slot_wCP);
      }
      continue;
    }

    // RSRP measurements
    uint8_t sss_symbol = SSS_SYMBOL_NB - PSS_SYMBOL_NB;
    neighboring_cell_info->ssb_rsrp = nr_ue_calculate_ssb_rsrp(frame_parms, rxdataF[sss_symbol], frame_parms->ssb_start_subcarrier);

    neighboring_cell_info->ssb_rsrp_dBm =
        10 * log10(neighboring_cell_info->ssb_rsrp) + 30 - SQ15_SQUARED_NORM_FACTOR_DB
        - ((int)openair0_cfg_g[ue->rf_map.card].rx_gain[0] - (int)openair0_cfg_g[ue->rf_map.card].rx_gain_offset[0])
        - dB_fixed(ue->frame_parms.ofdm_symbol_size);

    // Send SS measurements to RRC directly
    send_neighbor_cell_meas(ue, proc, neighbor_cell->Nid_cell, neighboring_cell_info->ssb_rsrp_dBm);
  }
}

void nr_ue_meas_neighboring_cell(void *arg)
{
  nr_meas_task_args_t *args = (nr_meas_task_args_t *)arg;
  c16_t *rx[args->nb_ant];
  for (int i = 0; i < args->nb_ant; i++)
    rx[i] = args->rxdata_ant + i * args->rxdata_size;
  do_neighboring_cell_measurements(&args->proc, args->ue, rx);

  args->ue->measurements.meas_request_pending = false;
  free(args);
}

void nr_ue_search_new_neighboring_cell(void *arg)
{
  nr_meas_task_args_t *args = (nr_meas_task_args_t *)arg;
  c16_t *rx[args->nb_ant];
  for (int i = 0; i < args->nb_ant; i++)
    rx[i] = args->rxdata_ant + i * args->rxdata_size;
  search_new_neighboring_cell(&args->proc, args->ue, rx, args->rxdata_size);

  args->ue->measurements.search_new_cells_pending = false;
  free(args);
}

// This function computes the received noise power
// Measurement units:
// - psd_awgn (AWGN power spectral density):     dBm/Hz
void nr_ue_rrc_measurements(PHY_VARS_NR_UE *ue,
                            const UE_nr_rxtx_proc_t *proc,
                            const c16_t rxdataF[ue->frame_parms.nb_antennas_rx][ue->frame_parms.ofdm_symbol_size])
{
  int slot = proc->nr_slot_rx;
  const int16_t *rxF_sss;
  const uint8_t k_left = 48;
  const uint8_t k_right = 183;
  const uint8_t k_length = 8;
  unsigned int ssb_offset = ue->frame_parms.first_carrier_offset + ue->frame_parms.ssb_start_subcarrier;
  double rx_gain = openair0_cfg_g[ue->rf_map.card].rx_gain[0];
  double rx_gain_offset = openair0_cfg_g[ue->rf_map.card].rx_gain_offset[0];

  ue->measurements.n0_power_tot = 0;

  LOG_D(PHY, "In %s doing measurements for ssb_offset %d \n", __FUNCTION__, ssb_offset);

  for (int aarx = 0; aarx < ue->frame_parms.nb_antennas_rx; aarx++) {
    uint32_t n0_power = 0;
    rxF_sss = (int16_t *)rxdataF[aarx];

    //-ve spectrum from SSS
    for (int k = k_left; k < k_left + k_length; k++) {
      int re = (ssb_offset + k) % ue->frame_parms.ofdm_symbol_size;

      #ifdef DEBUG_MEAS_RRC
      LOG_I(PHY, "In %s -rxF_sss %d %d\n", __FUNCTION__, rxF_sss[re*2], rxF_sss[re*2 + 1]);
      #endif

      n0_power += (((int32_t)rxF_sss[re * 2] * rxF_sss[re * 2]) + ((int32_t)rxF_sss[re * 2 + 1] * rxF_sss[re * 2 + 1]));
    }

    //+ve spectrum from SSS
    for (int k = k_right; k < k_right + k_length; k++) {
      int re = (ssb_offset + k) % ue->frame_parms.ofdm_symbol_size;

      #ifdef DEBUG_MEAS_RRC
      LOG_I(PHY, "In %s +rxF_sss %d %d\n", __FUNCTION__, rxF_sss[re*2], rxF_sss[re*2 + 1]);
      #endif

      n0_power += (((int32_t)rxF_sss[re * 2]*rxF_sss[re * 2]) + ((int32_t)rxF_sss[re * 2 + 1] * rxF_sss[re * 2 + 1]));
    }

    n0_power /= 2 * k_length;
    ue->measurements.n0_power_tot += n0_power;
  }

  ue->measurements.n0_power_tot_dB = dB_fixed(ue->measurements.n0_power_tot);

  #ifdef DEBUG_MEAS_RRC
  const int psd_awgn = -174;
  const int scs = 15000 * (1 << ue->frame_parms.numerology_index);
  const int nf_usrp = ue->measurements.n0_power_tot_dB + 3 + 30 - ((int)rx_gain - (int)rx_gain_offset) - SQ15_SQUARED_NORM_FACTOR_DB - (psd_awgn + dB_fixed(scs) + dB_fixed(ue->frame_parms.ofdm_symbol_size));
  LOG_D(PHY, "In [%s][slot:%d] NF USRP %d dB\n", __FUNCTION__, slot, nf_usrp);
  #endif

  LOG_D(PHY,
        "In [%s][slot:%d] Noise Level %d (digital level %d dB, noise power spectral density %f dBm/RE)\n",
        __FUNCTION__,
        slot,
        ue->measurements.n0_power_tot,
        ue->measurements.n0_power_tot_dB,
        ue->measurements.n0_power_tot_dB + 30 - SQ15_SQUARED_NORM_FACTOR_DB - dB_fixed(ue->frame_parms.ofdm_symbol_size)
            - ((int)rx_gain - (int)rx_gain_offset));
}

// This function implements:
// - PSBCH RSRP calculations according to 38.215 section 5.1.22 Release 16
// - PSBCH DMRS used for calculations
// - TBD: SSS REs for calculation.
// Measurement units:
// - RSRP:    W (dBW)
// returns RXgain to be adjusted based on target rx power (50db) - received digital power in db/RE
int nr_sl_psbch_rsrp_measurements(PHY_VARS_NR_UE *ue,
                                  sl_nr_ue_phy_params_t *sl_phy_params,
                                  const NR_DL_FRAME_PARMS *fp,
                                  const int symbol,
                                  const c16_t rxdataF[][fp->ofdm_symbol_size],
                                  bool use_SSS)
{
  SL_NR_UE_PSBCH_t *psbch_rx = &sl_phy_params->psbch;
  uint8_t maxsym = (fp->Ncp) ? SL_NR_NUM_SYMBOLS_SSB_EXT_CP : SL_NR_NUM_SYMBOLS_SSB_NORMAL_CP;
  uint8_t numsym = (fp->Ncp) ? 8 : 10;
  uint32_t re_offset = fp->first_carrier_offset + fp->ssb_start_subcarrier;
  uint32_t rsrp = 0, num_re = 0;

  LOG_D(PHY, "PSBCH RSRP MEAS: numsym:%d, re_offset:%d\n", numsym, re_offset);

  for (int aarx = 0; aarx < fp->nb_antennas_rx; aarx++) {
    // Calculate PSBCH RSRP based from DMRS REs
    const struct complex16 *rxF = rxdataF[aarx];

    for (int re = 0; re < SL_NR_NUM_PSBCH_RE_IN_ONE_SYMBOL; re++) {
      if (re % 4 == 0) { // DMRS RE
        uint16_t offset = (re_offset + re) % fp->ofdm_symbol_size;

        rsrp += c16amp2(rxF[offset]);
        num_re++;
      }
    }
  }

  if (use_SSS) {
    // TBD...
    // UE can decide between using only PSBCH DMRS or PSBCH DMRS and SSS for PSBCH RSRP computation.
    // If needed this can be implemented. Reference Spec 38.215
  }

  // Reset values
  if (symbol == 0) {
    psbch_rx->rsrp_dB_per_RE = 0;
    psbch_rx->rsrp_dBm_per_RE = 0;
    psbch_rx->rsrp_sum = 0;
    psbch_rx->rsrp_re_sum = 0;
  }
  // Sum uptill symbol
  psbch_rx->rsrp_sum += rsrp;
  psbch_rx->rsrp_re_sum += num_re;

  int adjust_rxgain = 0;
  // Average of all REs in slot
  if (symbol == maxsym - 1) {
    psbch_rx->rsrp_dB_per_RE = 10 * log10(psbch_rx->rsrp_sum / psbch_rx->rsrp_re_sum);
    psbch_rx->rsrp_dBm_per_RE =
        psbch_rx->rsrp_dB_per_RE + 30 - SQ15_SQUARED_NORM_FACTOR_DB
        - ((int)openair0_cfg_g[ue->rf_map.card].rx_gain[0] - (int)openair0_cfg_g[ue->rf_map.card].rx_gain_offset[0])
        - dB_fixed(fp->ofdm_symbol_size);
    adjust_rxgain = TARGET_RX_POWER - psbch_rx->rsrp_dB_per_RE;
    LOG_D(PHY,
          "PSBCH RSRP (DMRS REs): numREs:%d RSRP :%d dB/RE ,RSRP:%d dBm/RE, adjust_rxgain:%d dB\n",
          psbch_rx->rsrp_re_sum,
          psbch_rx->rsrp_dB_per_RE,
          psbch_rx->rsrp_dBm_per_RE,
          adjust_rxgain);
  }

  return adjust_rxgain;
}
