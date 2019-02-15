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

#include <netinet/in.h>
#include <arpa/inet.h>

//#include "rrc_nas_primitives.h"
#include "ioctl.h"
#include "constant.h"


#define NIPADDR(addr) \
  (uint8_t)(addr & 0x000000FF), \
  (uint8_t)((addr & 0x0000FF00) >> 8), \
  (uint8_t)((addr & 0x00FF0000) >> 16), \
  (uint8_t)((addr & 0xFF000000) >> 24)



#define NIP6ADDR(addr) \
  ntohs((addr)->s6_addr16[0]), \
  ntohs((addr)->s6_addr16[1]), \
  ntohs((addr)->s6_addr16[2]), \
  ntohs((addr)->s6_addr16[3]), \
  ntohs((addr)->s6_addr16[4]), \
  ntohs((addr)->s6_addr16[5]), \
  ntohs((addr)->s6_addr16[6]), \
  ntohs((addr)->s6_addr16[7])


// Global variables

int fd;
//ioctl
char dummy_buffer[1024];
struct oai_nw_drv_ioctl gifr;
//int wait_start_nas;

//---------------------------------------------------------------------------
void IAL_NAS_ioctl_init(int inst)
//---------------------------------------------------------------------------
{
  struct oai_nw_drv_msg_statistic_reply *msgrep;
  int err,rc;
  sprintf(gifr.name, "oai%d",inst);
  // Get an UDP IPv6 socket ??
  fd=socket(AF_INET6, SOCK_DGRAM, 0);

  if (fd<0) {
    printf("Error opening socket\n");
    exit(1);
  }

  sprintf(gifr.name, "oai%d",inst);
  gifr.type =  OAI_NW_DRV_MSG_STATISTIC_REQUEST;
  memset ((void *)dummy_buffer,0,800);
  gifr.msg= &(dummy_buffer[0]);
  msgrep=(struct oai_nw_drv_msg_statistic_reply *)(gifr.msg);
  printf("ioctl :Statistics requested\n");
  err=ioctl(fd, OAI_NW_DRV_IOCTL_RRM, &gifr);

  if (err<0) {
    printf("IOCTL error, err=%d\n",err);
    rc = -1;
  }

  printf("tx_packets = %u, rx_packets = %u\n", msgrep->tx_packets, msgrep->rx_packets);
  printf("tx_bytes = %u, rx_bytes = %u\n", msgrep->tx_bytes, msgrep->rx_bytes);
  printf("tx_errors = %u, rx_errors = %u\n", msgrep->tx_errors, msgrep->rx_errors);
  printf("tx_dropped = %u, rx_dropped = %u\n", msgrep->tx_dropped, msgrep->rx_dropped);
}

#define NO_ACTION_RB 0
#define ADD_RB 1
#define DEL_RB 2

