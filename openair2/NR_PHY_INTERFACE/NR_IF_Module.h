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

#include <stdint.h>
#include "nfapi_nr_interface.h"
#include "platform_constants.h"
#include "platform_types.h"

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

typedef struct{
  /// Module ID
  module_id_t module_id;
  /// CC ID
  int CC_id;
  /// frame 
  frame_t frame;
  /// subframe
  sub_frame_t subframe;

  /// harq indication list
  nfapi_harq_indication_t harq_ind;

  /// crc indication list
  nfapi_crc_indication_t crc_ind;

  /// SR indication list
  nfapi_sr_indication_t sr_ind;

  /// CQI indication list
  nfapi_cqi_indication_body_t cqi_ind;

  /// RACH indication list
  nfapi_rach_indication_t rach_ind;

#if (RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  /// RACH indication list for BR UEs
  nfapi_rach_indication_t rach_ind_br;
#endif

  /// SRS indication list
  nfapi_srs_indication_body_t srs_ind;

  /// RX indication
  nfapi_rx_indication_t rx_ind;

} NR_UL_IND_t;

// Downlink subframe P7


typedef struct{
  /// Module ID
  module_id_t module_id; 
  /// CC ID
  uint8_t CC_id;
  /// frame
  frame_t frame;
  /// subframe
  sub_frame_t subframe;
  /// nFAPI DL Config Request
  nfapi_nr_dl_config_request_t *DL_req;
  /// nFAPI UL Config Request
  nfapi_ul_config_request_t *UL_req;
  /// nFAPI HI_DCI Request
  nfapi_hi_dci0_request_t *HI_DCI0_req;
  /// Pointers to DL SDUs
  nfapi_tx_request_t *TX_req;
} NR_Sched_Rsp_t;

typedef struct {
    uint8_t Mod_id;
    int CC_id;
    nfapi_nr_config_request_t *cfg;
} NR_PHY_Config_t;

typedef struct NR_IF_Module_s{
//define the function pointer
  void (*NR_UL_indication)(NR_UL_IND_t *UL_INFO);
  void (*NR_Schedule_response)(NR_Sched_Rsp_t *Sched_INFO);
  void (*NR_PHY_config_req)(NR_PHY_Config_t* config_INFO);
  uint32_t CC_mask;
  uint16_t current_frame;
  uint8_t current_subframe;
  pthread_mutex_t if_mutex;
} NR_IF_Module_t;

/*Initial */
NR_IF_Module_t *NR_IF_Module_init(int Mod_id);

void NR_IF_Module_kill(int Mod_id);

void NR_UL_indication(NR_UL_IND_t *UL_INFO);

/*Interface for Downlink, transmitting the DLSCH SDU, DCI SDU*/
void NR_Schedule_Response(NR_Sched_Rsp_t *Sched_INFO);

#endif /*_NFAPI_INTERFACE_NR_H_*/