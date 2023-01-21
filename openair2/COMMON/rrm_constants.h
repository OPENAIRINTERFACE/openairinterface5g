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
                               rrm_constants.h
                             -------------------
  AUTHOR  : Lionel GAUTHIER
  COMPANY : EURECOM
  EMAIL   : Lionel.Gauthier@eurecom.fr



 ***************************************************************************/
#    ifndef __RRM_CONSTANTS_H__
#        define __RRM_CONSTANTS_H__
#include "common/platform_constants.h"

/*#################################################
  CONSTANTS FOR L1/L2/L3 LAYERS
#################################################*/
#        define RRM_CONFIG_VALID    0xFF
#        define RRM_CONFIG_INVALID  0x00
#        define RRM_NO_REFERENCE    -1
#        define TRANSACTION_NULL 0

// RRM action
/*
#        define ACTION_NULL    0
#        define ACTION_ADD     1
#        define ACTION_REMOVE  2
#        define ACTION_MODIFY  3
#        define ACTION_SET_SECURITY_MODE 4
#        define ACTION_MBMS_ADD 10
#        define ACTION_MBMS_MODIFY 11
*/

#        define MOBILE    7

// equipment type
#        define MOBILE_TERMINAL 1
#        define RADIO_GATEWAY   2


#        define UNUSED_PARAMETER -1
//---------------------------
// COMMON TRCH
//---------------------------
#        define PREFERED_RLC_BLOCK_SIZE 336
//(43 bytes)


#        define JRRM_MAX_TTI                       2
#        define JRRM_MAX_COMMANDS_PER_TRANSACTION  50

#    endif
