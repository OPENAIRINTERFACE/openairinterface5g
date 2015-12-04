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

#define DEFAULT_PERIOD_NS 200000 /* default value is calculated for 25 PRB */
#define RRH_UE_PORT 51000
#define RRH_UE_DEST_IP "127.0.0.1"

/*! \brief RRH supports two types of modules: eNB and UE
	   each module is associated with an ethernet device (device of ETH_IF) 
	   and optionally with a RF device (device type can be USRP_B200/USRP_X300/BLADERF_IF/EXMIMO_IF/NONE_IF)
           UE modules will always have RF device type NONE_IF */
typedef struct {
/*! \brief module id */
  uint8_t id;
/*! \brief! loopback flag */
uint8_t loopback;
/*! \brief measurement flag */
uint8_t measurements;
/*! \brief module's ethernet device */
openair0_device eth_dev;
/*! \brief pointer to RF module's device (pointer->since it's optional) */
openair0_device *devs;
}rrh_module_t;

/*! \fn void timer_signal_handler(int sig)
 * \brief this function
 * \param[in] signal type
 * \return none
 * \note
 * @ingroup  _oai
*/
void timer_signal_handler(int);

/*! \fn void *timer_proc(void *arg)
 * \brief this function
 * \param[in]
 * \param[out]
 * \return
 * \note
 * @ingroup  _oai
 */
void *timer_proc(void *);

/*! \fn void config_BBU_mod( rrh_module_t *mod_enb, uint8_t RT_flag,uint8_t NRT_flag)
 * \brief receive and apply configuration to modules' optional device
 * \param[in]  *mod_enb pointer to module 
 * \param[in]   RT_flag real time flag 
 * \return none
 * \note
 * @ingroup  _oai
 */
void config_BBU_mod( rrh_module_t *mod_enb, uint8_t RT_flag, uint8_t NRT_flag);

/*! \fn void  config_UE_mod( rrh_module_t *dev_ue, uint8_t RT_flag,uint8_t NRT_flag)
 * \brief this function
 * \param[in] *mod_ue pointer to module 
 * \param[in]
 * \return none
 * \note
 * @ingroup  _oai
 */
void config_UE_mod( rrh_module_t *dev_ue, uint8_t RT_flag, uint8_t NRT_flag);



void signal_handler(int sig);

#endif
