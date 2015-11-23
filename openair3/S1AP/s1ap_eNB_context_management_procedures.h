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

#ifndef S1AP_ENB_CONTEXT_MANAGEMENT_PROCEDURES_H_
#define S1AP_ENB_CONTEXT_MANAGEMENT_PROCEDURES_H_


int s1ap_ue_context_release_complete(instance_t instance,
                                     s1ap_ue_release_complete_t *ue_release_complete_p);

int s1ap_ue_context_release_req(instance_t instance,
                                s1ap_ue_release_req_t *ue_release_req_p);

#endif /* S1AP_ENB_CONTEXT_MANAGEMENT_PROCEDURES_H_ */
