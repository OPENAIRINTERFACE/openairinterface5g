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

/***************************************************************************
                          rb_tool.c  -  User-space utility for driving NASMESH IOCTL interface
                             -------------------
    copyright            : (C) 2008 by Eurecom
    email                : raymond.knopp@eurecom.fr and navid.nikaein@eurecom.fr
 ***************************************************************************

 ***************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <ctype.h>

#include <netinet/in.h>
#include <arpa/inet.h>


//

//#include "ral_cdma_variables.h"
//#include "ral_cdma_proto.h"

#include "rrc_nas_primitives.h"
#include "ioctl.h"
#include "constant.h"

//#include "nasmt_constant.h"
//#include "nasmt_iocontrol.h"



// Global variables
//int sd_graal;
int fd;
//char  myIPAddr[16]; // local IP Address
//int meas_counter;
//struct ralu_priv ru_priv;
//struct ralu_priv *ralupriv;
//ioctl
char dummy_buffer[1024];
struct nas_ioctl gifr;
//int wait_start_nas;

/*
//---------------------------------------------------------------------------
int NAS_RALconnect(void)
//---------------------------------------------------------------------------
{
    struct sockaddr_un remote;
    int len,s;

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("NAS_RALconnect - socket");
        exit(1);
    }
    //
    printf("Trying to connect to NAS ...\n");
    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, SOCKET_RAL_TD_CDMA_NAME);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);

    while (wait_start_graal){
        if (connect(s, (struct sockaddr *)&remote, len) == -1) {
            perror("NAS_RALconnect - waiting for connection - ");
    //        exit(1);
            wait_start_graal = 1;
            sleep(5);
        } else {
            wait_start_graal =0;
            printf("RAL connected to NAS.\n");
        }

    }
    return s;
}

*/

//---------------------------------------------------------------------------
void IAL_NAS_ioctl_init(int inst)
//---------------------------------------------------------------------------
{
  struct nas_msg_statistic_reply *msgrep;
  int err;
  sprintf(gifr.name, "oai%d",inst);
  // Get an UDP IPv6 socket ??
  fd=socket(AF_INET6, SOCK_DGRAM, 0);

  if (fd<0) {
    printf("Error opening socket\n");
    exit(1);
  }

  sprintf(gifr.name, "oai%d",inst);
  gifr.type =  NAS_MSG_STATISTIC_REQUEST;
  memset ((void *)dummy_buffer,0,800);
  gifr.msg= &(dummy_buffer[0]);
  msgrep=(struct nas_msg_statistic_reply *)(gifr.msg);
  printf("ioctl :Statistics requested\n");
  err=ioctl(fd, NAS_IOCTL_RRM, &gifr);

  if (err<0) {
    printf("IOCTL error, err=%d\n",err);
  }

  printf("tx_packets = %u, rx_packets = %u\n", msgrep->tx_packets, msgrep->rx_packets);
  printf("tx_bytes = %u, rx_bytes = %u\n", msgrep->tx_bytes, msgrep->rx_bytes);
  printf("tx_errors = %u, rx_errors = %u\n", msgrep->tx_errors, msgrep->rx_errors);
  printf("tx_dropped = %u, rx_dropped = %u\n", msgrep->tx_dropped, msgrep->rx_dropped);
}


#define ADD_RB 0
#define DEL_RB 1

