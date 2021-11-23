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

/*! \file openair2/NR_PHY_INTERFACE/NR_IF_Module.c
* \brief data structures for PHY/MAC interface modules
* \author EURECOM/NTUST
* \date 2018
* \version 0.1
* \company Eurecom, NTUST
* \email: raymond.knopp@eurecom.fr, kroempa@gmail.com
* \note
* \warning
*/

#include "openair1/SCHED_NR/fapi_nr_l1.h"
#include "openair2/NR_PHY_INTERFACE/NR_IF_Module.h"
#include "LAYER2/NR_MAC_COMMON/nr_mac_extern.h"
#include "LAYER2/NR_MAC_gNB/mac_proto.h"
#include "common/ran_context.h"
#include "executables/softmodem-common.h"
#include "nfapi/oai_integration/vendor_ext.h" 
#include "nfapi/oai_integration/gnb_ind_vars.h"
#include "openair2/PHY_INTERFACE/queue_t.h"

#define MAX_IF_MODULES 100
//#define UL_HARQ_PRINT

static NR_IF_Module_t *nr_if_inst[MAX_IF_MODULES];
static NR_Sched_Rsp_t NR_Sched_INFO[MAX_IF_MODULES][MAX_NUM_CCs];
extern int oai_nfapi_harq_indication(nfapi_harq_indication_t *harq_ind);
extern int oai_nfapi_crc_indication(nfapi_crc_indication_t *crc_ind);
extern int oai_nfapi_cqi_indication(nfapi_cqi_indication_t *cqi_ind);
extern int oai_nfapi_sr_indication(nfapi_sr_indication_t *ind);
extern int oai_nfapi_rx_ind(nfapi_rx_indication_t *ind);
extern int oai_nfapi_nr_slot_indication(nfapi_nr_slot_indication_scf_t *ind);
extern int oai_nfapi_nr_rx_data_indication(nfapi_nr_rx_data_indication_t *ind);
extern int oai_nfapi_nr_crc_indication(nfapi_nr_crc_indication_t *ind);
extern int oai_nfapi_nr_srs_indication(nfapi_nr_srs_indication_t *ind);
extern int oai_nfapi_nr_uci_indication(nfapi_nr_uci_indication_t *ind);
extern int oai_nfapi_nr_rach_indication(nfapi_nr_rach_indication_t *ind);
extern uint8_t nfapi_mode;
extern uint16_t sf_ahead;
extern uint16_t sl_ahead;


