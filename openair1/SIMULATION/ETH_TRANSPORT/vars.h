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
* \brief specifies the variables for phy emulation
* \author Navid Nikaein and Raymomd Knopp and Hicham Anouar
* \date 2011
* \version 1.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
*/

#ifndef USER_MODE
#include <rtai_posix.h>
#else
#include <pthread.h>
#endif
#include "defs.h"

#ifndef __BYPASS_SESSION_LAYER_VARS_H__
#    define __BYPASS_SESSION_LAYER_VARS_H__

unsigned char Emulation_status;
unsigned char emu_tx_status;
unsigned char emu_rx_status;
//unsigned int Master_list=0;
//unsigned short Master_id;
//unsigned int Is_primary_master;

#if !defined(ENABLE_NEW_MULTICAST)
pthread_mutex_t emul_low_mutex;
pthread_cond_t emul_low_cond;
char emul_low_mutex_var;
pthread_mutex_t Tx_mutex;
pthread_cond_t Tx_cond;
char Tx_mutex_var;
#endif

/* Handlers for RX and TX */
rx_handler_t rx_handler;
tx_handler_t tx_handler;

eNB_transport_info_t eNB_transport_info[NUMBER_OF_eNB_MAX][MAX_NUM_CCs];
uint16_t eNB_transport_info_TB_index[NUMBER_OF_eNB_MAX][MAX_NUM_CCs];

UE_transport_info_t UE_transport_info[NUMBER_OF_UE_MAX][MAX_NUM_CCs];
uint16_t UE_transport_info_TB_index[NUMBER_OF_UE_MAX][MAX_NUM_CCs];

UE_cntl ue_cntl_delay[NUMBER_OF_UE_MAX][MAX_NUM_CCs][2];

#endif
