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

/*! \file proto_agent.h
 * \brief top level protocol agent
 * \author Navid Nikaein and Xenofon Foukas
 * \date 2016
 * \version 0.1
 */

#ifndef PROTO_AGENT_H_
#define PROTO_AGENT_H_
#include "ENB_APP/enb_config.h" // for enb properties
#include "proto_agent_common.h"


void *proto_agent_receive(void *args);

int proto_agent_start(mod_id_t mod_id, const cudu_params_t *p);
void proto_agent_stop(mod_id_t mod_id);

rlc_op_status_t proto_agent_send_rlc_data_req( const protocol_ctxt_t *const ctxt_pP,
    const srb_flag_t srb_flagP, const MBMS_flag_t MBMS_flagP,
    const rb_id_t rb_idP, const mui_t muiP, confirm_t confirmP,
    sdu_size_t sdu_sizeP, mem_block_t *sdu_pP);

boolean_t proto_agent_send_pdcp_data_ind(const protocol_ctxt_t *const ctxt_pP,
                                    const srb_flag_t srb_flagP, const MBMS_flag_t MBMS_flagP,
                                    const rb_id_t rb_idP, sdu_size_t sdu_sizeP, mem_block_t *sdu_pP);

#endif
