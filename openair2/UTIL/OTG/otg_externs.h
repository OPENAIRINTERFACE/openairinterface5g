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

/*! \file otg_externs.h
* \brief extern parameters
* \author n. nikaein A. Hafsaoui
* \date 2012
* \version 1.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
* \note
* \warning
*/

#ifndef __OTG_EXTERNS_H__
#    define __OTG_EXTERNS_H__


/*!< \brief main log variables */
extern otg_t *g_otg; /*!< \brief global params */
extern otg_multicast_t *g_otg_multicast; /*!< \brief global params */
extern otg_info_t *otg_info; /*!< \brief info otg */
extern otg_multicast_info_t *otg_multicast_info; /*!< \brief  info otg: measurements about the simulation  */
extern otg_forms_info_t *otg_forms_info;

extern mapping otg_multicast_app_type_names[] ;

extern mapping otg_app_type_names[];

extern mapping otg_transport_protocol_names[];

extern mapping otg_ip_version_names[];

extern mapping otg_multicast_app_type_names[];

extern mapping otg_distribution_names[];

extern mapping frame_type_names[];

extern mapping switch_names[] ;

extern mapping packet_gen_names[];

#endif

