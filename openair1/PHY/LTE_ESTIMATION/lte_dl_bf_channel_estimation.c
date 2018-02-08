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

#include <string.h>
#include "defs.h"
#include "PHY/defs.h"
#include "filt16_32.h"
//#define DEBUG_BF_CH

int lte_dl_bf_channel_estimation(PHY_VARS_UE *phy_vars_ue,
                                 uint8_t eNB_id,
                                 uint8_t eNB_offset,
                                 unsigned char Ns,
                                 unsigned char p,
                                 unsigned char symbol)
{
  
  unsigned short rb,nb_rb=0;
  unsigned char aarx,l,lprime,nsymb,skip_half=0,sss_symb,pss_symb=0,rb_alloc_ind,harq_pid,uespec_pilots=0;
  int beamforming_mode, ch_offset;
  uint8_t subframe;
  int8_t uespec_nushift, uespec_poffset=0, pil_offset=0;
  uint8_t pilot0,pilot1,pilot2,pilot3;

  short ch[2], *pil, *rxF, *dl_bf_ch, *dl_bf_ch_prev;
  short *fl, *fm, *fr, *fl_dc, *fm_dc, *fr_dc, *f1, *f2l, *f2r;

  unsigned int *rballoc; 
  int **rxdataF;
  int32_t **dl_bf_ch_estimates;
  int uespec_pilot[300];

  LTE_DL_FRAME_PARMS *frame_parms = &phy_vars_ue->frame_parms;
  LTE_UE_DLSCH_t **dlsch_ue       = phy_vars_ue->dlsch[phy_vars_ue->current_thread_id[Ns>>1]][eNB_id];
  LTE_DL_UE_HARQ_t *dlsch0_harq; 

  harq_pid    = dlsch_ue[0]->current_harq_pid;
  dlsch0_harq = dlsch_ue[0]->harq_processes[harq_pid];

  if (((frame_parms->Ncp == NORMAL) && (symbol>=7)) ||
      ((frame_parms->Ncp == EXTENDED) && (symbol>=6)))
    rballoc = dlsch0_harq->rb_alloc_odd;
  else
    rballoc = dlsch0_harq->rb_alloc_even;

  rxdataF = phy_vars_ue->common_vars.common_vars_rx_data_per_thread[phy_vars_ue->current_thread_id[Ns>>1]].rxdataF;

  dl_bf_ch_estimates = phy_vars_ue->pdsch_vars[phy_vars_ue->current_thread_id[Ns>>1]][eNB_id]->dl_bf_ch_estimates;
  beamforming_mode   = phy_vars_ue->transmission_mode[eNB_id]>6 ? phy_vars_ue->transmission_mode[eNB_id] : 0;

  if (phy_vars_ue->high_speed_flag == 0) // use second channel estimate position for temporary storage
    ch_offset     = frame_parms->ofdm_symbol_size;
  else
    ch_offset     = frame_parms->ofdm_symbol_size*symbol;

  
  uespec_nushift = frame_parms->Nid_cell%3;
  subframe = Ns>>1;
 

    //generate ue specific pilots
    lprime = symbol/3-1;
    lte_dl_ue_spec_rx(phy_vars_ue,uespec_pilot,Ns,5,lprime,0,dlsch0_harq->nb_rb);
    //write_output("uespec_pilot_rx.m","uespec_pilot",uespec_pilot,300,1,1);

    if (frame_parms->Ncp==0){
      if (symbol==3 || symbol==6 || symbol==9 || symbol==12)
        uespec_pilots = 1;
    } else{
      if (symbol==4 || symbol==7 || symbol==10)
        uespec_pilots = 1;
    }
   
    if ((frame_parms->Ncp==0 && (symbol==6 ||symbol ==12)) || (frame_parms->Ncp==1 && symbol==7))
      uespec_poffset = 2;

    if (phy_vars_ue->frame_parms.Ncp == 0) { // normal prefix
      pilot0 = 3;
      pilot1 = 6;
      pilot2 = 9;
      pilot3 = 12;
    } else { // extended prefix
      pilot0 = 4;
      pilot1 = 7;
      pilot2 = 10;
    }

    //define the filter
    pil_offset = (uespec_nushift+uespec_poffset)%3;
    // printf("symbol=%d,pil_offset=%d\n",symbol,pil_offset);
    switch (pil_offset) {
    case 0:
      fl = filt16_l0;
      fm = filt16_m0;
      fr = filt16_r0;
      fl_dc = filt16_l0;
      fm_dc = filt16_m0;
      fr_dc = filt16_r0;
      f1 = filt16_1;
      f2l = filt16_2l0;
      f2r = filt16_2r0;
      break;

    case 1:
      fl = filt16_l1;
      fm = filt16_m1;
      fr = filt16_r1;
      fl_dc = filt16_l1;
      fm_dc = filt16_m1;
      fr_dc = filt16_r1;
      f1 = filt16_1;
      f2l = filt16_2l1;
      f2r = filt16_2r1;
      break;

    case 2:
      fl = filt16_l2;
      fm = filt16_m2;
      fr = filt16_r2;
      fl_dc = filt16_l2;
      fm_dc = filt16_m2;
      fr_dc = filt16_r2;
      f1 = filt16_1;
      f2l = filt16_2l0;
      f2r = filt16_2r0;
      break;

    case 3:
      fl = filt16_l3;
      fm = filt16_m3;
      fr = filt16_r3;
      fl_dc = filt16_l3;
      fm_dc = filt16_m3;
      fr_dc = filt16_r3;
      f1 = filt16_1;
      f2l = filt16_2l1;
      f2r = filt16_2r1;
      break;
    }
 // beamforming mode extension
 /* }
  else if (beamforming_mode==0)
    msg("lte_dl_bf_channel_estimation:No beamforming is performed.\n");
  else
    msg("lte_dl_bf_channel_estimation:Beamforming mode not supported yet.\n");*/
  

  l=symbol;
  nsymb = (frame_parms->Ncp==NORMAL) ? 14:12;

  if (frame_parms->frame_type == TDD) {  //TDD
    sss_symb = nsymb-1;
    pss_symb = 2;
  } else {
    sss_symb = (nsymb>>1)-2;
    pss_symb = (nsymb>>1)-1;
  }


  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

    rxF  = (short *)&rxdataF[aarx][pil_offset + frame_parms->first_carrier_offset + symbol*frame_parms->ofdm_symbol_size];
    pil  = (short *)uespec_pilot;
    dl_bf_ch = (short *)&dl_bf_ch_estimates[aarx][ch_offset];

    memset(dl_bf_ch,0,4*(frame_parms->ofdm_symbol_size));
    //memset(dl_bf_ch,0,2*(frame_parms->ofdm_symbol_size));

    if (phy_vars_ue->high_speed_flag==0) {
    // multiply previous channel estimate by ch_est_alpha
      if (frame_parms->Ncp==0){
        multadd_complex_vector_real_scalar(dl_bf_ch-(frame_parms->ofdm_symbol_size<<1),
                                           phy_vars_ue->ch_est_alpha,dl_bf_ch-(frame_parms->ofdm_symbol_size<<1),
                                           1,frame_parms->ofdm_symbol_size);
      } else {
        LOG_E(PHY,"lte_dl_bf_channel_estimation: beamforming channel estimation not supported for TM7 Extended CP.\n"); // phy_vars_ue->ch_est_beta should be defined equaling 1/3
      }
    }
    //estimation and interpolation

    if ((frame_parms->N_RB_DL&1) == 0) { // even number of RBs
      for (rb=0; rb<frame_parms->N_RB_DL; rb++) {

        if (rb < 32)
          rb_alloc_ind = (rballoc[0]>>rb) & 1;
        else if (rb < 64)
          rb_alloc_ind = (rballoc[1]>>(rb-32)) & 1;
        else if (rb < 96)
          rb_alloc_ind = (rballoc[2]>>(rb-64)) & 1;
        else if (rb < 100)
          rb_alloc_ind = (rballoc[3]>>(rb-96)) & 1;
        else
          rb_alloc_ind = 0;

        // For second half of RBs skip DC carrier
        if (rb==(frame_parms->N_RB_DL>>1)) {
          rxF       = (short *)&rxdataF[aarx][(1 + (symbol*(frame_parms->ofdm_symbol_size)))];
        }

        if (rb_alloc_ind==1) {
          if (uespec_pilots==1) {
            if (beamforming_mode==7) {
              if (frame_parms->Ncp==0) {

                ch[0] = (short)(((int)pil[0]*rxF[0] - (int)pil[1]*rxF[1])>>15);
                ch[1] = (short)(((int)pil[0]*rxF[1] + (int)pil[1]*rxF[0])>>15);
                multadd_real_vector_complex_scalar(fl,ch,dl_bf_ch,16);
                
                ch[0] = (short)(((int)pil[0]*rxF[8] - (int)pil[1]*rxF[9])>>15);
                ch[1] = (short)(((int)pil[0]*rxF[9] + (int)pil[1]*rxF[8])>>15);
                multadd_real_vector_complex_scalar(fm,ch,dl_bf_ch,16);

                ch[0] = (short)(((int)pil[0]*rxF[16] - (int)pil[1]*rxF[17])>>15);
                ch[1] = (short)(((int)pil[0]*rxF[17] + (int)pil[1]*rxF[16])>>15);
                multadd_real_vector_complex_scalar(fr,ch,dl_bf_ch,16);
              } else {
                LOG_E(PHY,"lte_dl_bf_channel_estimation(lte_dl_bf_channel_estimation.c):TM7 beamgforming channel estimation not supported for extented CP\n");
                exit(-1);
              }
            } else {
              LOG_E(PHY,"lte_dl_bf_channel_estimation(lte_dl_bf_channel_estimation.c): transmission mode not supported.\n");
            }
          }
          nb_rb++;
        }

        rxF+=24;
        dl_bf_ch+=24;
      }
    } else {  // Odd number of RBs
      for (rb=0; rb<frame_parms->N_RB_DL>>1; rb++) {
        skip_half=0;

        if (rb < 32)
          rb_alloc_ind = (rballoc[0]>>rb) & 1;
        else if (rb < 64)
          rb_alloc_ind = (rballoc[1]>>(rb-32)) & 1;
        else if (rb < 96)
          rb_alloc_ind = (rballoc[2]>>(rb-64)) & 1;
        else if (rb < 100)
          rb_alloc_ind = (rballoc[3]>>(rb-96)) & 1;
        else
          rb_alloc_ind = 0;

        // PBCH
        if ((subframe==0) && (rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l>=(nsymb>>1)) && (l<((nsymb>>1) + 4))) {
          rb_alloc_ind = 0;
        }

        //PBCH subframe 0, symbols nsymb>>1 ... nsymb>>1 + 3
        if ((subframe==0) && (rb==((frame_parms->N_RB_DL>>1)-3)) && (l>=(nsymb>>1)) && (l<((nsymb>>1) + 4)))
          skip_half=1;
        else if ((subframe==0) && (rb==((frame_parms->N_RB_DL>>1)+3)) && (l>=(nsymb>>1)) && (l<((nsymb>>1) + 4)))
          skip_half=2;

        //SSS
        if (((subframe==0)||(subframe==5)) &&
            (rb>((frame_parms->N_RB_DL>>1)-3)) &&
            (rb<((frame_parms->N_RB_DL>>1)+3)) &&
            (l==sss_symb) ) {
          rb_alloc_ind = 0;
        }

        //SSS
        if (((subframe==0)||(subframe==5)) &&
            (rb==((frame_parms->N_RB_DL>>1)-3)) &&
            (l==sss_symb))
          skip_half=1;
        else if (((subframe==0)||(subframe==5)) &&
                 (rb==((frame_parms->N_RB_DL>>1)+3)) &&
                 (l==sss_symb))
          skip_half=2;

        //PSS in subframe 0/5 if FDD
        if (frame_parms->frame_type == FDD) {  //FDD
          if (((subframe==0)||(subframe==5)) && (rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb) ) {
            rb_alloc_ind = 0;
          }

          if (((subframe==0)||(subframe==5)) && (rb==((frame_parms->N_RB_DL>>1)-3)) && (l==pss_symb))
            skip_half=1;
          else if (((subframe==0)||(subframe==5)) && (rb==((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb))
            skip_half=2;
        }

        if ((frame_parms->frame_type == TDD) && ((subframe==1)||(subframe==6))) { //TDD Subframe 1 and 6
          if ((rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb) ) {
            rb_alloc_ind = 0;
          }

          if ((rb==((frame_parms->N_RB_DL>>1)-3)) && (l==pss_symb))
            skip_half=1;
          else if ((rb==((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb))
            skip_half=2;
        }

        //printf("symbol=%d,pil_offset=%d\ni,rb_alloc_ind=%d,uespec_pilots=%d,beamforming_mode=%d,Ncp=%d,skip_half=%d\n",symbol,pil_offset,rb_alloc_ind,uespec_pilots,beamforming_mode,frame_parms->Ncp,skip_half);
        if (rb_alloc_ind==1) {
          if (uespec_pilots==1) {
            if (beamforming_mode==7) {
              if (frame_parms->Ncp==0) {
                if (skip_half==1) {
                  if (pil_offset<2) {

                    ch[0] = (short)(((int)pil[0]*rxF[0] - (int)pil[1]*rxF[1])>>15);
                    ch[1] = (short)(((int)pil[0]*rxF[1] + (int)pil[1]*rxF[0])>>15);
                    multadd_real_vector_complex_scalar(f2l,ch,dl_bf_ch,16); 
                    pil+=2;
                    
                    ch[0] = (short)(((int)pil[0]*rxF[8] - (int)pil[1]*rxF[9])>>15);
                    ch[1] = (short)(((int)pil[0]*rxF[9] + (int)pil[1]*rxF[8])>>15);
                    multadd_real_vector_complex_scalar(f2r,ch,dl_bf_ch,16); 
                    pil+=2;

                  } else {

                    ch[0] = (short)(((int)pil[0]*rxF[0] - (int)pil[1]*rxF[1])>>15);
                    ch[1] = (short)(((int)pil[0]*rxF[1] + (int)pil[1]*rxF[0])>>15);
                    multadd_real_vector_complex_scalar(f1,ch,dl_bf_ch,16);
                    pil+=2;
                  }
                } else if (skip_half==2) {
                  if (pil_offset<2) {

                    ch[0] = (short)(((int)pil[0]*rxF[16] - (int)pil[1]*rxF[17])>>15);
                    ch[1] = (short)(((int)pil[0]*rxF[17] + (int)pil[1]*rxF[16])>>15);
                    multadd_real_vector_complex_scalar(f1,ch,dl_bf_ch,16); 
                    pil+=2;
                    
                  } else {

                    ch[0] = (short)(((int)pil[0]*rxF[8] - (int)pil[1]*rxF[9])>>15);
                    ch[1] = (short)(((int)pil[0]*rxF[9] + (int)pil[1]*rxF[8])>>15);
                    multadd_real_vector_complex_scalar(f2l,ch,dl_bf_ch,16);
                    pil+=2;

                    ch[0] = (short)(((int)pil[0]*rxF[16] - (int)pil[1]*rxF[17])>>15);
                    ch[1] = (short)(((int)pil[0]*rxF[17] + (int)pil[1]*rxF[16])>>15);
                    multadd_real_vector_complex_scalar(f2r,ch,dl_bf_ch,16); 
                    pil+=2;

                  }
                } else {

                  ch[0] = (short)(((int)pil[0]*rxF[0] - (int)pil[1]*rxF[1])>>15);
                  ch[1] = (short)(((int)pil[0]*rxF[1] + (int)pil[1]*rxF[0])>>15);
                  multadd_real_vector_complex_scalar(fl,ch,dl_bf_ch,16);
#ifdef DEBUG_BF_CH
                  printf("symbol=%d,rxF[0]=(%d,%d),pil=(%d,%d),ch=(%d,%d)\n",symbol,rxF[0],rxF[1],pil[0],pil[1],ch[0],ch[1]);
#endif
                  pil+=2;
                  
                  ch[0] = (short)(((int)pil[0]*rxF[8] - (int)pil[1]*rxF[9])>>15);
                  ch[1] = (short)(((int)pil[0]*rxF[9] + (int)pil[1]*rxF[8])>>15);
                  multadd_real_vector_complex_scalar(fm,ch,dl_bf_ch,16);
#ifdef DEBUG_BF_CH
                  printf("symbol=%d,rxF[4]=(%d,%d),pil=(%d,%d),ch=(%d,%d)\n",symbol,rxF[8],rxF[9],pil[0],pil[1],ch[0],ch[1]);
#endif
                  pil+=2;

                  ch[0] = (short)(((int)pil[0]*rxF[16] - (int)pil[1]*rxF[17])>>15);
                  ch[1] = (short)(((int)pil[0]*rxF[17] + (int)pil[1]*rxF[16])>>15);
                  multadd_real_vector_complex_scalar(fr,ch,dl_bf_ch,16);
#ifdef DEBUG_BF_CH
                  printf("symbol=%d,rxF[8]=(%d,%d),pil=(%d,%d),ch=(%d,%d)\n",symbol,rxF[16],rxF[17],pil[0],pil[1],ch[0],ch[1]);
#endif
                  pil+=2;

               }  
             } else {
		LOG_E(PHY,"lte_dl_bf_channel_estimation(lte_dl_bf_channel_estimation.c):TM7 beamgforming channel estimation not supported for extented CP\n");
		exit(-1);
             }
          
           } else {
	      LOG_E(PHY,"lte_dl_bf_channel_estimation(lte_dl_bf_channel_estimation.c):transmission mode not supported.\n");
           }
         }       
         nb_rb++;
       }

        rxF+=24;
        dl_bf_ch+=24;
      } // first half loop

      // Do middle RB (around DC) 
      if (rb < 32)
        rb_alloc_ind = (rballoc[0]>>rb) & 1;
      else if (rb < 64)
        rb_alloc_ind = (rballoc[1]>>(rb-32)) & 1;
      else if (rb < 96)
        rb_alloc_ind = (rballoc[2]>>(rb-64)) & 1;
      else if (rb < 100)
        rb_alloc_ind = (rballoc[3]>>(rb-96)) & 1;
      else
        rb_alloc_ind = 0;

      // PBCH
      if ((subframe==0) && (rb>=((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l>=(nsymb>>1)) && (l<((nsymb>>1) + 4))) {
        rb_alloc_ind = 0;
      }

      //SSS
      if (((subframe==0)||(subframe==5)) && (rb>=((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==sss_symb) ) {
        rb_alloc_ind = 0;
      }

      if (frame_parms->frame_type == FDD) {
       //PSS
        if (((subframe==0)||(subframe==5)) && (rb>=((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb) ) {
          rb_alloc_ind = 0;
        }
      }

      if ((frame_parms->frame_type == TDD) && ((subframe==1)||(subframe==6))) {
        //PSS
        if ((rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb) ) {
          rb_alloc_ind = 0;
        }
      }

      //printf("DC rb %d (%p)\n",rb,rxF);
      if (rb_alloc_ind==1) {
        if (pil_offset<2) {
          ch[0] = (short)(((int)pil[0]*rxF[0] - (int)pil[1]*rxF[1])>>15);
          ch[1] = (short)(((int)pil[0]*rxF[1] + (int)pil[1]*rxF[0])>>15);
          multadd_real_vector_complex_scalar(fl_dc,ch,dl_bf_ch,16);
#ifdef DEBUG_BF_CH
          //printf("symbol=%d,rxF[0]=(%d,%d),pil=(%d,%d),ch=(%d,%d)\n",symbol,rxF[0],rxF[1],pil[0],pil[1],ch[0],ch[1]);
#endif
          pil+=2;;
                  
          ch[0] = (short)(((int)pil[0]*rxF[8] - (int)pil[1]*rxF[9])>>15);
          ch[1] = (short)(((int)pil[0]*rxF[9] + (int)pil[1]*rxF[8])>>15);
          multadd_real_vector_complex_scalar(fm_dc,ch,dl_bf_ch,16);
#ifdef DEBUG_BF_CH
          //printf("symbol=%d,rxF[4]=(%d,%d),pil=(%d,%d),ch=(%d,%d)\n",symbol,rxF[8],rxF[9],pil[0],pil[1],ch[0],ch[1]);
#endif
          pil+=2;;

          rxF   = (short *)&rxdataF[aarx][symbol*(frame_parms->ofdm_symbol_size)];

          ch[0] = (short)(((int)pil[0]*rxF[6] - (int)pil[1]*rxF[7])>>15);
          ch[1] = (short)(((int)pil[0]*rxF[7] + (int)pil[1]*rxF[6])>>15);
          multadd_real_vector_complex_scalar(fr_dc,ch,dl_bf_ch,16);
#ifdef DEBUG_BF_CH
          //printf("symbol=%d,rxF[3]=(%d,%d),pil=(%d,%d),ch=(%d,%d)\n",symbol,rxF[6],rxF[7],pil[0],pil[1],ch[0],ch[1]);
#endif
          pil+=2;;
        } else {
          ch[0] = (short)(((int)pil[0]*rxF[0] - (int)pil[1]*rxF[1])>>15);
          ch[1] = (short)(((int)pil[0]*rxF[1] + (int)pil[1]*rxF[0])>>15);
          multadd_real_vector_complex_scalar(fl_dc,ch,dl_bf_ch,16);
#ifdef DEBUG_BF_CH
          //printf("symbol=%d,rxF[0]=(%d,%d),pil=(%d,%d),ch=(%d,%d)\n",symbol,rxF[0],rxF[1],pil[0],pil[1],ch[0],ch[1]);
#endif
          pil+=2;;
                  
          rxF   = (short *)&rxdataF[aarx][symbol*(frame_parms->ofdm_symbol_size)];

          ch[0] = (short)(((int)pil[0]*rxF[2] - (int)pil[1]*rxF[3])>>15);
          ch[1] = (short)(((int)pil[0]*rxF[3] + (int)pil[1]*rxF[2])>>15);
          multadd_real_vector_complex_scalar(fm_dc,ch,dl_bf_ch,16);
#ifdef DEBUG_BF_CH
          //printf("symbol=%d,rxF[1]=(%d,%d),pil=(%d,%d),ch=(%d,%d)\n",symbol,rxF[2],rxF[3],pil[0],pil[1],ch[0],ch[1]);
#endif
          pil+=2;;

          ch[0] = (short)(((int)pil[0]*rxF[10] - (int)pil[1]*rxF[11])>>15);
          ch[1] = (short)(((int)pil[0]*rxF[11] + (int)pil[1]*rxF[10])>>15);
          multadd_real_vector_complex_scalar(fr_dc,ch,dl_bf_ch,16);
#ifdef DEBUG_BF_CH
          //printf("symbol=%d,rxF[5]=(%d,%d),pil=(%d,%d),ch=(%d,%d)\n",symbol,rxF[10],rxF[11],pil[0],pil[1],ch[0],ch[1]);
#endif
          pil+=2;;
        }
      } // rballoc==1
      else {
        rxF       = (short *)&rxdataF[aarx][pil_offset+((symbol*(frame_parms->ofdm_symbol_size)))];
      }

      rxF+=14+2*pil_offset;
      dl_bf_ch+=24;
      rb++;

      for (; rb<frame_parms->N_RB_DL; rb++) {
        skip_half=0;

        if (rb < 32)
          rb_alloc_ind = (rballoc[0]>>rb) & 1;
        else if (rb < 64)
          rb_alloc_ind = (rballoc[1]>>(rb-32)) & 1;
        else if (rb < 96)
          rb_alloc_ind = (rballoc[2]>>(rb-64)) & 1;
        else if (rb < 100)
          rb_alloc_ind = (rballoc[3]>>(rb-96)) & 1;
        else
          rb_alloc_ind = 0;

        // PBCH
        if ((subframe==0) && (rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l>=nsymb>>1) && (l<((nsymb>>1) + 4))) {
          rb_alloc_ind = 0;
        }

        //PBCH subframe 0, symbols nsymb>>1 ... nsymb>>1 + 3
        if ((subframe==0) && (rb==((frame_parms->N_RB_DL>>1)-3)) && (l>=(nsymb>>1)) && (l<((nsymb>>1) + 4)))
          skip_half=1;
        else if ((subframe==0) && (rb==((frame_parms->N_RB_DL>>1)+3)) && (l>=(nsymb>>1)) && (l<((nsymb>>1) + 4)))
          skip_half=2;

        //SSS
        if (((subframe==0)||(subframe==5)) && (rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==sss_symb) ) {
          rb_alloc_ind = 0;
        }

        //SSS
        if (((subframe==0)||(subframe==5)) && (rb==((frame_parms->N_RB_DL>>1)-3)) && (l==sss_symb))
          skip_half=1;
        else if (((subframe==0)||(subframe==5)) && (rb==((frame_parms->N_RB_DL>>1)+3)) && (l==sss_symb))
          skip_half=2;

        if (frame_parms->frame_type == FDD) {
          //PSS
          if (((subframe==0)||(subframe==5)) && (rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb) ) {
            rb_alloc_ind = 0;
          }

          //PSS
          if (((subframe==0)||(subframe==5)) && (rb==((frame_parms->N_RB_DL>>1)-3)) && (l==pss_symb))
            skip_half=1;
          else if (((subframe==0)||(subframe==5)) && (rb==((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb))
            skip_half=2;
        }

        if ((frame_parms->frame_type == TDD) && ((subframe==1)||(subframe==6))) { //TDD Subframe 1 and 6
          if ((rb>((frame_parms->N_RB_DL>>1)-3)) && (rb<((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb) ) {
            rb_alloc_ind = 0;
          }

          if ((rb==((frame_parms->N_RB_DL>>1)-3)) && (l==pss_symb))
            skip_half=1;
          else if ((rb==((frame_parms->N_RB_DL>>1)+3)) && (l==pss_symb))
            skip_half=2;
        }

        if (rb_alloc_ind==1) {
          if (uespec_pilots==1) {
            if (beamforming_mode==7) {
              if (frame_parms->Ncp==0) {
                if (skip_half==1) {
                  if (pil_offset<2) {

                    ch[0] = (short)(((int)pil[0]*rxF[0] - (int)pil[1]*rxF[1])>>15);
                    ch[1] = (short)(((int)pil[0]*rxF[1] + (int)pil[1]*rxF[0])>>15);
                    multadd_real_vector_complex_scalar(f2l,ch,dl_bf_ch,16); 
                    pil+=2;
                    
                    ch[0] = (short)(((int)pil[0]*rxF[8] - (int)pil[1]*rxF[9])>>15);
                    ch[1] = (short)(((int)pil[0]*rxF[9] + (int)pil[1]*rxF[8])>>15);
                    multadd_real_vector_complex_scalar(f2r,ch,dl_bf_ch,16); 
                    pil+=2;
          

                  } else {

                    ch[0] = (short)(((int)pil[0]*rxF[0] - (int)pil[1]*rxF[1])>>15);
                    ch[1] = (short)(((int)pil[0]*rxF[1] + (int)pil[1]*rxF[0])>>15);
                    multadd_real_vector_complex_scalar(f1,ch,dl_bf_ch,16);
                    pil+=2;
                  }
                } else if (skip_half==2) {
                   if (pil_offset<2) {

                     ch[0] = (short)(((int)pil[0]*rxF[16] - (int)pil[1]*rxF[17])>>15);
                     ch[1] = (short)(((int)pil[0]*rxF[17] + (int)pil[1]*rxF[16])>>15);
                     multadd_real_vector_complex_scalar(f1,ch,dl_bf_ch,16); 
                     pil+=2;
                     
                   } else {

                     ch[0] = (short)(((int)pil[0]*rxF[8] - (int)pil[1]*rxF[9])>>15);
                     ch[1] = (short)(((int)pil[0]*rxF[9] + (int)pil[1]*rxF[8])>>15);
                     multadd_real_vector_complex_scalar(f2l,ch,dl_bf_ch,16);
                     pil+=2;

                     ch[0] = (short)(((int)pil[0]*rxF[16] - (int)pil[1]*rxF[17])>>15);
                     ch[1] = (short)(((int)pil[0]*rxF[17] + (int)pil[1]*rxF[16])>>15);
                     multadd_real_vector_complex_scalar(f2r,ch,dl_bf_ch,16); 
                     pil+=2;

                   }
                } else {

                  ch[0] = (short)(((int)pil[0]*rxF[0] - (int)pil[1]*rxF[1])>>15);
                  ch[1] = (short)(((int)pil[0]*rxF[1] + (int)pil[1]*rxF[0])>>15);
                  multadd_real_vector_complex_scalar(fl,ch,dl_bf_ch,16);
#ifdef DEBUG_BF_CH
                  printf("symbol=%d,rxF[0]=(%d,%d),pil=(%d,%d),ch=(%d,%d)\n",symbol,rxF[0],rxF[1],pil[0],pil[1],ch[0],ch[1]);
#endif
                  pil+=2;
                  
                  ch[0] = (short)(((int)pil[0]*rxF[8] - (int)pil[1]*rxF[9])>>15);
                  ch[1] = (short)(((int)pil[0]*rxF[9] + (int)pil[1]*rxF[8])>>15);
                  multadd_real_vector_complex_scalar(fm,ch,dl_bf_ch,16);
#ifdef DEBUG_BF_CH
                  printf("symbol=%d,rxF[4]=(%d,%d),pil=(%d,%d),ch=(%d,%d)\n",symbol,rxF[8],rxF[9],pil[0],pil[1],ch[0],ch[1]);
#endif
                  pil+=2;

                  ch[0] = (short)(((int)pil[0]*rxF[16] - (int)pil[1]*rxF[17])>>15);
                  ch[1] = (short)(((int)pil[0]*rxF[17] + (int)pil[1]*rxF[16])>>15);
                  multadd_real_vector_complex_scalar(fr,ch,dl_bf_ch,16);
#ifdef DEBUG_BF_CH
                  printf("symbol=%d,rxF[8]=(%d,%d),pil=(%d,%d),ch=(%d,%d)\n",symbol,rxF[16],rxF[17],pil[0],pil[1],ch[0],ch[1]);
#endif
                  pil+=2;

                }
              } else {
                LOG_E(PHY,"lte_dl_bf_channel_estimation(lte_dl_bf_channel_estimation.c):TM7 beamgforming channel estimation not supported for extented CP\n");
                exit(-1);
              }
            
            } else {
              LOG_E(PHY,"lte_dl_bf_channel_estimation(lte_dl_bf_channel_estimation.c):transmission mode not supported.\n");
            }
          }
          nb_rb++;
        }

        rxF+=24;
        dl_bf_ch+=24;
      } // second half of RBs
    } // odd number of RBs  

    // Temporal Interpolation
    if (phy_vars_ue->perfect_ce == 0) {

      dl_bf_ch = (short *)&dl_bf_ch_estimates[aarx][ch_offset];
#ifdef DEBUG_BF_CH
      printf("[dlsch_bf_ch_est.c]:symbol %d, dl_bf_ch (%d,%d)\n",symbol,dl_bf_ch[0],dl_bf_ch[1]);
#endif

      if (phy_vars_ue->high_speed_flag == 0) {
        multadd_complex_vector_real_scalar(dl_bf_ch,
                                           32767-phy_vars_ue->ch_est_alpha,
                                           dl_bf_ch-(frame_parms->ofdm_symbol_size<<1),0,frame_parms->ofdm_symbol_size);
        //printf("dlsch_bf_ch_est.c:symbol %d,dl_bf_ch (%d,%d)\n",symbol,*(dl_bf_ch-512*2),*(dl_bf_ch-512*2+1));
      } else { // high_speed_flag == 1
        if (beamforming_mode==7) {
          if (frame_parms->Ncp==0) {
            if (symbol == pilot0) {
              //      printf("Interpolating %d->0\n",4-phy_vars_ue->lte_frame_parms.Ncp);
              //      dl_bf_ch_prev = (short *)&dl_bf_ch_estimates[aarx][(4-phy_vars_ue->lte_frame_parms.Ncp)*(frame_parms->ofdm_symbol_size)];
              dl_bf_ch_prev = (short *)&dl_bf_ch_estimates[aarx][pilot3*(frame_parms->ofdm_symbol_size)];
#ifdef DEBUG_BF_CH
              printf("[dlsch_bf_ch_est.c] symbol=%d, dl_bf_ch_prev=(%d,%d), dl_bf_ch=(%d,%d)\n", symbol, dl_bf_ch_prev[0], dl_bf_ch_prev[1], dl_bf_ch[0], dl_bf_ch[1]);
#endif
              // pilot spacing 5 symbols (1/5,2/5,3/5,4/5 combination)
              multadd_complex_vector_real_scalar(dl_bf_ch_prev,26214,dl_bf_ch_prev+(2*(frame_parms->ofdm_symbol_size)),1,frame_parms->ofdm_symbol_size);
              multadd_complex_vector_real_scalar(dl_bf_ch,6554,dl_bf_ch_prev+(2*(frame_parms->ofdm_symbol_size)),0,frame_parms->ofdm_symbol_size);

              multadd_complex_vector_real_scalar(dl_bf_ch_prev,19661,dl_bf_ch-(3*2*(frame_parms->ofdm_symbol_size)),1,frame_parms->ofdm_symbol_size);
              multadd_complex_vector_real_scalar(dl_bf_ch,13107,dl_bf_ch-(3*2*(frame_parms->ofdm_symbol_size)),0,frame_parms->ofdm_symbol_size);

              multadd_complex_vector_real_scalar(dl_bf_ch_prev,13107,dl_bf_ch-(2*((frame_parms->ofdm_symbol_size)<<1)),1,frame_parms->ofdm_symbol_size);
              multadd_complex_vector_real_scalar(dl_bf_ch,19661,dl_bf_ch-(2*((frame_parms->ofdm_symbol_size)<<1)),0,frame_parms->ofdm_symbol_size);

              multadd_complex_vector_real_scalar(dl_bf_ch_prev,6554,dl_bf_ch-(2*(frame_parms->ofdm_symbol_size)),1,frame_parms->ofdm_symbol_size);
              multadd_complex_vector_real_scalar(dl_bf_ch,26214,dl_bf_ch-(2*(frame_parms->ofdm_symbol_size)),0,frame_parms->ofdm_symbol_size);
            } else if (symbol == pilot1) {
              dl_bf_ch_prev = (short *)&dl_bf_ch_estimates[aarx][pilot0*(frame_parms->ofdm_symbol_size)];
#ifdef DEBUG_BF_CH
              printf("[dlsch_bf_ch_est.c] symbol=%d, dl_bf_ch_prev=(%d,%d), dl_bf_ch=(%d,%d)\n", symbol, dl_bf_ch_prev[0], dl_bf_ch_prev[1], dl_bf_ch[0], dl_bf_ch[1]);
#endif

              // pilot spacing 3 symbols (1/3,2/3 combination)
              multadd_complex_vector_real_scalar(dl_bf_ch_prev,21845,dl_bf_ch_prev+(2*(frame_parms->ofdm_symbol_size)),1,frame_parms->ofdm_symbol_size);
              multadd_complex_vector_real_scalar(dl_bf_ch,10923,dl_bf_ch_prev+(2*(frame_parms->ofdm_symbol_size)),0,frame_parms->ofdm_symbol_size);

              multadd_complex_vector_real_scalar(dl_bf_ch_prev,10923,dl_bf_ch_prev+(2*((frame_parms->ofdm_symbol_size)<<1)),1,frame_parms->ofdm_symbol_size);
              multadd_complex_vector_real_scalar(dl_bf_ch,21845,dl_bf_ch_prev+(2*((frame_parms->ofdm_symbol_size)<<1)),0,frame_parms->ofdm_symbol_size);

            } else if (symbol == pilot2) {
              dl_bf_ch_prev = (short *)&dl_bf_ch_estimates[aarx][pilot1*(frame_parms->ofdm_symbol_size)];
#ifdef DEBUG_BF_CH
              printf("[dlsch_bf_ch_est.c] symbol=%d, dl_bf_ch_prev=(%d,%d), dl_bf_ch=(%d,%d)\n", symbol, dl_bf_ch_prev[0], dl_bf_ch_prev[1], dl_bf_ch[0], dl_bf_ch[1]);
#endif

              multadd_complex_vector_real_scalar(dl_bf_ch_prev,21845,dl_bf_ch_prev+(2*(frame_parms->ofdm_symbol_size)),1,frame_parms->ofdm_symbol_size);
              multadd_complex_vector_real_scalar(dl_bf_ch,10923,dl_bf_ch_prev+(2*(frame_parms->ofdm_symbol_size)),0,frame_parms->ofdm_symbol_size);

              multadd_complex_vector_real_scalar(dl_bf_ch_prev,10923,dl_bf_ch_prev+(2*((frame_parms->ofdm_symbol_size)<<1)),1,frame_parms->ofdm_symbol_size);
              multadd_complex_vector_real_scalar(dl_bf_ch,21845,dl_bf_ch_prev+(2*((frame_parms->ofdm_symbol_size)<<1)),0,frame_parms->ofdm_symbol_size);
            } else { // symbol == pilot3
            //      printf("Interpolating 0->%d\n",4-phy_vars_ue->lte_frame_parms.Ncp);
            dl_bf_ch_prev = (short *)&dl_bf_ch_estimates[aarx][pilot2*(frame_parms->ofdm_symbol_size)];

            // pilot spacing 3 symbols (1/3,2/3 combination)
            multadd_complex_vector_real_scalar(dl_bf_ch_prev,21845,dl_bf_ch_prev+(2*(frame_parms->ofdm_symbol_size)),1,frame_parms->ofdm_symbol_size);
            multadd_complex_vector_real_scalar(dl_bf_ch,10923,dl_bf_ch_prev+(2*(frame_parms->ofdm_symbol_size)),0,frame_parms->ofdm_symbol_size);

            multadd_complex_vector_real_scalar(dl_bf_ch_prev,10923,dl_bf_ch_prev+(2*((frame_parms->ofdm_symbol_size)<<1)),1,frame_parms->ofdm_symbol_size);
            multadd_complex_vector_real_scalar(dl_bf_ch,21845,dl_bf_ch_prev+(2*((frame_parms->ofdm_symbol_size)<<1)),0,frame_parms->ofdm_symbol_size);
            }

          } else {
            LOG_E(PHY,"lte_dl_bf_channel_estimation:temporal interpolation not supported for TM7 extented CP.\n");
          }
        } else {
          LOG_E(PHY,"lte_dl_bf_channel_estimation:temporal interpolation not supported for this beamforming mode.\n");
        } 
      }
    }
  } //aarx
 
#ifdef DEBUG_BF_CH  
    printf("[dlsch_bf_ch_est.c]: dl_bf_estimates[0][600] %d, %d \n",*(short *)&dl_bf_ch_estimates[0][600],*(short*)&phy_vars_ue->lte_ue_pdsch_vars[eNB_id]->dl_bf_ch_estimates[0][600]);
#endif

  return(0);

}