void handle_nr_rach(NR_UL_IND_t *UL_info)
{
  if(NFAPI_MODE == NFAPI_MODE_PNF) {
    if (UL_info->rach_ind.number_of_pdus > 0) {
      LOG_D(PHY,"UL_info->UL_info->rach_ind.number_of_pdus:%d SFN/Slot:%d.%d \n", UL_info->rach_ind.number_of_pdus, UL_info->rach_ind.sfn,UL_info->rach_ind.slot);
      oai_nfapi_nr_rach_indication(&UL_info->rach_ind);
      UL_info->rach_ind.number_of_pdus = 0;
    }
    return;
  }
  NR_UL_IND_t UL_INFO;
  nfapi_nr_rach_indication_t *rach_ind = NULL;
  if (get_softmodem_params()->emulate_l1)
  {
    if (gnb_rach_ind_queue.num_items == 0)
      return; 
    LOG_I(NR_MAC, "gnb_rach_ind_queue size = %zu\n", gnb_rach_ind_queue.num_items);
    rach_ind = get_queue(&gnb_rach_ind_queue);

    UL_INFO.rach_ind = *rach_ind;
    UL_INFO.module_id = UL_info->module_id;
    UL_INFO.CC_id = UL_info->CC_id;
  }
  else
  {
    UL_INFO.rach_ind = UL_info->rach_ind;
    UL_INFO.module_id = UL_info->module_id;
    UL_INFO.CC_id = UL_info->CC_id;
  }

  int frame_diff = UL_info->frame - UL_INFO.rach_ind.sfn;
  if (frame_diff < 0)
  {
    frame_diff += 1024;
  }
  bool in_timewindow = frame_diff == 0 || (frame_diff == 1 && UL_info->slot < 7);

  if (UL_INFO.rach_ind.number_of_pdus>0 && in_timewindow) {
    LOG_I(MAC,"UL_info[Frame %d, Slot %d] Calling initiate_ra_proc RACH:SFN/SLOT:%d/%d\n",
          UL_info->frame, UL_info->slot, UL_INFO.rach_ind.sfn, UL_INFO.rach_ind.slot);
    int npdus = UL_INFO.rach_ind.number_of_pdus;
    for(int i = 0; i < npdus; i++) {
      UL_INFO.rach_ind.number_of_pdus--;
      if (UL_INFO.rach_ind.pdu_list[i].num_preamble>0)
      AssertFatal(UL_INFO.rach_ind.pdu_list[i].num_preamble==1,
                  "More than 1 preamble not supported\n");
    
      nr_initiate_ra_proc(UL_INFO.module_id,
                          UL_INFO.CC_id,
                          UL_INFO.rach_ind.sfn,
                          UL_INFO.rach_ind.slot,
                          UL_INFO.rach_ind.pdu_list[i].preamble_list[0].preamble_index,
                          UL_INFO.rach_ind.pdu_list[i].freq_index,
                          UL_INFO.rach_ind.pdu_list[i].symbol_index,
                          UL_INFO.rach_ind.pdu_list[i].preamble_list[0].timing_advance);
    }
  }
  if (get_softmodem_params()->emulate_l1)
  {
    if (rach_ind && rach_ind->number_of_pdus > 0)
    {
      for(int i = 0; i < rach_ind->number_of_pdus; i++)
      {
        free(rach_ind->pdu_list[i].preamble_list);
        rach_ind->pdu_list[i].preamble_list = NULL;
      }
      free(rach_ind->pdu_list);
      rach_ind->pdu_list = NULL;
    }
    free(rach_ind);
    rach_ind = NULL;
  }
}


