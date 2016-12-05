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

/*! \file flexran_agent_mac_proto.h
 * \brief MAC functions for FlexRAN agent
 * \author Xenofon Foukas
 * \date 2016
 * \email: x.foukas@sms.ed.ac.uk
 * \version 0.1
 * @ingroup _mac

 */

#ifndef __LAYER2_MAC_FLEXRAN_AGENT_MAC_PROTO_H__
#define __LAYER2_MAC_FLEXRAN_AGENT_MAC_PROTO_H__

#include "flexran_agent_defs.h"
#include "header.pb-c.h"
#include "flexran.pb-c.h"

/*
 * Default scheduler used by the eNB agent
 */
void flexran_schedule_ue_spec_default(mid_t mod_id, uint32_t frame, uint32_t subframe,
				      int *mbsfn_flag, Protocol__FlexranMessage **dl_info);

/*
 * Data plane function for applying the DL decisions of the scheduler
 */
void flexran_apply_dl_scheduling_decisions(mid_t mod_id, uint32_t frame, uint32_t subframe, int *mbsfn_flag,
					   const Protocol__FlexranMessage *dl_scheduling_info);

/*
 * Data plane function for applying the UE specific DL decisions of the scheduler
 */
void flexran_apply_ue_spec_scheduling_decisions(mid_t mod_id, uint32_t frame, uint32_t subframe, int *mbsfn_flag,
						uint32_t n_dl_ue_data, const Protocol__FlexDlData **dl_ue_data);

/*
 * Data plane function for filling the DCI structure
 */
void flexran_fill_oai_dci(mid_t mod_id, uint32_t CC_id, uint32_t rnti, const Protocol__FlexDlDci *dl_dci);

#endif
