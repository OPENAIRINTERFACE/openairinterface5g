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
 * mphy_messages_def.h
 *
 *  Created on: Dec 12, 2013
 *      Author: winckel
 */

//-------------------------------------------------------------------------------------------//
// eNB: ENB_APP -> PHY messages
MESSAGE_DEF(PHY_CONFIGURATION_REQ,   MESSAGE_PRIORITY_MED_PLUS, PhyConfigurationReq,  phy_configuration_req)

// UE: RRC -> PHY messages
MESSAGE_DEF(PHY_DEACTIVATE_REQ,      MESSAGE_PRIORITY_MED_PLUS, PhyDeactivateReq,     phy_deactivate_req)

MESSAGE_DEF(PHY_FIND_CELL_REQ,       MESSAGE_PRIORITY_MED_PLUS, PhyFindCellReq,       phy_find_cell_req)
MESSAGE_DEF(PHY_FIND_NEXT_CELL_REQ,  MESSAGE_PRIORITY_MED_PLUS, PhyFindNextCellReq,   phy_find_next_cell_req)
MESSAGE_DEF(PHY_MEAS_THRESHOLD_REQ,  MESSAGE_PRIORITY_MED_PLUS, PhyMeasThresholdReq,  phy_meas_threshold_req)
// UE: PHY -> RRC messages
MESSAGE_DEF(PHY_FIND_CELL_IND,       MESSAGE_PRIORITY_MED_PLUS, PhyFindCellInd,       phy_find_cell_ind)
MESSAGE_DEF(PHY_MEAS_THRESHOLD_CONF, MESSAGE_PRIORITY_MED_PLUS, PhyMeasThresholdConf, phy_meas_threshold_conf)
MESSAGE_DEF(PHY_MEAS_REPORT_IND,     MESSAGE_PRIORITY_MED_PLUS, PhyMeasReportInd,     phy_meas_report_ind)
