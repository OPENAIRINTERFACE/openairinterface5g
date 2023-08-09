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

/*! \file PHY/LTE_TRANSPORT/phich_common.c
* \brief Top-level routines for generating and decoding  the PHICH/HI physical/transport channel V8.6 2009-03
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr
* \note
* \warning
*/

#include "PHY/defs_eNB.h"


uint8_t get_mi(LTE_DL_FRAME_PARMS *frame_parms,uint8_t subframe)
{
  // for FeMBMS
  if(frame_parms->FeMBMS_active!=0){
	return(0);
  }

  // for FDD
  if (frame_parms->frame_type == FDD)
    return 1;

  // for TDD
  switch (frame_parms->tdd_config) {
    case 0:
      if ((subframe==0) || (subframe==5))
        return(2);
      else return(1);

      break;

    case 1:
      if ((subframe==0) || (subframe==5))
        return(0);
      else return(1);

      break;

    case 2:
      if ((subframe==3) || (subframe==8))
        return(1);
      else return(0);

      break;

    case 3:
      if ((subframe==0) || (subframe==8) || (subframe==9))
        return(1);
      else return(0);

      break;

    case 4:
      if ((subframe==8) || (subframe==9))
        return(1);
      else return(0);

      break;

    case 5:
      if (subframe==8)
        return(1);
      else return(0);

      break;

    case 6:
      return(1);
      break;

    default:
      return(0);
  }
}

unsigned char subframe2_ul_harq(LTE_DL_FRAME_PARMS *frame_parms,unsigned char subframe) {
  if (frame_parms->frame_type == FDD)
    return(subframe&7);

  switch (frame_parms->tdd_config) {
  case 1:
    if (subframe == 6) {
      return(0);
    } else if (subframe==9){
      return(1);
    } else if (subframe==1){
      return(2);
    } else if (subframe==4){
      return(3);
    } else {
      LOG_E(PHY,"phich.c: subframe2_ul_harq, illegal subframe %d for tdd_config %d\n",
            subframe,frame_parms->tdd_config);
      return(0);
    }

    break;
  case 2:
    if (subframe == 3) {
      return(1);
    } else if (subframe==8){
      return(0);
    } else {
      LOG_E(PHY,"phich.c: subframe2_ul_harq, illegal subframe %d for tdd_config %d\n",
            subframe,frame_parms->tdd_config);
      return(0);
    }

    break;
    case 3:
      if ( (subframe == 8) || (subframe == 9) ) {
        return(subframe-8);
      } else if (subframe==0)
        return(2);
      else {
        LOG_E(PHY,"phich.c: subframe2_ul_harq, illegal subframe %d for tdd_config %d\n",
              subframe,frame_parms->tdd_config);
        return(0);
      }

      break;

    case 4:
      if ( (subframe == 8) || (subframe == 9) ) {
        return(subframe-8);
      } else {
        LOG_E(PHY,"phich.c: subframe2_ul_harq, illegal subframe %d for tdd_config %d\n",
              subframe,frame_parms->tdd_config);
        return(0);
      }

      break;
  }

  return(0);
}

int phich_frame2_pusch_frame(LTE_DL_FRAME_PARMS *frame_parms, int frame, int subframe) {
  int pusch_frame;

  if (frame_parms->frame_type == FDD) {
    pusch_frame = subframe<4 ? frame + 1024 - 1 : frame;
  } else {
    // Note this is not true, but it doesn't matter, the frame number is irrelevant for TDD!
    pusch_frame = (frame);
  }

  //LOG_D(PHY, "frame %d subframe %d: PUSCH frame = %d\n", frame, subframe, pusch_frame);
  return pusch_frame % 1024;
}

