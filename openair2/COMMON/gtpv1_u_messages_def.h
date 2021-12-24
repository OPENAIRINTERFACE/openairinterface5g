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

MESSAGE_DEF(GTPV1U_ENB_UPDATE_TUNNEL_REQ,   MESSAGE_PRIORITY_MED, gtpv1u_enb_update_tunnel_req_t,  Gtpv1uUpdateTunnelReq)
MESSAGE_DEF(GTPV1U_ENB_UPDATE_TUNNEL_RESP,  MESSAGE_PRIORITY_MED, gtpv1u_enb_update_tunnel_resp_t, Gtpv1uUpdateTunnelResp)
MESSAGE_DEF(GTPV1U_ENB_DELETE_TUNNEL_REQ,   MESSAGE_PRIORITY_MED, gtpv1u_enb_delete_tunnel_req_t,  Gtpv1uDeleteTunnelReq)
MESSAGE_DEF(GTPV1U_ENB_DELETE_TUNNEL_RESP,  MESSAGE_PRIORITY_MED, gtpv1u_enb_delete_tunnel_resp_t, Gtpv1uDeleteTunnelResp)
MESSAGE_DEF(GTPV1U_ENB_TUNNEL_DATA_IND,     MESSAGE_PRIORITY_MED, gtpv1u_enb_tunnel_data_ind_t,    Gtpv1uTunnelDataInd)
MESSAGE_DEF(GTPV1U_ENB_TUNNEL_DATA_REQ,     MESSAGE_PRIORITY_MED, gtpv1u_enb_tunnel_data_req_t,    Gtpv1uTunnelDataReq)
MESSAGE_DEF(GTPV1U_ENB_DATA_FORWARDING_REQ, MESSAGE_PRIORITY_MED, gtpv1u_enb_data_forwarding_req_t,Gtpv1uDataForwardingReq)
MESSAGE_DEF(GTPV1U_ENB_DATA_FORWARDING_IND, MESSAGE_PRIORITY_MED, gtpv1u_enb_data_forwarding_ind_t,Gtpv1uDataForwardingInd)
MESSAGE_DEF(GTPV1U_ENB_END_MARKER_REQ,      MESSAGE_PRIORITY_MED, gtpv1u_enb_end_marker_req_t,    Gtpv1uEndMarkerReq)
MESSAGE_DEF(GTPV1U_ENB_END_MARKER_IND,      MESSAGE_PRIORITY_MED, gtpv1u_enb_end_marker_ind_t,    Gtpv1uEndMarkerInd)
MESSAGE_DEF(GTPV1U_ENB_S1_REQ,        MESSAGE_PRIORITY_MED, Gtpv1uS1Req,    gtpv1uS1Req)

MESSAGE_DEF(GTPV1U_GNB_DELETE_TUNNEL_REQ,   MESSAGE_PRIORITY_MED, gtpv1u_gnb_delete_tunnel_req_t,  NRGtpv1uDeleteTunnelReq)
MESSAGE_DEF(GTPV1U_GNB_DELETE_TUNNEL_RESP,  MESSAGE_PRIORITY_MED, gtpv1u_gnb_delete_tunnel_resp_t, NRGtpv1uDeleteTunnelResp)
MESSAGE_DEF(GTPV1U_GNB_NG_REQ,              MESSAGE_PRIORITY_MED, Gtpv1uNGReq,                     gtpv1uNGReq)
MESSAGE_DEF(GTPV1U_GNB_TUNNEL_DATA_REQ,     MESSAGE_PRIORITY_MED, gtpv1u_gnb_tunnel_data_req_t,    NRGtpv1uTunnelDataReq)
