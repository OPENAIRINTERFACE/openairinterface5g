/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

*******************************************************************************/

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
