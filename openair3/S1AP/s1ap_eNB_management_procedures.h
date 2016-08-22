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

#ifndef S1AP_ENB_MANAGEMENT_PROCEDURES_H_
#define S1AP_ENB_MANAGEMENT_PROCEDURES_H_

struct s1ap_eNB_mme_data_s *s1ap_eNB_get_MME(
  s1ap_eNB_instance_t *instance_p,
  int32_t assoc_id, uint16_t cnx_id);

struct s1ap_eNB_mme_data_s *s1ap_eNB_get_MME_from_instance(s1ap_eNB_instance_t *instance_p);

void s1ap_eNB_remove_mme_desc(s1ap_eNB_instance_t * instance);

void s1ap_eNB_insert_new_instance(s1ap_eNB_instance_t *new_instance_p);

s1ap_eNB_instance_t *s1ap_eNB_get_instance(uint8_t mod_id);

uint16_t s1ap_eNB_fetch_add_global_cnx_id(void);

void s1ap_eNB_prepare_internal_data(void);

#endif /* S1AP_ENB_MANAGEMENT_PROCEDURES_H_ */
