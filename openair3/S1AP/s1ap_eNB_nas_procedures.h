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

#ifndef S1AP_ENB_NAS_PROCEDURES_H_
#define S1AP_ENB_NAS_PROCEDURES_H_

int s1ap_eNB_handle_nas_downlink(
  const uint32_t               assoc_id,
  const uint32_t               stream,
  struct s1ap_message_s* message_p);

int s1ap_eNB_nas_uplink(instance_t instance, s1ap_uplink_nas_t *s1ap_uplink_nas_p);

void s1ap_eNB_nas_non_delivery_ind(instance_t instance,
                                   s1ap_nas_non_delivery_ind_t *s1ap_nas_non_delivery_ind);

int s1ap_eNB_handle_nas_first_req(
  instance_t instance, s1ap_nas_first_req_t *s1ap_nas_first_req_p);

int s1ap_eNB_initial_ctxt_resp(
  instance_t instance, s1ap_initial_context_setup_resp_t *initial_ctxt_resp_p);

int s1ap_eNB_ue_capabilities(instance_t instance,
                             s1ap_ue_cap_info_ind_t *ue_cap_info_ind_p);


#endif /* S1AP_ENB_NAS_PROCEDURES_H_ */
