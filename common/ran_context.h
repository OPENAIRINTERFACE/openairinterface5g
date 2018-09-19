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

/*! \file PHY/impl_defs_lte.h
* \brief LTE Physical channel configuration and variable structure definitions
* \author R. Knopp, F. Kaltenberger
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr
* \note
* \warning
*/

#ifndef __RAN_CONTEXT_H__
#define __RAN_CONTEXT_H__

#include <pthread.h>
#include "COMMON/platform_constants.h"
#include "PHY/defs_eNB.h"
#include "PHY/types.h"
#include "PHY/impl_defs_top.h"
#include "PHY/impl_defs_lte.h"

#include "ENB_APP/enb_config.h"
#include "RRC/LTE/rrc_defs.h"
#include "flexran_agent_defs.h"

#include "gtpv1u.h"
#include "NwGtpv1u.h"
#include "NwGtpv1uMsg.h"
#include "NwGtpv1uPrivate.h"
#include "gtpv1u_eNB_defs.h"

#include "PHY/defs_L1_NB_IoT.h"

#include "RRC/LTE/defs_NB_IoT.h"



typedef struct {
  /// RAN context config file name
  char *config_file_name;
  /// Number of RRC instances in this node
  int nb_inst;
  /// Number of Component Carriers per instance in this node
  int *nb_CC;
  /// Number of NB_IoT instances in this node
  int nb_nb_iot_rrc_inst;
  /// Number of MACRLC instances in this node
  int nb_macrlc_inst;
  /// Number of NB_IoT MACRLC instances in this node
  int nb_nb_iot_macrlc_inst;
  /// Number of component carriers per instance in this node
  int *nb_mac_CC;
  /// Number of L1 instances in this node
  int nb_L1_inst;
  /// Number of NB_IoT L1 instances in this node
  int nb_nb_iot_L1_inst;
  /// Number of Component Carriers per instance in this node
  int *nb_L1_CC;
  /// Number of RU instances in this node
  int nb_RU;
  /// FlexRAN context variables
  flexran_agent_info_t **flexran;
  /// eNB context variables
  struct PHY_VARS_eNB_s ***eNB;
  /// NB_IoT L1 context variables
  struct PHY_VARS_eNB_NB_IoT_s **L1_NB_IoT;
  /// RRC context variables
  struct eNB_RRC_INST_s **rrc;
  /// NB_IoT RRC context variables
  //struct eNB_RRC_INST_NB_IoT_s **nb_iot_rrc;
  /// MAC context variables
  struct eNB_MAC_INST_s **mac;
  /// NB_IoT MAC context variables
  struct eNB_MAC_INST_NB_IoT_s **nb_iot_mac;
  /// GTPu descriptor 
  gtpv1u_data_t *gtpv1u_data_g;
  /// RU descriptors. These describe what each radio unit is supposed to do and contain the necessary functions for fronthaul interfaces
  struct RU_t_s **ru;
  /// Mask to indicate fronthaul setup status of RU (hard-limit to 64 RUs)
  uint64_t ru_mask;
  /// Mutex for protecting ru_mask
  pthread_mutex_t ru_mutex;
  /// condition variable for signaling setup completion of an RU
  pthread_cond_t ru_cond;
} RAN_CONTEXT_t;


#endif