uint8_t phich_subframe2_pusch_subframe(LTE_DL_FRAME_PARMS *frame_parms,uint8_t subframe) {
  uint8_t pusch_subframe = 255;

  if (frame_parms->frame_type == FDD)
    return subframe < 4 ? subframe + 6 : subframe - 4;

  switch (frame_parms->tdd_config) {
    case 0:
      if (subframe == 0)
        pusch_subframe = (3);
      else if (subframe == 5) {
        pusch_subframe = (8);
      } else if (subframe == 6)
        pusch_subframe = (2);
      else if (subframe == 1)
        pusch_subframe = (7);
      else {
        AssertFatal(1==0,"phich.c: phich_subframe2_pusch_subframe, illegal subframe %d for tdd_config %d\n",
                    subframe,frame_parms->tdd_config);
        pusch_subframe = (0);
      }

      break;

    case 1:
      if (subframe == 6)
        pusch_subframe = (2);
      else if (subframe == 9)
        pusch_subframe = (3);
      else if (subframe == 1)
        pusch_subframe = (7);
      else if (subframe == 4)
        pusch_subframe = (8);
      else {
        AssertFatal(1==0,"phich.c: phich_subframe2_pusch_subframe, illegal subframe %d for tdd_config %d\n",
                    subframe,frame_parms->tdd_config);
        pusch_subframe = (0);
      }

      break;

    case 2:
      if (subframe == 8)
        pusch_subframe = (2);
      else if (subframe == 3)
        pusch_subframe = (7);
      else {
        AssertFatal(1==0,"phich.c: phich_subframe2_pusch_subframe, illegal subframe %d for tdd_config %d\n",
                    subframe,frame_parms->tdd_config);
        pusch_subframe = (0);
      }

      break;

    case 3:
      if ( (subframe == 8) || (subframe == 9) ) {
        pusch_subframe = (subframe-6);
      } else if (subframe==0)
        pusch_subframe = (4);
      else {
        AssertFatal(1==0,"phich.c: phich_subframe2_pusch_subframe, illegal subframe %d for tdd_config %d\n",
                    subframe,frame_parms->tdd_config);
        pusch_subframe = (0);
      }

      break;

    case 4:
      if ( (subframe == 8) || (subframe == 9) ) {
        pusch_subframe = (subframe-6);
      } else {
        AssertFatal(1==0,"phich.c: phich_subframe2_pusch_subframe, illegal subframe %d for tdd_config %d\n",
                    subframe,frame_parms->tdd_config);
        pusch_subframe = (0);
      }

      break;

    case 5:
      if (subframe == 8) {
        pusch_subframe = (2);
      } else {
        AssertFatal(1==0,"phich.c: phich_subframe2_pusch_subframe, illegal subframe %d for tdd_config %d\n",
                    subframe,frame_parms->tdd_config);
        pusch_subframe = (0);
      }

      break;

    case 6:
      if (subframe == 6) {
        pusch_subframe = (2);
      } else if (subframe == 9) {
        pusch_subframe = (3);
      } else if (subframe == 0) {
        pusch_subframe = (4);
      } else if (subframe == 1) {
        pusch_subframe = (7);
      } else if (subframe == 5) {
        pusch_subframe = (8);
      } else {
        AssertFatal(1==0,"phich.c: phich_subframe2_pusch_subframe, illegal subframe %d for tdd_config %d\n",
                    subframe,frame_parms->tdd_config);
        pusch_subframe = (0);
      }

      break;

    default:
      AssertFatal(1==0, "no implementation for TDD UL/DL-config = %d!\n", frame_parms->tdd_config);
      pusch_subframe = (0);
  }

  LOG_D(PHY, "subframe  %d: PUSCH subframe = %d\n", subframe, pusch_subframe);
  return pusch_subframe;
}

int check_pcfich(LTE_DL_FRAME_PARMS *frame_parms,uint16_t reg) {
  if ((reg == frame_parms->pcfich_reg[0]) ||
      (reg == frame_parms->pcfich_reg[1]) ||
      (reg == frame_parms->pcfich_reg[2]) ||
      (reg == frame_parms->pcfich_reg[3]))
    return(1);

  return(0);
}

