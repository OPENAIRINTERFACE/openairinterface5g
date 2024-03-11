/*
* Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
* contributor license agreements.  See the NOTICE file distributed with
* this work for additional information regarding copyright ownership.
* The OpenAirInterface Software Alliance licenses this file to You under
* the OAI Public License, Version 1.1  (the "License"); you may not use this file
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

/*! \file fapi/oai-integration/fapi_vnf_p7.c
* \brief OAI FAPI VNF P7 message handler procedures
* \author Ruben S. Silva
* \date 2023
* \version 0.1
* \company OpenAirInterface Software Alliance
* \email: contact@openairinterface.org, rsilva@allbesmart.pt
* \note
* \warning
 */
#ifdef ENABLE_AERIAL
#include "fapi_vnf_p7.h"
#include "nfapi/open-nFAPI/nfapi/src/nfapi_p7.c"

extern RAN_CONTEXT_t RC;
extern UL_RCC_IND_t UL_RCC_INFO;
extern int single_thread_flag;
extern uint16_t sf_ahead;

int aerial_wake_gNB_rxtx(PHY_VARS_gNB *gNB, uint16_t sfn, uint16_t slot)
{
  struct timespec curr_t;
  clock_gettime(CLOCK_MONOTONIC, &curr_t);
  // NFAPI_TRACE(NFAPI_TRACE_INFO, "\n wake_gNB_rxtx before assignment sfn:%d slot:%d TIME
  // %d.%d",sfn,slot,curr_t.tv_sec,curr_t.tv_nsec);
  gNB_L1_proc_t *proc = &gNB->proc;
  gNB_L1_rxtx_proc_t *L1_proc = (slot & 1) ? &proc->L1_proc : &proc->L1_proc_tx;

  NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;
  // NFAPI_TRACE(NFAPI_TRACE_INFO, "%s(eNB:%p, sfn:%d, sf:%d)\n", __FUNCTION__, eNB, sfn, sf);
  // int i;
  struct timespec wait;
  clock_gettime(CLOCK_REALTIME, &wait);
  wait.tv_sec = 0;
  wait.tv_nsec += 5000L;
  // wait.tv_nsec = 0;
  //  wake up TX for subframe n+sf_ahead
  //  lock the TX mutex and make sure the thread is ready
  if (pthread_mutex_timedlock(&L1_proc->mutex, &wait) != 0) {
    LOG_E(PHY, "[gNB] ERROR pthread_mutex_lock for gNB RXTX thread %d (IC %d)\n", L1_proc->slot_rx & 1, L1_proc->instance_cnt);
    exit_fun("error locking mutex_rxtx");
    return (-1);
  }

  {
    static uint16_t old_slot = 0;
    static uint16_t old_sfn = 0;
    proc->slot_rx = old_slot;
    proc->frame_rx = old_sfn;
    // Try to be 1 frame back
    old_slot = slot;
    old_sfn = sfn;
    // NFAPI_TRACE(NFAPI_TRACE_INFO, "\n wake_gNB_rxtx after assignment sfn:%d slot:%d",proc->frame_rx,proc->slot_rx);
    if (old_slot == 0 && old_sfn % 100 == 0)
      LOG_W(PHY,
            "[gNB] sfn/slot:%d%d old_sfn/slot:%d%d proc[rx:%d%d]\n",
            sfn,
            slot,
            old_sfn,
            old_slot,
            proc->frame_rx,
            proc->slot_rx);
  }

  ++L1_proc->instance_cnt;
  // LOG_D( PHY,"[VNF-subframe_ind] sfn/sf:%d:%d proc[frame_rx:%d subframe_rx:%d] L1_proc->instance_cnt_rxtx:%d \n", sfn, sf,
  // proc->frame_rx, proc->subframe_rx, L1_proc->instance_cnt_rxtx);
  //  We have just received and processed the common part of a subframe, say n.
  //  TS_rx is the last received timestamp (start of 1st slot), TS_tx is the desired
  //  transmitted timestamp of the next TX slot (first).
  //  The last (TS_rx mod samples_per_frame) was n*samples_per_tti,
  //  we want to generate subframe (n+N), so TS_tx = TX_rx+N*samples_per_tti,
  //  and proc->subframe_tx = proc->subframe_rx+sf_ahead
  L1_proc->timestamp_tx = proc->timestamp_rx + (gNB->if_inst->sl_ahead * fp->samples_per_subframe);
  L1_proc->frame_rx = proc->frame_rx;
  L1_proc->slot_rx = proc->slot_rx;
  L1_proc->frame_tx = (L1_proc->slot_rx > (19 - gNB->if_inst->sl_ahead)) ? (L1_proc->frame_rx + 1) & 1023 : L1_proc->frame_rx;
  L1_proc->slot_tx = (L1_proc->slot_rx + gNB->if_inst->sl_ahead) % 20;

  // LOG_I(PHY, "sfn/sf:%d%d proc[rx:%d%d] rx:%d%d] About to wake rxtx thread\n\n", sfn, slot, proc->frame_rx, proc->slot_rx,
  // L1_proc->frame_rx, L1_proc->slot_rx); NFAPI_TRACE(NFAPI_TRACE_INFO, "\nEntering wake_gNB_rxtx sfn %d slot
  // %d\n",L1_proc->frame_rx,L1_proc->slot_rx);
  //  the thread can now be woken up
  if (pthread_cond_signal(&L1_proc->cond) != 0) {
    LOG_E(PHY, "[gNB] ERROR pthread_cond_signal for gNB RXn-TXnp4 thread\n");
    exit_fun("ERROR pthread_clond_signal");
    return (-1);
  }

  // LOG_D(PHY,"%s() About to attempt pthread_mutex_unlock\n", __FUNCTION__);
  pthread_mutex_unlock(&L1_proc->mutex);
  // LOG_D(PHY,"%s() UNLOCKED pthread_mutex_unlock\n", __FUNCTION__);
  return (0);
}

