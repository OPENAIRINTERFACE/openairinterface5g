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

/*! \file vars.h
* \brief specifies the variables for phy emulation
* \author Navid Nikaein and Raymomd Knopp and Hicham Anouar
* \date 2011
* \version 1.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
*/

#include <pthread.h>
#include "defs.h"

#ifndef __BYPASS_SESSION_LAYER_VARS_H__
#    define __BYPASS_SESSION_LAYER_VARS_H__

unsigned char Emulation_status;
unsigned char emu_tx_status;
unsigned char emu_rx_status;
//unsigned int Master_list=0;
//unsigned short Master_id;
//unsigned int Is_primary_master;

#if !defined(ENABLE_NEW_MULTICAST)
pthread_mutex_t emul_low_mutex;
pthread_cond_t emul_low_cond;
char emul_low_mutex_var;
pthread_mutex_t Tx_mutex;
pthread_cond_t Tx_cond;
char Tx_mutex_var;
#endif

/* Handlers for RX and TX */
rx_handler_t rx_handler;
tx_handler_t tx_handler;

eNB_transport_info_t eNB_transport_info[NUMBER_OF_eNB_MAX][MAX_NUM_CCs];
uint16_t eNB_transport_info_TB_index[NUMBER_OF_eNB_MAX][MAX_NUM_CCs];

UE_transport_info_t UE_transport_info[NUMBER_OF_UE_MAX][MAX_NUM_CCs];
uint16_t UE_transport_info_TB_index[NUMBER_OF_UE_MAX][MAX_NUM_CCs];

UE_cntl ue_cntl_delay[NUMBER_OF_UE_MAX][MAX_NUM_CCs][2];

#endif
