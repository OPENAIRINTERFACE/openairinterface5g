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

/*! \file otg_vars.h
* \brief Global structure
* \author navid nikaein and A. Hafsaoui
* \date 2011
* \version 1.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/
#ifndef __OTG_VARS_H__
#    define __OTG_VARS_H__


/*!< \brief main log variables */
otg_t *g_otg; /*!< \brief  global params */
otg_multicast_t *g_otg_multicast; /*!< \brief  global params */
otg_info_t *otg_info; /*!< \brief  info otg: measurements about the simulation  */
otg_multicast_info_t *otg_multicast_info; /*!< \brief  info otg: measurements about the simulation  */
otg_forms_info_t *otg_forms_info;

mapping otg_app_type_names[] = {
  {"no_predefined_traffic", 0},
  {"m2m", 1},
  {"scbr", 2},
  {"mcbr", 3},
  {"bcbr", 4},
  {"auto_pilot", 5},
  {"bicycle_race", 6},
  {"open_arena", 7},
  {"team_fortress", 8},
  {"full_buffer", 9},
  {"m2m_traffic", 10},
  {"auto_pilot_l",11},
  {"auto_pilot_m", 12},
  {"auto_pilot_h", 13},
  {"auto_pilot_e", 14},
  {"virtual_game_l", 15},
  {"virtual_game_m", 16},
  {"virtual_game_h", 17},
  {"virtual_game_f", 18},
  {"alarm_humidity", 19},
  {"alarm_smoke",20},
  {"alarm_temperature", 21},
  {"openarena_dl", 22},
  {"openarena_ul", 23},
  {"voip_g711", 24},
  {"voip_g729", 25},
  {"iqsim_mango", 26},
  {"iqsim_newsteo", 27},
  {"openarena_dl_tarma", 28},
  {"video_vbr_10mbps", 29},
  {"video_vbr_4mbps", 30},
  {"video_vbr_2mbps", 31},
  {"video_vbr_768kbps", 32},
  {"video_vbr_384kbps", 33},
  {"video_vbr_192kpbs", 34},
  {"background_users", 35},
  {NULL, -1}
};
mapping otg_transport_protocol_names[] = {
  {"no_proto", 0},
  {"udp", 1},
  {"tcp", 2},
  {NULL, -1}
};
mapping otg_ip_version_names[] = {
  {"no_ip", 0},
  {"ipv4", 1},
  {"ipv6", 2},
  {NULL, -1}
};

mapping otg_multicast_app_type_names[] = {
  {"no_predefined_multicast_traffic", 0},
  {"mscbr", 1},
  {"mmcbr", 2},
  {"mbcbr", 3},
  {"msvbr", 4},
  {"mmvbr", 5},
  {"mbvbr", 6},
  {"mvideo_vbr_4mbps", 7},
  {NULL, -1}
}  ;

mapping otg_distribution_names[] = {
  {"no_customized_traffic", 0},
  {"uniform", 1},
  {"gaussian", 2},
  {"exponential", 3},
  {"poisson", 4},
  {"fixed", 5},
  {"weibull", 6},
  {"pareto", 7},
  {"gamma", 8},
  {"cauchy",9},
  {"log_normal",10},
  {"tarma",11},
  {"video",12},
  {"background_dist",13},
  {NULL, -1}
};
mapping frame_type_names[] = {
  {"TDD", 1},
  {"FDD", 0},
  {NULL, -1}
};

mapping switch_names[] = {
  {"disable", 0},
  {"enable", 1},
  {NULL, -1}
};

mapping packet_gen_names[] = {
  {"repeat_string", 0},
  {"substract_string", 1},
  {"random_position", 2},
  {"random_string", 3},
  {NULL, -1}
};
//int ptime; /*!< \brief ptime is uses to compute the local simulation time */

#endif
