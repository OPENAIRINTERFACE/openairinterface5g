/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
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
