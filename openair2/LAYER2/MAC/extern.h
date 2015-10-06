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
/*! \file extern.h
* \brief mac externs
* \author  Navid Nikaein and Raymond Knopp
* \date 2010 - 2014
* \version 1.0
* \email navid.nikaein@eurecom.fr
* @ingroup _mac

*/

#ifndef __MAC_EXTERN_H__
#define __MAC_EXTERN_H__


#ifdef USER_MODE
//#include "stdio.h"
#endif //USER_MODE
#include "PHY/defs.h"
#include "defs.h"
#include "COMMON/mac_rrc_primitives.h"
#ifdef PHY_EMUL
//#include "SIMULATION/simulation_defs.h"
#endif //PHY_EMUL
#include "PHY_INTERFACE/defs.h"

extern const uint32_t BSR_TABLE[BSR_TABLE_SIZE];
//extern uint32_t EBSR_Level[63];

extern UE_MAC_INST *UE_mac_inst;
extern eNB_MAC_INST *eNB_mac_inst;
extern MAC_RLC_XFACE *Mac_rlc_xface;
extern uint8_t Is_rrc_registered;

extern eNB_ULSCH_INFO eNB_ulsch_info[NUMBER_OF_eNB_MAX][MAX_NUM_CCs][NUMBER_OF_UE_MAX]; // eNBxUE = 8x8
extern eNB_DLSCH_INFO eNB_dlsch_info[NUMBER_OF_eNB_MAX][MAX_NUM_CCs][NUMBER_OF_UE_MAX]; // eNBxUE = 8x8



//#ifndef USER_MODE
extern MAC_xface *mac_xface;
extern RRC_XFACE *Rrc_xface;

extern uint8_t Is_rrc_registered;

#ifndef PHY_EMUL
#ifndef PHYSIM
#define NB_INST 1
#else
extern unsigned char NB_INST;
#endif
extern unsigned char NB_eNB_INST;
extern unsigned char NB_UE_INST;
extern unsigned char NB_RN_INST;
extern unsigned short NODE_ID[1];
extern void* bigphys_malloc(int);
#else
extern EMULATION_VARS *Emul_vars;
#endif //PHY_EMUL


extern int cqi_to_mcs[16];

extern uint32_t RRC_CONNECTION_FLAG;

extern uint8_t rb_table[33];

extern DCI0_5MHz_TDD_1_6_t       UL_alloc_pdu;

extern DCI1A_5MHz_TDD_1_6_t      RA_alloc_pdu;
extern DCI1A_5MHz_TDD_1_6_t      DLSCH_alloc_pdu1A;
extern DCI1A_5MHz_TDD_1_6_t      BCCH_alloc_pdu;

extern DCI1A_5MHz_TDD_1_6_t      CCCH_alloc_pdu;
extern DCI1_5MHz_TDD_t           DLSCH_alloc_pdu;

extern DCI0_5MHz_FDD_t       UL_alloc_pdu_fdd;

extern DCI1A_5MHz_FDD_t      DLSCH_alloc_pdu1A_fdd;
extern DCI1A_5MHz_FDD_t      RA_alloc_pdu_fdd;
extern DCI1A_5MHz_FDD_t      BCCH_alloc_pdu_fdd;

extern DCI1A_5MHz_FDD_t      CCCH_alloc_pdu_fdd;
extern DCI1_5MHz_FDD_t       DLSCH_alloc_pdu_fdd;

extern DCI2_5MHz_2A_TDD_t DLSCH_alloc_pdu1;
extern DCI2_5MHz_2A_TDD_t DLSCH_alloc_pdu2;
extern DCI1E_5MHz_2A_M10PRB_TDD_t DLSCH_alloc_pdu1E;

#endif //DEF_H


