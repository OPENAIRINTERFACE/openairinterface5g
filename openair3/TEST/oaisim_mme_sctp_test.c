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

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>

#include "sctp_primitives_client.h"

#include "s1ap_common.h"
#include "s1ap_eNB.h"
#include "s1ap_mme.h"
#include "s1ap_ies_defs.h"

#include "s1ap_eNB_encoder.h"
#include "s1ap_eNB_decoder.h"

int recv_callback(uint32_t  assocId,
                  uint32_t  stream,
                  uint8_t  *buffer,
                  uint32_t  length)
{

  free(buffer);

  return 0;
}

int sctp_connected(void     *args,
                   uint32_t  assocId,
                   uint32_t  instreams,
                   uint32_t  outstreams)
{

  return 0;
}

int main(int argc, char *argv[])
{
  asn_enc_rval_t retVal;

  int i;

  for (i = 0; i < nb_eNB; i++) {
    sctp_connect_to_remote_host(ip_addr, 36412, NULL, sctp_connected, recv_callback);
  }

  while (1) {
    sleep(1);
  }

  sctp_terminate();

  return(0);
}
