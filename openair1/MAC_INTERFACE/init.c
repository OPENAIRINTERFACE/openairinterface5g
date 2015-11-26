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
/*________________________mac_init.c________________________

 Authors : Hicham Anouar, Raymond Knopp
 Company : EURECOM
 Emails  : anouar@eurecom.fr,  knopp@eurecom.fr
________________________________________________________________*/

/*!\brief Initilization and reconfiguration routines for generic MAC interface */

#include "defs.h"
#include "extern.h"
#include "SCHED/defs.h"

int mac_init(void)
{
  int i;

#ifndef USER_MODE
  // mac_xface->macphy_init();
  mac_xface->macphy_exit = openair_sched_exit;
#else
  mac_xface->macphy_exit=(void (*)(void)) exit;
#endif

  /* this is done in cbmimo1_fileops
  #ifdef OPENAIR2
  mac_xface->macphy_init();
  #endif //OPENAIR2
  */

  return(1);
}

void mac_cleanup(void)
{

}

void mac_resynch(void)
{

}

/*
#ifdef OPENAIR2
EXPORT_SYMBOL(frame);
#endif //OPENAIR2
*/
