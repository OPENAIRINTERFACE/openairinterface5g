/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2016 Eurecom

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

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/

/*! \file ENB_APP/extern.h
 * \brief FlexRAN agent - mac interface primitives
 * \author Xenofon Foukas
 * \date 2016
 * \version 0.1
 * \mail x.foukas@sms.ed.ac.uk
 */

#ifndef __FLEXRAN_AGENT_EXTERN_H__
#define __FLEXRAN_AGENT_EXTERN_H__

#include "flexran_agent_defs.h"
#include "flexran_agent_mac_defs.h"


//extern msg_context_t shared_ctxt[NUM_MAX_ENB][FLEXRAN_AGENT_MAX];

/* full path of the local cache for storing VSFs */
extern char local_cache[40];

/* Control module interface for the communication of the MAC Control Module with the agent */
extern AGENT_MAC_xface *agent_mac_xface[NUM_MAX_ENB];

/* Flag indicating whether the VSFs for the MAC control module have been registered */
extern unsigned int mac_agent_registered[NUM_MAX_ENB];

#endif
