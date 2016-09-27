/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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
                                 platform_defines.h
                             -------------------
 ***************************************************************************/
#ifndef __PLATFORM_DEFINES_H__
#    define __PLATFORM_DEFINES_H__

// RLC_MODE
#    define RLC_NONE     0
#    define RLC_MODE_AM  1
#    define RLC_MODE_TM  2
#    define RLC_MODE_UM  3
#    define RLC_MODE_UM_BIDIRECTIONAL  3
#    define RLC_MODE_UM_UNIDIRECTIONAL_UL  4
#    define RLC_MODE_UM_UNIDIRECTIONAL_DL  5


#    define COMMAND_ACTION_NULL    0
#    define COMMAND_ACTION_ADD     1
#    define COMMAND_ACTION_REMOVE  2
#    define COMMAND_ACTION_MODIFY  3

#    define COMMAND_OBJECT_SIGNALLING_RADIO_BEARER 1
#    define COMMAND_OBJECT_DATA_RADIO_BEARER       2
#    define COMMAND_OBJECT_MOBILE                  3

#    define COMMAND_EQUIPMENT_MOBILE_TERMINAL 2
#    define COMMAND_EQUIPMENT_RADIO_GATEWAY   1

// ASN1 TYPES INCLUDES
#include "PhysCellId.h"
#include "TransactionId.h"
#include "OpenAir-RRM-Response-Reason.h"
#include "OpenAir-RRM-Response-Status.h"


typedef unsigned char                  class_of_service_t;
typedef unsigned char                  quality_t;
typedef unsigned int                   kbit_rate_t;
typedef unsigned int                   packet_size_t;

typedef unsigned char                  mobile_id_t;
typedef TransactionId_t                transaction_id_t;
typedef PhysCellId_t                   cell_id_t;
typedef unsigned char                  rb_id_t;

typedef unsigned char                  msg_type_t;
typedef unsigned short                 msg_length_t;
typedef unsigned long int              frame_t;
typedef OpenAir_RRM_Response_Status_t  msg_response_status_t;
typedef OpenAir_RRM_Response_Reason_t  msg_response_reason_t;

typedef unsigned char                  rlc_mode_t;
typedef unsigned char                  logical_channel_priority_t;
typedef unsigned char                  logical_channel_group_t;

typedef unsigned short                 t_reordering_t;

#endif
