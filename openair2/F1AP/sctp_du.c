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

/*! \file sctp_client.c
 * \brief sctp client procedures for f1ap
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
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netinet/sctp.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <unistd.h> // for close
#include <stdlib.h>

#define MAX_BUFFER  1024
#define NUM_THREADS 1

int cfd, i, flags, ret;
struct sockaddr_in saddr;
struct sctp_sndrcvinfo sndrcvinfo;
struct sctp_event_subscribe events;
struct sctp_initmsg initmsg;
uint8_t buffer_send[MAX_BUFFER+1];
uint8_t buffer_recv[MAX_BUFFER+1];

void f1ap_du_send_message(uint8_t *buffer_send, uint32_t length)
{
    /* Changing 9th character the character after # in the message buffer */
    //while (1) {
        //buffer_send[0] = rand();
        sctp_sendmsg( cfd, (void *)buffer_send, length,
                     NULL, 0, htonl(62), 0, 1 /* stream */, 0, 0 );
        printf("C - Sent: ");

        int i_ret;
        for (i_ret = 0; i_ret < length; i_ret++) {
          printf("%x ", *(buffer_send+i_ret));
        }

        printf("\n");
    //}
}

/* for test*/
void *send_func(void *argument)
{
    /* Changing 9th character the character after # in the message buffer */
    while (1) {
        buffer_send[0] = rand();
        sctp_sendmsg( cfd, (void *)buffer_send, (size_t)strlen(buffer_send),
                    NULL, 0, 0, 0, 1 /* stream */, 0, 0 );
        printf("C - Sent: %s\n", buffer_send);
    }
}

void *recv_func(void *argument)
{
    bzero( (void *)&buffer_recv, sizeof(buffer_recv) );

    while(ret = sctp_recvmsg( cfd, (void *)buffer_recv, sizeof(buffer_recv),
                           (struct sockaddr *)NULL, 0, &sndrcvinfo, &flags )) {
    
        printf("C - Received following data on stream %d, data is: %s\n",
                sndrcvinfo.sinfo_stream, buffer_recv); 
    }
    close(cfd);
    printf("C - close\n");
}

int sctp_du_init()
{
    pthread_t threads[NUM_THREADS];

    char *ipadd;
    ipadd = "127.0.0.1";
    printf("Use default ipaddress %s \n", ipadd);
   
    cfd = socket( AF_INET, SOCK_STREAM, IPPROTO_SCTP );
     
    /* Specify that a maximum of 3 streams will be available per socket */
    memset( &initmsg, 0, sizeof(initmsg) );
    initmsg.sinit_num_ostreams = 3;
    initmsg.sinit_max_instreams = 3;
    initmsg.sinit_max_attempts = 2;
    setsockopt( cfd, IPPROTO_SCTP, SCTP_INITMSG,
            &initmsg, sizeof(initmsg) );
 
    bzero( (void *)&saddr, sizeof(saddr) );
    saddr.sin_family = AF_INET;
    inet_pton(AF_INET, ipadd, &saddr.sin_addr);
    saddr.sin_port = htons(29008);

    int ret; 
    ret = connect( cfd, (struct sockaddr *)&saddr, sizeof(saddr) );
    if (ret == -1) {
        printf("C - Not founded Server, close\n");
        return 0;
    }
    memset( (void *)&events, 0, sizeof(events) );
    events.sctp_data_io_event = 1;
    setsockopt( cfd, SOL_SCTP, SCTP_EVENTS,
        (const void *)&events, sizeof(events) );

    /* Sending three messages on different streams */
    // send message
    //pthread_create(&threads[0], NULL, send_func, (void *)0);

    pthread_create(&threads[0], NULL, recv_func, (void *)0);

    return 0;
}