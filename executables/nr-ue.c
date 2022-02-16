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

#include <openair1/PHY/impl_defs_top.h>
#include "executables/nr-uesoftmodem.h"
#include "PHY/phy_extern_nr_ue.h"
#include "PHY/INIT/phy_init.h"
#include "NR_MAC_UE/mac_proto.h"
#include "RRC/NR_UE/rrc_proto.h"
#include "SCHED_NR_UE/phy_frame_config_nr.h"
#include "SCHED_NR_UE/defs.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "executables/softmodem-common.h"
#include "LAYER2/nr_pdcp/nr_pdcp_entity.h"
#include "SCHED_NR_UE/pucch_uci_ue_nr.h"
#include "openair2/NR_UE_PHY_INTERFACE/NR_IF_Module.h"

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

queue_t nr_rach_ind_queue;

static void *NRUE_phy_stub_standalone_pnf_task(void *arg);

static int dump_L1_UE_meas_stats(PHY_VARS_NR_UE *ue, char *output, int max_len)
{
  int stroff = 0;
  stroff += print_meas_log(&ue->phy_proc_tx, "L1 TX processing", NULL, NULL, output);
  stroff += print_meas_log(&ue->ulsch_encoding_stats, "ULSCH encoding", NULL, NULL, output + stroff);
  stroff += print_meas_log(&ue->phy_proc_rx[0], "L1 RX processing t0", NULL, NULL, output + stroff);
  stroff += print_meas_log(&ue->phy_proc_rx[1], "L1 RX processing t1", NULL, NULL, output + stroff);
  stroff += print_meas_log(&ue->ue_ul_indication_stats, "UL Indication", NULL, NULL, output + stroff);
  stroff += print_meas_log(&ue->rx_pdsch_stats, "PDSCH receiver", NULL, NULL, output + stroff);
  stroff += print_meas_log(&ue->dlsch_decoding_stats[0], "PDSCH decoding t0", NULL, NULL, output + stroff);
  stroff += print_meas_log(&ue->dlsch_decoding_stats[1], "PDSCH decoding t1", NULL, NULL, output + stroff);
  stroff += print_meas_log(&ue->dlsch_deinterleaving_stats, " -> Deinterleive", NULL, NULL, output + stroff);
  stroff += print_meas_log(&ue->dlsch_rate_unmatching_stats, " -> Rate Unmatch", NULL, NULL, output + stroff);
  stroff += print_meas_log(&ue->dlsch_ldpc_decoding_stats, " ->  LDPC Decode", NULL, NULL, output + stroff);
  stroff += print_meas_log(&ue->dlsch_unscrambling_stats, "PDSCH unscrambling", NULL, NULL, output + stroff);
  stroff += print_meas_log(&ue->dlsch_rx_pdcch_stats, "PDCCH handling", NULL, NULL, output + stroff);
  return stroff;
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

  int nb_connected_gNB = 1, gNB_id;

  ue->Mod_id      = UE_id;
  ue->mac_enabled = 1;
  ue->if_inst     = nr_ue_if_module_init(0);
  ue->dci_thres   = 0;

  // Setting UE mode to NOT_SYNCHED by default
  for (gNB_id = 0; gNB_id < nb_connected_gNB; gNB_id++){
    ue->UE_mode[gNB_id] = NOT_SYNCHED;
    ue->prach_resources[gNB_id] = (NR_PRACH_RESOURCES_t *)malloc16_clear(sizeof(NR_PRACH_RESOURCES_t));
  }

  // initialize all signal buffers
  init_nr_ue_signal(ue, nb_connected_gNB, abstraction_flag);

  // intialize transport
  init_nr_ue_transport(ue, abstraction_flag);

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

static void L1_nsa_prach_procedures(frame_t frame, int slot, fapi_nr_ul_config_prach_pdu *prach_pdu)
{
  NR_UE_MAC_INST_t *mac    = get_mac_inst(0);
  nfapi_nr_rach_indication_t *rach_ind = CALLOC(1, sizeof(*rach_ind));
  rach_ind->sfn = frame;
  rach_ind->slot = slot;
  rach_ind->header.message_id = NFAPI_NR_PHY_MSG_TYPE_RACH_INDICATION;

  uint8_t pdu_index = 0;
  rach_ind->pdu_list = CALLOC(1, sizeof(*rach_ind->pdu_list));
  rach_ind->number_of_pdus  = 1;
  rach_ind->pdu_list[pdu_index].phy_cell_id                         = prach_pdu->phys_cell_id;
  rach_ind->pdu_list[pdu_index].symbol_index                        = prach_pdu->prach_start_symbol;
  rach_ind->pdu_list[pdu_index].slot_index                          = prach_pdu->prach_slot;
  rach_ind->pdu_list[pdu_index].freq_index                          = prach_pdu->num_ra;
  rach_ind->pdu_list[pdu_index].avg_rssi                            = 128;
  rach_ind->pdu_list[pdu_index].avg_snr                             = 0xff; // invalid for now

  rach_ind->pdu_list[pdu_index].num_preamble                        = 1;
  const int num_p = rach_ind->pdu_list[pdu_index].num_preamble;
  rach_ind->pdu_list[pdu_index].preamble_list = calloc(num_p, sizeof(nfapi_nr_prach_indication_preamble_t));
  uint8_t preamble_index = get_softmodem_params()->nsa ?
                           mac->ra.rach_ConfigDedicated->cfra->resources.choice.ssb->ssb_ResourceList.list.array[0]->ra_PreambleIndex :
                           mac->ra.ra_PreambleIndex;
  rach_ind->pdu_list[pdu_index].preamble_list[0].preamble_index     = preamble_index;

  rach_ind->pdu_list[pdu_index].preamble_list[0].timing_advance     = 0;
  rach_ind->pdu_list[pdu_index].preamble_list[0].preamble_pwr       = 0xffffffff;

  if (!put_queue(&nr_rach_ind_queue, rach_ind))
  {
    for (int pdu_index = 0; pdu_index < rach_ind->number_of_pdus; pdu_index++)
    {
      free(rach_ind->pdu_list[pdu_index].preamble_list);
    }
    free(rach_ind->pdu_list);
    free(rach_ind);
  }
  LOG_D(NR_MAC, "We have successfully filled the rach_ind queue with the recently filled rach ind\n");
}

static bool sfn_slot_matcher(void *wanted, void *candidate)
{
  nfapi_p7_message_header_t *msg = candidate;
  int sfn_sf = *(int*)wanted;

  switch (msg->message_id)
  {
    case NFAPI_NR_PHY_MSG_TYPE_RACH_INDICATION:
    {
      nfapi_nr_rach_indication_t *ind = candidate;
      return NFAPI_SFNSLOT2SFN(sfn_sf) == ind->sfn && NFAPI_SFNSLOT2SLOT(sfn_sf) == ind->slot;
    }

    case NFAPI_NR_PHY_MSG_TYPE_RX_DATA_INDICATION:
    {
      nfapi_nr_rx_data_indication_t *ind = candidate;
      return NFAPI_SFNSLOT2SFN(sfn_sf) == ind->sfn && NFAPI_SFNSLOT2SLOT(sfn_sf) == ind->slot;
    }

    case NFAPI_NR_PHY_MSG_TYPE_CRC_INDICATION:
    {
      nfapi_nr_crc_indication_t *ind = candidate;
      return NFAPI_SFNSLOT2SFN(sfn_sf) == ind->sfn && NFAPI_SFNSLOT2SLOT(sfn_sf) == ind->slot;
    }

    case NFAPI_NR_PHY_MSG_TYPE_UCI_INDICATION:
    {
      nfapi_nr_uci_indication_t *ind = candidate;
      return NFAPI_SFNSLOT2SFN(sfn_sf) == ind->sfn && NFAPI_SFNSLOT2SLOT(sfn_sf) == ind->slot;
    }

    case NFAPI_NR_PHY_MSG_TYPE_DL_TTI_REQUEST:
    {
      nfapi_nr_dl_tti_request_t *ind = candidate;
      return NFAPI_SFNSLOT2SFN(sfn_sf) == ind->SFN && NFAPI_SFNSLOT2SLOT(sfn_sf) == ind->Slot;
    }

    case NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST:
    {
      nfapi_nr_tx_data_request_t *ind = candidate;
      return NFAPI_SFNSLOT2SFN(sfn_sf) == ind->SFN && NFAPI_SFNSLOT2SLOT(sfn_sf) == ind->Slot;
    }

    case NFAPI_NR_PHY_MSG_TYPE_UL_DCI_REQUEST:
    {
      nfapi_nr_ul_dci_request_t *ind = candidate;
      return NFAPI_SFNSLOT2SFN(sfn_sf) == ind->SFN && NFAPI_SFNSLOT2SLOT(sfn_sf) == ind->Slot;
    }

    case NFAPI_NR_PHY_MSG_TYPE_UL_TTI_REQUEST:
    {
      nfapi_nr_ul_tti_request_t *ind = candidate;
      return NFAPI_SFNSLOT2SFN(sfn_sf) == ind->SFN && NFAPI_SFNSLOT2SLOT(sfn_sf) == ind->Slot;
    }

    default:
      LOG_E(NR_MAC, "sfn_slot_match bad ID: %d\n", msg->message_id);

  }

  return false;
}

static void process_queued_nr_nfapi_msgs(NR_UE_MAC_INST_t *mac, int sfn_slot)
{
  nfapi_nr_rach_indication_t *rach_ind = unqueue_matching(&nr_rach_ind_queue, MAX_QUEUE_SIZE, sfn_slot_matcher, &sfn_slot);
  nfapi_nr_rx_data_indication_t *rx_ind = unqueue_matching(&nr_rx_ind_queue, MAX_QUEUE_SIZE, sfn_slot_matcher, &sfn_slot);
  nfapi_nr_crc_indication_t *crc_ind = unqueue_matching(&nr_crc_ind_queue, MAX_QUEUE_SIZE, sfn_slot_matcher, &sfn_slot);
  nfapi_nr_dl_tti_request_t *dl_tti_request = get_queue(&nr_dl_tti_req_queue);
  nfapi_nr_ul_dci_request_t *ul_dci_request = get_queue(&nr_ul_dci_req_queue);

  LOG_D(NR_MAC, "Try to get a ul_tti_req for sfn/slot %d %d from queue with %lu items\n",
        NFAPI_SFNSLOT2SFN(mac->nr_ue_emul_l1.active_harq_sfn_slot),NFAPI_SFNSLOT2SLOT(mac->nr_ue_emul_l1.active_harq_sfn_slot), nr_ul_tti_req_queue.num_items);
  nfapi_nr_ul_tti_request_t *ul_tti_request = unqueue_matching(&nr_ul_tti_req_queue, MAX_QUEUE_SIZE, sfn_slot_matcher, &mac->nr_ue_emul_l1.active_harq_sfn_slot);
  if (!ul_tti_request)
  {
      LOG_D(NR_MAC, "Try to get a ul_tti_req from seprate queue because dl_tti_req was late\n");
      ul_tti_request = unqueue_matching(&nr_wait_ul_tti_req_queue, MAX_QUEUE_SIZE, sfn_slot_matcher, &mac->nr_ue_emul_l1.active_harq_sfn_slot);
  }

  if (rach_ind && rach_ind->number_of_pdus > 0)
  {
      NR_UL_IND_t UL_INFO = {
        .rach_ind = *rach_ind,
      };
      send_nsa_standalone_msg(&UL_INFO, rach_ind->header.message_id);
      for (int i = 0; i < rach_ind->number_of_pdus; i++)
      {
        free(rach_ind->pdu_list[i].preamble_list);
      }
      free(rach_ind->pdu_list);
      free(rach_ind);
      nr_Msg1_transmitted(0, 0, NFAPI_SFNSLOT2SFN(sfn_slot), 0);
  }
  if (crc_ind && crc_ind->number_crcs > 0)
  {
    NR_UL_IND_t UL_INFO = {
      .crc_ind = *crc_ind,
    };
    send_nsa_standalone_msg(&UL_INFO, crc_ind->header.message_id);
    free(crc_ind->crc_list);
    free(crc_ind);
  }
  if (rx_ind && rx_ind->number_of_pdus > 0)
  {
    NR_UL_IND_t UL_INFO = {
      .rx_ind = *rx_ind,
    };
    send_nsa_standalone_msg(&UL_INFO, rx_ind->header.message_id);
    free(rx_ind->pdu_list);
    free(rx_ind);
  }
  if (dl_tti_request)
  {
    int dl_tti_sfn_slot = NFAPI_SFNSLOT2HEX(dl_tti_request->SFN, dl_tti_request->Slot);
    nfapi_nr_tx_data_request_t *tx_data_request = unqueue_matching(&nr_tx_req_queue, MAX_QUEUE_SIZE, sfn_slot_matcher, &dl_tti_sfn_slot);
    if (!tx_data_request)
    {
      LOG_E(NR_MAC, "[%d %d] No corresponding tx_data_request for given dl_tti_request sfn/slot\n",
            NFAPI_SFNSLOT2SFN(dl_tti_sfn_slot), NFAPI_SFNSLOT2SLOT(dl_tti_sfn_slot));
      if (get_softmodem_params()->nsa)
        save_nr_measurement_info(dl_tti_request);
      free(dl_tti_request);
      dl_tti_request = NULL;
    }
    else if (dl_tti_request->dl_tti_request_body.nPDUs > 0 && tx_data_request->Number_of_PDUs > 0)
    {
      if (get_softmodem_params()->nsa)
        save_nr_measurement_info(dl_tti_request);
      check_and_process_dci(dl_tti_request, tx_data_request, NULL, NULL);
    }
    else
    {
      AssertFatal(false, "We dont have PDUs in either dl_tti %d or tx_req %d\n",
                  dl_tti_request->dl_tti_request_body.nPDUs, tx_data_request->Number_of_PDUs);
    }
  }
  if (ul_dci_request && ul_dci_request->numPdus > 0)
  {
    check_and_process_dci(NULL, NULL, ul_dci_request, NULL);
  }
  if (ul_tti_request && ul_tti_request->n_pdus > 0)
  {
    check_and_process_dci(NULL, NULL, NULL, ul_tti_request);
  }
}

static void check_nr_prach(NR_UE_MAC_INST_t *mac, nr_uplink_indication_t *ul_info, NR_PRACH_RESOURCES_t *prach_resources)
{
  fapi_nr_ul_config_request_t *ul_config = get_ul_config_request(mac, ul_info->slot_tx);
  if (!ul_config)
  {
    LOG_E(NR_MAC, "mac->ul_config is null! \n");
    return;
  }
  if (mac->ra.ra_state != RA_SUCCEEDED)
  {
    AssertFatal(ul_config->number_pdus < sizeof(ul_config->ul_config_list) / sizeof(ul_config->ul_config_list[0]),
                "Number of PDUS in ul_config = %d > ul_config_list num elements", ul_config->number_pdus);
    fapi_nr_ul_config_prach_pdu *prach_pdu = &ul_config->ul_config_list[ul_config->number_pdus].prach_config_pdu;
    uint8_t nr_prach = nr_ue_get_rach(prach_resources,
                                      prach_pdu,
                                      ul_info->module_id,
                                      ul_info->cc_id,
                                      ul_info->frame_tx,
                                      ul_info->gNB_index,
                                      ul_info->slot_tx);
    if (nr_prach == 1)
    {
      L1_nsa_prach_procedures(ul_info->frame_tx, ul_info->slot_tx, prach_pdu);
      ul_config->number_pdus = 0;
      ul_info->ue_sched_mode = SCHED_ALL;
    }
    else if (nr_prach == 2)
    {
      LOG_I(NR_PHY, "In %s: [UE %d] RA completed, setting UE mode to PUSCH\n", __FUNCTION__, ul_info->module_id);
    }
    else if(nr_prach == 3)
    {
      LOG_I(NR_PHY, "In %s: [UE %d] RA failed, setting UE mode to PRACH\n", __FUNCTION__, ul_info->module_id);
    }
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
  reset_queue(&nr_wait_ul_tti_req_queue);

  NR_PRACH_RESOURCES_t prach_resources;
  memset(&prach_resources, 0, sizeof(prach_resources));
  NR_UL_TIME_ALIGNMENT_t ul_time_alignment;
  memset(&ul_time_alignment, 0, sizeof(ul_time_alignment));
  int last_sfn_slot = -1;
  uint16_t sfn_slot = 0;

  while (!oai_exit)
  {
    if (sem_wait(&sfn_slot_semaphore) != 0)
    {
      LOG_E(NR_MAC, "sem_wait() error\n");
      abort();
    }
    uint16_t *slot_ind = get_queue(&nr_sfn_slot_queue);
    nr_phy_channel_params_t *ch_info = get_queue(&nr_chan_param_queue);
    if (!slot_ind && !ch_info)
    {
      LOG_D(MAC, "get nr_sfn_slot_queue and nr_chan_param_queue == NULL!\n");
      continue;
    }
    if (slot_ind) {
      sfn_slot = *slot_ind;
    }
    else if (ch_info) {
      sfn_slot = ch_info->sfn_slot;
    }

    frame_t frame = NFAPI_SFNSLOT2SFN(sfn_slot);
    int slot = NFAPI_SFNSLOT2SLOT(sfn_slot);
    if (sfn_slot == last_sfn_slot)
    {
      LOG_D(NR_MAC, "repeated sfn_sf = %d.%d\n",
            frame, slot);
      continue;
    }
    last_sfn_slot = sfn_slot;

    LOG_D(NR_MAC, "The received sfn/slot [%d %d] from proxy\n",
          frame, slot);

    module_id_t mod_id = 0;
    NR_UE_MAC_INST_t *mac = get_mac_inst(mod_id);
    if (get_softmodem_params()->sa && mac->mib == NULL)
    {
      LOG_D(NR_MAC, "We haven't gotten MIB. Lets see if we received it\n");
      nr_ue_dl_indication(&mac->dl_info, &ul_time_alignment);
      process_queued_nr_nfapi_msgs(mac, sfn_slot);
    }
    if (mac->scc == NULL && mac->scc_SIB == NULL)
    {
      LOG_D(MAC, "[NSA] mac->scc == NULL and [SA] mac->scc_SIB == NULL!\n");
      continue;
    }

    mac->ra.generate_nr_prach = 0;
    int CC_id = 0;
    uint8_t gNB_id = 0;
    nr_uplink_indication_t ul_info;
    int slots_per_frame = 20; //30 kHZ subcarrier spacing
    int slot_ahead = 2; // TODO: Make this dynamic
    ul_info.cc_id = CC_id;
    ul_info.gNB_index = gNB_id;
    ul_info.module_id = mod_id;
    ul_info.frame_rx = frame;
    ul_info.slot_rx = slot;
    ul_info.slot_tx = (slot + slot_ahead) % slots_per_frame;
    ul_info.frame_tx = (ul_info.slot_rx + slot_ahead >= slots_per_frame) ? ul_info.frame_rx + 1 : ul_info.frame_rx;
    ul_info.ue_sched_mode = SCHED_ALL;

    if (pthread_mutex_lock(&mac->mutex_dl_info)) abort();

    memset(&mac->dl_info, 0, sizeof(mac->dl_info));
    mac->dl_info.cc_id = CC_id;
    mac->dl_info.gNB_index = gNB_id;
    mac->dl_info.module_id = mod_id;
    mac->dl_info.frame = frame;
    mac->dl_info.slot = slot;
    mac->dl_info.thread_id = 0;
    mac->dl_info.dci_ind = NULL;
    mac->dl_info.rx_ind = NULL;

    if (is_nr_DL_slot(get_softmodem_params()->nsa ?
                      mac->scc->tdd_UL_DL_ConfigurationCommon :
                      mac->scc_SIB->tdd_UL_DL_ConfigurationCommon,
                      ul_info.slot_rx))
    {
      nr_ue_dl_indication(&mac->dl_info, &ul_time_alignment);
    }

    if (pthread_mutex_unlock(&mac->mutex_dl_info)) abort();

    if (is_nr_UL_slot(get_softmodem_params()->nsa ?
                      mac->scc->tdd_UL_DL_ConfigurationCommon :
                      mac->scc_SIB->tdd_UL_DL_ConfigurationCommon,
                      ul_info.slot_tx, mac->frame_type))
    {
      LOG_D(NR_MAC, "Slot %d. calling nr_ue_ul_ind() from %s\n", ul_info.slot_tx, __FUNCTION__);
      nr_ue_ul_indication(&ul_info);
      check_nr_prach(mac, &ul_info, &prach_resources);
    }
    if (!IS_SOFTMODEM_NOS1 && get_softmodem_params()->sa) {
      NR_UE_MAC_INST_t *mac = get_mac_inst(0);
      protocol_ctxt_t ctxt;
      PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, 0, ENB_FLAG_NO, mac->crnti, frame, slot, 0);
      pdcp_run(&ctxt);
    }
    process_queued_nr_nfapi_msgs(mac, sfn_slot);
    free(slot_ind);
    slot_ind = NULL;
    free(ch_info);
    ch_info = NULL;
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
  static int freq_offset=0;
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

  LOG_W(PHY, "Starting sync detection\n");

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
      LOG_I(PHY, "[UE thread Synch] Running Initial Synch (mode %d)\n",UE->mode);

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

        if (UE->mode != loop_through_memory) {
          UE->rfdevice.trx_set_freq_func(&UE->rfdevice,&openair0_cfg[0],0);
          //UE->rfdevice.trx_set_gains_func(&openair0,&openair0_cfg[0]);
          //UE->rfdevice.trx_stop_func(&UE->rfdevice);
          // sleep(1);
          /*if (UE->rfdevice.trx_start_func(&UE->rfdevice) != 0 ) {
            LOG_E(HW,"Could not start the device\n");
            oai_exit=1;
            }*/
        }

        if (UE->UE_scan_carrier == 1) {
          UE->UE_scan_carrier = 0;
        } else {
          UE->is_synchronized = 1;
        }
      } else {

        if (UE->UE_scan_carrier == 1) {

          if (freq_offset >= 0)
            freq_offset += 100;

          freq_offset *= -1;

          nr_rf_card_config_freq(&openair0_cfg[UE->rf_map.card], ul_carrier, dl_carrier, freq_offset);

          LOG_I(PHY, "Initial sync failed: trying carrier off %d Hz\n", freq_offset);

          if (UE->mode != loop_through_memory)
            UE->rfdevice.trx_set_freq_func(&UE->rfdevice,&openair0_cfg[0],0);
        }

        break;

      case si:
      default:
        break;
      }
  }
}

