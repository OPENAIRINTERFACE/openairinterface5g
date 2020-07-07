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
 * mac_messages_types.h
 *
 *  Created on: Oct 24, 2013
 *      Author: winckel and Navid Nikaein
 */

#ifndef MAC_MESSAGES_TYPES_H_
#define MAC_MESSAGES_TYPES_H_

#include <LTE_DRX-Config.h>

//-------------------------------------------------------------------------------------------//
// Defines to access message fields.
#define RRC_MAC_IN_SYNC_IND(mSGpTR)             (mSGpTR)->ittiMsg.rrc_mac_in_sync_ind
#define RRC_MAC_OUT_OF_SYNC_IND(mSGpTR)         (mSGpTR)->ittiMsg.rrc_mac_out_of_sync_ind

#define RRC_MAC_BCCH_DATA_REQ(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_bcch_data_req
#define RRC_MAC_BCCH_DATA_IND(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_bcch_data_ind

#define RRC_MAC_BCCH_MBMS_DATA_REQ(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_bcch_mbms_data_req
#define RRC_MAC_BCCH_MBMS_DATA_IND(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_bcch_mbms_data_ind

#define RRC_MAC_CCCH_DATA_REQ(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_ccch_data_req
#define RRC_MAC_CCCH_DATA_CNF(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_ccch_data_cnf
#define RRC_MAC_CCCH_DATA_IND(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_ccch_data_ind

#define RRC_MAC_MCCH_DATA_REQ(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_mcch_data_req
#define RRC_MAC_MCCH_DATA_IND(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_mcch_data_ind
#define RRC_MAC_PCCH_DATA_REQ(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_pcch_data_req

#define RRC_MAC_DRX_CONFIG_REQ(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_drx_config_req

// Some constants from "LAYER2/MAC/defs.h"
#define BCCH_SDU_SIZE                           (512)
#define BCCH_SDU_MBMS_SIZE                      (512)
#define CCCH_SDU_SIZE                           (512)
#define MCCH_SDU_SIZE                           (512)
#define PCCH_SDU_SIZE                           (512)

//-------------------------------------------------------------------------------------------//
// Messages between RRC and MAC layers
typedef struct RrcMacInSyncInd_s {
  uint32_t  frame;
  uint8_t   sub_frame;
  uint16_t  enb_index;
} RrcMacInSyncInd;

typedef RrcMacInSyncInd RrcMacOutOfSyncInd;

typedef struct RrcMacBcchDataReq_s {
  uint32_t  frame;
  uint32_t  sdu_size;
  uint8_t   sdu[BCCH_SDU_SIZE];
  uint8_t   enb_index;
} RrcMacBcchDataReq;

typedef struct RrcMacBcchDataInd_s {
  uint32_t  frame;
  uint8_t   sub_frame;
  uint32_t  sdu_size;
  uint8_t   sdu[BCCH_SDU_SIZE];
  uint8_t   enb_index;
  uint8_t   rsrq;
  uint8_t   rsrp;
} RrcMacBcchDataInd;


typedef struct RrcMacBcchMbmsDataReq_s {
  uint32_t  frame;
  uint32_t  sdu_size;
  uint8_t   sdu[BCCH_SDU_MBMS_SIZE];
  uint8_t   enb_index;
} RrcMacBcchMbmsDataReq;

typedef struct RrcMacBcchMbmsDataInd_s {
  uint32_t  frame;
  uint8_t   sub_frame;
  uint32_t  sdu_size;
  uint8_t   sdu[BCCH_SDU_MBMS_SIZE];
  uint8_t   enb_index;
  uint8_t   rsrq;
  uint8_t   rsrp;
} RrcMacBcchMbmsDataInd;


typedef struct RrcMacCcchDataReq_s {
  uint32_t  frame;
  uint32_t  sdu_size;
  uint8_t   sdu[CCCH_SDU_SIZE];
  uint8_t   enb_index;
} RrcMacCcchDataReq;

typedef struct RrcMacCcchDataCnf_s {
  uint8_t   enb_index;
} RrcMacCcchDataCnf;

typedef struct RrcMacCcchDataInd_s {
  uint32_t  frame;
  uint8_t   sub_frame;
  uint16_t  rnti;
  uint32_t  sdu_size;
  uint8_t   sdu[CCCH_SDU_SIZE];
  uint8_t   enb_index;
  int       CC_id;
} RrcMacCcchDataInd;

typedef struct RrcMacMcchDataReq_s {
  uint32_t  frame;
  uint32_t  sdu_size;
  uint8_t   sdu[MCCH_SDU_SIZE];
  uint8_t   enb_index;
  uint8_t   mbsfn_sync_area;
} RrcMacMcchDataReq;

typedef struct RrcMacMcchDataInd_s {
  uint32_t  frame;
  uint8_t   sub_frame;
  uint32_t  sdu_size;
  uint8_t   sdu[MCCH_SDU_SIZE];
  uint8_t   enb_index;
  uint8_t   mbsfn_sync_area;
} RrcMacMcchDataInd;

typedef struct RrcMacPcchDataReq_s {
  uint32_t  frame;
  uint32_t  sdu_size;
  uint8_t   sdu[PCCH_SDU_SIZE];
  uint8_t   enb_index;
} RrcMacPcchDataReq;

/* RRC configures DRX context (MAC timers) of a UE */
typedef struct rrc_mac_drx_config_req_s {
  /* UE RNTI to configure */
  rnti_t rnti;

  /* DRX configuration from MacMainConfig to configure UE's local timers */
  LTE_DRX_Config_t * drx_Configuration;
} rrc_mac_drx_config_req_t;

#endif /* MAC_MESSAGES_TYPES_H_ */
