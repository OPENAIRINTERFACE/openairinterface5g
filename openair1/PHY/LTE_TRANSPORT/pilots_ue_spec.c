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

/*! \file PHY/LTE_TRANSPORT/uespec_pilots.c
* \brief Top-level routines for generating DL ue-specific reference signals V12.5 2015-03
* \author X.JIANG
* \date 2011
* \version 0.1
* \company Eurecom
* \email: xiwen.jiangeurecom.fr
* \note
* \warning
*/
//#include "defs.h"
#include "PHY/defs.h"

void generate_ue_spec_pilots(PHY_VARS_eNB *phy_vars_eNB,
                     uint8_t UE_id,
                     int32_t **txdataF,
                     int16_t amp,
                     uint16_t Ntti,
		     uint8_t beamforming_mode)
{

  /*LTE_DL_FRAME_PARMS *frame_parms = &phy_vars_eNB->lte_frame_parms;

  uint32_t tti,tti_offset,slot_offset,Nsymb,samples_per_symbol;
  uint8_t second_pilot,aa;

  //  printf("Doing TX pilots Nsymb %d, second_pilot %d\n",Nsymb,second_pilot);
  
  switch(beamforming_mode){
  case 7:
    for (tti=0; tti<Ntti; tti++) {

      tti_offset = tti*frame_parms->ofdm_symbol_size*Nsymb;
      samples_per_symbol = frame_parms->ofdm_symbol_size;
      slot_offset = (tti*2)%20;

      //    printf("tti %d : offset %d (slot %d)\n",tti,tti_offset,slot_offset);
      //Generate UE specific Pilots
      printf("generate_dl_ue_spec:tti_offset=%d\n",tti_offset);

      if(frame_parms->Ncp==0) {
        for(aa=0;aa<frame_parms->nb_antennas_tx;aa++){ 
          //antenna port 5 symbol 0 slot 0
          lte_dl_ue_spec(phy_vars_eNB,
                         UE_id,
                         &txdataF[aa][tti_offset+3*samples_per_symbol],
                         amp,
                         slot_offset,
              	         1,
                         5,
                         0);

          //antenna port 5 symbol 1 slot 0
          lte_dl_ue_spec(phy_vars_eNB,
                         UE_id,
                         &txdataF[aa][tti_offset+6*samples_per_symbol],
                         amp,
                         slot_offset,
            	         1,
                         5,
                         0);

          //antenna port 5 symbol 0 slot 1
          lte_dl_ue_spec(phy_vars_eNB,
                         UE_id,
                         &txdataF[aa][tti_offset+9*samples_per_symbol],
                         amp,
                         slot_offset+1,
            	         0,
                         5,
                         0);

          //antenna port 5 symbol 1 slot 1
          lte_dl_ue_spec(phy_vars_eNB,
                         UE_id,
                         &txdataF[aa][tti_offset+12*samples_per_symbol],
                         amp,
                         slot_offset+1,
            	         1,
                         5,
                         0);
	}
      } else{
      	msg("generate_ue_soec_pilots:Extented Cyclic Prefix for TM7 is not supported yet.\n");
      }


    }
    break;

  case 8:
  case 9:
  case 10:
  default:
    msg("[generate_ue_spec_pilots(in uespec_pilots.c)]ERROR:beamforming mode %d is not supported\n",beamforming_mode);
    
 }*/
}

/*int generate_ue_spec_pilots_slot(PHY_VARS_eNB *phy_vars_eNB,
                         int32_t **txdataF,
                         int16_t amp,
                         uint16_t slot,
                         int first_pilot_only)
{

  LTE_DL_FRAME_PARMS *frame_parms = &phy_vars_eNB->lte_frame_parms;
  uint32_t slot_offset,Nsymb,samples_per_symbol;
  uint8_t second_pilot;

  if (slot<0 || slot>= 20) {
    msg("generate_pilots_slot: slot not in range (%d)\n",slot);
    return(-1);
  }

  Nsymb = (frame_parms->Ncp==0)?7:6;
  second_pilot = (frame_parms->Ncp==0)?4:3;


  slot_offset = slot*frame_parms->ofdm_symbol_size*Nsymb;
  samples_per_symbol = frame_parms->ofdm_symbol_size;

  //    printf("tti %d : offset %d (slot %d)\n",tti,tti_offset,slot_offset);
  //Generate Pilots

  //antenna 0 symbol 0 slot 0
  lte_dl_cell_spec(phy_vars_eNB,
                   &txdataF[0][slot_offset],
                   amp,
                   slot,
                   0,
                   0);


  if (first_pilot_only==0) {
    //antenna 0 symbol 3 slot 0
    lte_dl_cell_spec(phy_vars_eNB,
                     &txdataF[0][slot_offset+(second_pilot*samples_per_symbol)],
                     amp,
                     slot,
                     1,
                     0);
  }

  if (frame_parms->nb_antennas_tx > 1) {
    if (frame_parms->mode1_flag) {
      // antenna 1 symbol 0 slot 0
      lte_dl_cell_spec(phy_vars_eNB,
                       &txdataF[1][slot_offset],
                       amp,
                       slot,
                       0,
                       0);

      if (first_pilot_only==0) {
        // antenna 1 symbol 3 slot 0
        lte_dl_cell_spec(phy_vars_eNB,
                         &txdataF[1][slot_offset+(second_pilot*samples_per_symbol)],
                         amp,
                         slot,
                         1,
                         0);
      }
    } else {

      // antenna 1 symbol 0 slot 0
      lte_dl_cell_spec(phy_vars_eNB,
                       &txdataF[1][slot_offset],
                       amp,
                       slot,
                       0,
                       1);

      if (first_pilot_only == 0) {
        // antenna 1 symbol 3 slot 0
        lte_dl_cell_spec(phy_vars_eNB,
                         &txdataF[1][slot_offset+(second_pilot*samples_per_symbol)],
                         amp,
                         slot,
                         1,
                         1);
      }
    }
  }

  return(0);
}*/

