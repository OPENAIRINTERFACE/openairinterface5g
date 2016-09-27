/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*! \file rrc_config.c
* \brief rrc cinfiguration
* \author Raymond Knopp and Navid Nikaein
* \date 2011
* \version 1.0
* \company Eurecom
* \email: raymond.knopp@eurecom.fr and  navid.nikaein@eurecom.fr
*/


#include "defs.h"

#include "extern.h"


#ifdef PHY_EMUL
#include "SIMULATION/simulation_defs.h"
extern EMULATION_VARS *Emul_vars;
#endif
extern CH_MAC_INST *CH_mac_inst;
extern UE_MAC_INST *UE_mac_inst;
#ifdef BIGPHYSAREA
extern void *bigphys_malloc(int);
#endif

void rrc_init_ch_req(unsigned char Mod_id, rrm_init_ch_req_t  *smsg)
{
  uint16_t Index;
  MAC_CONFIG_REQ Mac_config_req;
  // MAC_MEAS_REQ Mac_meas_req;

  msg("-----------------------------------------------------------------------------------------------------------------\n");
  printk("[RRC_CONFIG]OPENAIR RRC INIT CH %d...\n",Mod_id);

  Mac_config_req.Lchan_type = smsg->Lchan_desc_srb0.Lchan_t;
  memcpy(&Mac_config_req.Lchan_desc[0],(LCHAN_DESC*)&smsg->Lchan_desc_srb0,LCHAN_DESC_SIZE); //0 rx, 1 tx
  memcpy(&Mac_config_req.Lchan_desc[1],(LCHAN_DESC*)&smsg->Lchan_desc_srb0,LCHAN_DESC_SIZE); //0 rx, 1 tx
  Mac_config_req.UE_CH_index=0;
  Mac_config_req.Lchan_id.Index=(0 << RAB_SHIFT2) + BCCH;
  msg("Calling Lchan_config\n");
  Index=Mac_rlc_xface->mac_config_req(Mod_id,ADD_LC,&Mac_config_req);
  msg("[OPENAIR][RRC][RRC_ON] NODE %d, Config BCCH %d done\n",CH_rrc_inst[Mod_id].Node_id,Index);
  CH_rrc_inst[Mod_id].Srb0.Srb_id = Index;

  memcpy(&CH_rrc_inst[Mod_id].Srb0.Lchan_desc[0],(LCHAN_DESC*)&smsg->Lchan_desc_srb0,LCHAN_DESC_SIZE); //0 rx, 1 tx
  memcpy(&CH_rrc_inst[Mod_id].Srb0.Lchan_desc[1],(LCHAN_DESC*)&smsg->Lchan_desc_srb0,LCHAN_DESC_SIZE); //0 rx, 1 tx

  rrc_config_buffer(&CH_rrc_inst[Mod_id].Srb0,BCCH,0);
  ((CH_BCCH_HEADER*)(&CH_rrc_inst[Mod_id].Srb0.Tx_buffer.Header[0]))->Rv_tb_idx=0;
  msg("[OPENAIR][RRC][RRC_ON] NODE %d, Config BCCH for TB_size %d\n",NODE_ID[Mod_id],
      CH_rrc_inst[Mod_id].Srb0.Lchan_desc[1].transport_block_size);
  CH_rrc_inst[Mod_id].Srb0.Tx_buffer.generate_fun=ch_rrc_generate_bcch;
  CH_rrc_inst[Mod_id].Srb0.Active=1;

  Mac_config_req.Lchan_type = smsg->Lchan_desc_srb1.Lchan_t;
  memcpy(&Mac_config_req.Lchan_desc[0],(LCHAN_DESC*)&smsg->Lchan_desc_srb1,LCHAN_DESC_SIZE); //0 rx, 1 tx
  memcpy(&Mac_config_req.Lchan_desc[1],(LCHAN_DESC*)&smsg->Lchan_desc_srb1,LCHAN_DESC_SIZE); //0 rx, 1 tx
  Mac_config_req.UE_CH_index=0;
  Mac_config_req.Lchan_id.Index=(0 << RAB_SHIFT2) + CCCH;
  // printk("Calling Lchan_config\n");
  Index=Mac_rlc_xface->mac_config_req(Mod_id,ADD_LC,&Mac_config_req);
  msg("[OPENAIR][RRC][RRC_ON] NODE %d, Config BCCH %d done\n",CH_rrc_inst[Mod_id].Node_id,Index);
  CH_rrc_inst[Mod_id].Srb1.Srb_id = Index;

  memcpy(&CH_rrc_inst[Mod_id].Srb1.Lchan_desc[0],(LCHAN_DESC*)&smsg->Lchan_desc_srb1,LCHAN_DESC_SIZE); //0 rx, 1 tx
  memcpy(&CH_rrc_inst[Mod_id].Srb1.Lchan_desc[1],(LCHAN_DESC*)&smsg->Lchan_desc_srb1,LCHAN_DESC_SIZE); //0 rx, 1 tx


  rrc_config_buffer(&CH_rrc_inst[Mod_id].Srb1,CCCH,1);
  ((CH_CCCH_HEADER*)(&CH_rrc_inst[Mod_id].Srb1.Tx_buffer.Header[0]))->Rv_tb_idx=0;
  printk("[OPENAIR][RRC][RRC_ON] NODE %d, Config CCCH %d done, TB_size=%d,%d\n",NODE_ID[Mod_id],Index,
         CH_rrc_inst[Mod_id].Srb1.Tx_buffer.Tb_size,CH_rrc_inst[Mod_id].Srb1.Rx_buffer.Tb_size);
  CH_rrc_inst[Mod_id].Srb1.Tx_buffer.generate_fun=ch_rrc_generate_ccch;
  CH_rrc_inst[Mod_id].Srb1.Active=1;

  //CH_rrc_inst[Mod_id].Info.UE_list[i].L2_id[0]=i;

  //      CH_rrc_inst[Mod_id].Info.UE_list[0]=0;

}

