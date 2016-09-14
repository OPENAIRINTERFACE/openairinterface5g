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