int aerialwake_eNB_rxtx(PHY_VARS_eNB *eNB, uint16_t sfn, uint16_t sf)
{
  L1_proc_t *proc = &eNB->proc;
  L1_rxtx_proc_t *L1_proc = (sf & 1) ? &proc->L1_proc : &proc->L1_proc_tx;
  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  // NFAPI_TRACE(NFAPI_TRACE_INFO, "%s(eNB:%p, sfn:%d, sf:%d)\n", __FUNCTION__, eNB, sfn, sf);
  // int i;
  struct timespec wait;
  wait.tv_sec = 0;
  wait.tv_nsec = 5000000L;

  // wake up TX for subframe n+sf_ahead
  // lock the TX mutex and make sure the thread is ready
  // if (pthread_mutex_timedlock(&L1_proc->mutex,&wait) != 0) {
  //  LOG_E( PHY, "[eNB] ERROR pthread_mutex_lock for eNB RXTX thread %d (IC %d)\n", L1_proc->subframe_rx&1,L1_proc->instance_cnt );
  //  exit_fun( "error locking mutex_rxtx" );
  //  return(-1);
  //}

  {
    static uint16_t old_sf = 0;
    static uint16_t old_sfn = 0;
    proc->subframe_rx = old_sf;
    proc->frame_rx = old_sfn;
    // Try to be 1 frame back
    old_sf = sf;
    old_sfn = sfn;

    if (old_sf == 0 && old_sfn % 100 == 0)
      LOG_D(PHY, "[eNB] sfn/sf:%d%d old_sfn/sf:%d%d proc[rx:%d%d]\n", sfn, sf, old_sfn, old_sf, proc->frame_rx, proc->subframe_rx);
  }
  // wake up TX for subframe n+sf_ahead
  // lock the TX mutex and make sure the thread is ready
  if (pthread_mutex_timedlock(&L1_proc->mutex, &wait) != 0) {
    LOG_E(PHY, "[eNB] ERROR pthread_mutex_lock for eNB RXTX thread %d (IC %d)\n", L1_proc->subframe_rx & 1, L1_proc->instance_cnt);
    // exit_fun( "error locking mutex_rxtx" );
    return (-1);
  }
  static int busy_log_cnt = 0;
  if (L1_proc->instance_cnt < 0) {
    ++L1_proc->instance_cnt;
    if (busy_log_cnt != 0) {
      LOG_E(MAC,
            "RCC singal to rxtx frame %d subframe %d busy end %d (frame %d subframe %d)\n",
            L1_proc->frame_rx,
            L1_proc->subframe_rx,
            busy_log_cnt,
            proc->frame_rx,
            proc->subframe_rx);
    }
    busy_log_cnt = 0;
  } else {
    if (busy_log_cnt == 0) {
      LOG_E(MAC,
            "RCC singal to rxtx frame %d subframe %d busy %d (frame %d subframe %d)\n",
            L1_proc->frame_rx,
            L1_proc->subframe_rx,
            L1_proc->instance_cnt,
            proc->frame_rx,
            proc->subframe_rx);
    }
    pthread_mutex_unlock(&L1_proc->mutex);
    busy_log_cnt++;
    return (0);
  }

  pthread_mutex_unlock(&L1_proc->mutex);

  // LOG_D( PHY,"[VNF-subframe_ind] sfn/sf:%d:%d proc[frame_rx:%d subframe_rx:%d] L1_proc->instance_cnt_rxtx:%d \n", sfn, sf,
  // proc->frame_rx, proc->subframe_rx, L1_proc->instance_cnt_rxtx);
  //  We have just received and processed the common part of a subframe, say n.
  //  TS_rx is the last received timestamp (start of 1st slot), TS_tx is the desired
  //  transmitted timestamp of the next TX slot (first).
  //  The last (TS_rx mod samples_per_frame) was n*samples_per_tti,
  //  we want to generate subframe (n+N), so TS_tx = TX_rx+N*samples_per_tti,
  //  and proc->subframe_tx = proc->subframe_rx+sf_ahead
  L1_proc->timestamp_tx = proc->timestamp_rx + (sf_ahead * fp->samples_per_tti);
  L1_proc->frame_rx = proc->frame_rx;
  L1_proc->subframe_rx = proc->subframe_rx;
  L1_proc->frame_tx = (L1_proc->subframe_rx > (9 - sf_ahead)) ? (L1_proc->frame_rx + 1) & 1023 : L1_proc->frame_rx;
  L1_proc->subframe_tx = (L1_proc->subframe_rx + sf_ahead) % 10;

  // LOG_D(PHY, "sfn/sf:%d%d proc[rx:%d%d] L1_proc[instance_cnt_rxtx:%d rx:%d%d] About to wake rxtx thread\n\n", sfn, sf,
  // proc->frame_rx, proc->subframe_rx, L1_proc->instance_cnt_rxtx, L1_proc->frame_rx, L1_proc->subframe_rx);

  // the thread can now be woken up
  if (pthread_cond_signal(&L1_proc->cond) != 0) {
    LOG_E(PHY, "[eNB] ERROR pthread_cond_signal for eNB RXn-TXnp4 thread\n");
    exit_fun("ERROR pthread_cond_signal");
    return (-1);
  }

  return (0);
}

extern pthread_cond_t nfapi_sync_cond;
extern pthread_mutex_t nfapi_sync_mutex;
extern int nfapi_sync_var;

int aerial_phy_sync_indication(struct nfapi_vnf_p7_config *config, uint8_t sync)
{
  // NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] SYNC %s\n", sync==1 ? "ACHIEVED" : "LOST");

  if (sync == 1 && nfapi_sync_var != 0) {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] Signal to OAI main code that it can go\n");
    pthread_mutex_lock(&nfapi_sync_mutex);
    nfapi_sync_var = 0;
    pthread_cond_broadcast(&nfapi_sync_cond);
    pthread_mutex_unlock(&nfapi_sync_mutex);
  }

  return (0);
}

int aerial_phy_slot_indication(struct nfapi_vnf_p7_config *config, uint16_t phy_id, uint16_t sfn, uint16_t slot)
{
  static uint8_t first_time = 1;

  if (first_time) {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] slot indication %d\n", NFAPI_SFNSLOT2DEC(sfn, slot));
    first_time = 0;
  }

  if (RC.gNB && RC.gNB[0]->configured) {
    // uint16_t sfn = NFAPI_SFNSF2SFN(sfn_sf);
    // uint16_t sf = NFAPI_SFNSF2SF(sfn_sf);
    LOG_D(PHY, "[VNF] slot indication sfn:%d slot:%d\n", sfn, slot);
    aerial_wake_gNB_rxtx(RC.gNB[0], sfn, slot); // DONE: find NR equivalent
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] %s() RC.gNB:%p\n", __FUNCTION__, RC.gNB);

    if (RC.gNB)
      NFAPI_TRACE(NFAPI_TRACE_INFO, "RC.gNB[0]->configured:%d\n", RC.gNB[0]->configured);
  }

  return 0;
}

int aerial_phy_harq_indication(struct nfapi_vnf_p7_config *config, nfapi_harq_indication_t *ind)
{
  struct PHY_VARS_eNB_s *eNB = RC.eNB[0][0];
  LOG_D(MAC,
        "%s() NFAPI SFN/SF:%d number_of_harqs:%u\n",
        __FUNCTION__,
        NFAPI_SFNSF2DEC(ind->sfn_sf),
        ind->harq_indication_body.number_of_harqs);
  AssertFatal(pthread_mutex_lock(&eNB->UL_INFO_mutex) == 0, "Mutex lock failed");
  if (NFAPI_MODE == NFAPI_MODE_VNF) {
    int8_t index = NFAPI_SFNSF2SF(ind->sfn_sf);

    UL_RCC_INFO.harq_ind[index] = *ind;

    assert(ind->harq_indication_body.number_of_harqs <= NFAPI_HARQ_IND_MAX_PDU);
    if (ind->harq_indication_body.number_of_harqs > 0) {
      UL_RCC_INFO.harq_ind[index].harq_indication_body.harq_pdu_list =
          malloc(sizeof(nfapi_harq_indication_pdu_t) * NFAPI_HARQ_IND_MAX_PDU);
    }
    for (int i = 0; i < ind->harq_indication_body.number_of_harqs; i++) {
      memcpy(&UL_RCC_INFO.harq_ind[index].harq_indication_body.harq_pdu_list[i],
             &ind->harq_indication_body.harq_pdu_list[i],
             sizeof(nfapi_harq_indication_pdu_t));
    }
  } else {
    eNB->UL_INFO.harq_ind = *ind;
    eNB->UL_INFO.harq_ind.harq_indication_body.harq_pdu_list = eNB->harq_pdu_list;

    assert(ind->harq_indication_body.number_of_harqs <= NFAPI_HARQ_IND_MAX_PDU);
    for (int i = 0; i < ind->harq_indication_body.number_of_harqs; i++) {
      memcpy(&eNB->UL_INFO.harq_ind.harq_indication_body.harq_pdu_list[i],
             &ind->harq_indication_body.harq_pdu_list[i],
             sizeof(eNB->UL_INFO.harq_ind.harq_indication_body.harq_pdu_list[i]));
    }
  }
  AssertFatal(pthread_mutex_unlock(&eNB->UL_INFO_mutex) == 0, "Mutex unlock failed");
  // vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
  // mac_harq_ind(p7_vnf->mac, ind);
  return 1;
}

int aerial_phy_nr_crc_indication(nfapi_nr_crc_indication_t *ind)
{
  if (NFAPI_MODE == NFAPI_MODE_AERIAL) {
    nfapi_nr_crc_indication_t *crc_ind = CALLOC(1, sizeof(*crc_ind));
    crc_ind->header.message_id = ind->header.message_id;
    crc_ind->number_crcs = ind->number_crcs;
    crc_ind->sfn = ind->sfn;
    crc_ind->slot = ind->slot;
    if (ind->number_crcs > 0) {
      crc_ind->crc_list = CALLOC(NFAPI_NR_CRC_IND_MAX_PDU, sizeof(nfapi_nr_crc_t));
      AssertFatal(crc_ind->crc_list != NULL, "Memory not allocated for crc_ind->crc_list in phy_nr_crc_indication.");
    }
    for (int j = 0; j < ind->number_crcs; j++) {
      crc_ind->crc_list[j].handle = ind->crc_list[j].handle;
      crc_ind->crc_list[j].rnti = ind->crc_list[j].rnti;
      crc_ind->crc_list[j].harq_id = ind->crc_list[j].harq_id;
      crc_ind->crc_list[j].tb_crc_status = ind->crc_list[j].tb_crc_status;
      crc_ind->crc_list[j].num_cb = ind->crc_list[j].num_cb;
      crc_ind->crc_list[j].cb_crc_status = ind->crc_list[j].cb_crc_status;
      crc_ind->crc_list[j].ul_cqi = ind->crc_list[j].ul_cqi;
      crc_ind->crc_list[j].timing_advance = ind->crc_list[j].timing_advance;
      crc_ind->crc_list[j].rssi = ind->crc_list[j].rssi;
      if (crc_ind->crc_list[j].tb_crc_status != 0) {
        LOG_D(NR_MAC,
              "Received crc_ind.harq_id %d status %d for index %d SFN SLot %u %u with rnti %04x\n",
              crc_ind->crc_list[j].harq_id,
              crc_ind->crc_list[j].tb_crc_status,
              j,
              crc_ind->sfn,
              crc_ind->slot,
              crc_ind->crc_list[j].rnti);
      }
    }
    if (!put_queue(&gnb_crc_ind_queue, crc_ind)) {
      LOG_E(NR_MAC, "Put_queue failed for crc_ind\n");
      free(crc_ind->crc_list);
      free(crc_ind);
    }
  } else {
    LOG_E(NR_MAC, "NFAPI_MODE = %d not NFAPI_MODE_AERIAL(3)\n", nfapi_getmode());
  }
  return 1;
}

