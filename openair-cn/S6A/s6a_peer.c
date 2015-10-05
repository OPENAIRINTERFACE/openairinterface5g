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
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Compus SophiaTech 450, route des chappes, 06451 Biot, France.

 *******************************************************************************/

/*! \file s6a_peers.c
 * \brief Add a new entity to the list of peers to connect
 * \author Sebastien ROUX <sebastien.roux@eurecom.fr>
 * \date 2013
 * \version 0.1
 */

#include <stdint.h>
#include <unistd.h>

#include "common_types.h"
#include "intertask_interface.h"
#include "s6a_defs.h"
#include "s6a_messages.h"

#include "assertions.h"

void s6a_peer_connected_cb(struct peer_info *info, void *arg)
{
  if (info == NULL) {
    S6A_ERROR("Failed to connect to HSS entity\n");
  } else {
    MessageDef     *message_p;

    S6A_DEBUG("Peer %*s is now connected...\n",
              (int)info->pi_diamidlen, info->pi_diamid);

    /* Inform S1AP that connection to HSS is established */
    message_p = itti_alloc_new_message(TASK_S6A, ACTIVATE_MESSAGE);

    itti_send_msg_to_task(TASK_S1AP, INSTANCE_DEFAULT, message_p);

  }

  /* For test */
#if 0
  s6a_auth_info_req_t s6a_air;

  memset(&s6a_air, 0, sizeof(s6a_auth_info_req_t));
  sprintf(s6a_air.imsi, "%14llu", 20834123456789ULL);

  s6a_air.nb_of_vectors = 1;
  s6a_air.visited_plmn.MCCdigit2 = 0,
                       s6a_air.visited_plmn.MCCdigit1 = 8,
                                            s6a_air.visited_plmn.MCCdigit3 = 2,
                                                                 s6a_air.visited_plmn.MNCdigit1 = 0,
                                                                                      s6a_air.visited_plmn.MNCdigit2 = 3,
                                                                                                           s6a_air.visited_plmn.MNCdigit3 = 4,

                                                                                                                                s6a_generate_authentication_info_req(&s6a_air);
  // #else
  //     s6a_update_location_req_t s6a_ulr;
  //
  //     memset(&s6a_ulr, 0, sizeof(s6a_update_location_req_t));
  //
  //     sprintf(s6a_ulr.imsi, "%14llu", 20834123456789ULL);
  //     s6a_ulr.initial_attach = INITIAL_ATTACH;
  //     s6a_ulr.rat_type = RAT_EUTRAN;
  //     s6a_generate_update_location(&s6a_ulr);
#endif
}

int s6a_fd_new_peer(void)
{
  char host_name[100];
  size_t host_name_len;
  char *hss_name;

  int ret = 0;
  struct peer_info info;

  memset(&info, 0, sizeof(struct peer_info));

  if (config_read_lock(&mme_config) != 0) {
    S6A_ERROR("Failed to lock configuration for reading\n");
    return -1;
  }

  if(fd_g_config->cnf_diamid != NULL) {
    free(fd_g_config->cnf_diamid);
    fd_g_config->cnf_diamid_len = 0;
  }

  DevAssert(gethostname(host_name, 100) == 0);

  host_name_len = strlen(host_name);
  host_name[host_name_len]   = '.';
  host_name[host_name_len+1] = '\0';

  strcat(host_name, mme_config.realm);
  fd_g_config->cnf_diamid = strdup(host_name);
  fd_g_config->cnf_diamid_len = strlen(fd_g_config->cnf_diamid);

  S6A_DEBUG("Diameter identity of MME: %s with length: %zd\n",
            fd_g_config->cnf_diamid, fd_g_config->cnf_diamid_len);

  hss_name = calloc(1,100);
  strcat(hss_name, mme_config.s6a_config.hss_host_name);
  strcat(hss_name, ".");
  strcat(hss_name, mme_config.realm);
  info.pi_diamid = hss_name;
  info.pi_diamidlen = strlen(info.pi_diamid);

  S6A_DEBUG("Diameter identity of HSS: %s with length: %zd\n",
            info.pi_diamid, info.pi_diamidlen);

  info.config.pic_flags.sec  = PI_SEC_NONE;
  info.config.pic_flags.pro4 = PI_P4_SCTP;

  info.config.pic_flags.persist = PI_PRST_NONE;

  CHECK_FCT(fd_peer_add(&info, "", s6a_peer_connected_cb, NULL));

  if (config_unlock(&mme_config) != 0) {
    S6A_ERROR("Failed to unlock configuration\n");
    return -1;
  }

  return ret;
}
