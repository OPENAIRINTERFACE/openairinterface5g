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
/*! \file rrh_gw_extern.h
 * \brief rrh gatewy external vars
 * \author Navid Nikaein,  Katerina Trilyraki, Raymond Knopp
 * \date 2015
 * \version 0.1
 * \company Eurecom
 * \maintainer:  navid.nikaein@eurecom.fr
 * \note
 * \warning very experimental 
 */

#ifndef RRH_GW_EXTERNS_H_
#define RRH_GW_EXTERNS_H_
extern char   rf_config_file[1024];

extern openair0_timestamp 	timestamp_UE_tx[4] ,timestamp_UE_rx[4] ,timestamp_eNB_rx[4],timestamp_eNB_tx[4];
extern openair0_vtimestamp 	hw_counter;
extern int32_t			UE_tx_started,UE_rx_started,eNB_rx_started ,eNB_tx_started;
extern int32_t			nsamps_UE[4],nsamps_eNB[4];
extern int32_t			overflow_rx_buffer_eNB[4],overflow_rx_buffer_UE[4];
extern uint8_t			rrh_exit;
extern int32_t			**rx_buffer_eNB, **rx_buffer_UE;
extern unsigned int	        rt_period;
extern pthread_mutex_t          timer_mutex;


#endif 
