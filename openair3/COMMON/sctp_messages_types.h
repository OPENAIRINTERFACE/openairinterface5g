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

#ifndef SCTP_MESSAGES_TYPES_H_
#define SCTP_MESSAGES_TYPES_H_

#define SCTP_DATA_IND(mSGpTR)           (mSGpTR)->ittiMsg.sctp_data_ind
#define SCTP_DATA_REQ(mSGpTR)           (mSGpTR)->ittiMsg.sctp_data_req
#define SCTP_INIT_MSG(mSGpTR)           (mSGpTR)->ittiMsg.sctpInit
#define SCTP_CLOSE_ASSOCIATION(mSGpTR)  (mSGpTR)->ittiMsg.sctp_close_association

typedef struct sctp_data_req_s {
  uint8_t  *buffer;
  uint32_t  bufLen;
  uint32_t  assocId;
  uint16_t  stream;
} sctp_data_req_t;

typedef struct sctp_data_ind_s {
  uint8_t  *buffer;           ///< SCTP buffer
  uint32_t  buf_length;       ///< SCTP buffer length
  int32_t   assoc_id;         ///< SCTP physical association ID
  uint8_t   stream;           ///< Stream number on which data had been received
  uint16_t  instreams;        ///< Number of input streams for the SCTP connection between peers
  uint16_t  outstreams;       ///< Number of output streams for the SCTP connection between peers
} sctp_data_ind_t;

typedef struct sctp_init_s {
  /* Request usage of ipv4 */
  unsigned  ipv4:1;
  /* Request usage of ipv6 */
  unsigned  ipv6:1;
  uint8_t   nb_ipv4_addr;
  uint32_t  ipv4_address[10];
  uint8_t   nb_ipv6_addr;
  char     *ipv6_address[10];
  uint16_t  port;
  uint32_t  ppid;
} SctpInit;

typedef struct sctp_close_association_s {
  uint32_t  assoc_id;
} sctp_close_association_t;

typedef struct sctp_new_peer_s {
  uint32_t instreams;
  uint32_t outstreams;
  uint32_t assoc_id;
} sctp_new_peer_t;

#endif /* SCTP_MESSAGES_TYPES_H_ */
