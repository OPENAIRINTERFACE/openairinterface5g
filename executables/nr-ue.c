/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "PHY/defs_nr_common.h"
#define _GNU_SOURCE // For pthread_setname_np
#include <pthread.h>
#include "executables/nr-ue-ru.h"
#include "executables/nr-uesoftmodem.h"
#include "PHY/INIT/nr_phy_init.h"
#include "NR_MAC_UE/mac_proto.h"
#include "RRC/NR_UE/rrc_proto.h"
#include "RRC/NR_UE/L2_interface_ue.h"
#include "SCHED_NR_UE/defs.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "executables/softmodem-common.h"
#include "radio/COMMON/common_lib.h"
#include "LAYER2/nr_pdcp/nr_pdcp_oai_api.h"
#include "LAYER2/nr_rlc/nr_rlc_oai_api.h"
#include "openair1/PHY/TOOLS/phy_scope_interface.h"
#include "instrumentation.h"
#include "common/utils/threadPool/notified_fifo.h"
#include "position_interface.h"
#include "nr_phy_common.h"
#include "PHY/MODULATION/nr_modulation.h"
#include "common/utils/time_manager/time_manager.h"
#include "log.h"

/*
 *  NR SLOT PROCESSING SEQUENCE
 *
 *  Processing occurs with following steps for connected mode:
 *
 *  - Rx samples for a slot are received,
 *  - PDCCH processing (including DCI extraction for downlink and uplink),
 *  - PDSCH processing (including transport blocks decoding),
 *  - PUCCH/PUSCH (transmission of acknowledgements, CSI, ... or data).
 *
 *  Time between reception of the slot and related transmission depends on UE processing performance.
 *  It is defined by the value NR_UE_CAPABILITY_SLOT_RX_TO_TX.
 *
 *  In NR, network gives the duration between Rx slot and Tx slot in the DCI:
 *  - for reception of a PDSCH and its associated acknowledgment slot (with a PUCCH or a PUSCH),
 *  - for reception of an uplink grant and its associated PUSCH slot.
 *
 *  So duration between reception and it associated transmission depends on its transmission slot given in the DCI.
 *  NR_UE_CAPABILITY_SLOT_RX_TO_TX means the minimum duration but higher duration can be given by the network because UE can support it.
 *
 *                                                                                                    Slot k
 *                                                                                  -------+------------+--------
 *                Frame                                                                    | Tx samples |
 *                Subframe                                                                 |   buffer   |
 *                Slot n                                                            -------+------------+--------
 *       ------ +------------+--------                                                     |
 *              | Rx samples |                                                             |
 *              |   buffer   |                                                             |
 *       -------+------------+--------                                                     |
 *                           |                                                             |
 *                           V                                                             |
 *                           +------------+                                                |
 *                           |   PDCCH    |                                                |
 *                           | processing |                                                |
 *                           +------------+                                                |
 *                           |            |                                                |
 *                           |            v                                                |
 *                           |            +------------+                                   |
 *                           |            |   PDSCH    |                                   |
 *                           |            | processing | decoding result                   |
 *                           |            +------------+    -> ACK/NACK of PDSCH           |
 *                           |                         |                                   |
 *                           |                         v                                   |
 *                           |                         +-------------+------------+        |
 *                           |                         | PUCCH/PUSCH | Tx samples |        |
 *                           |                         |  processing | transfer   |        |
 *                           |                         +-------------+------------+        |
 *                           |                                                             |
 *                           |/___________________________________________________________\|
 *                            \  duration between reception and associated transmission   /
 *
 * Remark: processing is done slot by slot, it can be distribute on different threads which are executed in parallel.
 * This is an architecture optimization in order to cope with real time constraints.
 * By example, for LTE, subframe processing is spread over 4 different threads.
 *
 */

static void start_process_slot_tx(void* arg) {
  notifiedFIFO_elt_t *newTx = arg;
  nr_rxtx_thread_data_t *curMsgTx = NotifiedFifoData(newTx);
  int num_ul_actors = get_nrUE_params()->num_ul_actors;
  if (num_ul_actors > 0) {
    pushNotifiedFIFO(&curMsgTx->UE->ul_actors[curMsgTx->proc.nr_slot_tx % num_ul_actors].fifo, newTx);
  } else {
    newTx->processingFunc(curMsgTx);
  }
}

static size_t dump_L1_UE_meas_stats(PHY_VARS_NR_UE *ue, char *output, size_t max_len)
{
  const char *begin = output;
  const char *end = output + max_len;
  for (int i = 0; i < MAX_CPU_STAT_TYPE; i++) {
    output += print_meas_log(&ue->phy_cpu_stats.cpu_time_stats[i],
                             ue->phy_cpu_stats.cpu_time_stats[i].meas_name,
                             NULL,
                             NULL,
                             output,
                             end - output);
  }
  return output - begin;
}

static void *nrL1_UE_stats_thread(void *param)
{
  PHY_VARS_NR_UE *ue = (PHY_VARS_NR_UE *) param;
  const int max_len = 16384;
  char output[max_len];
  char filename[30];
  snprintf(filename, 29, "nrL1_UE_stats-%d.log", ue->Mod_id);
  filename[29] = 0;
  FILE *fd = fopen(filename, "w");
  AssertFatal(fd != NULL, "Cannot open %s\n", filename);

  while (!oai_exit) {
    sleep(1);
    const int len = dump_L1_UE_meas_stats(ue, output, max_len);
    AssertFatal(len < max_len, "exceeded length\n");
    fwrite(output, len + 1, 1, fd); // + 1 for terminating NULL byte
    fflush(fd);
    fseek(fd, 0, SEEK_SET);
  }
  fclose(fd);

  return NULL;
}

static int determine_N_TA_offset(PHY_VARS_NR_UE *ue) {
  if (ue->sl_mode == 2)
    return 0;
  else {
    int N_TA_offset = ue->nrUE_config.cell_config.N_TA_offset;
    if (N_TA_offset == -1) {
      return set_default_nta_offset(ue->frame_parms.freq_range, ue->frame_parms.samples_per_subframe);
    } else {
      // Return N_TA_offet in samples, as described in 38.211 4.1 and 4.3.1
      // T_c[s] =  1/(Δf_max x N_f) = 1 / (480 * 1000 * 4096)
      // N_TA_offset[s] = N_TA_offset x T_c
      // N_TA_offset[samples] = samples_per_second x N_TA_offset[s]
      // N_TA_offset[samples] = N_TA_offset x samples_per_subframe x 1000 x T_c
      return (N_TA_offset * ue->frame_parms.samples_per_subframe) / (4096 * 480);
    }
  }
}

void init_nr_ue_vars(PHY_VARS_NR_UE *ue, uint8_t UE_id)
{
  int nb_connected_gNB = 1;

  ue->Mod_id      = UE_id;
  ue->if_inst     = nr_ue_if_module_init(UE_id);
  ue->dci_thres   = 0;
  ue->target_Nid_cell = -1;

  // initialize all signal buffers
  init_nr_ue_signal(ue, nb_connected_gNB);

  // intialize transport
  init_nr_ue_transport(ue);

  // Initialization of measurement variables
  init_phy_nr_measurements(ue);

  ue->ta_frame = -1;
  ue->ta_slot = -1;
}

