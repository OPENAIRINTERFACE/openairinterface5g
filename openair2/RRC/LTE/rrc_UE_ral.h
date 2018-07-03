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

/*! \file rrc_UE_ral.h
 * \brief rrc procedures for handling RAL messages
 * \author Lionel GAUTHIER
 * \date 2013
 * \version 1.0
 * \company Eurecom
 * \email: lionel.gauthier@eurecom.fr
 */
#ifndef __RRC_UE_RAL_H__
#    define __RRC_UE_RAL_H__
//-----------------------------------------------------------------------------
//-----------------------------------------------------------------------------
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <ctype.h>
//-----------------------------------------------------------------------------
#include "intertask_interface.h"
#include "ral_messages_types.h"
#include "defs.h"

int rrc_ue_ral_delete_all_thresholds_type        (unsigned int mod_idP, ral_link_param_type_t *param_type_pP);
int rrc_ue_ral_delete_threshold                  (unsigned int mod_idP, ral_link_param_type_t *param_type_pP, ral_threshold_t *threshold_pP);
int rrc_ue_ral_handle_configure_threshold_request(unsigned int mod_idP, MessageDef *msg_pP);
#endif
