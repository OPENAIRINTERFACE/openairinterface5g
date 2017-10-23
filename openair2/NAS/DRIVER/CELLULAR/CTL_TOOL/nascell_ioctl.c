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

#include "nascell_ioctl.h"
#include "rrc_nas_primitives.h"
//#include "rrc_qos.h"
//#include "graal_local.h"
#include "nasmt_constant.h"
#include "nasmt_iocontrol.h"
#define USHRT_MAX 255
//---------------------------------------------------------------------------
void print_error(int status)
{
  //---------------------------------------------------------------------------
  switch(status) {
  case -NAS_ERROR_NOMEMORY:
    printf(" The memory is low\n");
    break;

  case -NAS_ERROR_NORB:
    printf(" The radio bearer is not identified\n");
    break;

  case -NAS_ERROR_NOTMT:
    printf(" The entity must be a mobile\n");
    break;

  case -NAS_ERROR_NOTRG:
    printf(" The entity must be an access router\n");
    break;

  case -NAS_ERROR_NOTIDLE:
    printf(" The selected connection or radio bearer must be IDLE\n");
    break;

  case -NAS_ERROR_NOTCONNECTED:
    printf(" The selected connection or radio bearer must be CONNECTED\n");
    break;

  case -NAS_ERROR_NOTCORRECTLCR:
    printf(" The LCR value is not correct\n");
    break;

  case -NAS_ERROR_NOTCORRECTDIR:
    printf(" The DIR value is not correct\n");
    break;

  case -NAS_ERROR_NOTCORRECTDSCP:
    printf(" The DSCP value is not correct\n");
    break;

  case -NAS_ERROR_NOTCORRECTVERSION:
    printf(" The version value is not correct\n");
    break;

  case -NAS_ERROR_NOTCORRECTRABI:
    printf(" The rab_id value is not correct\n");
    break;

  case -NAS_ERROR_ALREADYEXIST:
    printf(" Some components must not be duplicated\n");
    break;

  default:
    printf(" Unknown error cause %d\n", status);
  }
}

//---------------------------------------------------------------------------
void print_state(uint8_t state)
{
  //---------------------------------------------------------------------------
  switch(state) {
  case NAS_IDLE:
    printf("NAS_IDLE\n");
    return;

    //  case NAS_STATE_CONNECTED:printf("NAS_STATE_CONNECTED\n");return;
    //  case NAS_STATE_ESTABLISHMENT_REQUEST:printf("NAS_STATE_ESTABLISHMENT_REQUEST\n");return;
    //  case NAS_STATE_ESTABLISHMENT_FAILURE:printf("NAS_STATE_ESTABLISHMENT_FAILURE\n");return;
    //  case NAS_STATE_RELEASE_FAILURE:printf("NAS_STATE_RELEASE_FAILURE\n");return;
  case  NAS_CX_FACH:
    printf("NAS_CX_FACH\n");
    return;

  case  NAS_CX_DCH:
    printf("NAS_CX_DCH\n");
    return;

  case  NAS_CX_RECEIVED:
    printf("NAS_CX_RECEIVED\n");
    return;

  case  NAS_CX_CONNECTING:
    printf("NAS_CX_CONNECTING\n");
    return;

  case  NAS_CX_RELEASING:
    printf("NAS_CX_RELEASING\n");
    return;

  case  NAS_CX_CONNECTING_FAILURE:
    printf("NAS_CX_CONNECTING_FAILURE\n");
    return;

  case  NAS_CX_RELEASING_FAILURE:
    printf("NAS_CX_RELEASING_FAILURE\n");
    return;

  case  NAS_RB_ESTABLISHING:
    printf("NAS_RB_ESTABLISHING\n");
    return;

  case  NAS_RB_RELEASING:
    printf("NAS_RB_RELEASING\n");
    return;

  default:
    printf(" Unknown state\n");
  }
}