/*!
 * It performs band scanning and synchonization.
 * \param arg is a pointer to a \ref PHY_VARS_NR_UE structure.
 */

typedef struct {
  PHY_VARS_NR_UE *UE;
  UE_nr_rxtx_proc_t proc;
  nr_gscn_info_t gscnInfo[MAX_GSCN_BAND];
  int numGscn;
  int rx_offset;
} syncData_t;

static void UE_synch(void *arg) {
  syncData_t *syncD = (syncData_t *)arg;
  PHY_VARS_NR_UE *UE = syncD->UE;
  UE->is_synchronized = 0;

  if (UE->target_Nid_cell != -1) {
    LOG_W(NR_PHY, "Starting re-sync detection for target Nid_cell %i\n", UE->target_Nid_cell);
  } else {
    LOG_W(NR_PHY, "Starting sync detection\n");
  }

  LOG_I(PHY, "[UE thread Synch] Running Initial Synch \n");

  uint64_t dl_carrier, ul_carrier;
  const NR_DL_FRAME_PARMS *fp = &UE->frame_parms;
  nr_initial_sync_t ret = {false, 0, 0};
  if (UE->sl_mode == 2) {
    fp = &UE->SL_UE_PHY_PARAMS.sl_frame_params;
    dl_carrier = fp->sl_CarrierFreq;
    ul_carrier = fp->sl_CarrierFreq;
    ret = sl_nr_slss_search(UE, &syncD->proc, SL_NR_SSB_REPETITION_IN_FRAMES);
  } else {
    nr_get_carrier_frequencies(UE, &dl_carrier, &ul_carrier);
    ret = nr_initial_sync(&syncD->proc, UE, 2, syncD->gscnInfo, syncD->numGscn);
  }

  if (ret.cell_detected) {
    syncD->rx_offset = ret.rx_offset;
    const int freq_offset = UE->common_vars.freq_offset; // frequency offset computed with pss in initial sync
    const int hw_slot_offset =
        ((ret.rx_offset << 1) / fp->samples_per_subframe * fp->slots_per_subframe)
        + round((float)((ret.rx_offset << 1) % fp->samples_per_subframe) / fp->samples_per_slot0);

    UE->freq_offset = freq_offset - UE->dl_Doppler_shift;
    if (!get_nrUE_params()->cont_fo_comp) {
      // rerun with new cell parameters and frequency-offset
      nrue_ru_set_freq(UE, ul_carrier, dl_carrier, freq_offset);
    }

    if (get_nrUE_params()->agc) {
      nrue_ru_adjust_rx_gain(UE, UE->adjust_rxgain);
    }

    LOG_I(PHY, "Got synch: hw_slot_offset %d, carrier off %d Hz\n", hw_slot_offset, freq_offset);

    UE->is_synchronized = 1;
  } else {
    int gain_change = 0;
    if (get_nrUE_params()->agc)
      gain_change = nrue_ru_adjust_rx_gain(UE, INCREASE_IN_RXGAIN);
    if (gain_change)
      LOG_I(PHY, "synch retry: Rx gain increased \n");
    else
      LOG_E(PHY, "synch Failed: \n");
  }
}

static int nr_ue_slot_select(const fapi_nr_config_request_t *cfg, int nr_slot)
{
  if (cfg->cell_config.frame_duplex_type == FDD)
    return NR_UPLINK_SLOT | NR_DOWNLINK_SLOT;

  const fapi_nr_tdd_table_t *tdd_table = &cfg->tdd_table;
  int rel_slot = nr_slot % tdd_table->tdd_period_in_slots;

  if (tdd_table->max_tdd_periodicity_list == NULL) // this happens before receiving TDD configuration
    return NR_DOWNLINK_SLOT;

  const fapi_nr_max_tdd_periodicity_t *current_slot = &tdd_table->max_tdd_periodicity_list[rel_slot];

  // if the 1st symbol is UL the whole slot is UL
  if (current_slot->max_num_of_symbol_per_slot_list[0].slot_config == 1)
    return NR_UPLINK_SLOT;

  // if the 1st symbol is flexible the whole slot is mixed
  if (current_slot->max_num_of_symbol_per_slot_list[0].slot_config == 2)
    return NR_MIXED_SLOT;

  for (int i = 1; i < NR_SYMBOLS_PER_SLOT; i++) {
    // if the 1st symbol is DL and any other is not, the slot is mixed
    if (current_slot->max_num_of_symbol_per_slot_list[i].slot_config != 0) {
      return NR_MIXED_SLOT;
    }
  }

  // if here, all the symbols where DL
  return NR_DOWNLINK_SLOT;
}

