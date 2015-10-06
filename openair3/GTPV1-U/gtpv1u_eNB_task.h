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
/*! \file gtpv1u_eNB_task.h
* \brief
* \author Lionel Gauthier
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/

#ifndef GTPV1U_ENB_TASK_H_
#define GTPV1U_ENB_TASK_H_

#include "messages_types.h"

int
gtpv1u_new_data_req(
  uint8_t enb_id,
  uint8_t ue_id,
  uint8_t rab_id,
  uint8_t *buffer,
  uint32_t buf_len);
void *gtpv1u_eNB_task(void *args);

int
gtpv1u_create_s1u_tunnel(
  const instance_t instanceP,
  const gtpv1u_enb_create_tunnel_req_t *  const create_tunnel_req_pP,
        gtpv1u_enb_create_tunnel_resp_t * const create_tunnel_resp_pP);


#endif /* GTPV1U_ENB_TASK_H_ */
