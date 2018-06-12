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

\file       rrc_2_rrm_msg.c

\brief     data formating for sending message over socket between RRC and RRM

\author     BURLOT Pascal

\date       16/07/08

\par     Historique:
        P.BURLOT 2009-01-20
            bug fix (memeory leak)
      Macro changed :
      RRM_CALLOC() by RRM_CALLOC2() in msg_rrc_sensing_meas_ind()

*******************************************************************************
*/
#ifdef RRC_EMUL

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include "L3_rrc_defs.h"
#include "rrm_sock.h"
#include "L3_rrc_interface.h"
#include "rrc_rrm_msg.h"
#include "rrm_util.h"

#else

#include "defs.h"

#endif

#ifdef TRACE
//! Macro creant la chaine a partir du nom de la variable
#define STRINGIZER(x) #x
//! Tableau pour le mode trace faisant la translation entre le numero et le nom du message
const char *Str_msg_rrc_rrm[NB_MSG_RRC_RRM] = {
  STRINGIZER(RRM_RB_ESTABLISH_REQ     ),
  STRINGIZER(RRC_RB_ESTABLISH_RESP    ),
  STRINGIZER(RRC_RB_ESTABLISH_CFM     ),
  STRINGIZER(RRM_RB_MODIFY_REQ        ),
  STRINGIZER(RRC_RB_MODIFY_RESP       ),
  STRINGIZER(RRC_RB_MODIFY_CFM        ),
  STRINGIZER(RRM_RB_RELEASE_REQ       ),
  STRINGIZER(RRC_RB_RELEASE_RESP      ),
  STRINGIZER(RRC_MR_ATTACH_IND        ),
  STRINGIZER(RRM_SENSING_MEAS_REQ     ),
  STRINGIZER(RRC_SENSING_MEAS_RESP    ),
  STRINGIZER(RRC_CX_ESTABLISH_IND     ),
  STRINGIZER(RRC_PHY_SYNCH_TO_MR_IND  ),
  STRINGIZER(RRC_PHY_SYNCH_TO_CH_IND  ),
  STRINGIZER(RRCI_CX_ESTABLISH_RESP   ),
  STRINGIZER(RRC_SENSING_MEAS_IND     ),
  STRINGIZER(RRM_SENSING_MEAS_RESP    ),
  STRINGIZER(RRC_RB_MEAS_IND          ),
  STRINGIZER(RRM_RB_MEAS_RESP         ),
  STRINGIZER(RRM_INIT_CH_REQ          ),
  STRINGIZER(RRCI_INIT_MR_REQ         )
} ;
#endif

/*!
*******************************************************************************
\brief  This function initialize the message header
\return any return value
*/
static void init_rrc_msg_head(
  msg_head_t    *msg_head , //!< message header to initialize
  Instance_t     inst     , //!< Instance ID
  MSG_RRC_RRM_T  msg_type , //!< type of message to initialize
  unsigned int   size     , //!< size of message
  Transaction_t  Trans_id   //!< transaction id associated to this message
)
{
  if ( msg_head != NULL ) {
    msg_head->start    = START_MSG ;
    msg_head->msg_type = 0xFF & msg_type ;
    msg_head->inst     = inst  ;
    msg_head->Trans_id = Trans_id  ;
    msg_head->size     = size;
  }
}


/*!
*******************************************************************************
\brief  La fonction formate en un message generique de reponse pour les
          fonctions :
            - rrc_rb_establish_resp(),rrc_rb_modify_resp(),rrc_rb_modify_resp(),
            - rrc_rb_release_resp() et rrc_sensing_meas_resp() .
\return message formate
*/
msg_t mesg;
msg_t *msg_rrc_generic_resp(
  Instance_t    inst     , //!< Instance ID
  MSG_RRC_RRM_T msg_type , //!< type of message
  Transaction_t Trans_id   //!< Transaction ID
)
{
  init_rrc_msg_head(&(mesg.head),inst, msg_type, 0 , Trans_id);
  mesg.data = NULL ;

  return &mesg ;
}

