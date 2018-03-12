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

/*________________________mac_phy_primitives.c________________________

 Authors : Hicham Anouar, Raymond Knopp
 Company : EURECOM
 Emails  : anouar@eurecom.fr,  knopp@eurecom.fr
________________________________________________________________*/


//#include "openair_extern.h"

#ifdef MAC_CONTEXT
#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/extern.h"
//#include "extern.h"
#include "defs.h"
#endif //MAC_CONTEXT


//#define DEBUG_UE_DECODE_SACH
//#define DEBUG_NODEB_DECODE_SACH

#ifdef PHY_CONTEXT
#ifdef PHY_EMUL
#include "extern.h"
#include "SIMULATION/simulation_defs.h"
#else //PHY_EMUL
#include "MAC_INTERFACE/extern.h"
#endif //PHY_EMUL

void clear_macphy_data_req(unsigned char Mod_id)
{
  //msg("CLEAR DATA_REQ\n");
  unsigned char i;

  Macphy_req_table[Mod_id].Macphy_req_cnt = 0;

  for (i=0; i<NB_REQ_MAX; i++)
    Macphy_req_table[Mod_id].Macphy_req_table_entry[i].Active = 0;
}

/*
unsigned char phy_resources_compare(PHY_RESOURCES *Phy1,PHY_RESOURCES* Phy2 ){

  if(Phy1->Time_alloc==Phy2->Time_alloc && Phy1->Freq_alloc==Phy2->Freq_alloc)// && Phy1->Coding_fmt==Phy2->Coding_fmt && Phy1->Seq_index==Phy2->Seq_index)
    return 1;
  else
    return 0;

}
*/

MACPHY_DATA_REQ_TABLE_ENTRY* find_data_req_entry(unsigned char Mod_id,MACPHY_REQ_ENTRY_KEY *Search_key)
{

  unsigned char i;

  //msg("[MAC_PHY]MAC_PHY_REQUEST_CNT=%d\n",Macphy_req_table.Macphy_req_cnt);
  if (Macphy_req_table[Mod_id].Macphy_req_cnt > 0) {
#ifdef DEBUG_PHY
    //    msg("[MACPHY_FIND_REQ] SEARCH KEY=%d\n",Search_key->Key_type);
#endif //DEBUG_PHY
    //msg("[MACPHY_FIND_REQ] SEARCH KEY=%d, NB_REQ_MAX=%d\n",Search_key->Key_type,NB_REQ_MAX);

    switch(Search_key->Key_type) {
    case PDU_TYPE_KEY:
      for(i=0; i<NB_REQ_MAX; i++) {
        if ( (Macphy_req_table[Mod_id].Macphy_req_table_entry[i].Macphy_data_req.Pdu_type==Search_key->Key.Pdu_type) &&
             (Macphy_req_table[Mod_id].Macphy_req_table_entry[i].Active == 1) ) {
          //msg("[MACPHY_FIND] MACPHY_req_table_entry=%p,idx=%d,Phy_resources %p",       &Macphy_req_table.Macphy_req_table_entry[i],i,Macphy_req_table.Macphy_req_table_entry[i].Macphy_data_req.Phy_Resources_Entry);
          return(&Macphy_req_table[Mod_id].Macphy_req_table_entry[i]);
        }
      }

      break;
      /*
      case LCHAN_KEY:
      for(i=0;i<NB_REQ_MAX;i++){
      if ((Macphy_req_table[Mod_id].Macphy_req_table_entry[i].Macphy_data_req.Lchan_id.Index==Search_key->Key.Lchan_id->Index) &&
      (Macphy_req_table[Mod_id].Macphy_req_table_entry[i].Active == 1) )
      return(&Macphy_req_table[Mod_id].Macphy_req_table_entry[i]);
      }
      break;

      case PHY_RESOURCES_KEY:
      for(i=0;i<NB_REQ_MAX;i++){
      if(Macphy_req_table[Mod_id].Macphy_req_table_entry[i].Active == 1)
      if ( ( Macphy_req_table[Mod_id].Macphy_req_table_entry[i].Macphy_data_req.Phy_resources->Time_alloc ==
        Search_key->Key.Phy_resources.Time_alloc )
         &&( Macphy_req_table[Mod_id].Macphy_req_table_entry[i].Macphy_data_req.Phy_resources->Freq_alloc ==
       Search_key->Key.Phy_resources.Freq_alloc )
         &&( Macphy_req_table[Mod_id].Macphy_req_table_entry[i].Macphy_data_req.CH_index ==
      Search_key->CH_index )
         && ( Macphy_req_table[Mod_id].Macphy_req_table_entry[i].Macphy_data_req.Direction == RX))
      return(&Macphy_req_table[Mod_id].Macphy_req_table_entry[i]);
      }
      break;
      */
    }
  }

#ifndef PHY_EMUL
  //  msg("[PHY][PHY_MAC] Frame %d : No data request\n",mac_xface->frame);
#endif //PHY_EMUL


  return (MACPHY_DATA_REQ_TABLE_ENTRY*)0;

}






