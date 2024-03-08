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

/*! \file nr_rrc_extern.h
* \brief rrc external vars
* \author Navid Nikaein and Raymond Knopp, WEI-TAI CHEN
* \date 2011, 2018
* \version 1.0
* \company Eurecom, NTUST
* \email: navid.nikaein@eurecom.fr, kroempa@gmail.com
*/

#ifndef __OPENAIR_NR_RRC_EXTERN_H__
#define __OPENAIR_NR_RRC_EXTERN_H__
#include "nr_rrc_defs.h"
#include "COMMON/mac_rrc_primitives.h"
#include "LAYER2/RLC/rlc.h"
#include "openair2/RRC/common.h"
void openair_rrc_gNB_configuration(gNB_RRC_INST *rrc, gNB_RrcConfigurationReq *configuration);
#endif