void processSlotTX(void *arg) {

  nr_rxtx_thread_data_t *rxtxD = (nr_rxtx_thread_data_t *) arg;
  UE_nr_rxtx_proc_t *proc = &rxtxD->proc;
  PHY_VARS_NR_UE    *UE   = rxtxD->UE;
  fapi_nr_config_request_t *cfg = &UE->nrUE_config;
  int tx_slot_type = nr_ue_slot_select(cfg, proc->frame_tx, proc->nr_slot_tx);
  uint8_t gNB_id = 0;

  LOG_D(PHY,"%d.%d => slot type %d\n",proc->frame_tx,proc->nr_slot_tx,tx_slot_type);
  if (tx_slot_type == NR_UPLINK_SLOT || tx_slot_type == NR_MIXED_SLOT){

    // trigger L2 to run ue_scheduler thru IF module
    // [TODO] mapping right after NR initial sync
    if(UE->if_inst != NULL && UE->if_inst->ul_indication != NULL) {
      start_meas(&UE->ue_ul_indication_stats);
      nr_uplink_indication_t ul_indication;
      memset((void*)&ul_indication, 0, sizeof(ul_indication));

      ul_indication.module_id = UE->Mod_id;
      ul_indication.gNB_index = gNB_id;
      ul_indication.cc_id     = UE->CC_id;
      ul_indication.frame_rx  = proc->frame_rx;
      ul_indication.slot_rx   = proc->nr_slot_rx;
      ul_indication.frame_tx  = proc->frame_tx;
      ul_indication.slot_tx   = proc->nr_slot_tx;
      ul_indication.thread_id = proc->thread_id;
      ul_indication.ue_sched_mode = rxtxD->ue_sched_mode;

      UE->if_inst->ul_indication(&ul_indication);
      stop_meas(&UE->ue_ul_indication_stats);
    }

    if ((UE->mode != loop_through_memory) && (rxtxD->ue_sched_mode != NOT_PUSCH)) {
      phy_procedures_nrUE_TX(UE,proc,0);
    }
  }
}

