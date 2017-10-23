/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

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
