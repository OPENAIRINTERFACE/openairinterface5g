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

/*________________________openair_rrc_utils.c________________________

 Authors : Hicham Anouar
 Company : EURECOM
 Emails  : anouar@eurecom.fr
________________________________________________________________*/




#include "defs.h"
#include "extern.h"


//------------------------------------------------------------------------------------------------------------------//
uint16_t find_free_dtch_position(uint8_t Mod_id, uint16_t UE_CH_index) {
  uint16_t i,j;

  if(UE_CH_index==0) {
    j=0;
  } else {
    j=1;
  }

  for(i=j; i<NB_RAB_MAX; i++) { //first RAB IS BROADCAST DTCH
    if(CH_rrc_inst[Mod_id].Rab[i][UE_CH_index].Active==0) {
      return( i);
    }
  }

  msg("NO FREE DTCH LCHAN, exit... \n");
  Mac_rlc_xface->macphy_exit("NO FREE DTCH LCHAN, exit");
  return 0;
}

//-------------------------------------------------------------------------------------------//
uint8_t rrc_find_free_ue_index(uint8_t Mod_id) {
  //-------------------------------------------------------------------------------------------//
  uint16_t i;

  for(i=1; i<=NB_CNX_CH; i++)
    if ( (CH_rrc_inst[Mod_id].Info.UE_list[i][0] == 0) &&
         (CH_rrc_inst[Mod_id].Info.UE_list[i][1] == 0) &&
         (CH_rrc_inst[Mod_id].Info.UE_list[i][2] == 0) &&
         (CH_rrc_inst[Mod_id].Info.UE_list[i][3] == 0) &&
         (CH_rrc_inst[Mod_id].Info.UE_list[i][4] == 0)) {
      return i;
    }

  return 0xff;
}


//-------------------------------------------------------------------------------------------//
unsigned short rrc_find_ue_index(unsigned char Mod_id, L2_ID Mac_id) {
  //-------------------------------------------------------------------------------------------//
  unsigned char i;
  /*
  for(i=0;i<=NB_CNX_CH;i++)
    if( bcmp(Mac_id.L2_id,CH_rrc_inst[Mod_id].Info.UE_list[i].L2_id,sizeof(L2_ID))==0)
      return i;
  return i;
  */
}
//-------------------------------------------------------------------------------------------//
uint8_t rrc_is_node_isolated(uint8_t Mod_id) {
  //-------------------------------------------------------------------------------------------//
  /*uint8_t i;
  for(i=0;i<NB_CNX_UE;i++)
    if( Rrc_inst[Mod_id].Rrc_info[i].Info.UE_info.Nb_bcch_wait < NB_WAIT_CH_BCCH)
      break;
  return((i==NB_CNX_UE)?1:0);
  */
}

