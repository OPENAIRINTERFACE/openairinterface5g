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

/*
  \author Guy De Souza
  \company EURECOM
  \email desouza@eurecom.fr
*/

#ifndef __openair_SCHED_NR_DEFS_H__
#define __openair_SCHED_NR_DEFS_H__

#include "PHY/defs_gNB.h"
#include "PHY/NR_TRANSPORT/nr_dci.h"
#include "phy_frame_config_nr.h"

void fill_ul_rb_mask(PHY_VARS_gNB *gNB, int frame_rx, int slot_rx);
void nr_set_ssb_first_subcarrier(nfapi_nr_config_request_scf_t *cfg, NR_DL_FRAME_PARMS *fp);
void phy_procedures_gNB_TX(processingData_L1tx_t *msgTx, int frame_tx, int slot_tx, int do_meas);
int  phy_procedures_gNB_uespec_RX(PHY_VARS_gNB *gNB, int frame_rx, int slot_rx);
void L1_nr_prach_procedures(PHY_VARS_gNB *gNB,int frame,int slot);
void nr_common_signal_procedures (PHY_VARS_gNB *gNB,int frame,int slot,nfapi_nr_dl_tti_ssb_pdu ssb_pdu);
void nr_feptx_ofdm(RU_t *ru,int frame_tx,int tti_tx);
void nr_feptx0(RU_t *ru,int tti_tx,int first_symbol, int num_symbols, int aa);

void fep_full(RU_t *ru,int slot);
void nr_feptx_prec(RU_t *ru,int frame_tx,int tti_tx);
void nr_feptx_prec_control(RU_t *ru,int frame,int tti_tx);
void nr_fep_full(RU_t *ru, int slot);
void nr_fep_tp(RU_t *ru, int slot);
void nr_feptx_tp(RU_t *ru, int frame_tx, int slot);
void feptx_prec(RU_t *ru,int frame_tx,int tti_tx);
int nr_phy_init_RU(RU_t *ru);
void nr_phy_free_RU(RU_t *ru);

#endif
