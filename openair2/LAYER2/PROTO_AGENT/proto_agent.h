
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

/*! \file proto_agent.h
 * \brief top level protocol agent
 * \author Navid Nikaein and Xenofon Foukas
 * \date 2016
 * \version 0.1
 */

#ifndef PROTO_AGENT_H_
#define PROTO_AGENT_H_
#include "ENB_APP/enb_config.h" // for enb properties
#include "proto_agent_common.h"


void *proto_agent_receive(void *args);

int proto_agent_start(mod_id_t mod_id, const cudu_params_t *p);
void proto_agent_stop(mod_id_t mod_id);

rlc_op_status_t proto_agent_send_rlc_data_req( const protocol_ctxt_t *const ctxt_pP,
    const srb_flag_t srb_flagP, const MBMS_flag_t MBMS_flagP,
    const rb_id_t rb_idP, const mui_t muiP, confirm_t confirmP,
    sdu_size_t sdu_sizeP, mem_block_t *sdu_pP);

boolean_t proto_agent_send_pdcp_data_ind(const protocol_ctxt_t *const ctxt_pP,
                                    const srb_flag_t srb_flagP, const MBMS_flag_t MBMS_flagP,
                                    const rb_id_t rb_idP, sdu_size_t sdu_sizeP, mem_block_t *sdu_pP);

#endif
