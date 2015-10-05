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
/*! \file sgw_lite_task.c
* \brief
* \author Lionel Gauthier
* \company Eurecom
* \email: lionel.gauthier@eurecom.fr
*/
#define SGW_LITE
#define SGW_LITE_TASK_C

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <assert.h>

#include "intertask_interface.h"
#include "mme_config.h"
#include "sgw_lite_defs.h"
#include "sgw_lite_handlers.h"
#include "sgw_lite.h"
#include "hashtable.h"
#include "spgw_config.h"
#include "pgw_lite_paa.h"
#include "msc.h"

spgw_config_t spgw_config;
sgw_app_t     sgw_app;
pgw_app_t     pgw_app;

static void *sgw_lite_intertask_interface(void *args_p)
{
  itti_mark_task_ready(TASK_SPGW_APP);
  MSC_START_USE();

  while(1) {
    MessageDef *received_message_p;
    itti_receive_msg(TASK_SPGW_APP, &received_message_p);

    switch (ITTI_MSG_ID(received_message_p)) {
    case SGW_CREATE_SESSION_REQUEST: {
      /* We received a create session request from MME (with GTP abstraction here)
       * procedures might be:
       *      E-UTRAN Initial Attach
       *      UE requests PDN connectivity
       */
      sgw_lite_handle_create_session_request(&received_message_p->ittiMsg.sgwCreateSessionRequest);
    }
    break;

    case SGW_MODIFY_BEARER_REQUEST: {
      sgw_lite_handle_modify_bearer_request(&received_message_p->ittiMsg.sgwModifyBearerRequest);
    }
    break;

    case SGW_RELEASE_ACCESS_BEARERS_REQUEST: {
      sgw_lite_handle_release_access_bearers_request(&received_message_p->ittiMsg.sgwReleaseAccessBearersRequest);
    }
    break;

    case SGW_DELETE_SESSION_REQUEST: {
      sgw_lite_handle_delete_session_request(&received_message_p->ittiMsg.sgwDeleteSessionRequest);
    }
    break;

    case GTPV1U_CREATE_TUNNEL_RESP: {
      SPGW_APP_DEBUG("Received teid for S1-U: %u and status: %s\n",
                     received_message_p->ittiMsg.gtpv1uCreateTunnelResp.S1u_teid ,
                     received_message_p->ittiMsg.gtpv1uCreateTunnelResp.status == 0 ? "Success" :
                     "Failure");
      sgw_lite_handle_gtpv1uCreateTunnelResp(&received_message_p->ittiMsg.gtpv1uCreateTunnelResp);
    }
    break;

    case GTPV1U_UPDATE_TUNNEL_RESP: {
      sgw_lite_handle_gtpv1uUpdateTunnelResp(&received_message_p->ittiMsg.gtpv1uUpdateTunnelResp);
    }
    break;

    case SGI_CREATE_ENDPOINT_RESPONSE: {
      sgw_lite_handle_sgi_endpoint_created(&received_message_p->ittiMsg.sgiCreateEndpointResp);
    }
    break;

    case SGI_UPDATE_ENDPOINT_RESPONSE: {
      sgw_lite_handle_sgi_endpoint_updated(&received_message_p->ittiMsg.sgiUpdateEndpointResp);
    }
    break;

    case TERMINATE_MESSAGE: {
      itti_exit_task();
    }
    break;

    default: {
      SPGW_APP_DEBUG("Unkwnon message ID %d:%s\n",
                     ITTI_MSG_ID(received_message_p),
                     ITTI_MSG_NAME(received_message_p));
    }
    break;
    }

    itti_free(ITTI_MSG_ORIGIN_ID(received_message_p), received_message_p);
    received_message_p = NULL;
  }

  return NULL;
}



