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
 * pdcp_messages_def.h
 *
 *  Created on: Oct 24, 2013
 *      Author: winckel and Navid Nikaein
 */

//-------------------------------------------------------------------------------------------//
// Messages between RRC and PDCP layers
MESSAGE_DEF(RRC_DCCH_DATA_REQ,          MESSAGE_PRIORITY_MED_PLUS, RrcDcchDataReq,              rrc_dcch_data_req)
MESSAGE_DEF(RRC_DCCH_DATA_IND,          MESSAGE_PRIORITY_MED_PLUS, RrcDcchDataInd,              rrc_dcch_data_ind)
MESSAGE_DEF(RRC_PCCH_DATA_REQ,          MESSAGE_PRIORITY_MED_PLUS, RrcPcchDataReq,              rrc_pcch_data_req)
