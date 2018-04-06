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

/*! \file PHY/defs.h
 \brief Top-level defines and structure definitions
 \author R. Knopp, F. Kaltenberger
 \date 2011
 \version 0.1
 \company Eurecom
 \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr
 \note
 \warning
*/
#ifndef __PHY_DEFS_COMMON__H__
#define __PHY_DEFS_COMMON__H__

#define MAX_NUM_RU_PER_eNB 64
#define MAX_NUM_RU_PER_gNB 64

#define NUMBER_OF_SUBBANDS_MAX 13
#define NUMBER_OF_HARQ_PID_MAX 8

#define MAX_FRAME_NUMBER 0x400


#define NUMBER_OF_RN_MAX 3
typedef enum {no_relay=1,unicast_relay_type1,unicast_relay_type2, multicast_relay} relaying_type_t;



#define MCS_COUNT 28
#define MCS_TABLE_LENGTH_MAX 64


#define NUM_DCI_MAX 32

#define NUMBER_OF_eNB_SECTORS_MAX 3

#define NB_BANDS_MAX 8

#define MAX_BANDS_PER_RRU 4


#ifdef OCP_FRAMEWORK
#include <enums.h>
#else
typedef enum {normal_txrx=0,rx_calib_ue=1,rx_calib_ue_med=2,rx_calib_ue_byp=3,debug_prach=4,no_L2_connect=5,calib_prach_tx=6,rx_dump_frame=7,loop_through_memory=8} runmode_t;

/*! \brief Extension Type */
typedef enum {
  CYCLIC_PREFIX,
  CYCLIC_SUFFIX,
  ZEROS,
  NONE
} Extension_t;
	
enum transmission_access_mode {
  NO_ACCESS=0,
  POSTPONED_ACCESS,
  CANCELED_ACCESS,
  UNKNOWN_ACCESS,
  SCHEDULED_ACCESS,
  CBA_ACCESS};

typedef enum  {
  eNodeB_3GPP=0,   // classical eNodeB function
  NGFI_RAU_IF5,    // RAU with NGFI IF5
  NGFI_RAU_IF4p5,  // RAU with NFGI IF4p5
  NGFI_RRU_IF5,    // NGFI_RRU (NGFI remote radio-unit,IF5)
  NGFI_RRU_IF4p5,  // NGFI_RRU (NGFI remote radio-unit,IF4p5)
  MBP_RRU_IF5      // Mobipass RRU
} node_function_t;

typedef enum {

  synch_to_ext_device=0,  // synch to RF or Ethernet device
  synch_to_other,          // synch to another source_(timer, other RU)
  synch_to_mobipass_standalone  // special case for mobipass in standalone mode
} node_timing_t;
#endif

typedef struct {
  struct PHY_VARS_eNB_s *eNB;
  int UE_id;
  int harq_pid;
  int llr8_flag;
  int ret;
} td_params;

typedef struct {
  struct PHY_VARS_eNB_s *eNB;
  LTE_eNB_DLSCH_t *dlsch;
  int G;
  int harq_pid;
} te_params;

#endif