void processSlotRX(void *arg) {

  nr_rxtx_thread_data_t *rxtxD = (nr_rxtx_thread_data_t *) arg;
  UE_nr_rxtx_proc_t *proc = &rxtxD->proc;
  PHY_VARS_NR_UE    *UE   = rxtxD->UE;
  fapi_nr_config_request_t *cfg = &UE->nrUE_config;
  int rx_slot_type = nr_ue_slot_select(cfg, proc->frame_rx, proc->nr_slot_rx);
  int tx_slot_type = nr_ue_slot_select(cfg, proc->frame_tx, proc->nr_slot_tx);
  uint8_t gNB_id = 0;

  if (IS_SOFTMODEM_NOS1 || get_softmodem_params()->sa) {
    /* send tick to RLC and PDCP every ms */
    if (proc->nr_slot_rx % UE->frame_parms.slots_per_subframe == 0) {
      void nr_rlc_tick(int frame, int subframe);
      void nr_pdcp_tick(int frame, int subframe);
      nr_rlc_tick(proc->frame_rx, proc->nr_slot_rx / UE->frame_parms.slots_per_subframe);
      nr_pdcp_tick(proc->frame_rx, proc->nr_slot_rx / UE->frame_parms.slots_per_subframe);
    }
  }

  if (rx_slot_type == NR_DOWNLINK_SLOT || rx_slot_type == NR_MIXED_SLOT){

    if(UE->if_inst != NULL && UE->if_inst->dl_indication != NULL) {
      nr_downlink_indication_t dl_indication;
      nr_fill_dl_indication(&dl_indication, NULL, NULL, proc, UE, gNB_id);
      UE->if_inst->dl_indication(&dl_indication, NULL);
    }

  // Process Rx data for one sub-frame
#ifdef UE_SLOT_PARALLELISATION
    phy_procedures_slot_parallelization_nrUE_RX( UE, proc, 0, 0, 1, no_relay, NULL );
#else
    uint64_t a=rdtsc();
    phy_procedures_nrUE_RX(UE, proc, gNB_id, get_nrUE_params()->nr_dlsch_parallel, &rxtxD->txFifo);
    LOG_D(PHY, "In %s: slot %d, time %lu\n", __FUNCTION__, proc->nr_slot_rx, (rdtsc()-a)/3500);
#endif

    if(IS_SOFTMODEM_NOS1 || get_softmodem_params()->sa){
      NR_UE_MAC_INST_t *mac = get_mac_inst(0);
      protocol_ctxt_t ctxt;
      PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, UE->Mod_id, ENB_FLAG_NO, mac->crnti, proc->frame_rx, proc->nr_slot_rx, 0);
      pdcp_run(&ctxt);
    }

    // Wait for PUSCH processing to finish
    notifiedFIFO_elt_t *res;
    res = pullTpool(&rxtxD->txFifo,&(get_nrUE_params()->Tpool));
    delNotifiedFIFO_elt(res);

    // calling UL_indication to schedule things other than PUSCH (eg, PUCCH)
    rxtxD->ue_sched_mode = NOT_PUSCH;
    processSlotTX(rxtxD);

  } else {
    rxtxD->ue_sched_mode = SCHED_ALL;
    processSlotTX(rxtxD);
  }

  if (tx_slot_type == NR_UPLINK_SLOT || tx_slot_type == NR_MIXED_SLOT){
    if (UE->UE_mode[gNB_id] <= PUSCH) {
      if (get_softmodem_params()->usim_test==0) {
        pucch_procedures_ue_nr(UE,
                               gNB_id,
                               proc);
      }

      LOG_D(PHY, "Sending Uplink data \n");
      nr_ue_pusch_common_procedures(UE,
                                    proc->nr_slot_tx,
                                    &UE->frame_parms,1);
    }

    if (UE->UE_mode[gNB_id] > NOT_SYNCHED && UE->UE_mode[gNB_id] < PUSCH) {
      nr_ue_prach_procedures(UE, proc, gNB_id);
    }
    LOG_D(PHY,"****** end TX-Chain for AbsSubframe %d.%d ******\n", proc->frame_tx, proc->nr_slot_tx);
  }

  ue_ta_procedures(UE, proc->nr_slot_tx, proc->frame_tx);
}

