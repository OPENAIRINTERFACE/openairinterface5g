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
/*! \file common_lib.c 
 * \brief common APIs for different RF frontend device 
 * \author HongliangXU, Navid Nikaein
 * \date 2015
 * \version 0.2
 * \company Eurecom
 * \maintainer:  navid.nikaein@eurecom.fr
 * \note
 * \warning
 */
#include <stdio.h>
#include "common_lib.h"


int openair0_device_init(openair0_device *device, openair0_config_t *openair0_cfg) {
  
#ifdef ETHERNET 
  device->type=ETH_IF; 
  device->func_type = BBU_FUNC;
  openair0_dev_init_eth(device, openair0_cfg);
  printf(" openair0_dev_init_eth ...\n");
#elif EXMIMO
  device->type=EXMIMO_IF;
  openair0_dev_init_exmimo(device, openair0_cfg);
  printf("openair0_dev_init_exmimo...\n");
#elif OAI_USRP
  device->type=USRP_B200_IF;
  openair0_dev_init_usrp(device, openair0_cfg);
  printf("openair0_dev_init_usrp ...\n");
#elif OAI_BLADERF  
  device->type=BLADERF_IF;
  openair0_dev_init_bladerf(device, openair0_cfg);	
  printf(" openair0_dev_init_bladerf ...\n");   
#endif 
   
}
