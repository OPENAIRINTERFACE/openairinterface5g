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
#include "PHY/LTE_TRANSPORT/transport_proto.h"
#include "SCHED/sched_eNB.h"
#include "SCHED/sched_common.h"
#include "SCHED_NR/sched_nr.h"
#include "nfapi_nr_interface.h"

void nr_schedule_response(NR_Sched_Rsp_t *Sched_INFO);
void handle_nfapi_nr_dci_dl_pdu(PHY_VARS_gNB *gNB,
                                int frame, int subframe,
                                gNB_rxtx_proc_t *proc,
                                nfapi_nr_dl_config_request_pdu_t *dl_config_pdu);