int aerial_phy_nr_rx_data_indication(nfapi_nr_rx_data_indication_t *ind)
{
  if (NFAPI_MODE == NFAPI_MODE_AERIAL) {
    nfapi_nr_rx_data_indication_t *rx_ind = CALLOC(1, sizeof(*rx_ind));
    rx_ind->header.message_id = ind->header.message_id;
    rx_ind->sfn = ind->sfn;
    rx_ind->slot = ind->slot;
    rx_ind->number_of_pdus = ind->number_of_pdus;

    if (ind->number_of_pdus > 0) {
      rx_ind->pdu_list = CALLOC(NFAPI_NR_RX_DATA_IND_MAX_PDU, sizeof(nfapi_nr_rx_data_pdu_t));
      AssertFatal(rx_ind->pdu_list != NULL, "Memory not allocated for rx_ind->pdu_list in phy_nr_rx_data_indication.");
    }
    for (int j = 0; j < ind->number_of_pdus; j++) {
      rx_ind->pdu_list[j].handle = ind->pdu_list[j].handle;
      rx_ind->pdu_list[j].rnti = ind->pdu_list[j].rnti;
      rx_ind->pdu_list[j].harq_id = ind->pdu_list[j].harq_id;
      rx_ind->pdu_list[j].pdu_length = ind->pdu_list[j].pdu_length;
      rx_ind->pdu_list[j].ul_cqi = ind->pdu_list[j].ul_cqi;
      rx_ind->pdu_list[j].timing_advance = ind->pdu_list[j].timing_advance;
      rx_ind->pdu_list[j].rssi = ind->pdu_list[j].rssi;
      rx_ind->pdu_list[j].pdu = ind->pdu_list[j].pdu;
      LOG_D(NR_MAC,
            "(%d.%d) Handle %d for index %d, RNTI, %04x, HARQID %d\n",
            ind->sfn,
            ind->slot,
            ind->pdu_list[j].handle,
            j,
            ind->pdu_list[j].rnti,
            ind->pdu_list[j].harq_id);
    }
    if (!put_queue(&gnb_rx_ind_queue, rx_ind)) {
      LOG_E(NR_MAC, "Put_queue failed for rx_ind\n");
      free(rx_ind->pdu_list);
      free(rx_ind);
    }
  } else {
    LOG_E(NR_MAC, "NFAPI_MODE = %d not NFAPI_MODE_AERIAL(3)\n", nfapi_getmode());
  }
  return 1;
}

int aerial_phy_nr_rach_indication(nfapi_nr_rach_indication_t *ind)
{
  if (NFAPI_MODE == NFAPI_MODE_AERIAL) {
    nfapi_nr_rach_indication_t *rach_ind = CALLOC(1, sizeof(*rach_ind));
    rach_ind->header.message_id = ind->header.message_id;
    rach_ind->sfn = ind->sfn;
    rach_ind->slot = ind->slot;
    rach_ind->number_of_pdus = ind->number_of_pdus;
    rach_ind->pdu_list = CALLOC(rach_ind->number_of_pdus, sizeof(*rach_ind->pdu_list));
    AssertFatal(rach_ind->pdu_list != NULL, "Memory not allocated for rach_ind->pdu_list in phy_nr_rach_indication.");
    for (int i = 0; i < ind->number_of_pdus; i++) {
      rach_ind->pdu_list[i].phy_cell_id = ind->pdu_list[i].phy_cell_id;
      rach_ind->pdu_list[i].symbol_index = ind->pdu_list[i].symbol_index;
      rach_ind->pdu_list[i].slot_index = ind->pdu_list[i].slot_index;
      rach_ind->pdu_list[i].freq_index = ind->pdu_list[i].freq_index;
      rach_ind->pdu_list[i].avg_rssi = ind->pdu_list[i].avg_rssi;
      rach_ind->pdu_list[i].avg_snr = ind->pdu_list[i].avg_snr;
      rach_ind->pdu_list[i].num_preamble = ind->pdu_list[i].num_preamble;
      rach_ind->pdu_list[i].preamble_list = CALLOC(ind->pdu_list[i].num_preamble, sizeof(nfapi_nr_prach_indication_preamble_t));
      AssertFatal(rach_ind->pdu_list[i].preamble_list != NULL,
                  "Memory not allocated for rach_ind->pdu_list[i].preamble_list  in phy_nr_rach_indication.");
      for (int j = 0; j < ind->pdu_list[i].num_preamble; j++) {
        rach_ind->pdu_list[i].preamble_list[j].preamble_index = ind->pdu_list[i].preamble_list[j].preamble_index;
        rach_ind->pdu_list[i].preamble_list[j].timing_advance = ind->pdu_list[i].preamble_list[j].timing_advance;
        rach_ind->pdu_list[i].preamble_list[j].preamble_pwr = ind->pdu_list[i].preamble_list[j].preamble_pwr;
      }
    }
    if (!put_queue(&gnb_rach_ind_queue, rach_ind)) {
      LOG_E(NR_MAC, "Put_queue failed for rach_ind\n");
      for (int i = 0; i < ind->number_of_pdus; i++) {
        free(rach_ind->pdu_list[i].preamble_list);
      }
      free(rach_ind->pdu_list);
      free(rach_ind);
    } else {
      LOG_I(NR_MAC, "RACH.indication put_queue successfull\n");
    }
  } else {
    LOG_E(NR_MAC, "NFAPI_MODE = %d not NFAPI_MODE_AERIAL(3)\n", nfapi_getmode());
  }
  return 1;
}