void handle_nr_uci(NR_UL_IND_t *UL_info)
{
  if(NFAPI_MODE == NFAPI_MODE_PNF) {
    if (UL_info->uci_ind.num_ucis > 0) {
      LOG_D(PHY,"PNF Sending UL_info->num_ucis:%d PDU_type: %d, SFN/SF:%d.%d \n", UL_info->uci_ind.num_ucis, UL_info->uci_ind.uci_list[0].pdu_type ,UL_info->frame, UL_info->slot);
      oai_nfapi_nr_uci_indication(&UL_info->uci_ind);
      UL_info->uci_ind.num_ucis = 0;
    }
    return;
  }

  NR_UL_IND_t UL_INFO;
  nfapi_nr_uci_indication_t *uci_ind = NULL;
  if (get_softmodem_params()->emulate_l1)
  {
    if (gnb_uci_ind_queue.num_items == 0)
      return; 
    LOG_I(NR_MAC, "gnb_uci_ind_queue size = %zu\n", gnb_uci_ind_queue.num_items);
    uci_ind = get_queue(&gnb_uci_ind_queue);
    UL_INFO.uci_ind = *uci_ind;
  }
  else
  {
    UL_INFO.uci_ind = UL_info->uci_ind;
  }

  const module_id_t mod_id = UL_info->module_id;
  const frame_t frame = UL_INFO.uci_ind.sfn;
  const sub_frame_t slot = UL_INFO.uci_ind.slot;
  int num_ucis = UL_INFO.uci_ind.num_ucis;
  nfapi_nr_uci_t *uci_list = UL_INFO.uci_ind.uci_list;

  for (int i = 0; i < num_ucis; i++) {
    switch (uci_list[i].pdu_type) {
      case NFAPI_NR_UCI_PUSCH_PDU_TYPE:
        LOG_E(MAC, "%s(): unhandled NFAPI_NR_UCI_PUSCH_PDU_TYPE\n", __func__);
        break;

      case NFAPI_NR_UCI_FORMAT_0_1_PDU_TYPE: {
        const nfapi_nr_uci_pucch_pdu_format_0_1_t *uci_pdu = &uci_list[i].pucch_pdu_format_0_1;
        LOG_D(NR_MAC, "The received uci has sfn slot %d %d, num_ucis %d and pdu_size %d\n",
                UL_INFO.uci_ind.sfn, UL_INFO.uci_ind.slot, num_ucis, uci_list[i].pdu_size);
        handle_nr_uci_pucch_0_1(mod_id, frame, slot, uci_pdu);
        break;
      }

        case NFAPI_NR_UCI_FORMAT_2_3_4_PDU_TYPE: {
          const nfapi_nr_uci_pucch_pdu_format_2_3_4_t *uci_pdu = &uci_list[i].pucch_pdu_format_2_3_4;
          handle_nr_uci_pucch_2_3_4(mod_id, frame, slot, uci_pdu);
          break;
        }
      LOG_D(MAC, "UCI handled \n");
    }
  }

  if (get_softmodem_params()->emulate_l1)
  {
    for (int i = 0; i < num_ucis; i++) {
      switch (uci_list[i].pdu_type) {
        case NFAPI_NR_UCI_FORMAT_0_1_PDU_TYPE:
          if (uci_list[i].pucch_pdu_format_0_1.harq) {
            free(uci_list[i].pucch_pdu_format_0_1.harq->harq_list);
            uci_list[i].pucch_pdu_format_0_1.harq->harq_list = NULL;
          }
          free(uci_list[i].pucch_pdu_format_0_1.harq);
          uci_list[i].pucch_pdu_format_0_1.harq = NULL;
          free(uci_list[i].pucch_pdu_format_0_1.sr);
          uci_list[i].pucch_pdu_format_0_1.sr = NULL;
          break;

        case NFAPI_NR_UCI_FORMAT_2_3_4_PDU_TYPE:
          free(uci_list[i].pucch_pdu_format_2_3_4.harq.harq_payload);
          free(uci_list[i].pucch_pdu_format_2_3_4.csi_part1.csi_part1_payload);
          free(uci_list[i].pucch_pdu_format_2_3_4.csi_part2.csi_part2_payload);
          break;
      }
    }
    if (uci_ind && num_ucis > 0) {
      free(uci_list);
      uci_list = NULL;
    }
    free(uci_ind);
    uci_ind = NULL;
  }
  else
  {
    UL_info->uci_ind.num_ucis = 0;
  }
}

static bool crc_sfn_slot_matcher(void *wanted, void *candidate)
{
  nfapi_p7_message_header_t *msg = candidate;
  int sfn_sf = *(int*)wanted;

  switch (msg->message_id)
  {
    case NFAPI_NR_PHY_MSG_TYPE_CRC_INDICATION:
    {
      nfapi_nr_crc_indication_t *ind = candidate;
      return NFAPI_SFNSLOT2SFN(sfn_sf) == ind->sfn && NFAPI_SFNSLOT2SLOT(sfn_sf) == ind->slot;
    }

    default:
      LOG_E(NR_MAC, "sfn_slot_match bad ID: %d\n", msg->message_id);

  }
  return false;
}

