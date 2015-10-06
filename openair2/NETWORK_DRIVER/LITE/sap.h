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

#ifndef OAI_SAP_H
#define OAI_SAP_H
typedef unsigned short     OaiNwDrvRadioBearerId_t;
typedef unsigned int       OaiNwDrvSapId_t;              // Id of the QoS SAP to use
typedef unsigned short     OaiNwDrvQoSTrafficClass_t;    // QoS traffic class requested
typedef unsigned int       OaiNwDrvLocalConnectionRef_t; // local identifier
typedef unsigned short     OaiNwDrvCellID_t;             // ID of the cell for connection
typedef unsigned short     OaiNwDrvNumRGsMeas_t;         // number of RGs that could be measured
typedef unsigned int       OaiNwDrvSigLevel_t;           // Signal level measured

#define OAI_NW_DRV_SAPI_CX_MAX                 2
#define OAI_NW_DRV_MAX_MEASURE_NB              5
#define OAI_NW_DRV_PRIMITIVE_MAX_LENGTH        180  // maximum length of a NAS primitive
#define OAI_NW_DRV_SAPI_MAX                    4
#define OAI_NW_DRV_RAB_INPUT_SAPI     2
#define OAI_NW_DRV_RAB_OUTPUT_SAPI      3
#define OAI_NW_DRV_MAX_RABS     8 * 64 //NB_RAB_MAX * MAX_MOBILES_PER_RG  //27   // = MAXURAB
#define OAI_NW_DRV_LIST_RB_MAX 32
#endif
