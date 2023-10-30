/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
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

/*! \file rrc_gNB_GTPV1U.h
 * \brief rrc GTPV1U procedures for gNB
 * \author Lionel GAUTHIER, Panos MATZAKOS
 * \version 1.0
 * \company Eurecom
 * \email: lionel.gauthier@eurecom.fr, panagiotis.matzakos@eurecom.fr
 */

#ifndef RRC_GNB_GTPV1U_H_
#define RRC_GNB_GTPV1U_H_


int
rrc_gNB_process_GTPV1U_CREATE_TUNNEL_RESP(
  const protocol_ctxt_t *const ctxt_pP,
  const gtpv1u_enb_create_tunnel_resp_t *const create_tunnel_resp_pP,
  uint8_t                         *inde_list
);

#endif
