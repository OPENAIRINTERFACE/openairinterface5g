/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

#define _GNU_SOURCE // For pthread_setname_np
#include <pthread.h>
#include <openair1/PHY/impl_defs_top.h>
#include "executables/nr-uesoftmodem.h"
#include "PHY/phy_extern_nr_ue.h"
#include "PHY/INIT/nr_phy_init.h"
#include "NR_MAC_UE/mac_proto.h"
#include "RRC/NR_UE/rrc_proto.h"
#include "SCHED_NR_UE/phy_frame_config_nr.h"
#include "SCHED_NR_UE/defs.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "executables/softmodem-common.h"
#include "PHY/NR_REFSIG/refsig_defs_ue.h"
#include "radio/COMMON/common_lib.h"
#include "LAYER2/nr_pdcp/nr_pdcp_oai_api.h"
#include "LAYER2/nr_rlc/nr_rlc_oai_api.h"
#include "RRC/NR/MESSAGES/asn1_msg.h"

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


#define RX_JOB_ID 0x1010
#define TX_JOB_ID 100

typedef enum {
  pss = 0,
  pbch = 1,
  si = 2
} sync_mode_t;

static void *NRUE_phy_stub_standalone_pnf_task(void *arg);

static size_t dump_L1_UE_meas_stats(PHY_VARS_NR_UE *ue, char *output, size_t max_len)
{
  const char *begin = output;
  const char *end = output + max_len;
  output += print_meas_log(&ue->phy_proc_tx, "L1 TX processing", NULL, NULL, output, end - output);
  output += print_meas_log(&ue->ulsch_encoding_stats, "ULSCH encoding", NULL, NULL, output, end - output);
  output += print_meas_log(&ue->phy_proc_rx, "L1 RX processing", NULL, NULL, output, end - output);
  output += print_meas_log(&ue->ue_ul_indication_stats, "UL Indication", NULL, NULL, output, end - output);
  output += print_meas_log(&ue->rx_pdsch_stats, "PDSCH receiver", NULL, NULL, output, end - output);
  output += print_meas_log(&ue->dlsch_decoding_stats, "PDSCH decoding", NULL, NULL, output, end - output);
  output += print_meas_log(&ue->dlsch_deinterleaving_stats, " -> Deinterleive", NULL, NULL, output, end - output);
  output += print_meas_log(&ue->dlsch_rate_unmatching_stats, " -> Rate Unmatch", NULL, NULL, output, end - output);
  output += print_meas_log(&ue->dlsch_ldpc_decoding_stats, " ->  LDPC Decode", NULL, NULL, output, end - output);
  output += print_meas_log(&ue->dlsch_unscrambling_stats, "PDSCH unscrambling", NULL, NULL, output, end - output);
  output += print_meas_log(&ue->dlsch_rx_pdcch_stats, "PDCCH handling", NULL, NULL, output, end - output);
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

void init_nr_ue_vars(PHY_VARS_NR_UE *ue,
                     uint8_t UE_id,
                     uint8_t abstraction_flag)
{

  int nb_connected_gNB = 1;

  ue->Mod_id      = UE_id;
  ue->if_inst     = nr_ue_if_module_init(0);
  ue->dci_thres   = 0;
  ue->target_Nid_cell = -1;

  // initialize all signal buffers
  init_nr_ue_signal(ue, nb_connected_gNB);

  // intialize transport
  init_nr_ue_transport(ue);

  // init N_TA offset
  init_N_TA_offset(ue);
}

void init_nrUE_standalone_thread(int ue_idx)
{
  int standalone_tx_port = 3611 + ue_idx * 2;
  int standalone_rx_port = 3612 + ue_idx * 2;
  nrue_init_standalone_socket(standalone_tx_port, standalone_rx_port);

  NR_UE_MAC_INST_t *mac = get_mac_inst(0);
  pthread_mutex_init(&mac->mutex_dl_info, NULL);

  pthread_t thread;
  if (pthread_create(&thread, NULL, nrue_standalone_pnf_task, NULL) != 0) {
    LOG_E(NR_MAC, "pthread_create failed for calling nrue_standalone_pnf_task");
  }
  pthread_setname_np(thread, "oai:nrue-stand");
  pthread_t phy_thread;
  if (pthread_create(&phy_thread, NULL, NRUE_phy_stub_standalone_pnf_task, NULL) != 0) {
    LOG_E(NR_MAC, "pthread_create failed for calling NRUE_phy_stub_standalone_pnf_task");
  }
  pthread_setname_np(phy_thread, "oai:nrue-stand-phy");
}

static void process_queued_nr_nfapi_msgs(NR_UE_MAC_INST_t *mac, int sfn_slot)
{
  nfapi_nr_rach_indication_t *rach_ind = unqueue_matching(&nr_rach_ind_queue, MAX_QUEUE_SIZE, sfn_slot_matcher, &sfn_slot);
  nfapi_nr_dl_tti_request_t *dl_tti_request = get_queue(&nr_dl_tti_req_queue);
  nfapi_nr_ul_dci_request_t *ul_dci_request = get_queue(&nr_ul_dci_req_queue);

  for (int i = 0; i < NR_MAX_HARQ_PROCESSES; i++) {
    LOG_D(NR_MAC, "Try to get a ul_tti_req by matching CRC active SFN %d/SLOT %d from queue with %lu items\n",
            NFAPI_SFNSLOT2SFN(mac->nr_ue_emul_l1.harq[i].active_ul_harq_sfn_slot),
            NFAPI_SFNSLOT2SLOT(mac->nr_ue_emul_l1.harq[i].active_ul_harq_sfn_slot), nr_ul_tti_req_queue.num_items);
    nfapi_nr_ul_tti_request_t *ul_tti_request_crc = unqueue_matching(&nr_ul_tti_req_queue, MAX_QUEUE_SIZE, sfn_slot_matcher, &mac->nr_ue_emul_l1.harq[i].active_ul_harq_sfn_slot);
    if (ul_tti_request_crc && ul_tti_request_crc->n_pdus > 0) {
      check_and_process_dci(NULL, NULL, NULL, ul_tti_request_crc);
      free_and_zero(ul_tti_request_crc);
    }
  }

  if (rach_ind && rach_ind->number_of_pdus > 0) {
      NR_UL_IND_t UL_INFO = {
        .rach_ind = *rach_ind,
      };
      send_nsa_standalone_msg(&UL_INFO, rach_ind->header.message_id);
      for (int i = 0; i < rach_ind->number_of_pdus; i++)
      {
        free_and_zero(rach_ind->pdu_list[i].preamble_list);
      }
      free_and_zero(rach_ind->pdu_list);
      free_and_zero(rach_ind);
  }
  if (dl_tti_request) {
    int dl_tti_sfn_slot = NFAPI_SFNSLOT2HEX(dl_tti_request->SFN, dl_tti_request->Slot);
    nfapi_nr_tx_data_request_t *tx_data_request = unqueue_matching(&nr_tx_req_queue, MAX_QUEUE_SIZE, sfn_slot_matcher, &dl_tti_sfn_slot);
    if (!tx_data_request) {
      LOG_E(NR_MAC, "[%d %d] No corresponding tx_data_request for given dl_tti_request sfn/slot\n",
            NFAPI_SFNSLOT2SFN(dl_tti_sfn_slot), NFAPI_SFNSLOT2SLOT(dl_tti_sfn_slot));
      if (get_softmodem_params()->nsa)
        save_nr_measurement_info(dl_tti_request);
      free_and_zero(dl_tti_request);
    }
    else if (dl_tti_request->dl_tti_request_body.nPDUs > 0 && tx_data_request->Number_of_PDUs > 0) {
      if (get_softmodem_params()->nsa)
        save_nr_measurement_info(dl_tti_request);
      check_and_process_dci(dl_tti_request, tx_data_request, NULL, NULL);
      free_and_zero(dl_tti_request);
      free_and_zero(tx_data_request);
    }
    else {
      AssertFatal(false, "We dont have PDUs in either dl_tti %d or tx_req %d\n",
                  dl_tti_request->dl_tti_request_body.nPDUs, tx_data_request->Number_of_PDUs);
    }
  }
  if (ul_dci_request && ul_dci_request->numPdus > 0) {
    check_and_process_dci(NULL, NULL, ul_dci_request, NULL);
    free_and_zero(ul_dci_request);
  }
}

static void *NRUE_phy_stub_standalone_pnf_task(void *arg)
{
  LOG_I(MAC, "Clearing Queues\n");
  reset_queue(&nr_rach_ind_queue);
  reset_queue(&nr_rx_ind_queue);
  reset_queue(&nr_crc_ind_queue);
  reset_queue(&nr_uci_ind_queue);
  reset_queue(&nr_dl_tti_req_queue);
  reset_queue(&nr_tx_req_queue);
  reset_queue(&nr_ul_dci_req_queue);
  reset_queue(&nr_ul_tti_req_queue);

  int last_sfn_slot = -1;
  uint16_t sfn_slot = 0;

  module_id_t mod_id = 0;
  NR_UE_MAC_INST_t *mac = get_mac_inst(mod_id);
  for (int i = 0; i < NR_MAX_HARQ_PROCESSES; i++) {
      mac->nr_ue_emul_l1.harq[i].active = false;
      mac->nr_ue_emul_l1.harq[i].active_ul_harq_sfn_slot = -1;
  }

  while (!oai_exit) {
    if (sem_wait(&sfn_slot_semaphore) != 0) {
      LOG_E(NR_MAC, "sem_wait() error\n");
      abort();
    }
    uint16_t *slot_ind = get_queue(&nr_sfn_slot_queue);
    nr_phy_channel_params_t *ch_info = get_queue(&nr_chan_param_queue);
    if (!slot_ind && !ch_info) {
      LOG_D(MAC, "get nr_sfn_slot_queue and nr_chan_param_queue == NULL!\n");
      continue;
    }
    if (slot_ind) {
      sfn_slot = *slot_ind;
      free_and_zero(slot_ind);
    }
    else if (ch_info) {
      sfn_slot = ch_info->sfn_slot;
      free_and_zero(ch_info);
    }

    frame_t frame = NFAPI_SFNSLOT2SFN(sfn_slot);
    int slot = NFAPI_SFNSLOT2SLOT(sfn_slot);
    if (sfn_slot == last_sfn_slot) {
      LOG_D(NR_MAC, "repeated sfn_sf = %d.%d\n",
            frame, slot);
      continue;
    }
    last_sfn_slot = sfn_slot;

    LOG_D(NR_MAC, "The received sfn/slot [%d %d] from proxy\n",
          frame, slot);

    if (get_softmodem_params()->sa && mac->mib == NULL) {
      LOG_D(NR_MAC, "We haven't gotten MIB. Lets see if we received it\n");
      nr_ue_dl_indication(&mac->dl_info);
      process_queued_nr_nfapi_msgs(mac, sfn_slot);
    }

    int CC_id = 0;
    uint8_t gNB_id = 0;
    int slots_per_frame = 20; //30 kHZ subcarrier spacing
    int slot_ahead = 2; // TODO: Make this dynamic
    nr_uplink_indication_t ul_info = {.cc_id = CC_id,
                                      .gNB_index = gNB_id,
                                      .module_id = mod_id,
                                      .slot = (slot + slot_ahead) % slots_per_frame,
                                      .frame = (slot + slot_ahead >= slots_per_frame) ? (frame + 1) % 1024 : frame};

    if (pthread_mutex_lock(&mac->mutex_dl_info)) abort();

    if (ch_info) {
      mac->nr_ue_emul_l1.pmi = ch_info->csi[0].pmi;
      mac->nr_ue_emul_l1.ri = ch_info->csi[0].ri;
      mac->nr_ue_emul_l1.cqi = ch_info->csi[0].cqi;
      free_and_zero(ch_info);
    }

    if (is_nr_DL_slot(mac->tdd_UL_DL_ConfigurationCommon, slot)) {
      memset(&mac->dl_info, 0, sizeof(mac->dl_info));
      mac->dl_info.cc_id = CC_id;
      mac->dl_info.gNB_index = gNB_id;
      mac->dl_info.module_id = mod_id;
      mac->dl_info.frame = frame;
      mac->dl_info.slot = slot;
      mac->dl_info.dci_ind = NULL;
      mac->dl_info.rx_ind = NULL;
      nr_ue_dl_indication(&mac->dl_info);
    }

    if (pthread_mutex_unlock(&mac->mutex_dl_info)) abort();

    if (is_nr_UL_slot(mac->tdd_UL_DL_ConfigurationCommon, ul_info.slot, mac->frame_type)) {
      LOG_D(NR_MAC, "Slot %d. calling nr_ue_ul_ind()\n", ul_info.slot);
      nr_ue_ul_scheduler(mac, &ul_info);
    }
    process_queued_nr_nfapi_msgs(mac, sfn_slot);
  }
  return NULL;
}


/*!
 * It performs band scanning and synchonization.
 * \param arg is a pointer to a \ref PHY_VARS_NR_UE structure.
 */

typedef nr_rxtx_thread_data_t syncData_t;

static void UE_synch(void *arg) {
  syncData_t *syncD=(syncData_t *) arg;
  int i, hw_slot_offset;
  PHY_VARS_NR_UE *UE = syncD->UE;
  sync_mode_t sync_mode = pbch;
  //int CC_id = UE->CC_id;
  static int freq_offset = 0;
  UE->is_synchronized = 0;

  if (UE->UE_scan == 0) {

    for (i=0; i<openair0_cfg[UE->rf_map.card].rx_num_channels; i++) {

      LOG_I( PHY, "[SCHED][UE] Check absolute frequency DL %f, UL %f (RF card %d, oai_exit %d, channel %d, rx_num_channels %d)\n",
        openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i],
        openair0_cfg[UE->rf_map.card].tx_freq[UE->rf_map.chain+i],
        UE->rf_map.card,
        oai_exit,
        i,
        openair0_cfg[0].rx_num_channels);

    }

    sync_mode = pbch;
  } else {
    LOG_E(PHY,"Fixme!\n");
    /*
    for (i=0; i<openair0_cfg[UE->rf_map.card].rx_num_channels; i++) {
      downlink_frequency[UE->rf_map.card][UE->rf_map.chain+i] = bands_to_scan.band_info[CC_id].dl_min;
      uplink_frequency_offset[UE->rf_map.card][UE->rf_map.chain+i] =
        bands_to_scan.band_info[CC_id].ul_min-bands_to_scan.band_info[CC_id].dl_min;
      openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i];
      openair0_cfg[UE->rf_map.card].tx_freq[UE->rf_map.chain+i] =
        downlink_frequency[CC_id][i]+uplink_frequency_offset[CC_id][i];
      openair0_cfg[UE->rf_map.card].rx_gain[UE->rf_map.chain+i] = UE->rx_total_gain_dB;
    }
    */
  }

  if (UE->target_Nid_cell != -1) {
    LOG_W(NR_PHY, "Starting re-sync detection for target Nid_cell %i\n", UE->target_Nid_cell);
  } else {
    LOG_W(NR_PHY, "Starting sync detection\n");
  }

  switch (sync_mode) {
    /*
    case pss:
      LOG_I(PHY,"[SCHED][UE] Scanning band %d (%d), freq %u\n",bands_to_scan.band_info[current_band].band, current_band,bands_to_scan.band_info[current_band].dl_min+current_offset);
      //lte_sync_timefreq(UE,current_band,bands_to_scan.band_info[current_band].dl_min+current_offset);
      current_offset += 20000000; // increase by 20 MHz

      if (current_offset > bands_to_scan.band_info[current_band].dl_max-bands_to_scan.band_info[current_band].dl_min) {
        current_band++;
        current_offset=0;
      }

      if (current_band==bands_to_scan.nbands) {
        current_band=0;
        oai_exit=1;
      }

      for (i=0; i<openair0_cfg[UE->rf_map.card].rx_num_channels; i++) {
        downlink_frequency[UE->rf_map.card][UE->rf_map.chain+i] = bands_to_scan.band_info[current_band].dl_min+current_offset;
        uplink_frequency_offset[UE->rf_map.card][UE->rf_map.chain+i] = bands_to_scan.band_info[current_band].ul_min-bands_to_scan.band_info[0].dl_min + current_offset;
        openair0_cfg[UE->rf_map.card].rx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i];
        openair0_cfg[UE->rf_map.card].tx_freq[UE->rf_map.chain+i] = downlink_frequency[CC_id][i]+uplink_frequency_offset[CC_id][i];
        openair0_cfg[UE->rf_map.card].rx_gain[UE->rf_map.chain+i] = UE->rx_total_gain_dB;

        if (UE->UE_scan_carrier) {
          openair0_cfg[UE->rf_map.card].autocal[UE->rf_map.chain+i] = 1;
        }
      }

      break;
    */
    case pbch:
      LOG_I(PHY, "[UE thread Synch] Running Initial Synch \n");

      uint64_t dl_carrier, ul_carrier;
      nr_get_carrier_frequencies(UE, &dl_carrier, &ul_carrier);

      if (nr_initial_sync(&syncD->proc, UE, 2, get_softmodem_params()->sa) == 0) {
        freq_offset = UE->common_vars.freq_offset; // frequency offset computed with pss in initial sync
        hw_slot_offset = ((UE->rx_offset<<1) / UE->frame_parms.samples_per_subframe * UE->frame_parms.slots_per_subframe) +
                         round((float)((UE->rx_offset<<1) % UE->frame_parms.samples_per_subframe)/UE->frame_parms.samples_per_slot0);

        // rerun with new cell parameters and frequency-offset
        // todo: the freq_offset computed on DL shall be scaled before being applied to UL
        nr_rf_card_config_freq(&openair0_cfg[UE->rf_map.card], ul_carrier, dl_carrier, freq_offset);

        LOG_I(PHY,"Got synch: hw_slot_offset %d, carrier off %d Hz, rxgain %f (DL %f Hz, UL %f Hz)\n",
              hw_slot_offset,
              freq_offset,
              openair0_cfg[UE->rf_map.card].rx_gain[0],
              openair0_cfg[UE->rf_map.card].rx_freq[0],
              openair0_cfg[UE->rf_map.card].tx_freq[0]);

        UE->rfdevice.trx_set_freq_func(&UE->rfdevice,&openair0_cfg[0]);
        if (UE->UE_scan_carrier == 1)
          UE->UE_scan_carrier = 0;
        else
          UE->is_synchronized = 1;
      } else {

        if (UE->UE_scan_carrier == 1) {

          if (freq_offset >= 0)
            freq_offset += 100;

          freq_offset *= -1;

          nr_rf_card_config_freq(&openair0_cfg[UE->rf_map.card], ul_carrier, dl_carrier, freq_offset);

          LOG_I(PHY, "Initial sync failed: trying carrier off %d Hz\n", freq_offset);

          UE->rfdevice.trx_set_freq_func(&UE->rfdevice,&openair0_cfg[0]);
        }
      }
      break;

    case si:
    default:
      break;

  }
}

