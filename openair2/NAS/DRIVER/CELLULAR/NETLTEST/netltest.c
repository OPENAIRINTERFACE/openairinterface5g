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

//-----------------------------------------------------------------------------
// For FIFOS / NETLINK interface
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <linux/netlink.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/********************
// RRC definitions
 ********************/
#include "nasmt_constant.h"
#include "rrc_nas_sap.h"
#include "rrc_constant.h"
//-----------------------------------------------------------------------------
//#define MAX_PAYLOAD 4096  /* this sould cover the max mtu size*/
struct nlmsghdr *rrcnl_nlh;
int rrcnl_sock_fd;
struct sockaddr_nl rrcnl_src_addr, rrcnl_dest_addr;
struct iovec rrcnl_iov;
struct msghdr rrcnl_msg;

//-----------------------------------------------------------------------------
typedef unsigned int       sdu_size_t;
typedef unsigned int       rb_id_t;

typedef struct pdcp_data_req_header_t {
  rb_id_t             rb_id;
  sdu_size_t           data_size;
  int       inst;
} pdcp_data_req_header_t;

typedef struct pdcp_data_ind_header_t {
  rb_id_t             rb_id;
  sdu_size_t           data_size;
  int       inst;
} pdcp_data_ind_header_t;

#define MAX_PAYLOAD 1024  /* maximum payload size*/
struct sockaddr_nl src_addr, dest_addr;
struct nlmsghdr *nlh = NULL;
struct iovec iov;
int sock_fd;
struct msghdr msg;
#define OAI_IP_DRIVER_NETLINK_ID 31
//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
void foo( int sig )
{
  //-----------------------------------------------------------------------------
  printf("I got cntl-C, closing socket\n");
  close(sock_fd);
  close(rrcnl_sock_fd);
  exit(-1);
}

//-----------------------------------------------------------------------------
int main()
{
  //-----------------------------------------------------------------------------
  struct sigaction newaction;
  int i=0;
  int ret;
  int len, count;

  newaction.sa_handler = foo;

  // register cntl-C handler
  if ( sigaction( SIGINT, &newaction, NULL ) == -1)
    perror("Could not install the new signal handler");

  // create both sockets
  sock_fd = socket(PF_NETLINK, SOCK_RAW,OAI_IP_DRIVER_NETLINK_ID);
  printf("Opened socket with fd %d\n",sock_fd);
  ret = fcntl(sock_fd,F_SETFL,O_NONBLOCK);
  printf("fcntl returns %d\n",ret);

  rrcnl_sock_fd = socket(PF_NETLINK, SOCK_RAW, NAS_RRCNL_ID);
  printf("rrc_ue_netlink_init - Opened socket with fd %d\n", rrcnl_sock_fd);
  ret = fcntl(rrcnl_sock_fd,F_SETFL,O_NONBLOCK);
  printf("rrc_ue_netlink_init - fcntl returns %d\n",ret);

  // set src addresses blocks
  memset(&src_addr, 0, sizeof(src_addr));
  src_addr.nl_family = AF_NETLINK;
  src_addr.nl_pid = 1;//getpid();  /* self pid */
  src_addr.nl_groups = 0;  /* not in mcast groups */
  ret = bind(sock_fd, (struct sockaddr*)&src_addr, sizeof(src_addr));
  printf("bind returns %d\n",ret);

  memset(&rrcnl_src_addr, 0, sizeof(rrcnl_src_addr));
  rrcnl_src_addr.nl_family = AF_NETLINK;
  rrcnl_src_addr.nl_pid = NL_DEST_RRC_PID;//getpid();  /* self pid */
  rrcnl_src_addr.nl_groups = 0;  /* not in mcast groups */
  ret = bind(rrcnl_sock_fd, (struct sockaddr*)&rrcnl_src_addr, sizeof(rrcnl_src_addr));
  printf("rrc_ue_netlink_init - bind returns %d\n",ret);

  // set dest addresses blocks
  memset(&dest_addr, 0, sizeof(dest_addr));
  dest_addr.nl_family = AF_NETLINK;
  dest_addr.nl_pid = 0;   /* For Linux Kernel */
  dest_addr.nl_groups = 0; /* unicast */

  memset(&rrcnl_dest_addr, 0, sizeof(rrcnl_dest_addr));
  rrcnl_dest_addr.nl_family = AF_NETLINK;
  rrcnl_dest_addr.nl_pid = 0;   /* For Linux Kernel */
  rrcnl_dest_addr.nl_groups = 0; /* unicast */

  // set nlh
  nlh=(struct nlmsghdr *)malloc(NLMSG_SPACE(MAX_PAYLOAD));
  /* Fill the netlink message header */
  nlh->nlmsg_len = NLMSG_SPACE(MAX_PAYLOAD);
  nlh->nlmsg_pid = 1;//getpid();  /* self pid */
  nlh->nlmsg_flags = 0;

  rrcnl_nlh=(struct nlmsghdr *)malloc(NLMSG_SPACE(RRC_NAS_MAX_SIZE));
  //memset(rrcnl_nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));
  /* Fill the netlink message header */
  rrcnl_nlh->nlmsg_len = NLMSG_SPACE(RRC_NAS_MAX_SIZE);
  rrcnl_nlh->nlmsg_pid = NL_DEST_RRC_PID;//getpid();  /* self pid */
  rrcnl_nlh->nlmsg_flags = 0;

  // set iov
  iov.iov_base = (void *)nlh;
  iov.iov_len = nlh->nlmsg_len;
  memset(&msg,0,sizeof(msg));
  msg.msg_name = (void *)&dest_addr;
  msg.msg_namelen = sizeof(dest_addr);
  msg.msg_iov = &iov;
  msg.msg_iovlen = 1;

  rrcnl_iov.iov_base = (void *)rrcnl_nlh;
  rrcnl_iov.iov_len = rrcnl_nlh->nlmsg_len;
  memset(&rrcnl_msg,0,sizeof(rrcnl_msg));
  rrcnl_msg.msg_name = (void *)&rrcnl_dest_addr;
  rrcnl_msg.msg_namelen = sizeof(rrcnl_dest_addr);
  rrcnl_msg.msg_iov = &rrcnl_iov;
  rrcnl_msg.msg_iovlen = 1;

  /* Read message from kernel */
  memset(nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));

  /* Read message from kernel */
  memset(rrcnl_nlh, 0, NLMSG_SPACE(MAX_PAYLOAD));

  while (1) {
    len = recvmsg(sock_fd, &msg, 0);
    count = recvmsg(rrcnl_sock_fd, &rrcnl_msg, 0);

    if (len<0) {
      //  exit(-1);
    } else {
      printf("Received PDCP socket with length %d (nlmsg_len = %d)\n",len,nlh->nlmsg_len);
    }

    if (count<0) {
      //  exit(-1);
    } else {
      printf("Received RRC socket with length %d (nlmsg_len = %d)\n",count,rrcnl_nlh->nlmsg_len);
    }

    /*    usleep(1000);
        i=i+1;
        if ((i % 100) == 0)
          printf("%d\n",i);*/
    usleep(50);
    i=i+1;

    if ((i % 1000) == 0)
      printf("%d\n",i);

    /*
    for (i=0;i<nlh->nlmsg_len - sizeof(struct nlmsghdr);i++) {
      printf("%x ",
       ((unsigned char *)NLMSG_DATA(nlh))[i]);
       }
       */
  }

  /* Close Netlink Socket */
  return 0;
}




