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

/*! \file otg_tx_socket.c
* \brief function containing the OTG TX traffic generation functions with sockets
* \author A. Hafsaoui
* \date 2011
* \version 0.1
* \company Eurecom
* \email: openair_tech@eurecom.fr
* \note
* \warning
*/

#include"otg_tx_socket.h"
#include "otg_vars.h"


#ifdef WIN32 /* si vous êtes sous Windows */

#include <winsock2.h>

#elif defined (linux) /* si vous êtes sous Linux */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h> /* close */
#include <netdb.h> /* gethostbyname */
#include <errno.h>

#define INVALID_SOCKET -1
#define SOCKET_ERROR -1
#define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;
#else /* sinon vous êtes sur une plateforme non supportée */

#error not defined for this platform

#endif


control_hdr_t *control_hdr;




void socket_packet_send(int src, int dst, int state,int ctime)
{

  init_control_header();

  LOG_I(OTG,"SOCKET:: IP version %d, Transport Protocol %d \n", g_otg->ip_v[src], g_otg->trans_proto[src]);

  if ((g_otg->ip_v[src]==1) && (g_otg->trans_proto[src]==2))
    client_socket_tcp_ip4(src, dst, state, ctime);

  if ((g_otg->ip_v[src]==2) && (g_otg->trans_proto[src]==2))
    client_socket_tcp_ip6(src, dst, state, ctime);

  if ((g_otg->ip_v[src]==1) && (g_otg->trans_proto[src]==1))
    client_socket_udp_ip4(src, dst, state, ctime);

  if ((g_otg->ip_v[src]==2) && (g_otg->trans_proto[src]==1))
    client_socket_udp_ip6(src, dst, state, ctime);



}




void client_socket_tcp_ip4(int src, int dst, int state, int ctime)
{

#define PORT 7777


  LOG_I(OTG,"SOCKET:: TCP-IP4 :: src= %d , dst= %d , state= %d \n", src, dst, state);

#if defined (WIN32)
  WSADATA WSAData;
  int erreur = WSAStartup(MAKEWORD(2,2), &WSAData);
#else
  int erreur = 0;
#endif

  SOCKET sock;
  SOCKADDR_IN sin;
  int sock_err;
  double idt;
  int size;

  /*  If we are in the case of Windows sockets*/
  if(!erreur) {
    /* Create socket */
    sock = socket(AF_INET, SOCK_STREAM, 0);

    /* Configure the connection */
    sin.sin_addr.s_addr = inet_addr(g_otg->dst_ip[src]);
    sin.sin_family = AF_INET;
    sin.sin_port = htons(PORT);

    /* connection is ok */

    printf("SOCKET:: TCP-IP4 :: \n");

    if(connect(sock, (SOCKADDR*)&sin, sizeof(sin)) != SOCKET_ERROR) {
      LOG_I(OTG,"SOCKET:: TCP-IP4 :: Create socket %s with dst port %d\n", inet_ntoa(sin.sin_addr), htons(sin.sin_port));
      ctime=0;
      LOG_I(OTG,"SOCKET:: TCP-IP4 :: ctime=%d, duration=%d \n", ctime, g_otg->duration[src]);

      init_control_header();


      do {
        //payload=NULL;
        //payload=packet_gen_socket(src, dst, state, ctime);
        //payload="CCCCC";

        char *payload_rest;
        payload_rest=packet_gen_socket(src, dst, state, ctime);

        if (payload_rest!=NULL) {



          payload_t *payload;
          payload= malloc(sizeof(payload_t));
          // Data serialization
          char *tx_buffer;
          tx_buffer= (char *)malloc(PAYLOAD_MAX);

          payload->control_hdr=otg_info_hdr_gen(src, dst, TCP, IPV4);

          payload->payload_rest=payload_rest;
          memcpy(tx_buffer, payload->control_hdr, sizeof (control_hdr_t));
          memcpy(tx_buffer+ sizeof (control_hdr_t), payload->payload_rest, strlen(payload_rest));


          int total_size=sizeof(control_hdr_t) + strlen(payload_rest);


          if((sock_err = send(sock, tx_buffer, total_size, 0)) != SOCKET_ERROR)
            LOG_I(OTG,"SOCKET:: TCP-IP4 :: Payload to send size :: %d \n",sock_err);
          else
            LOG_I(OTG,"SOCKET:: TCP-IP4 :: Transmission Error\n");

        }

        ctime+=1;


      } while (ctime<=g_otg->duration[src]) ;


    }
    /* connection is not possible..." */
    else
      LOG_I(OTG,"SOCKET:: TCP-IP4 :: connection is not possible to connect \n");


    /* close the socket */
    closesocket(sock);

  }
}



