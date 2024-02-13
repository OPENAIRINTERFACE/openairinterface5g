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

/*! \file openair2/NR_PHY_INTERFACE/NR_IF_Module.h
* \brief data structures for PHY/MAC interface modules
* \author EURECOM/NTUST
* \date 2018
* \version 0.1
* \company Eurecom, NTUST
* \email: raymond.knopp@eurecom.fr, kroempa@gmail.com
* \note
* \warning
*/

#ifndef __NR_IF_MODULE__H__
#define __NR_IF_MODULE__H__

#include <pthread.h>
#include <stdint.h>
#include "nfapi_nr_interface.h"
#include "nfapi_nr_interface_scf.h"
#include "common/platform_constants.h"
#include "common/platform_types.h"

#define MAX_NUM_DL_PDU 100
#define MAX_NUM_UL_PDU 100
#define MAX_NUM_HI_DCI0_PDU 100
#define MAX_NUM_TX_REQUEST_PDU 100

#define MAX_NUM_HARQ_IND 100
#define MAX_NUM_CRC_IND 100
#define MAX_NUM_SR_IND 100
#define MAX_NUM_CQI_IND 100
#define MAX_NUM_RACH_IND 100
#define MAX_NUM_SRS_IND 100

typedef struct {
  /// Module ID
  module_id_t module_id;
  /// CC ID
  int CC_id;
  /// frame
  frame_t frame;
  /// slot
  slot_t slot;

  /// crc indication list
  nfapi_nr_crc_indication_t crc_ind;

  /// RACH indication list
  nfapi_nr_rach_indication_t rach_ind;

  /// SRS indication list
  nfapi_nr_srs_indication_t srs_ind;

  /// RX indication
  nfapi_nr_rx_data_indication_t rx_ind;

  /// UCI indication
  nfapi_nr_uci_indication_t uci_ind;

} NR_UL_IND_t;

// Downlink slot P7


typedef struct {
  /// the ID of this sched_response - used by sched_reponse memory management
  int sched_response_id;
  /// Module ID
  module_id_t module_id;
  /// CC ID
  uint8_t CC_id;
  /// frame
  frame_t frame;
  /// slot
  slot_t slot;
  /// nFAPI DL Config Request
  nfapi_nr_dl_tti_request_t DL_req;
  /// nFAPI UL Config Request
  nfapi_nr_ul_tti_request_t UL_tti_req;
  /// nFAPI UL_DCI Request
  nfapi_nr_ul_dci_request_t UL_dci_req;
  /// Pointers to DL SDUs
  nfapi_nr_tx_data_request_t TX_req;
} NR_Sched_Rsp_t;

typedef struct {
  uint8_t Mod_id;
  int CC_id;
  nfapi_nr_config_request_scf_t *cfg;
} NR_PHY_Config_t;

typedef struct NR_IF_Module_s {
  //define the function pointer
  void (*NR_UL_indication)(NR_UL_IND_t *UL_INFO);
  void (*NR_Schedule_response)(NR_Sched_Rsp_t *Sched_INFO);
  void (*NR_slot_indication)(module_id_t module_idP, int CC_id, int frame, int slot);
  void (*NR_PHY_config_req)(NR_PHY_Config_t *config_INFO);
  uint32_t CC_mask;
  uint16_t current_frame;
  uint8_t current_slot;
  pthread_mutex_t if_mutex;
  int sl_ahead;
} NR_IF_Module_t;

/*Initial */
NR_IF_Module_t *NR_IF_Module_init(int Mod_id);

void NR_IF_Module_kill(int Mod_id);

void NR_UL_indication(NR_UL_IND_t *UL_INFO);

void RCconfig_nr_ue_macrlc(void);

/*Interface for Downlink, transmitting the DLSCH SDU, DCI SDU*/
void NR_Schedule_Response(NR_Sched_Rsp_t *Sched_INFO);

#endif /*_NFAPI_INTERFACE_NR_H_*/
