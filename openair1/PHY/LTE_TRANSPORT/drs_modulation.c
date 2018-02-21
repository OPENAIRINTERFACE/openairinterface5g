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

/*! \file PHY/LTE_TRANSPORT/drs_modulation.c
* \brief Top-level routines for generating the Demodulation Reference Signals from 36-211, V8.6 2009-03
* \author R. Knopp, F. Kaltenberger, A. Bhamri
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr,ankit.bhamri@eurecom.fr
* \note
* \warning
*/
#include "PHY/defs.h"
#include "PHY/extern.h"
#include "PHY/sse_intrin.h"
//#define DEBUG_DRS

int generate_drs_pusch(PHY_VARS_UE *ue,
		       UE_rxtx_proc_t *proc,
                       uint8_t eNB_id,
                       short amp,
                       unsigned int subframe,
                       unsigned int first_rb,
                       unsigned int nb_rb,
                       uint8_t ant)
{

  uint16_t k,l,Msc_RS,Msc_RS_idx,rb,drs_offset;
  uint16_t * Msc_idx_ptr;
  int subframe_offset,re_offset,symbol_offset;

  //uint32_t phase_shift; // phase shift for cyclic delay in DM RS
  //uint8_t alpha_ind;

  int16_t alpha_re[12] = {32767, 28377, 16383,     0,-16384,  -28378,-32768,-28378,-16384,    -1, 16383, 28377};
  int16_t alpha_im[12] = {0,     16383, 28377, 32767, 28377,   16383,     0,-16384,-28378,-32768,-28378,-16384};

  uint8_t cyclic_shift,cyclic_shift0,cyclic_shift1;
  LTE_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  int32_t *txdataF = ue->common_vars.txdataF[ant];
  uint32_t u,v,alpha_ind;
  uint32_t u0=frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.grouphop[subframe<<1];
  uint32_t u1=frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.grouphop[1+(subframe<<1)];
  uint32_t v0=frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.seqhop[subframe<<1];
  uint32_t v1=frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.seqhop[1+(subframe<<1)];
  int32_t ref_re,ref_im;
  uint8_t harq_pid = subframe2harq_pid(frame_parms,proc->frame_tx,subframe);

  cyclic_shift0 = (frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift +
                   ue->ulsch[eNB_id]->harq_processes[harq_pid]->n_DMRS2 +
                   frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.nPRS[subframe<<1]+
                   ((ue->ulsch[0]->cooperation_flag==2)?10:0)+
                   ant*6) % 12;
  //  printf("PUSCH.cyclicShift %d, n_DMRS2 %d, nPRS %d\n",frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift,ue->ulsch[eNB_id]->n_DMRS2,ue->lte_frame_parms.pusch_config_common.ul_ReferenceSignalsPUSCH.nPRS[subframe<<1]);
  cyclic_shift1 = (frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift +
                   ue->ulsch[eNB_id]->harq_processes[harq_pid]->n_DMRS2 +
                   frame_parms->pusch_config_common.ul_ReferenceSignalsPUSCH.nPRS[(subframe<<1)+1]+
                   ((ue->ulsch[0]->cooperation_flag==2)?10:0)+
                   ant*6) % 12;

  //       cyclic_shift0 = 0;
  //        cyclic_shift1 = 0;
  Msc_RS = 12*nb_rb;

  Msc_idx_ptr = (uint16_t*) bsearch(&Msc_RS, dftsizes, 33, sizeof(uint16_t), compareints);

  if (Msc_idx_ptr)
    Msc_RS_idx = Msc_idx_ptr - dftsizes;
  else {
    LOG_I(PHY,"generate_drs_pusch: index for Msc_RS=%d not found\n",Msc_RS);
    return(-1);
  }

  for (l = (3 - frame_parms->Ncp),u=u0,v=v0,cyclic_shift=cyclic_shift0;
       l<frame_parms->symbols_per_tti;
       l += (7 - frame_parms->Ncp),u=u1,v=v1,cyclic_shift=cyclic_shift1) {

    drs_offset = 0;
#ifdef DEBUG_DRS
    printf("drs_modulation: Msc_RS = %d, Msc_RS_idx = %d, u=%d,v=%d\n",Msc_RS, Msc_RS_idx,u,v);
#endif


    re_offset = frame_parms->first_carrier_offset;
    subframe_offset = subframe*frame_parms->symbols_per_tti*frame_parms->ofdm_symbol_size;
    symbol_offset = subframe_offset + frame_parms->ofdm_symbol_size*l;


#ifdef DEBUG_DRS
    printf("generate_drs_pusch: symbol_offset %d, subframe offset %d, cyclic shift %d\n",symbol_offset,subframe_offset,cyclic_shift);
#endif
    alpha_ind = 0;

    for (rb=0; rb<frame_parms->N_RB_UL; rb++) {

      if ((rb >= first_rb) && (rb<(first_rb+nb_rb))) {

#ifdef DEBUG_DRS
        printf("generate_drs_pusch: doing RB %d, re_offset=%d, drs_offset=%d,cyclic shift %d\n",rb,re_offset,drs_offset,cyclic_shift);
#endif

        for (k=0; k<12; k++) {
          ref_re = (int32_t) ul_ref_sigs[u][v][Msc_RS_idx][drs_offset<<1];
          ref_im = (int32_t) ul_ref_sigs[u][v][Msc_RS_idx][(drs_offset<<1)+1];

          ((int16_t*) txdataF)[2*(symbol_offset + re_offset)]   = (int16_t) (((ref_re*alpha_re[alpha_ind]) -
              (ref_im*alpha_im[alpha_ind]))>>15);
          ((int16_t*) txdataF)[2*(symbol_offset + re_offset)+1] = (int16_t) (((ref_re*alpha_im[alpha_ind]) +
              (ref_im*alpha_re[alpha_ind]))>>15);
          ((short*) txdataF)[2*(symbol_offset + re_offset)]   = (short) ((((short*) txdataF)[2*(symbol_offset + re_offset)]*(int32_t)amp)>>15);
          ((short*) txdataF)[2*(symbol_offset + re_offset)+1] = (short) ((((short*) txdataF)[2*(symbol_offset + re_offset)+1]*(int32_t)amp)>>15);


          alpha_ind = (alpha_ind + cyclic_shift);

          if (alpha_ind > 11)
            alpha_ind-=12;

#ifdef DEBUG_DRS
          printf("symbol_offset %d, alpha_ind %d , re_offset %d : (%d,%d)\n",
              symbol_offset,
              alpha_ind,
              re_offset,
              ((short*) txdataF)[2*(symbol_offset + re_offset)],
              ((short*) txdataF)[2*(symbol_offset + re_offset)+1]);

#endif  // DEBUG_DRS
          re_offset++;
          drs_offset++;

          if (re_offset >= frame_parms->ofdm_symbol_size)
            re_offset = 0;
        }

      } else {
        re_offset+=12; // go to next RB

        // check if we crossed the symbol boundary and skip DC

        if (re_offset >= frame_parms->ofdm_symbol_size) {
          if (frame_parms->N_RB_DL&1)  // odd number of RBs
            re_offset=6;
          else                         // even number of RBs (doesn't straddle DC)
            re_offset=0;
        }


      }
    }
  }

  return(0);
}