void handle_nr_ulsch(NR_UL_IND_t *UL_info)
{
  if(NFAPI_MODE == NFAPI_MODE_PNF) {
    if (UL_info->crc_ind.number_crcs > 0) {
      LOG_D(PHY,"UL_info->UL_info->crc_ind.number_crcs:%d CRC_IND:SFN/Slot:%d.%d\n", UL_info->crc_ind.number_crcs, UL_info->crc_ind.sfn, UL_info->crc_ind.slot);
      oai_nfapi_nr_crc_indication(&UL_info->crc_ind);
      UL_info->crc_ind.number_crcs = 0;
    }

    if (UL_info->rx_ind.number_of_pdus > 0) {
      LOG_D(PHY,"UL_info->rx_ind.number_of_pdus:%d RX_IND:SFN/Slot:%d.%d \n", UL_info->rx_ind.number_of_pdus, UL_info->rx_ind.sfn, UL_info->rx_ind.slot);
      oai_nfapi_nr_rx_data_indication(&UL_info->rx_ind);
      UL_info->rx_ind.number_of_pdus = 0;
    }
    return;
  }

  NR_UL_IND_t UL_INFO;
  nfapi_nr_rx_data_indication_t *rx_ind = NULL;
  nfapi_nr_crc_indication_t *crc_ind = NULL;
  if (get_softmodem_params()->emulate_l1)
  {
    rx_ind = get_queue(&gnb_rx_ind_queue);
    if (!rx_ind)
    {
      LOG_D(NR_PHY, "No rx data indication (empty gnb_rx_ind_queue)\n");
      return;
    }

    int sfn_slot = NFAPI_SFNSLOT2HEX(rx_ind->sfn, rx_ind->slot); 
    
    crc_ind = unqueue_matching(&gnb_crc_ind_queue,
                               MAX_QUEUE_SIZE,
                               crc_sfn_slot_matcher,
                               &sfn_slot);
    if (!crc_ind)
    {
      LOG_D(NR_PHY, "No crc indication with the same SFN SLOT of rx indication %u %u\n", rx_ind->sfn, rx_ind->slot);
      put_queue(&gnb_rx_ind_queue, rx_ind);
      return;
    }

    UL_INFO.rx_ind = *rx_ind;
    UL_INFO.crc_ind = *crc_ind;
    UL_INFO.module_id = UL_info->module_id;
    UL_INFO.CC_id = UL_info->CC_id;
  }
  else
  {
    UL_INFO.rx_ind = UL_info->rx_ind;
    UL_INFO.crc_ind = UL_info->crc_ind;
    UL_INFO.module_id = UL_info->module_id;
    UL_INFO.CC_id = UL_info->CC_id;
  }

  UL_INFO.frame = UL_INFO.rx_ind.sfn;
  UL_INFO.slot = UL_INFO.rx_ind.slot;
  UL_INFO.module_id = UL_info->module_id;
  UL_INFO.CC_id = UL_info->CC_id;
  LOG_D(NR_MAC, " UL_info frame slot vs rx_ind frame slot vs crc_ind slot frame slot = %u %u vs %u %u vs %u %u\n",
                  UL_info->frame, UL_info->slot,
                  UL_INFO.rx_ind.sfn, UL_INFO.rx_ind.slot,
                  UL_INFO.crc_ind.sfn, UL_INFO.crc_ind.slot
                  );

  if (UL_INFO.rx_ind.number_of_pdus > 0 && UL_INFO.crc_ind.number_crcs > 0) {
    for (int i = 0; i < UL_INFO.rx_ind.number_of_pdus; i++) {
      int j;
      for (j = 0; j < UL_INFO.crc_ind.number_crcs; j++) {
        // find crc_indication j corresponding rx_indication i
        const nfapi_nr_rx_data_pdu_t *rx = &UL_INFO.rx_ind.pdu_list[i];
        const nfapi_nr_crc_t *crc = &UL_INFO.crc_ind.crc_list[j];
        LOG_D(NR_PHY,
              "UL_INFO.crc_ind.pdu_list[%d].rnti:%04x "
              "UL_INFO.rx_ind.pdu_list[%d].rnti:%04x\n",
              j,
              crc->rnti,
              i,
              rx->rnti);

        if (crc->rnti != rx->rnti)
        {
          LOG_D(NR_MAC, "mis-match between CRC rnti %04x and RX rnit %04x\n",  crc->rnti,  rx->rnti);
          continue;
        }

        LOG_D(NR_MAC,
              "%4d.%2d Calling rx_sdu (CRC %s/tb_crc_status %d)\n",
              UL_INFO.frame,
              UL_INFO.slot,
              crc->tb_crc_status ? "error" : "ok",
              crc->tb_crc_status);

        /* if CRC passes, pass PDU, otherwise pass NULL as error indication */
        nr_rx_sdu(UL_INFO.module_id,
                  UL_INFO.CC_id,
                  UL_INFO.rx_ind.sfn,
                  UL_INFO.rx_ind.slot,
                  rx->rnti,
                  crc->tb_crc_status ? NULL : rx->pdu,
                  rx->pdu_length,
                  rx->timing_advance,
                  rx->ul_cqi,
                  rx->rssi);
        handle_nr_ul_harq(UL_INFO.CC_id, UL_INFO.module_id, UL_INFO.frame, UL_INFO.slot, crc);
        break;
      } //    for (j=0;j<UL_INFO.crc_ind.number_crcs;j++)
      if (get_softmodem_params()->emulate_l1)
      {
        int last = UL_INFO.crc_ind.number_crcs - 1;
        if (j < last)
        {
          crc_ind->crc_list[j] = crc_ind->crc_list[last];
          UL_INFO.crc_ind.number_crcs--;
        }
        else if (j == last)
        {
          UL_INFO.crc_ind.number_crcs--;
        }

        last = UL_INFO.rx_ind.number_of_pdus - 1;
        if (i < last)
        {
          rx_ind->pdu_list[i] = rx_ind->pdu_list[last];
          UL_INFO.rx_ind.number_of_pdus--;
          i--;
        }
        else if (i == last)
        {
          UL_INFO.rx_ind.number_of_pdus--;
        }
      }
    } //   for (i=0;i<UL_INFO.rx_ind.number_of_pdus;i++)
    if (get_softmodem_params()->emulate_l1)
    {
      if (UL_INFO.crc_ind.number_crcs > 0)
      {
        crc_ind->number_crcs = UL_INFO.crc_ind.number_crcs;
        requeue(&gnb_crc_ind_queue, crc_ind);
      }

      if (UL_INFO.rx_ind.number_of_pdus > 0)
      {
        rx_ind->number_of_pdus = UL_INFO.rx_ind.number_of_pdus;
        requeue(&gnb_rx_ind_queue, rx_ind);
      }

      if (UL_INFO.crc_ind.number_crcs == 0)
      {
        free(crc_ind->crc_list);
        crc_ind->crc_list = NULL;
        free(crc_ind);
        crc_ind = NULL;
      }
      if (UL_INFO.rx_ind.number_of_pdus == 0)
      {
        free(rx_ind->pdu_list);
        rx_ind->pdu_list = NULL;
        free(rx_ind);
        rx_ind = NULL;
      }
    }
    else
    {
      UL_info->crc_ind.number_crcs = 0;
      UL_info->rx_ind.number_of_pdus = 0;
    }
  } else if ((UL_INFO.rx_ind.number_of_pdus != 0)
             || (UL_INFO.crc_ind.number_crcs != 0)) {
     LOG_E(NR_PHY,
          "hoping not to have mis-match between CRC ind and RX ind - "
          "hopefully the missing message is coming shortly "
          "rx_ind:%d(SFN/SL:%d/%d) crc_ind:%d(SFN/SL:%d/%d) \n",
          UL_INFO.rx_ind.number_of_pdus,
          UL_INFO.rx_ind.sfn,
          UL_INFO.rx_ind.slot,
          UL_INFO.crc_ind.number_crcs,
          UL_INFO.crc_ind.sfn,
          UL_INFO.crc_ind.slot);
  }
}

