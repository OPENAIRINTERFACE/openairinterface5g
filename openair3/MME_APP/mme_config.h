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
  mme_config.h
  AUTHOR  : Javier Morgade
  COMPANY : Vicomtech, Spain
  EMAIL   : javier.morgade@ieee.org
*/

#ifndef MME_CONFIG_H_
#define MME_CONFIG_H_
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <libconfig.h>

#include "commonDef.h"
#include "platform_types.h"
#include "platform_constants.h"
#include "PHY/impl_defs_lte.h"
#include "PHY/defs_eNB.h"
#include "s1ap_messages_types.h"
#include "f1ap_messages_types.h"
#include "LTE_SystemInformationBlockType2.h"
#include "rrc_messages_types.h"
#include "RRC/LTE/rrc_defs.h"
#include <intertask_interface.h>
#include "enb_paramdef.h"

int RCconfig_MME(void);

#endif /* MME_CONFIG_H_ */
/** @} */
