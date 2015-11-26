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
/*________________________mac_register.c________________________

 Authors : Hicham Anouar, Raymond Knopp
 Company : EURECOM
 Emails  : anouar@eurecom.fr,  knopp@eurecom.fr
________________________________________________________________*/


/*!\brief Registration routines for generic MAC interface.  Register/unregister a MAC.
Upon registration, the MAC retrieves the transport channel (MAC_tch) data structure and provides the
pointer to the macphy_scheduler() routine*/
#define __NO_VERSION__

#undef __PHY_IMPLEMENTATION_DEFS_H__

#include "ARCH/CBMIMO1/DEVICE_DRIVER/defs.h"
#include "defs.h"
#include "extern.h"
#include "SCHED/extern.h"

#ifndef USER_MODE





void dummy_macphy_scheduler(uint8_t last_slot)
{
  //if (last_slot == 0)
  //  mac_xface->frame++;

  msg("[OPENAIR][MAC XFACE] in dummy_macphy_scheduler(): MAC no yet registered!\n");

  //#ifdef EMOS
  //  phy_procedures(last_slot);
  //#endif

}

void dummy_macphy_setparams(void *params)
{
  msg("[OPENAIR][MAC XFACE] dummy_macphy_setparams(): no MAC registered!\n");
}

void dummy_macphy_init(void )
{
  msg("[OPENAIR][MAC XFACE] dummy_macphy_init(): no MAC registered!\n");
}

MAC_xface *mac_register(void macphy_scheduler(uint8_t last_slot), void macphy_setparams(void *),void macphy_init(void ),void mrbch_phy_sync_failure(unsigned char,unsigned char),
                        void chbch_phy_sync_success(unsigned char,unsigned char))
{

  if (openair_daq_vars.mac_registered == 0) {

    msg("[OPENAIR][MAC XFACE] Registering new MAC interface at %p, scheduler %p, setparams at %p, init at %p\n",
        mac_xface,macphy_scheduler,macphy_setparams,macphy_init);
    mac_xface->macphy_scheduler = macphy_scheduler;
    mac_xface->macphy_setparams = macphy_setparams;
    mac_xface->macphy_init      = macphy_init;
#ifndef PC_TARGET
    //    mac_xface->ublaze_mac_xface = ublaze_mac_xface;
#endif
    openair_daq_vars.mac_registered=1;
    return(mac_xface);
  } else {
    msg("[OPENAIR][MAC XFACE] MAC interface already registered, aborting ...\n");
    return NULL;
  }


}

int mac_unregister(MAC_xface *mac_xface_rx)
{

  if (mac_xface_rx == mac_xface) {
    msg("[OPENAIR][MAC XFACE] Unregistering MAC interface\n");
    mac_xface->macphy_scheduler = dummy_macphy_scheduler;
    mac_xface->macphy_setparams = dummy_macphy_setparams;
    mac_xface->macphy_init      = dummy_macphy_init;
    openair_daq_vars.mac_registered=0;
    return(0);
  } else {
    msg("[OPENAIR][MAC XFACE] Not the right interface descriptor pointer!!!, aborting ...\n");
    return (-1);
  }

}

EXPORT_SYMBOL(mac_register);
EXPORT_SYMBOL(mac_unregister);
#endif //USER_MODE
