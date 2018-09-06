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

/*! \file sctp_server.c
 * \brief sctp server procedures for f1ap
 * \author EURECOM/NTUST
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email: navid.nikaein@eurecom.fr, bing-kai.hong@eurecom.fr
 * \note
 * \warning
 */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h> // for close
#include <stdlib.h>
#include "f1ap_handlers.h"

#define MAX_BUFFER  1024

/* server */

int sfd, cfd, len, flags;
struct sctp_sndrcvinfo sndrcvinfo;
struct sockaddr_in saddr, caddr;
struct sctp_initmsg initmsg;
uint8_t buff[INET_ADDRSTRLEN]; 

void send_func(int cfd) {
    uint8_t buffer_send[MAX_BUFFER+1];
    /* Changing 9th character the character after # in the message buffer */
    buffer_send[0] = rand();
    sctp_sendmsg( cfd, (void *)buffer_send, (size_t)strlen(buffer_send),
                NULL, 0, htons(62), 0, 1 /* stream */, 0, 0 );
    printf("S - Sent: %s\n", buffer_send);
}

void *recv_func(void *cfd) {
    int ret;
    uint8_t buffer_recv[MAX_BUFFER+1];

    //recv message
    bzero( (void *)&buffer_recv, sizeof(buffer_recv) );
    while(ret = sctp_recvmsg( *(int*)cfd, (void *)buffer_recv, sizeof(buffer_recv),
                        (struct sockaddr *)NULL, 0, &sndrcvinfo, &flags )) {
         //send_func(*(int*)cfd);
         //printf("S - cfd = %d\n", *(int*)cfd);
        printf("S - Received following data on ppid %d, data is: %x\n",
                sndrcvinfo.sinfo_ppid, buffer_recv);
        printf("S - Received following data on stream %d, data is: \n", sndrcvinfo.sinfo_stream);

        int i_ret;
        for (i_ret = 0; i_ret < sizeof(buffer_recv); i_ret++) {
          printf("%x ", *(buffer_recv+i_ret));
        }

        printf("\n");

        f1ap_handle_message(1/*sctp_data_ind->assoc_id*/, 1/*sctp_data_ind->stream*/,
                            buffer_recv, sizeof(buffer_recv));

        //f1ap_decode_pdu(NULL , buffer_recv, sizeof(buffer_recv));
    }
    printf("ret = %d\n", ret);
    close( *(int*)cfd );
  return NULL;
}

int sctp_cu_init(void) {
    pthread_t threads;
    printf("S - Waiting for socket_accept\n"); 

    sfd = socket( AF_INET, SOCK_STREAM, IPPROTO_SCTP );
    bzero( (void *)&saddr, sizeof(saddr) );
    saddr.sin_family = AF_INET;
    saddr.sin_addr.s_addr = htonl( INADDR_ANY );
    saddr.sin_port = htons(29008);
 
    bind( sfd, (struct sockaddr *)&saddr, sizeof(saddr) );
 
    /* Maximum of 3 streams will be available per socket */
    memset( &initmsg, 0, sizeof(initmsg) );
    initmsg.sinit_num_ostreams = 3;
    initmsg.sinit_max_instreams = 3;
    initmsg.sinit_max_attempts = 2;
    setsockopt( sfd, IPPROTO_SCTP, SCTP_INITMSG,
            &initmsg, sizeof(initmsg) );
 
    listen( sfd, 5 );

    while(cfd=accept(sfd, (struct sockaddr *)&caddr, (socklen_t*)&caddr)) {
        printf("-------- S - Connected to %s\n",
               inet_ntop(AF_INET, &caddr.sin_addr, buff,
               sizeof(buff)));
        int *thread_args = malloc(sizeof(int));
        *thread_args = cfd;
        pthread_create(&threads, NULL, recv_func, thread_args);
    }

    printf("S - close\n");
    return 0;
}