void NR_UL_indication(NR_UL_IND_t *UL_info) {
  AssertFatal(UL_info!=NULL,"UL_INFO is null\n");
#ifdef DUMP_FAPI
  dump_ul(UL_info);
#endif
  module_id_t      module_id   = UL_info->module_id;
  int              CC_id       = UL_info->CC_id;
  NR_Sched_Rsp_t   *sched_info = &NR_Sched_INFO[module_id][CC_id];
  NR_IF_Module_t   *ifi        = nr_if_inst[module_id];
  gNB_MAC_INST     *mac        = RC.nrmac[module_id];
  LOG_D(NR_PHY,"SFN/SLOT:%d.%d module_id:%d CC_id:%d UL_info[rach_pdus:%zu rx_ind:%zu crcs:%zu]\n",
        UL_info->frame,UL_info->slot,
        module_id,CC_id, gnb_rach_ind_queue.num_items,
        gnb_rx_ind_queue.num_items, gnb_crc_ind_queue.num_items);

  handle_nr_rach(UL_info);
  
  handle_nr_uci(UL_info);
  // clear UL DCI prior to handling ULSCH
  mac->UL_dci_req[CC_id].numPdus = 0;
  handle_nr_ulsch(UL_info);

  if (NFAPI_MODE != NFAPI_MODE_PNF) {

    if (ifi->CC_mask==0) {
      ifi->current_frame    = UL_info->frame;
      ifi->current_slot = UL_info->slot;
    } else {
      AssertFatal(UL_info->frame != ifi->current_frame,"CC_mask %x is not full and frame has changed\n",ifi->CC_mask);
      AssertFatal(UL_info->slot != ifi->current_slot,"CC_mask %x is not full and slot has changed\n",ifi->CC_mask);
    }

    ifi->CC_mask |= (1<<CC_id);

    if (ifi->CC_mask == ((1<<MAX_NUM_CCs)-1)) {
      /*
      eNB_dlsch_ulsch_scheduler(module_id,
          (UL_info->frame+((UL_info->slot>(9-sl_ahead))?1:0)) % 1024,
          (UL_info->slot+sl_ahead)%10);
      */
      nfapi_nr_config_request_scf_t *cfg = &mac->config[CC_id];
      int spf = get_spf(cfg);
      gNB_dlsch_ulsch_scheduler(module_id,
				(UL_info->frame+((UL_info->slot>(spf-1-sl_ahead))?1:0)) % 1024,
				(UL_info->slot+sl_ahead)%spf);

      ifi->CC_mask            = 0;
      sched_info->module_id   = module_id;
      sched_info->CC_id       = CC_id;
      sched_info->frame       = (UL_info->frame + ((UL_info->slot>(spf-1-sl_ahead)) ? 1 : 0)) % 1024;
      sched_info->slot        = (UL_info->slot+sl_ahead)%spf;
      sched_info->DL_req      = &mac->DL_req[CC_id];
      sched_info->UL_dci_req  = &mac->UL_dci_req[CC_id];

      sched_info->UL_tti_req  = mac->UL_tti_req[CC_id];

      sched_info->TX_req      = &mac->TX_req[CC_id];
#ifdef DUMP_FAPI
      dump_dl(sched_info);
#endif

      if (ifi->NR_Schedule_response) {
        AssertFatal(ifi->NR_Schedule_response!=NULL,
                    "nr_schedule_response is null (mod %d, cc %d)\n",
                    module_id,
                    CC_id);
        ifi->NR_Schedule_response(sched_info);
      }

      LOG_D(NR_PHY,"NR_Schedule_response: SFN SLOT:%d %d dl_pdus:%d\n",
	    sched_info->frame,
	    sched_info->slot,
	    sched_info->DL_req->dl_tti_request_body.nPDUs);
    }
  }
}

NR_IF_Module_t *NR_IF_Module_init(int Mod_id) {
  AssertFatal(Mod_id<MAX_MODULES,"Asking for Module %d > %d\n",Mod_id,MAX_IF_MODULES);
  LOG_I(PHY,"Installing callbacks for IF_Module - UL_indication\n");

  if (nr_if_inst[Mod_id]==NULL) {
    nr_if_inst[Mod_id] = (NR_IF_Module_t*)malloc(sizeof(NR_IF_Module_t));
    memset((void*)nr_if_inst[Mod_id],0,sizeof(NR_IF_Module_t));

    LOG_I(MAC,"Allocating shared L1/L2 interface structure for instance %d @ %p\n",Mod_id,nr_if_inst[Mod_id]);

    nr_if_inst[Mod_id]->CC_mask=0;
    nr_if_inst[Mod_id]->NR_UL_indication = NR_UL_indication;
    AssertFatal(pthread_mutex_init(&nr_if_inst[Mod_id]->if_mutex,NULL)==0,
                "allocation of nr_if_inst[%d]->if_mutex fails\n",Mod_id);
  }

  return nr_if_inst[Mod_id];
}
