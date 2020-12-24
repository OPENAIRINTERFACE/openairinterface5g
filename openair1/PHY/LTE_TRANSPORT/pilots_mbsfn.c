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

/*! \file PHY/LTE_TRANSPORT/pilots_mbsfn.c
* \brief Top-level routines for generating DL mbsfn reference signals / DL mbsfn reference signals for FeMBMS
* \authors S. Paranche, R. Knopp, J. Morgade
* \date 2012
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr, javier.morgade@ieee.org
* \note
* \warning
*/

#include "PHY/defs_eNB.h"
#include "PHY/LTE_REFSIG/lte_refsig.h"

int generate_mbsfn_pilot(PHY_VARS_eNB *eNB,
			 L1_rxtx_proc_t *proc,
                         int32_t **txdataF,
                         int16_t amp)

{

  LTE_DL_FRAME_PARMS *frame_parms = &eNB->frame_parms;
  uint32_t subframe_offset,Nsymb,samples_per_symbol;
  int subframe = proc->subframe_tx;


  if (subframe<0 || subframe>= 10) {
    LOG_E(PHY,"generate_mbsfn_pilots_subframe: subframe not in range (%d)\n",subframe);
    return(-1);
  }

  Nsymb = (frame_parms->Ncp==NORMAL) ? 7 : 6;

  subframe_offset = subframe*frame_parms->ofdm_symbol_size*Nsymb<<1;
  samples_per_symbol = frame_parms->ofdm_symbol_size;

  //    printf("tti %d : offset %d (slot %d)\n",tti,tti_offset,slot_offset);
  //Generate Pilots

  //antenna 4 symbol 2 Slot 0
  lte_dl_mbsfn(eNB,
               &txdataF[0][subframe_offset+(2*samples_per_symbol)],
               amp,
               subframe,
               0);



  //antenna 4 symbol 0 slot 1
  lte_dl_mbsfn(eNB,
               &txdataF[0][subframe_offset+(6*samples_per_symbol)],
               amp,
               subframe,
               1);

  //antenna 4 symbol 4 slot 1
  lte_dl_mbsfn(eNB,
               &txdataF[0][subframe_offset+(10*samples_per_symbol)],
               amp,
               subframe,
               2);

  return(0);
}


int generate_mbsfn_pilot_khz_1dot25(PHY_VARS_eNB *eNB,
                         L1_rxtx_proc_t *proc,
                         int32_t **txdataF,
                         int16_t amp)

{

  LTE_DL_FRAME_PARMS *frame_parms = &eNB->frame_parms;
  uint32_t subframe_offset,Nsymb;
  int subframe = proc->subframe_tx;


  if (subframe<0 || subframe>= 10) {
    LOG_E(PHY,"generate_mbsfn_pilots_subframe: subframe not in range (%d)\n",subframe);
    return(-1);
  }

  Nsymb = (frame_parms->Ncp==NORMAL) ? 7 : 6;

  subframe_offset = subframe*frame_parms->ofdm_symbol_size*Nsymb<<1;

  lte_dl_mbsfn_khz_1dot25(eNB,
                &txdataF[0][subframe_offset+0],
                amp,subframe);

  return(0);
}

