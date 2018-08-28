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

/*! \file PHY/LTE_TRANSPORT/sss_ue.c
* \brief Top-level routines for decoding the secondary synchronization signal (SSS) V8.6 2009-03
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr
* \note
* \warning
*/
#include "PHY/defs_UE.h"
#include "transport_ue.h"
#include "PHY/phy_extern_ue.h"
#include "PHY/MODULATION/modulation_UE.h"

//#define DEBUG_SSS




int pss_ch_est(PHY_VARS_UE *ue,
               int32_t pss_ext[4][72],
               int32_t sss_ext[4][72])
{

  int16_t *pss;
  int16_t *pss_ext2,*sss_ext2,*sss_ext3,tmp_re,tmp_im,tmp_re2,tmp_im2;
  uint8_t aarx,i;
  LTE_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;

  switch (ue->common_vars.eNb_id) {

  case 0:
    pss = &primary_synch0[10];
    break;

  case 1:
    pss = &primary_synch1[10];
    break;

  case 2:
    pss = &primary_synch2[10];
    break;

  default:
    pss = &primary_synch0[10];
    break;
  }

  sss_ext3 = (int16_t*)&sss_ext[0][5];

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

    sss_ext2 = (int16_t*)&sss_ext[aarx][5];
    pss_ext2 = (int16_t*)&pss_ext[aarx][5];

    for (i=0; i<62; i++) {

      // This is H*(PSS) = R* \cdot PSS
      tmp_re = (int16_t)(((pss_ext2[i<<1] * (int32_t)pss[i<<1])>>15)     + ((pss_ext2[1+(i<<1)] * (int32_t)pss[1+(i<<1)])>>15));
      tmp_im = (int16_t)(((pss_ext2[i<<1] * (int32_t)pss[1+(i<<1)])>>15) - ((pss_ext2[1+(i<<1)] * (int32_t)pss[(i<<1)])>>15));
      //      printf("H*(%d,%d) : (%d,%d)\n",aarx,i,tmp_re,tmp_im);
      // This is R(SSS) \cdot H*(PSS)
      tmp_re2 = (int16_t)(((tmp_re * (int32_t)sss_ext2[i<<1])>>15)     - ((tmp_im * (int32_t)sss_ext2[1+(i<<1)]>>15)));
      tmp_im2 = (int16_t)(((tmp_re * (int32_t)sss_ext2[1+(i<<1)])>>15) + ((tmp_im * (int32_t)sss_ext2[(i<<1)]>>15)));

      //      printf("SSSi(%d,%d) : (%d,%d)\n",aarx,i,sss_ext2[i<<1],sss_ext2[1+(i<<1)]);
      //      printf("SSSo(%d,%d) : (%d,%d)\n",aarx,i,tmp_re2,tmp_im2);
      // MRC on RX antennas
      if (aarx==0) {
        sss_ext3[i<<1]      = tmp_re2;
        sss_ext3[1+(i<<1)]  = tmp_im2;
      } else {
        sss_ext3[i<<1]      += tmp_re2;
        sss_ext3[1+(i<<1)]  += tmp_im2;
      }
    }
  }

  // sss_ext now contains the compensated SSS
  return(0);
}