static void RU_write(nr_rxtx_thread_data_t *rxtxD) {

  PHY_VARS_NR_UE *UE = rxtxD->UE;
  const UE_nr_rxtx_proc_t *proc = &rxtxD->proc;

  void *txp[NB_ANTENNAS_TX];
  int slot = proc->nr_slot_tx;
  for (int i = 0; i < UE->frame_parms.nb_antennas_tx; i++)
    txp[i] = (void *)&UE->common_vars.txData[i][UE->frame_parms.get_samples_slot_timestamp(slot, &UE->frame_parms, 0)];

  radio_tx_burst_flag_t flags = TX_BURST_INVALID;

  NR_UE_MAC_INST_t *mac = get_mac_inst(0);
  if (mac->phy_config_request_sent &&
      openair0_cfg[0].duplex_mode == duplex_mode_TDD &&
      !get_softmodem_params()->continuous_tx) {

    int slots_frame = UE->frame_parms.slots_per_frame;
    int curr_slot = nr_ue_slot_select(&UE->nrUE_config, slot);
    if (curr_slot != NR_DOWNLINK_SLOT) {
      int next_slot = nr_ue_slot_select(&UE->nrUE_config, (slot + 1) % slots_frame);
      int prev_slot = nr_ue_slot_select(&UE->nrUE_config, (slot + slots_frame - 1) % slots_frame);
      if (prev_slot == NR_DOWNLINK_SLOT)
        flags = TX_BURST_START;
      else if (next_slot == NR_DOWNLINK_SLOT)
        flags = TX_BURST_END;
      else
        flags = TX_BURST_MIDDLE;
    }
  } else {
    flags = TX_BURST_MIDDLE;
  }

  if (flags || IS_SOFTMODEM_RFSIM)
    AssertFatal(rxtxD->writeBlockSize ==
                UE->rfdevice.trx_write_func(&UE->rfdevice,
                                            proc->timestamp_tx,
                                            txp,
                                            rxtxD->writeBlockSize,
                                            UE->frame_parms.nb_antennas_tx,
                                            flags),"");

  for (int i=0; i<UE->frame_parms.nb_antennas_tx; i++)
    memset(txp[i], 0, rxtxD->writeBlockSize);

}