/*!
*******************************************************************************
\brief  La fonction formate en un message les parametres de la fonction
        rrc_rb_establish_resp().
\return message formate
*/
msg_t *msg_rrc_rb_establish_resp(
  Instance_t    inst     , //!< Instance ID
  Transaction_t Trans_id   //!< Transaction ID
)
{
  return msg_rrc_generic_resp( inst, RRC_RB_ESTABLISH_RESP, Trans_id) ;
}
/*!
*******************************************************************************
\brief  La fonction formate en un message les parametres de la fonction
        rrc_rb_establish_cfm().
\return message formate
*/
rrc_rb_establish_cfm_t P_rb_est_cfm ;//= RRM_CALLOC(rrc_rb_establish_cfm_t , 1 ) ;
msg_t *msg_rrc_rb_establish_cfm(
  Instance_t    inst     , //!< Instance ID
  RB_ID         Rb_id    , //!< Radio Bearer ID used by RRC
  RB_TYPE       RB_type  , //!< Radio Bearer Type
  Transaction_t Trans_id   //!< Transaction ID
)
{
  //  msg_t mesg;

  init_rrc_msg_head(&(mesg.head),inst,RRC_RB_ESTABLISH_CFM, sizeof( rrc_rb_establish_cfm_t ) ,Trans_id);
  P_rb_est_cfm.Rb_id        = Rb_id ;
  P_rb_est_cfm.RB_type      = RB_type ;
  mesg.data = (char *) &P_rb_est_cfm ;
  return &mesg ;

}

/*!
*******************************************************************************
\brief  La fonction formate en un message les parametres de la fonction
        rrc_rb_modify_resp().
\return message formate
*/
msg_t *msg_rrc_rb_modify_resp(
  Instance_t    inst     , //!< Instance ID
  Transaction_t Trans_id   //!< Transaction ID
)
{
  return msg_rrc_generic_resp( inst,RRC_RB_MODIFY_RESP, Trans_id) ;
}
/*!
*******************************************************************************
\brief  La fonction formate en un message les parametres de la fonction
        rrc_rb_modify_cfm().
\return message formate
*/
rrc_rb_modify_cfm_t P_rb_md_cfm;
msg_t *msg_rrc_rb_modify_cfm(
  Instance_t    inst     , //!< Instance ID
  RB_ID         Rb_id    , //!< Radio Bearer ID used by RRC
  Transaction_t Trans_id   //!< Transaction ID
)
{
  // msg_t mesg;
  init_rrc_msg_head(&(mesg.head),inst,RRC_RB_MODIFY_CFM, sizeof( rrc_rb_modify_cfm_t ) ,Trans_id);
  P_rb_md_cfm.Rb_id        = Rb_id ;
  mesg.data = (char *) &P_rb_md_cfm ;
  return &mesg ;

}

/*!
*******************************************************************************
\brief  La fonction formate en un message les parametres de la fonction
          rrc_rb_release_resp().
\return message formate
*/
msg_t *msg_rrc_rb_release_resp(
  Instance_t    inst     , //!< Instance ID
  Transaction_t Trans_id   //!< Transaction ID
)
{
  return msg_rrc_generic_resp( inst,RRC_RB_RELEASE_RESP, Trans_id) ;
}
/*!
*******************************************************************************
\brief  La fonction formate en un message les parametres de la fonction
        rrc_MR_attach_ind().
\return message formate
*/
rrc_MR_attach_ind_t P_att_ind;
msg_t * msg_rrc_MR_attach_ind(
  Instance_t    inst     , //!< Instance ID
  L2_ID         L2_id
)
{
  // msg_t mesg;
  init_rrc_msg_head(&(mesg.head),inst,RRC_MR_ATTACH_IND, sizeof( rrc_MR_attach_ind_t ) ,0);
  memcpy( P_att_ind.L2_id.L2_id, L2_id.L2_id, sizeof(L2_ID) )  ;
  mesg.data = (char *) &P_att_ind ;
  return &mesg ;
}