int aerial_phy_nr_uci_indication(nfapi_nr_uci_indication_t *ind)
{
  if (NFAPI_MODE == NFAPI_MODE_AERIAL) {
    nfapi_nr_uci_indication_t *uci_ind = CALLOC(1, sizeof(*uci_ind));
    AssertFatal(uci_ind != NULL, "Memory not allocated for uci_ind in phy_nr_uci_indication.");
    *uci_ind = *ind;

    uci_ind->uci_list = CALLOC(NFAPI_NR_UCI_IND_MAX_PDU, sizeof(nfapi_nr_uci_t));
    AssertFatal(uci_ind->uci_list != NULL, "Memory not allocated for uci_ind->uci_list in phy_nr_uci_indication.");
    for (int i = 0; i < ind->num_ucis; i++) {
      uci_ind->uci_list[i] = ind->uci_list[i];

      switch (uci_ind->uci_list[i].pdu_type) {
        case NFAPI_NR_UCI_PUSCH_PDU_TYPE:
          LOG_E(MAC, "%s(): unhandled NFAPI_NR_UCI_PUSCH_PDU_TYPE\n", __func__);
          break;

        case NFAPI_NR_UCI_FORMAT_0_1_PDU_TYPE: {
//          nfapi_nr_uci_pucch_pdu_format_0_1_t *uci_ind_pdu = &uci_ind->uci_list[i].pucch_pdu_format_0_1;
//          nfapi_nr_uci_pucch_pdu_format_0_1_t *ind_pdu = &ind->uci_list[i].pucch_pdu_format_0_1;
//          if (ind_pdu->sr) {
//            uci_ind_pdu->sr = CALLOC(1, sizeof(*uci_ind_pdu->sr));
//            AssertFatal(uci_ind_pdu->sr != NULL, "Memory not allocated for uci_ind_pdu->harq in phy_nr_uci_indication.");
//            *uci_ind_pdu->sr = *ind_pdu->sr;
//          }
//          if (ind_pdu->harq) {
//            uci_ind_pdu->harq = CALLOC(1, sizeof(*uci_ind_pdu->harq));
//            AssertFatal(uci_ind_pdu->harq != NULL, "Memory not allocated for uci_ind_pdu->harq in phy_nr_uci_indication.");
//
//            *uci_ind_pdu->harq = *ind_pdu->harq;
//            uci_ind_pdu->harq->harq_list = CALLOC(uci_ind_pdu->harq->num_harq, sizeof(*uci_ind_pdu->harq->harq_list));
//            AssertFatal(uci_ind_pdu->harq->harq_list != NULL,
//                        "Memory not allocated for uci_ind_pdu->harq->harq_list in phy_nr_uci_indication.");
//            for (int j = 0; j < uci_ind_pdu->harq->num_harq; j++)
//              uci_ind_pdu->harq->harq_list[j].harq_value = ind_pdu->harq->harq_list[j].harq_value;
//          }
          break;
        }

        case NFAPI_NR_UCI_FORMAT_2_3_4_PDU_TYPE: {
          nfapi_nr_uci_pucch_pdu_format_2_3_4_t *uci_ind_pdu = &uci_ind->uci_list[i].pucch_pdu_format_2_3_4;
          nfapi_nr_uci_pucch_pdu_format_2_3_4_t *ind_pdu = &ind->uci_list[i].pucch_pdu_format_2_3_4;
          *uci_ind_pdu = *ind_pdu;
          if (ind_pdu->harq.harq_payload) {
            uci_ind_pdu->harq.harq_payload = CALLOC(1, sizeof(*uci_ind_pdu->harq.harq_payload));
            AssertFatal(uci_ind_pdu->harq.harq_payload != NULL,
                        "Memory not allocated for uci_ind_pdu->harq.harq_payload in phy_nr_uci_indication.");
            *uci_ind_pdu->harq.harq_payload = *ind_pdu->harq.harq_payload;
          }
          if (ind_pdu->sr.sr_payload) {
            uci_ind_pdu->sr.sr_payload = CALLOC(1, sizeof(*uci_ind_pdu->sr.sr_payload));
            AssertFatal(uci_ind_pdu->sr.sr_payload != NULL,
                        "Memory not allocated for uci_ind_pdu->sr.sr_payload in phy_nr_uci_indication.");
            *uci_ind_pdu->sr.sr_payload = *ind_pdu->sr.sr_payload;
          }
          if (ind_pdu->csi_part1.csi_part1_payload) {
            uci_ind_pdu->csi_part1.csi_part1_payload = CALLOC(1, sizeof(*uci_ind_pdu->csi_part1.csi_part1_payload));
            AssertFatal(uci_ind_pdu->csi_part1.csi_part1_payload != NULL,
                        "Memory not allocated for uci_ind_pdu->csi_part1.csi_part1_payload in phy_nr_uci_indication.");
            *uci_ind_pdu->csi_part1.csi_part1_payload = *ind_pdu->csi_part1.csi_part1_payload;
          }
          if (ind_pdu->csi_part2.csi_part2_payload) {
            uci_ind_pdu->csi_part2.csi_part2_payload = CALLOC(1, sizeof(*uci_ind_pdu->csi_part2.csi_part2_payload));
            AssertFatal(uci_ind_pdu->csi_part2.csi_part2_payload != NULL,
                        "Memory not allocated for uci_ind_pdu->csi_part2.csi_part2_payload in phy_nr_uci_indication.");
            *uci_ind_pdu->csi_part2.csi_part2_payload = *ind_pdu->csi_part2.csi_part2_payload;
          }
          break;
        }
      }
    }

    if (!put_queue(&gnb_uci_ind_queue, uci_ind)) {
      LOG_E(NR_MAC, "Put_queue failed for uci_ind\n");
      for (int i = 0; i < ind->num_ucis; i++) {
        if (uci_ind->uci_list[i].pdu_type == NFAPI_NR_UCI_FORMAT_0_1_PDU_TYPE) {
//          if (uci_ind->uci_list[i].pucch_pdu_format_0_1.harq) {
//            free(uci_ind->uci_list[i].pucch_pdu_format_0_1.harq->harq_list);
//            uci_ind->uci_list[i].pucch_pdu_format_0_1.harq->harq_list = NULL;
//            free(uci_ind->uci_list[i].pucch_pdu_format_0_1.harq);
//            uci_ind->uci_list[i].pucch_pdu_format_0_1.harq = NULL;
//          }
//          if (uci_ind->uci_list[i].pucch_pdu_format_0_1.sr) {
//            free(uci_ind->uci_list[i].pucch_pdu_format_0_1.sr);
//            uci_ind->uci_list[i].pucch_pdu_format_0_1.sr = NULL;
//          }
        }
        if (uci_ind->uci_list[i].pdu_type == NFAPI_NR_UCI_FORMAT_2_3_4_PDU_TYPE) {
          free(uci_ind->uci_list[i].pucch_pdu_format_2_3_4.harq.harq_payload);
          free(uci_ind->uci_list[i].pucch_pdu_format_2_3_4.csi_part1.csi_part1_payload);
          free(uci_ind->uci_list[i].pucch_pdu_format_2_3_4.csi_part2.csi_part2_payload);
        }
      }
      free(uci_ind->uci_list);
      uci_ind->uci_list = NULL;
      free(uci_ind);
      uci_ind = NULL;
    }
  } else {
    LOG_E(NR_MAC, "NFAPI_MODE = %d not NFAPI_MODE_AERIAL(3)\n", nfapi_getmode());
  }
  return 1;
}

int aerial_phy_srs_indication(struct nfapi_vnf_p7_config *config, nfapi_srs_indication_t *ind)
{
  // vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
  // mac_srs_ind(p7_vnf->mac, ind);
  return 1;
}

int aerial_phy_sr_indication(struct nfapi_vnf_p7_config *config, nfapi_sr_indication_t *ind)
{
  struct PHY_VARS_eNB_s *eNB = RC.eNB[0][0];
  LOG_D(MAC, "%s() NFAPI SFN/SF:%d srs:%d\n", __FUNCTION__, NFAPI_SFNSF2DEC(ind->sfn_sf), ind->sr_indication_body.number_of_srs);
  AssertFatal(pthread_mutex_lock(&eNB->UL_INFO_mutex) == 0, "Mutex lock failed");
  if (NFAPI_MODE == NFAPI_MODE_VNF) {
    int8_t index = NFAPI_SFNSF2SF(ind->sfn_sf);

    UL_RCC_INFO.sr_ind[index] = *ind;
    LOG_D(MAC,
          "%s() UL_INFO[%d].sr_ind.sr_indication_body.number_of_srs:%d\n",
          __FUNCTION__,
          index,
          eNB->UL_INFO.sr_ind.sr_indication_body.number_of_srs);
    if (ind->sr_indication_body.number_of_srs > 0) {
      assert(ind->sr_indication_body.number_of_srs <= NFAPI_SR_IND_MAX_PDU);
      UL_RCC_INFO.sr_ind[index].sr_indication_body.sr_pdu_list = malloc(sizeof(nfapi_sr_indication_pdu_t) * NFAPI_SR_IND_MAX_PDU);
    }

    assert(ind->sr_indication_body.number_of_srs <= NFAPI_SR_IND_MAX_PDU);
    for (int i = 0; i < ind->sr_indication_body.number_of_srs; i++) {
      nfapi_sr_indication_pdu_t *dest_pdu = &UL_RCC_INFO.sr_ind[index].sr_indication_body.sr_pdu_list[i];
      nfapi_sr_indication_pdu_t *src_pdu = &ind->sr_indication_body.sr_pdu_list[i];

      LOG_D(MAC,
            "SR_IND[PDU:%d %d][rnti:%x cqi:%d channel:%d]\n",
            index,
            i,
            src_pdu->rx_ue_information.rnti,
            src_pdu->ul_cqi_information.ul_cqi,
            src_pdu->ul_cqi_information.channel);

      memcpy(dest_pdu, src_pdu, sizeof(*src_pdu));
    }
  } else {
    nfapi_sr_indication_t *dest_ind = &eNB->UL_INFO.sr_ind;
    nfapi_sr_indication_pdu_t *dest_pdu_list = eNB->sr_pdu_list;
    *dest_ind = *ind;
    dest_ind->sr_indication_body.sr_pdu_list = dest_pdu_list;
    LOG_D(MAC,
          "%s() eNB->UL_INFO.sr_ind.sr_indication_body.number_of_srs:%d\n",
          __FUNCTION__,
          eNB->UL_INFO.sr_ind.sr_indication_body.number_of_srs);

    assert(eNB->UL_INFO.sr_ind.sr_indication_body.number_of_srs <= NFAPI_SR_IND_MAX_PDU);
    for (int i = 0; i < eNB->UL_INFO.sr_ind.sr_indication_body.number_of_srs; i++) {
      nfapi_sr_indication_pdu_t *dest_pdu = &dest_ind->sr_indication_body.sr_pdu_list[i];
      nfapi_sr_indication_pdu_t *src_pdu = &ind->sr_indication_body.sr_pdu_list[i];
      LOG_D(MAC,
            "SR_IND[PDU:%d][rnti:%x cqi:%d channel:%d]\n",
            i,
            src_pdu->rx_ue_information.rnti,
            src_pdu->ul_cqi_information.ul_cqi,
            src_pdu->ul_cqi_information.channel);
      memcpy(dest_pdu, src_pdu, sizeof(*src_pdu));
    }
  }
  AssertFatal(pthread_mutex_unlock(&eNB->UL_INFO_mutex) == 0, "Mutex unlock failed");
  // vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
  // mac_sr_ind(p7_vnf->mac, ind);
  return 1;
}

