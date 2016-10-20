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

/*! \file enb_agent_mac_proto.h
 * \brief MAC functions for eNB agent
 * \author Xenofon Foukas
 * \date 2016
 * \email: x.foukas@sms.ed.ac.uk
 * \version 0.1
 * @ingroup _mac

 */

#ifndef __LAYER2_MAC_ENB_AGENT_MAC_PROTO_H__
#define __LAYER2_MAC_ENB_AGENT_MAC_PROTO_H__

#include "enb_agent_defs.h"
#include "header.pb-c.h"
#include "flexran.pb-c.h"

/*
 * Default scheduler used by the eNB agent
 */
void schedule_ue_spec_default(mid_t mod_id, uint32_t frame, uint32_t subframe,
				     int *mbsfn_flag, Protocol__FlexranMessage **dl_info);

/*
 * Data plane function for applying the DL decisions of the scheduler
 */
void apply_dl_scheduling_decisions(mid_t mod_id, uint32_t frame, uint32_t subframe, int *mbsfn_flag,
				const Protocol__FlexranMessage *dl_scheduling_info);

/*
 * Data plane function for applying the UE specific DL decisions of the scheduler
 */
void apply_ue_spec_scheduling_decisions(mid_t mod_id, uint32_t frame, uint32_t subframe, int *mbsfn_flag,
					uint32_t n_dl_ue_data, const Protocol__FlexDlData **dl_ue_data);

/*
 * Data plane function for filling the DCI structure
 */
void fill_oai_dci(mid_t mod_id, uint32_t CC_id, uint32_t rnti, const Protocol__FlexDlDci *dl_dci);

#endif