int _do_pss_sss_extract(PHY_VARS_UE *ue,
                    int32_t pss_ext[4][72],
                    int32_t sss_ext[4][72],
                    uint8_t doPss, uint8_t doSss,
					uint8_t subframe) // add flag to indicate extracting only PSS, only SSS, or both
{



  uint16_t rb,nb_rb=6;
  uint8_t i,aarx;
  int32_t *pss_rxF,*pss_rxF_ext;
  int32_t *sss_rxF,*sss_rxF_ext;
  LTE_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  uint8_t next_thread_id = ue->current_thread_id[subframe]== (RX_NB_TH-1) ? 0:(ue->current_thread_id[subframe]+1);

  int rx_offset = frame_parms->ofdm_symbol_size-3*12;
  uint8_t pss_symb,sss_symb;

  int32_t **rxdataF;

  //LOG_I(PHY,"do_pss_sss_extract subframe %d \n",subframe);
  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

	  if (frame_parms->frame_type == FDD) {
	    pss_symb = 6-frame_parms->Ncp;
	    sss_symb = pss_symb-1;

	    rxdataF  =  ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].rxdataF;
	    pss_rxF  =  &rxdataF[aarx][(rx_offset + (pss_symb*(frame_parms->ofdm_symbol_size)))];
	    sss_rxF  =  &rxdataF[aarx][(rx_offset + (sss_symb*(frame_parms->ofdm_symbol_size)))];

	  } else {
	    pss_symb = 2;
	    sss_symb = frame_parms->symbols_per_tti-1;

	    if(subframe==5 || subframe==0)
	    {
	    rxdataF  =  ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].rxdataF;
	    sss_rxF  =  &rxdataF[aarx][(rx_offset + (sss_symb*(frame_parms->ofdm_symbol_size)))];

	    rxdataF  =  ue->common_vars.common_vars_rx_data_per_thread[next_thread_id].rxdataF;
	    pss_rxF  =  &rxdataF[aarx][(rx_offset + (pss_symb*(frame_parms->ofdm_symbol_size)))];
	    }
	    else if(subframe==6 || subframe==1)
	    {
		    rxdataF  =  ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].rxdataF;
		    pss_rxF  =  &rxdataF[aarx][(rx_offset + (pss_symb*(frame_parms->ofdm_symbol_size)))];

		    rxdataF  =  ue->common_vars.common_vars_rx_data_per_thread[next_thread_id].rxdataF;
		    sss_rxF  =  &rxdataF[aarx][(rx_offset + (sss_symb*(frame_parms->ofdm_symbol_size)))];
	    }
	    else
	    {
	    	AssertFatal(0,"");
	    }

	  }
    //printf("extract_rbs: symbol_mod=%d, rx_offset=%d, ch_offset=%d\n",symbol_mod,
    //   (rx_offset + (symbol*(frame_parms->ofdm_symbol_size)))*2,
    //   LTE_CE_OFFSET+ch_offset+(symbol_mod*(frame_parms->ofdm_symbol_size)));

    pss_rxF_ext    = &pss_ext[aarx][0];
    sss_rxF_ext    = &sss_ext[aarx][0];

    for (rb=0; rb<nb_rb; rb++) {
      // skip DC carrier
      if (rb==3) {
        if(frame_parms->frame_type == FDD)
        {
          sss_rxF       = &rxdataF[aarx][(1 + (sss_symb*(frame_parms->ofdm_symbol_size)))];
          pss_rxF       = &rxdataF[aarx][(1 + (pss_symb*(frame_parms->ofdm_symbol_size)))];
        }
        else
        {
        	if(subframe==5 || subframe==0)
        	{
    	    rxdataF  =  ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].rxdataF;
    	    sss_rxF  =  &rxdataF[aarx][(1 + (sss_symb*(frame_parms->ofdm_symbol_size)))];

    	    rxdataF  =  ue->common_vars.common_vars_rx_data_per_thread[next_thread_id].rxdataF;
    	    pss_rxF  =  &rxdataF[aarx][(1 + (pss_symb*(frame_parms->ofdm_symbol_size)))];
        	}
    	    else if(subframe==6 || subframe==1)
    	    {
    		    rxdataF  =  ue->common_vars.common_vars_rx_data_per_thread[ue->current_thread_id[subframe]].rxdataF;
    		    pss_rxF  =  &rxdataF[aarx][(rx_offset + (pss_symb*(frame_parms->ofdm_symbol_size)))];

    		    rxdataF  =  ue->common_vars.common_vars_rx_data_per_thread[next_thread_id].rxdataF;
    		    sss_rxF  =  &rxdataF[aarx][(rx_offset + (sss_symb*(frame_parms->ofdm_symbol_size)))];
    	    }
    	    else
    	    {
    	    	AssertFatal(0,"");
    	    }
        }
      }

      for (i=0; i<12; i++) {
        if (doPss) {pss_rxF_ext[i]=pss_rxF[i];}
        if (doSss) {sss_rxF_ext[i]=sss_rxF[i];}
      }

      pss_rxF+=12;
      sss_rxF+=12;
      pss_rxF_ext+=12;
      sss_rxF_ext+=12;
    }

  }

  return(0);
}

