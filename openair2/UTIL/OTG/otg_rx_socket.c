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

/*! \file otg_rx_socket.c
* \brief function containing the OTG RX traffic generation functions with sockets
* \author A. Hafsaoui
* \date 2012
* \version 0.1
* \company Eurecom
* \email: openair_tech@eurecom.fr
* \note
* \warning
*/









#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <errno.h>



#include <pthread.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#define INVALID_SOCKET -1
#define SOCKET_ERROR -1


#include "otg_config.h"
#include "otg_rx_socket.h"
#include "otg_vars.h"



typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;


void *recv_ip4_tcp(void* csock)
{

  int sock_rcv;
  int socket=(int)csock;
  char buffer[PAYLOAD_MAX];

  int i=0;

  do {


    sock_rcv=recv(socket, buffer, PAYLOAD_MAX, 0);


    if  ((buffer!=NULL)&& (strlen(buffer)>0)) {
      //    payload_t* payload;
      //    payload->control_hdr = (control_hdr_t*) malloc (sizeof(control_hdr_t));
      //    payload->payload_rest = (char *) malloc (sock_rcv - sizeof(control_hdr_t));
      //    memcpy (payload->control_hdr, buffer, sizeof(control_hdr_t));
      //    memcpy (payload->payload_rest , buffer+sizeof(control_hdr_t), (sock_rcv - sizeof(control_hdr_t)));
      //    LOG_I(OTG,"SOCKET:: UDP-IP4 :: SRC=%d, DST=%d, PROTO=%d, IP VERSION=%d\n", payload->control_hdr->src,payload->control_hdr->dst, payload->control_hdr->trans_proto, payload->control_hdr->ip_v);


      LOG_I(OTG,"SOCKET:: TCP-IP4 :: size=%d  received=%d, Received buffer: %s   \n\n\n", strlen(buffer),  sock_rcv, buffer);
      //buffer[PAYLOAD_MAX] != '\0';

    }


  } while(sock_rcv !=-1);;

  LOG_I(OTG,"SOCKET:: TCP-IP4 :: size %d \n ", i) ;



  close(socket);
  pthread_exit(NULL);



  return NULL;
}




void server_socket_tcp_ip4()
{
#if defined (WIN32)
#include <winsock2.h>
  typedef int socklen_t;
#elif defined (linux)

#endif
#define PORT 7777


#if defined (WIN32)
  WSADATA WSAData;
  int erreur = WSAStartup(MAKEWORD(2,2), &WSAData);
#else
  int erreur = 0;
#endif

  SOCKADDR_IN sin;
  int sock;
  int recsize = sizeof sin;

  int sock_err, sock_rcv;



  if(!erreur) {
    sock = socket(AF_INET, SOCK_STREAM, 0);

    if(sock != INVALID_SOCKET) {
      LOG_I(OTG,"SOCKET:: TCP-IP4 :: Socket number= %d  is opend using TCP and IPv4\n", sock);

      sin.sin_addr.s_addr = htonl(INADDR_ANY);
      sin.sin_family = AF_INET;
      sin.sin_port = htons(PORT);
      sock_err = bind(sock, (SOCKADDR*) &sin, recsize);

      if(sock_err != SOCKET_ERROR) {
        sock_err = listen(sock, 5);
        LOG_I(OTG,"SOCKET:: TCP-IP4 :: Port Open %d...\n", PORT);

        if(sock_err != SOCKET_ERROR) {
          int cmpt_cl=1;

          /* Creation of the set of reading */
          fd_set readfs;

          while(1) {

            int csock;    /* conncted socket  */
            pthread_t id;  /* thread that manage the opened connection */


            /* Empty the set of reading and add the server to the socket */
            FD_ZERO(&readfs);
            FD_SET(sock, &readfs);

            /* If an error occurred at the select */
            if(select(sock + 1, &readfs, NULL, NULL, NULL) < 0) {
              perror("select()");
              exit(errno);
            }

            /* check if the socket server provides information to read */
            if(FD_ISSET(sock, &readfs)) {
              /* the server socket necessarily means that a client wants to connect to the server*/

              SOCKADDR_IN csin;
              int crecsize = sizeof csin;


              csock = accept(sock, (SOCKADDR *) &csin, &crecsize);


              /* create  new thread for the new connection */
              if (pthread_create(&id, NULL, (void *)recv_ip4_tcp, (void*)csock))
                LOG_W(OTG,"SOCKET:: TCP-IP4 ::pthread_create OK!\n");

              else
                LOG_W(OTG,"SOCKET:: TCP-IP4 ::Error in pthread_create \n");

              if (pthread_detach(id))
                LOG_W(OTG,"SOCKET:: TCP-IP4 ::pthread_detach OK!\n");
              else
                LOG_W(OTG,"SOCKET:: TCP-IP4 ::Error in pthread_detach\n");

              LOG_I(OTG,"SOCKET:: TCP-IP4 :: Client n=%d finish transmission\n", cmpt_cl);
              cmpt_cl+=1;
            }
          }
        }
      }
    }
  }

#if defined (WIN32)
  WSACleanup();
#endif

}