void client_socket_udp_ip4(int src, int dst, int state,int ctime)
{

  char *payload_rest;
  signed int udp_send;



  int sockfd, ok, addr_in_size;
  u_short portnum = 12345;
  struct sockaddr_in *to;
  struct hostent *toinfo;
  char *htoname = "127.0.0.1";
  u_long toaddr;

  LOG_I(OTG,"SOCKET:: UDP-IP4 :: src= %d , dst= %d , state= %d \n", src, dst, state);
  to = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in));

  if((toinfo = gethostbyname(htoname)) == NULL) {
    LOG_W(OTG,"SOCKET:: UDP-IP4 :: Error %d in gethostbyname: %s\n", errno,sys_errlist[errno]);
    exit(errno);
  };

  toaddr = *(u_long *)toinfo->h_addr_list[0];

  addr_in_size = sizeof(struct sockaddr_in);

  memset((char *)to,(char)0,addr_in_size);

  to->sin_family = AF_INET;

  to->sin_addr.s_addr = toaddr;

  to->sin_port = portnum;


  //
  /*

    //set otg header to 0
    control_hdr = calloc(1, sizeof(control_hdr_t));
    if (control_hdr == NULL)
      // Memory could not be allocated
        printf("Couldn't allocate memory\n");
    memset(control_hdr, 0, sizeof(control_hdr_t));
  */
  //


  if((sockfd = socket (PF_INET, SOCK_DGRAM, 0)) == -1) {
    LOG_W(OTG,"SOCKET:: UDP-IP4 :: Error %d in socket: %s\n",errno,sys_errlist[errno]);
    exit(errno);
  };

  do {
    payload_rest=packet_gen_socket(src, dst, state, ctime);

    if (payload_rest!=NULL) {

      payload_t *payload;
      payload= malloc(sizeof(payload_t));
      // Data serialization
      char *tx_buffer;
      tx_buffer= (char *)malloc(PAYLOAD_MAX);


      payload->control_hdr=otg_info_hdr_gen(src, dst, UDP, IPV4);
      payload->payload_rest=payload_rest;
      memcpy(tx_buffer, payload->control_hdr, sizeof (control_hdr_t));
      memcpy(tx_buffer+ sizeof (control_hdr_t), payload->payload_rest, strlen(payload_rest));
      int total_size=sizeof(control_hdr_t) + strlen(payload_rest);


      udp_send=sendto(sockfd, tx_buffer,total_size ,0,(struct sockaddr *)to,addr_in_size);
      LOG_I(OTG,"SOCKET:: UDP-IP4 :: Payload to send :: data sent:%d \n", udp_send);
      // Update TX OTG info
      //otg_info->rx_num_pkt[src][dst]+=1;
      //otg_info->rx_num_bytes[src][dst]+=  udp_send + (HDR_IP_v4 + HDR_UDP);
      //

      if (NULL != payload) {
        payload=NULL;
        free(payload);
      }

      if (NULL != tx_buffer) {
        tx_buffer=NULL;
        free(tx_buffer);
      }



    }

    if(udp_send == -1) {
      LOG_I(OTG,"SOCKET:: UDP-IP4 :: Transmission Error\n");
      exit(errno);
    }

    else
      LOG_I(OTG,"SOCKET:: UDP-IP4 :: No data to transmit\n");

    ctime+=1;


  } while (ctime<=g_otg->duration[src]) ;

  closesocket(sockfd);

}


void client_socket_tcp_ip6(int src, int dst, int state,int ctime)
{
  printf("client TCP IPv6\n");

}

void client_socket_udp_ip6(int src, int dst, int state,int ctime)
{
  printf("client UDP IPv6\n");

}