static bool aerial_is_ue_same(uint16_t ue_id_1, uint16_t ue_id_2)
{
  return (ue_id_1 == ue_id_2);
}

static void aerial_analyze_cqi_pdus_for_duplicates(nfapi_cqi_indication_t *ind)
{
  uint16_t num_cqis = ind->cqi_indication_body.number_of_cqis;
  assert(num_cqis <= NFAPI_CQI_IND_MAX_PDU);
  for (int i = 0; i < num_cqis; i++) {
    nfapi_cqi_indication_pdu_t *src_pdu = &ind->cqi_indication_body.cqi_pdu_list[i];

    LOG_I(MAC,
          "CQI_IND[PDU:%d][rnti:%x cqi:%d channel:%d]\n",
          i,
          src_pdu->rx_ue_information.rnti,
          src_pdu->ul_cqi_information.ul_cqi,
          src_pdu->ul_cqi_information.channel);

    for (int j = i + 1; j < num_cqis; j++) {
      uint16_t rnti_i = ind->cqi_indication_body.cqi_pdu_list[i].rx_ue_information.rnti;
      uint16_t rnti_j = ind->cqi_indication_body.cqi_pdu_list[j].rx_ue_information.rnti;
      if (aerial_is_ue_same(rnti_i, rnti_j)) {
        LOG_E(MAC, "Problem, two cqis received from a single UE for rnti %x\n", rnti_i);
        // abort(); This will be fixed in merge request which handles multiple CQIs.
      }
    }
  }
}

int aerial_phy_cqi_indication(struct nfapi_vnf_p7_config *config, nfapi_cqi_indication_t *ind)
{
  // vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
  // mac_cqi_ind(p7_vnf->mac, ind);
  struct PHY_VARS_eNB_s *eNB = RC.eNB[0][0];
  LOG_D(MAC,
        "%s() NFAPI SFN/SF:%d number_of_cqis:%u\n",
        __FUNCTION__,
        NFAPI_SFNSF2DEC(ind->sfn_sf),
        ind->cqi_indication_body.number_of_cqis);
  AssertFatal(pthread_mutex_lock(&eNB->UL_INFO_mutex) == 0, "Mutex lock failed");
  if (NFAPI_MODE == NFAPI_MODE_VNF) {
    int8_t index = NFAPI_SFNSF2SF(ind->sfn_sf);

    UL_RCC_INFO.cqi_ind[index] = *ind;
    assert(ind->cqi_indication_body.number_of_cqis <= NFAPI_CQI_IND_MAX_PDU);
    if (ind->cqi_indication_body.number_of_cqis > 0) {
      UL_RCC_INFO.cqi_ind[index].cqi_indication_body.cqi_pdu_list =
          malloc(sizeof(nfapi_cqi_indication_pdu_t) * NFAPI_CQI_IND_MAX_PDU);
      UL_RCC_INFO.cqi_ind[index].cqi_indication_body.cqi_raw_pdu_list =
          malloc(sizeof(nfapi_cqi_indication_raw_pdu_t) * NFAPI_CQI_IND_MAX_PDU);
    }

    aerial_analyze_cqi_pdus_for_duplicates(ind);

    assert(ind->cqi_indication_body.number_of_cqis <= NFAPI_CQI_IND_MAX_PDU);
    for (int i = 0; i < ind->cqi_indication_body.number_of_cqis; i++) {
      nfapi_cqi_indication_pdu_t *src_pdu = &ind->cqi_indication_body.cqi_pdu_list[i];
      LOG_D(MAC,
            "SR_IND[PDU:%d][rnti:%x cqi:%d channel:%d]\n",
            i,
            src_pdu->rx_ue_information.rnti,
            src_pdu->ul_cqi_information.ul_cqi,
            src_pdu->ul_cqi_information.channel);
      memcpy(&UL_RCC_INFO.cqi_ind[index].cqi_indication_body.cqi_pdu_list[i], src_pdu, sizeof(nfapi_cqi_indication_pdu_t));

      memcpy(&UL_RCC_INFO.cqi_ind[index].cqi_indication_body.cqi_raw_pdu_list[i],
             &ind->cqi_indication_body.cqi_raw_pdu_list[i],
             sizeof(nfapi_cqi_indication_raw_pdu_t));
    }
  } else {
    nfapi_cqi_indication_t *dest_ind = &eNB->UL_INFO.cqi_ind;
    *dest_ind = *ind;
    dest_ind->cqi_indication_body.cqi_pdu_list = ind->cqi_indication_body.cqi_pdu_list;
    dest_ind->cqi_indication_body.cqi_raw_pdu_list = ind->cqi_indication_body.cqi_raw_pdu_list;
    assert(ind->cqi_indication_body.number_of_cqis <= NFAPI_CQI_IND_MAX_PDU);
    for (int i = 0; i < ind->cqi_indication_body.number_of_cqis; i++) {
      nfapi_cqi_indication_pdu_t *src_pdu = &ind->cqi_indication_body.cqi_pdu_list[i];
      LOG_D(MAC,
            "CQI_IND[PDU:%d][rnti:%x cqi:%d channel:%d]\n",
            i,
            src_pdu->rx_ue_information.rnti,
            src_pdu->ul_cqi_information.ul_cqi,
            src_pdu->ul_cqi_information.channel);
      memcpy(&dest_ind->cqi_indication_body.cqi_pdu_list[i], src_pdu, sizeof(nfapi_cqi_indication_pdu_t));
      memcpy(&dest_ind->cqi_indication_body.cqi_raw_pdu_list[i],
             &ind->cqi_indication_body.cqi_raw_pdu_list[i],
             sizeof(nfapi_cqi_indication_raw_pdu_t));
    }
  }
  AssertFatal(pthread_mutex_unlock(&eNB->UL_INFO_mutex) == 0, "Mutex unlock failed");
  return 1;
}

int aerial_phy_lbt_dl_indication(struct nfapi_vnf_p7_config *config, nfapi_lbt_dl_indication_t *ind)
{
  // vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
  // mac_lbt_dl_ind(p7_vnf->mac, ind);
  return 1;
}

int aerial_phy_nb_harq_indication(struct nfapi_vnf_p7_config *config, nfapi_nb_harq_indication_t *ind)
{
  // vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
  // mac_nb_harq_ind(p7_vnf->mac, ind);
  return 1;
}

int aerial_phy_nrach_indication(struct nfapi_vnf_p7_config *config, nfapi_nrach_indication_t *ind)
{
  // vnf_p7_info* p7_vnf = (vnf_p7_info*)(config->user_data);
  // mac_nrach_ind(p7_vnf->mac, ind);
  return 1;
}


NR_Sched_Rsp_t g_sched_resp;
void gNB_dlsch_ulsch_scheduler(module_id_t module_idP, frame_t frame, sub_frame_t slot, NR_Sched_Rsp_t* sched_info);
int oai_fapi_dl_tti_req(nfapi_nr_dl_tti_request_t *dl_config_req);
int oai_fapi_ul_tti_req(nfapi_nr_ul_tti_request_t *ul_tti_req);
int oai_fapi_tx_data_req(nfapi_nr_tx_data_request_t* tx_data_req);
int oai_fapi_ul_dci_req(nfapi_nr_ul_dci_request_t* ul_dci_req);
int oai_fapi_send_end_request(int cell, uint32_t frame, uint32_t slot);