static void RU_write(nr_rxtx_thread_data_t *rxtxD, bool sl_tx_action, c16_t **txp)
{
  int writeBlockSize = rxtxD->writeBlockSize;
  if (writeBlockSize == 0)
    return;

  PHY_VARS_NR_UE *UE = rxtxD->UE;
  const fapi_nr_config_request_t *cfg = &UE->nrUE_config;
  const UE_nr_rxtx_proc_t *proc = &rxtxD->proc;

  const NR_DL_FRAME_PARMS *fp = &UE->frame_parms;
  if (UE->sl_mode == 2)
    fp = &UE->SL_UE_PHY_PARAMS.sl_frame_params;

  int slot = proc->nr_slot_tx;

  radio_tx_burst_flag_t flags = TX_BURST_INVALID;

  if (UE->received_config_request) {
    if (fp->frame_type == FDD || get_softmodem_params()->continuous_tx) {
      flags = TX_BURST_MIDDLE;
    // In case of Sidelink, USRP write needed only in case transmission
    // needs to be done in this slot and not based on tdd ULDL configuration.
    } else if (UE->sl_mode == 2) {
      if (sl_tx_action)
        flags = TX_BURST_START_AND_END;
    } else {
      int slots_frame = fp->slots_per_frame;
      int curr_slot = nr_ue_slot_select(cfg, slot);
      if (curr_slot != NR_DOWNLINK_SLOT) {
        int next_slot = nr_ue_slot_select(cfg, (slot + 1) % slots_frame);
        int prev_slot = nr_ue_slot_select(cfg, (slot + slots_frame - 1) % slots_frame);
        if (prev_slot == NR_DOWNLINK_SLOT)
          flags = TX_BURST_START;
        else if (next_slot == NR_DOWNLINK_SLOT)
          flags = TX_BURST_END;
        else
          flags = TX_BURST_MIDDLE;
      }
    }
  }

  if (!IS_SOFTMODEM_RFSIM) {
    uint64_t deadline_us = rxtxD->absolute_deadline_us;
    struct timespec current_time;
    if (clock_gettime(CLOCK_REALTIME, &current_time)) {
      LOG_E(PHY, "clock_gettime failed\n");
    }
    uint64_t current_time_us = current_time.tv_sec * 1e6 + current_time.tv_nsec / 1e3;
    if (current_time_us > deadline_us) {
      static unsigned int deadline_warning_rate_limit = 0;
      if (deadline_warning_rate_limit % 1000 == 0) {
        LOG_W(PHY,
              "Deadline missed for tx slot %d.%d (current time %lu us, deadline %lu us, missed by %lu)\n",
              proc->frame_tx,
              proc->nr_slot_tx,
              current_time_us,
              deadline_us,
              current_time_us - deadline_us);
      }
      deadline_warning_rate_limit++;
    }
  }

  openair0_timestamp_t writeTimestamp = proc->timestamp_tx;
  // if writeBlockSize gets longer that slot size, fill with dummy
  const int maxWriteBlockSize = get_samples_per_slot(proc->nr_slot_tx, fp);
  while (writeBlockSize > maxWriteBlockSize) {
    const int dummyBlockSize = min(writeBlockSize - maxWriteBlockSize, maxWriteBlockSize);
    int tmp = nrue_ru_write_reorder(UE, writeTimestamp, (void **)txp, dummyBlockSize, fp->nb_antennas_tx, flags);
    AssertFatal(tmp == dummyBlockSize, "write samples to reorder function failed %d", tmp);

    writeTimestamp += dummyBlockSize;
    writeBlockSize -= dummyBlockSize;
  }

  // pre-compensate UL frequency offset
  if (flags != TX_BURST_INVALID && get_nrUE_params()->cont_fo_comp) {
    double ul_freq_offset = -UE->freq_offset * ((double)fp->ul_CarrierFreq / (double)fp->dl_CarrierFreq);
    if (get_nrUE_params()->cont_fo_comp == 2) // different from LO frequency error compensation, Doppler UL pre-compensation has to be negative
      ul_freq_offset = -ul_freq_offset;
    else if (get_nrUE_params()->cont_fo_comp == 3) // do not consider residual DL FO for UL pre-compensation at all
      ul_freq_offset = 0;
    for (int i = 0; i < fp->nb_antennas_tx; i++)
      nr_fo_compensation(UE->ul_Doppler_shift + ul_freq_offset,
                         fp->samples_per_subframe,
                         writeTimestamp,
                         txp[i],
                         txp[i],
                         writeBlockSize);
  }

  int tmp = nrue_ru_write_reorder(UE, writeTimestamp, (void **)txp, writeBlockSize, fp->nb_antennas_tx, flags);
  AssertFatal(tmp == writeBlockSize, "write to reorder function failed %d", tmp);
}

void processSlotTX(void *arg)
{
  TracyCZone(ctx, true);
  nr_rxtx_thread_data_t *rxtxD = arg;
  const UE_nr_rxtx_proc_t *proc = &rxtxD->proc;
  PHY_VARS_NR_UE *UE = rxtxD->UE;
  nr_phy_data_tx_t phy_data = {0};
  bool sl_tx_action = false;

  if (UE->if_inst)
    UE->if_inst->slot_indication(UE->Mod_id, true);

  LOG_D(PHY, "SlotTx %d.%d => slot type %d\n", proc->frame_tx, proc->nr_slot_tx, proc->tx_slot_type);

  const NR_DL_FRAME_PARMS *fp = &UE->frame_parms;
  c16_t *txp[fp->nb_antennas_tx];
  for (int i = 0; i < fp->nb_antennas_tx; i++) {
    txp[i] = UE->common_vars.txData[i] + get_samples_slot_timestamp(fp, proc->nr_slot_tx);
  }

  if (proc->tx_slot_type == NR_UPLINK_SLOT || proc->tx_slot_type == NR_MIXED_SLOT) {
    if (UE->sl_mode == 2 && proc->tx_slot_type == NR_SIDELINK_SLOT) {
      // trigger L2 to run ue_sidelink_scheduler thru IF module
      if (UE->if_inst != NULL && UE->if_inst->sl_indication != NULL) {
        start_meas(&UE->ue_ul_indication_stats);
        nr_sidelink_indication_t sl_indication = {.module_id = UE->Mod_id,
                                                  .gNB_index = proc->gNB_id,
                                                  .cc_id = UE->CC_id,
                                                  .hfn_tx = proc->hfn_tx,
                                                  .frame_tx = proc->frame_tx,
                                                  .slot_tx = proc->nr_slot_tx,
                                                  .hfn_rx = proc->hfn_rx,
                                                  .frame_rx = proc->frame_rx,
                                                  .slot_rx = proc->nr_slot_rx,
                                                  .slot_type = SIDELINK_SLOT_TYPE_TX,
                                                  .phy_data = &phy_data};

        UE->if_inst->sl_indication(&sl_indication);
        stop_meas(&UE->ue_ul_indication_stats);
      }
      dynamic_barrier_join(rxtxD->next_barrier);

      if (phy_data.sl_tx_action) {

        AssertFatal((phy_data.sl_tx_action >= SL_NR_CONFIG_TYPE_TX_PSBCH &&
                     phy_data.sl_tx_action < SL_NR_CONFIG_TYPE_TX_MAXIMUM), "Incorrect SL TX Action Scheduled\n");

        phy_procedures_nrUE_SL_TX(UE, proc, &phy_data, txp);

        sl_tx_action = true;
      }

    } else {
      // trigger L2 to run ue_scheduler thru IF module
      // [TODO] mapping right after NR initial sync
      if (UE->if_inst != NULL && UE->if_inst->ul_indication != NULL) {
        start_meas(&UE->ue_ul_indication_stats);
        nr_uplink_indication_t ul_indication = {.module_id = UE->Mod_id,
                                                .gNB_index = proc->gNB_id,
                                                .cc_id = UE->CC_id,
                                                .frame = proc->frame_tx,
                                                .slot = proc->nr_slot_tx,
                                                .phy_data = &phy_data};

        UE->if_inst->ul_indication(&ul_indication);
        stop_meas(&UE->ue_ul_indication_stats);
      }
      dynamic_barrier_join(rxtxD->next_barrier);

      phy_procedures_nrUE_TX(UE, proc, &phy_data, txp);
    }
  } else {
    dynamic_barrier_join(rxtxD->next_barrier);
  }
  RU_write(rxtxD, sl_tx_action, txp);
  TracyCZoneEnd(ctx);
}

static uint64_t get_carrier_frequency(const int N_RB, const int mu, const uint32_t pointA_freq_khz)
{
  const uint64_t bw = (NR_NB_SC_PER_RB * N_RB) * MU_SCS(mu);
  const uint64_t carrier_freq = (pointA_freq_khz + bw / 2) * 1000;
  return carrier_freq;
}

