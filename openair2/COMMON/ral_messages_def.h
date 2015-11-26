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

MESSAGE_DEF(RRC_RAL_SYSTEM_CONFIGURATION_IND,          MESSAGE_PRIORITY_MED, rrc_ral_system_configuration_ind_t,          rrc_ral_system_configuration_ind)
MESSAGE_DEF(RRC_RAL_SYSTEM_INFORMATION_IND,            MESSAGE_PRIORITY_MED, rrc_ral_system_information_ind_t,            rrc_ral_system_information_ind)

MESSAGE_DEF(RRC_RAL_SCAN_REQ,                          MESSAGE_PRIORITY_MED, rrc_ral_scan_req_t,                          rrc_ral_scan_req)
MESSAGE_DEF(RRC_RAL_SCAN_CONF,                         MESSAGE_PRIORITY_MED, rrc_ral_scan_conf_t,                         rrc_ral_scan_conf)

MESSAGE_DEF(RRC_RAL_CONFIGURE_THRESHOLD_REQ,           MESSAGE_PRIORITY_MED, rrc_ral_configure_threshold_req_t,           rrc_ral_configure_threshold_req)
MESSAGE_DEF(RRC_RAL_CONFIGURE_THRESHOLD_CONF,          MESSAGE_PRIORITY_MED, rrc_ral_configure_threshold_conf_t,          rrc_ral_configure_threshold_conf)
MESSAGE_DEF(RRC_RAL_MEASUREMENT_REPORT_IND,            MESSAGE_PRIORITY_MED, rrc_ral_measurement_report_ind_t,            rrc_ral_measurement_report_ind)

MESSAGE_DEF(RRC_RAL_CONNECTION_ESTABLISHMENT_REQ,      MESSAGE_PRIORITY_MED, rrc_ral_connection_establishment_req_t,      rrc_ral_connection_establishment_req)
MESSAGE_DEF(RRC_RAL_CONNECTION_ESTABLISHMENT_CONF,     MESSAGE_PRIORITY_MED, rrc_ral_connection_establishment_conf_t,     rrc_ral_connection_establishment_conf)
MESSAGE_DEF(RRC_RAL_CONNECTION_ESTABLISHMENT_IND,      MESSAGE_PRIORITY_MED, rrc_ral_connection_establishment_ind_t,      rrc_ral_connection_establishment_ind)
MESSAGE_DEF(RRC_RAL_CONNECTION_REESTABLISHMENT_IND,    MESSAGE_PRIORITY_MED, rrc_ral_connection_reestablishment_ind_t,    rrc_ral_connection_reestablishment_ind)
MESSAGE_DEF(RRC_RAL_CONNECTION_RECONFIGURATION_REQ,    MESSAGE_PRIORITY_MAX, rrc_ral_connection_reconfiguration_req_t,    rrc_ral_connection_reconfiguration_req)
MESSAGE_DEF(RRC_RAL_CONNECTION_RECONFIGURATION_IND,    MESSAGE_PRIORITY_MAX, rrc_ral_connection_reconfiguration_ind_t,    rrc_ral_connection_reconfiguration_ind)
MESSAGE_DEF(RRC_RAL_CONNECTION_RECONFIGURATION_HO_IND, MESSAGE_PRIORITY_MAX, rrc_ral_connection_reconfiguration_ho_ind_t, rrc_ral_connection_reconfiguration_ho_ind)

MESSAGE_DEF(RRC_RAL_CONNECTION_RELEASE_REQ,            MESSAGE_PRIORITY_MED, rrc_ral_connection_release_req_t,            rrc_ral_connection_release_req)
MESSAGE_DEF(RRC_RAL_CONNECTION_RELEASE_CONF,           MESSAGE_PRIORITY_MED, rrc_ral_connection_release_conf_t,           rrc_ral_connection_release_conf)
MESSAGE_DEF(RRC_RAL_CONNECTION_RELEASE_IND,            MESSAGE_PRIORITY_MED, rrc_ral_connection_release_ind_t,            rrc_ral_connection_release_ind)

