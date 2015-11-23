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

// These messages files are mandatory and must always be placed in first position
#include "intertask_messages_def.h"
#include "timer_messages_def.h"

// Messages files used between tasks
#include "phy_messages_def.h"
#include "mac_messages_def.h"
#include "rlc_messages_def.h"
#include "pdcp_messages_def.h"
#include "rrc_messages_def.h"
#include "nas_messages_def.h"
#if ENABLE_RAL
#include "ral_messages_def.h"
#endif
#include "s1ap_messages_def.h"
#include "x2ap_messages_def.h"
#include "sctp_messages_def.h"
#include "udp_messages_def.h"
#include "gtpv1_u_messages_def.h"

