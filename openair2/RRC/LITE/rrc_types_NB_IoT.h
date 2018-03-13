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

#ifndef RRC_TYPES_NB_IOT_H_
#define RRC_TYPES_NB_IOT_H_

typedef enum Rrc_State_NB_IoT_e {
  RRC_STATE_INACTIVE_NB_IoT=0,
  RRC_STATE_IDLE_NB_IoT,
  RRC_STATE_CONNECTED_NB_IoT,

  RRC_STATE_FIRST_NB_IoT = RRC_STATE_INACTIVE_NB_IoT,
  RRC_STATE_LAST_NB_IoT = RRC_STATE_CONNECTED_NB_IoT,
} Rrc_State_NB_IoT_t;

typedef enum Rrc_Sub_State_NB_IoT_e {
  RRC_SUB_STATE_INACTIVE_NB_IoT=0,

  RRC_SUB_STATE_IDLE_SEARCHING_NB_IoT,
  RRC_SUB_STATE_IDLE_RECEIVING_SIB_NB_IoT,
  RRC_SUB_STATE_IDLE_SIB_COMPLETE_NB_IoT,
  RRC_SUB_STATE_IDLE_CONNECTING_NB_IoT,
  RRC_SUB_STATE_IDLE_NB_IoT,

  RRC_SUB_STATE_CONNECTED_NB_IoT,

  RRC_SUB_STATE_INACTIVE_FIRST_NB_IoT = RRC_SUB_STATE_INACTIVE_NB_IoT,
  RRC_SUB_STATE_INACTIVE_LAST_NB_IoT = RRC_SUB_STATE_INACTIVE_NB_IoT,

  RRC_SUB_STATE_IDLE_FIRST_NB_IoT = RRC_SUB_STATE_IDLE_SEARCHING_NB_IoT,
  RRC_SUB_STATE_IDLE_LAST_NB_IoT = RRC_SUB_STATE_IDLE_NB_IoT,

  RRC_SUB_STATE_CONNECTED_FIRST_NB_IoT = RRC_SUB_STATE_CONNECTED_NB_IoT,
  RRC_SUB_STATE_CONNECTED_LAST_NB_IoT = RRC_SUB_STATE_CONNECTED_NB_IoT,
} Rrc_Sub_State_NB_IoT_t;

#endif /* RRC_TYPES_H_ */
