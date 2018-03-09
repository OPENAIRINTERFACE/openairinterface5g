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

/*! \file fapi_l1.h
 * \brief function prototypes for FAPI L1 interface
 * \author R. Knopp
 * \date 2017
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */

#include "PHY/defs.h"
#include "PHY/extern.h"
#include "SCHED/defs.h"
#include "SCHED/extern.h"
#include "nfapi_interface.h"

void fill_uci_harq_indication(PHY_VARS_eNB *eNB,LTE_eNB_UCI *uci,int frame,int subframe,uint8_t *harq_ack,uint8_t tdd_mapping_mode,uint16_t tdd_multiplexing_mask);
void fill_ulsch_harq_indication(PHY_VARS_eNB *eNB,LTE_UL_eNB_HARQ_t *ulsch_harq,uint16_t rnti, int frame,int subframe,int bundling);
void fill_ulsch_cqi_indication(PHY_VARS_eNB *eNB,uint16_t frame,uint8_t subframe,LTE_UL_eNB_HARQ_t *ulsch_harq,uint16_t rnti);
void fill_sr_indication(PHY_VARS_eNB *eNB,uint16_t rnti,int frame,int subframe,uint32_t stat);
void fill_rx_indication(PHY_VARS_eNB *eNB,int UE_id,int frame,int subframe);
void fill_crc_indication(PHY_VARS_eNB *eNB,int UE_id,int frame,int subframe,uint8_t crc_flag);
void handle_nfapi_dci_dl_pdu(PHY_VARS_eNB *eNB,int frame,int subframe,eNB_rxtx_proc_t *proc,nfapi_dl_config_request_pdu_t *dl_config_pdu);
void handle_nfapi_mpdcch_pdu(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc,nfapi_dl_config_request_pdu_t *dl_config_pdu);
void handle_nfapi_hi_dci0_dci_pdu(PHY_VARS_eNB *eNB,int frame,int subframe,eNB_rxtx_proc_t *proc,
				  nfapi_hi_dci0_request_pdu_t *hi_dci0_config_pdu);
void handle_nfapi_hi_dci0_hi_pdu(PHY_VARS_eNB *eNB,int frame,int subframe,eNB_rxtx_proc_t *proc,
				 nfapi_hi_dci0_request_pdu_t *hi_dci0_config_pdu);
void handle_nfapi_dlsch_pdu(PHY_VARS_eNB *eNB,int frame,int subframe,eNB_rxtx_proc_t *proc,
			    nfapi_dl_config_request_pdu_t *dl_config_pdu,
			    uint8_t codeword_index,
			    uint8_t *sdu);
void handle_nfapi_ul_pdu(PHY_VARS_eNB *eNB,eNB_rxtx_proc_t *proc,
			 nfapi_ul_config_request_pdu_t *ul_config_pdu,
			 uint16_t frame,uint8_t subframe,uint8_t srs_present);

void handle_ulsch_harq_pdu(
        PHY_VARS_eNB                           *eNB,
        int                                     UE_id,
        nfapi_ul_config_request_pdu_t          *ul_config_pdu,
        nfapi_ul_config_ulsch_harq_information *harq_information,
        uint16_t                                frame,
        uint8_t                                 subframe);

void handle_ulsch_cqi_ri_pdu(PHY_VARS_eNB *eNB,int UE_id,nfapi_ul_config_request_pdu_t *ul_config_pdu,uint16_t frame,uint8_t subframe);

void handle_uci_harq_information(PHY_VARS_eNB *eNB, LTE_eNB_UCI *uci,nfapi_ul_config_harq_information *harq_information);

void handle_uci_sr_pdu(PHY_VARS_eNB *eNB,int UE_id,nfapi_ul_config_request_pdu_t *ul_config_pdu,uint16_t frame,uint8_t subframe,uint8_t srs_active);

void handle_uci_sr_harq_pdu(PHY_VARS_eNB *eNB,int UE_id,nfapi_ul_config_request_pdu_t *ul_config_pdu,uint16_t frame,uint8_t subframe,uint8_t srs_active);
void handle_uci_harq_pdu(PHY_VARS_eNB *eNB,int UE_id,nfapi_ul_config_request_pdu_t *ul_config_pdu,uint16_t frame,uint8_t subframe,uint8_t srs_active);

void handle_srs_pdu(PHY_VARS_eNB *eNB,nfapi_ul_config_request_pdu_t *ul_config_pdu,uint16_t frame,uint8_t subframe);

void schedule_response(Sched_Rsp_t *Sched_INFO);
