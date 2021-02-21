/* Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file gtpv1u_gNB_task.h
* \brief
* \author Lionel Gauthier Panos Matzakos
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/

#ifndef GTPV1U_GNB_TASK_H_
#define GTPV1U_GNB_TASK_H_

int   gtpv1u_gNB_init(void);

void *gtpv1u_gNB_task(void *args);

void *nr_gtpv1u_gNB_task(void *args);

int
gtpv1u_create_ngu_tunnel(
  const instance_t instanceP,
  const gtpv1u_gnb_create_tunnel_req_t *  const create_tunnel_req_pP,
        gtpv1u_gnb_create_tunnel_resp_t * const create_tunnel_resp_pP);

int
gtpv1u_update_ngu_tunnel(
  const instance_t                              instanceP,
  const gtpv1u_gnb_create_tunnel_req_t *const  create_tunnel_req_pP,
  const rnti_t                                  prior_rnti
);

#endif /* GTPV1U_GNB_TASK_H_ */

