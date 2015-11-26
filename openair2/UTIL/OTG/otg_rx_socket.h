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

/*! \file otg_rx_socket.h
* \brief brief explain how this block is organized, and how it works: OTG RX traffic generation functions with sockets
* \author A. Hafsaoui
* \date 2012
* \version 0.1
* \company Eurecom
* \email: openair_tech@eurecom.fr
* \note
* \warning
*/
#ifndef __OTG_RX_SOCKET_H__
#define __OTG_RX_SOCKET_H__

#include"otg.h"


//-----------------------begin func proto-------------------

/*! \fn server_socket_tcp_ip4()
* \brief this function allow to run a server dedicated to IP v4 and TCP traffic .
* \param[in]
* \param[out]
* \return void
* \note
* @ingroup  _otg
*/
void server_socket_tcp_ip4();



//-----------------------end func proto-------------------



#endif