int trigger_scheduler(nfapi_nr_slot_indication_scf_t *slot_ind)
{

  NR_UL_IND_t ind = {.frame = slot_ind->sfn, .slot = slot_ind->slot, };
  NR_UL_indication(&ind);
  // Call into the scheduler (this is hardcoded and should be init properly!)
  // memset(sched_resp, 0, sizeof(*sched_resp));
  gNB_dlsch_ulsch_scheduler(0, slot_ind->sfn, slot_ind->slot, &g_sched_resp);
  if (NFAPI_MODE == NFAPI_MODE_AERIAL) {
#ifdef ENABLE_AERIAL
    bool send_slt_resp = false;
    if (g_sched_resp.DL_req.dl_tti_request_body.nPDUs> 0) {
      oai_fapi_dl_tti_req(&g_sched_resp.DL_req);
      send_slt_resp = true;
    }
    if (g_sched_resp.UL_tti_req.n_pdus > 0) {
      oai_fapi_ul_tti_req(&g_sched_resp.UL_tti_req);
      send_slt_resp = true;
    }
    if (g_sched_resp.TX_req.Number_of_PDUs > 0) {
      oai_fapi_tx_data_req(&g_sched_resp.TX_req);
      send_slt_resp = true;
    }
    if (g_sched_resp.UL_dci_req.numPdus > 0) {
      oai_fapi_ul_dci_req(&g_sched_resp.UL_dci_req);
      send_slt_resp = true;
    }
    if (send_slt_resp) {
      oai_fapi_send_end_request(0,slot_ind->sfn, slot_ind->slot);
    }
#endif
  }

  return 1;
}


int aerial_phy_nr_slot_indication(nfapi_nr_slot_indication_scf_t *ind)
{
  uint8_t vnf_slot_ahead = 0;
  uint32_t vnf_sfn_slot = sfnslot_add_slot(ind->sfn, ind->slot, vnf_slot_ahead);
  uint16_t vnf_sfn = NFAPI_SFNSLOT2SFN(vnf_sfn_slot);
  uint8_t vnf_slot = NFAPI_SFNSLOT2SLOT(vnf_sfn_slot);
  LOG_D(MAC, "VNF SFN/Slot %d.%d \n", vnf_sfn, vnf_slot);
  // printf( "VNF SFN/Slot %d.%d \n", vnf_sfn, vnf_slot);
  trigger_scheduler(ind);

  return 1;
}

int aerial_phy_nr_srs_indication(nfapi_nr_srs_indication_t *ind)
{
  struct PHY_VARS_gNB_s *gNB = RC.gNB[0];

  gNB->UL_INFO.srs_ind = *ind;

  if (ind->number_of_pdus > 0)
    gNB->UL_INFO.srs_ind.pdu_list = malloc(sizeof(nfapi_nr_srs_indication_pdu_t) * ind->number_of_pdus);

  for (int i = 0; i < ind->number_of_pdus; i++) {
    memcpy(&gNB->UL_INFO.srs_ind.pdu_list[i], &ind->pdu_list[i], sizeof(ind->pdu_list[0]));

    LOG_D(MAC,
          "%s() NFAPI SFN/Slot:%d.%d SRS_IND:number_of_pdus:%d UL_INFO:pdus:%d\n",
          __FUNCTION__,
          ind->sfn,
          ind->slot,
          ind->number_of_pdus,
          gNB->UL_INFO.srs_ind.number_of_pdus);
  }


  return 1;
}

void *aerial_vnf_allocate(size_t size)
{
  // return (void*)memory_pool::allocate(size);
  return (void *)malloc(size);
}

void aerial_vnf_deallocate(void *ptr)
{
  // memory_pool::deallocate((uint8_t*)ptr);
  free(ptr);
}

int aerial_phy_vendor_ext(struct nfapi_vnf_p7_config *config, nfapi_p7_message_header_t *msg)
{
  if (msg->message_id == P7_VENDOR_EXT_IND) {
    // vendor_ext_p7_ind* ind = (vendor_ext_p7_ind*)msg;
    // NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] vendor_ext (error_code:%d)\n", ind->error_code);
  } else {
    NFAPI_TRACE(NFAPI_TRACE_INFO, "[VNF] unknown %02x\n", msg->message_id);
  }

  return 0;
}

int aerial_phy_unpack_p7_vendor_extension(nfapi_p7_message_header_t *header,
                                          uint8_t **ppReadPackedMessage,
                                          uint8_t *end,
                                          nfapi_p7_codec_config_t *config)
{
  // NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
  if (header->message_id == P7_VENDOR_EXT_IND) {
    vendor_ext_p7_ind *req = (vendor_ext_p7_ind *)(header);

    if (!pull16(ppReadPackedMessage, &req->error_code, end))
      return 0;
  }

  return 1;
}

int aerial_phy_pack_p7_vendor_extension(nfapi_p7_message_header_t *header,
                                        uint8_t **ppWritePackedMsg,
                                        uint8_t *end,
                                        nfapi_p7_codec_config_t *config)
{
  // NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
  if (header->message_id == P7_VENDOR_EXT_REQ) {
    // NFAPI_TRACE(NFAPI_TRACE_INFO, "%s\n", __FUNCTION__);
    vendor_ext_p7_req *req = (vendor_ext_p7_req *)(header);

    if (!(push16(req->dummy1, ppWritePackedMsg, end) && push16(req->dummy2, ppWritePackedMsg, end)))
      return 0;
  }

  return 1;
}

int aerial_phy_unpack_vendor_extension_tlv(nfapi_tl_t *tl,
                                           uint8_t **ppReadPackedMessage,
                                           uint8_t *end,
                                           void **ve,
                                           nfapi_p7_codec_config_t *codec)
{
  (void)tl;
  (void)ppReadPackedMessage;
  (void)ve;
  return -1;
}

int aerial_phy_pack_vendor_extension_tlv(void *ve, uint8_t **ppWritePackedMsg, uint8_t *end, nfapi_p7_codec_config_t *codec)
{
  // NFAPI_TRACE(NFAPI_TRACE_INFO, "phy_pack_vendor_extension_tlv\n");
  nfapi_tl_t *tlv = (nfapi_tl_t *)ve;

  switch (tlv->tag) {
    case VENDOR_EXT_TLV_1_TAG: {
      // NFAPI_TRACE(NFAPI_TRACE_INFO, "Packing VENDOR_EXT_TLV_1\n");
      vendor_ext_tlv_1 *ve = (vendor_ext_tlv_1 *)tlv;

      if (!push32(ve->dummy, ppWritePackedMsg, end))
        return 0;

      return 1;
    } break;

    default:
      return -1;
      break;
  }
}

nfapi_p7_message_header_t *aerial_phy_allocate_p7_vendor_ext(uint16_t message_id, uint16_t *msg_size)
{
  if (message_id == P7_VENDOR_EXT_IND) {
    *msg_size = sizeof(vendor_ext_p7_ind);
    return (nfapi_p7_message_header_t *)malloc(sizeof(vendor_ext_p7_ind));
  }

  return 0;
}

void aerial_phy_deallocate_p7_vendor_ext(nfapi_p7_message_header_t *header)
{
  free(header);
}

uint8_t aerial_unpack_nr_slot_indication(uint8_t **ppReadPackedMsg,
                                         uint8_t *end,
                                         nfapi_nr_slot_indication_scf_t *msg,
                                         nfapi_p7_codec_config_t *config)
{
  return unpack_nr_slot_indication(ppReadPackedMsg, end, msg, config);
}