static int handle_sync_req_from_mac(PHY_VARS_NR_UE *UE)
{
  NR_DL_FRAME_PARMS *fp = &UE->frame_parms;
  // Start synchronization with a target gNB
  if (UE->synch_request.received_synch_request == 1) {
    // if upper layers signal BW scan we do as instructed by command line parameter
    // if upper layers disable BW scan we set it to false
    if (UE->synch_request.synch_req.ssb_bw_scan)
      UE->UE_scan_carrier = get_nrUE_params()->UE_scan_carrier;
    else
      UE->UE_scan_carrier = false;
    UE->target_Nid_cell = UE->synch_request.synch_req.target_Nid_cell;

    const fapi_nr_config_request_t *config = &UE->nrUE_config;
    const fapi_nr_ue_carrier_config_t *cfg = &config->carrier_config;
    uint64_t dl_CarrierFreq = get_carrier_frequency(fp->N_RB_DL, fp->numerology_index, cfg->dl_frequency);
    uint64_t ul_CarrierFreq = get_carrier_frequency(fp->N_RB_UL, fp->numerology_index, cfg->uplink_frequency);
    if (dl_CarrierFreq != fp->dl_CarrierFreq || ul_CarrierFreq != fp->ul_CarrierFreq) {
      LOG_I(NR_PHY,
            "[UE %d] SYNC REQ: RF frequency change: dl %lu->%lu Hz, ul %lu->%lu Hz (from dl_frequency=%u kHz, target_Nid_cell=%d)\n",
            UE->Mod_id,
            fp->dl_CarrierFreq,
            dl_CarrierFreq,
            fp->ul_CarrierFreq,
            ul_CarrierFreq,
            cfg->dl_frequency,
            UE->target_Nid_cell);
      nrue_ru_set_freq(UE, ul_CarrierFreq, dl_CarrierFreq, 0);
      fp->dl_CarrierFreq = dl_CarrierFreq;
      fp->ul_CarrierFreq = ul_CarrierFreq;
      init_symbol_rotation(fp);
    }

    int ssb_start_subcarrier = nr_get_ssb_start_sc(fp->numerology_index,
                                                   config->ssb_table.ssb_offset_point_a,
                                                   config->ssb_table.ssb_subcarrier_offset,
                                                   fp->freq_range);
    // SSB location can change during for ex: handover on the target cell
    if (ssb_start_subcarrier != fp->ssb_start_subcarrier) {
      fp->ssb_start_subcarrier = ssb_start_subcarrier;
      LOG_I(NR_PHY, "SYNC REQ: SSB location changed:%d\n", fp->ssb_start_subcarrier);
    }

    // Apply Doppler based on NTN-Config for target cell
    if (UE->nrUE_config.ntn_config.is_targetcell)
      apply_ntn_timing_advance_and_doppler(UE, fp, -1);
    // Apply NTN DL Doppler as initial FO
    UE->initial_fo = UE->dl_Doppler_shift;

    /* Clearing UE harq while DL actors are active causes race condition.
        So we let the current execution to complete here.*/
    for (int i = 0; i < get_nrUE_params()->num_dl_actors; i++) {
      flush_actor(UE->dl_actors + i);
    }
    for (int i = 0; i < get_nrUE_params()->num_ul_actors; i++) {
      flush_actor(UE->ul_actors + i);
    }

    clean_UE_harq(UE);
    UE->is_synchronized = 0;
    UE->synch_request.received_synch_request = 0;
    return 0;
  }
  return 1;
}

static int UE_dl_preprocessing(PHY_VARS_NR_UE *UE,
                               const UE_nr_rxtx_proc_t *proc,
                               int *tx_wait_for_dlsch,
                               nr_phy_data_t *phy_data,
                               bool *stats_printed)
{
  TracyCZone(ctx, true);
  int sampleShift = INT_MAX;
  const NR_DL_FRAME_PARMS *fp = &UE->frame_parms;
  if (UE->sl_mode == 2)
    fp = &UE->SL_UE_PHY_PARAMS.sl_frame_params;

  // process what RRC thread sent to MAC
  do {
    notifiedFIFO_elt_t *elt = pollNotifiedFIFO(&get_mac_inst(UE->Mod_id)->input_nf);
    if (!elt) {
      break;
    }
    process_msg_rcc_to_mac(NotifiedFifoData(elt), UE->Mod_id);
    delNotifiedFIFO_elt(elt);
  } while (true);

  if (UE->if_inst)
    UE->if_inst->slot_indication(UE->Mod_id, false);

  bool dl_slot = false;
  if (proc->rx_slot_type == NR_DOWNLINK_SLOT || proc->rx_slot_type == NR_MIXED_SLOT) {
    dl_slot = true;
    if(UE->if_inst != NULL && UE->if_inst->dl_indication != NULL) {
      nr_downlink_indication_t dl_indication = (nr_downlink_indication_t){
          .gNB_index = proc->gNB_id,
          .module_id = UE->Mod_id,
          .cc_id = UE->CC_id,
          .hfn = proc->hfn_rx,
          .frame = proc->frame_rx,
          .slot = proc->nr_slot_rx,
          .phy_data = phy_data,
      };
      UE->if_inst->dl_indication(&dl_indication);
    }

    sampleShift = pbch_processing(UE, proc, phy_data);
    pdcch_processing(UE, proc, phy_data);
    if (phy_data->dlsch[0].active
        && (phy_data->dlsch[0].rnti_type == TYPE_C_RNTI_ || phy_data->dlsch[0].rnti_type == TYPE_RA_RNTI_)) {
      // indicate to tx thread to wait for DLSCH decoding
      if (phy_data->dlsch_config.k1_feedback) {  // if feedback is 0 there is no HARQ associated with this DLSCH
        const int ack_nack_slot = (proc->nr_slot_rx + phy_data->dlsch_config.k1_feedback) % fp->slots_per_frame;
        tx_wait_for_dlsch[ack_nack_slot]++;
      }
    }
  }
  if (fp->frame_type == FDD || !dl_slot) {
    // good time to print statistics, we don't have to spend time  to decode DCI
    if (proc->frame_rx % 128 == 0) {
      if (*stats_printed == false) {
        print_ue_mac_stats(UE->Mod_id, proc->frame_rx, proc->nr_slot_rx);
        *stats_printed = true;
      }
    } else {
      *stats_printed = false;
    }
  }

  if (UE->sl_mode == 2) {
    if (proc->rx_slot_type == NR_SIDELINK_SLOT) {
      phy_data->sl_rx_action = 0;
      if (UE->if_inst != NULL && UE->if_inst->sl_indication != NULL) {
        nr_sidelink_indication_t sl_indication;
        nr_fill_sl_indication(&sl_indication, NULL, NULL, proc, UE, phy_data);
        UE->if_inst->sl_indication(&sl_indication);
      }

      if (phy_data->sl_rx_action) {

        AssertFatal((phy_data->sl_rx_action >= SL_NR_CONFIG_TYPE_RX_PSBCH &&
                     phy_data->sl_rx_action < SL_NR_CONFIG_TYPE_RX_MAXIMUM), "Incorrect SL RX Action Scheduled\n");

        sampleShift = psbch_pscch_processing(UE, proc, phy_data);
      }
    }
  } else
    ue_ta_procedures(UE, proc->nr_slot_tx, proc->frame_tx);

  TracyCZoneEnd(ctx);
  return sampleShift;
}