void rrc_init_mr_req(unsigned char Mod_id, rrci_init_mr_req_t  *smsg)
{
  uint16_t Index;
  MAC_CONFIG_REQ Mac_config_req;
  // MAC_MEAS_REQ Mac_meas_req;
  msg("-----------------------------------------------------------------------------------------------------------------\n");
  msg("[RRC_CONFIG]: Node %d: INIT MR REQUEST for CH %d\n",NODE_ID[Mod_id],smsg->CH_index);
  Mac_config_req.Lchan_type = smsg->Lchan_desc_srb0.Lchan_t;
  memcpy(&Mac_config_req.Lchan_desc[0],(LCHAN_DESC*)&smsg->Lchan_desc_srb0,LCHAN_DESC_SIZE); //0 rx, 1 tx
  memcpy(&Mac_config_req.Lchan_desc[1],(LCHAN_DESC*)&smsg->Lchan_desc_srb0,LCHAN_DESC_SIZE); //0 rx, 1 tx

  Mac_config_req.UE_CH_index=smsg->CH_index;
  Mac_config_req.Lchan_id.Index=(smsg->CH_index << RAB_SHIFT2) + BCCH;
  Index=Mac_rlc_xface->mac_config_req(Mod_id,ADD_LC,&Mac_config_req);
  msg("[OPENAIR][RRC][RRC_CONFIG] NODE %d, Config BCCH %d done\n",UE_rrc_inst[Mod_id-NB_CH_INST].Node_id,Index);

  UE_rrc_inst[Mod_id].Srb0[smsg->CH_index].Srb_id = Index;
  memcpy(& UE_rrc_inst[Mod_id-NB_CH_INST].Srb0[smsg->CH_index].Lchan_desc[0],(LCHAN_DESC*)&smsg->Lchan_desc_srb0,LCHAN_DESC_SIZE); //0 rx, 1 tx
  memcpy(& UE_rrc_inst[Mod_id-NB_CH_INST].Srb0[smsg->CH_index].Lchan_desc[1],(LCHAN_DESC*)&smsg->Lchan_desc_srb0,LCHAN_DESC_SIZE); //0 rx, 1 tx

  rrc_config_buffer(&UE_rrc_inst[Mod_id-NB_CH_INST].Srb0[smsg->CH_index],BCCH,0);
  ((CH_BCCH_HEADER*)(&UE_rrc_inst[Mod_id-NB_CH_INST].Srb0[smsg->CH_index].Rx_buffer.Header[0]))->Rv_tb_idx=0;
  UE_rrc_inst[Mod_id-NB_CH_INST].Srb0[smsg->CH_index].Active=1;
  /*
      Mac_meas_req.Lchan_id.Index = Index;
      Mac_meas_req.UE_CH_index = i;
      Mac_meas_req.Meas_trigger = BCCH_MEAS_TRIGGER;
      Mac_meas_req.Mac_avg = BCCH_MEAS_AVG;
      Mac_meas_req.Rep_amount = 0;
      Mac_meas_req.Rep_interval = 1000;
      UE_rrc_inst[Mod_id].Srb0[i].Meas_entry=Mac_rlc_xface->mac_meas_req(Mod_id+NB_CH_INST,&Mac_meas_req);
      UE_rrc_inst[Mod_id].Srb0[i].Meas_entry->Status=RADIO_CONFIG_OK;
      UE_rrc_inst[Mod_id].Srb0[i].Meas_entry->Last_report_frame=Rrc_xface->Frame_index;
      UE_rrc_inst[Mod_id].Srb0[i].Meas_entry->Next_check_frame=Rrc_xface->Frame_index+1000;
  */

  //printk("[OPENAIR][RRC][RRC_ON] NODE %d, Config CCCH %d done\n",NODE_ID[Mod_id],Index);
  //      printk("check meas, LC_Index %d, Next %d, Last %d, Int %d \n",UE_rrc_inst[Mod_id].Srb0[i].Meas_entry->Mac_meas_req.Lchan_id.Index,UE_rrc_inst[Mod_id].Srb0[i].Meas_entry->Next_check_frame,UE_rrc_inst[Mod_id].Srb0[i].Meas_entry->Last_report_frame,UE_rrc_inst[Mod_id].Srb0[i].Meas_entry->Mac_meas_req.Rep_interval);

  Mac_config_req.Lchan_type = smsg->Lchan_desc_srb1.Lchan_t;
  memcpy(&Mac_config_req.Lchan_desc[0],(LCHAN_DESC*)&smsg->Lchan_desc_srb1,LCHAN_DESC_SIZE); //0 rx, 1 tx
  memcpy(&Mac_config_req.Lchan_desc[1],(LCHAN_DESC*)&smsg->Lchan_desc_srb1,LCHAN_DESC_SIZE); //0 rx, 1 tx

  Mac_config_req.UE_CH_index=smsg->CH_index;
  Mac_config_req.Lchan_id.Index=(smsg->CH_index << RAB_SHIFT2) + CCCH;
  Index=Mac_rlc_xface->mac_config_req(Mod_id,ADD_LC,&Mac_config_req);
  msg("[OPENAIR][RRC][RRC_CONFIG] NODE %d, Config CCCH %d done\n",NODE_ID[Mod_id],Index);
  UE_rrc_inst[Mod_id-NB_CH_INST].Srb1[smsg->CH_index].Srb_id = Index;
  memcpy(&UE_rrc_inst[Mod_id-NB_CH_INST].Srb1[smsg->CH_index].Lchan_desc[0],(LCHAN_DESC*)&smsg->Lchan_desc_srb1,LCHAN_DESC_SIZE); //0 rx, 1 tx
  memcpy(&UE_rrc_inst[Mod_id-NB_CH_INST].Srb1[smsg->CH_index].Lchan_desc[1],(LCHAN_DESC*)&smsg->Lchan_desc_srb1,LCHAN_DESC_SIZE); //0 rx, 1 tx
  rrc_config_buffer(&UE_rrc_inst[Mod_id-NB_CH_INST].Srb1[smsg->CH_index],CCCH,1);
  ((CH_CCCH_HEADER*)(&UE_rrc_inst[Mod_id-NB_CH_INST].Srb1[smsg->CH_index].Rx_buffer.Header[0]))->Rv_tb_idx=0;
  UE_rrc_inst[Mod_id-NB_CH_INST].Srb1[smsg->CH_index].Active=1;
  /*

  Mac_meas_req.Lchan_id.Index = Index;
  //    Mac_meas_req.UE_CH_index = i;
  Mac_meas_req.Meas_trigger = CCCH_MEAS_TRIGGER;
  Mac_meas_req.Mac_avg = CCCH_MEAS_AVG;
  Mac_meas_req.Rep_amount = 0;
  Mac_meas_req.Rep_interval = 1000;
  UE_rrc_inst[Mod_id].Srb1[i].Meas_entry=Mac_rlc_xface->mac_meas_req(Mod_id+NB_CH_INST,&Mac_meas_req);
  UE_rrc_inst[Mod_id].Srb1[i].Meas_entry->Status=RADIO_CONFIG_OK;
  UE_rrc_inst[Mod_id].Srb1[i].Meas_entry->Last_report_frame=Rrc_xface->Frame_index;
  UE_rrc_inst[Mod_id].Srb1[i].Meas_entry->Next_check_frame=Rrc_xface->Frame_index+1000;
  //printk("[OPENAIR][RRC][RRC_ON] NODE %d, Config CCCH %d done\n",NODE_ID[Mod_id],Index);
  */


}




