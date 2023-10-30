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

/*! \file extern_NB_IoT.h
 * \brief rrc external vars
 * \author Navid Nikaein and Raymond Knopp, Michele Paffetti
 * \date 2011-2017
 * \version 1.0
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr, michele.paffetti@studio.unibo.it
 */

#ifndef __OPENAIR_RRC_EXTERN_NB_IOT_H__
#define __OPENAIR_RRC_EXTERN_NB_IOT_H__
#include "RRC/LTE/defs_NB_IoT.h"
#include "PHY_INTERFACE/IF_Module_NB_IoT.h"
#include "LAYER2/RLC/rlc.h"
#include "LTE_LogicalChannelConfig-NB-r13.h"
#include "LAYER2/MAC/defs_NB_IoT.h"

#include "common/ran_context.h"


//MP: NOTE:XXX some of the parameters defined in vars_nb_iot are called by the extern.h file so not replicated here

extern UE_RRC_INST_NB_IoT 					*UE_rrc_inst_NB_IoT;

extern eNB_RRC_INST_NB_IoT 					*eNB_rrc_inst_NB_IoT;
extern PHY_Config_NB_IoT_t 						*config_INFO;

extern uint8_t 								DRB2LCHAN_NB_IoT[2];
extern LTE_LogicalChannelConfig_NB_r13_t 		SRB1bis_logicalChannelConfig_defaultValue_NB_IoT;
extern LTE_LogicalChannelConfig_NB_r13_t 		SRB1_logicalChannelConfig_defaultValue_NB_IoT;

extern uint16_t 							T300_NB_IoT[8];
extern uint16_t 							T301_NB_IoT[8];
extern uint16_t 							T310_NB_IoT[8];
extern uint16_t 							T311_NB_IoT[8];
extern uint16_t 							N310_NB_IoT[8];
extern uint16_t 							N311_NB_IoT[8];
extern uint8_t *get_NB_IoT_MIB(struct eNB_RRC_INST_NB_IoT_s *nb_iot_rrc);
#endif