//---------------------------------------------------------------------------
int main(int argc,char **argv)
//---------------------------------------------------------------------------
{
  int err = 0;
  int c = 0;
  int action=0,rbset=0,cxset=0,instset=0,saddr_ipv4set=0,saddr_ipv6set=0,daddr_ipv4set=0,daddr_ipv6set=0,dscpset=0,mpls_outlabelset=0,mpls_inlabelset=0;
  char rb[100],cx[100],dscp[100],inst[100],mpls_outgoinglabel[100],mpls_incominglabel[100];
  struct nas_msg_rb_establishment_request *msgreq = NULL;
  struct nas_msg_class_add_request *msgreq_class = NULL;
  in_addr_t saddr_ipv4 = 0,daddr_ipv4 = 0;
  struct in6_addr saddr_ipv6,daddr_ipv6;
  unsigned int mpls_outlabel=0,mpls_inlabel=0;
  char addr_str[46];
  // scan options
  rb[0] = '\0';
  cx[0] = '\0';
  dscp[0] = '\0';
  mpls_incominglabel[0] = '\0';
  mpls_outgoinglabel[0] = '\0';

  while ((c = getopt (argc, argv, "adr:i:c:l:m:s:t:x:y:z:")) != -1)
    switch (c) {
      case 'a':
        action = ADD_RB;
        break;

      case 'd':
        action = DEL_RB;
        break;

      case 'r':
        strcpy(rb,optarg);
        rbset = 1;
        break;

      case 'i':
        strcpy(inst,optarg);
        instset = 1;
        break;

      case 'c':
        strcpy(cx,optarg);
        cxset = 1;
        break;

      case 'l':
        strcpy(mpls_outgoinglabel,optarg);
        mpls_outlabelset=1;
        break;

      case 'm':
        strcpy(mpls_incominglabel,optarg);
        mpls_inlabelset=1;
        break;

      case 's': {
        struct in_addr a;
        inet_aton(optarg,&a);
        saddr_ipv4 = a.s_addr;
        saddr_ipv4set = 1;
        break;
      }

      case 't': {
        struct in_addr a;
        inet_aton(optarg,&a);
        daddr_ipv4 = a.s_addr;
        daddr_ipv4set = 1;
        break;
      }

      case 'x':
        printf("IPv6: %s\n",optarg);
        inet_pton(AF_INET6,optarg,(void *)&saddr_ipv6);
        saddr_ipv6set = 1;
        break;

      case 'y':
        inet_pton(AF_INET6,optarg,(void *)&daddr_ipv6);
        daddr_ipv6set = 1;
        break;

      case 'z':
        dscpset=1;
        strcpy(dscp,optarg);
        break;

      case '?':
        if (isprint (optopt))
          fprintf (stderr, "Unknown option `-%c'.\n", optopt);
        else
          fprintf (stderr,
                   "Unknown option character `\\x%x'.\n",
                   optopt);

        return 1;

      default:
        abort ();
    }

  printf ("action = %d, rb = %s,cx = %s\n", action, rb, cx);

  if (rbset==0) {
    printf("ERROR: Specify a RAB id!!\n");
    exit(-1);
  }

  if (cxset==0) {
    printf("ERROR: Specify an LCR !!\n");
    exit(-1);
  }

  if (instset==0) {
    printf("ERROR: Specify an interface !!\n");
    exit(-1);
  }

  if ((mpls_outlabelset == 0) && (saddr_ipv4set==0) && (saddr_ipv6set==0)) {
    printf("ERROR: Specify a source IP address\n");
    exit(-1);
  }

  if ((mpls_outlabelset == 0) && (daddr_ipv4set==0) && (daddr_ipv6set==0)) {
    printf("ERROR: Specify a destination IP address\n");
    exit(-1);
  }

  if ((mpls_outlabelset == 1) && (mpls_inlabelset == 0)) {
    printf("ERROR: Specify an incoming MPLS label\n");
    exit(-1);
  }

  if ((mpls_inlabelset == 1) && (mpls_outlabelset == 0)) {
    printf("ERROR: Specify an outgoing MPLS label\n");
    exit(-1);
  }

  IAL_NAS_ioctl_init(atoi(inst));
  msgreq = (struct nas_msg_rb_establishment_request *)(gifr.msg);
  msgreq->rab_id = atoi(rb);
  msgreq->lcr = atoi(cx);
  msgreq->qos = 0;

  if (action == ADD_RB) {
    gifr.type =  NAS_MSG_RB_ESTABLISHMENT_REQUEST;
    err=ioctl(fd, NAS_IOCTL_RRM, &gifr);

    if (err == -1) perror("ioctl");

    if (saddr_ipv4set == 1) {
      msgreq_class = (struct nas_msg_class_add_request *)(gifr.msg);
      msgreq_class->rab_id = atoi(rb);
      msgreq_class->lcr = atoi(cx);
      msgreq_class->version = 4;
      msgreq_class->classref = 0 + (msgreq_class->lcr<<3);
      msgreq_class->dir = NAS_DIRECTION_SEND;
      msgreq_class->fct = NAS_FCT_QOS_SEND;
      msgreq_class->saddr.ipv4 = saddr_ipv4;
      msgreq_class->daddr.ipv4 = daddr_ipv4;
      // TO BE FIXED WHEN WE CAN SPECIFY A PROTOCOL-based rule
      msgreq_class->protocol = NAS_PROTOCOL_DEFAULT;

      if (dscpset==0)
        msgreq_class->dscp=0;
      else
        msgreq_class->dscp=atoi(dscp);

      gifr.type =  NAS_MSG_CLASS_ADD_REQUEST;
      err=ioctl(fd, NAS_IOCTL_RRM, &gifr);

      if (err == -1) perror("ioctl");

      msgreq_class->rab_id = atoi(rb);
      msgreq_class->lcr = atoi(cx);
      msgreq_class->classref = 1+(msgreq_class->lcr<<3);
      msgreq_class->dir = NAS_DIRECTION_RECEIVE;
      msgreq_class->daddr.ipv4 = saddr_ipv4;
      msgreq_class->saddr.ipv4 = daddr_ipv4;
      gifr.type =  NAS_MSG_CLASS_ADD_REQUEST;
      err=ioctl(fd, NAS_IOCTL_RRM, &gifr);

      if (err == -1) perror("ioctl");
    }

    if (saddr_ipv6set == 1) {
      msgreq_class = (struct nas_msg_class_add_request *)(gifr.msg);
      msgreq_class->rab_id = atoi(rb);
      msgreq_class->lcr = atoi(cx);
      msgreq_class->version = 6;

      if (dscpset==0)
        msgreq_class->dscp=0;
      else
        msgreq_class->dscp=atoi(dscp);

      msgreq_class->classref = 2+(msgreq_class->lcr<<3);
      msgreq_class->dir=NAS_DIRECTION_SEND;
      msgreq_class->fct=NAS_FCT_QOS_SEND;
      // TO BE FIXED WHEN WE CAN SPECIFY A PROTOCOL-based rule
      msgreq_class->protocol = NAS_PROTOCOL_DEFAULT;
      memcpy(&msgreq_class->saddr.ipv6,&saddr_ipv6,16);
      memcpy(&msgreq_class->daddr.ipv6,&daddr_ipv6,16);
      inet_ntop(AF_INET6,(void *)&saddr_ipv6,addr_str,46);
      printf("IPV6: Source %s\n",addr_str);
      inet_ntop(AF_INET6,(void *)&daddr_ipv6,addr_str,46);
      printf("IPV6: Dest %s\n",addr_str);
      gifr.type =  NAS_MSG_CLASS_ADD_REQUEST;
      err=ioctl(fd, NAS_IOCTL_RRM, &gifr);

      if (err == -1) perror("ioctl");

      msgreq_class->rab_id = atoi(rb);
      msgreq_class->lcr = atoi(cx);
      msgreq_class->classref = 3+(msgreq_class->lcr<<3);
      msgreq_class->dir=NAS_DIRECTION_RECEIVE;
      memcpy(&msgreq_class->daddr.ipv6,&saddr_ipv6,16);
      memcpy(&msgreq_class->saddr.ipv6,&daddr_ipv6,16);
      gifr.type =  NAS_MSG_CLASS_ADD_REQUEST;
      err=ioctl(fd, NAS_IOCTL_RRM, &gifr);

      if (err == -1) perror("ioctl");
    }

    if (mpls_inlabelset == 1) {
      msgreq_class = (struct nas_msg_class_add_request *)(gifr.msg);
      msgreq_class->rab_id = atoi(rb);
      msgreq_class->lcr = atoi(cx);
      msgreq_class->version = NAS_MPLS_VERSION_CODE;

      if (dscpset==0)
        msgreq_class->dscp=0;
      else
        msgreq_class->dscp=atoi(dscp);

      msgreq_class->classref = 4 + (msgreq_class->lcr<<3);
      msgreq_class->dir=NAS_DIRECTION_SEND;
      msgreq_class->fct=NAS_FCT_QOS_SEND;
      // TO BE FIXED WHEN WE CAN SPECIFY A PROTOCOL-based rule
      msgreq_class->protocol = NAS_PROTOCOL_DEFAULT;
      mpls_outlabel = atoi(mpls_outgoinglabel);
      printf("Setting MPLS outlabel %u with exp %d\n",mpls_outlabel,msgreq_class->dscp);
      msgreq_class->daddr.mpls_label = mpls_outlabel;
      gifr.type =  NAS_MSG_CLASS_ADD_REQUEST;
      err=ioctl(fd, NAS_IOCTL_RRM, &gifr);

      if (err == -1) perror("ioctl");

      msgreq_class->rab_id = atoi(rb);
      msgreq_class->lcr = atoi(cx);
      msgreq_class->classref = 5 + (msgreq_class->lcr<<3);
      msgreq_class->dir=NAS_DIRECTION_RECEIVE;
      // TO BE FIXED WHEN WE CAN SPECIFY A PROTOCOL-based rule
      msgreq_class->protocol = NAS_PROTOCOL_DEFAULT;
      mpls_inlabel  = atoi(mpls_incominglabel);
      printf("Setting MPLS inlabel %u with exp %d\n",mpls_inlabel,msgreq_class->dscp);
      msgreq_class->daddr.mpls_label = mpls_inlabel;
      gifr.type =  NAS_MSG_CLASS_ADD_REQUEST;
      err=ioctl(fd, NAS_IOCTL_RRM, &gifr);

      if (err == -1) perror("ioctl");
    }
  } else if (action == DEL_RB) {
    gifr.type =  NAS_MSG_RB_RELEASE_REQUEST;
    err=ioctl(fd, NAS_IOCTL_RRM, &gifr);

    if (err == -1) perror("ioctl");
  }
}