char* packet_gen_socket(int src, int dst, int state, int ctime)
{

  int size;
  char *payload=NULL;


  set_ctime(ctime);
  LOG_I(OTG,"SOCKET :: num_nodes_tx:: %d , seed:: %d \n", g_otg->num_nodes, g_otg->seed);

  LOG_I(OTG,"SOCKET :: NODE_INFO (Source= %d, Destination= %d,State= %d) ctime %d \n", src, dst, state, otg_info->ctime);


  LOG_I(OTG,"SOCKET :: INFO_SIM (src=%d, dst=%d, state=%d) application=%d, idt dist =%d, pkts dist= %d\n", src, dst, state, g_otg->application_type[src][dst], g_otg->idt_dist[src][dst][state],
        g_otg->size_dist[src][dst][state]);

  LOG_I(OTG,"SOCKET :: Transmission info: idt=%d, simulation time=%d \n", otg_info->idt[src][dst], ctime);

  // do not generate packet for this pair of src, dst : no app type and/or idt are defined
  if ((g_otg->application_type[src][dst] == 0) && (g_otg->idt_dist[src][dst][0] == 0)) {
    LOG_I(OTG,"SOCKET :: Do not generate packet for this pair of src=%d, dst =%d: no app type and/or idt are defined\n", src, dst);
    return 0;
  }

  //pre-config for the standalone
  if (ctime<otg_info->ptime[src][dst][state]) //it happends when the emulation was finished
    otg_info->ptime[src][dst][state]=ctime;

  if (ctime==0)
    otg_info->idt[src][dst]=0; //for the standalone mode: the emulation is run several times, we need to initialise the idt to 0 when ctime=0

  //end pre-config

  if ((otg_info->idt[src][dst]==(ctime-otg_info->ptime[src][dst][state])) || (otg_info->idt[src][dst]==0)) {

    LOG_I(OTG,"SOCKET :: Time To Transmit (Source= %d, Destination= %d,State= %d) , (IDT= %d ,simu time= %d, previous packet time= %d) \n", src, dst, state ,otg_info->idt[src][dst], ctime,
          otg_info->ptime[src][dst][state]);
    otg_info->ptime[src][dst][state]=ctime;
    otg_info->idt[src][dst]=time_dist(src, dst, state); // update the idt for the next otg_tx
  } else {
    LOG_I(OTG,"SOCKET :: It is not the time to transmit (ctime= %d, previous time=%d, packet idt=%d),  node( %d,%d) \n", ctime,otg_info->ptime[src][dst][state], otg_info->idt[src][dst], src, dst);
    return 0; // do not generate the packet, and keep the idt
  }



  size=size_dist(src, dst, state);
  LOG_I(OTG,"SOCKET :: Generate Packet for (Source= %d, Destination= %d,State= %d) , pkt size dist= %d, simu time= %d ,packet size=%d \n",
        src, dst, state, g_otg->size_dist[src][dst][state], otg_info->ctime, size);

  if (size>(5* sizeof(int)))
    size=size-(5* sizeof(int));
  else
    size=(5* sizeof(int))+10;

  payload=payload_pkts(size);

  return(payload);

}



control_hdr_t *otg_info_hdr_gen(int src, int dst, int trans_proto, int ip_v)
{
  control_hdr->src=src;
  control_hdr->dst=dst;
  control_hdr->trans_proto=trans_proto;
  control_hdr->ip_v=ip_v;
  LOG_I(OTG,"SOCKET :: control header src %d\n",control_hdr->src);
  LOG_I(OTG,"SOCKET :: control header src %d\n",control_hdr->dst);
  LOG_I(OTG,"SOCKET :: control header src %d\n",control_hdr->trans_proto);
  LOG_I(OTG,"SOCKET :: control header src %d\n",control_hdr->ip_v);


  return control_hdr;

}


void init_control_header()
{
  //set otg header to 0
  control_hdr = calloc(1, sizeof(control_hdr_t));

  if (control_hdr == NULL)
    /* Memory could not be allocated */
    printf("Couldn't allocate memory\n");

  memset(control_hdr, 0, sizeof(control_hdr_t));
}
