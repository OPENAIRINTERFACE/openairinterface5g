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

/*!
*******************************************************************************

\file     rrm_sock.c

\brief    RRM (Radio Ressource Manager ) Socket: communication withe other medium:
          - RRC ,
          - CMM ,
          - PUSU

\author   BURLOT Pascal

\date     10/07/08


\par     Historique:
        P.BURLOT 2009-01-20
            + send a message via fifo:
                - sending header
                - and data if any

*******************************************************************************
*/

#ifndef RRC_RRM_FIFOS_XFACE
  #include <stdio.h>
  #include <stdlib.h>
  #include <errno.h>
  #include <string.h>
  #include <unistd.h>

  #include <sys/socket.h>
  #include <sys/un.h>

#else

  #include<rtai_fifos.h>

#endif

#include "rrc_rrm_interface.h"
#include "defs.h"
//! \brief  Taille maximale de la charge utile
#define SIZE_MAX_PAYLOAD  2048
//! \brief PID de l'espace utilisateur (Netlink mode)
//#define PID_USERSPACE   0xAA





#ifndef RRC_RRM_FIFOS_XFACE

/*!
*******************************************************************************
\brief  This function opens a unix socket for the rrm communication
\return  The return value is a socket handle
*/
int open_socket(
  sock_rrm_t *s,    ///< socket descriptor
  char *path_local,   ///< local socket path if unix socket
  char *path_dest,    ///< host  Socket path if unix socket
  int rrm_inst        ///< instance of the rrm entity
) {
  /* Unix socket */
  int   socket_fd ;
  int   len ;

  if ((socket_fd = socket(AF_UNIX, SOCK_DGRAM, 0)) == -1) {
    perror("unix socket");
    return -1 ;
  }

  memset(&(s->un_local_addr), 0, sizeof(struct  sockaddr_un));
  s->un_local_addr.sun_family = AF_UNIX;
  sprintf(s->un_local_addr.sun_path,"%s%d", path_local, rrm_inst );
  unlink(s->un_local_addr.sun_path);
  msg("local %s\n",s->un_local_addr.sun_path);
  len = strlen((s->un_local_addr).sun_path) + sizeof((s->un_local_addr).sun_family);

  if (bind(socket_fd, (struct sockaddr *)&(s->un_local_addr), len) == -1) {
    perror("bind");
    return -1 ;
  }

  memset(&(s->un_dest_addr), 0, sizeof(struct   sockaddr_un));
  s->un_dest_addr.sun_family = AF_UNIX;
  sprintf(s->un_dest_addr.sun_path,"%s%d", path_dest, rrm_inst );
  msg("Dest %s\n",s->un_dest_addr.sun_path);
  s->s = socket_fd ;
  return socket_fd ;
}
/*!
*******************************************************************************
\brief  This function closes a RRM socket
\return none
*/
void close_socket(
  sock_rrm_t *sock  ///< the socket handle
) {
  shutdown(sock->s, SHUT_RDWR);
  close(sock->s);
}

/*!
*******************************************************************************
\brief  This function send a buffer message to the unix socket
\return if OK then "0" is returned else "-1"
*/
char BUFF[2048];
int send_msg_sock(
  sock_rrm_t *s,   ///< socket descriptor
  msg_t *smsg       ///< the message to send
) {
  /* Unix socket */
  int         ret   = 0 ;
  //  char        *buf    = NULL;
  struct  msghdr    msghd ;
  struct  iovec     iov;
  int         taille  = sizeof(msg_head_t)  ;

  if ( smsg == NULL ) {
    return -1 ;
  }

  if ( smsg->data != NULL ) {
    taille += smsg->head.size ;
  }

  //buf = RRM_MALLOC(char, taille);
  //if (buf ==NULL)
  //return -1 ;
  memcpy( BUFF, &(smsg->head), sizeof(msg_head_t) ) ;
  memcpy( BUFF+sizeof(msg_head_t), smsg->data, smsg->head.size ) ;
  iov.iov_base    = (void *)BUFF;
  iov.iov_len     = taille ;
  msghd.msg_name        = (void *)&(s->un_dest_addr);
  msghd.msg_namelen     = sizeof(s->un_dest_addr);
  msghd.msg_iov         = &iov;
  msghd.msg_iovlen      = 1;
  msghd.msg_control     = NULL ;
  msghd.msg_controllen  =   0 ;

  if ( sendmsg(s->s, &msghd, 0) < 0 ) {
    ret = -1;
    msg("socket %d, dest %s\n",s->s,s->un_dest_addr.sun_path);
    perror("sendmsg:unix socket");
  }

  //RRM_FREE(buf) ;
  //RRM_FREE(msg->data) ;
  //RRM_FREE(msg) ;
  return ret ;
}

/*!
*******************************************************************************
\brief  This function read a buffer from a unix socket
\return the function returns a message pointer. If the pointeur is NULL, a error
        is happened.
*/
char *recv_msg(
  sock_rrm_t *s   ///< socket descriptor
) {
  /* Unix socket */
  char        *buf = NULL;
  char        *smsg = NULL;
  struct  msghdr    msghd ;
  struct  iovec     iov;
  int         size_msg ;
  msg_head_t      *head  ;
  int         ret ;
  int taille =  SIZE_MAX_PAYLOAD ;
  buf         = RRM_CALLOC( char,taille);

  if ( buf == NULL ) {
    return NULL ;
  }

  iov.iov_base      = (void *)buf;
  iov.iov_len       = taille ;
  msghd.msg_name      = (void *)&(s->un_dest_addr);
  msghd.msg_namelen   = sizeof(s->un_dest_addr);
  msghd.msg_iov       = &iov;
  msghd.msg_iovlen    = 1;
  msghd.msg_control   = NULL ;
  msghd.msg_controllen= 0 ;
  ret = recvmsg(s->s, &msghd, 0 ) ;

  if ( ret <= 0  ) {
    // non-blocking socket
    // perror("PB recvmsg_un");
    RRM_FREE(buf);
    return NULL ;
  }

  if (msghd.msg_flags != 0 ) {
    fprintf(stderr,"error recvmsg_un: 0x%02x\n", msghd.msg_flags) ;
    RRM_FREE(buf);
    return NULL ;
  }

  head    = (msg_head_t *) buf  ;
  size_msg  = sizeof(msg_head_t) + head->size ;
  smsg    = RRM_CALLOC(char, size_msg ) ;

  if ( smsg != NULL ) {
    memcpy( smsg, buf, size_msg ) ;
  }

  RRM_FREE( buf ) ;
  return smsg ;
}

#else  //XFACE

int send_msg_fifo(int *s, msg_t *fmsg) {
  int   ret   = 0, ret1;
  int  taille = sizeof(msg_head_t)  ;
  msg("write on fifos %d, msg %p\n",*s,fmsg);

  if ( fmsg == NULL ) {
    return -1 ;
  }

  // envoi le header
  ret1 = rtf_put (*s,(char *) &(fmsg->head), taille);

  if(ret1 <0) {
    msg("rtf_put H ERR %d\n",ret1);
    rtf_reset(*s);
    return ret1;
  }

  ret=ret1;

  // envoi les datas si elles sont definis
  if ( fmsg->data != NULL ) {
    ret1 += rtf_put (*s,(char *) fmsg->data, fmsg->head.size);

    if(ret1 <0) {
      msg("rtf_put D ERR %d\n",ret1);
      rtf_reset(*s);
      return ret1;
    }
  }

  ret+=ret1;
  return ret;
}

#endif //XFACE

int send_msg(void *s, msg_t *smsg) {
  send_msg_sock((sock_rrm_t *)s, smsg);
}