/*!
*******************************************************************************
\brief  La fonction formate en un message les parametres de la fonction
        rrc_sensing_meas_resp().
\return message formate
*/
msg_t *msg_rrc_sensing_meas_resp(
  Instance_t    inst     , //!< Instance ID
  Transaction_t Trans_id   //!< Transaction ID
)
{
  return msg_rrc_generic_resp( inst,RRC_SENSING_MEAS_RESP, Trans_id) ;
}
/*!
*******************************************************************************
\brief  La fonction formate en un message les parametres de la fonction
        rrc_cx_establish_ind().
\return message formate
*/
rrc_cx_establish_ind_t P_cx_est_ind;
msg_t * msg_rrc_cx_establish_ind(
  Instance_t     inst      , //!< Instance ID
  L2_ID          L2_id     , //!< Layer 2 (MAC) ID
  Transaction_t  Trans_id  , //!< Transaction ID
  unsigned char *L3_info   , //!< Optional L3 Information
  L3_INFO_T      L3_info_t , //!< Type of L3 Information
  RB_ID          DTCH_B_id , //!< RBID of broadcast IP service (MR only)
  RB_ID          DTCH_id     //!< RBID of default IP service (MR only)
)
{
  //msg_t mesg;
  init_rrc_msg_head(&(mesg.head),inst,RRC_CX_ESTABLISH_IND, sizeof( rrc_cx_establish_ind_t ) ,Trans_id);
  memcpy( P_cx_est_ind.L2_id.L2_id, L2_id.L2_id, sizeof(L2_ID) )  ;
  P_cx_est_ind.DTCH_B_id    = DTCH_B_id ;
  P_cx_est_ind.DTCH_id      = DTCH_id   ;
  P_cx_est_ind.L3_info_t    = L3_info_t ;

  if ( L3_info_t == IPv4_ADDR ) {
    memcpy( P_cx_est_ind.L3_info, L3_info, 4 );
  } else if ( L3_info_t == IPv6_ADDR ) {
    memcpy( P_cx_est_ind.L3_info, L3_info, 16 );
  }

  mesg.data = (char *) &P_cx_est_ind ;
  return &mesg ;
}
/*!
*******************************************************************************
\brief  La fonction formate en un message les parametres de la fonction
        rrc_phy_synch_to_MR_ind().
\return message formate
*/
rrc_phy_synch_to_MR_ind_t P_snc_mr;
msg_t * msg_rrc_phy_synch_to_MR_ind(
  Instance_t     inst  , //!< Instance ID
  L2_ID          L2_id
)
{
  //msg_t smsg;
  init_rrc_msg_head(&(mesg.head),inst,RRC_PHY_SYNCH_TO_MR_IND, sizeof( rrc_phy_synch_to_MR_ind_t ) ,0);
  memcpy(&P_snc_mr.L2_id,(L2_ID*)&L2_id,sizeof(L2_ID));
  mesg.data = (char *)&P_snc_mr ;
  msg("[msg_rrc_phy_synch_to_MR_ind] from Inst :%d\n",mesg.head.inst);
  return &mesg ;
}
/*!
*******************************************************************************
\brief  La fonction formate en un message les parametres de la fonction
          rrc_phy_synch_to_CH_ind().
\return message formate
*/
rrc_phy_synch_to_CH_ind_t P_snc_ch;
msg_t * msg_rrc_phy_synch_to_CH_ind(
  Instance_t   inst      , //!< Instance ID
  unsigned int Ch_index  , //!< Clusterhead index
  L2_ID        L2_id
)
{
  //  msg_t smsg;
  init_rrc_msg_head(&(mesg.head),inst, RRC_PHY_SYNCH_TO_CH_IND, sizeof( rrc_phy_synch_to_CH_ind_t ) ,0);
  P_snc_ch.Ch_index     = Ch_index  ;
  memcpy(&P_snc_ch.L2_id,(L2_ID*)&L2_id,sizeof(L2_ID));
  mesg.data = (char *) &P_snc_ch;
  msg("[msg_rrc_phy_synch_to_CH_ind] from Inst :%d\n",mesg.head.inst);
  return &mesg ;
}

/*!
*******************************************************************************
\brief  La fonction formate en un message les parametres de la fonction
          rrc_sensing_meas_ind().
\return message formate
*/
rrc_sensing_meas_ind_t P_sens_ind;
msg_t * msg_rrc_sensing_meas_ind(
  Instance_t      inst         , //!< Instance ID
  L2_ID           L2_id        , //!< Layer 2 ID (MAC) of sensing node
  unsigned int    NB_meas      , //!< Layer 2 ID (MAC) of sensing node
  SENSING_MEAS_T *Sensing_meas , //!< Sensing Information
  Transaction_t   Trans_id       //!< Transaction ID
)
{

  //  msg_t mesg;
  unsigned int size = sizeof( rrc_sensing_meas_ind_t ) + (NB_meas-1) * sizeof(SENSING_MEAS_T) ;
  // Note : (NB_meas-1) car la première est incorporé dans  rrc_sensing_meas_ind_t
  init_rrc_msg_head(&(mesg.head),inst, RRC_SENSING_MEAS_IND, size ,Trans_id);
  memcpy( P_sens_ind.L2_id.L2_id, L2_id.L2_id, sizeof(L2_ID) )  ;

  if ( NB_meas > 0 ) {
    memcpy( P_sens_ind.Sensing_meas , Sensing_meas, NB_meas * sizeof(SENSING_MEAS_T) )  ;
  }

  P_sens_ind.NB_meas      = NB_meas   ;
  mesg.data = (char *) &P_sens_ind ;
  return &mesg ;
}

