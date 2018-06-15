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

/*! \file proto.h
 * \brief RRC functions prototypes for eNB and UE
 * \author Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \email navid.nikaein@eurecom.fr
 * \version 1.0
 
 */
/** \addtogroup _rrc
 *  @{
 */

#include "RRC/NR/nr_rrc_defs.h"

#include "flexran_agent_extern.h"

int rrc_init_nr_global_param(void);

void rrc_config_nr_buffer(NR_SRB_INFO* Srb_info,
                          uint8_t Lchan_type,
                          uint8_t Role);

#if defined(ENABLE_ITTI)
/**\brief RRC eNB task.
   \param void *args_p Pointer on arguments to start the task. */
void *rrc_gnb_task(void *args_p);

#endif