void dummyWrite(PHY_VARS_NR_UE *UE,openair0_timestamp timestamp, int writeBlockSize) {
  void *dummy_tx[UE->frame_parms.nb_antennas_tx];

  for (int i=0; i<UE->frame_parms.nb_antennas_tx; i++)
    dummy_tx[i]=malloc16_clear(writeBlockSize*4);

  AssertFatal( writeBlockSize ==
               UE->rfdevice.trx_write_func(&UE->rfdevice,
               timestamp,
               dummy_tx,
               writeBlockSize,
               UE->frame_parms.nb_antennas_tx,
               4),"");

  for (int i=0; i<UE->frame_parms.nb_antennas_tx; i++)
    free(dummy_tx[i]);
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

    LOG_I(PHY,"Resynchronizing RX by %d samples (mode = %d)\n",UE->rx_offset,UE->mode);

    *timestamp += UE->frame_parms.get_samples_per_slot(1,&UE->frame_parms);
    for ( int size=UE->rx_offset ; size > 0 ; size -= UE->frame_parms.samples_per_subframe ) {
      int unitTransfer=size>UE->frame_parms.samples_per_subframe ? UE->frame_parms.samples_per_subframe : size ;
      // we write before read becasue gNB waits for UE to write and both executions halt
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

int computeSamplesShift(PHY_VARS_NR_UE *UE) {

  // compute TO compensation that should be applied for this frame
  if ( UE->rx_offset < UE->frame_parms.samples_per_frame/2  &&
       UE->rx_offset > 0 ) {
    LOG_I(PHY,"!!!adjusting -1 samples!!! rx_offset == %d\n", UE->rx_offset);
    UE->rx_offset   = 0; // reset so that it is not applied falsely in case of SSB being only in every second frame
    UE->max_pos_fil = 0; // reset IIR filter when sample shift is applied
    return -1 ;
  }

  if ( UE->rx_offset > UE->frame_parms.samples_per_frame/2 &&
       UE->rx_offset < UE->frame_parms.samples_per_frame ) {
    int rx_offset = UE->rx_offset - UE->frame_parms.samples_per_frame;
    LOG_I(PHY,"!!!adjusting +1 samples!!! rx_offset == %d\n", rx_offset);
    UE->rx_offset   = 0; // reset so that it is not applied falsely in case of SSB being only in every second frame
    UE->max_pos_fil = 0; // reset IIR filter when sample shift is applied
    return 1;
  }

  return 0;
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

void *UE_thread(void *arg) {
  //this thread should be over the processing thread to keep in real time
  PHY_VARS_NR_UE *UE = (PHY_VARS_NR_UE *) arg;
  //  int tx_enabled = 0;
  openair0_timestamp timestamp, writeTimestamp;
  void *rxp[NB_ANTENNAS_RX], *txp[NB_ANTENNAS_TX];
  int start_rx_stream = 0;
  AssertFatal(0== openair0_device_load(&(UE->rfdevice), &openair0_cfg[0]), "");
  UE->rfdevice.host_type = RAU_HOST;
  UE->lost_sync = 0;
  UE->is_synchronized = 0;
  AssertFatal(UE->rfdevice.trx_start_func(&UE->rfdevice) == 0, "Could not start the device\n");

  notifiedFIFO_t nf;
  initNotifiedFIFO(&nf);

  notifiedFIFO_t freeBlocks;
  initNotifiedFIFO_nothreadSafe(&freeBlocks);

  int nbSlotProcessing=0;
  int thread_idx=0;
  NR_UE_MAC_INST_t *mac = get_mac_inst(0);
  int timing_advance = UE->timing_advance;

  bool syncRunning=false;
  const int nb_slot_frame = UE->frame_parms.slots_per_frame;
  int absolute_slot=0, decoded_frame_rx=INT_MAX, trashed_frames=0;

  for (int i=0; i<NR_RX_NB_TH+1; i++) {// NR_RX_NB_TH working + 1 we are making to be pushed
    notifiedFIFO_elt_t *newElt = newNotifiedFIFO_elt(sizeof(nr_rxtx_thread_data_t), RX_JOB_ID,&nf,processSlotRX);
    nr_rxtx_thread_data_t *curMsg=(nr_rxtx_thread_data_t *)NotifiedFifoData(newElt);
    initNotifiedFIFO(&curMsg->txFifo);
    pushNotifiedFIFO_nothreadSafe(&freeBlocks, newElt);
  }

  while (!oai_exit) {
    if (UE->lost_sync) {
      int nb = abortTpool(&(get_nrUE_params()->Tpool),RX_JOB_ID);
      nb += abortNotifiedFIFO(&nf, RX_JOB_ID);
      LOG_I(PHY,"Number of aborted slots %d\n",nb);
      for (int i=0; i<nb; i++)
        pushNotifiedFIFO_nothreadSafe(&freeBlocks, newNotifiedFIFO_elt(sizeof(nr_rxtx_thread_data_t), RX_JOB_ID,&nf,processSlotRX));
      nbSlotProcessing = 0;
      UE->is_synchronized = 0;
      UE->lost_sync = 0;
    }

    if (syncRunning) {
      notifiedFIFO_elt_t *res=tryPullTpool(&nf,&(get_nrUE_params()->Tpool));

      if (res) {
        syncRunning=false;
        syncData_t *tmp=(syncData_t *)NotifiedFifoData(res);
        if (UE->is_synchronized) {
          decoded_frame_rx=(((mac->mib->systemFrameNumber.buf[0] >> mac->mib->systemFrameNumber.bits_unused)<<4) | tmp->proc.decoded_frame_rx);
          // shift the frame index with all the frames we trashed meanwhile we perform the synch search
          decoded_frame_rx=(decoded_frame_rx + UE->init_sync_frame + trashed_frames) % MAX_FRAME_NUMBER;
        }
        delNotifiedFIFO_elt(res);
        start_rx_stream=0;
      } else {
        readFrame(UE, &timestamp, true);
        trashed_frames+=2;
        continue;
      }
    }

    AssertFatal( !syncRunning, "At this point synchronization can't be running\n");

    if (!UE->is_synchronized) {
      readFrame(UE, &timestamp, false);
      notifiedFIFO_elt_t *Msg=newNotifiedFIFO_elt(sizeof(syncData_t),0,&nf,UE_synch);
      syncData_t *syncMsg=(syncData_t *)NotifiedFifoData(Msg);
      syncMsg->UE=UE;
      memset(&syncMsg->proc, 0, sizeof(syncMsg->proc));
      pushTpool(&(get_nrUE_params()->Tpool), Msg);
      trashed_frames=0;
      syncRunning=true;
      continue;
    }

    if (start_rx_stream==0) {
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
      absolute_slot=decoded_frame_rx*nb_slot_frame -1;
      continue;
    }


    absolute_slot++;

    // whatever means thread_idx
    // Fix me: will be wrong when slot 1 is slow, as slot 2 finishes
    // Slot 3 will overlap if NR_RX_NB_TH is 2
    // this is general failure in UE !!!
    thread_idx = absolute_slot % NR_RX_NB_TH;
    int slot_nr = absolute_slot % nb_slot_frame;
    notifiedFIFO_elt_t *msgToPush;
    AssertFatal((msgToPush=pullNotifiedFIFO_nothreadSafe(&freeBlocks)) != NULL,"chained list failure");
    nr_rxtx_thread_data_t *curMsg=(nr_rxtx_thread_data_t *)NotifiedFifoData(msgToPush);
    curMsg->UE=UE;
    // update thread index for received subframe
    curMsg->proc.thread_id   = thread_idx;
    curMsg->proc.CC_id       = UE->CC_id;
    curMsg->proc.nr_slot_rx  = slot_nr;
    curMsg->proc.nr_slot_tx  = (absolute_slot + DURATION_RX_TO_TX) % nb_slot_frame;
    curMsg->proc.frame_rx    = (absolute_slot/nb_slot_frame) % MAX_FRAME_NUMBER;
    curMsg->proc.frame_tx    = ((absolute_slot+DURATION_RX_TO_TX)/nb_slot_frame) % MAX_FRAME_NUMBER;
    curMsg->proc.decoded_frame_rx=-1;
    //LOG_I(PHY,"Process slot %d thread Idx %d total gain %d\n", slot_nr, thread_idx, UE->rx_total_gain_dB);

#ifdef OAI_ADRV9371_ZC706
    /*uint32_t total_gain_dB_prev = 0;
    if (total_gain_dB_prev != UE->rx_total_gain_dB) {
        total_gain_dB_prev = UE->rx_total_gain_dB;
        openair0_cfg[0].rx_gain[0] = UE->rx_total_gain_dB;
        UE->rfdevice.trx_set_gains_func(&UE->rfdevice,&openair0_cfg[0]);
    }*/
#endif

    int firstSymSamp = get_firstSymSamp(slot_nr, &UE->frame_parms);
    for (int i=0; i<UE->frame_parms.nb_antennas_rx; i++)
      rxp[i] = (void *)&UE->common_vars.rxdata[i][firstSymSamp+
               UE->frame_parms.get_samples_slot_timestamp(slot_nr,&UE->frame_parms,0)];

    for (int i=0; i<UE->frame_parms.nb_antennas_tx; i++)
      txp[i] = (void *)&UE->common_vars.txdata[i][UE->frame_parms.get_samples_slot_timestamp(
               ((slot_nr + DURATION_RX_TO_TX - NR_RX_NB_TH)%nb_slot_frame),&UE->frame_parms,0)];

    int readBlockSize, writeBlockSize;

    if (slot_nr<(nb_slot_frame - 1)) {
      readBlockSize=get_readBlockSize(slot_nr, &UE->frame_parms);
      writeBlockSize=UE->frame_parms.get_samples_per_slot((slot_nr + DURATION_RX_TO_TX - NR_RX_NB_TH) % nb_slot_frame, &UE->frame_parms);
    } else {
      UE->rx_offset_diff = computeSamplesShift(UE);
      readBlockSize=get_readBlockSize(slot_nr, &UE->frame_parms) -
                    UE->rx_offset_diff;
      writeBlockSize=UE->frame_parms.get_samples_per_slot((slot_nr + DURATION_RX_TO_TX - NR_RX_NB_TH) % nb_slot_frame, &UE->frame_parms)- UE->rx_offset_diff;
    }

    AssertFatal(readBlockSize ==
                UE->rfdevice.trx_read_func(&UE->rfdevice,
                                           &timestamp,
                                           rxp,
                                           readBlockSize,
                                           UE->frame_parms.nb_antennas_rx),"");

    if( slot_nr==(nb_slot_frame-1)) {
      // read in first symbol of next frame and adjust for timing drift
      int first_symbols=UE->frame_parms.ofdm_symbol_size+UE->frame_parms.nb_prefix_samples0; // first symbol of every frames

      if ( first_symbols > 0 ) {
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

    curMsg->proc.timestamp_tx = timestamp+
      UE->frame_parms.get_samples_slot_timestamp(slot_nr,&UE->frame_parms,DURATION_RX_TO_TX) 
      - firstSymSamp;

    notifiedFIFO_elt_t *res;

    while (nbSlotProcessing >= NR_RX_NB_TH) {
      res=pullTpool(&nf, &(get_nrUE_params()->Tpool));
      nbSlotProcessing--;
      nr_rxtx_thread_data_t *tmp=(nr_rxtx_thread_data_t *)res->msgData;

      if (tmp->proc.decoded_frame_rx != -1)
        decoded_frame_rx=(((mac->mib->systemFrameNumber.buf[0] >> mac->mib->systemFrameNumber.bits_unused)<<4) | tmp->proc.decoded_frame_rx);
      else
         decoded_frame_rx=-1;

      pushNotifiedFIFO_nothreadSafe(&freeBlocks,res);
    }

    if (decoded_frame_rx>0 && decoded_frame_rx != curMsg->proc.frame_rx)
      LOG_E(PHY,"Decoded frame index (%d) is not compatible with current context (%d), UE should go back to synch mode\n",
            decoded_frame_rx, curMsg->proc.frame_rx);

    // use previous timing_advance value to compute writeTimestamp
    writeTimestamp = timestamp+
      UE->frame_parms.get_samples_slot_timestamp(slot_nr,&UE->frame_parms,DURATION_RX_TO_TX
      - NR_RX_NB_TH) - firstSymSamp - openair0_cfg[0].tx_sample_advance -
      UE->N_TA_offset - timing_advance;

    // but use current UE->timing_advance value to compute writeBlockSize
    if (UE->timing_advance != timing_advance) {
      writeBlockSize -= UE->timing_advance - timing_advance;
      timing_advance = UE->timing_advance;
    }

    int flags = 0;
    int slot_tx_usrp = slot_nr + DURATION_RX_TO_TX - NR_RX_NB_TH;

    if (openair0_cfg[0].duplex_mode == duplex_mode_TDD) {

      uint8_t tdd_period = mac->phy_config.config_req.tdd_table.tdd_period_in_slots;
      int nrofUplinkSlots, nrofUplinkSymbols;
      if (mac->scc) {
        nrofUplinkSlots = mac->scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSlots;
        nrofUplinkSymbols = mac->scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSymbols;
      }
      else {
        nrofUplinkSlots = mac->scc_SIB->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSlots;
        nrofUplinkSymbols = mac->scc_SIB->tdd_UL_DL_ConfigurationCommon->pattern1.nrofUplinkSymbols;
      }
      uint8_t  num_UL_slots = nrofUplinkSlots + (nrofUplinkSymbols != 0);

      uint8_t first_tx_slot = tdd_period - num_UL_slots;

      if (slot_tx_usrp % tdd_period == first_tx_slot)
        flags = 2;
      else if (slot_tx_usrp % tdd_period == first_tx_slot + num_UL_slots - 1)
        flags = 3;
      else if (slot_tx_usrp % tdd_period > first_tx_slot)
        flags = 1;
    } else {
      flags = 1;
    }

    if (flags || IS_SOFTMODEM_RFSIM)
      AssertFatal(writeBlockSize ==
                  UE->rfdevice.trx_write_func(&UE->rfdevice,
                                              writeTimestamp,
                                              txp,
                                              writeBlockSize,
                                              UE->frame_parms.nb_antennas_tx,
                                              flags),"");
    
    for (int i=0; i<UE->frame_parms.nb_antennas_tx; i++)
      memset(txp[i], 0, writeBlockSize);

    nbSlotProcessing++;
    LOG_D(PHY,"Number of slots being processed at the moment: %d\n",nbSlotProcessing);
    pushTpool(&(get_nrUE_params()->Tpool), msgToPush);

  } // while !oai_exit

  return NULL;
}

void init_NR_UE(int nb_inst,
                char* uecap_file,
                char* rrc_config_path) {
  int inst;
  NR_UE_MAC_INST_t *mac_inst;
  NR_UE_RRC_INST_t* rrc_inst;
  
  for (inst=0; inst < nb_inst; inst++) {
    AssertFatal((rrc_inst = nr_l3_init_ue(uecap_file,rrc_config_path)) != NULL, "can not initialize RRC module\n");
    AssertFatal((mac_inst = nr_l2_init_ue(rrc_inst)) != NULL, "can not initialize L2 module\n");
    AssertFatal((mac_inst->if_module = nr_ue_if_module_init(inst)) != NULL, "can not initialize IF module\n");
  }
}

void init_NR_UE_threads(int nb_inst) {
  int inst;

  pthread_t threads[nb_inst];

  for (inst=0; inst < nb_inst; inst++) {
    PHY_VARS_NR_UE *UE = PHY_vars_UE_g[inst][0];

    LOG_I(PHY,"Intializing UE Threads for instance %d (%p,%p)...\n",inst,PHY_vars_UE_g[inst],PHY_vars_UE_g[inst][0]);
    threadCreate(&threads[inst], UE_thread, (void *)UE, "UEthread", -1, OAI_PRIORITY_RT_MAX);
    pthread_t stat_pthread;
    threadCreate(&stat_pthread, nrL1_UE_stats_thread, UE, "L1_UE_stats", -1, OAI_PRIORITY_RT_LOW);
  }
}

/* HACK: this function is needed to compile the UE
 * fix it somehow
 */
int find_dlsch(uint16_t rnti,
                  PHY_VARS_eNB *eNB,
                  find_type_t type)
{
  printf("you cannot read this\n");
  abort();
}

void multicast_link_write_sock(int groupP, char *dataP, uint32_t sizeP) {}
