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

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/

/*! \file 
 * \brief 
 * \author 
 * \date 2016
 * \version 0.1
 */

#ifndef ENB_AGENT_MAC_H_
#define ENB_AGENT_MAC_H_

#include "header.pb-c.h"
#include "progran.pb-c.h"
#include "stats_messages.pb-c.h"
#include "stats_common.pb-c.h"


/* These types will be used to give
   instructions for the type of stats reports
   we need to create */
typedef struct {
  uint16_t ue_rnti;
  uint32_t ue_report_flags; /* Indicates the report elements
			       required for this UE id. See
			       ProgRAN specification 1.2.4.2 */
} ue_report_type_t;

typedef struct {
  uint16_t cc_id;
  uint32_t cc_report_flags; /* Indicates the report elements
			      required for this CC index. See
			      ProgRAN specification 1.2.4.3 */
} cc_report_type_t;

typedef struct {
  int nr_ue;
  ue_report_type_t *ue_report_type;
  int nr_cc;
  cc_report_type_t *cc_report_type;
} report_config_t;


int enb_agent_mac_reply(uint32_t xid, const void *params, Protocol__ProgranMessage **msg);

int enb_agent_mac_stats_reply(uint32_t xid, const report_config_t *report_config, Protocol__ProgranMessage **msg);

int enb_agent_mac_destroy_stats_reply(Protocol__ProgranMessage *msg);

#endif