int sgw_lite_init(char* config_file_name_pP)
{
  SPGW_APP_DEBUG("Initializing SPGW-APP  task interface\n");
  spgw_system("modprobe ip_tables",    SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system("modprobe x_tables",     SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system("iptables -P INPUT ACCEPT", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("iptables -F INPUT",     SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("iptables -P OUTPUT ACCEPT", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("iptables -F OUTPUT",    SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("iptables -P FORWARD ACCEPT", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("iptables -F FORWARD",   SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("iptables -t nat    -F", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("iptables -t mangle -F", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("iptables -t filter -F", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("iptables -t raw    -F", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("iptables -t nat    -Z", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("iptables -t mangle -Z", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("iptables -t filter -Z", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("iptables -t raw    -Z", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("ip route flush cache",  SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("rmmod iptable_raw    > /dev/null 2>&1", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("rmmod iptable_mangle > /dev/null 2>&1", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("rmmod iptable_nat    > /dev/null 2>&1", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("rmmod iptable_filter > /dev/null 2>&1", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("rmmod ip_tables      > /dev/null 2>&1", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("rmmod xt_state xt_mark xt_tcpudp xt_connmark ipt_LOG ipt_MASQUERADE > /dev/null 2>&1", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("rmmod x_tables       > /dev/null 2>&1", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("rmmod nf_conntrack_netlink nfnetlink nf_nat nf_conntrack_ipv4 nf_conntrack  > /dev/null 2>&1", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("modprobe ip_tables",      SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system("modprobe iptable_filter", SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system("modprobe iptable_mangle", SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system("modprobe iptable_nat",    SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system("modprobe iptable_raw",    SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system("modprobe ipt_MASQUERADE", SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system("modprobe ipt_LOG",        SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system("modprobe nf_conntrack",   SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system("modprobe nf_conntrack_ipv4", SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system("modprobe nf_nat",         SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system("modprobe x_tables",       SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);
  spgw_system("sysctl -w net.ipv4.ip_forward=1", SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("sysctl -w net.ipv4.conf.all.accept_local=1",SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("sysctl -w net.ipv4.conf.all.log_martians=1",SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("sysctl -w net.ipv4.conf.all.route_localnet=1",SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("sysctl -w net.ipv4.conf.all.rp_filter=0",SPGW_WARN_ON_ERROR, __FILE__, __LINE__);
  spgw_system("sync",                    SPGW_ABORT_ON_ERROR, __FILE__, __LINE__);

  spgw_config_init(config_file_name_pP, &spgw_config);
  pgw_lite_load_pool_ip_addresses();

  sgw_app.s11teid2mme_hashtable = hashtable_create (8192, NULL, NULL);

  if (sgw_app.s11teid2mme_hashtable == NULL) {
    perror("hashtable_create");
    SPGW_APP_DEBUG("Initializing SPGW-APP task interface: ERROR\n");
    return -1;
  }

  sgw_app.s1uteid2enb_hashtable = hashtable_create (8192, NULL, NULL);

  if (sgw_app.s1uteid2enb_hashtable == NULL) {
    perror("hashtable_create");
    SPGW_APP_DEBUG("Initializing SPGW-APP task interface: ERROR\n");
    return -1;
  }


  sgw_app.s11_bearer_context_information_hashtable = hashtable_create (8192, NULL, sgw_lite_cm_free_s_plus_p_gw_eps_bearer_context_information);

  if (sgw_app.s11_bearer_context_information_hashtable == NULL) {
    perror("hashtable_create");
    SPGW_APP_DEBUG("Initializing SPGW-APP task interface: ERROR\n");
    return -1;
  }

  sgw_app.sgw_interface_name_for_S1u_S12_S4_up = spgw_config.sgw_config.ipv4.sgw_interface_name_for_S1u_S12_S4_up;
  sgw_app.sgw_ip_address_for_S1u_S12_S4_up     = spgw_config.sgw_config.ipv4.sgw_ipv4_address_for_S1u_S12_S4_up;
  sgw_app.sgw_interface_name_for_S11_S4        = spgw_config.sgw_config.ipv4.sgw_interface_name_for_S11;
  sgw_app.sgw_ip_address_for_S11_S4            = spgw_config.sgw_config.ipv4.sgw_ipv4_address_for_S11;
  //sgw_app.sgw_ip_address_for_S5_S8_cp          = spgw_config.sgw_config.ipv4.sgw_ip_address_for_S5_S8_cp;
  sgw_app.sgw_ip_address_for_S5_S8_up          = spgw_config.sgw_config.ipv4.sgw_ipv4_address_for_S5_S8_up;

  if (itti_create_task(TASK_SPGW_APP,
                       &sgw_lite_intertask_interface, NULL) < 0) {
    perror("pthread_create");
    SPGW_APP_DEBUG("Initializing SPGW-APP task interface: ERROR\n");
    return -1;
  }

  SPGW_APP_DEBUG("Initializing SPGW-APP task interface: DONE\n");
  return 0;
}