void UE_dl_processing(void *arg) {
  TracyCZone(ctx, true);;
  nr_rxtx_thread_data_t *rxtxD = (nr_rxtx_thread_data_t *) arg;
  UE_nr_rxtx_proc_t *proc = &rxtxD->proc;
  PHY_VARS_NR_UE    *UE   = rxtxD->UE;
  nr_phy_data_t *phy_data = &rxtxD->phy_data;

  if (!UE->sl_mode)
    pdsch_processing(UE, proc, phy_data);

  TracyCZoneEnd(ctx);
}

void dummyWrite(PHY_VARS_NR_UE *UE, openair0_timestamp_t timestamp, int writeBlockSize)
{
  const NR_DL_FRAME_PARMS *fp = &UE->frame_parms;
  if (UE->sl_mode == 2)
    fp = &UE->SL_UE_PHY_PARAMS.sl_frame_params;

  c16_t *dummy_tx[fp->nb_antennas_tx];
  c16_t dummy_tx_data[writeBlockSize];
  memset(dummy_tx_data, 0, sizeof(dummy_tx_data));
  for (int i = 0; i < fp->nb_antennas_tx; i++)
    dummy_tx[i] = dummy_tx_data;

  int tmp = nrue_ru_write(UE, timestamp, (void **)dummy_tx, writeBlockSize, fp->nb_antennas_tx, 4);
  AssertFatal(writeBlockSize == tmp, "write to reorder function failed %d", tmp);
}

void readFrame(PHY_VARS_NR_UE *UE, openair0_timestamp_t *timestamp, int duration_rx_to_tx, bool toTrash)
{
  const NR_DL_FRAME_PARMS *fp = &UE->frame_parms;
  // two frames for initial sync
  int num_frames = 2;
  // In Sidelink worst case SL-SSB can be sent once in 16 frames
  if (UE->sl_mode == 2) {
    fp = &UE->SL_UE_PHY_PARAMS.sl_frame_params;
    num_frames = SL_NR_PSBCH_REPETITION_IN_FRAMES;
  }

  c16_t *rxp[fp->nb_antennas_rx];
  if (toTrash) {
    rxp[0] = malloc16(get_samples_per_slot(0, fp) * sizeof(c16_t));
    for (int i = 1; i < fp->nb_antennas_rx; i++)
      rxp[i] = rxp[0];
  }

  for (int x = 0; x < num_frames * NR_NUMBER_OF_SUBFRAMES_PER_FRAME; x++) { // two frames for initial sync
    for (int slot_rx = 0; slot_rx < fp->slots_per_subframe; slot_rx++) {
      if (!toTrash)
        for (int i = 0; i < fp->nb_antennas_rx; i++)
          rxp[i] = &UE->common_vars.rxdata[i][x * fp->samples_per_subframe + get_samples_slot_timestamp(fp, slot_rx)];

      int readBlockSize = get_samples_per_slot(slot_rx, fp);
      int tmp = nrue_ru_read(UE, timestamp, (void **)rxp, readBlockSize, fp->nb_antennas_rx);
      UEscopeCopy(UE, ueTimeDomainSamplesBeforeSync, rxp[0], sizeof(c16_t), 1, readBlockSize, 0);
      AssertFatal(readBlockSize == tmp, "read rf board failed %d", tmp);

      if (IS_SOFTMODEM_RFSIM) {
        int slot_tx = (slot_rx + duration_rx_to_tx) % fp->slots_per_frame;
        int writeBlockSize = get_samples_per_slot(slot_tx, fp);
        int ta = UE->timing_advance + UE->timing_advance_ntn;
        const openair0_timestamp_t writeTimestamp =
            *timestamp + get_samples_slot_duration(fp, slot_rx, duration_rx_to_tx) - UE->N_TA_offset - ta;
        dummyWrite(UE, writeTimestamp, writeBlockSize);
      }
    }
  }

  if (toTrash)
    free(rxp[0]);
}

static void syncInFrame(PHY_VARS_NR_UE *UE, openair0_timestamp_t *timestamp, int duration_rx_to_tx, openair0_timestamp_t rx_offset)
{
  const NR_DL_FRAME_PARMS *fp = &UE->frame_parms;
  if (UE->sl_mode == 2)
    fp = &UE->SL_UE_PHY_PARAMS.sl_frame_params;

  LOG_I(PHY, "Resynchronizing RX by %ld samples\n", rx_offset);

  int size = rx_offset;
  while (size > 0) {
    // Set a maximum transfer size. As we usually read/write single slots, we use the size of slot 0 as maximum here.
    const int unitTransfer = min(get_samples_per_slot(0, fp), size);
    const int res = nrue_ru_read(UE, timestamp, (void **)UE->common_vars.rxdata, unitTransfer, fp->nb_antennas_rx);
    DevAssert(unitTransfer == res);
    if (IS_SOFTMODEM_RFSIM) {
      int ta = UE->timing_advance + UE->timing_advance_ntn;
      const openair0_timestamp_t writeTimestamp =
          *timestamp + get_samples_slot_duration(fp, 0, duration_rx_to_tx) - UE->N_TA_offset - ta;
      dummyWrite(UE, writeTimestamp, unitTransfer);
    }
    size -= unitTransfer;
  }
}

static inline int get_firstSymSamp(uint16_t slot, const NR_DL_FRAME_PARMS *fp)
{
  return get_samples_symbol_duration(fp, slot, 0, 1);
}

static inline int get_readBlockSize(uint16_t slot, const NR_DL_FRAME_PARMS *fp)
{
  int rem_samples = get_samples_per_slot(slot, fp) - get_firstSymSamp(slot, fp);
  int next_slot_first_symbol = 0;
  if (slot < (fp->slots_per_frame-1))
    next_slot_first_symbol = get_firstSymSamp(slot+1, fp);
  return rem_samples + next_slot_first_symbol;
}

void trs_freq_correction(PHY_VARS_NR_UE *ue, int cfo)
{
  if (abs(cfo) > TRS_CFO_THRESH) {
    LOG_A(PHY, "CFO estimated (%d) from TRS exceeded threshold (%d). Adjusting radio CF\n", cfo, TRS_CFO_THRESH);
    ue->freq_offset += cfo;
    uint64_t dl_carrier;
    uint64_t ul_carrier;
    nr_get_carrier_frequencies(ue, &dl_carrier, &ul_carrier);
    nrue_ru_set_freq(ue, ul_carrier, dl_carrier, ue->freq_offset);
  }
}