void rrc_config_req(Instance_t Mod_id, void *smsg, unsigned char Action,Transaction_t Trans_id)
{

  MAC_CONFIG_REQ Mac_config_req;
  MAC_MEAS_REQ Mac_meas_req;
  unsigned short Idx,UE_index,In_idx;


  switch(Action) {

  case RRM_RB_ESTABLISH_REQ: {
    rrm_rb_establish_req_t *p = (rrm_rb_establish_req_t *) smsg ;
    msg("[RRC]CH %d: config req for CH L2_id %d to MR L2_id %d\n",NODE_ID[Mod_id],p->L2_id[0].L2_id[0],p->L2_id[1].L2_id[0]);
    UE_index=rrc_find_ue_index(Mod_id, p->L2_id[1]);

    if(UE_index > NB_CNX_CH) {
      msg("[FATAL ERROR] NO MORE UE_INDEX!!!!!\n");
      return;
    }

    if(p->Lchan_desc.Lchan_t==DCCH) {
      Mac_config_req.Lchan_type = p->Lchan_desc.Lchan_t;
      Mac_config_req.UE_CH_index = UE_index;
      memcpy(&Mac_config_req.Lchan_desc[0],(LCHAN_DESC*)&p->Lchan_desc,LCHAN_DESC_SIZE); //0 rx, 1 tx
      memcpy(&Mac_config_req.Lchan_desc[1],(LCHAN_DESC*)&p->Lchan_desc,LCHAN_DESC_SIZE); //0 rx, 1 tx
      Mac_config_req.Lchan_id.Index=( UE_index << RAB_SHIFT2) + DCCH;
      Idx = Mac_rlc_xface->mac_config_req(Mod_id,ADD_LC,&Mac_config_req);
      CH_rrc_inst[Mod_id].Srb2[UE_index].Active = 1;
      CH_rrc_inst[Mod_id].Srb2[UE_index].Next_check_frame = Rrc_xface->Frame_index + 250;
      CH_rrc_inst[Mod_id].Srb2[UE_index].Status = NEED_RADIO_CONFIG;//RADIO CFG
      CH_rrc_inst[Mod_id].Srb2[UE_index].Srb_info.Srb_id = Idx;


      memcpy(&CH_rrc_inst[Mod_id].Srb2[UE_index].Srb_info.Lchan_desc[0],(LCHAN_DESC*)&p->Lchan_desc,LCHAN_DESC_SIZE); //0 rx, 1 tx
      memcpy(&CH_rrc_inst[Mod_id].Srb2[UE_index].Srb_info.Lchan_desc[1],(LCHAN_DESC*)&p->Lchan_desc,LCHAN_DESC_SIZE); //0 rx, 1 tx
      //      CH_rrc_inst[Mod_id].Srb2[UE_index].Srb_info.Lchan_desc[0] = &DCCH_LCHAN_DESC;
      //      CH_rrc_inst[Mod_id].Srb2[UE_index].Srb_info.Lchan_desc[1] = &DCCH_LCHAN_DESC;

      //CH_rrc_inst[Mod_id].Rab[In_idx][UE_index].Rb_info.Lchan_desc[0] = &msg->Lchan_desc;
      // CH_rrc_inst[Mod_id].Rab[In_idx][UE_index].Rb_info.Lchan_desc[1] = &msg-;
      //Configure a correponding measurement process
      //  msg("[RRC]Inst %d: Programing RADIO CONFIG of DCCH LCHAN %d\n",Mod_id,Idx);
      //CH_rrc_inst[Mod_id].Nb_rb[UE_index]++;
      //msg("[OPENAIR][RRC] CALLING RLC CONFIG RADIO BEARER %d\n",Idx);
      Mac_rlc_xface->rrc_rlc_config_req(Mod_id,CONFIG_ACTION_ADD,Idx,SIGNALLING_RADIO_BEARER,Rlc_info_um);
      /*
      CH_rrc_inst[Mod_id].Srb2[UE_index].Srb_info.CH_ip_addr_type=p->L3_info_t;
      if(p->L3_info_t == IPv4_ADDR){

        memcpy(CH_rrc_inst[Mod_id].Srb2[UE_index].Srb_info.CH_ip_addr,p->L3_info,4);
      }
      else
        memcpy(UE_rrc_inst[Mod_id].Srb2[UE_index].Srb_info.CH_ip_addr,p->L3_info,16);
      */




      //      if(msg->Mac_rlc_meas_desc !=NULL){
      CH_rrc_inst[Mod_id].Def_meas[UE_index]= &CH_mac_inst[Mod_id].Def_meas[UE_index];
      CH_rrc_inst[Mod_id].Def_meas[UE_index]->Active = 1;
      CH_rrc_inst[Mod_id].Def_meas[UE_index]->Status = NEED_RADIO_CONFIG;
      CH_rrc_inst[Mod_id].Def_meas[UE_index]->Forg_fact=1;
      CH_rrc_inst[Mod_id].Def_meas[UE_index]->Rep_interval=50;
      CH_rrc_inst[Mod_id].Def_meas[UE_index]->Last_report_frame=Rrc_xface->Frame_index;
      CH_rrc_inst[Mod_id].Def_meas[UE_index]->Next_check_frame=Rrc_xface->Frame_index +200 ;
      // }
    } else {
      msg("RRC_config: Inst %d:, UE_index %d",Mod_id,UE_index);
      Mac_config_req.Lchan_type = p->Lchan_desc.Lchan_t;
      Mac_config_req.UE_CH_index = UE_index;
      memcpy(&Mac_config_req.Lchan_desc[0],&p->Lchan_desc,LCHAN_DESC_SIZE); //0 rx, 1 tx
      memcpy(&Mac_config_req.Lchan_desc[1],&p->Lchan_desc,LCHAN_DESC_SIZE); //0 rx, 1 tx
      In_idx=find_free_dtch_position(Mod_id,UE_index);
      Mac_config_req.Lchan_id.Index=(UE_index << RAB_SHIFT2) + DTCH + In_idx;
      Idx = Mac_rlc_xface->mac_config_req(Mod_id,ADD_LC,&Mac_config_req);
      CH_rrc_inst[Mod_id].Rab[In_idx][UE_index].Active = 1;
      CH_rrc_inst[Mod_id].Rab[In_idx][UE_index].Next_check_frame = Rrc_xface->Frame_index + 250;
      CH_rrc_inst[Mod_id].Rab[In_idx][UE_index].Status = NEED_RADIO_CONFIG;//RADIO CFG
      CH_rrc_inst[Mod_id].Rab[In_idx][UE_index].Rb_info.Rb_id = Idx;

      memcpy(&CH_rrc_inst[Mod_id].Rab[In_idx][UE_index].Rb_info.Lchan_desc[0],&p->Lchan_desc,LCHAN_DESC_SIZE); //0 rx, 1 tx
      memcpy(&CH_rrc_inst[Mod_id].Rab[In_idx][UE_index].Rb_info.Lchan_desc[1],&p->Lchan_desc,LCHAN_DESC_SIZE); //0 rx, 1 tx

      //Configure a correponding measurement process
      msg("[RRC]Inst %d: Programing RADIO CONFIG of DTCH LCHAN %d\n",Mod_id,Idx);
      //CH_rrc_inst[Mod_id].Nb_rb[UE_index]++;
      //msg("[OPENAIR][RRC] CALLING RLC CONFIG RADIO BEARER %d\n",Idx);

      //  if(p->Mac_rlc_meas_desc !=NULL){
      if(p->Lchan_desc.Lchan_t!=DTCH) {
        Mac_meas_req.Lchan_id.Index = Idx;
        Mac_meas_req.UE_CH_index = UE_index;
        Mac_meas_req.Meas_trigger = p->Mac_rlc_meas_desc.Meas_trigger;
        Mac_meas_req.Mac_avg = p->Mac_rlc_meas_desc.Mac_avg;
        Mac_meas_req.Rep_amount = p->Mac_rlc_meas_desc.Rep_amount;
        Mac_meas_req.Rep_interval = p->Mac_rlc_meas_desc.Rep_interval;
        CH_rrc_inst[Mod_id].Rab[In_idx][UE_index].Rb_info.Meas_entry=Mac_rlc_xface->mac_meas_req(Mod_id,&Mac_meas_req);
        CH_rrc_inst[Mod_id].Rab_meas[In_idx][UE_index].Status=NEED_RADIO_CONFIG;
        CH_rrc_inst[Mod_id].Rab_meas[In_idx][UE_index].Mac_meas_req.Lchan_id.Index=Idx;
        CH_rrc_inst[Mod_id].Rab_meas[In_idx][UE_index].Last_report_frame=Rrc_xface->Frame_index;
        CH_rrc_inst[Mod_id].Rab_meas[In_idx][UE_index].Next_check_frame=Rrc_xface->Frame_index+Mac_meas_req.Rep_interval;
      }


      if(p->Lchan_desc.Lchan_t==DTCH) {
        CH_rrc_inst[Mod_id].Rab[In_idx][UE_index].Status = RADIO_CONFIG_OK;//RADIO CFG
        Mac_rlc_xface->rrc_rlc_config_req(Mod_id,CONFIG_ACTION_ADD,Idx,RADIO_ACCESS_BEARER,Rlc_info_um);
        CH_rrc_inst[Mod_id].IP_addr_type = p->L3_info_t;

        if(CH_rrc_inst[Mod_id].IP_addr_type == IPv4_ADDR) {
          memcpy(CH_rrc_inst[Mod_id].IP_addr,p->L3_info,4);
        } else {
          memcpy(CH_rrc_inst[Mod_id].IP_addr,p->L3_info,16);
        }
      } else {
        Mac_rlc_xface->rrc_rlc_config_req(Mod_id,CONFIG_ACTION_ADD,Idx,RADIO_ACCESS_BEARER,Rlc_info_am_config);
      }
    }

    if(p->Lchan_desc.Lchan_t==DTCH) {
      send_msg(&S_rrc,msg_rrc_rb_establish_cfm(Mod_id,Idx,0,Trans_id));
    } else {
      send_msg(&S_rrc,msg_rrc_rb_establish_cfm(Mod_id,Idx,1,Trans_id));
    }
  }
  break;


  case RRM_RB_MODIFY_REQ: {
    //rrm_rb_modify_req_t *p = (rrm_rb_modify_req_t *) msg ;
  }

  case RRM_RB_RELEASE_REQ: {
    //rrm_rb_release_req_t *p = (rrm_rb_release_req_t *) msg ;
  }
  }



}
