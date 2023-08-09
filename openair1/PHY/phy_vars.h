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

#ifndef __PHY_VARS_H__
#define __PHY_VARS_H__

#include "PHY/defs_UE.h"

#include "openair2/NR_PHY_INTERFACE/NR_IF_Module.h"
#include "openair2/PHY_INTERFACE/IF_Module.h"


PHY_VARS_UE ***PHY_vars_UE_g;
UL_RCC_IND_t UL_RCC_INFO;
NR_UL_IND_t UL_INFO;

unsigned char NB_RU=0;
int number_of_cards;
uint8_t max_turbo_iterations=4;

#endif /*__PHY_VARS_H__ */
