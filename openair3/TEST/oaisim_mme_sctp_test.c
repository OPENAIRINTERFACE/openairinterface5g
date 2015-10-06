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