void server_socket_udp_ip4()
{


  int sockfd, bytes_recv, addr_in_size, cmpt_cl=1;
  u_short portnum = 12345;
  struct sockaddr_in *my_addr, *from;
  char msg[PAYLOAD_MAX];
  u_long fromaddr;

  addr_in_size = sizeof(struct sockaddr_in);

  //msg = (char *)malloc(PAYLOAD_MAX);
  from = (struct sockaddr_in *)malloc(addr_in_size);
  my_addr = (struct sockaddr_in *)malloc(addr_in_size);

  memset((char *)my_addr,(char)0,addr_in_size);
  my_addr->sin_family = AF_INET;
  my_addr->sin_addr.s_addr = htonl(INADDR_ANY);
  my_addr->sin_port = portnum;

  if((sockfd = socket (PF_INET, SOCK_DGRAM, 0)) < 0) {
    LOG_W(OTG,"SOCKET:: UDP-IP4 :: Error %d in socket: %s\n",errno,sys_errlist[errno]);
    exit(errno);
  };

  if(bind(sockfd, (struct sockaddr *)my_addr, addr_in_size) < 0) {
    LOG_W(OTG,"SOCKET:: UDP-IP4 :: Error %d in bind: %s\n",errno,sys_errlist[errno]);

    if(errno != EADDRINUSE) exit(errno);
  };

  LOG_I(OTG,"SOCKET:: UDP-IP4 :: Ready to receive UDP traffic\n");

  do {
    bytes_recv = recvfrom (sockfd,msg,PAYLOAD_MAX,0,(struct sockaddr *)from, &addr_in_size);



    if  (bytes_recv>0) {
      payload_t* payload;
      payload->control_hdr = (control_hdr_t*) malloc (sizeof(control_hdr_t));
      payload->payload_rest = (char *) malloc (bytes_recv - sizeof(control_hdr_t));
      memcpy (payload->control_hdr, msg, sizeof(control_hdr_t));
      memcpy (payload->payload_rest , msg+sizeof(control_hdr_t), (bytes_recv - sizeof(control_hdr_t)));



      LOG_I(OTG,"SOCKET:: UDP-IP4 :: SRC=%d, DST=%d, PROTO=%d, IP VERSION=%d\n", payload->control_hdr->src,payload->control_hdr->dst, payload->control_hdr->trans_proto, payload->control_hdr->ip_v);
      fromaddr = from->sin_addr.s_addr;

      // Update RX OTG info
      //otg_info->rx_num_pkt[payload->control_hdr->src][payload->control_hdr->dst]+=1;
      //otg_info->rx_num_bytes[payload->control_hdr->src][payload->control_hdr->dst]+=  bytes_recv + (HDR_IP_v4 + HDR_UDP);
      //


      LOG_I(OTG,"SOCKET:: UDP-IP4 :: From=%s , port= %d, data= , bytes NB=%d\n", (gethostbyaddr((char *)&fromaddr, sizeof(fromaddr), AF_INET))->h_name, from->sin_port, bytes_recv);
    }

  } while(bytes_recv !=-1);

  close(sockfd);

  LOG_I(OTG,"SOCKET:: TCP-IP4 :: Client n=%d finish transmission\n", cmpt_cl);
  cmpt_cl+=1;

}





int main (int argc, char **argv)
{

  int i;
  char *protocol=NULL;
  char *ip_version=NULL;

  for (i = 1; i <argc ; i ++) {
    if ('-' == argv[i][0]) {

      if(('h' == argv[i][1]) || ('H' == argv[i][1])) {
        printf("Help OTG RX:  \n. ./server [-P (protocol: TCP or UDP)] [-I (ip version: IP4 or IP6)]\n");
        return(0);

      }

      else if ('P' == argv[i][1]) {
        protocol=argv[i+1];

        if ((strcmp(argv[i+1],"TCP")==0) || (strcmp(argv[i+1],"UDP")==0) || (strcmp(argv[i+1],"tcp")==0) || (strcmp(argv[i+1],"udp")==0)) {
          protocol=argv[i+1];
          printf("Protocol=%s\n", protocol);
        }
      }

      else if ('I' == argv[i][1]) {
        if ((strcmp(argv[i+1],"IP4")==0) || (strcmp(argv[i+1],"IP6")==0) || (strcmp(argv[i+1],"ip4")==0) || (strcmp(argv[i+1],"ip6")==0)) {
          ip_version=argv[i+1];
          printf("IP version=%s\n", ip_version);
        }

      }
    }
  }


  //Select the server to use


  if ((ip_version !=NULL) && (protocol!=NULL)) {
    if (((strcmp(ip_version,"IP4")==0) ||(strcmp(ip_version,"ip4")==0)) && ((strcmp(protocol,"TCP")==0) ||(strcmp(protocol,"tcp")==0)))
      server_socket_tcp_ip4();
    else if  (((strcmp(ip_version,"IP4")==0) ||(strcmp(ip_version,"ip4")==0)) && ((strcmp(protocol,"UDP")==0) ||(strcmp(protocol,"udp")==0)))
      server_socket_udp_ip4();
  }


  return 0;

}