int pss_sss_extract(PHY_VARS_UE *phy_vars_ue,
                    int32_t pss_ext[4][72],
                    int32_t sss_ext[4][72],
					uint8_t subframe)
{
  return _do_pss_sss_extract(phy_vars_ue, pss_ext, sss_ext, 1 /* doPss */, 1 /* doSss */, subframe);
}

int pss_only_extract(PHY_VARS_UE *phy_vars_ue,
                    int32_t pss_ext[4][72],
                    uint8_t subframe)
{
  static int32_t dummy[4][72];
  return _do_pss_sss_extract(phy_vars_ue, pss_ext, dummy, 1 /* doPss */, 0 /* doSss */, subframe);
}


int sss_only_extract(PHY_VARS_UE *phy_vars_ue,
                    int32_t sss_ext[4][72],
                    uint8_t subframe)
{
  static int32_t dummy[4][72];
  return _do_pss_sss_extract(phy_vars_ue, dummy, sss_ext, 0 /* doPss */, 1 /* doSss */, subframe);
}


int16_t phase_re[7] = {16383, 25101, 30791, 32767, 30791, 25101, 16383};
int16_t phase_im[7] = {-28378, -21063, -11208, 0, 11207, 21062, 28377};


int rx_sss(PHY_VARS_UE *ue,int32_t *tot_metric,uint8_t *flip_max,uint8_t *phase_max)
{

  uint8_t i;
  int32_t pss_ext[4][72];
  int32_t sss0_ext[4][72],sss5_ext[4][72];
  uint8_t Nid2 = ue->common_vars.eNb_id;
  uint8_t flip,phase;
  uint16_t Nid1;
  int16_t *sss0,*sss5;
  LTE_DL_FRAME_PARMS *frame_parms=&ue->frame_parms;
  int32_t metric;
  int16_t *d0,*d5;

  if (frame_parms->frame_type == FDD) {
#ifdef DEBUG_SSS

    if (frame_parms->Ncp == NORMAL)
      msg("[PHY][UE%d] Doing SSS for FDD Normal Prefix\n",ue->Mod_id);
    else
      msg("[PHY][UE%d] Doing SSS for FDD Extended Prefix\n",ue->Mod_id);

#endif
    // Do FFTs for SSS/PSS
    // SSS
    slot_fep(ue,
             (frame_parms->symbols_per_tti/2)-2, // second to last symbol of
             0,                                  // slot 0
             ue->rx_offset,
             0,
	     1);
    // PSS
    slot_fep(ue,
             (frame_parms->symbols_per_tti/2)-1, // last symbol of
             0,                                  // slot 0
             ue->rx_offset,
             0,
	     1);
  } else { // TDD
#ifdef DEBUG_SSS
    if (ue->frame_parms->Ncp == NORMAL)
      msg("[PHY][UE%d] Doing SSS for TDD Normal Prefix\n",ue->Mod_id);
    else
      msg("[PHY][UE%d] Doing SSS for TDD Extended Prefix\n",ue->Mod_id);

#endif
    // SSS
    slot_fep(ue,
             (frame_parms->symbols_per_tti>>1)-1,  // last symbol of
             1,                                    // slot 1
             ue->rx_offset,
             0,
	     1);
    // PSS
    slot_fep(ue,
             2,                                   // symbol 2 of
             2,                                   // slot 2
             ue->rx_offset,
             0,
	     1);
  }
  // pss sss extract for subframe 0
  pss_sss_extract(ue,
                  pss_ext,
                  sss0_ext,0);
  /*
  LOG_M("rxsig0.m","rxs0",&ue->common_vars.rxdata[0][0],ue->frame_parms.samples_per_tti,1,1);
  LOG_M("rxdataF0.m","rxF0",&ue->common_vars.rxdataF[0][0],2*14*ue->frame_parms.ofdm_symbol_size,2,1);
  LOG_M("pss_ext0.m","pssext0",pss_ext,72,1,1);
  LOG_M("sss0_ext0.m","sss0ext0",sss0_ext,72,1,1);
  */

  // get conjugated channel estimate from PSS (symbol 6), H* = R* \cdot PSS
  // and do channel estimation and compensation based on PSS

  pss_ch_est(ue,
             pss_ext,
             sss0_ext);

  //  LOG_M("sss0_comp0.m","sss0comp0",sss0_ext,72,1,1);

  if (ue->frame_parms.frame_type == FDD) { // FDD

    // SSS
    slot_fep(ue,
             (frame_parms->symbols_per_tti/2)-2,
             10,
             ue->rx_offset,
             0,1);
    // PSS
    slot_fep(ue,
             (frame_parms->symbols_per_tti/2)-1,
             10,
             ue->rx_offset,
             0,1);
  } else { // TDD
    // SSS
    slot_fep(ue,
             (frame_parms->symbols_per_tti>>1)-1,
             11,
             ue->rx_offset,
             0,
	     1);
    // PSS
    slot_fep(ue,
             2,
             12,
             ue->rx_offset,
             0,
	     1);
  }

  // pss sss extract for subframe 5
  pss_sss_extract(ue,
                  pss_ext,
                  sss5_ext,5);

  //  LOG_M("sss5_ext0.m","sss5ext0",sss5_ext,72,1,1);
  // get conjugated channel estimate from PSS (symbol 6), H* = R* \cdot PSS
  // and do channel estimation and compensation based on PSS

  pss_ch_est(ue,
             pss_ext,
             sss5_ext);



  // now do the SSS detection based on the precomputed sequences in PHY/LTE_TRANSPORT/sss.h

  *tot_metric = -99999999;


  sss0 = (int16_t*)&sss0_ext[0][5];
  sss5 = (int16_t*)&sss5_ext[0][5];

  for (flip=0; flip<2; flip++) {      //  d0/d5 flip in RX frame
    for (phase=0; phase<7; phase++) { // phase offset between PSS and SSS
      for (Nid1 = 0 ; Nid1 <= 167; Nid1++) {  // 168 possible Nid1 values
        metric = 0;

        if (flip==0) {
          d0 = &d0_sss[62*(Nid2 + (Nid1*3))];
          d5 = &d5_sss[62*(Nid2 + (Nid1*3))];
        } else {
          d5 = &d0_sss[62*(Nid2 + (Nid1*3))];
          d0 = &d5_sss[62*(Nid2 + (Nid1*3))];
        }

        // This is the inner product using one particular value of each unknown parameter
        for (i=0; i<62; i++) {
          metric += (int16_t)(((d0[i]*((((phase_re[phase]*(int32_t)sss0[i<<1])>>19)-((phase_im[phase]*(int32_t)sss0[1+(i<<1)])>>19)))) +
                               (d5[i]*((((phase_re[phase]*(int32_t)sss5[i<<1])>>19)-((phase_im[phase]*(int32_t)sss5[1+(i<<1)])>>19))))));
        }

        // if the current metric is better than the last save it
        if (metric > *tot_metric) {
          *tot_metric = metric;
          ue->frame_parms.Nid_cell = Nid2+(3*Nid1);
          *phase_max = phase;
          *flip_max=flip;
#ifdef DEBUG_SSS
          msg("(flip,phase,Nid1) (%d,%d,%d), metric_phase %d tot_metric %d, phase_max %d, flip_max %d\n",flip,phase,Nid1,metric,*tot_metric,*phase_max,*flip_max);
#endif

        }
      }
    }
  }

  return(0);
}