void print_active_requests(unsigned char Mod_id)
{

  int i;
  msg("_________________________INST %d , FRAME %d ACTIVE_REQUESTS_________________\n",Mod_id,mac_xface->frame);

  for (i=0; i<NB_REQ_MAX; i++) {

    if (Macphy_req_table[Mod_id].Macphy_req_table_entry[i].Active == 1) {
      msg("[MACPHY][DATA][REQ] Request %d: Direction %d, Pdu_type %d\n",
          i,
          Macphy_req_table[Mod_id].Macphy_req_table_entry[i].Macphy_data_req.Direction,
          Macphy_req_table[Mod_id].Macphy_req_table_entry[i].Macphy_data_req.Pdu_type);
      //    Macphy_req_table[Mod_id].Macphy_req_table_entry[i].Macphy_data_req.Lchan_id.Index);
      //    Macphy_req_table[Mod_id].Macphy_req_table_entry[i].Macphy_data_req.Phy_resources,
      //    Macphy_req_table[Mod_id].Macphy_req_table_entry[i].Macphy_data_req.Phy_resources->Time_alloc,
      //    Macphy_req_table[Mod_id].Macphy_req_table_entry[i].Macphy_data_req.Phy_resources->Freq_alloc);
      //if(Macphy_req_table[Mod_id].Macphy_req_table_entry[i].Macphy_data_req.Pdu_type==RACH)
      //msg("[RACH_REQ] Rach_pdu %p, Payload %p\n",Macphy_req_table[Mod_id].Macphy_req_table_entry[i].Macphy_data_req.Dir.Req_rx.Pdu.Rach_pdu,
      //    Macphy_req_table[Mod_id].Macphy_req_table_entry[i].Macphy_data_req.Dir.Req_rx.Pdu.Rach_pdu->Rach_payload);

    }
  }
}


/*___________________________________________________________________________________________________*/
#define RCNT Macphy_req_table[Mod_id].Macphy_req_cnt

MACPHY_DATA_REQ *new_macphy_data_req(unsigned char Mod_id)
{
  /*___________________________________________________________________________________________________*/
  unsigned char i;



  for (i=0; i<NB_REQ_MAX; i++) {
    if (Macphy_req_table[Mod_id].Macphy_req_table_entry[(i)%NB_REQ_MAX].Active == 0) {
      Macphy_req_table[Mod_id].Macphy_req_table_entry[(i)%NB_REQ_MAX].Active = 1;
      RCNT = (RCNT + 1)%NB_REQ_MAX;
      //            msg("[MAC_PHY]NEW MAC_REQUEST_CNT=%d,frame %d, Module %d, entry %d \n",Macphy_req_table[Mod_id].Macphy_req_cnt,mac_xface->frame,Mod_id,i);
      //        Macphy_req_table[Mod_id].Macphy_req_table_entry[i%NB_REQ_MAX].Macphy_data_req.Phy_resources=(PHY_RESOURCES*)malloc16(sizeof(PHY_RESOURCES));
      return(&Macphy_req_table[Mod_id].Macphy_req_table_entry[i%NB_REQ_MAX].Macphy_data_req);
    }
  }

  msg("[OPENAIR][MAC][ERROR] frame %d: No more DATA_REQ !!!!\n",mac_xface->frame);

  print_active_requests(Mod_id);
  mac_xface->macphy_exit("new_macphy_data_req: no more DATA_REQ");
  //rt_sleep(nano2count(2000));

  return((MACPHY_DATA_REQ*)0);
}


