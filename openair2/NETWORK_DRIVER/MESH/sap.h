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
                          graal_sap.h  -  description
                             -------------------
    copyright            : (C) 2002 by Eurecom
    email                : michelle.wetterwald@eurecom.fr
                           yan.moret@eurecom.fr
 ***************************************************************************

 ***************************************************************************/

#ifndef _NAS_SAP_H
#define _NAS_SAP_H


// RT-FIFO identifiers ** must be identical to Access Stratum as_sap.h and rrc_sap.h

#define RRC_DEVICE_GC          RRC_SAPI_GCSAP
#define RRC_DEVICE_NT          RRC_SAPI_NTSAP
#define RRC_DEVICE_DC_INPUT0   RRC_SAPI_DCSAP_IN
#define RRC_DEVICE_DC_OUTPUT0  RRC_SAPI_DCSAP_OUT


//FIFO indexes in control blocks

#define NAS_DC_INPUT_SAPI   0
#define NAS_DC_OUTPUT_SAPI  1
#define NAS_SAPI_CX_MAX     2

#define NAS_GC_SAPI         0
#define NAS_NT_SAPI         1

#define NAS_RAB_INPUT_SAPI      2
#define NAS_RAB_OUTPUT_SAPI     3


#define NAS_SAPI_MAX       4




#endif