void generate_phich_reg_mapping(LTE_DL_FRAME_PARMS *frame_parms) {
  unsigned short n0 = (frame_parms->N_RB_DL * 2) - 4;  // 2 REG per RB less the 4 used by PCFICH in first symbol
  unsigned short n1 = (frame_parms->N_RB_DL * 3);      // 3 REG per RB in second and third symbol
  unsigned short n2 = n1;
  unsigned short mprime = 0;
  unsigned short Ngroup_PHICH;
  //  uint16_t *phich_reg = frame_parms->phich_reg;
  uint16_t *pcfich_reg = frame_parms->pcfich_reg;
  // compute Ngroup_PHICH (see formula at beginning of Section 6.9 in 36-211
  Ngroup_PHICH = (frame_parms->phich_config_common.phich_resource*frame_parms->N_RB_DL)/48;

  if (((frame_parms->phich_config_common.phich_resource*frame_parms->N_RB_DL)%48) > 0)
    Ngroup_PHICH++;

  // check if Extended prefix
  if (frame_parms->Ncp == 1) {
    Ngroup_PHICH<<=1;
  }

#ifdef DEBUG_PHICH
  LOG_D(PHY,
        "Ngroup_PHICH %d (phich_config_common.phich_resource %d,phich_config_common.phich_duration %s, NidCell %d,Ncp %d, frame_type %d), smallest pcfich REG %d, n0 %d, n1 %d (first PHICH REG %d)\n",
        ((frame_parms->Ncp == NORMAL)?Ngroup_PHICH:(Ngroup_PHICH>>1)),
        frame_parms->phich_config_common.phich_resource,
        frame_parms->phich_config_common.phich_duration==normal?"normal":"extended",
        frame_parms->Nid_cell,frame_parms->Ncp,frame_parms->frame_type,
        pcfich_reg[frame_parms->pcfich_first_reg_idx],
        n0,
        n1,
        ((frame_parms->Nid_cell))%n0);
#endif

  // This is the algorithm from Section 6.9.3 in 36-211, it works only for normal PHICH duration for now ...

  for (mprime=0;
       mprime<((frame_parms->Ncp == NORMAL)?Ngroup_PHICH:(Ngroup_PHICH>>1));
       mprime++) {
    if (frame_parms->phich_config_common.phich_duration==normal) { // normal PHICH duration
      frame_parms->phich_reg[mprime][0] = (frame_parms->Nid_cell + mprime)%n0;

      if (frame_parms->phich_reg[mprime][0]>=pcfich_reg[frame_parms->pcfich_first_reg_idx])
        frame_parms->phich_reg[mprime][0]++;

      if (frame_parms->phich_reg[mprime][0]>=pcfich_reg[(frame_parms->pcfich_first_reg_idx+1)&3])
        frame_parms->phich_reg[mprime][0]++;

      if (frame_parms->phich_reg[mprime][0]>=pcfich_reg[(frame_parms->pcfich_first_reg_idx+2)&3])
        frame_parms->phich_reg[mprime][0]++;

      if (frame_parms->phich_reg[mprime][0]>=pcfich_reg[(frame_parms->pcfich_first_reg_idx+3)&3])
        frame_parms->phich_reg[mprime][0]++;

      frame_parms->phich_reg[mprime][1] = (frame_parms->Nid_cell + mprime + (n0/3))%n0;

      if (frame_parms->phich_reg[mprime][1]>=pcfich_reg[frame_parms->pcfich_first_reg_idx])
        frame_parms->phich_reg[mprime][1]++;

      if (frame_parms->phich_reg[mprime][1]>=pcfich_reg[(frame_parms->pcfich_first_reg_idx+1)&3])
        frame_parms->phich_reg[mprime][1]++;

      if (frame_parms->phich_reg[mprime][1]>=pcfich_reg[(frame_parms->pcfich_first_reg_idx+2)&3])
        frame_parms->phich_reg[mprime][1]++;

      if (frame_parms->phich_reg[mprime][1]>=pcfich_reg[(frame_parms->pcfich_first_reg_idx+3)&3])
        frame_parms->phich_reg[mprime][1]++;

      frame_parms->phich_reg[mprime][2] = (frame_parms->Nid_cell + mprime + (2*n0/3))%n0;

      if (frame_parms->phich_reg[mprime][2]>=pcfich_reg[frame_parms->pcfich_first_reg_idx])
        frame_parms->phich_reg[mprime][2]++;

      if (frame_parms->phich_reg[mprime][2]>=pcfich_reg[(frame_parms->pcfich_first_reg_idx+1)&3])
        frame_parms->phich_reg[mprime][2]++;

      if (frame_parms->phich_reg[mprime][2]>=pcfich_reg[(frame_parms->pcfich_first_reg_idx+2)&3])
        frame_parms->phich_reg[mprime][2]++;

      if (frame_parms->phich_reg[mprime][2]>=pcfich_reg[(frame_parms->pcfich_first_reg_idx+3)&3])
        frame_parms->phich_reg[mprime][2]++;

#ifdef DEBUG_PHICH
      printf("phich_reg :%d => %d,%d,%d\n",mprime,frame_parms->phich_reg[mprime][0],frame_parms->phich_reg[mprime][1],frame_parms->phich_reg[mprime][2]);
#endif
    } else { // extended PHICH duration
      frame_parms->phich_reg[mprime<<1][0] = (frame_parms->Nid_cell + mprime)%n0;
      frame_parms->phich_reg[1+(mprime<<1)][0] = (frame_parms->Nid_cell + mprime)%n0;
      frame_parms->phich_reg[mprime<<1][1] = ((frame_parms->Nid_cell*n1/n0) + mprime + (n1/3))%n1;
      frame_parms->phich_reg[mprime<<1][2] = ((frame_parms->Nid_cell*n2/n0) + mprime + (2*n2/3))%n2;
      frame_parms->phich_reg[1+(mprime<<1)][1] = ((frame_parms->Nid_cell*n1/n0) + mprime + (n1/3))%n1;
      frame_parms->phich_reg[1+(mprime<<1)][2] = ((frame_parms->Nid_cell*n2/n0) + mprime + (2*n2/3))%n2;
      //#ifdef DEBUG_PHICH
      printf("phich_reg :%u => %d,%d,%d\n",mprime<<1,frame_parms->phich_reg[mprime<<1][0],frame_parms->phich_reg[mprime][1],frame_parms->phich_reg[mprime][2]);
      printf("phich_reg :%d => %d,%d,%d\n",1+(mprime<<1),frame_parms->phich_reg[1+(mprime<<1)][0],frame_parms->phich_reg[1+(mprime<<1)][1],frame_parms->phich_reg[1+(mprime<<1)][2]);
      //#endif
    }
  } // mprime loop
}  // num_pdcch_symbols loop