#endif //PHY_CONTEXT

#ifdef MAC_CONTEXT
#include "LAYER2/MAC/extern.h"

// Function called by PHY to indicate available data/measurements for MAC

/*___________________________________________________________________________________________________*/
void macphy_data_ind(unsigned char Mod_id,unsigned char Pdu_type,void *pdu,unsigned short rnti)
{
  /*___________________________________________________________________________________________________*/
  //msg("[OPENAIR][MACPHY] Calling mac_resp In\n");

  int i;

  //  if (Req_rx->crc_status[0]!= -1) {  //CRC_STATUS

  // msg("[OPENAIR][MACPHY] Calling mac_indicate In\n");
  //     Req_rx->Meas.UL_meas=&UL_meas[Mod_id];
  //   Req_rx->Meas.DL_meas=&DL_meas[Mod_id];

  switch (Pdu_type) {
  case ULSCH:

    //        msg("[OPENAIR][MACPHY] Received RACH, Sending to MAC\n");
    nodeb_decode_ulsch(Mod_id,(ULSCH_PDU *)pdu,rnti);

    break;

  case DLSCH:
#ifdef DEBUG_UE_DECODE_SACH
    msg("[MAC][UE][MAC_PHY] TTI %d Inst %d\n",mac_xface->frame,Mod_id);
#endif

    //      ue_decode_dlsch(Mod_id-NB_CH_INST,
    //          (DLSCH_PDU *)pdu,rnti);

    break;

  default:
    break;
  }

  //    msg("Freeing Req %p\n",Macphy_data_req_table_entry);
  //  }

}





/*PHY_RESOURCES_TABLE_ENTRY *new_phy_resources() {


  unsigned char i;

  //msg("[OPENAIR][PHY][MAC Interface] New Phy Resource, cnt %d\n",Phy_resources_table.Phy_resources_cnt);

  for (i=0;i<NB_PHY_RESOURCES_MAX;i++){

    if (Phy_resources_table.Phy_resources_table_entry[(i+Phy_resources_table.Phy_resources_cnt+1)%NB_PHY_RESOURCES_MAX].Active == 0) {
      Phy_resources_table.Phy_resources_table_entry[(i+Phy_resources_table.Phy_resources_cnt+1)%NB_PHY_RESOURCES_MAX].Active = 1;
      Phy_resources_table.Phy_resources_cnt = (Phy_resources_table.Phy_resources_cnt + 1)%NB_PHY_RESOURCES_MAX;
      //  msg("[OPENAIR][PHY][MAC Interface] NEW PHY_RESOURCES: Taking index %d\n\n",(i+Phy_resources_table.Phy_resources_cnt+1)%NB_PHY_RESOURCES_MAX);

      return(&Phy_resources_table.Phy_resources_table_entry[(i+Phy_resources_table.Phy_resources_cnt)%NB_PHY_RESOURCES_MAX]);
    }
  }
  msg("[OPENAIR][MAC][ERROR] No more PHY_RESOURCES !!!!\n");
  exit(-1);
}
*/

#endif //MAC_CONTEXT


// Measurements, etc ..

//short phy_resource_cnt = 0, macphy_data_req_cnt = 0, macphy_data_ind_cnt = 0;












