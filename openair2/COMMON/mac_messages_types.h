/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

/*
 * mac_messages_types.h
 *
 *  Created on: Oct 24, 2013
 *      Author: winckel and Navid Nikaein
 */

#ifndef MAC_MESSAGES_TYPES_H_
#define MAC_MESSAGES_TYPES_H_

//-------------------------------------------------------------------------------------------//
// Defines to access message fields.
#define RRC_MAC_IN_SYNC_IND(mSGpTR)             (mSGpTR)->ittiMsg.rrc_mac_in_sync_ind
#define RRC_MAC_OUT_OF_SYNC_IND(mSGpTR)         (mSGpTR)->ittiMsg.rrc_mac_out_of_sync_ind

#define RRC_MAC_BCCH_DATA_REQ(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_bcch_data_req
#define RRC_MAC_BCCH_DATA_IND(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_bcch_data_ind

#define RRC_MAC_CCCH_DATA_REQ(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_ccch_data_req
#define RRC_MAC_CCCH_DATA_CNF(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_ccch_data_cnf
#define RRC_MAC_CCCH_DATA_IND(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_ccch_data_ind

#define RRC_MAC_MCCH_DATA_REQ(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_mcch_data_req
#define RRC_MAC_MCCH_DATA_IND(mSGpTR)           (mSGpTR)->ittiMsg.rrc_mac_mcch_data_ind

// Some constants from "LAYER2/MAC/defs.h"
#define BCCH_SDU_SIZE                           (512)
#define CCCH_SDU_SIZE                           (512)
#define MCCH_SDU_SIZE                           (512)

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

#endif /* MAC_MESSAGES_TYPES_H_ */
