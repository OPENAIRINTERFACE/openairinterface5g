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

#define MAX_IF_MODULES 100
//#define UL_HARQ_PRINT

NR_IF_Module_t *if_inst[MAX_IF_MODULES];
NR_Sched_Rsp_t Sched_INFO[MAX_IF_MODULES][MAX_NUM_CCs];

extern int oai_nfapi_harq_indication(nfapi_harq_indication_t *harq_ind);
extern int oai_nfapi_crc_indication(nfapi_crc_indication_t *crc_ind);
extern int oai_nfapi_cqi_indication(nfapi_cqi_indication_t *cqi_ind);
extern int oai_nfapi_sr_indication(nfapi_sr_indication_t *ind);
extern int oai_nfapi_rx_ind(nfapi_rx_indication_t *ind);
extern uint8_t nfapi_mode;
extern uint16_t sf_ahead;
extern uint16_t sl_ahead;
extern NR_UL_IND_t UL_INFO;

void handle_nr_rach(NR_UL_IND_t *UL_info)
{
  // Melissa: TODO come back and differentiate between global UL_info and passed in arg
  if (UL_INFO.rach_ind.number_of_pdus>0) {
    LOG_I(MAC,"UL_INFO[Frame %d, Slot %d] Calling initiate_ra_proc RACH:SFN/SLOT:%d/%d\n",
          UL_INFO.frame,UL_INFO.slot, UL_INFO.rach_ind.sfn,UL_INFO.rach_ind.slot);
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
}


void handle_nr_uci(NR_UL_IND_t *UL_info)
{
  const module_id_t mod_id = UL_INFO.module_id;
  const frame_t frame = UL_INFO.frame;
  const sub_frame_t slot = UL_INFO.slot;
  int num_ucis = UL_INFO.uci_ind.num_ucis;
  nfapi_nr_uci_t *uci_list = UL_INFO.uci_ind.uci_list;

  for (int i = 0; i < num_ucis; i++) {
    switch (uci_list[i].pdu_type) {
      case NFAPI_NR_UCI_PUSCH_PDU_TYPE:
        LOG_E(MAC, "%s(): unhandled NFAPI_NR_UCI_PUSCH_PDU_TYPE\n", __func__);
        break;

      case NFAPI_NR_UCI_FORMAT_0_1_PDU_TYPE: {
        const nfapi_nr_uci_pucch_pdu_format_0_1_t *uci_pdu = &uci_list[i].pucch_pdu_format_0_1;
        handle_nr_uci_pucch_0_1(mod_id, frame, slot, uci_pdu);
        break;
      }
      case NFAPI_NR_UCI_FORMAT_2_3_4_PDU_TYPE: {
        const nfapi_nr_uci_pucch_pdu_format_2_3_4_t *uci_pdu = &uci_list[i].pucch_pdu_format_2_3_4;
        handle_nr_uci_pucch_2_3_4(mod_id, frame, slot, uci_pdu);
        break;
      }
    }
  }

  UL_INFO.uci_ind.num_ucis = 0;
  if(NFAPI_MODE != NFAPI_MODE_PNF)
  // mark corresponding PUCCH resources as free
  // NOTE: we just assume it is BWP ID 1, to be revised for multiple BWPs
  RC.nrmac[mod_id]->pucch_index_used[1][slot] = 0;
}


void handle_nr_ulsch(NR_UL_IND_t *UL_info)
{
  // Melissa: TODO come back and differentiate between global UL_info and passed in arg
  if (UL_INFO.rx_ind.number_of_pdus > 0 && UL_INFO.crc_ind.number_crcs > 0) {
    for (int i = 0; i < UL_INFO.rx_ind.number_of_pdus; i++) {
      for (int j = 0; j < UL_INFO.crc_ind.number_crcs; j++) {
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
          continue;

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
        handle_nr_ul_harq(UL_INFO.module_id, UL_INFO.frame, UL_INFO.slot, crc);
        break;
      } //    for (j=0;j<UL_INFO.crc_ind.number_crcs;j++)
    } //   for (i=0;i<UL_INFO.rx_ind.number_of_pdus;i++)

    UL_INFO.crc_ind.number_crcs = 0;
    UL_INFO.rx_ind.number_of_pdus = 0;
  } else if (UL_INFO.rx_ind.number_of_pdus != 0
             || UL_INFO.crc_ind.number_crcs != 0) {
    LOG_E(NR_PHY,
          "hoping not to have mis-match between CRC ind and RX ind - "
          "hopefully the missing message is coming shortly "
          "rx_ind:%d(SFN/SL:%d/%d) crc_ind:%d(SFN/SL:%d/%d) \n",
          UL_INFO.rx_ind.number_of_pdus,
          UL_INFO.rx_ind.sfn,
          UL_INFO.rx_ind.slot,
          UL_INFO.crc_ind.number_crcs,
          UL_INFO.rx_ind.sfn,
          UL_INFO.rx_ind.slot);
  }
}

void NR_UL_indication(NR_UL_IND_t *UL_info) {
  AssertFatal(UL_info!=NULL,"UL_INFO is null\n");
#ifdef DUMP_FAPI
  dump_ul(UL_info);
#endif
  module_id_t      module_id   = UL_info->module_id;
  int              CC_id       = UL_info->CC_id;
  NR_Sched_Rsp_t   *sched_info = &Sched_INFO[module_id][CC_id];
  NR_IF_Module_t   *ifi        = if_inst[module_id];
  gNB_MAC_INST     *mac        = RC.nrmac[module_id];
  LOG_D(PHY,"SFN/SF:%d%d module_id:%d CC_id:%d UL_info[rach_pdus:%d rx_ind:%d crcs:%d]\n",
        UL_info->frame,UL_info->slot,
        module_id,CC_id, UL_info->rach_ind.number_of_pdus,
        UL_info->rx_ind.number_of_pdus, UL_info->crc_ind.number_crcs);

  if (NFAPI_MODE != NFAPI_MODE_PNF) {
    if (ifi->CC_mask==0) {
      ifi->current_frame    = UL_info->frame;
      ifi->current_slot = UL_info->slot;
    } else {
      AssertFatal(UL_info->frame != ifi->current_frame,"CC_mask %x is not full and frame has changed\n",ifi->CC_mask);
      AssertFatal(UL_info->slot != ifi->current_slot,"CC_mask %x is not full and slot has changed\n",ifi->CC_mask);
    }

    ifi->CC_mask |= (1<<CC_id);
  }

  handle_nr_rach(UL_info);

  handle_nr_uci(UL_info);

  // clear HI prior to handling ULSCH
  mac->UL_dci_req[CC_id].numPdus = 0;
  handle_nr_ulsch(UL_info);

  if (NFAPI_MODE != NFAPI_MODE_PNF) {
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

      LOG_D(PHY,"NR_Schedule_response: SFN_SF:%d%d dl_pdus:%d\n",
	    sched_info->frame,
	    sched_info->slot,
	    sched_info->DL_req->dl_tti_request_body.nPDUs);
    }
  }
}

NR_IF_Module_t *NR_IF_Module_init(int Mod_id) {
  AssertFatal(Mod_id<MAX_MODULES,"Asking for Module %d > %d\n",Mod_id,MAX_IF_MODULES);
  LOG_I(PHY,"Installing callbacks for IF_Module - UL_indication\n");

  if (if_inst[Mod_id]==NULL) {
    if_inst[Mod_id] = (NR_IF_Module_t*)malloc(sizeof(NR_IF_Module_t));
    memset((void*)if_inst[Mod_id],0,sizeof(NR_IF_Module_t));

    LOG_I(MAC,"Allocating shared L1/L2 interface structure for instance %d @ %p\n",Mod_id,if_inst[Mod_id]);

    if_inst[Mod_id]->CC_mask=0;
    if_inst[Mod_id]->NR_UL_indication = NR_UL_indication;
    AssertFatal(pthread_mutex_init(&if_inst[Mod_id]->if_mutex,NULL)==0,
                "allocation of if_inst[%d]->if_mutex fails\n",Mod_id);
  }

  return if_inst[Mod_id];
}
