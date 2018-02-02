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
 * phy_messages_types.h
 *
 *  Created on: Dec 12, 2013
 *      Author: winckel
 */

#ifndef PHY_MESSAGES_TYPES_H_
#define PHY_MESSAGES_TYPES_H_

#include "PHY/impl_defs_lte.h"
#if ENABLE_RAL
#include "ral_messages_types.h" //LG: MIH moved from repository/trunk to repository/extras
#endif
//-------------------------------------------------------------------------------------------//
// Defines to access message fields.
#define PHY_CONFIGURATION_REQ(mSGpTR)       (mSGpTR)->ittiMsg.phy_configuration_req

#define PHY_DEACTIVATE_REQ(mSGpTR)          (mSGpTR)->ittiMsg.phy_deactivate_req

#define PHY_FIND_CELL_REQ(mSGpTR)           (mSGpTR)->ittiMsg.phy_find_cell_req
#define PHY_FIND_NEXT_CELL_REQ(mSGpTR)      (mSGpTR)->ittiMsg.phy_find_next_cell_req

#define PHY_FIND_CELL_IND(mSGpTR)           (mSGpTR)->ittiMsg.phy_find_cell_ind

#define PHY_MEAS_THRESHOLD_REQ(mSGpTR)      (mSGpTR)->ittiMsg.phy_meas_threshold_req
#define PHY_MEAS_THRESHOLD_CONF(mSGpTR)     (mSGpTR)->ittiMsg.phy_meas_threshold_conf
#define PHY_MEAS_REPORT_IND(mSGpTR)         (mSGpTR)->ittiMsg.phy_meas_report_ind
//-------------------------------------------------------------------------------------------//
#define MAX_REPORTED_CELL   10

/* Enhance absolute radio frequency channel number */
typedef uint16_t    Earfcn;

/* Physical cell identity, valid value are from 0 to 503 */
typedef int16_t     PhyCellId;

/* Reference signal received power, valid value are from 0 (rsrp < -140 dBm) to 97 (rsrp <= -44 dBm) */
typedef int8_t      Rsrp;

/* Reference signal received quality, valid value are from 0 (rsrq < -19.50 dB) to 34 (rsrq <= -3 dB) */
typedef int8_t      Rsrq;

typedef struct CellInfo_s {
  Earfcn      earfcn;
  PhyCellId   cell_id;
  Rsrp        rsrp;
  Rsrq        rsrq;
} CellInfo;

//-------------------------------------------------------------------------------------------//
// eNB: ENB_APP -> PHY messages
typedef struct PhyConfigurationReq_s {
  lte_frame_type_t        frame_type[MAX_NUM_CCs];
  lte_prefix_type_t       prefix_type[MAX_NUM_CCs];
  uint32_t                downlink_frequency[MAX_NUM_CCs];
  int32_t                 uplink_frequency_offset[MAX_NUM_CCs];
  int32_t                 nb_antennas_tx[MAX_NUM_CCs];
  int32_t                 nb_antennas_rx[MAX_NUM_CCs];
  int32_t                 tx_gain[MAX_NUM_CCs];
  int32_t                 rx_gain[MAX_NUM_CCs];
} PhyConfigurationReq;

// UE: RRC -> PHY messages
typedef struct PhyDeactivateReq_s {
} PhyDeactivateReq;

typedef struct PhyFindCellReq_s {
#   if ENABLE_RAL
  ral_transaction_id_t    transaction_id;
#   endif
  Earfcn                  earfcn_start;
  Earfcn                  earfcn_end;
} PhyFindCellReq;

typedef struct PhyFindNextCellReq_s {
} PhyFindNextCellReq;

typedef struct PhyMeasThresholdReq_s {
#   if ENABLE_RAL
  ral_transaction_id_t    transaction_id;
  ral_link_cfg_param_t    cfg_param;
#endif
} PhyMeasThresholdReq;

typedef struct PhyMeasReportInd_s {
#   if ENABLE_RAL
  ral_threshold_t         threshold;
  ral_link_param_t        link_param;
#endif
} PhyMeasReportInd;

// UE: PHY -> RRC messages
typedef struct PhyFindCellInd_s {
#   if ENABLE_RAL
  ral_transaction_id_t    transaction_id;
#   endif
  uint8_t                  cell_nb;
  CellInfo                 cells[MAX_REPORTED_CELL];
} PhyFindCellInd;

typedef struct PhyMeasThresholdConf_s {
#   if ENABLE_RAL
  ral_transaction_id_t    transaction_id;
  ral_status_t            status;
  uint8_t                 num_link_cfg_params;
  ral_link_cfg_status_t   cfg_status[RAL_MAX_LINK_CFG_PARAMS];
#endif
} PhyMeasThresholdConf;
#endif /* PHY_MESSAGES_TYPES_H_ */
