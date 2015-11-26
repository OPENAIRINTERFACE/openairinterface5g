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
/***************************************************************************
                            pdcp_control_primitives_proto_extern.h
                             -------------------
    begin                : Mon Dec 10 2001
    email                : Lionel.Gauthier@eurecom.fr
                             -------------------
    description
    This file contains the prototypes of functions used for configuration of pdcp

 ***************************************************************************/
#ifndef PDCP_CONTROL_PRIMITIVES_PROTO_EXTERN_H
#    define PDCP_CONTROL_PRIMITIVES_PROTO_EXTERN_H
#    include "pdcp.h"
void            rrc_configure_pdcp (struct pdcp_entity *pdcpP, void *rlcP, uint8_t rlc_sap_typeP, uint8_t header_compression_typeP);
#endif