//---------------------------------------------------------------------------
void print_usage_class()
{
  //---------------------------------------------------------------------------
  printf("Usage: gioctl class add {send <lcr>|receive} -f {qos <rab_id>|del|ctl|dc} -cr <classref> [-dscp <dscp>] [-ip {6|4} <saddr> <splen> <daddr> <dplen>] [-p {tcp|udp|icmp4|icmp6} <sport> <dport>]\n");
  printf("Usage: gioctl class del {send <lcr>|receive} -cr <classref> [-dscp <dscp>] \n");
  printf("Usage: gioctl class list {send <lcr>|receive} [-dscp <dscp>]\n");
}
//---------------------------------------------------------------------------
void print_usage_cx()
{
  //---------------------------------------------------------------------------
  printf("Usage: gioctl cx add <lcr> <cellid>\n");
  printf("Usage: gioctl cx del <lcr>\n");
  printf("Usage: gioctl cx list\n");
}
//---------------------------------------------------------------------------
void print_usage_rb()
{
  //---------------------------------------------------------------------------
  printf("Usage: gioctl rb add <lcr> <rab_id> <qos>\n");
  printf("Usage: gioctl rb list <lcr>\n");
}

//---------------------------------------------------------------------------
int main(int argc, char *argv[])
{
  //---------------------------------------------------------------------------
  struct nas_ioctl gifr;
  int err, fd;

  // Invalid number of arguments
  if (argc<2) {
    printf("Usage: gioctl statistic\n");
    print_usage_cx();
    print_usage_rb();
    print_usage_class();
    return  -1;
  }

  // Get an UDP IPv6 socket ??
  fd=socket(AF_INET6, SOCK_DGRAM, 0);

  if (fd<0)  {
    printf("Error opening socket\n");
    return 0;
  }

  strcpy(gifr.name, "oai0");
  printf("Socket opened successfully\n");

  /***************************/
  if (strcmp(argv[1], "statistic")==0) {
    struct nas_msg_statistic_reply *msgrep;
    gifr.type=NAS_MSG_STATISTIC_REQUEST;
    gifr.msg=(char *)malloc(sizeof(msgrep));
    msgrep=(struct nas_msg_statistic_reply *)(gifr.msg);
    printf(" Statistics requested\n");
    err=ioctl(fd, NASMT_IOCTL_RAL, &gifr);

    if (err<0) {
      printf("IOCTL error, err=%d\n",err);
      return -1;
    }

    printf("tx_packets = %u, rx_packets = %u\n", msgrep->tx_packets, msgrep->rx_packets);
    printf("tx_bytes = %u, rx_bytes = %u\n", msgrep->tx_bytes, msgrep->rx_bytes);
    printf("tx_errors = %u, rx_errors = %u\n", msgrep->tx_errors, msgrep->rx_errors);
    printf("tx_dropped = %u, rx_dropped = %u\n", msgrep->tx_dropped, msgrep->rx_dropped);
    return 0;
  }

  /***************************/
  if (strcmp(argv[1], "cx")==0) {
    if (argc<3) {
      print_usage_cx();
      return -1;
    }

    /***/
    //  printf("Usage: gioctl cx add <lcr> <cellid>\n");
    if (strcmp(argv[2], "add")==0) {
      struct nas_msg_cx_establishment_request *msgreq;
      struct nas_msg_cx_establishment_reply *msgrep;

      if (argc<5) {
        print_usage_cx();
        return  -1;
      }

      gifr.type=NAS_MSG_CX_ESTABLISHMENT_REQUEST;
      gifr.msg=(char *)malloc(sizeof(msgrep)<sizeof(msgreq)?sizeof(msgreq):sizeof(msgrep));
      msgreq=(struct nas_msg_cx_establishment_request *)(gifr.msg);
      msgrep=(struct nas_msg_cx_establishment_reply *)(gifr.msg);
      msgreq->lcr=strtoul(argv[3], NULL, 0);
      msgreq->cellid=strtoul(argv[4], NULL, 0);
      //
      printf(" Connection establishment requested\n");
      err=ioctl(fd, NASMT_IOCTL_RAL, &gifr);

      if (err<0) {
        printf("IOCTL error, err=%d\n",err);
        perror("Connection establishment IOCTL error\n");
        return -1;
      }

      if (msgrep->status<0) {
        printf(" Connexion establishment failure: ");
        print_error(msgrep->status);
        return -1;
      }

      return 0;
    }

    /***/
    //  printf("Usage: gioctl cx del <lcr>\n");
    if (strcmp(argv[2], "del")==0) {
      struct nas_msg_cx_release_request *msgreq;
      struct nas_msg_cx_release_reply *msgrep;

      if (argc<4) {
        print_usage_cx();
        return  -1;
      }

      gifr.type=NAS_MSG_CX_RELEASE_REQUEST;
      gifr.msg=(char *)malloc(sizeof(msgrep)<sizeof(msgreq)?sizeof(msgreq):sizeof(msgrep));
      msgreq=(struct nas_msg_cx_release_request *)(gifr.msg);
      msgrep=(struct nas_msg_cx_release_reply *)(gifr.msg);
      msgreq->lcr=strtoul(argv[3], NULL, 0);
      //
      printf(" Connexion release requested\n");
      err=ioctl(fd, NASMT_IOCTL_RAL, &gifr);

      if (err<0) {
        printf("IOCTL error, err=%d\n",err);
        return -1;
      }

      if (msgrep->status<0) {
        printf(" Connexion release failure: ");
        print_error(msgrep->status);
        return -1;
      }

      return 0;
    }

    /***/
    //  printf("Usage: gioctl cx list\n");
    if (strcmp(argv[2], "list")==0) {
      uint8_t *msgrep;
      uint8_t i;
      struct nas_msg_cx_list_reply *list;
      uint8_t lcr;
      gifr.type=NAS_MSG_CX_LIST_REQUEST;
      gifr.msg=(char *)malloc(NAS_LIST_CX_MAX*sizeof(struct nas_msg_cx_list_reply)+1);
      msgrep=(uint8_t *)(gifr.msg);
      //
      printf(" Connexion list requested\n");
      err=ioctl(fd, NASMT_IOCTL_RAL, &gifr);

      if (err<0) {
        printf("IOCTL error, err=%d\n",err);
        return -1;
      }

      printf("Lcr\t\tCellId\tIID4\tIID6\t\t\tnrb\tnsclass\tState\n");
      list=(struct nas_msg_cx_list_reply *)(msgrep+1);

      for(lcr=0; lcr<msgrep[0]; ++lcr) {
        printf("%u\t\t%u\t%u\t", list[lcr].lcr, list[lcr].cellid, list[lcr].iid4);

        for (i=0; i<8; ++i)
          printf("%02x", *((uint8_t *)list[lcr].iid6+i));

        printf("\t%u\t%u\t", list[lcr].num_rb, list[lcr].nsclassifier);
        print_state(list[lcr].state);
      }

      return 0;
    }

    print_usage_cx();
    return -1;
  }

  /***************************/
  if (strcmp(argv[1], "rb")==0) {
    if (argc<3) {
      print_usage_rb();
      return -1;
    }

    /***/
    //  printf("Usage: gioctl rb add <lcr> <rab_id> <qos>\n");
    if (strcmp(argv[2], "add")==0) {
      struct nas_msg_rb_establishment_request *msgreq;
      struct nas_msg_rb_establishment_reply *msgrep;

      if (argc<6) {
        print_usage_rb();
        return  -1;
      }

      gifr.type=NAS_MSG_RB_ESTABLISHMENT_REQUEST;
      gifr.msg=(char *)malloc(sizeof(msgrep)<sizeof(msgreq)?sizeof(msgreq):sizeof(msgrep));
      msgreq=(struct nas_msg_rb_establishment_request *)(gifr.msg);
      msgrep=(struct nas_msg_rb_establishment_reply *)(gifr.msg);
      msgreq->lcr=strtoul(argv[3], NULL, 0);
      msgreq->rab_id=strtoul(argv[4], NULL, 0);
      msgreq->qos=strtoul(argv[5], NULL, 0);
      //
      printf(" Radio bearer establishment requested\n");
      err=ioctl(fd, NASMT_IOCTL_RAL, &gifr);

      if (err<0) {
        printf("IOCTL error, err=%d\n",err);
        return -1;
      }

      if (msgrep->status<0) {
        printf(" Radio bearer establishment failure: ");
        print_error(msgrep->status);
        return -1;
      }

      return 0;
    }

    /***/
    if (strcmp(argv[2], "del")==0) {
      struct nas_msg_rb_release_request *msgreq;
      struct nas_msg_rb_release_reply *msgrep;

      if (argc<5) {
        print_usage_rb();
        return  -1;
      }

      gifr.type=NAS_MSG_RB_RELEASE_REQUEST;
      gifr.msg=(char *)malloc(sizeof(msgrep)<sizeof(msgreq)?sizeof(msgreq):sizeof(msgrep));
      msgreq=(struct nas_msg_rb_release_request *)(gifr.msg);
      msgrep=(struct nas_msg_rb_release_reply *)(gifr.msg);
      msgreq->lcr=strtoul(argv[3], NULL, 0);
      msgreq->rab_id=strtoul(argv[4], NULL, 0);
      //
      printf(" Radio Bearer release requested\n");
      err=ioctl(fd, NASMT_IOCTL_RAL, &gifr);

      if (err<0) {
        printf("IOCTL error, err=%d\n",err);
        return -1;
      }

      if (msgrep->status<0) {
        printf(" Radio bearer release failure: ");
        print_error(msgrep->status);
        return -1;
      }

      return 0;
    }

    /***/
    //  printf("Usage: gioctl rb list <lcr>\n");
    if (strcmp(argv[2], "list")==0) {
      uint8_t *msgrep;
      uint8_t rbi;
      struct nas_msg_rb_list_reply *list;
      struct nas_msg_rb_list_request *msgreq;

      if (argc<4) {
        print_usage_rb();
        return  -1;
      }

      gifr.type=NAS_MSG_RB_LIST_REQUEST;
      gifr.msg=(char *)malloc(NAS_LIST_RB_MAX*sizeof(struct nas_msg_rb_list_reply)+1);
      msgreq=(struct nas_msg_rb_list_request *)(gifr.msg);
      msgrep=(uint8_t *)(gifr.msg);
      msgreq->lcr=strtoul(argv[3], NULL, 0);
      //
      printf(" Radio bearer list requested\n");
      err=ioctl(fd, NASMT_IOCTL_RAL, &gifr);

      if (err<0) {
        printf("IOCTL error, err=%d\n",err);
        return -1;
      }

      printf("rab_id\t\tSapi\t\tQoS\t\tState\n");
      list=(struct nas_msg_rb_list_reply *)(msgrep+1);

      for(rbi=0; rbi<msgrep[0]; ++rbi) {
        printf("%u\t\t%u\t\t%u\t\t", list[rbi].rab_id, list[rbi].sapi, list[rbi].qos);
        print_state(list[rbi].state);
      }

      return 0;
    }

    print_usage_rb();
    return -1;
  }

  /***************************/
  if (strcmp(argv[1], "class")==0) {
    if (argc<3) {
      print_usage_class();
      return  -1;
    }

    /***/
    if (strcmp(argv[2], "add")==0) {
      struct nas_msg_class_add_request *msgreq;
      struct nas_msg_class_add_reply *msgrep;
      uint8_t i,j;
      gifr.type=NAS_MSG_CLASS_ADD_REQUEST;
      gifr.msg=(char *)malloc(sizeof(msgrep)<sizeof(msgreq)?sizeof(msgreq):sizeof(msgrep));
      msgreq=(struct nas_msg_class_add_request *)(gifr.msg);
      msgrep=(struct nas_msg_class_add_reply *)(gifr.msg);
      i=3;

      if (strcmp(argv[i], "send")==0) {
        msgreq->dir=NAS_DIRECTION_SEND;
        msgreq->lcr=strtoul(argv[++i], NULL, 0);
        ++i;
      } else {
        if (strcmp(argv[i], "receive")==0) {
          msgreq->dir=NAS_DIRECTION_RECEIVE;
          ++i;
        } else {
          print_usage_class();
          return -1;
        }
      }

      msgreq->classref=USHRT_MAX;
      msgreq->dscp=NAS_DSCP_DEFAULT;
      msgreq->version=NAS_VERSION_DEFAULT;
      msgreq->protocol=NAS_PROTOCOL_DEFAULT;
      msgreq->sport=NAS_PORT_DEFAULT;
      msgreq->dport=NAS_PORT_DEFAULT;
      msgreq->fct=0;

      while (i<argc) {
        if (strcmp(argv[i], "-cr")==0) {
          msgreq->classref=strtoul(argv[++i], NULL, 0);
          ++i;
          continue;
        }

        if (strcmp(argv[i], "-dscp")==0) {
          msgreq->dscp=strtoul(argv[++i], NULL, 0);
          ++i;
          continue;
        }

        if (strcmp(argv[i], "-f")==0) {
          ++i;

          if (strcmp(argv[i], "qos")==0) {
            msgreq->fct=NAS_FCT_QOS_SEND;
            msgreq->rab_id=strtoul(argv[++i], NULL, 0);
            ++i;
            continue;
          }

          if (strcmp(argv[i], "del")==0) {
            msgreq->fct=NAS_FCT_DEL_SEND;
            msgreq->rab_id=0;
            ++i;
            continue;
          }

          if (strcmp(argv[i], "ctl")==0) {
            msgreq->fct=NAS_FCT_CTL_SEND;
            msgreq->rab_id=0;
            ++i;
            continue;
          }

          if (strcmp(argv[i], "dc")==0) {
            msgreq->fct=NAS_FCT_DC_SEND;
            msgreq->rab_id=0;
            ++i;
            continue;
          }

          print_usage_class();
          return -1;
        }

        if (strcmp(argv[i], "-ip")==0) {
          ++i;

          if (strcmp(argv[i], "4")==0) {
            if (msgreq->version!=NAS_VERSION_DEFAULT) {
              print_usage_class();
              return -1;
            }

            msgreq->version=NAS_VERSION_4;
            inet_pton(AF_INET, argv[++i], (void *)(&msgreq->saddr));
            msgreq->splen=strtoul(argv[++i], NULL, 0);
            inet_pton(AF_INET, argv[++i], (void *)(&msgreq->daddr));
            msgreq->dplen=strtoul(argv[++i], NULL, 0);
            ++i;
            continue;
          }

          if (strcmp(argv[i], "6")==0) {
            if (msgreq->version!=NAS_VERSION_DEFAULT) {
              print_usage_class();
              return -1;
            }

            msgreq->version=NAS_VERSION_6;
            inet_pton(AF_INET6, argv[++i], (void *)(&msgreq->saddr));
            msgreq->splen=strtoul(argv[++i], NULL, 0);
            inet_pton(AF_INET6, argv[++i], (void *)(&msgreq->daddr));
            msgreq->dplen=strtoul(argv[++i], NULL, 0);
            ++i;
            continue;
          }

          print_usage_class();
          return -1;
        }

        if (strcmp(argv[i], "-p")==0) {
          ++i;

          if (strcmp(argv[i], "tcp")==0) {
            if (msgreq->protocol!=NAS_PROTOCOL_DEFAULT) {
              print_usage_class();
              return -1;
            }

            msgreq->protocol=NAS_PROTOCOL_TCP;
            msgreq->sport=strtoul(argv[++i], NULL, 0);
            msgreq->dport=strtoul(argv[++i], NULL, 0);
            ++i;
            continue;
          }

          if (strcmp(argv[i], "udp")==0) {
            if (msgreq->protocol!=NAS_PROTOCOL_DEFAULT) {
              print_usage_class();
              return -1;
            }

            msgreq->protocol=NAS_PROTOCOL_UDP;
            msgreq->sport=strtoul(argv[++i], NULL, 0);
            msgreq->dport=strtoul(argv[++i], NULL, 0);
            ++i;
            continue;
          }

          if (strcmp(argv[i], "icmp4")==0) {
            if (msgreq->protocol!=NAS_PROTOCOL_DEFAULT) {
              print_usage_class();
              return -1;
            }

            msgreq->protocol=NAS_PROTOCOL_ICMP4;
            msgreq->sport=strtoul(argv[++i], NULL, 0);
            msgreq->dport=strtoul(argv[++i], NULL, 0);
            ++i;
            continue;
          }

          if (strcmp(argv[i], "icmp6")==0) {
            if (msgreq->protocol!=NAS_PROTOCOL_DEFAULT) {
              print_usage_class();
              return -1;
            }

            msgreq->protocol=NAS_PROTOCOL_ICMP6;
            msgreq->sport=strtoul(argv[++i], NULL, 0);
            msgreq->dport=strtoul(argv[++i], NULL, 0);
            ++i;
            continue;
          }

          print_usage_class();
          return -1;
        }

        print_usage_class();
        return -1;
      }

      if ((msgreq->classref==USHRT_MAX)||(msgreq->fct==0)) {
        print_usage_class();
        return -1;
      }

      printf(" Class add requested\n");
      err=ioctl(fd, NASMT_IOCTL_RAL, &gifr);

      if (err<0) {
        printf("IOCTL error, err=%d\n",err);
        return -1;
      }

      if (msgrep->status<0) {
        printf(" Class add failure: ");
        print_error(msgrep->status);
        return -1;
      }

      return 0;
    }

    /***/
    if (strcmp(argv[2], "del")==0) {
      uint8_t i;
      struct nas_msg_class_del_request *msgreq;
      struct nas_msg_class_del_reply *msgrep;
      gifr.type=NAS_MSG_CLASS_DEL_REQUEST;
      gifr.msg=(char *)malloc(sizeof(msgrep)<sizeof(msgreq)?sizeof(msgreq):sizeof(msgrep));
      msgreq=(struct nas_msg_class_del_request *)(gifr.msg);
      msgrep=(struct nas_msg_class_del_reply *)(gifr.msg);
      i=3;

      if (strcmp(argv[i], "send")==0) {
        msgreq->dir=NAS_DIRECTION_SEND;
        msgreq->lcr=strtoul(argv[++i], NULL, 0);
        ++i;
      } else {
        if (strcmp(argv[i], "receive")==0) {
          msgreq->dir=NAS_DIRECTION_RECEIVE;
          ++i;
        } else {
          print_usage_class();
          return -1;
        }
      }

      msgreq->classref=USHRT_MAX;
      msgreq->dscp=NAS_DSCP_DEFAULT;

      while (i<argc) {
        if (strcmp(argv[i], "-cr")==0) {
          msgreq->classref=strtoul(argv[++i], NULL, 0);
          ++i;
          continue;
        }

        if (strcmp(argv[i], "-dscp")==0) {
          msgreq->dscp=strtoul(argv[++i], NULL, 0);
          ++i;
          continue;
        }

        print_usage_class();
        return -1;
      }

      if (msgreq->classref==USHRT_MAX) {
        print_usage_class();
        return -1;
      }

      printf(" Class del requested\n");
      err=ioctl(fd, NASMT_IOCTL_RAL, &gifr);

      if (err<0) {
        printf("IOCTL error, err=%d\n",err);
        return -1;
      }

      if (msgrep->status<0) {
        printf(" Class add failure: ");
        print_error(msgrep->status);
        return -1;
      }

      return 0;
    }

    /***/
    if (strcmp(argv[2], "list")==0) {
      uint8_t *msgrep;
      uint8_t cli, i;
      struct nas_msg_class_list_reply *list;
      struct nas_msg_class_list_request *msgreq;
      char addr[45];

      if (argc<4) {
        print_usage_class();
        return  -1;
      }

      gifr.type=NAS_MSG_CLASS_LIST_REQUEST;
      gifr.msg=(char *)malloc(NAS_LIST_CLASS_MAX*sizeof(struct nas_msg_class_list_reply)+1);

      if (gifr.msg==NULL) {
        printf(" No memory\n");
        return -1;
      }

      msgreq=(struct nas_msg_class_list_request *)(gifr.msg);
      msgrep=(uint8_t *)(gifr.msg);
      i=3;

      if (strcmp(argv[i], "send")==0) {
        msgreq->dir=NAS_DIRECTION_SEND;
        msgreq->lcr=strtoul(argv[++i], NULL, 0);
        ++i;
      } else {
        if (strcmp(argv[i], "receive")==0) {
          msgreq->dir=NAS_DIRECTION_RECEIVE;
          msgreq->lcr=0;
          ++i;
        } else {
          print_usage_class();
          return  -1;
        }
      }

      msgreq->dscp=NAS_DSCP_DEFAULT;

      while (i<argc) {
        if (strcmp(argv[i], "-dscp")==0)
          msgreq->dscp=strtoul(argv[++i], NULL, 0);
      }

      printf(" Classifier list requested\n");
      err=ioctl(fd, NASMT_IOCTL_RAL, &gifr);

      //
      if (err<0) {
        printf("IOCTL error, err=%d\n",err);
        return -1;
      }

      printf("Lcr\trab_id\tCref\tFct\t(Vers., saddr, daddr)\t\t\t\t(Proto., Sport, Dport)\n");
      list=(struct nas_msg_class_list_reply *)(msgrep+1);

      for(cli=0; cli<msgrep[0]; ++cli) {
        printf("%u\t%u\t%u\t", list[cli].lcr, list[cli].rab_id, list[cli].classref);

        switch(list[cli].fct) {
        case NAS_FCT_QOS_SEND:
          printf("qos\t");
          break;

        case NAS_FCT_DEL_SEND:
          printf("del\t");
          break;

        case NAS_FCT_CTL_SEND:
          printf("ctl\t");
          break;

        case NAS_FCT_DC_SEND:
          printf("dc\t");
          break;

        default:
          printf("?\t");
        }

        switch(list[cli].version) {
        case 4:
          inet_ntop(AF_INET, (void *)&list[cli].saddr, addr, 45);
          printf("(ip4, %s/%u,", addr, list[cli].splen);
          inet_ntop(AF_INET, (void *)&list[cli].daddr, addr, 45);
          printf("%s/%u\t", addr, list[cli].dplen);
          break;

        case 6:
          inet_ntop(AF_INET6, (void *)&list[cli].saddr, addr, 45);
          printf("(ip6, %s/%u", addr, list[cli].splen);
          inet_ntop(AF_INET6, (void *)&list[cli].daddr, addr, 45);
          printf("%s/%u\t", addr, list[cli].dplen);
          break;

        default:
          printf("(--, --, --)\t\t\t");
          break;
        }

        switch(list[cli].protocol) {
        case IPPROTO_UDP:
          printf("(udp, %u, %u)\t\n", list[cli].sport, list[cli].dport);
          break;

        case IPPROTO_TCP:
          printf("(tcp, %u, %u)\t\n", list[cli].sport, list[cli].dport);
          break;

        case IPPROTO_ICMP:
          printf("(icmp4, %u, %u)\t\n", list[cli].sport, list[cli].dport);
          break;

        case IPPROTO_ICMPV6:
          printf("(icmp6, %u, %u)\t\n", list[cli].sport, list[cli].dport);
          break;

        default:
          printf("(--, --, --)\t\n");
        }
      }

      return 0;
    }

    return 0;
  }

  printf(" Unknown command %s\n",argv[1]);
  return 0;
}

