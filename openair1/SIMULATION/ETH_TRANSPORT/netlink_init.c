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

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/
/*! \file netlink_init.c
* \brief initiate the netlink socket for communication with nas dirver
* \author Navid Nikaein and Raymomd Knopp
* \date 2011
* \version 1.0
* \company Eurecom
* \email: navid.nikaein@eurecom.fr
*/

#include <sys/socket.h>
#include <linux/netlink.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <fcntl.h>
#include <errno.h>
#include "platform_constants.h"

char nl_rx_buf[NL_MAX_PAYLOAD];

struct sockaddr_nl nas_src_addr, nas_dest_addr;
struct nlmsghdr *nas_nlh_tx = NULL;
struct nlmsghdr *nas_nlh_rx = NULL;
struct iovec nas_iov_tx;
struct iovec nas_iov_rx = {nl_rx_buf, sizeof(nl_rx_buf)};
int nas_sock_fd;
struct msghdr nas_msg_tx;
struct msghdr nas_msg_rx;

#define GRAAL_NETLINK_ID 31

int netlink_init(void)
{
  int ret;

  nas_sock_fd = socket(PF_NETLINK, SOCK_RAW,GRAAL_NETLINK_ID);

  if (nas_sock_fd == -1) {
    printf("[NETLINK] Error opening socket %d (%d:%s)\n",nas_sock_fd,errno, strerror(errno));
#if defined(LINK_ENB_PDCP_TO_IP_DRIVER)
    exit(1);
#endif
  }

  printf("[NETLINK]Opened socket with fd %d\n",nas_sock_fd);

#if !defined(PDCP_USE_NETLINK_QUEUES)
  ret = fcntl(nas_sock_fd,F_SETFL,O_NONBLOCK);

  if (ret == -1) {
    printf("[NETLINK] Error fcntl (%d:%s)\n",errno, strerror(errno));
#if defined(LINK_ENB_PDCP_TO_IP_DRIVER)
    exit(1);
#endif
  }

#endif

  memset(&nas_src_addr, 0, sizeof(nas_src_addr));
  nas_src_addr.nl_family = AF_NETLINK;
  nas_src_addr.nl_pid = 1;//getpid();  /* self pid */
  nas_src_addr.nl_groups = 0;  /* not in mcast groups */
  ret = bind(nas_sock_fd, (struct sockaddr*)&nas_src_addr, sizeof(nas_src_addr));



  memset(&nas_dest_addr, 0, sizeof(nas_dest_addr));
  nas_dest_addr.nl_family = AF_NETLINK;
  nas_dest_addr.nl_pid = 0;   /* For Linux Kernel */
  nas_dest_addr.nl_groups = 0; /* unicast */

  // TX PART
  nas_nlh_tx=(struct nlmsghdr *)malloc(NLMSG_SPACE(NL_MAX_PAYLOAD));
  memset(nas_nlh_tx, 0, NLMSG_SPACE(NL_MAX_PAYLOAD));
  /* Fill the netlink message header */
  nas_nlh_tx->nlmsg_len = NLMSG_SPACE(NL_MAX_PAYLOAD);
  nas_nlh_tx->nlmsg_pid = 1;//getpid();  /* self pid */
  nas_nlh_tx->nlmsg_flags = 0;

  nas_iov_tx.iov_base = (void *)nas_nlh_tx;
  nas_iov_tx.iov_len = nas_nlh_tx->nlmsg_len;
  memset(&nas_msg_tx,0,sizeof(nas_msg_tx));
  nas_msg_tx.msg_name = (void *)&nas_dest_addr;
  nas_msg_tx.msg_namelen = sizeof(nas_dest_addr);
  nas_msg_tx.msg_iov = &nas_iov_tx;
  nas_msg_tx.msg_iovlen = 1;


  // RX PART
  memset(&nas_msg_rx,0,sizeof(nas_msg_rx));
  nas_msg_rx.msg_name = (void *)&nas_src_addr;
  nas_msg_rx.msg_namelen = sizeof(nas_src_addr);
  nas_msg_rx.msg_iov = &nas_iov_rx;
  nas_msg_rx.msg_iovlen = 1;

  return(nas_sock_fd);
}