void *UE_thread(void *arg)
{
  //this thread should be over the processing thread to keep in real time
  PHY_VARS_NR_UE *UE = (PHY_VARS_NR_UE *)arg;
  const NR_DL_FRAME_PARMS *fp = &UE->frame_parms;
  //  int tx_enabled = 0;
  enum stream_status_e stream_status = STREAM_STATUS_UNSYNC;
  fapi_nr_config_request_t *cfg = &UE->nrUE_config;
  sl_nr_phy_config_request_t *sl_cfg = NULL;
  if (UE->sl_mode == 2) {
    fp = &UE->SL_UE_PHY_PARAMS.sl_frame_params;
    sl_cfg = &UE->SL_UE_PHY_PARAMS.sl_config;
  }

  UE->is_synchronized = 0;
  InitSinLUT();

  notifiedFIFO_t nf;
  initNotifiedFIFO(&nf);

  notifiedFIFO_t freeBlocks;
  initNotifiedFIFO_nothreadSafe(&freeBlocks);

  const double ntn_init_time_drift = get_nrUE_params()->ntn_init_time_drift;
  if (get_nrUE_params()->time_sync_I)
    // ntn_init_time_drift is in µs/s, max_pos_acc * time_sync_I is in samples/frame
    UE->max_pos_acc = ntn_init_time_drift * 1e-6 * fp->samples_per_frame / get_nrUE_params()->time_sync_I;
  else
    UE->max_pos_acc = 0;

  bool ntn_targetcell = false;
  int ntn_koffset = 0;
  int duration_rx_to_tx = NR_UE_CAPABILITY_SLOT_RX_TO_TX;
  int timing_advance = UE->timing_advance + UE->timing_advance_ntn;
  UE->N_TA_offset = determine_N_TA_offset(UE);
  NR_UE_MAC_INST_t *mac = get_mac_inst(UE->Mod_id);

  bool syncRunning = false;
  const int nb_slot_frame = fp->slots_per_frame;
  int absolute_slot = 0, decoded_frame_rx = MAX_FRAME_NUMBER - 1, trashed_frames = 0;
  int tx_wait_for_dlsch[NR_MAX_SLOTS_PER_FRAME];

  for(int i = 0; i < NUM_PROCESS_SLOT_TX_BARRIERS; i++) {
    dynamic_barrier_init(&UE->process_slot_tx_barriers[i]);
  }
  int shiftForNextFrame = 0;
  int intialSyncOffset = 0;
  openair0_timestamp_t sync_timestamp;
  bool stats_printed = false;

  if (get_softmodem_params()->sync_ref && UE->sl_mode == 2) {
    UE->is_synchronized = 1;
  } else {
    //warm up the RF board
    openair0_timestamp_t tmp;
    for (int i = 0; i < 50; i++)
      readFrame(UE, &tmp, duration_rx_to_tx, true);
  }

  while (!oai_exit) {
    if (syncRunning) {
      notifiedFIFO_elt_t *res = pollNotifiedFIFO(&nf);

      if (res) {
        syncRunning = false;
        if (UE->is_synchronized) {
          UE->synch_request.received_synch_request = 0;
          if (UE->sl_mode == SL_MODE2_SUPPORTED)
            decoded_frame_rx = UE->SL_UE_PHY_PARAMS.sync_params.DFN;
          else {
            // We must wait the RRC layer decoded the MIB and sent us the frame number
            notifiedFIFO_elt_t *elt = pullNotifiedFIFO(&mac->input_nf);
            AssertFatal(elt != NULL, "fifo error while waiting for MIB");
            process_msg_rcc_to_mac(NotifiedFifoData(elt), UE->Mod_id);
            delNotifiedFIFO_elt(elt);
            decoded_frame_rx = mac->mib_frame;
          }
          LOG_A(PHY,
                "UE synchronized! decoded_frame_rx=%d UE->init_sync_frame=%d trashed_frames=%d\n",
                decoded_frame_rx,
                UE->init_sync_frame,
                trashed_frames);
          // shift the frame index with all the frames we trashed meanwhile we perform the synch search
          decoded_frame_rx = (decoded_frame_rx + UE->init_sync_frame + trashed_frames) % MAX_FRAME_NUMBER;
          syncData_t *syncMsg = (syncData_t *)NotifiedFifoData(res);
          intialSyncOffset = syncMsg->rx_offset;
        }
        delNotifiedFIFO_elt(res);
        stream_status = STREAM_STATUS_UNSYNC;
      } else {
        if (IS_SOFTMODEM_IQPLAYER || IS_SOFTMODEM_IQRECORDER) {
          /* For IQ recorder-player we force synchronization to happen in a fixed duration so that
             the replay runs in sync with recorded samples.
          */
          openair0_config_t *cfg0 = &openair0_cfg_g[UE->rf_map.card];
          const unsigned int sync_in_frames = cfg0->recplay_conf->u_f_sync;
          while (trashed_frames != sync_in_frames) {
            readFrame(UE, &sync_timestamp, duration_rx_to_tx, true);
            trashed_frames += 2;
          }
        } else {
          readFrame(UE, &sync_timestamp, duration_rx_to_tx, true);
          trashed_frames += ((UE->sl_mode == 2) ? SL_NR_PSBCH_REPETITION_IN_FRAMES : 2);
        }
        continue;
      }
    }

    AssertFatal(!syncRunning, "At this point synchronization can't be running\n");

    if (!UE->is_synchronized) {
      readFrame(UE, &sync_timestamp, duration_rx_to_tx, false);
      notifiedFIFO_elt_t *Msg = newNotifiedFIFO_elt(sizeof(syncData_t), 0, &nf, UE_synch);
      syncData_t *syncMsg = (syncData_t *)NotifiedFifoData(Msg);
      *syncMsg = (syncData_t){0};
      if (UE->UE_scan_carrier) {
        // Get list of GSCN in this band for UE's bandwidth and center frequency.
        LOG_W(PHY, "UE set to scan all GSCN in current bandwidth\n");
        syncMsg->numGscn =
            get_scan_ssb_first_sc(fp->dl_CarrierFreq, fp->N_RB_DL, nrue_get_band(UE), fp->numerology_index, syncMsg->gscnInfo);
      } else {
        LOG_W(PHY, "SSB position provided\n");
        syncMsg->gscnInfo[0] = (nr_gscn_info_t){.ssbFirstSC = fp->ssb_start_subcarrier};
        syncMsg->numGscn = 1;
      }
      syncMsg->UE = UE;
      memset(&syncMsg->proc, 0, sizeof(syncMsg->proc));
      pushNotifiedFIFO(&UE->sync_actor.fifo, Msg);
      trashed_frames = 0;
      syncRunning = true;
      continue;
    }

    if (stream_status == STREAM_STATUS_UNSYNC) {
      stream_status = STREAM_STATUS_SYNCING;
      syncInFrame(UE, &sync_timestamp, duration_rx_to_tx, intialSyncOffset);
      nrue_ru_write_reorder_clear_context(UE);
      shiftForNextFrame = -(UE->init_sync_frame + trashed_frames + 2) * UE->max_pos_acc * get_nrUE_params()->time_sync_I; // compensate for the time drift that happened during initial sync
      LOG_I(PHY, "max_pos_acc = %d, shiftForNextFrame = %d\n", UE->max_pos_acc, shiftForNextFrame);
      // read in first symbol
      int ret = nrue_ru_read(UE,
                             &sync_timestamp,
                             (void **)UE->common_vars.rxdata,
                             fp->ofdm_symbol_size + fp->nb_prefix_samples0,
                             fp->nb_antennas_rx);
      AssertFatal(fp->ofdm_symbol_size + fp->nb_prefix_samples0 == ret, "read rf board failed %d", ret);
      // we have the decoded frame index in the return of the synch process
      // and we shifted above to the first slot of next frame
      decoded_frame_rx = (decoded_frame_rx + 1) % MAX_FRAME_NUMBER;
      const int prev_frame_rx = (absolute_slot / nb_slot_frame) % MAX_FRAME_NUMBER;
      const int prev_hfn_rx = (absolute_slot / nb_slot_frame) / MAX_FRAME_NUMBER;
      int decoded_hfn_rx = prev_hfn_rx;
      if (decoded_frame_rx <= prev_frame_rx)
        decoded_hfn_rx++;
      // we do ++ first in the regular processing, so it will be begin of frame;
      absolute_slot = (decoded_hfn_rx * MAX_FRAME_NUMBER + decoded_frame_rx) * nb_slot_frame - 1;
      if (UE->sl_mode == 2) {
        // Set to the slot where the SL-SSB was decoded
        absolute_slot += UE->SL_UE_PHY_PARAMS.sync_params.slot_offset;
      }
      // With the correct frame and slot numbers, we can now fix the UL timing
      fix_ntn_epoch_hfn(UE, decoded_hfn_rx, decoded_frame_rx);
      if (UE->nrUE_config.ntn_config.params_changed) {
        apply_ntn_config(UE,
                         fp,
                         decoded_hfn_rx,
                         decoded_frame_rx,
                         0,
                         &duration_rx_to_tx,
                         &timing_advance,
                         &ntn_koffset,
                         &ntn_targetcell);
      } else {
        const int abs_subframe_tx = (absolute_slot + 1 + duration_rx_to_tx) / fp->slots_per_subframe;
        apply_ntn_timing_advance_and_doppler(UE, fp, abs_subframe_tx);
        ntn_targetcell = false;
      }
      UE->timing_advance = 0;
      // We have resynchronized, maybe after RF loss so we need to purge any existing context
      memset(tx_wait_for_dlsch, 0, sizeof(tx_wait_for_dlsch));
      for (int i = 0; i < NUM_PROCESS_SLOT_TX_BARRIERS; i++) {
        dynamic_barrier_reset(&UE->process_slot_tx_barriers[i]);
      }
      continue;
    }

    /* check if MAC has sent sync request */
    if (handle_sync_req_from_mac(UE) == 0)
      continue;

    // start of normal case, the UE is in sync
    absolute_slot++;
    TracyCFrameMark;

    // pretend we have 1 iq sample per slot
    // and so nb_slot_frame * 100 iq samples per second (1 frame being 10ms)
    time_manager_iq_samples(1, nb_slot_frame * 100);

    int slot_nr = absolute_slot % nb_slot_frame;
    nr_rxtx_thread_data_t curMsg = {0};
    curMsg.UE=UE;
    // update thread index for received subframe
    curMsg.proc.nr_slot_rx  = slot_nr;
    curMsg.proc.nr_slot_tx  = (absolute_slot + duration_rx_to_tx) % nb_slot_frame;
    curMsg.proc.frame_rx    = (absolute_slot / nb_slot_frame) % MAX_FRAME_NUMBER;
    curMsg.proc.frame_tx    = ((absolute_slot + duration_rx_to_tx) / nb_slot_frame) % MAX_FRAME_NUMBER;
    curMsg.proc.hfn_rx      = (absolute_slot / nb_slot_frame) / MAX_FRAME_NUMBER;
    curMsg.proc.hfn_tx      = ((absolute_slot + duration_rx_to_tx) / nb_slot_frame) / MAX_FRAME_NUMBER;
    if (UE->received_config_request) {
      if (UE->sl_mode) {
        curMsg.proc.rx_slot_type = sl_nr_ue_slot_select(sl_cfg, curMsg.proc.nr_slot_rx, TDD);
        curMsg.proc.tx_slot_type = sl_nr_ue_slot_select(sl_cfg, curMsg.proc.nr_slot_tx, TDD);
      } else {
        curMsg.proc.rx_slot_type = nr_ue_slot_select(cfg, curMsg.proc.nr_slot_rx);
        curMsg.proc.tx_slot_type = nr_ue_slot_select(cfg, curMsg.proc.nr_slot_tx);
      }
    }
    else {
      curMsg.proc.rx_slot_type = NR_DOWNLINK_SLOT;
      curMsg.proc.tx_slot_type = NR_DOWNLINK_SLOT;
    }

    int firstSymSamp = get_firstSymSamp(slot_nr, fp);
    c16_t *rxp[fp->nb_antennas_rx];
    for (int i = 0; i < fp->nb_antennas_rx; i++)
      rxp[i] = &UE->common_vars.rxdata[i][firstSymSamp + get_samples_slot_timestamp(fp, slot_nr)];

    int iq_shift_to_apply = 0;
    if (slot_nr == nb_slot_frame - 1) {
      // we shift of half of measured drift, at each beginning of frame for both rx and tx
      iq_shift_to_apply = shiftForNextFrame;
      // autonomous timing advance calculation, which does not use SIB19 information
      if (ntn_koffset && get_nrUE_params()->autonomous_ta)
        UE->timing_advance_ntn -= 2 * shiftForNextFrame;
      shiftForNextFrame = -round(UE->max_pos_acc * get_nrUE_params()->time_sync_I);
    }

    // Calculate new TA based on SIB19 information for each subframe in NTN mode, if "autonomous_ta" is not enabled
    if (ntn_koffset && !ntn_targetcell && !get_nrUE_params()->autonomous_ta
        && (absolute_slot + duration_rx_to_tx) % fp->slots_per_subframe == 0) {
      const int abs_subframe_tx = (absolute_slot + duration_rx_to_tx) / fp->slots_per_subframe;
      apply_ntn_timing_advance_and_doppler(UE, fp, abs_subframe_tx);
    }

    const int readBlockSize = get_readBlockSize(slot_nr, fp) - iq_shift_to_apply;
    openair0_timestamp_t rx_timestamp;
    int tmp = nrue_ru_read(UE, &rx_timestamp, (void **)rxp, readBlockSize, fp->nb_antennas_rx);
    metadata meta = {.slot =  curMsg.proc.nr_slot_rx, .frame =  curMsg.proc.frame_rx};
    UEscopeCopyWithMetadata(UE, ueTimeDomainSamples, rxp[0] - firstSymSamp, sizeof(c16_t), 1, readBlockSize, 0, &meta);
    AssertFatal(readBlockSize == tmp, "read to rf board failed %d", tmp);
    struct timespec current_time;
    if (clock_gettime(CLOCK_REALTIME, &current_time)) {
      LOG_E(PHY, "clock_gettime failed\n");
    }

    if(slot_nr == (nb_slot_frame - 1)) {
      // read in first symbol of next frame and adjust for timing drift
      int first_symbols = fp->ofdm_symbol_size + fp->nb_prefix_samples0; // first symbol of every frames

      if (first_symbols > 0) {
        openair0_timestamp_t ignore_timestamp;
        int tmp = nrue_ru_read(UE, &ignore_timestamp, (void **)UE->common_vars.rxdata, first_symbols, fp->nb_antennas_rx);
        AssertFatal(first_symbols == tmp, "read to rf board failed %d", tmp);

      } else
        LOG_E(PHY,"can't compensate: diff =%d\n", first_symbols);
    }

    // use previous timing_advance value to compute writeTimestamp
    const openair0_timestamp_t writeTimestamp =
        rx_timestamp + get_samples_slot_duration(fp, slot_nr, duration_rx_to_tx) - firstSymSamp - UE->N_TA_offset - timing_advance;

    // Calculate TX deadline, approximately 1 symbol before the first sample should be written
    const uint64_t samples_diff = writeTimestamp - rx_timestamp - fp->ofdm_symbol_size;
    const float deadline_us = samples_diff * 1e3 / fp->samples_per_subframe;
    const uint64_t absolute_deadline_us = current_time.tv_sec * 1e6 + current_time.tv_nsec * 1e-3 + deadline_us;

    // but use current UE->timing_advance value to compute writeBlockSize
    int writeBlockSize = get_samples_per_slot((slot_nr + duration_rx_to_tx) % nb_slot_frame, fp) - iq_shift_to_apply;
    int new_timing_advance = UE->timing_advance + UE->timing_advance_ntn;
    if (new_timing_advance != timing_advance) {
      writeBlockSize -= new_timing_advance - timing_advance;
      timing_advance = new_timing_advance;
    }
    int new_N_TA_offset = determine_N_TA_offset(UE);
    if (new_N_TA_offset != UE->N_TA_offset) {
      LOG_I(PHY, "N_TA_offset changed from %d to %d\n", UE->N_TA_offset, new_N_TA_offset);
      writeBlockSize -= new_N_TA_offset - UE->N_TA_offset;
      UE->N_TA_offset = new_N_TA_offset;
    }
    if (writeBlockSize < 0) {
      timing_advance += writeBlockSize;
      LOG_I(PHY, "writeBlockSize is %d, setting it to 0 and changing timing_advance to %d\n", writeBlockSize, timing_advance);
      writeBlockSize = 0;
    }

    if (curMsg.proc.nr_slot_rx == 0)
      nr_ue_rrc_timer_trigger(UE->Mod_id, curMsg.proc.hfn_rx, curMsg.proc.frame_rx, curMsg.proc.gNB_id);

    // RX slot processing. We launch and forget.
    notifiedFIFO_elt_t *newRx = newNotifiedFIFO_elt(sizeof(nr_rxtx_thread_data_t), curMsg.proc.nr_slot_tx, NULL, UE_dl_processing);
    nr_rxtx_thread_data_t *curMsgRx = (nr_rxtx_thread_data_t *)NotifiedFifoData(newRx);
    *curMsgRx = (nr_rxtx_thread_data_t){.proc = curMsg.proc, .UE = UE};
    int ret = UE_dl_preprocessing(UE, &curMsgRx->proc, tx_wait_for_dlsch, &curMsgRx->phy_data, &stats_printed);
    if (ret != INT_MAX)
      shiftForNextFrame = ret;
    if (get_nrUE_params()->num_dl_actors > 0) {
      pushNotifiedFIFO(&UE->dl_actors[curMsg.proc.nr_slot_rx % get_nrUE_params()->num_dl_actors].fifo, newRx);
    } else {
      newRx->processingFunc(curMsgRx);
    }

    // apply new NTN timing information
    apply_ntn_config(UE,
                     fp,
                     curMsg.proc.hfn_rx,
                     curMsg.proc.frame_rx,
                     curMsg.proc.nr_slot_rx,
                     &duration_rx_to_tx,
                     &timing_advance,
                     &ntn_koffset,
                     &ntn_targetcell);

    // Start TX slot processing here. It runs in parallel with RX slot processing
    // in current code, DURATION_RX_TO_TX constant is the limit to get UL data to encode from a RX slot
    notifiedFIFO_elt_t *newTx = newNotifiedFIFO_elt(sizeof(nr_rxtx_thread_data_t), 0, 0, processSlotTX);
    nr_rxtx_thread_data_t *curMsgTx = NotifiedFifoData(newTx);
    memset(curMsgTx, 0, sizeof(*curMsgTx));
    curMsgTx->proc = curMsg.proc;
    curMsgTx->writeBlockSize = writeBlockSize;
    curMsgTx->proc.timestamp_tx = writeTimestamp;
    curMsgTx->UE = UE;
    curMsgTx->absolute_deadline_us = absolute_deadline_us;

    int slot = curMsgTx->proc.nr_slot_tx;
    int slot_and_frame = slot + curMsgTx->proc.frame_tx * nb_slot_frame;
    int next_tx_slot_and_frame = absolute_slot + duration_rx_to_tx + 1;
    int wait_for_prev_slot = stream_status == STREAM_STATUS_SYNCED ? 1 : 0;

    dynamic_barrier_t *next_barrier = &UE->process_slot_tx_barriers[next_tx_slot_and_frame % NUM_PROCESS_SLOT_TX_BARRIERS];
    curMsgTx->next_barrier = next_barrier;
    dynamic_barrier_update(&UE->process_slot_tx_barriers[slot_and_frame % NUM_PROCESS_SLOT_TX_BARRIERS],
                           tx_wait_for_dlsch[slot] + wait_for_prev_slot,
                           start_process_slot_tx,
                           newTx);
    stream_status = STREAM_STATUS_SYNCED;
    tx_wait_for_dlsch[slot] = 0;
  }
  LOG_W(NR_PHY, "UE main thread is ending\n");
  return NULL;
}

