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

/*! \file defs.h
 *  \brief specifies the data structure and variable for phy emulation
 *  \author Navid Nikaein, Raymomd Knopp  and Hicham Anouar
 *  \date 2011
 *  \version 1.1
 *  \company Eurecom
 *  \email: navid.nikaein@eurecom.fr
 */

#ifndef __BYPASS_SESSION_LAYER_DEFS_H__
#    define __BYPASS_SESSION_LAYER_DEFS_H__
//-----------------------------------------------------------------------------
//#include "openair_defs.h"
#include "common/openairinterface5g_limits.h"
#include "PHY/defs_common.h"
#include "PHY/LTE_TRANSPORT/uci_common.h"

typedef enum emu_transport_info_e {
  EMU_TRANSPORT_INFO_ERROR    = 0x0,
  EMU_TRANSPORT_INFO_WAIT_PM,
  EMU_TRANSPORT_INFO_WAIT_SM,
  EMU_TRANSPORT_INFO_SYNC,
  EMU_TRANSPORT_INFO_ENB,
  EMU_TRANSPORT_INFO_UE,
  EMU_TRANSPORT_INFO_RELEASE
} emu_transport_info_t;

#define WAIT_PM_TRANSPORT 1
#define WAIT_SM_TRANSPORT 2
#define SYNC_TRANSPORT 3
#define ENB_TRANSPORT 4
#define UE_TRANSPORT 5
#define RELEASE_TRANSPORT 6

#define WAIT_SYNC_TRANSPORT 1
#define SYNCED_TRANSPORT 2
//#define WAIT_ENB_TRANSPORT 2
//#define WAIT_UE_TRANSPORT 3


#define BYPASS_RX_BUFFER_SIZE 64000
#define BYPASS_TX_BUFFER_SIZE 64000

typedef unsigned int (*tx_handler_t) (unsigned char, char*, unsigned int*, unsigned int*);
typedef unsigned int (*rx_handler_t) (unsigned char, char*, unsigned int);

/*************************************************************/

typedef struct eNB_cntl_s {
  uint32_t pbch_flag:1;
  uint32_t pmch_flag:1;
  uint32_t pss:2;
  uint32_t sss:8;
  uint32_t cfi:2;
  uint32_t phich:19; // max 200 bit
  //  uint32_t pbch_payload:24;
  uint32_t pbch_payload;
} eNB_cntl;

typedef struct UE_cntl_s {
  uint8_t pucch_flag:3;  // 0,7 = none, 1 = type 1, 2=type 1a, 3=type 1b, 4=type 2, 5=type 2a, 6=type 2b
  uint8_t pucch_Ncs1:3;  // physical configuration of pucch, for abstraction purposes
  uint32_t pucch_payload:21;        // ack/nak/cqi information
  uint8_t sr:1;
  uint8_t pusch_flag:1;  // 0=none,1=active
  uint8_t pucch_sel:1;
  //uint32_t pusch_uci;     // uci information on pusch
  uint8_t pusch_uci[MAX_CQI_BYTES];
  uint8_t uci_format;
  uint8_t length_uci;
  uint8_t pusch_ri:2;    // ri information on pusch
  uint8_t pusch_ack:2;   // ack/nak on pusch
  uint8_t prach_flag:1;  // 0=none,1=active
  uint8_t prach_id:6;    // this is the PHY preamble index for the prach
} UE_cntl;

#define MAX_TRANSPORT_BLOCKS_BUFFER_SIZE 16384
//#define MAX_PMCH_TRANSPORT_BLOCKS_BUFFER_SIZE 8192 // 16384
#define MAX_NUM_DCI 5+1 // +1: for PMCH/MCH

typedef struct eNB_transport_info_s {
  eNB_cntl cntl;
  uint8_t num_pmch;
  uint8_t num_common_dci;
  uint8_t num_ue_spec_dci;
  DCI_ALLOC_t dci_alloc[MAX_NUM_DCI];
  uint8_t dlsch_type[MAX_NUM_DCI];
  uint8_t harq_pid[MAX_NUM_DCI];
  uint8_t ue_id[MAX_NUM_DCI];
  uint16_t tbs[MAX_NUM_DCI*2];    // times 2 for dual-stream MIMO formats
  uint8_t transport_blocks[MAX_TRANSPORT_BLOCKS_BUFFER_SIZE];
  //uint8_t pmch_transport_blocks[MAX_PMCH_TRANSPORT_BLOCKS_BUFFER_SIZE];
} __attribute__((__packed__)) eNB_transport_info_t ;

typedef struct UE_transport_info_s {
  UE_cntl cntl;
  uint8_t num_eNB;
  uint16_t rnti[NUMBER_OF_CONNECTED_eNB_MAX];
  uint8_t eNB_id[NUMBER_OF_CONNECTED_eNB_MAX];
  uint8_t harq_pid[NUMBER_OF_CONNECTED_eNB_MAX];
  uint16_t tbs[NUMBER_OF_CONNECTED_eNB_MAX];
  uint8_t transport_blocks[MAX_TRANSPORT_BLOCKS_BUFFER_SIZE];//*NUMBER_OF_CONNECTED_eNB_MAX];
} __attribute__((__packed__)) UE_transport_info_t ;

/*! \brief */
typedef struct bypass_msg_header_s {
  unsigned char  Message_type; /*! \brief control or data*/
  //unsigned char  nb_master; /*! \brief */
  unsigned char  master_id; /*! \brief */
  unsigned int   nb_enb; /*! \brief */
  unsigned int   nb_ue; /*! \brief */
  unsigned int   nb_flow; /*! \brief */
  unsigned int   frame;
  unsigned int   subframe;
  uint64_t       seq_num;
  unsigned int   failing_master_id;
} __attribute__((__packed__)) bypass_msg_header_t;

typedef struct bypass_proto2multicast_header_s {
  unsigned int      size;
} bypass_proto2multicast_header_t;

#endif /* __BYPASS_SESSION_LAYER_DEFS_H__ */
