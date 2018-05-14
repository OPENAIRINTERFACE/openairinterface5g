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

\file     rrm_sock.h

\brief    common header for the communication between RRM and RRC/CMM/PUSU.

\author   BURLOT Pascal

\date     15/07/08


\par     History:
      $Author$  $Date$  $Revision$
      $Id$
      $Log$

*******************************************************************************
*/

#ifndef __RRC_RRM_INTERFACE_H__
#define __RRC_RRM_INTERFACE_H__


//#ifdef RRC_RRM_XFACE

/*!
*******************************************************************************
\brief   Entete des messages de RRM/CMM/RRC
*/
typedef struct {
  unsigned short start    ; ///< Identification du debut de message
  unsigned char  inst     ; ///< Identification de l'instance RRM
  unsigned char  msg_type ; ///< Identification du type message
  unsigned int   size     ; ///< taille du message
  unsigned int   Trans_id ; ///< Identification de la transaction
} msg_head_t ;

/*!
*******************************************************************************
\brief   Definition de la structure d'un message a envoyer sur un socket:
          - RRM->RRC
          - RRC->RRM
          - RRCI->RRC
          - RRC->RRCI
          - CMM->RRM
          - RRM->CMM
*/
typedef struct {
  msg_head_t  head  ; ///< entete du message
  char    *data ; ///< message
} msg_t ;




#ifdef __cplusplus
extern "C" {
#endif

//! \brief Socket path associated to RRM-CMM interface
#define RRM_CMM_SOCK_PATH "/tmp/rrm_cmm_socket"
//! \brief Socket path associated to CMM-RRM interface
#define CMM_RRM_SOCK_PATH "/tmp/cmm_rrm_socket"

//! \brief Socket path associated to RRM-RRC interface
#define RRM_RRC_SOCK_PATH "/tmp/rrm_rrc_socket"
//! \brief Socket path associated to RRC-RRM interface
#define RRC_RRM_SOCK_PATH "/tmp/rrc_rrm_socket"

//! \brief Socket path associated to RRM-PUSU interface
#define RRM_PUSU_SOCK_PATH "/tmp/rrm_pusu_socket"
//! \brief Socket path associated to PUSU-RRM interface
#define PUSU_RRM_SOCK_PATH "/tmp/pusu_rrm_socket"


//! \brief Identification of the RRM/CMM/RRC message begin
#define START_MSG      0xA533
//! \brief Identification of the PUSU message begin
#define START_MSG_PUSU 0xCC


#include <sys/socket.h>
#include <sys/un.h>


/*!
*******************************************************************************
\brief  Definition de la structure definissant le socket pour envoyer les messages
*/
typedef struct {
  int s                   ; ///< identification du socket
  struct  sockaddr_un un_local_addr     ; ///< Adresse local si unix socket
  struct  sockaddr_un un_dest_addr    ; ///< Adresse destinataire si unix socket
} sock_rrm_t ;


/* *** Fonctions relatives aux interfaces CMM ou RRC *** */

int open_socket( sock_rrm_t *s  ,char *path_local, char *path_dest , int rrm_inst ) ;
void close_socket(sock_rrm_t *sock ) ;
int send_msg_sock(sock_rrm_t *s   ,msg_t *msg ) ;
char *recv_msg( sock_rrm_t *s ) ;
#ifdef __cplusplus
}
#endif

int send_msg(void *, msg_t *);

#endif