void init_NR_UE(int nb_inst, char *uecap_file, char *reconfig_file, char *rbconfig_file, int numerology)
{
  for (int instance_id = 0; instance_id < nb_inst; instance_id++) {
    NR_UE_RRC_INST_t* rrc = nr_rrc_init_ue(uecap_file, instance_id, get_nrUE_params()->nb_antennas_tx);
    NR_UE_MAC_INST_t *mac = nr_l2_init_ue(instance_id, numerology);

    nr_rrc_set_mac_queue(instance_id, &mac->input_nf);
    mac->if_module = nr_ue_if_module_init(instance_id);
    AssertFatal(mac->if_module, "can not initialize IF module\n");
    if (!IS_SA_MODE(get_softmodem_params()) && !get_softmodem_params()->sl_mode) {
      init_nsa_message(rrc, reconfig_file, rbconfig_file);
      nr_rlc_activate_srb0(mac->crnti, NULL, send_srb0_rrc);
    }
    //TODO: Move this call to RRC
    start_sidelink(instance_id);
  }
}

void init_NR_UE_threads(PHY_VARS_NR_UE *UE) {
  char thread_name[16];
  sprintf(thread_name, "UEthread_%d", UE->Mod_id);
  threadCreate(&UE->main_thread, UE_thread, (void *)UE, thread_name, -1, OAI_PRIORITY_RT_MAX);
  if (!IS_SOFTMODEM_NOSTATS) {
    sprintf(thread_name, "L1_UE_stats_%d", UE->Mod_id);
    threadCreate(&UE->stat_thread, nrL1_UE_stats_thread, UE, thread_name, -1, OAI_PRIORITY_RT_LOW);
  }
}
