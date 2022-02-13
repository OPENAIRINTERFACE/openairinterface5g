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

/*! \file rrc_types.h
* \brief rrc types and subtypes
* \author Navid Nikaein and Raymond Knopp
* \date 2011 - 2014
* \version 1.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr, raymond.knopp@eurecom.fr
*/

#ifndef RRC_TYPES_H_
#define RRC_TYPES_H_

typedef enum Rrc_State_e {
  RRC_STATE_INACTIVE=0,
  RRC_STATE_IDLE,
  RRC_STATE_CONNECTED,

  RRC_STATE_FIRST = RRC_STATE_INACTIVE,
  RRC_STATE_LAST = RRC_STATE_CONNECTED,
} Rrc_State_t;

typedef enum Rrc_Sub_State_e {
  RRC_SUB_STATE_INACTIVE=0,

  RRC_SUB_STATE_IDLE_SEARCHING,
  RRC_SUB_STATE_IDLE_RECEIVING_SIB,
  RRC_SUB_STATE_IDLE_SIB_COMPLETE,
  RRC_SUB_STATE_IDLE_CONNECTING,
  RRC_SUB_STATE_IDLE,

  RRC_SUB_STATE_CONNECTED,

  RRC_SUB_STATE_INACTIVE_FIRST = RRC_SUB_STATE_INACTIVE,
  RRC_SUB_STATE_INACTIVE_LAST = RRC_SUB_STATE_INACTIVE,

  RRC_SUB_STATE_IDLE_FIRST = RRC_SUB_STATE_IDLE_SEARCHING,
  RRC_SUB_STATE_IDLE_LAST = RRC_SUB_STATE_IDLE,

  RRC_SUB_STATE_CONNECTED_FIRST = RRC_SUB_STATE_CONNECTED,
  RRC_SUB_STATE_CONNECTED_LAST = RRC_SUB_STATE_CONNECTED,
} Rrc_Sub_State_t;

typedef enum Rrc_Msg_Type_e {
  UE_CAPABILITY_DUMMY = 0xa0,
  UE_CAPABILITY_ENQUIRY,
  NRUE_CAPABILITY_ENQUIRY,
  UE_CAPABILITY_INFO,
  NRUE_CAPABILITY_INFO,
  RRC_MEASUREMENT_PROCEDURE,
  NR_UE_RRC_MEASUREMENT,
  RRC_CONFIG_COMPLETE_REQ,
  NR_RRC_CONFIG_COMPLETE_REQ,
  OAI_TUN_IFACE_NSA
} Rrc_Msg_Type_t;


#endif /* RRC_TYPES_H_ */