static uint8_t aerial_unpack_nr_rx_data_indication_body(nfapi_nr_rx_data_pdu_t *value,
                                                        uint8_t **ppReadPackedMsg,
                                                        uint8_t *end,
                                                        nfapi_p7_codec_config_t *config)
{
  if (!(pull32(ppReadPackedMsg, &value->handle, end) && pull16(ppReadPackedMsg, &value->rnti, end)
        && pull8(ppReadPackedMsg, &value->harq_id, end) &&
        // For Aerial, RX_DATA.indication PDULength is changed to 32 bit field
        pull32(ppReadPackedMsg, &value->pdu_length, end) && pull8(ppReadPackedMsg, &value->ul_cqi, end)
        && pull16(ppReadPackedMsg, &value->timing_advance, end) && pull16(ppReadPackedMsg, &value->rssi, end))) {
    return 0;
  }

  // Allocate space for the pdu to be unpacked later
  uint32_t length = value->pdu_length;
  value->pdu = nfapi_p7_allocate(sizeof(*value->pdu) * length, config);

  return 1;
}
uint8_t aerial_unpack_nr_rx_data_indication(uint8_t **ppReadPackedMsg,
                                            uint8_t *end,
                                            uint8_t **pDataMsg,
                                            uint8_t *data_end,
                                            nfapi_nr_rx_data_indication_t *msg,
                                            nfapi_p7_codec_config_t *config)
{
  // For Aerial unpacking, the PDU data is packed in pDataMsg, and we read it after unpacking the PDU headers

  nfapi_nr_rx_data_indication_t *pNfapiMsg = (nfapi_nr_rx_data_indication_t *)msg;
  // Unpack SFN, slot, nPDU
  if (!(pull16(ppReadPackedMsg, &pNfapiMsg->sfn, end) && pull16(ppReadPackedMsg, &pNfapiMsg->slot, end)
        && pull16(ppReadPackedMsg, &pNfapiMsg->number_of_pdus, end))) {
    return 0;
  }
  // Allocate the PDU list for number of PDUs
  if (pNfapiMsg->number_of_pdus > 0) {
    pNfapiMsg->pdu_list = nfapi_p7_allocate(sizeof(*pNfapiMsg->pdu_list) * pNfapiMsg->number_of_pdus, config);
  }
  // For each PDU, unpack its header
  for (int i = 0; i < pNfapiMsg->number_of_pdus; i++) {
    if (!aerial_unpack_nr_rx_data_indication_body(&pNfapiMsg->pdu_list[i], ppReadPackedMsg, end, config))
      return 0;
  }
  // After unpacking the PDU headers, unpack the PDU data from the separate buffer
  for (int i = 0; i < pNfapiMsg->number_of_pdus; i++) {
    if(!pullarray8(pDataMsg,
               pNfapiMsg->pdu_list[i].pdu,
               pNfapiMsg->pdu_list[i].pdu_length,
               pNfapiMsg->pdu_list[i].pdu_length,
               data_end)){
      return 0;
    }
  }

  return 1;
}

uint8_t aerial_unpack_nr_crc_indication(uint8_t **ppReadPackedMsg,
                                        uint8_t *end,
                                        nfapi_nr_crc_indication_t *msg,
                                        nfapi_p7_codec_config_t *config)
{
  return unpack_nr_crc_indication(ppReadPackedMsg, end, msg, config);
}

uint8_t aerial_unpack_nr_uci_indication(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config)
{
  return unpack_nr_uci_indication(ppReadPackedMsg, end, msg, config);
}

uint8_t aerial_unpack_nr_srs_indication(uint8_t **ppReadPackedMsg, uint8_t *end, void *msg, nfapi_p7_codec_config_t *config)
{
  return unpack_nr_srs_indication(ppReadPackedMsg, end, msg, config);
}

uint8_t aerial_unpack_nr_rach_indication(uint8_t **ppReadPackedMsg,
                                         uint8_t *end,
                                         nfapi_nr_rach_indication_t *msg,
                                         nfapi_p7_codec_config_t *config)
{
  return unpack_nr_rach_indication(ppReadPackedMsg, end, msg, config);
}

static int32_t aerial_pack_tx_data_request(void *pMessageBuf,
                                           void *pPackedBuf,
                                           void *pDataBuf,
                                           uint32_t packedBufLen,
                                           uint32_t dataBufLen,
                                           nfapi_p7_codec_config_t *config,
                                           uint32_t *data_len)
{
  nfapi_p7_message_header_t *pMessageHeader = pMessageBuf;
  uint8_t *end = pPackedBuf + packedBufLen;
  uint8_t *data_end = pDataBuf + dataBufLen;
  uint8_t *pWritePackedMessage = pPackedBuf;
  uint8_t *pDataPackedMessage = pDataBuf;
  uint8_t *pPackMessageEnd = pPackedBuf + packedBufLen;
  uint8_t *pPackedLengthField = &pWritePackedMessage[4];
  uint8_t *pPacketBodyField = &pWritePackedMessage[8];
  uint8_t *pPacketBodyFieldStart = &pWritePackedMessage[8];
  uint8_t *pPackedDataField = &pDataPackedMessage[0];
  uint8_t *pPackedDataFieldStart = &pDataPackedMessage[0];

  if (pMessageBuf == NULL || pPackedBuf == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 Pack supplied pointers are null\n");
    return -1;
  }

  // PHY API message header
  // Number of messages [0]
  // Opaque handle [1]
  // PHY API Message structure
  // Message type ID [2,3]
  // Message Length [4,5,6,7]
  // Message Body [8,...]
  if (!(push8(1, &pWritePackedMessage, pPackMessageEnd) && push8(0, &pWritePackedMessage, pPackMessageEnd)
        && push16(pMessageHeader->message_id, &pWritePackedMessage, pPackMessageEnd))) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 Pack header failed\n");
    return -1;
  }

  uint8_t **ppWriteBody = &pPacketBodyField;
  uint8_t **ppWriteData = &pPackedDataField;
  nfapi_nr_tx_data_request_t *pNfapiMsg = (nfapi_nr_tx_data_request_t *)pMessageHeader;

  if (!(push16(pNfapiMsg->SFN, ppWriteBody, end) && push16(pNfapiMsg->Slot, ppWriteBody, end)
        && push16(pNfapiMsg->Number_of_PDUs, ppWriteBody, end))) {
    return 0;
  }

  for (int i = 0; i < pNfapiMsg->Number_of_PDUs; i++) {
    nfapi_nr_pdu_t *value = (nfapi_nr_pdu_t *)&pNfapiMsg->pdu_list[i];
    // recalculate PDU_Length for Aerial (leave only the size occupied in the payload buffer afterward)
    // assuming there is only 1 TLV present
    value->PDU_length = value->TLVs[0].length;
    if (!(push32(value->PDU_length, ppWriteBody, end)
          && // cuBB expects TX_DATA.request PDUSize to be 32 bit
          push16(value->PDU_index, ppWriteBody, end) && push32(value->num_TLV, ppWriteBody, end))) {
      return 0;
    }
    uint16_t k = 0;
    uint16_t total_number_of_tlvs = value->num_TLV;

    for (; k < total_number_of_tlvs; ++k) {
      // For Aerial, the TLV tag is always 2
      if (!(push16(/*value->TLVs[k].tag*/ 2, ppWriteBody, end) && push16(/*value->TLVs[k].length*/ 4, ppWriteBody, end))) {
        return 0;
      }
      // value
      if (!push32(0, ppWriteBody, end)) {
        return 0;
      }
    }
  }

  //Actual payloads are packed in a separate buffer
  for (int i = 0; i < pNfapiMsg->Number_of_PDUs; i++) {
    nfapi_nr_pdu_t *value = (nfapi_nr_pdu_t *)&pNfapiMsg->pdu_list[i];

    uint16_t k = 0;
    uint16_t total_number_of_tlvs = value->num_TLV;

    for (; k < total_number_of_tlvs; ++k) {
      if (value->TLVs[k].length > 0) {
        if (value->TLVs[k].length % 4 != 0) {
          if (!pusharray32(value->TLVs[k].value.direct, 16384, ((value->TLVs[k].length + 3) / 4) - 1, ppWriteData, data_end)) {
            return 0;
          }
          int bytesToAdd = 4 - (4 - (value->TLVs[k].length % 4)) % 4;
          if (bytesToAdd != 4) {
            for (int j = 0; j < bytesToAdd; j++) {
              uint8_t toPush = (uint8_t)(value->TLVs[k].value.direct[((value->TLVs[k].length + 3) / 4) - 1] >> (j * 8));
              if (!push8(toPush, ppWriteData, data_end)) {
                return 0;
              }
            }
          }
        } else {
          // no padding needed
          if (!pusharray32(value->TLVs[k].value.direct, 16384, ((value->TLVs[k].length + 3) / 4), ppWriteData, data_end)) {
            return 0;
          }
        }
      } else {
        LOG_E(NR_MAC,"value->TLVs[i].length was 0! (%d.%d) \n", pNfapiMsg->SFN, pNfapiMsg->Slot);
      }
    }
  }


  // calculate data_len
  uintptr_t dataHead = (uintptr_t)pPackedDataFieldStart;
  uintptr_t dataEnd = (uintptr_t)pPackedDataField;
  data_len[0] = dataEnd - dataHead;

  // check for a valid message length
  uintptr_t msgHead = (uintptr_t)pPacketBodyFieldStart;
  uintptr_t msgEnd = (uintptr_t)pPacketBodyField;
  uint32_t packedMsgLen = msgEnd - msgHead;
  uint16_t packedMsgLen16;
  if (packedMsgLen > 0xFFFF || packedMsgLen > packedBufLen) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "Packed message length error %d, buffer supplied %d\n", packedMsgLen, packedBufLen);
    return 0;
  } else {
    packedMsgLen16 = (uint16_t)packedMsgLen;
  }

  // Update the message length in the header
  pMessageHeader->message_length = packedMsgLen16;

  // Update the message length in the header
  if (!push32(packedMsgLen, &pPackedLengthField, pPackMessageEnd))
    return 0;

  if (1) {
    // quick test
    if (pMessageHeader->message_length != packedMsgLen) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR,
                  "nfapi packedMsgLen(%d) != message_length(%d) id %d\n",
                  packedMsgLen,
                  pMessageHeader->message_length,
                  pMessageHeader->message_id);
    }
  }

  return (packedMsgLen16);
}