/*!
*******************************************************************************
\brief  La fonction formate en un message les parametres de la fonction
        rrc_rb_meas_ind().
\return message formate
*/
rrc_rb_meas_ind_t P_rb_meas;
msg_t * msg_rrc_rb_meas_ind(
  Instance_t      inst         , //!< Instance ID
  RB_ID           Rb_id        , //!< Radio Bearer ID
  L2_ID           L2_id        , //!< Layer 2 (MAC) IDs for link
  MEAS_MODE       Meas_mode    , //!< Measurement mode (periodic or event-driven)
  MAC_RLC_MEAS_T *Mac_rlc_meas , //!< MAC/RLC measurements
  Transaction_t   Trans_id       //!< Transaction ID
)
{
  //msg_t mesg;
  init_rrc_msg_head(&(mesg.head),inst, RRC_RB_MEAS_IND, sizeof( rrc_rb_meas_ind_t ) ,Trans_id);
  memcpy( P_rb_meas.L2_id.L2_id, L2_id.L2_id, sizeof(L2_ID) )  ;
  memcpy( &(P_rb_meas.Mac_rlc_meas), Mac_rlc_meas, sizeof(L2_ID) )  ;
  P_rb_meas.Rb_id        = Rb_id     ;
  P_rb_meas.Meas_mode    = Meas_mode ;
  mesg.data = (char *) &P_rb_meas;
  return &mesg ;

}



/*****************************************************************************/
rrc_init_scan_req_t P_init_scan;
msg_t * msg_rrc_init_scan_req(
  Instance_t      inst         , //!< Instance ID
  L2_ID           L2_id        , //!< Layer 2 (MAC) IDs for link
  unsigned int Interv
)
{
  //msg_t mesg;
  msg("RRCI: inst %d sendind rrc_init_scan_req from FC %d %d\n",inst,L2_id.L2_id[0],L2_id.L2_id[7]);
  init_rrc_msg_head(&(mesg.head),inst, RRC_INIT_SCAN_REQ, sizeof( rrc_init_scan_req_t ) ,0);
  memcpy( &P_init_scan.L2_id.L2_id[0], &L2_id.L2_id[0], sizeof(L2_ID) )  ;
  P_init_scan.interv=Interv;

  return &mesg ;

}



/*****************************************************************************/

rrc_update_sens_t P_update_sens;
msg_t * msg_rrc_update_sens(
  Instance_t      inst         , //!< Instance ID
  Sens_ch_t *Sens,
  L2_ID L2_id,
  unsigned char NB_info
)
{
  //msg_t mesg;
  init_rrc_msg_head(&(mesg.head),inst, RRC_UPDATE_SENS, sizeof( rrc_update_sens_t ) ,0);
  memcpy( &P_update_sens.L2_id.L2_id[0], &L2_id.L2_id[0], sizeof(L2_ID) )  ;
  memcpy( &P_update_sens.Sens_meas[0], (rrc_update_sens_t *)Sens, NB_info*sizeof(Sens_ch_t) )  ;
  P_update_sens.NB_info=NB_info;
  return &mesg ;

}



/*****************************************************************************/

rrc_end_scan_req_t P_end_scan;
msg_t * msg_rrc_end_scan_req(
  Instance_t      inst         , //!< Instance ID
  unsigned char CH_index
)
{

  //msg_t mesg;
  msg("RRC: send END_SCAN_REQ, INST %d\n",inst);
  init_rrc_msg_head(&(mesg.head),inst, RRC_END_SCAN_REQ, sizeof( rrc_end_scan_req_t ) ,0);
  P_update_sens.L2_id.L2_id[0]=CH_index;
  return &mesg ;

}

