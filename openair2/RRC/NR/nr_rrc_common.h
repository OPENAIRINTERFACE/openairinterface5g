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

#ifndef __NR_RRC_COMMON_H__
#define __NR_RRC_COMMON_H__

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h> 

#define NR_RRC_HEADER_SIZE_MAX 64
#define NR_RRC_BUFFER_SIZE_MAX 1024
#define NR_NUM_SRB 4

typedef struct {
  char Payload[NR_RRC_BUFFER_SIZE_MAX];
  char Header[NR_RRC_HEADER_SIZE_MAX];
  uint16_t payload_size;
} NR_RRC_BUFFER;

typedef enum UE_STATE_NR_e {
  NR_RRC_INACTIVE=0,
  NR_RRC_IDLE,
  NR_RRC_SI_RECEIVED,
  NR_RRC_CONNECTED,
  NR_RRC_RECONFIGURED,
  NR_RRC_HO_EXECUTION
} NR_UE_STATE_t;

typedef struct {
  unsigned short transport_block_size; /*!< \brief Minimum PDU size in bytes provided by RLC to MAC layer interface */
  unsigned short max_transport_blocks; /*!< \brief Maximum PDU size in bytes provided by RLC to MAC layer interface */
  unsigned long Guaranteed_bit_rate; /*!< \brief Guaranteed Bit Rate (average) to be offered by MAC layer scheduling*/
  unsigned long Max_bit_rate; /*!< \brief Maximum Bit Rate that can be offered by MAC layer scheduling*/
  uint8_t Delay_class; /*!< \brief Delay class offered by MAC layer scheduling*/
  uint8_t Target_bler; /*!< \brief Target Average Transport Block Error rate*/
  uint8_t Lchan_t; /*!< \brief Logical Channel Type (BCCH,CCCH,DCCH,DTCH_B,DTCH,MRBCH)*/
} __attribute__ ((__packed__))  NR_LCHAN_DESC;

typedef struct RB_INFO_NR_s {
  uint16_t Rb_id; //=Lchan_id
  NR_LCHAN_DESC Lchan_desc[2];
  //MAC_MEAS_REQ_ENTRY *Meas_entry; //may not needed for NB-IoT
} NR_RB_INFO;

typedef struct SRB_INFO_TABLE_ENTRY_NR_s {
  uint8_t Active;
  uint8_t status;
} NR_SRB_INFO_TABLE_ENTRY;

void rrc_init_nr_srb_param(NR_LCHAN_DESC *chan);

#endif
