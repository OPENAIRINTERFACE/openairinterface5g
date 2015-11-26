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


/*! \file vars.h
* \brief rrc external vars
* \author Navid Nikaein and Raymond Knopp
* \date 2011
* \version 1.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
*/

#ifndef __OPENAIR_RRC_EXTERN_H__
#define __OPENAIR_RRC_EXTERN_H__
#include "defs.h"
#include "COMMON/mac_rrc_primitives.h"
#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/extern.h"
#include "LAYER2/RLC/rlc.h"

extern UE_RRC_INST *UE_rrc_inst;
extern eNB_RRC_INST *eNB_rrc_inst;
//extern RRC_XFACE *Rrc_xface;
#ifndef USER_MODE
//extern MAC_RLC_XFACE *Mac_rlc_xface;
extern int S_rrc;
//extern int R_rrc;
#else
#include "LAYER2/MAC/extern.h"
#ifndef NO_RRM
extern sock_rrm_t S_rrc;
#endif
#endif

#ifndef NO_RRM
#ifndef USER_MODE
extern char *Header_buf;
extern char *Data;
extern unsigned short Header_read_idx,Data_read_idx,Header_size;
#endif
extern unsigned short Data_to_read;
#endif //NO_RRM

extern uint8_t DRB2LCHAN[8];

extern LogicalChannelConfig_t SRB1_logicalChannelConfig_defaultValue;
extern LogicalChannelConfig_t SRB2_logicalChannelConfig_defaultValue;


#ifndef PHY_EMUL
#ifndef PHYSIM
//#define NB_INST 1
#else
extern unsigned char NB_INST;
#endif
extern unsigned char NB_eNB_INST;
extern unsigned char NB_UE_INST;
extern unsigned short NODE_ID[1];
extern void* bigphys_malloc(int);
#endif


//CONSTANTS
extern rlc_info_t Rlc_info_um,Rlc_info_am_config;
//uint8_t RACH_TIME_ALLOC;
extern uint16_t RACH_FREQ_ALLOC;
//uint8_t NB_RACH;
extern LCHAN_DESC BCCH_LCHAN_DESC,CCCH_LCHAN_DESC,DCCH_LCHAN_DESC,DTCH_DL_LCHAN_DESC,DTCH_UL_LCHAN_DESC;
extern MAC_MEAS_T BCCH_MEAS_TRIGGER,CCCH_MEAS_TRIGGER,DCCH_MEAS_TRIGGER,DTCH_MEAS_TRIGGER;
extern MAC_AVG_T BCCH_MEAS_AVG,CCCH_MEAS_AVG,DCCH_MEAS_AVG, DTCH_MEAS_AVG;

extern uint16_t T300[8];
extern uint16_t T310[8];
extern uint16_t N310[8];
extern uint16_t N311[8];
extern uint32_t T304[8];
extern uint32_t timeToTrigger_ms[16];
extern float RSRP_meas_mapping[100];
extern float RSRQ_meas_mapping[33];

#endif


