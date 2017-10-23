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

/*! \file rrh_gw_extern.h
 * \brief rrh gatewy external vars
 * \author Navid Nikaein,  Katerina Trilyraki, Raymond Knopp
 * \date 2015
 * \version 0.1
 * \company Eurecom
 * \maintainer:  navid.nikaein@eurecom.fr
 * \note
 * \warning very experimental 
 */

#ifndef RRH_GW_EXTERNS_H_
#define RRH_GW_EXTERNS_H_
extern char   rf_config_file[1024];

extern openair0_timestamp 	timestamp_UE_tx[4] ,timestamp_UE_rx[4] ,timestamp_eNB_rx[4],timestamp_eNB_tx[4];
extern openair0_vtimestamp 	hw_counter;
extern int32_t			UE_tx_started,UE_rx_started,eNB_rx_started ,eNB_tx_started;
extern int32_t			nsamps_UE[4],nsamps_eNB[4];
extern int32_t			overflow_rx_buffer_eNB[4],overflow_rx_buffer_UE[4];
extern uint8_t			rrh_exit;
extern int32_t			**rx_buffer_eNB, **rx_buffer_UE;
extern unsigned int	        rt_period;
extern pthread_mutex_t          timer_mutex;


#endif 