//-------------------------------------------------------------------------------------------//
uint8_t find_rrc_info_index(uint8_t Mod_id,uint8_t CH_id) {
  //-------------------------------------------------------------------------------------------//
  /*uint8_t i;

  for (i=0;i<NB_CNX_UE;i++){
    msg("RRC_FIND_CH_INFO %d, Now %d \n",CH_id,Rrc_inst[Mod_id].Rrc_info[i].Info.UE_info.CH_id);
    if(Rrc_inst[Mod_id].Rrc_info[i].Info.UE_info.CH_id == CH_id) return i;
  }
  error_msg("[OPENAIR][RRC] RRC_INFO_INDEX: FATAL ERROR: Not yet Pre_Synchronized with CH ???%d\n",CH_id);
  exit(-1);
  */
}
/*
//--------------------------------------------------------------------------------------------//
SRB_INFO* rrc_find_srb_info(uint8_t Mod_id,uint16_t Srb_id){
    //----------------------------------------------------------------------------------------//
  uint8_t i;
  if(Rrc_inst[Mod_id].Rrc_info[0].Status == CH_READY){
    for(i=0;i<NB_CNX_CH;i++){
      //msg("i=%d, Srb0Id %d Srb2Id %d S_id=%d\n",i,Rrc_inst[Mod_id].Srb0[i].Srb_id,Rrc_inst[Mod_id].Srb2[i].Srb_info.Srb_id, Srb_id);
      if(Rrc_inst[Mod_id].Srb0[i].Srb_id == Srb_id)
  return &Rrc_inst[Mod_id].Srb0[i];
      if(Rrc_inst[Mod_id].Srb1[i].Srb_id == Srb_id)
  return &Rrc_inst[Mod_id].Srb1[i];
      if(Rrc_inst[Mod_id].Srb2[i].Srb_info.Srb_id == Srb_id)
  return &Rrc_inst[Mod_id].Srb2[i].Srb_info;
    }
  }
  else{
    for(i=0;i<NB_CNX_UE;i++){
      //msg("i=%d, Srb0Id %d Srb2Id %d S_id=%d\n",i,Rrc_inst[Mod_id].Srb0[i].Srb_id,Rrc_inst[Mod_id].Srb2[i].Srb_info.Srb_id, Srb_id);
      if(Rrc_inst[Mod_id].Srb0[i].Srb_id == Srb_id)
  return &Rrc_inst[Mod_id].Srb0[i];
      if(Rrc_inst[Mod_id].Srb1[i].Srb_id == Srb_id)
  return &Rrc_inst[Mod_id].Srb1[i];
      if(Rrc_inst[Mod_id].Srb2[i].Srb_info.Srb_id == Srb_id)
  return &Rrc_inst[Mod_id].Srb2[i].Srb_info;
    }
  }
  return(SRB_INFO*)0;
}

//--------------------------------------------------------------------------------------------//
RB_INFO* rrc_find_rb_info(uint8_t Mod_id,uint16_t Rb_id){
    //----------------------------------------------------------------------------------------//
  uint16_t i,j;
  if(Rrc_inst[Mod_id].Rrc_info[0].Status == CH_READY){
    for(i=0;i<NB_CNX_CH;i++)
      for(j=0;j<NB_RAB_MAX;j++){
  //  msg("j=%d, exit_id %d rb_id %d\n",j,Rrc_inst[Mod_id].Rab[j][i].Rb_info.Rb_id,Rb_id);
        if((Rrc_inst[Mod_id].Rab[j][i].Active == 1) && (Rrc_inst[Mod_id].Rab[j][i].Rb_info.Rb_id == Rb_id))
    return &Rrc_inst[Mod_id].Rab[j][i].Rb_info;
      }
  }
  else{
    for(i=0;i<NB_CNX_UE;i++)
      for(j=0;j<NB_RAB_MAX;j++){
  //  msg("j=%d, exit_id %d rb_id %d\n",j,Rrc_inst[Mod_id].Rab[j][i].Rb_info.Rb_id,Rb_id);
        if((Rrc_inst[Mod_id].Rab[j][i].Active == 1) && (Rrc_inst[Mod_id].Rab[j][i].Rb_info.Rb_id == Rb_id))
    return &Rrc_inst[Mod_id].Rab[j][i].Rb_info;
      }
  }
  return(RB_INFO*)0;
}
*/

/*------------------------------------------------------------------------------*/
unsigned char rrc_is_mobile_already_associated(uint8_t Mod_id, L2_ID Mac_id) {
  /*------------------------------------------------------------------------------*/
  /*
  unsigned char i;
  for(i=0;i<NB_CNX_CH;i++)
    if( bcmp(Mac_id.L2_id,CH_rrc_inst[Mod_id].Info.UE_list[i].L2_id,sizeof(L2_ID))==0)
      return 1;
  return 0;
  */
}


//-------------------------------------------------------------------------------------------//
void rrc_reset_buffer(RRC_BUFFER *Rrc_buffer) {
  //-------------------------------------------------------------------------------------------//
  //    Rrc_buffer->Header->Rv_tb_idx=0;
  //  Rrc_buffer->W_idx=0;
  //Rrc_buffer->R_idx=0;
}