void processSlotTX(void *arg) {

  nr_rxtx_thread_data_t *rxtxD = (nr_rxtx_thread_data_t *) arg;
  const UE_nr_rxtx_proc_t *proc = &rxtxD->proc;
  PHY_VARS_NR_UE    *UE   = rxtxD->UE;
  nr_phy_data_tx_t phy_data = {0};

  LOG_D(PHY,
        "SlotTx %d.%d => slot type %d, wait: %d \n",
        proc->frame_tx,
        proc->nr_slot_tx,
        proc->tx_slot_type,
        rxtxD->tx_wait_for_dlsch);
  if (proc->tx_slot_type == NR_UPLINK_SLOT || proc->tx_slot_type == NR_MIXED_SLOT){
    if (rxtxD->tx_wait_for_dlsch)
      LOG_D(PHY, "enter wait for tx, slot %d, nb events to wait %d; ", proc->nr_slot_tx, rxtxD->tx_wait_for_dlsch);
    // wait for rx slots to send indication (if any) that DLSCH decoding is finished
    for(int i=0; i < rxtxD->tx_wait_for_dlsch; i++) {
      notifiedFIFO_elt_t *res = pullNotifiedFIFO(UE->tx_resume_ind_fifo + proc->nr_slot_tx);
      delNotifiedFIFO_elt(res);
    }
    LOG_D(PHY, "completed wait for tx, slot %d\n", proc->nr_slot_tx);

    /*
      This herafter code is costing some perfomance for a check that should be useless
      But, we face today several bugs arround the matching between events in UE->tx_resume_ind_fifo[slot]
      and the corresponding tx_wait_for_dlsch[slot]
      The algorithm is we accumlate the actions that should end before processing a tx slot in tx_wait_for_dlsch[slot]
      later, other threads push events in UE->tx_resume_ind_fifo[slot]
      so, the tx encoding starts only when related actions are done (mainly DLSCH ACK/NACK to encode PUCCH)
      if there is a bug that misses to send a event in UE->tx_resume_ind_fifo[slot], the process hangs, we detect the issue
      if there is a bug that makes a extra event in UE->tx_resume_ind_fifo[slot], and if we drop the hereafter check
      the system runs with random race conditions, very hard to debug

      Likely we should later remove completly UE->tx_resume_ind_fifo with notifications,
      instead,
      we may run in place the processSlotTX() when the conditions are met (when a decreasing tx_wait_for_dlsch[slot] will become 0)
      It will remove the condition signals (for a thread safe semaphore or counter) and make the system simpler
      This require also other modifications to
          remove txFifo that is also a big issue
	  add out of order RF board sending, because,
	    if we encode and send tx slot as soon as we can,
	    it will be thrown out of order, especially in TDD mode
    */
    notifiedFIFO_elt_t *res = pollNotifiedFIFO(UE->tx_resume_ind_fifo + proc->nr_slot_tx);
    if (res)
      LOG_E(NR_PHY,
            "Internal error: extra event on Tx waiting queue for slot %d, event comes from rx slot %d\n",
            proc->nr_slot_tx,
            *(int *)NotifiedFifoData(res));
    // trigger L2 to run ue_scheduler thru IF module
    // [TODO] mapping right after NR initial sync
    if(UE->if_inst != NULL && UE->if_inst->ul_indication != NULL) {
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

    phy_procedures_nrUE_TX(UE, proc, &phy_data);
  }

  RU_write(rxtxD);
}

static void UE_dl_preprocessing(PHY_VARS_NR_UE *UE, const UE_nr_rxtx_proc_t *proc, int *tx_wait_for_dlsch, nr_phy_data_t *phy_data)
{
  if (IS_SOFTMODEM_NOS1 || get_softmodem_params()->sa) {

    // Start synchronization with a target gNB
    if (UE->synch_request.received_synch_request == 1) {
      UE->is_synchronized = 0;
      UE->target_Nid_cell = UE->synch_request.synch_req.target_Nid_cell;
      clean_UE_harq(UE);
      UE->synch_request.received_synch_request = 0;
    }

    /* send tick to RLC and PDCP every ms */
    if (proc->nr_slot_rx % UE->frame_parms.slots_per_subframe == 0) {
      void nr_rlc_tick(int frame, int subframe);
      void nr_pdcp_tick(int frame, int subframe);
      nr_rlc_tick(proc->frame_rx, proc->nr_slot_rx / UE->frame_parms.slots_per_subframe);
      nr_pdcp_tick(proc->frame_rx, proc->nr_slot_rx / UE->frame_parms.slots_per_subframe);
    }
  }

  if (proc->rx_slot_type == NR_DOWNLINK_SLOT || proc->rx_slot_type == NR_MIXED_SLOT) {

    if(UE->if_inst != NULL && UE->if_inst->dl_indication != NULL) {
      nr_downlink_indication_t dl_indication;
      nr_fill_dl_indication(&dl_indication, NULL, NULL, proc, UE, phy_data);
      UE->if_inst->dl_indication(&dl_indication);
    }

    uint64_t a=rdtsc_oai();
    pbch_pdcch_processing(UE, proc, phy_data);
    if (phy_data->dlsch[0].active && phy_data->dlsch[0].rnti_type == TYPE_C_RNTI_) {
      // indicate to tx thread to wait for DLSCH decoding
      const int ack_nack_slot = (proc->nr_slot_rx + phy_data->dlsch[0].dlsch_config.k1_feedback) % UE->frame_parms.slots_per_frame;
      LOG_D(NR_PHY, "Adding one event to wait after decoding slot %d, for futre tx slot %d\n", proc->nr_slot_rx, ack_nack_slot);
      tx_wait_for_dlsch[ack_nack_slot]++;
    }

    LOG_D(PHY, "In %s: slot %d, time %llu\n", __FUNCTION__, proc->nr_slot_rx, (rdtsc_oai()-a)/3500);
  }

  ue_ta_procedures(UE, proc->nr_slot_tx, proc->frame_tx);
  return;
}

void UE_dl_processing(void *arg) {
  nr_rxtx_thread_data_t *rxtxD = (nr_rxtx_thread_data_t *) arg;
  UE_nr_rxtx_proc_t *proc = &rxtxD->proc;
  PHY_VARS_NR_UE    *UE   = rxtxD->UE;
  nr_phy_data_t *phy_data = &rxtxD->phy_data;

  pdsch_processing(UE, proc, phy_data);
}

void dummyWrite(PHY_VARS_NR_UE *UE,openair0_timestamp timestamp, int writeBlockSize) {
  void *dummy_tx[UE->frame_parms.nb_antennas_tx];
  int16_t dummy_tx_data[UE->frame_parms.nb_antennas_tx][2*writeBlockSize]; // 2 because the function we call use pairs of int16_t implicitly as complex numbers
  memset(dummy_tx_data, 0, sizeof(dummy_tx_data));
  for (int i=0; i<UE->frame_parms.nb_antennas_tx; i++)
    dummy_tx[i]=dummy_tx_data[i];

  AssertFatal( writeBlockSize ==
               UE->rfdevice.trx_write_func(&UE->rfdevice,
               timestamp,
               dummy_tx,
               writeBlockSize,
               UE->frame_parms.nb_antennas_tx,
               4),"");

}

void readFrame(PHY_VARS_NR_UE *UE,  openair0_timestamp *timestamp, bool toTrash) {

  void *rxp[NB_ANTENNAS_RX];

  for(int x=0; x<20; x++) {  // two frames for initial sync
    for (int slot=0; slot<UE->frame_parms.slots_per_subframe; slot ++ ) {
      for (int i=0; i<UE->frame_parms.nb_antennas_rx; i++) {
        if (toTrash)
          rxp[i]=malloc16(UE->frame_parms.get_samples_per_slot(slot,&UE->frame_parms)*4);
        else
          rxp[i] = ((void *)&UE->common_vars.rxdata[i][0]) +
                   4*((x*UE->frame_parms.samples_per_subframe)+
                   UE->frame_parms.get_samples_slot_timestamp(slot,&UE->frame_parms,0));
      }
        
      AssertFatal( UE->frame_parms.get_samples_per_slot(slot,&UE->frame_parms) ==
                   UE->rfdevice.trx_read_func(&UE->rfdevice,
                   timestamp,
                   rxp,
                   UE->frame_parms.get_samples_per_slot(slot,&UE->frame_parms),
                   UE->frame_parms.nb_antennas_rx), "");

      if (IS_SOFTMODEM_RFSIM)
        dummyWrite(UE,*timestamp, UE->frame_parms.get_samples_per_slot(slot,&UE->frame_parms));
      if (toTrash)
        for (int i=0; i<UE->frame_parms.nb_antennas_rx; i++)
          free(rxp[i]);
    }
  }

}

void syncInFrame(PHY_VARS_NR_UE *UE, openair0_timestamp *timestamp) {

    LOG_I(PHY,"Resynchronizing RX by %d samples\n",UE->rx_offset);

    if (IS_SOFTMODEM_IQPLAYER || IS_SOFTMODEM_IQRECORDER) {
      // Resynchonize by slot (will work with numerology 1 only)
      for ( int size=UE->rx_offset ; size > 0 ; size -= UE->frame_parms.samples_per_subframe/2 ) {
	int unitTransfer=size>UE->frame_parms.samples_per_subframe/2 ? UE->frame_parms.samples_per_subframe/2 : size ;
	AssertFatal(unitTransfer ==
		    UE->rfdevice.trx_read_func(&UE->rfdevice,
					       timestamp,
					       (void **)UE->common_vars.rxdata,
					       unitTransfer,
					       UE->frame_parms.nb_antennas_rx),"");
      }
    } else {
      *timestamp += UE->frame_parms.get_samples_per_slot(1,&UE->frame_parms);
      for ( int size=UE->rx_offset ; size > 0 ; size -= UE->frame_parms.samples_per_subframe ) {
	int unitTransfer=size>UE->frame_parms.samples_per_subframe ? UE->frame_parms.samples_per_subframe : size ;
	// we write before read because gNB waits for UE to write and both executions halt
	// this happens here as the read size is samples_per_subframe which is very much larger than samp_per_slot
	if (IS_SOFTMODEM_RFSIM) dummyWrite(UE,*timestamp, unitTransfer);
	AssertFatal(unitTransfer ==
		    UE->rfdevice.trx_read_func(&UE->rfdevice,
					       timestamp,
					       (void **)UE->common_vars.rxdata,
					       unitTransfer,
					       UE->frame_parms.nb_antennas_rx),"");
	*timestamp += unitTransfer; // this does not affect the read but needed for RFSIM write
      }
    }
}

static inline int get_firstSymSamp(uint16_t slot, NR_DL_FRAME_PARMS *fp) {
  if (fp->numerology_index == 0)
    return fp->nb_prefix_samples0 + fp->ofdm_symbol_size;
  int num_samples = (slot%(fp->slots_per_subframe/2)) ? fp->nb_prefix_samples : fp->nb_prefix_samples0;
  num_samples += fp->ofdm_symbol_size;
  return num_samples;
}

static inline int get_readBlockSize(uint16_t slot, NR_DL_FRAME_PARMS *fp) {
  int rem_samples = fp->get_samples_per_slot(slot, fp) - get_firstSymSamp(slot, fp);
  int next_slot_first_symbol = 0;
  if (slot < (fp->slots_per_frame-1))
    next_slot_first_symbol = get_firstSymSamp(slot+1, fp);
  return rem_samples + next_slot_first_symbol;
}

void *UE_thread(void *arg)
{
  //this thread should be over the processing thread to keep in real time
  PHY_VARS_NR_UE *UE = (PHY_VARS_NR_UE *) arg;
  //  int tx_enabled = 0;
  openair0_timestamp timestamp = 0;
  openair0_timestamp writeTimestamp = 0;
  void *rxp[NB_ANTENNAS_RX];
  int start_rx_stream = 0;
  fapi_nr_config_request_t *cfg = &UE->nrUE_config;
  AssertFatal(0== openair0_device_load(&(UE->rfdevice), &openair0_cfg[0]), "");
  UE->rfdevice.host_type = RAU_HOST;
  UE->is_synchronized = 0;
  AssertFatal(UE->rfdevice.trx_start_func(&UE->rfdevice) == 0, "Could not start the device\n");

  notifiedFIFO_t nf;
  initNotifiedFIFO(&nf);

  notifiedFIFO_t txFifo;
  initNotifiedFIFO(&txFifo);

  notifiedFIFO_t freeBlocks;
  initNotifiedFIFO_nothreadSafe(&freeBlocks);

  int timing_advance = UE->timing_advance;
  NR_UE_MAC_INST_t *mac = get_mac_inst(0);

  bool syncRunning=false;
  const int nb_slot_frame = UE->frame_parms.slots_per_frame;
  int absolute_slot=0, decoded_frame_rx=INT_MAX, trashed_frames=0;
  initNotifiedFIFO(&UE->phy_config_ind);
  int tx_wait_for_dlsch[NR_MAX_SLOTS_PER_FRAME];

  int num_ind_fifo = nb_slot_frame;
  for(int i=0; i < num_ind_fifo; i++) {
    initNotifiedFIFO(UE->tx_resume_ind_fifo + i);
  }

  while (!oai_exit) {

    if (syncRunning) {
      notifiedFIFO_elt_t *res=tryPullTpool(&nf,&(get_nrUE_params()->Tpool));

      if (res) {
        syncRunning=false;
        if (UE->is_synchronized) {
          decoded_frame_rx = mac->mib_frame;
          LOG_I(PHY,"UE synchronized decoded_frame_rx=%d UE->init_sync_frame=%d trashed_frames=%d\n",
                decoded_frame_rx,
                UE->init_sync_frame,
                trashed_frames);
          // shift the frame index with all the frames we trashed meanwhile we perform the synch search
          decoded_frame_rx = (decoded_frame_rx + UE->init_sync_frame + trashed_frames) % MAX_FRAME_NUMBER;
        }
        delNotifiedFIFO_elt(res);
        start_rx_stream=0;
      } else {
	if (IS_SOFTMODEM_IQPLAYER || IS_SOFTMODEM_IQRECORDER) {
	  // For IQ recorder/player we force synchronization to happen in 280 ms
	  while (trashed_frames != 28) {
	    readFrame(UE, &timestamp, true);
	    trashed_frames+=2;
	  }
	} else {
	  readFrame(UE, &timestamp, true);
	  trashed_frames+=2;
	}
        continue;
      }
    }

    AssertFatal(!syncRunning, "At this point synchronization can't be running\n");

    if (!UE->is_synchronized) {
      readFrame(UE, &timestamp, false);
      notifiedFIFO_elt_t *Msg = newNotifiedFIFO_elt(sizeof(syncData_t), 0, &nf, UE_synch);
      syncData_t *syncMsg = (syncData_t *)NotifiedFifoData(Msg);
      syncMsg->UE = UE;
      memset(&syncMsg->proc, 0, sizeof(syncMsg->proc));
      pushTpool(&(get_nrUE_params()->Tpool), Msg);
      trashed_frames = 0;
      syncRunning = true;
      continue;
    }

    if (start_rx_stream == 0) {
      start_rx_stream=1;
      syncInFrame(UE, &timestamp);
      UE->rx_offset=0;
      UE->time_sync_cell=0;
      // read in first symbol
      AssertFatal (UE->frame_parms.ofdm_symbol_size+UE->frame_parms.nb_prefix_samples0 ==
                   UE->rfdevice.trx_read_func(&UE->rfdevice,
                                              &timestamp,
                                              (void **)UE->common_vars.rxdata,
                                              UE->frame_parms.ofdm_symbol_size+UE->frame_parms.nb_prefix_samples0,
                                              UE->frame_parms.nb_antennas_rx),"");
      // we have the decoded frame index in the return of the synch process
      // and we shifted above to the first slot of next frame
      decoded_frame_rx++;
      // we do ++ first in the regular processing, so it will be begin of frame;
      absolute_slot = decoded_frame_rx * nb_slot_frame - 1;
      // We have resynchronized, maybe after RF loss so we need to purge any existing context
      memset(tx_wait_for_dlsch, 0, sizeof(tx_wait_for_dlsch));
      for (int i = 0; i < num_ind_fifo; i++) {
        notifiedFIFO_elt_t *res;
        while ((res = pollNotifiedFIFO(UE->tx_resume_ind_fifo + i)))
          delNotifiedFIFO_elt(res);
      }
      continue;
    }


    absolute_slot++;

    int slot_nr = absolute_slot % nb_slot_frame;
    nr_rxtx_thread_data_t curMsg = {0};
    curMsg.UE=UE;
    // update thread index for received subframe
    curMsg.proc.nr_slot_rx  = slot_nr;
    curMsg.proc.nr_slot_tx  = (absolute_slot + DURATION_RX_TO_TX) % nb_slot_frame;
    curMsg.proc.frame_rx    = (absolute_slot / nb_slot_frame) % MAX_FRAME_NUMBER;
    curMsg.proc.frame_tx    = ((absolute_slot + DURATION_RX_TO_TX) / nb_slot_frame) % MAX_FRAME_NUMBER;
    if (mac->phy_config_request_sent) {
      curMsg.proc.rx_slot_type = nr_ue_slot_select(cfg, curMsg.proc.nr_slot_rx);
      curMsg.proc.tx_slot_type = nr_ue_slot_select(cfg, curMsg.proc.nr_slot_tx);
    }
    else {
      curMsg.proc.rx_slot_type = NR_DOWNLINK_SLOT;
      curMsg.proc.tx_slot_type = NR_DOWNLINK_SLOT;
    }

    int firstSymSamp = get_firstSymSamp(slot_nr, &UE->frame_parms);
    for (int i=0; i<UE->frame_parms.nb_antennas_rx; i++)
      rxp[i] = (void *)&UE->common_vars.rxdata[i][firstSymSamp+
               UE->frame_parms.get_samples_slot_timestamp(slot_nr,&UE->frame_parms,0)];

    int readBlockSize, writeBlockSize;

    readBlockSize = get_readBlockSize(slot_nr, &UE->frame_parms);
    writeBlockSize = UE->frame_parms.get_samples_per_slot((slot_nr + DURATION_RX_TO_TX) % nb_slot_frame, &UE->frame_parms);
    if (UE->apply_timing_offset && (slot_nr == nb_slot_frame - 1)) {
      const int sampShift = -(UE->rx_offset>>1);
      readBlockSize -= sampShift;
      writeBlockSize -= sampShift;
      UE->apply_timing_offset = false;
    }

    AssertFatal(readBlockSize ==
                UE->rfdevice.trx_read_func(&UE->rfdevice,
                                           &timestamp,
                                           rxp,
                                           readBlockSize,
                                           UE->frame_parms.nb_antennas_rx),"");

    if(slot_nr == (nb_slot_frame - 1)) {
      // read in first symbol of next frame and adjust for timing drift
      int first_symbols = UE->frame_parms.ofdm_symbol_size + UE->frame_parms.nb_prefix_samples0; // first symbol of every frames

      if (first_symbols > 0) {
        openair0_timestamp ignore_timestamp;
        AssertFatal(first_symbols ==
                    UE->rfdevice.trx_read_func(&UE->rfdevice,
                                               &ignore_timestamp,
                                               (void **)UE->common_vars.rxdata,
                                               first_symbols,
                                               UE->frame_parms.nb_antennas_rx),"");
      } else
        LOG_E(PHY,"can't compensate: diff =%d\n", first_symbols);
    }

    // use previous timing_advance value to compute writeTimestamp
    writeTimestamp = timestamp +
      UE->frame_parms.get_samples_slot_timestamp(slot_nr,&UE->frame_parms,DURATION_RX_TO_TX)
      - firstSymSamp - openair0_cfg[0].tx_sample_advance -
      UE->N_TA_offset - timing_advance;

    // but use current UE->timing_advance value to compute writeBlockSize
    if (UE->timing_advance != timing_advance) {
      writeBlockSize -= UE->timing_advance - timing_advance;
      timing_advance = UE->timing_advance;
    }

    if (curMsg.proc.nr_slot_tx == 0)
      nr_ue_rrc_timer_trigger(UE->Mod_id, curMsg.proc.frame_tx, curMsg.proc.gNB_id);

    // RX slot processing. We launch and forget.
    notifiedFIFO_elt_t *newRx = newNotifiedFIFO_elt(sizeof(nr_rxtx_thread_data_t), curMsg.proc.nr_slot_rx, NULL, UE_dl_processing);
    nr_rxtx_thread_data_t *curMsgRx = (nr_rxtx_thread_data_t *)NotifiedFifoData(newRx);
    *curMsgRx = (nr_rxtx_thread_data_t){.proc = curMsg.proc, .UE = UE};
    UE_dl_preprocessing(UE, &curMsgRx->proc, tx_wait_for_dlsch, &curMsgRx->phy_data);
    pushTpool(&(get_nrUE_params()->Tpool), newRx);

    // Start TX slot processing here. It runs in parallel with RX slot processing
    // in current code, DURATION_RX_TO_TX constant is the limit to get UL data to encode from a RX slot
    notifiedFIFO_elt_t *newTx = newNotifiedFIFO_elt(sizeof(nr_rxtx_thread_data_t), curMsg.proc.nr_slot_tx, &txFifo, processSlotTX);
    nr_rxtx_thread_data_t *curMsgTx = (nr_rxtx_thread_data_t *)NotifiedFifoData(newTx);
    curMsgTx->proc = curMsg.proc;
    curMsgTx->writeBlockSize = writeBlockSize;
    curMsgTx->proc.timestamp_tx = writeTimestamp;
    curMsgTx->UE = UE;
    curMsgTx->tx_wait_for_dlsch = tx_wait_for_dlsch[curMsgTx->proc.nr_slot_tx];
    tx_wait_for_dlsch[curMsgTx->proc.nr_slot_tx] = 0;
    pushTpool(&(get_nrUE_params()->Tpool), newTx);

    // Wait for TX slot processing to finish
    // Should be removed when bugs, race conditions, will be fixed
    notifiedFIFO_elt_t *res;
    res = pullTpool(&txFifo, &(get_nrUE_params()->Tpool));
    if (res == NULL)
      LOG_E(PHY, "Tpool has been aborted\n");
    else
      delNotifiedFIFO_elt(res);
  } // while !oai_exit

  return NULL;
}

void init_NR_UE(int nb_inst, char *uecap_file, char *reconfig_file, char *rbconfig_file)
{
  NR_UE_RRC_INST_t *rrc_inst = nr_rrc_init_ue(uecap_file, nb_inst);
  NR_UE_MAC_INST_t *mac_inst = nr_l2_init_ue(nb_inst);
  AssertFatal(mac_inst, "Couldn't allocate MAC module\n");

  for (int i = 0; i < nb_inst; i++) {
    NR_UE_MAC_INST_t *mac = get_mac_inst(i);
    AssertFatal((mac->if_module = nr_ue_if_module_init(i)) != NULL, "can not initialize IF module\n");
    if (!get_softmodem_params()->sa) {
      init_nsa_message(rrc_inst, reconfig_file, rbconfig_file);
      nr_rlc_activate_srb0(mac_inst->crnti, NULL, send_srb0_rrc);
    }
    //TODO: Move this call to RRC
    start_sidelink((&rrc_inst[i])->ue_id);
  }
}

void init_NR_UE_threads(int nb_inst) {
  int inst;

  pthread_t threads[nb_inst];

  for (inst=0; inst < nb_inst; inst++) {
    PHY_VARS_NR_UE *UE = PHY_vars_UE_g[inst][0];

    LOG_I(PHY,"Intializing UE Threads for instance %d (%p,%p)...\n",inst,PHY_vars_UE_g[inst],PHY_vars_UE_g[inst][0]);
    threadCreate(&threads[inst], UE_thread, (void *)UE, "UEthread", -1, OAI_PRIORITY_RT_MAX);
    if (!IS_SOFTMODEM_NOSTATS_BIT) {
      pthread_t stat_pthread;
      threadCreate(&stat_pthread, nrL1_UE_stats_thread, UE, "L1_UE_stats", -1, OAI_PRIORITY_RT_LOW);
    }
  }
}
