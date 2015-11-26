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
/*________________________rrc_register.c________________________

 Authors : Hicham Anouar, Raymond Knopp
 Company : EURECOM
 Emails  : anouar@eurecom.fr,  knopp@eurecom.fr
________________________________________________________________*/


#ifndef USER_MODE
#define __NO_VERSION__
#else
#include <stdio.h>
#include <stdlib.h>

#include <pthread.h>
#endif

#include "COMMON/openair_types.h"
#include "MAC/extern.h"

#include <linux/module.h>

#ifndef USER_MODE

//-----------------------------------------------------------------------------------------------------------//
MAC_RLC_XFACE* mac_rrc_register(RRC_XFACE* RRC_xface)
{
  //-----------------------------------------------------------------------------------------------------------//
  if(Is_rrc_registered) {
    msg("[OPENAIR][MAC][RRC_REGISTER] RRC interface already registered, aborting ...\n");
    return NULL;
  } else {
    msg("[OPENAIR][MAC][RRC_REGISTER] Registering RRC Interface, Mac_rlc_xface=%p\n",Mac_rlc_xface);
    Rrc_xface=RRC_xface;
    Is_rrc_registered=1;
    return Mac_rlc_xface;
  }
}

//-----------------------------------------------------------------------------------------------------------//
int mac_rrc_unregister(RRC_XFACE *RRC_xface)
{
  //-----------------------------------------------------------------------------------------------------------//
  if (Rrc_xface == RRC_xface) {
    msg("[OPENAIR][MAC XFACE][RRC_UNREGISTER] Unregistering RRC interface\n");
    Rrc_xface=NULL;
    Is_rrc_registered=0;
    return(0);
  } else {
    msg("[OPENAIR][MAC XFACE][RRC_UNREGISTER] Not the right interface descriptor pointer!!!, aborting ...\n");
    return (-1);
  }

}

EXPORT_SYMBOL(mac_rrc_register);
EXPORT_SYMBOL(mac_rrc_unregister);
#endif //USER_MODE


