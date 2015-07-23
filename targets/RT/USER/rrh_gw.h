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
   OpenAirInterface Dev  : openair4g-devel@eurecom.fr

   Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

/*! \file rrh_gw.h
 * \brief header file for remote radio head gateway (RRH_gw) module 
 * \author Navid Nikaein,  Katerina Trilyraki, Raymond Knopp
 * \date 2015
 * \version 0.1
 * \company Eurecom
 * \maintainer:  navid.nikaein@eurecom.fr
 * \note
 * \warning very experimental 
 */

#ifndef RRH_GW_H_
#define RRH_GW_H_

#include "ethernet_lib.h"
#include "vcd_signal_dumper.h"
#include "assertions.h"

#define DEFAULT_PERIOD_NS 200000
#define RRH_UE_PORT 51000
#define RRH_UE_DEST_IP "127.0.0.1"

/*! \brief RRH supports two types of modules: eNB and UE
	 each module is associated a device of type ETH_IF 
	 and optionally with an RF device (USRP/BLADERF/EXMIMO) */
typedef struct {
  //! module id
  uint8_t id;
  //! loopback flag
  uint8_t loopback;
  //! measurement flag
  uint8_t measurements;
  //! module's ethernet device
  openair0_device eth_dev;
  //! pointer to RF module's device (pointer->since its optional)
  openair0_device *devs;
  
}rrh_module_t;


/******************************************************************************
 **                               FUNCTION PROTOTYPES                        **
 ******************************************************************************/
void signal_handler(int sig);
void timer_signal_handler(int);
void *timer_proc(void *);
void create_timer_thread(void);


/******************************************************************************
 **                               FUNCTION PROTOTYPES                        **
 ******************************************************************************/
void create_UE_trx_threads( rrh_module_t *dev_ue, uint8_t RT_flag, uint8_t NRT_flag);
void create_eNB_trx_threads( rrh_module_t *mod_enb, uint8_t RT_flag, uint8_t NRT_flag);

#endif