//---------------------------------------------------------------------------
int main(int argc,char **argv)
//---------------------------------------------------------------------------
{
  int                                      done;
  int                                      rc;
  int                                      meas_polling_counter;
  fd_set                                   readfds;
  struct timeval                           tv;
  int                                      i;
  int                                      err;
  char                                    *buffer;
  int                                      c;
  int                                      action;
  int                                      rbset            = 0;
  int                                      cxset            = 0;
  int                                      classrefset      = 0;
  int                                      instset          = 0;
  int                                      saddr_ipv4set    = 0;
  int                                      saddr_ipv6set    = 0;
  int                                      daddr_ipv4set    = 0;
  int                                      daddr_ipv6set    = 0;
  int                                      dscpset          = 0;
  int                                      dplen            = 0;
  int                                      splen            = 0;
  int                                      mpls_outlabelset = 0;
  int                                      mpls_inlabelset;
  char                                     rb[20];
  char                                     cx[20];
  char                                     classref[20];
  char                                     dscp[20];
  char                                     inst[20];
  char                                     mpls_outgoinglabel[100];
  char                                     mpls_incominglabel[100];
  int                                      index;
  struct oai_nw_drv_msg_rb_establishment_request *msgreq;
  struct oai_nw_drv_msg_class_add_request        *msgreq_class;
  in_addr_t                                saddr_ipv4;
  in_addr_t                                daddr_ipv4;
  struct in6_addr                          saddr_ipv6;
  struct in6_addr                          daddr_ipv6;
  unsigned int                             mpls_outlabel;
  unsigned int                             mpls_inlabel;
  char                                     addr_str[46];
  char                                     mask_len_delims[] = "/";
  char                                     *result;
  // scan options
  rb[0] = '\0';
  cx[0] = '\0';
  dscp[0] = '\0';
  mpls_incominglabel[0] = '\0';
  mpls_outgoinglabel[0] = '\0';
  action  = NO_ACTION_RB;

  while ((c = getopt (argc, argv, "adr:i:c:f:l:m:s:t:x:y:z:")) != -1)
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

      case 'f':
        strcpy(classref,optarg);
        classrefset = 1;
        break;

      case 'l':
        strcpy(mpls_outgoinglabel,optarg);
        mpls_outlabelset=1;
        break;

      case 'm':
        strcpy(mpls_incominglabel,optarg);
        mpls_inlabelset=1;
        break;

      case 's':
        result = strtok( optarg, mask_len_delims );

        if ( result != NULL ) {
          inet_aton(result,&saddr_ipv4);
          printf("Arg Source Addr IPv4 string: %s\n",result);
          saddr_ipv4set = 1;
        } else {
          printf("Arg Source Addr IPv4 string: ERROR\n");
          break;
        }

        result = strtok( NULL, mask_len_delims );

        if ( result != NULL ) {
          splen = atoi(result);
        } else {
          splen = 32;
        }

        printf("Arg Source Addr IPv4 mask len: %d\n",splen);
        break;

      case 't':
        result = strtok( optarg, mask_len_delims );

        if ( result != NULL ) {
          inet_aton(result,&daddr_ipv4);
          printf("Arg Dest Addr IPv4 string: %s\n",result);
          daddr_ipv4set = 1;
        } else {
          printf("Arg Dest Addr IPv4 string: ERROR\n");
          break;
        }

        result = strtok( NULL, mask_len_delims );

        if ( result != NULL ) {
          dplen = atoi(result);
        } else {
          dplen = 32;
        }

        printf("Arg Dest Addr IPv4 mask len: %d\n",dplen);
        break;

      case 'x':
        result = strtok( optarg, mask_len_delims );

        if ( result != NULL ) {
          printf("Arg Source Addr IPv6 string: %s\n",result);
          inet_pton(AF_INET6,result,(void *)&saddr_ipv6);
          saddr_ipv6set = 1;
        } else {
          printf("Arg Source Addr IPv6 string: ERROR\n");
          break;
        }

        result = strtok( NULL, mask_len_delims );

        if ( result != NULL ) {
          splen = atoi(result);
        } else {
          splen = 128;
        }

        printf("Arg Source Addr IPv6 mask len: %d\n",splen);
        break;

      case 'y':
        result = strtok( optarg, mask_len_delims );

        if ( result != NULL ) {
          printf("Arg Dest Addr IPv6 string: %s\n",result);
          inet_pton(AF_INET6,result,(void *)&daddr_ipv6);
          daddr_ipv6set = 1;
        } else {
          printf("Arg Dest Addr IPv6 string: ERROR\n");
          break;
        }

        result = strtok( NULL, mask_len_delims );

        if ( result != NULL ) {
          dplen = atoi(result);
        } else {
          dplen = 128;
        }

        printf("Arg Dest Addr IPv6 mask len: %d\n",dplen);
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

  if (classrefset==0) {
    printf("ERROR: Specify an Class reference !!\n");
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
  msgreq = (struct oai_nw_drv_msg_rb_establishment_request *)(gifr.msg);
  msgreq->rab_id = atoi(rb);
  msgreq->lcr = atoi(cx);
  msgreq->qos = 0;

  if (action == ADD_RB) {
    gifr.type =  OAI_NW_DRV_MSG_RB_ESTABLISHMENT_REQUEST;
    printf("OAI_NW_DRV_MSG_RB_ESTABLISHMENT_REQUEST: RB %d LCR %d QOS %d\n ", msgreq->rab_id, msgreq->lcr, msgreq->qos);
    err=ioctl(fd, OAI_NW_DRV_IOCTL_RRM, &gifr);
  }

  // only add classification rule
  if ((action == ADD_RB ) || (action == NO_ACTION_RB)) {
    if (saddr_ipv4set == 1) {
      msgreq_class             = (struct oai_nw_drv_msg_class_add_request *)(gifr.msg);
      msgreq_class->rab_id     = atoi(rb);
      msgreq_class->lcr        = atoi(cx);
      msgreq_class->version    = 4;
      msgreq_class->classref   = atoi(classref) + (msgreq_class->lcr<<8);
      msgreq_class->dir        = OAI_NW_DRV_DIRECTION_SEND;
      msgreq_class->fct        = OAI_NW_DRV_FCT_QOS_SEND;
      msgreq_class->saddr.ipv4 = saddr_ipv4;
      msgreq_class->daddr.ipv4 = daddr_ipv4;
      msgreq_class->dplen      = dplen;
      msgreq_class->splen      = splen;
      // TO BE FIXED WHEN WE CAN SPECIFY A PROTOCOL-based rule
      msgreq_class->protocol   = OAI_NW_DRV_PROTOCOL_DEFAULT;

      if (dscpset==0) {
        msgreq_class->dscp=0;
      } else {
        msgreq_class->dscp=atoi(dscp);
      }

      printf("OAI_NW_DRV_MSG_CLASS_ADD_REQUEST: OAI_NW_DRV_DIRECTION_SEND RB %d LCR %d ClassRef %d ", msgreq_class->rab_id, msgreq_class->lcr, msgreq_class->classref);
      printf("IPV4: Source  = %d.%d.%d.%d/%d ", NIPADDR(msgreq_class->saddr.ipv4), msgreq_class->splen);
      printf(" IPV4: Dest    = %d.%d.%d.%d/%d\n", NIPADDR(msgreq_class->daddr.ipv4), msgreq_class->dplen);
      gifr.type                = OAI_NW_DRV_MSG_CLASS_ADD_REQUEST;
      err=ioctl(fd, OAI_NW_DRV_IOCTL_RRM, &gifr);
      msgreq_class->rab_id     = atoi(rb);
      msgreq_class->lcr        = atoi(cx);
      msgreq_class->version    = 4;
      msgreq_class->classref   = atoi(classref) + 1 + (msgreq_class->lcr<<8);
      msgreq_class->dir        = OAI_NW_DRV_DIRECTION_RECEIVE;
      msgreq_class->fct        = OAI_NW_DRV_FCT_QOS_SEND;
      msgreq_class->daddr.ipv4 = saddr_ipv4;
      msgreq_class->saddr.ipv4 = daddr_ipv4;
      msgreq_class->dplen      = splen;
      msgreq_class->splen      = dplen;
      printf("OAI_NW_DRV_MSG_CLASS_ADD_REQUEST: OAI_NW_DRV_DIRECTION_RECEIVE RB %d LCR %d ClassRef %d ", msgreq_class->rab_id, msgreq_class->lcr, msgreq_class->classref);
      printf("IPV4: Source  = %d.%d.%d.%d/%d ", NIPADDR(msgreq_class->saddr.ipv4), msgreq_class->splen);
      printf("IPV4: Dest    = %d.%d.%d.%d/%d\n", NIPADDR(msgreq_class->daddr.ipv4), msgreq_class->dplen);
      gifr.type =  OAI_NW_DRV_MSG_CLASS_ADD_REQUEST;
      err=ioctl(fd, OAI_NW_DRV_IOCTL_RRM, &gifr);
    }

    if (saddr_ipv6set == 1) {
      msgreq_class             = (struct oai_nw_drv_msg_class_add_request *)(gifr.msg);
      msgreq_class->rab_id     = atoi(rb);
      msgreq_class->lcr        = atoi(cx);
      msgreq_class->version    = 6;
      msgreq_class->dplen      = dplen;
      msgreq_class->splen      = splen;

      if (dscpset==0)
        msgreq_class->dscp   = 0;
      else
        msgreq_class->dscp   = atoi(dscp);

      msgreq_class->classref   = atoi(classref) + (msgreq_class->lcr<<8);
      msgreq_class->dir        = OAI_NW_DRV_DIRECTION_SEND;
      msgreq_class->fct        = OAI_NW_DRV_FCT_QOS_SEND;
      // TO BE FIXED WHEN WE CAN SPECIFY A PROTOCOL-based rule
      msgreq_class->protocol   = OAI_NW_DRV_PROTOCOL_DEFAULT;
      memcpy(&msgreq_class->saddr.ipv6,&saddr_ipv6,16);
      memcpy(&msgreq_class->daddr.ipv6,&daddr_ipv6,16);
      printf("OAI_NW_DRV_MSG_CLASS_ADD_REQUEST: OAI_NW_DRV_DIRECTION_SEND RB %d LCR %d ClassRef %d ", msgreq_class->rab_id, msgreq_class->lcr, msgreq_class->classref);
      printf("IPV6: Source  = %x:%x:%x:%x:%x:%x:%x:%x/%d ", NIP6ADDR(&msgreq_class->saddr.ipv6), msgreq_class->splen);
      printf("IPV6: Dest    = %x:%x:%x:%x:%x:%x:%x:%x/%d\n", NIP6ADDR(&msgreq_class->daddr.ipv6), msgreq_class->dplen);
      gifr.type =  OAI_NW_DRV_MSG_CLASS_ADD_REQUEST;
      err=ioctl(fd, OAI_NW_DRV_IOCTL_RRM, &gifr);
      msgreq_class->rab_id     = atoi(rb);
      msgreq_class->lcr        = atoi(cx);
      msgreq_class->dplen      = splen;
      msgreq_class->splen      = dplen;
      msgreq_class->classref   = atoi(classref) + 1 + (msgreq_class->lcr<<8);
      msgreq_class->dir        = OAI_NW_DRV_DIRECTION_RECEIVE;
      memcpy(&msgreq_class->daddr.ipv6,&saddr_ipv6,16);
      memcpy(&msgreq_class->saddr.ipv6,&daddr_ipv6,16);
      printf("OAI_NW_DRV_MSG_CLASS_ADD_REQUEST: OAI_NW_DRV_DIRECTION_RECEIVE RB %d LCR %d ClassRef %d ", msgreq_class->rab_id, msgreq_class->lcr, msgreq_class->classref);
      printf("IPV6: Source  = %x:%x:%x:%x:%x:%x:%x:%x/%d ", NIP6ADDR(&msgreq_class->saddr.ipv6), msgreq_class->splen);
      printf("IPV6: Dest    = %x:%x:%x:%x:%x:%x:%x:%x/%d\n", NIP6ADDR(&msgreq_class->daddr.ipv6), msgreq_class->dplen);
      gifr.type                =  OAI_NW_DRV_MSG_CLASS_ADD_REQUEST;
      err=ioctl(fd, OAI_NW_DRV_IOCTL_RRM, &gifr);
    }

    if (mpls_inlabelset == 1) {
      msgreq_class = (struct oai_nw_drv_msg_class_add_request *)(gifr.msg);
      msgreq_class->rab_id = atoi(rb);
      msgreq_class->lcr = atoi(cx);
      msgreq_class->version = OAI_NW_DRV_MPLS_VERSION_CODE;

      if (dscpset==0)
        msgreq_class->dscp=0;
      else
        msgreq_class->dscp=atoi(dscp);

      msgreq_class->classref = atoi(classref) + (msgreq_class->lcr<<8);
      //msgreq_class->classref = 4 + (msgreq_class->lcr<<3);
      msgreq_class->dir=OAI_NW_DRV_DIRECTION_SEND;
      msgreq_class->fct=OAI_NW_DRV_FCT_QOS_SEND;
      // TO BE FIXED WHEN WE CAN SPECIFY A PROTOCOL-based rule
      msgreq_class->protocol = OAI_NW_DRV_PROTOCOL_DEFAULT;
      mpls_outlabel = atoi(mpls_outgoinglabel);
      printf("Setting MPLS outlabel %u with exp %d\n",mpls_outlabel,msgreq_class->dscp);
      msgreq_class->daddr.mpls_label = mpls_outlabel;
      gifr.type =  OAI_NW_DRV_MSG_CLASS_ADD_REQUEST;
      err=ioctl(fd, OAI_NW_DRV_IOCTL_RRM, &gifr);
      msgreq_class->rab_id = atoi(rb);
      msgreq_class->lcr = atoi(cx);
      msgreq_class->classref = atoi(classref) + 1 + (msgreq_class->lcr<<8);
      //msgreq_class->classref = 5 + (msgreq_class->lcr<<3);
      msgreq_class->dir=OAI_NW_DRV_DIRECTION_RECEIVE;
      // TO BE FIXED WHEN WE CAN SPECIFY A PROTOCOL-based rule
      msgreq_class->protocol = OAI_NW_DRV_PROTOCOL_DEFAULT;
      mpls_inlabel  = atoi(mpls_incominglabel);
      printf("Setting MPLS inlabel %u with exp %d\n",mpls_inlabel,msgreq_class->dscp);
      msgreq_class->daddr.mpls_label = mpls_inlabel;
      gifr.type =  OAI_NW_DRV_MSG_CLASS_ADD_REQUEST;
      err=ioctl(fd, OAI_NW_DRV_IOCTL_RRM, &gifr);
    }
  } else if (action == DEL_RB) {
    gifr.type =  OAI_NW_DRV_MSG_RB_RELEASE_REQUEST;
    err=ioctl(fd, OAI_NW_DRV_IOCTL_RRM, &gifr);
  }
}