int fapi_nr_p7_message_pack(void *pMessageBuf, void *pPackedBuf, uint32_t packedBufLen, nfapi_p7_codec_config_t *config)
{
  nfapi_p7_message_header_t *pMessageHeader = pMessageBuf;
  uint8_t *end = pPackedBuf + packedBufLen;
  uint8_t *pWritePackedMessage = pPackedBuf;
  uint8_t *pPackMessageEnd = pPackedBuf + packedBufLen;
  uint8_t *pPackedLengthField = &pWritePackedMessage[4];
  uint8_t *pPacketBodyField = &pWritePackedMessage[8];
  uint8_t *pPacketBodyFieldStart = &pWritePackedMessage[8];

  if (pMessageBuf == NULL || pPackedBuf == NULL) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 Pack supplied pointers are null\n");
    return -1;
  }

  // PHY API message header
  // Number of messages [0]
  // Opaque handle [1]
  // PHY API Message structure
  // Message type ID [2,3]
  // Message Length [4,5,6,7]
  // Message Body [8,...]
  if (!(push8(1, &pWritePackedMessage, pPackMessageEnd) && push8(0, &pWritePackedMessage, pPackMessageEnd)
        && push16(pMessageHeader->message_id, &pWritePackedMessage, pPackMessageEnd))) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 Pack header failed\n");
    return -1;
  }

  // look for the specific message
  uint8_t result = 0;
  switch (pMessageHeader->message_id) {
    case NFAPI_NR_PHY_MSG_TYPE_DL_TTI_REQUEST:
      result = pack_dl_tti_request(pMessageHeader, &pPacketBodyField, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_UL_TTI_REQUEST:
      result = pack_ul_tti_request(pMessageHeader, &pPacketBodyField, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST:
      // TX_DATA.request already handled by aerial_pack_tx_data_request
      break;

    case NFAPI_NR_PHY_MSG_TYPE_UL_DCI_REQUEST:
      result = pack_ul_dci_request(pMessageHeader, &pPacketBodyField, end, config);
      break;

    case NFAPI_UE_RELEASE_REQUEST:
      result = pack_ue_release_request(pMessageHeader, &pPacketBodyField, end, config);
      break;

    case NFAPI_UE_RELEASE_RESPONSE:
      result = pack_ue_release_response(pMessageHeader, &pPacketBodyField, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_SLOT_INDICATION:
      result = pack_nr_slot_indication(pMessageHeader, &pPacketBodyField, end, config);

    case NFAPI_NR_PHY_MSG_TYPE_RX_DATA_INDICATION:
      result = pack_nr_rx_data_indication(pMessageHeader, &pPacketBodyField, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_CRC_INDICATION:
      result = pack_nr_crc_indication(pMessageHeader, &pPacketBodyField, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_UCI_INDICATION:
      result = pack_nr_uci_indication(pMessageHeader, &pPacketBodyField, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_SRS_INDICATION:
      result = pack_nr_srs_indication(pMessageHeader, &pPacketBodyField, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_RACH_INDICATION:
      result = pack_nr_rach_indication(pMessageHeader, &pPacketBodyField, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_DL_NODE_SYNC:
      result = pack_nr_dl_node_sync(pMessageHeader, &pPacketBodyField, end, config);
      break;

    case NFAPI_NR_PHY_MSG_TYPE_UL_NODE_SYNC:
      result = pack_nr_ul_node_sync(pMessageHeader, &pPacketBodyField, end, config);
      break;

    case NFAPI_TIMING_INFO:
      result = pack_nr_timing_info(pMessageHeader, &pPacketBodyField, end, config);
      break;

    case 0x8f:
      result = pack_nr_slot_indication(pMessageHeader, &pPacketBodyField, end, config);
      break;

    default: {
      if (pMessageHeader->message_id >= NFAPI_VENDOR_EXT_MSG_MIN && pMessageHeader->message_id <= NFAPI_VENDOR_EXT_MSG_MAX) {
        if (config && config->pack_p7_vendor_extension) {
          result = (config->pack_p7_vendor_extension)(pMessageHeader, &pPacketBodyField, end, config);
        } else {
          NFAPI_TRACE(NFAPI_TRACE_ERROR,
                      "%s VE NFAPI message ID %d. No ve ecoder provided\n",
                      __FUNCTION__,
                      pMessageHeader->message_id);
        }
      } else {
        NFAPI_TRACE(NFAPI_TRACE_ERROR, "%s NFAPI Unknown message ID %d\n", __FUNCTION__, pMessageHeader->message_id);
      }
    } break;
  }

  if (result == 0) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "P7 Pack failed to pack message\n");
    return -1;
  }

  // check for a valid message length
  uintptr_t msgHead = (uintptr_t)pPacketBodyFieldStart;
  uintptr_t msgEnd = (uintptr_t)pPacketBodyField;
  uint32_t packedMsgLen = msgEnd - msgHead;
  uint16_t packedMsgLen16;
  if (packedMsgLen > 0xFFFF || packedMsgLen > packedBufLen) {
    NFAPI_TRACE(NFAPI_TRACE_ERROR, "Packed message length error %d, buffer supplied %d\n", packedMsgLen, packedBufLen);
    return -1;
  } else {
    packedMsgLen16 = (uint16_t)packedMsgLen;
  }

  // Update the message length in the header
  pMessageHeader->message_length = packedMsgLen16;

  // Update the message length in the header
  if (!push32(packedMsgLen, &pPackedLengthField, pPackMessageEnd))
    return -1;

  if (1) {
    // quick test
    if (pMessageHeader->message_length != packedMsgLen) {
      NFAPI_TRACE(NFAPI_TRACE_ERROR,
                  "nfapi packedMsgLen(%d) != message_length(%d) id %d\n",
                  packedMsgLen,
                  pMessageHeader->message_length,
                  pMessageHeader->message_id);
    }
  }

  return (packedMsgLen16);
}

int fapi_nr_pack_and_send_p7_message(vnf_p7_t *vnf_p7, nfapi_p7_message_header_t *header)
{
  uint8_t FAPI_buffer[1024 * 64];
  // Check if TX_DATA request, if true, need to pack to data_buf
  if (header->message_id
      == NFAPI_NR_PHY_MSG_TYPE_TX_DATA_REQUEST) {
    uint8_t FAPI_data_buffer[1024 * 64];
    uint32_t data_len = 0;
    int32_t len_FAPI = aerial_pack_tx_data_request(header,
                                               FAPI_buffer,
                                               FAPI_data_buffer,
                                               sizeof(FAPI_buffer),
                                               sizeof(FAPI_data_buffer),
                                               &vnf_p7->_public.codec_config,
                                               &data_len);
    if (len_FAPI <=0) {
      LOG_E(NFAPI_VNF,"Problem packing TX_DATA_request\n");
      return len_FAPI;
    }
    else
      return aerial_send_P7_msg_with_data(FAPI_buffer, len_FAPI, FAPI_data_buffer, data_len, header);
  } else {
    // Create and send FAPI P7 message
    int len_FAPI = fapi_nr_p7_message_pack(header, FAPI_buffer, sizeof(FAPI_buffer), &vnf_p7->_public.codec_config);
    return aerial_send_P7_msg(FAPI_buffer, len_FAPI, header);
  }
}
#endif
