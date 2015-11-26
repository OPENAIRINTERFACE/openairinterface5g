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

/*! \file rrc_types.h
* \brief rrc types and subtypes
* \author Navid Nikaein and Raymond Knopp
* \date 2011 - 2014
* \version 1.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr, raymond.knopp@eurecom.fr
*/

#ifndef RRC_TYPES_H_
#define RRC_TYPES_H_

typedef enum Rrc_State_e {
  RRC_STATE_INACTIVE=0,
  RRC_STATE_IDLE,
  RRC_STATE_CONNECTED,

  RRC_STATE_FIRST = RRC_STATE_INACTIVE,
  RRC_STATE_LAST = RRC_STATE_CONNECTED,
} Rrc_State_t;

typedef enum Rrc_Sub_State_e {
  RRC_SUB_STATE_INACTIVE=0,

  RRC_SUB_STATE_IDLE_SEARCHING,
  RRC_SUB_STATE_IDLE_RECEIVING_SIB,
  RRC_SUB_STATE_IDLE_SIB_COMPLETE,
  RRC_SUB_STATE_IDLE_CONNECTING,
  RRC_SUB_STATE_IDLE,

  RRC_SUB_STATE_CONNECTED,

  RRC_SUB_STATE_INACTIVE_FIRST = RRC_SUB_STATE_INACTIVE,
  RRC_SUB_STATE_INACTIVE_LAST = RRC_SUB_STATE_INACTIVE,

  RRC_SUB_STATE_IDLE_FIRST = RRC_SUB_STATE_IDLE_SEARCHING,
  RRC_SUB_STATE_IDLE_LAST = RRC_SUB_STATE_IDLE,

  RRC_SUB_STATE_CONNECTED_FIRST = RRC_SUB_STATE_CONNECTED,
  RRC_SUB_STATE_CONNECTED_LAST = RRC_SUB_STATE_CONNECTED,
} Rrc_Sub_State_t;

#endif /* RRC_TYPES_H_ */
