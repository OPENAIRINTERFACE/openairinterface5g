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

/*! \file itti_sim_messages_def.h
 * \brief itti message for itti simulator
 * \author Yoshio INOUE, Masayuki HARADA
 * \email yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
 * \date 2020
 * \version 0.1
 */



MESSAGE_DEF(GNB_RRC_BCCH_DATA_IND,          MESSAGE_PRIORITY_MED, itti_sim_rrc_ch_t,   GNBBCCHind)
MESSAGE_DEF(GNB_RRC_CCCH_DATA_IND,          MESSAGE_PRIORITY_MED, itti_sim_rrc_ch_t,   GNBCCCHind)
MESSAGE_DEF(GNB_RRC_DCCH_DATA_IND,          MESSAGE_PRIORITY_MED, itti_sim_rrc_ch_t,   GNBDCCHind)
MESSAGE_DEF(UE_RRC_CCCH_DATA_IND,           MESSAGE_PRIORITY_MED, itti_sim_rrc_ch_t,   UECCCHind)
MESSAGE_DEF(UE_RRC_DCCH_DATA_IND,           MESSAGE_PRIORITY_MED, itti_sim_rrc_ch_t,   UEDCCHind)
