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

/*! \file fapi_nr_l1.h
 * \brief function prototypes for FAPI L1 interface
 * \author R. Knopp, WEI-TAI CHEN
 * \date 2017, 2018
 * \version 0.1
 * \company Eurecom, NTUST
 * \email: knopp@eurecom.fr, kroempa@gmail.com
 * \note
 * \warning
 */

#include "PHY/defs_gNB.h"
#include "PHY/phy_extern.h"
#include "SCHED_NR/sched_nr.h"
#include "nfapi_nr_interface.h"
#include "nfapi_nr_interface_scf.h"

// added
void handle_nr_nfapi_ssb_pdu(PHY_VARS_gNB *gNB,
						int frame,int slot,
						nfapi_nr_dl_tti_request_pdu_t *dl_tti_pdu);

void nr_schedule_response(NR_Sched_Rsp_t *Sched_INFO);

void handle_nfapi_nr_csirs_pdu(PHY_VARS_gNB *gNB,
			       int frame, int slot,
			       nfapi_nr_dl_tti_csi_rs_pdu *csirs_pdu);

void handle_nfapi_nr_pdcch_pdu(PHY_VARS_gNB *gNB,
			       int frame, int subframe,
			       nfapi_nr_dl_tti_pdcch_pdu *dcl_dl_pdu);

void handle_nr_nfapi_pdsch_pdu(PHY_VARS_gNB *gNB,int frame,int slot,
			       nfapi_nr_dl_tti_pdsch_pdu *pdsch_pdu,
                            uint8_t *sdu);


void nr_fill_indication(PHY_VARS_gNB *gNB, int frame, int slot_rx, int UE_id, uint8_t harq_pid, uint8_t crc_flag,int dtx_flag);
//added

void handle_nfapi_nr_ul_dci_pdu(PHY_VARS_gNB *gNB,
			       int frame, int slot,
			       nfapi_nr_ul_dci_request_pdus_t *ul_dci_request_pdu);
