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

MESSAGE_DEF(SCTP_NEW_ASSOCIATION_REQ ,      MESSAGE_PRIORITY_MED, sctp_new_association_req_t           , sctp_new_association_req)
MESSAGE_DEF(SCTP_NEW_ASSOCIATION_REQ_MULTI, MESSAGE_PRIORITY_MED, sctp_new_association_req_multi_t     , sctp_new_association_req_multi)
MESSAGE_DEF(SCTP_NEW_ASSOCIATION_RESP,      MESSAGE_PRIORITY_MED, sctp_new_association_resp_t          , sctp_new_association_resp)
MESSAGE_DEF(SCTP_NEW_ASSOCIATION_IND ,      MESSAGE_PRIORITY_MED, sctp_new_association_ind_t           , sctp_new_association_ind)
MESSAGE_DEF(SCTP_REGISTER_UPPER_LAYER,      MESSAGE_PRIORITY_MED, sctp_listener_register_upper_layer_t , sctp_listener_register_upper_layer)
MESSAGE_DEF(SCTP_DATA_REQ,                  MESSAGE_PRIORITY_MED, sctp_data_req_t                      , sctp_data_req)
MESSAGE_DEF(SCTP_DATA_IND,                  MESSAGE_PRIORITY_MED, sctp_data_ind_t                      , sctp_data_ind)
MESSAGE_DEF(SCTP_INIT_MSG,                  MESSAGE_PRIORITY_MED, sctp_init_t                          , sctp_init)
MESSAGE_DEF(SCTP_INIT_MSG_MULTI_REQ,        MESSAGE_PRIORITY_MED, sctp_init_t                          , sctp_init_multi)
MESSAGE_DEF(SCTP_INIT_MSG_MULTI_CNF,        MESSAGE_PRIORITY_MED, sctp_init_msg_multi_cnf_t            , sctp_init_msg_multi_cnf)
MESSAGE_DEF(SCTP_CLOSE_ASSOCIATION,         MESSAGE_PRIORITY_MAX, sctp_close_association_t             , sctp_close_association)
