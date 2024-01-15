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


#include <string.h>
#include "SCHED_NR_UE/defs.h"
#include "nr_estimation.h"
#include "PHY/NR_REFSIG/refsig_defs_ue.h"
#include "PHY/NR_REFSIG/nr_refsig.h"
#include "PHY/NR_REFSIG/dmrs_nr.h"
#include "PHY/NR_REFSIG/ptrs_nr.h"
#include "PHY/NR_REFSIG/nr_mod_table.h"
#include "PHY/NR_TRANSPORT/nr_sch_dmrs.h"
#include "PHY/NR_TRANSPORT/nr_transport_proto.h"
#include "common/utils/nr/nr_common.h"
#include "filt16a_32.h"
#include "T.h"
#include <openair1/PHY/TOOLS/phy_scope_interface.h>
extern openair0_config_t openair0_cfg[];

//#define DEBUG_PDSCH
//#define DEBUG_PDCCH
//#define DEBUG_PBCH
//#define DEBUG_PRS_CHEST   // To enable PRS Matlab dumps
//#define DEBUG_PRS_PRINTS  // To enable PRS channel estimation debug logs

#define CH_INTERP 0
#define NO_INTERP 1

/* Generic function to find the peak of channel estimation buffer */
void peak_estimator(int32_t *buffer, int32_t buf_len, int32_t *peak_idx, int32_t *peak_val, int32_t mean_val)
{
  int32_t max_val = 0, max_idx = 0, abs_val = 0;
  for(int k = 0; k < buf_len; k++)
  {
    abs_val = squaredMod(((c16_t*)buffer)[k]);
    if(abs_val > max_val)
    {
      max_val = abs_val;
      max_idx = k;
    }
  }

  // Check for detection threshold
  LOG_D(PHY, "PRS ToA estimator: max_val %d, mean_val %d, max_idx %d\n", max_val, mean_val, max_idx);
  if ((mean_val != 0) && (max_val / mean_val > 10)) {
    *peak_val = max_val;
    *peak_idx = max_idx;
  } else {
    *peak_val = 0;
    *peak_idx = 0;
  }
}

int nr_prs_channel_estimation(uint8_t gNB_id,
                              uint8_t rsc_id,
                              uint8_t rep_num,
                              PHY_VARS_NR_UE *ue,
                              const UE_nr_rxtx_proc_t *proc,
                              NR_DL_FRAME_PARMS *frame_params,
                              c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP])
{
  uint8_t rxAnt = 0, idx = 0;
  prs_config_t *prs_cfg  = &ue->prs_vars[gNB_id]->prs_resource[rsc_id].prs_cfg;
  prs_meas_t **prs_meas  = ue->prs_vars[gNB_id]->prs_resource[rsc_id].prs_meas;
  c16_t ch_tmp_buf[ ue->frame_parms.ofdm_symbol_size] __attribute__((aligned(32)));
  int32_t chF_interpol[frame_params->nb_antennas_rx][NR_PRS_IDFT_OVERSAMP_FACTOR*ue->frame_parms.ofdm_symbol_size] __attribute__((aligned(32)));
  int32_t chT_interpol[frame_params->nb_antennas_rx][NR_PRS_IDFT_OVERSAMP_FACTOR*ue->frame_parms.ofdm_symbol_size] __attribute__((aligned(32)));
  memset(ch_tmp_buf,0,sizeof(ch_tmp_buf));
  memset(chF_interpol,0,sizeof(chF_interpol));
  memset(chT_interpol,0,sizeof(chF_interpol));
  
  int slot_prs           = (proc->nr_slot_rx - rep_num*prs_cfg->PRSResourceTimeGap + frame_params->slots_per_frame)%frame_params->slots_per_frame;
  uint32_t **nr_gold_prs = ue->nr_gold_prs[gNB_id][rsc_id][slot_prs];

  int16_t *rxF, *pil, mod_prs[NR_MAX_PRS_LENGTH << 1];
  const int16_t *fl, *fm, *fmm, *fml, *fmr, *fr;
  int16_t ch[2] = {0}, noiseFig[2] = {0};
  int16_t k_prime = 0, k = 0;
  int32_t ch_pwr = 0, snr = 0,  rsrp = 0, mean_val = 0, prs_toa = 0;
  double ch_pwr_dbm = 0.0f;
#ifdef DEBUG_PRS_CHEST
  char filename[64] = {0}, varname[64] = {0};
#endif
  int16_t *ch_tmp     = (int16_t *)ch_tmp_buf; 
  int16_t scale_factor = (1.0f/(float)(prs_cfg->NumPRSSymbols))*(1<<15);
  int16_t num_pilots   = (12/prs_cfg->CombSize)*prs_cfg->NumRB;
  int16_t first_half = frame_params->ofdm_symbol_size - frame_params->first_carrier_offset;
  int16_t second_half = (prs_cfg->NumRB * 12) - first_half;
  int16_t start_offset = NR_PRS_IDFT_OVERSAMP_FACTOR * frame_params->ofdm_symbol_size - first_half;
  LOG_D(PHY, "start_offset %d, first_half %d, second_half %d\n", start_offset, first_half, second_half);

  int16_t k_prime_table[K_PRIME_TABLE_ROW_SIZE][K_PRIME_TABLE_COL_SIZE] = PRS_K_PRIME_TABLE;
  for(int l = prs_cfg->SymbolStart; l < prs_cfg->SymbolStart+prs_cfg->NumPRSSymbols; l++)
  {
    int symInd = l-prs_cfg->SymbolStart;
    if (prs_cfg->CombSize == 2) {
      k_prime = k_prime_table[0][symInd];
    }
    else if (prs_cfg->CombSize == 4){
      k_prime = k_prime_table[1][symInd];
    }
    else if (prs_cfg->CombSize == 6){
      k_prime = k_prime_table[2][symInd];
    }
    else if (prs_cfg->CombSize == 12){
      k_prime = k_prime_table[3][symInd];
    }
   
#ifdef DEBUG_PRS_PRINTS 
    printf("[gNB %d][rsc %d] PRS config l %d k_prime %d:\nprs_cfg->SymbolStart %d\nprs_cfg->NumPRSSymbols %d\nprs_cfg->NumRB %d\nprs_cfg->CombSize %d\n", gNB_id, rsc_id, l, k_prime, prs_cfg->SymbolStart, prs_cfg->NumPRSSymbols, prs_cfg->NumRB, prs_cfg->CombSize);
#endif
    // Pilots generation and modulation
    for (int m = 0; m < num_pilots; m++) 
    {
      idx = (((nr_gold_prs[l][(m<<1)>>5])>>((m<<1)&0x1f))&3);
      mod_prs[m<<1]     = nr_qpsk_mod_table[idx<<1];
      mod_prs[(m<<1)+1] = nr_qpsk_mod_table[(idx<<1) + 1];
    } 
     
    for (rxAnt=0; rxAnt < frame_params->nb_antennas_rx; rxAnt++)
    {
      // reset variables
      snr = 0;
      rsrp = 0;

      // calculate RE offset
      k = (prs_cfg->REOffset + k_prime) % prs_cfg->CombSize + prs_cfg->RBOffset * 12 + frame_params->first_carrier_offset;

      // Channel estimation and interpolation
      pil       = (int16_t *)&mod_prs[0];
      rxF       = (int16_t *)&rxdataF[rxAnt][l*frame_params->ofdm_symbol_size + k];
      
      if(prs_cfg->CombSize == 2)
      {
        // Choose the interpolation filters
        switch (k_prime) {
          case 0:
            fl  = filt8_l0;
            fml = filt8_m0;
            fmm = filt8_mm0;
            fmr = filt8_mr0;
            fm  = filt8_m0;
            fr  = filt8_r0;
            break;

          case 1:
            fl  = filt8_l1;
            fmm = filt8_mm1;
            fml = filt8_ml1;
            fmr = fmm;
            fm  = filt8_m1;
            fr  = filt8_r1;
            break;

          default:
            LOG_I(PHY, "%s: ERROR!! Invalid k_prime=%d for PRS comb_size %d, symbol %d\n",__FUNCTION__, k_prime, prs_cfg->CombSize, l);
            return(-1);
            break;
        }

        //Start pilot
        ch[0] = (int16_t)(((int32_t)rxF[0]*pil[0] + (int32_t)rxF[1]*pil[1])>>15);
        ch[1] = (int16_t)(((int32_t)rxF[1]*pil[0] - (int32_t)rxF[0]*pil[1])>>15);
        multadd_real_vector_complex_scalar(fl,
		    	       ch,
		    	       ch_tmp,
		    	       8);

        // SNR & RSRP estimation
        rsrp += squaredMod(*((c16_t *)rxF));
        noiseFig[0] = rxF[0] - (int16_t)(((int32_t)ch[0]*pil[0] - (int32_t)ch[1]*pil[1])>>15);
        noiseFig[1] = rxF[1] - (int16_t)(((int32_t)ch[1]*pil[0] + (int32_t)ch[0]*pil[1])>>15);
        snr += 10*log10(squaredMod(*(c16_t*)rxF) - squaredMod(*(c16_t*)noiseFig)) - 10*log10(squaredMod(*(c16_t*)noiseFig));
#ifdef DEBUG_PRS_PRINTS
        printf("[Rx %d] pilot %3d, SNR %+2d dB: rxF - > (%+3d, %+3d) addr %p  ch -> (%+3d, %+3d), pil -> (%+d, %+d) \n", rxAnt, 0, snr, rxF[0],rxF[1],&rxF[0],ch[0],ch[1],pil[0],pil[1]);
#endif
        pil +=2;
        k   = (k+prs_cfg->CombSize) % frame_params->ofdm_symbol_size;
        rxF = (int16_t *)&rxdataF[rxAnt][l*frame_params->ofdm_symbol_size + k];
        
        //Middle pilots
        for(int pIdx = 1; pIdx < num_pilots-1; pIdx+=2)
        {
          ch[0] = (int16_t)(((int32_t)rxF[0]*pil[0] + (int32_t)rxF[1]*pil[1])>>15);
          ch[1] = (int16_t)(((int32_t)rxF[1]*pil[0] - (int32_t)rxF[0]*pil[1])>>15);
          if(pIdx == 1) // 2nd pilot
          {
            multadd_real_vector_complex_scalar(fml,
		        	       ch,
		        	       ch_tmp,
		        	       8);
          }
          else
          {
            multadd_real_vector_complex_scalar(fm,
		        	       ch,
		        	       ch_tmp,
		        	       8);
          }

          // SNR & RSRP estimation
          rsrp += squaredMod(*((c16_t *)rxF));
          noiseFig[0] = rxF[0] - (int16_t)(((int32_t)ch[0]*pil[0] - (int32_t)ch[1]*pil[1])>>15);
          noiseFig[1] = rxF[1] - (int16_t)(((int32_t)ch[1]*pil[0] + (int32_t)ch[0]*pil[1])>>15);
          snr += 10*log10(squaredMod(*(c16_t*)rxF) - squaredMod(*(c16_t*)noiseFig)) - 10*log10(squaredMod(*(c16_t*)noiseFig));
#ifdef DEBUG_PRS_PRINTS
          printf("[Rx %d] pilot %3d, SNR %+2d dB: rxF - > (%+3d, %+3d) addr %p  ch -> (%+3d, %+3d), pil -> (%+d, %+d) \n", rxAnt, pIdx, snr/(pIdx+1), rxF[0],rxF[1],&rxF[0],ch[0],ch[1],pil[0],pil[1]);
#endif
          pil +=2;
          k   = (k+prs_cfg->CombSize) % frame_params->ofdm_symbol_size;
          rxF = (int16_t *)&rxdataF[rxAnt][l*frame_params->ofdm_symbol_size + k];

          ch[0] = (int16_t)(((int32_t)rxF[0]*pil[0] + (int32_t)rxF[1]*pil[1])>>15);
          ch[1] = (int16_t)(((int32_t)rxF[1]*pil[0] - (int32_t)rxF[0]*pil[1])>>15);
          if(pIdx == (num_pilots-3)) // 2nd last pilot
          {
            multadd_real_vector_complex_scalar(fmr,
		        	       ch,
		        	       ch_tmp,
		        	       8);
          }
          else
          {
            multadd_real_vector_complex_scalar(fmm,
		        	       ch,
		        	       ch_tmp,
		        	       8);
          }

          // SNR & RSRP estimation
          rsrp += squaredMod(*((c16_t *)rxF));
          noiseFig[0] = rxF[0] - (int16_t)(((int32_t)ch[0]*pil[0] - (int32_t)ch[1]*pil[1])>>15);
          noiseFig[1] = rxF[1] - (int16_t)(((int32_t)ch[1]*pil[0] + (int32_t)ch[0]*pil[1])>>15);
          snr += 10*log10(squaredMod(*((c16_t*)rxF)) - squaredMod(*((c16_t*)noiseFig))) - 10*log10(squaredMod(*((c16_t*)noiseFig)));
#ifdef DEBUG_PRS_PRINTS
          printf("[Rx %d] pilot %3d, SNR %+2d dB: rxF - > (%+3d, %+3d) addr %p  ch -> (%+3d, %+3d), pil -> (%+d, %+d) \n", rxAnt, pIdx+1, snr/(pIdx+2), rxF[0],rxF[1],&rxF[0],ch[0],ch[1],pil[0],pil[1]);
#endif
          pil +=2;
          k   = (k+prs_cfg->CombSize) % frame_params->ofdm_symbol_size;
          rxF = (int16_t *)&rxdataF[rxAnt][l*frame_params->ofdm_symbol_size + k];
          ch_tmp +=8;
        }

        //End pilot
        ch[0] = (int16_t)(((int32_t)rxF[0]*pil[0] + (int32_t)rxF[1]*pil[1])>>15);
        ch[1] = (int16_t)(((int32_t)rxF[1]*pil[0] - (int32_t)rxF[0]*pil[1])>>15);
        multadd_real_vector_complex_scalar(fr,
	      	       ch,
	      	       ch_tmp,
	      	       8);

        // SNR & RSRP estimation
        rsrp += squaredMod(*((c16_t *)rxF));
        noiseFig[0] = rxF[0] - (int16_t)(((int32_t)ch[0] * pil[0] - (int32_t)ch[1] * pil[1]) >> 15);
        noiseFig[1] = rxF[1] - (int16_t)(((int32_t)ch[1] * pil[0] + (int32_t)ch[0] * pil[1]) >> 15);
        snr += 10 * log10(squaredMod(*((c16_t *)rxF)) - squaredMod(*((c16_t *)noiseFig))) - 10 * log10(squaredMod(*((c16_t *)noiseFig)));
#ifdef DEBUG_PRS_PRINTS
        printf("[Rx %d] pilot %3d, SNR %+2d dB: rxF - > (%+3d, %+3d) addr %p  ch -> (%+3d, %+3d), pil -> (%+d, %+d) \n",
               rxAnt,
               num_pilots - 1,
               snr / num_pilots,
               rxF[0],
               rxF[1],
               &rxF[0],
               ch[0],
               ch[1],
               pil[0],
               pil[1]);
#endif
      }
      else if(prs_cfg->CombSize == 4)
      {
        // Choose the interpolation filters
        switch (k_prime) {
          case 0:
            fl  = filt16a_l0;
            fml = filt16a_mm0;
            fmm = filt16a_mm0;
            fmr = filt16a_m0;
            fm  = filt16a_m0;
            fr  = filt16a_r0;
            break;

          case 1:
            fl  = filt16a_l1;
            fml = filt16a_ml1;
            fmm = filt16a_mm1;
            fmr = filt16a_mr1;
            fm  = filt16a_m1;
            fr  = filt16a_r1;
            break;

          case 2:
            fl  = filt16a_l2;
            fml = filt16a_ml2;
            fmm = filt16a_mm2;
            fmr = filt16a_mr2;
            fm  = filt16a_m2;
            fr  = filt16a_r2;
            break;

          case 3:
            fl  = filt16a_l3;
            fml = filt16a_ml3;
            fmm = filt16a_mm3;
            fmr = filt16a_mm3;
            fm  = filt16a_m3;
            fr  = filt16a_r3;
            break;

          default:
            LOG_I(PHY, "%s: ERROR!! Invalid k_prime=%d for PRS comb_size %d, symbol %d\n",__FUNCTION__, k_prime, prs_cfg->CombSize, l);
            return(-1);
            break;
        }

        //Start pilot
        ch[0] = (int16_t)(((int32_t)rxF[0]*pil[0] + (int32_t)rxF[1]*pil[1])>>15);
        ch[1] = (int16_t)(((int32_t)rxF[1]*pil[0] - (int32_t)rxF[0]*pil[1])>>15);
        multadd_real_vector_complex_scalar(fl,
	      	       ch,
	      	       ch_tmp,
	      	       16);

        // SNR & RSRP estimation
        rsrp += squaredMod(*((c16_t *)rxF));
        noiseFig[0] = rxF[0] - (int16_t)(((int32_t)ch[0]*pil[0] - (int32_t)ch[1]*pil[1])>>15);
        noiseFig[1] = rxF[1] - (int16_t)(((int32_t)ch[1]*pil[0] + (int32_t)ch[0]*pil[1])>>15);
        snr += 10*log10(squaredMod(*((c16_t*)rxF)) - squaredMod(*((c16_t*)noiseFig))) - 10*log10(squaredMod(*((c16_t*)noiseFig)));
#ifdef DEBUG_PRS_PRINTS
        printf("[Rx %d] pilot %3d, SNR %+2d dB: rxF - > (%+3d, %+3d) addr %p  ch -> (%+3d, %+3d), pil -> (%+d, %+d) \n", rxAnt, 0, snr, rxF[0],rxF[1],&rxF[0],ch[0],ch[1],pil[0],pil[1]);
#endif
        pil +=2;
        k   = (k+prs_cfg->CombSize) % frame_params->ofdm_symbol_size;
        rxF = (int16_t *)&rxdataF[rxAnt][l*frame_params->ofdm_symbol_size + k];

        ch[0] = (int16_t)(((int32_t)rxF[0]*pil[0] + (int32_t)rxF[1]*pil[1])>>15);
        ch[1] = (int16_t)(((int32_t)rxF[1]*pil[0] - (int32_t)rxF[0]*pil[1])>>15);
        multadd_real_vector_complex_scalar(fml,
	      	       ch,
	      	       ch_tmp,
	      	       16);

        // SNR & RSRP estimation
        rsrp += squaredMod(*((c16_t *)rxF));
        noiseFig[0] = rxF[0] - (int16_t)(((int32_t)ch[0]*pil[0] - (int32_t)ch[1]*pil[1])>>15);
        noiseFig[1] = rxF[1] - (int16_t)(((int32_t)ch[1]*pil[0] + (int32_t)ch[0]*pil[1])>>15);
        snr += 10*log10(squaredMod(*((c16_t*)rxF)) - squaredMod(*((c16_t*)noiseFig))) - 10*log10(squaredMod(*((c16_t*)noiseFig)));
#ifdef DEBUG_PRS_PRINTS
        printf("[Rx %d] pilot %3d, SNR %+2d dB: rxF - > (%+3d, %+3d) addr %p  ch -> (%+3d, %+3d), pil -> (%+d, %+d) \n", rxAnt, 1, snr/2, rxF[0],rxF[1],&rxF[0],ch[0],ch[1],pil[0],pil[1]);
#endif
        pil +=2;
        k   = (k+prs_cfg->CombSize) % frame_params->ofdm_symbol_size;
        rxF = (int16_t *)&rxdataF[rxAnt][l*frame_params->ofdm_symbol_size + k];
        ch_tmp +=8;

        //Middle pilots
        for(int pIdx = 2; pIdx < num_pilots-2; pIdx++)
        {
          ch[0] = (int16_t)(((int32_t)rxF[0]*pil[0] + (int32_t)rxF[1]*pil[1])>>15);
          ch[1] = (int16_t)(((int32_t)rxF[1]*pil[0] - (int32_t)rxF[0]*pil[1])>>15);
          multadd_real_vector_complex_scalar(fmm,
	      	         ch,
	      	         ch_tmp,
	      	         16);

          // SNR & RSRP estimation
          rsrp += squaredMod(*((c16_t *)rxF));
          noiseFig[0] = rxF[0] - (int16_t)(((int32_t)ch[0]*pil[0] - (int32_t)ch[1]*pil[1])>>15);
          noiseFig[1] = rxF[1] - (int16_t)(((int32_t)ch[1]*pil[0] + (int32_t)ch[0]*pil[1])>>15);
          snr += 10*log10(squaredMod(*((c16_t*)rxF)) - squaredMod(*((c16_t*)noiseFig))) - 10*log10(squaredMod(*((c16_t*)noiseFig)));
#ifdef DEBUG_PRS_PRINTS
          printf("[Rx %d] pilot %3d, SNR %+2d dB: rxF - > (%+3d, %+3d) addr %p  ch -> (%+3d, %+3d), pil -> (%+d, %+d) \n", rxAnt, pIdx, snr/(pIdx+1), rxF[0],rxF[1],&rxF[0],ch[0],ch[1],pil[0],pil[1]);
#endif
          pil +=2;
          k   = (k+prs_cfg->CombSize) % frame_params->ofdm_symbol_size;
          rxF = (int16_t *)&rxdataF[rxAnt][l*frame_params->ofdm_symbol_size + k];
          ch_tmp +=8;
        }

        //End pilot
        ch[0] = (int16_t)(((int32_t)rxF[0]*pil[0] + (int32_t)rxF[1]*pil[1])>>15);
        ch[1] = (int16_t)(((int32_t)rxF[1]*pil[0] - (int32_t)rxF[0]*pil[1])>>15);
        multadd_real_vector_complex_scalar(fmr,
	      	       ch,
	      	       ch_tmp,
	      	       16);

        // SNR & RSRP estimation
        rsrp += squaredMod(*((c16_t *)rxF));
        noiseFig[0] = rxF[0] - (int16_t)(((int32_t)ch[0]*pil[0] - (int32_t)ch[1]*pil[1])>>15);
        noiseFig[1] = rxF[1] - (int16_t)(((int32_t)ch[1]*pil[0] + (int32_t)ch[0]*pil[1])>>15);
        snr += 10*log10(squaredMod(*((c16_t*)rxF)) - squaredMod(*((c16_t*)noiseFig))) - 10*log10(squaredMod(*((c16_t*)noiseFig)));
#ifdef DEBUG_PRS_PRINTS
        printf("[Rx %d] pilot %3d, SNR %+2d dB: rxF - > (%+3d, %+3d) addr %p  ch -> (%+3d, %+3d), pil -> (%+d, %+d) \n", rxAnt, num_pilots-2, snr/(num_pilots-1), rxF[0],rxF[1],&rxF[0],ch[0],ch[1],pil[0],pil[1]);
#endif
        pil +=2;
        k   = (k+prs_cfg->CombSize) % frame_params->ofdm_symbol_size;
        rxF = (int16_t *)&rxdataF[rxAnt][l*frame_params->ofdm_symbol_size + k];

        ch[0] = (int16_t)(((int32_t)rxF[0]*pil[0] + (int32_t)rxF[1]*pil[1])>>15);
        ch[1] = (int16_t)(((int32_t)rxF[1]*pil[0] - (int32_t)rxF[0]*pil[1])>>15);
        multadd_real_vector_complex_scalar(fr,
	      	       ch,
	      	       ch_tmp,
	      	       16);

        // SNR & RSRP estimation
        rsrp += squaredMod(*((c16_t *)rxF));
        noiseFig[0] = rxF[0] - (int16_t)(((int32_t)ch[0]*pil[0] - (int32_t)ch[1]*pil[1])>>15);
        noiseFig[1] = rxF[1] - (int16_t)(((int32_t)ch[1]*pil[0] + (int32_t)ch[0]*pil[1])>>15);
        snr += 10*log10(squaredMod(*((c16_t*)rxF)) - squaredMod(*((c16_t*)noiseFig))) - 10*log10(squaredMod(*((c16_t*)noiseFig)));
#ifdef DEBUG_PRS_PRINTS
        printf("[Rx %d] pilot %3d, SNR %+2d dB: rxF - > (%+3d, %+3d) addr %p  ch -> (%+3d, %+3d), pil -> (%+d, +%d) \n", rxAnt, num_pilots-1, snr/num_pilots, rxF[0],rxF[1],&rxF[0],ch[0],ch[1],pil[0],pil[1]);
#endif
      }
      else
      {
        AssertFatal((prs_cfg->CombSize == 2)||(prs_cfg->CombSize == 4), "[%s] DL PRS CombSize other than 2 and 4 are NOT supported currently. Exiting!!!", __FUNCTION__);
      }

      // average out the SNR and RSRP computed
      prs_meas[rxAnt]->snr = snr / (float)num_pilots;
      prs_meas[rxAnt]->rsrp = rsrp / (float)num_pilots;

      //reset channel pointer
      ch_tmp = (int16_t*)ch_tmp_buf;
    } // for rxAnt
  } //for l
  
  for (rxAnt=0; rxAnt < frame_params->nb_antennas_rx; rxAnt++)
  {
    // scale by averaging factor 1/NumPrsSymbols
    multadd_complex_vector_real_scalar(ch_tmp,
                                       scale_factor,
                                       ch_tmp,
                                       1,
                                       frame_params->ofdm_symbol_size);

#ifdef DEBUG_PRS_PRINTS
    for (int rb = 0; rb < prs_cfg->NumRB; rb++)
    {
      printf("================================================================\n");
      printf("\t\t\t[gNB %d][Rx %d][RB %d]\n", gNB_id, rxAnt, rb);
      printf("================================================================\n");
      idx = (12*rb)<<1;
      printf("%4d %4d  %4d %4d  %4d %4d  %4d %4d  %4d %4d  %4d %4d\n", ch_tmp[idx], ch_tmp[idx+1], ch_tmp[idx+2], ch_tmp[idx+3], ch_tmp[idx+4], ch_tmp[idx+5], ch_tmp[idx+6], ch_tmp[idx+7], ch_tmp[idx+8], ch_tmp[idx+9], ch_tmp[idx+10], ch_tmp[idx+11]);
      printf("%4d %4d  %4d %4d  %4d %4d  %4d %4d  %4d %4d  %4d %4d\n", ch_tmp[idx+12], ch_tmp[idx+13], ch_tmp[idx+14], ch_tmp[idx+15], ch_tmp[idx+16], ch_tmp[idx+17], ch_tmp[idx+18], ch_tmp[idx+19], ch_tmp[idx+20], ch_tmp[idx+21], ch_tmp[idx+22], ch_tmp[idx+23]);
      printf("\n");
    }
#endif

    // Place PRS channel estimates in FFT shifted format
    if(first_half > 0)
      memcpy((int16_t *)&chF_interpol[rxAnt][start_offset], &ch_tmp[0], first_half * sizeof(int32_t));
    if(second_half > 0)
      memcpy((int16_t *)&chF_interpol[rxAnt][0], &ch_tmp[first_half << 1], second_half * sizeof(int32_t));

    // Time domain IMPULSE response
    idft_size_idx_t idftsizeidx;
    switch (NR_PRS_IDFT_OVERSAMP_FACTOR*frame_params->ofdm_symbol_size) {
    case 128:
      idftsizeidx = IDFT_128;
      break;

    case 256:
      idftsizeidx = IDFT_256;
      break;

    case 512:
      idftsizeidx = IDFT_512;
      break;

    case 768:
      idftsizeidx = IDFT_768;
      break;

    case 1024:
      idftsizeidx = IDFT_1024;
      break;

    case 1536:
      idftsizeidx = IDFT_1536;
      break;

    case 2048:
      idftsizeidx = IDFT_2048;
      break;

    case 3072:
      idftsizeidx = IDFT_3072;
      break;

    case 4096:
      idftsizeidx = IDFT_4096;
      break;
    
    case 6144:
      idftsizeidx = IDFT_6144;
      break;
 
    // 16x IDFT oversampling
    case 8192:
      idftsizeidx = IDFT_8192;
      break;

    case 12288:
      idftsizeidx = IDFT_12288;
      break;

    case 16384:
      idftsizeidx = IDFT_16384;
      break;

    case 24576:
      idftsizeidx = IDFT_24576;
      break;

    case 32768:
      idftsizeidx = IDFT_32768;
      break;

    case 49152:
      idftsizeidx = IDFT_49152;
      break;

    case 65536:
      idftsizeidx = IDFT_65536;
      break;

    default:
      LOG_I(PHY, "%s: unsupported ofdm symbol size \n", __FUNCTION__);
      assert(0);
    }

    idft(idftsizeidx,
         (int16_t *)&chF_interpol[rxAnt][0],
         (int16_t *)&chT_interpol[rxAnt][0],1);

    // peak estimator
    mean_val = squaredMod(((c16_t *)ch_tmp)[(prs_cfg->NumRB * 12) >> 1]);
    peak_estimator(&chT_interpol[rxAnt][0], NR_PRS_IDFT_OVERSAMP_FACTOR * frame_params->ofdm_symbol_size, &prs_toa, &ch_pwr, mean_val);

    // adjusting the rx_gains for channel peak power
    ch_pwr_dbm = 10 * log10(ch_pwr) + 30 - 10 * log10(pow(2, 30)) - ((int)openair0_cfg[0].rx_gain[0] - (int)openair0_cfg[0].rx_gain_offset[0]) - dB_fixed(frame_params->ofdm_symbol_size);

    prs_meas[rxAnt]->rsrp_dBm =
        10 * log10(prs_meas[rxAnt]->rsrp) + 30 - 10 * log10(pow(2, 30)) - ((int)openair0_cfg[0].rx_gain[0] - (int)openair0_cfg[0].rx_gain_offset[0]) - dB_fixed(ue->frame_parms.ofdm_symbol_size);

    //prs measurements
    prs_meas[rxAnt]->gNB_id     = gNB_id;
    prs_meas[rxAnt]->sfn        = proc->frame_rx;
    prs_meas[rxAnt]->slot       = proc->nr_slot_rx;
    prs_meas[rxAnt]->rxAnt_idx  = rxAnt;
    prs_meas[rxAnt]->dl_aoa     = rsc_id;
    prs_meas[rxAnt]->dl_toa = prs_toa / (float)NR_PRS_IDFT_OVERSAMP_FACTOR;
    if ((frame_params->ofdm_symbol_size - prs_meas[rxAnt]->dl_toa) < frame_params->ofdm_symbol_size / 2)
      prs_meas[rxAnt]->dl_toa -= (frame_params->ofdm_symbol_size);
    LOG_I(PHY,
          "[gNB %d][rsc %d][Rx %d][sfn %d][slot %d] DL PRS ToA ==> %.1f / %d samples, peak channel power %.1f dBm, SNR %+.1f dB, rsrp %+.1f dBm\n",
          gNB_id,
          rsc_id,
          rxAnt,
          proc->frame_rx,
          proc->nr_slot_rx,
          prs_meas[rxAnt]->dl_toa,
          frame_params->ofdm_symbol_size,
          ch_pwr_dbm,
          prs_meas[rxAnt]->snr,
          prs_meas[rxAnt]->rsrp_dBm);

#ifdef DEBUG_PRS_CHEST
    sprintf(filename, "%s%i%s", "PRSpilot_", rxAnt, ".m");
    LOG_M(filename, "prs_loc", &mod_prs[0], num_pilots,1,1);
    sprintf(filename, "%s%i%s", "rxSigF_", rxAnt, ".m");
    sprintf(varname, "%s%i", "rxF_", rxAnt);
    LOG_M(filename, varname, &rxdataF[rxAnt][0], prs_cfg->NumPRSSymbols*frame_params->ofdm_symbol_size,1,1);
    sprintf(filename, "%s%i%s", "prsChestF_", rxAnt, ".m");
    sprintf(varname, "%s%i", "prsChF_", rxAnt);
    LOG_M(filename, varname, &chF_interpol[rxAnt][start_offset], frame_params->ofdm_symbol_size,1,1);
    sprintf(filename, "%s%i%s", "prsChestT_", rxAnt, ".m");
    sprintf(varname, "%s%i", "prsChT_", rxAnt);
    LOG_M(filename, varname, &chT_interpol[rxAnt][start_offset], frame_params->ofdm_symbol_size,1,1);
#endif

    // T tracer dump
    T(T_UE_PHY_INPUT_SIGNAL, T_INT(gNB_id),
      T_INT(proc->frame_rx), T_INT(proc->nr_slot_rx),
      T_INT(rxAnt), T_BUFFER(&rxdataF[rxAnt][0], frame_params->samples_per_slot_wCP*sizeof(int32_t)));

    T(T_UE_PHY_DL_CHANNEL_ESTIMATE_FREQ,
      T_INT(gNB_id),
      T_INT(rsc_id),
      T_INT(proc->frame_rx),
      T_INT(proc->nr_slot_rx),
      T_INT(rxAnt),
      T_BUFFER(&chF_interpol[rxAnt][0], NR_PRS_IDFT_OVERSAMP_FACTOR * frame_params->ofdm_symbol_size * sizeof(int32_t)));

    T(T_UE_PHY_DL_CHANNEL_ESTIMATE,
      T_INT(gNB_id),
      T_INT(rsc_id),
      T_INT(proc->frame_rx),
      T_INT(proc->nr_slot_rx),
      T_INT(rxAnt),
      T_BUFFER(&chT_interpol[rxAnt][0], NR_PRS_IDFT_OVERSAMP_FACTOR * frame_params->ofdm_symbol_size * sizeof(int32_t)));
  }

  return(0);
}

int nr_pbch_dmrs_correlation(PHY_VARS_NR_UE *ue,
                             const UE_nr_rxtx_proc_t *proc,
                             unsigned char symbol,
                             int dmrss,
                             NR_UE_SSB *current_ssb,
                             c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP])
{
  int pilot[200] __attribute__((aligned(16)));
  unsigned int pilot_cnt;
  int16_t ch[2],*pil,*rxF;
  int symbol_offset;

  uint8_t ssb_index=current_ssb->i_ssb;
  uint8_t n_hf=current_ssb->n_hf;

  unsigned int  ssb_offset = ue->frame_parms.first_carrier_offset + ue->frame_parms.ssb_start_subcarrier;
  if (ssb_offset>= ue->frame_parms.ofdm_symbol_size) ssb_offset-=ue->frame_parms.ofdm_symbol_size;

  AssertFatal(dmrss >= 0 && dmrss < 3,
	      "symbol %d is illegal for PBCH DM-RS \n",
	      dmrss);

  symbol_offset = ue->frame_parms.ofdm_symbol_size*symbol;

  unsigned int k = ue->frame_parms.Nid_cell % 4;

#ifdef DEBUG_PBCH
  printf("PBCH DMRS Correlation : gNB_id %d , OFDM size %d, Ncp=%d, Ns=%d, k=%d symbol %d\n", proc->gNB_id, ue->frame_parms.ofdm_symbol_size, ue->frame_parms.Ncp, Ns, k, symbol);
#endif

  // generate pilot
  nr_pbch_dmrs_rx(dmrss,ue->nr_gold_pbch[n_hf][ssb_index], &pilot[0]);

  for (int aarx=0; aarx<ue->frame_parms.nb_antennas_rx; aarx++) {

    int re_offset = ssb_offset;
    pil   = (int16_t *)&pilot[0];
    rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];

#ifdef DEBUG_PBCH
    printf("pbch ch est pilot addr %p RB_DL %d\n",&pilot[0], ue->frame_parms.N_RB_DL);
    printf("k %d, first_carrier %d\n",k,ue->frame_parms.first_carrier_offset);
    printf("rxF addr %p\n", rxF);
#endif
    //if ((ue->frame_parms.N_RB_DL&1)==0) {

    // Treat first 2 pilots specially (left edge)
    ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
    ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

    current_ssb->c_re += ch[0];
    current_ssb->c_im += ch[1];

#ifdef DEBUG_PBCH
    printf("ch 0 %d\n",((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1]));
    printf("pilot 0 : rxF - > (%d,%d) addr %p  ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],&rxF[0],ch[0],ch[1],pil[0],pil[1]);
#endif

    pil += 2;
    re_offset = (re_offset+4) % ue->frame_parms.ofdm_symbol_size;
    rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];


    ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
    ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

    current_ssb->c_re += ch[0];
    current_ssb->c_im += ch[1];

#ifdef DEBUG_PBCH
    printf("pilot 1 : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif

    pil += 2;
    re_offset = (re_offset+4) % ue->frame_parms.ofdm_symbol_size;
    rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];

    current_ssb->c_re += ch[0];
    current_ssb->c_im += ch[1];

#ifdef DEBUG_PBCH
    printf("pilot 2 : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif

    pil += 2;
    re_offset = (re_offset+4) % ue->frame_parms.ofdm_symbol_size;
    rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];

    for (pilot_cnt=3; pilot_cnt<(3*20); pilot_cnt += 3) {

      //	if (pilot_cnt == 30)
      //	  rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k)];

      // in 2nd symbol, skip middle  REs (48 with DMRS,  144 for SSS, and another 48 with DMRS) 
      if (dmrss == 1 && pilot_cnt == 12) {
	pilot_cnt=48;
	re_offset = (re_offset+144) % ue->frame_parms.ofdm_symbol_size;
	rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];
      }
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
      
      current_ssb->c_re += ch[0];
      current_ssb->c_im += ch[1];

#ifdef DEBUG_PBCH
      printf("pilot %u : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif

      pil += 2;
      re_offset = (re_offset+4) % ue->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];
        
  
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

      current_ssb->c_re += ch[0];
      current_ssb->c_im += ch[1];

#ifdef DEBUG_PBCH
      printf("pilot %u : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt+1,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
      pil += 2;
      re_offset = (re_offset+4) % ue->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];
        

      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

      current_ssb->c_re += ch[0];
      current_ssb->c_im += ch[1];

#ifdef DEBUG_PBCH
      printf("pilot %u : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt+2,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif

      pil += 2;
      re_offset = (re_offset+4) % ue->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];

    }


    //}

  }
  return(0);
}

int nr_pbch_channel_estimation(PHY_VARS_NR_UE *ue,
                               int estimateSz,
                               struct complex16 dl_ch_estimates[][estimateSz],
                               struct complex16 dl_ch_estimates_time[][ue->frame_parms.ofdm_symbol_size],
                               const UE_nr_rxtx_proc_t *proc,
                               unsigned char symbol,
                               int dmrss,
                               uint8_t ssb_index,
                               uint8_t n_hf,
                               c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP])
{
  int Ns = proc->nr_slot_rx;
  int pilot[200] __attribute__((aligned(16)));
  unsigned short k;
  unsigned int pilot_cnt;
  int16_t *pil, *rxF, *dl_ch;
  const int16_t *fl, *fm, *fr;
  int ch_offset,symbol_offset;
  //int slot_pbch;

  uint8_t nushift;
  nushift = ue->frame_parms.Nid_cell % 4;
  unsigned int  ssb_offset = ue->frame_parms.first_carrier_offset + ue->frame_parms.ssb_start_subcarrier;
  if (ssb_offset>= ue->frame_parms.ofdm_symbol_size) ssb_offset-=ue->frame_parms.ofdm_symbol_size;

  ch_offset     = ue->frame_parms.ofdm_symbol_size*symbol;

  AssertFatal(dmrss >= 0 && dmrss < 3,
	      "symbol %d is illegal for PBCH DM-RS \n",
	      dmrss);

  symbol_offset = ue->frame_parms.ofdm_symbol_size*symbol;


  k = nushift;

#ifdef DEBUG_PBCH
  printf("PBCH Channel Estimation : gNB_id %d ch_offset %d, OFDM size %d, Ncp=%d, Ns=%d, k=%d symbol %d\n", proc->gNB_id, ch_offset, ue->frame_parms.ofdm_symbol_size, ue->frame_parms.Ncp, Ns, k, symbol);
#endif

  switch (k) {
  case 0:
    fl = filt16a_l0;
    fm = filt16a_m0;
    fr = filt16a_r0;
    break;

  case 1:
    fl = filt16a_l1;
    fm = filt16a_m1;
    fr = filt16a_r1;
    break;

  case 2:
    fl = filt16a_l2;
    fm = filt16a_m2;
    fr = filt16a_r2;
    break;

  case 3:
    fl = filt16a_l3;
    fm = filt16a_m3;
    fr = filt16a_r3;
    break;

  default:
    msg("pbch_channel_estimation: k=%d -> ERROR\n",k);
    return(-1);
    break;
  }

  idft_size_idx_t idftsizeidx;
  
  switch (ue->frame_parms.ofdm_symbol_size) {
  case 128:
    idftsizeidx = IDFT_128;
    break;
    
  case 256:
    idftsizeidx = IDFT_256;
    break;
    
  case 512:
    idftsizeidx = IDFT_512;
    break;
    
  case 768:
    idftsizeidx = IDFT_768;
    break;

  case 1024:
    idftsizeidx = IDFT_1024;
    break;
    
  case 1536:
    idftsizeidx = IDFT_1536;
    break;
    
  case 2048:
    idftsizeidx = IDFT_2048;
    break;
    
  case 3072:
    idftsizeidx = IDFT_3072;
    break;
    
  case 4096:
    idftsizeidx = IDFT_4096;
    break;
  
  case 6144:
    idftsizeidx = IDFT_6144;
    break;
 
  default:
    printf("unsupported ofdm symbol size \n");
    assert(0);
  }
  
  // generate pilot
  nr_pbch_dmrs_rx(dmrss,ue->nr_gold_pbch[n_hf][ssb_index], &pilot[0]);

  for (int aarx=0; aarx<ue->frame_parms.nb_antennas_rx; aarx++) {

    int re_offset = ssb_offset;
    pil   = (int16_t *)&pilot[0];
    rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];
    dl_ch = (int16_t *)&dl_ch_estimates[aarx][ch_offset];

    memset(dl_ch,0,sizeof(struct complex16)*(ue->frame_parms.ofdm_symbol_size));

#ifdef DEBUG_PBCH
    printf("pbch ch est pilot addr %p RB_DL %d\n",&pilot[0], ue->frame_parms.N_RB_DL);
    printf("k %d, first_carrier %d\n",k,ue->frame_parms.first_carrier_offset);
    printf("rxF addr %p\n", rxF);
    printf("dl_ch addr %p\n",dl_ch);
#endif

    // Treat first 2 pilots specially (left edge)
    int16_t ch[2];
    ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
    ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_PBCH
    printf("ch 0 %d\n",((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1]));
    printf("pilot 0 : rxF - > (%d,%d) addr %p  ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],&rxF[0],ch[0],ch[1],pil[0],pil[1]);
#endif
    multadd_real_vector_complex_scalar(fl,
				       ch,
				       dl_ch,
				       16);
    pil += 2;
    re_offset = (re_offset+4) % ue->frame_parms.ofdm_symbol_size;
    rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];

    //for (int i= 0; i<8; i++)
    //printf("dl_ch addr %p %d\n", dl_ch+i, *(dl_ch+i));

    ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
    ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);


#ifdef DEBUG_PBCH
    printf("pilot 1 : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
    multadd_real_vector_complex_scalar(fm,
				       ch,
				       dl_ch,
				       16);
    pil += 2;
    re_offset = (re_offset+4) % ue->frame_parms.ofdm_symbol_size;
    rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];

    ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
    ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_PBCH
    printf("pilot 2 : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif

    multadd_real_vector_complex_scalar(fr,
				       ch,
				       dl_ch,
				       16);
    pil += 2;
    re_offset = (re_offset+4) % ue->frame_parms.ofdm_symbol_size;
    rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];
    dl_ch += 24;

    for (pilot_cnt=3; pilot_cnt<(3*20); pilot_cnt += 3) {

      //	if (pilot_cnt == 30)
      //	  rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k)];

      // in 2nd symbol, skip middle  REs (48 with DMRS,  144 for SSS, and another 48 with DMRS) 
      if (dmrss == 1 && pilot_cnt == 12) {
        pilot_cnt=48;
        re_offset = (re_offset+144) % ue->frame_parms.ofdm_symbol_size;
        rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];
        dl_ch += 288;
      }
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_PBCH
      printf("pilot %u : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
      multadd_real_vector_complex_scalar(fl,
					 ch,
					 dl_ch,
					 16);

      //for (int i= 0; i<8; i++)
      //            printf("pilot_cnt %d dl_ch %d %d\n", pilot_cnt, dl_ch+i, *(dl_ch+i));

      pil += 2;
      re_offset = (re_offset+4) % ue->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];
        
  
      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_PBCH
      printf("pilot %u : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt+1,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
      multadd_real_vector_complex_scalar(fm,
					 ch,
					 dl_ch,
					 16);
      pil += 2;
      re_offset = (re_offset+4) % ue->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];

      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_PBCH
      printf("pilot %u : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt+2,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif

      multadd_real_vector_complex_scalar(fr,
					 ch,
					 dl_ch,
					 16);
      pil += 2;
      re_offset = (re_offset+4) % ue->frame_parms.ofdm_symbol_size;
      rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+re_offset)];
      dl_ch += 24;

    }

    if( dmrss == 2) // update time statistics for last PBCH symbol
    {
      // do ifft of channel estimate
      LOG_D(PHY,"Channel Impulse Computation Slot %d Symbol %d ch_offset %d\n", Ns, symbol, ch_offset);
      idft(idftsizeidx,
	   (int16_t*) &dl_ch_estimates[aarx][ch_offset],
	   (int16_t*) dl_ch_estimates_time[aarx],
	   1);
    }
  }

  if (dmrss == 2)
    UEscopeCopy(ue,
                pbchDlChEstimateTime,
                (void *)dl_ch_estimates_time,
                sizeof(c16_t),
                ue->frame_parms.nb_antennas_rx,
                ue->frame_parms.ofdm_symbol_size,
                0);

  return(0);
}

void nr_pdcch_channel_estimation(PHY_VARS_NR_UE *ue,
                                 const UE_nr_rxtx_proc_t *proc,
                                 unsigned char symbol,
                                 fapi_nr_coreset_t *coreset,
                                 uint16_t first_carrier_offset,
                                 uint16_t BWPStart,
                                 int32_t pdcch_est_size,
                                 int32_t pdcch_dl_ch_estimates[][pdcch_est_size],
                                 c16_t rxdataF[][ue->frame_parms.samples_per_slot_wCP])
{

  int Ns = proc->nr_slot_rx;
  int gNB_id = proc->gNB_id;
  unsigned char aarx;
  unsigned short k;
  unsigned int pilot_cnt;
  int16_t ch[2],*pil,*rxF,*dl_ch;
  int ch_offset,symbol_offset;

  ch_offset     = ue->frame_parms.ofdm_symbol_size*symbol;

  symbol_offset = ue->frame_parms.ofdm_symbol_size*symbol;

  int nb_rb_coreset=0;
  int coreset_start_rb=0;
  get_coreset_rballoc(coreset->frequency_domain_resource,&nb_rb_coreset,&coreset_start_rb);
  if(nb_rb_coreset==0) return;

#ifdef DEBUG_PDCCH
  printf("pdcch_channel_estimation: first_carrier_offset %d, BWPStart %d, coreset_start_rb %d, coreset_nb_rb %d\n",
         first_carrier_offset, BWPStart, coreset_start_rb, nb_rb_coreset);
#endif

  unsigned short coreset_start_subcarrier = first_carrier_offset+(BWPStart + coreset_start_rb)*12;

#ifdef DEBUG_PDCCH
  printf("PDCCH Channel Estimation : gNB_id %d ch_offset %d, OFDM size %d, Ncp=%d, Ns=%d, symbol %d\n",
         gNB_id,ch_offset,ue->frame_parms.ofdm_symbol_size,ue->frame_parms.Ncp,Ns,symbol);
#endif

#if CH_INTERP
  int16_t *fl = filt16a_l1;
  int16_t *fm = filt16a_m1;
  int16_t *fr = filt16a_r1;
#endif

  unsigned short scrambling_id = coreset->pdcch_dmrs_scrambling_id;
  // checking if re-initialization of scrambling IDs is needed (should be done here but scrambling ID for PDCCH is not taken from RRC)
  if (scrambling_id != ue->scramblingID_pdcch){
    ue->scramblingID_pdcch = scrambling_id;
    nr_gold_pdcch(ue,ue->scramblingID_pdcch);
  }

  int dmrs_ref = 0;
  if (coreset->CoreSetType == NFAPI_NR_CSET_CONFIG_PDCCH_CONFIG)
    dmrs_ref = BWPStart;
  // generate pilot
  int pilot[(nb_rb_coreset + dmrs_ref) * 3] __attribute__((aligned(16)));
  nr_pdcch_dmrs_rx(ue,Ns,ue->nr_gold_pdcch[gNB_id][Ns][symbol], &pilot[0],2000,(nb_rb_coreset+dmrs_ref));

  for (aarx=0; aarx<ue->frame_parms.nb_antennas_rx; aarx++) {

    k = coreset_start_subcarrier;
    pil   = (int16_t *)&pilot[dmrs_ref*3];
    rxF   = (int16_t *)&rxdataF[aarx][(symbol_offset+k+1)];
    dl_ch = (int16_t *)&pdcch_dl_ch_estimates[aarx][ch_offset];

    memset(dl_ch,0,4*(ue->frame_parms.ofdm_symbol_size));

#ifdef DEBUG_PDCCH
    printf("pdcch ch est pilot addr %p RB_DL %d\n",&pilot[dmrs_ref*3], ue->frame_parms.N_RB_DL);
    printf("k %d, first_carrier %d\n",k,ue->frame_parms.first_carrier_offset);
    printf("rxF addr %p\n", rxF);

    printf("dl_ch addr %p\n",dl_ch);
#endif
  #if CH_INTERP
    //    if ((ue->frame_parms.N_RB_DL&1)==0) {
    // Treat first 2 pilots specially (left edge)
    ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
    ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_PDCCH
    printf("ch 0 %d\n",((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1]));
    printf("pilot 0 : rxF - > (%d,%d) addr %p  ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],&rxF[0],ch[0],ch[1],pil[0],pil[1]);
#endif
    multadd_real_vector_complex_scalar(fl,
				       ch,
				       dl_ch,
				       16);
    pil += 2;
    rxF += 8;
    k   += 4;

    if (k >= ue->frame_parms.ofdm_symbol_size) {
      k  -= ue->frame_parms.ofdm_symbol_size;
      rxF = (int16_t *)&rxdataF[aarx][(symbol_offset+k+1)];
    }

    ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
    ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_PDCCH
    printf("pilot 1 : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
    multadd_real_vector_complex_scalar(fm,
				       ch,
				       dl_ch,
				       16);
    pil += 2;
    rxF += 8;
    k   += 4;

    if (k >= ue->frame_parms.ofdm_symbol_size) {
      k  -= ue->frame_parms.ofdm_symbol_size;
      rxF = (int16_t *)&rxdataF[aarx][(symbol_offset+k+1)];
    }

    ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
    ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_PDCCH
    printf("pilot 2 : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif

    multadd_real_vector_complex_scalar(fr,
				       ch,
				       dl_ch,
				       16);
#ifdef DEBUG_PDCCH
    for (int m =0; m<12; m++)
      printf("data :  dl_ch -> (%d,%d)\n",dl_ch[0+2*m],dl_ch[1+2*m]);
#endif
    dl_ch += 24;

    pil += 2;
    rxF += 8;
    k   += 4;

    for (pilot_cnt=3; pilot_cnt<(3*nb_rb_coreset); pilot_cnt += 3) {

      if (k >= ue->frame_parms.ofdm_symbol_size) {
        k  -= ue->frame_parms.ofdm_symbol_size;
        rxF = (int16_t *)&rxdataF[aarx][(symbol_offset+k+1)];
      }

      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_PDCCH
      printf("pilot %u : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
      multadd_real_vector_complex_scalar(fl,
					 ch,
					 dl_ch,
					 16);

      //for (int i= 0; i<8; i++)
      //            printf("pilot_cnt %d dl_ch %d %d\n", pilot_cnt, dl_ch+i, *(dl_ch+i));

      pil += 2;
      rxF += 8;
      k   += 4;

      if (k >= ue->frame_parms.ofdm_symbol_size) {
        k  -= ue->frame_parms.ofdm_symbol_size;
        rxF = (int16_t *)&rxdataF[aarx][(symbol_offset+k+1)];
      }

      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
#ifdef DEBUG_PDCCH
      printf("pilot %u : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt+1,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif
      multadd_real_vector_complex_scalar(fm,
					 ch,
					 dl_ch,
					 16);
      pil += 2;
      rxF += 8;
      k   += 4;

      if (k >= ue->frame_parms.ofdm_symbol_size) {
        k  -= ue->frame_parms.ofdm_symbol_size;
        rxF = (int16_t *)&rxdataF[aarx][(symbol_offset+k+1)];
      }

      ch[0] = (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch[1] = (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);

#ifdef DEBUG_PDCCH
      printf("pilot %u : rxF - > (%d,%d) ch -> (%d,%d), pil -> (%d,%d) \n",pilot_cnt+2,rxF[0],rxF[1],ch[0],ch[1],pil[0],pil[1]);
#endif

      multadd_real_vector_complex_scalar(fr,
					 ch,
					 dl_ch,
					 16);
#ifdef DEBUG_PDCCH
    for (int m =0; m<12; m++)
      printf("data :  dl_ch -> (%d,%d)\n",dl_ch[0+2*m],dl_ch[1+2*m]);
#endif
      dl_ch += 24;

      pil += 2;
      rxF += 8;
      k   += 4;
    }
  #else //ELSE CH_INTERP
    int ch_sum[2] = {0, 0};

    for (pilot_cnt = 0; pilot_cnt < 3*nb_rb_coreset; pilot_cnt++) {
      if (k >= ue->frame_parms.ofdm_symbol_size) {
        k  -= ue->frame_parms.ofdm_symbol_size;
        rxF = (int16_t *)&rxdataF[aarx][(symbol_offset+k+1)];
      }
#ifdef DEBUG_PDCCH
      printf("pilot[%u] = (%d, %d)\trxF[%d] = (%d, %d)\n", pilot_cnt, pil[0], pil[1], k+1, rxF[0], rxF[1]);
#endif
      ch_sum[0] += (int16_t)(((int32_t)pil[0]*rxF[0] - (int32_t)pil[1]*rxF[1])>>15);
      ch_sum[1] += (int16_t)(((int32_t)pil[0]*rxF[1] + (int32_t)pil[1]*rxF[0])>>15);
      pil += 2;
      rxF += 8;
      k   += 4;

      if (pilot_cnt % 3 == 2) {
        ch[0] = ch_sum[0] / 3;
        ch[1] = ch_sum[1] / 3;
        multadd_real_vector_complex_scalar(filt16a_1, ch, dl_ch, 16);
#ifdef DEBUG_PDCCH
        for (int m =0; m<12; m++)
          printf("data :  dl_ch -> (%d,%d)\n",dl_ch[0+2*m],dl_ch[1+2*m]);
#endif
        dl_ch += 24;
        ch_sum[0] = 0;
        ch_sum[1] = 0;
      }
    }
  #endif //END CH_INTERP


    //}

  }
}

void NFAPI_NR_DMRS_TYPE1_linear_interp(NR_DL_FRAME_PARMS *frame_parms,
                                       c16_t *rxF,
                                       c16_t *pil,
                                       c16_t *dl_ch,
                                       unsigned short bwp_start_subcarrier,
                                       unsigned short nb_rb_pdsch,
                                       int8_t delta,
                                       delay_t *delay)
{
  c16_t *dl_ch0 = dl_ch;
  int re_offset = bwp_start_subcarrier % frame_parms->ofdm_symbol_size;

  c16_t dl_ls_est[frame_parms->ofdm_symbol_size] __attribute__((aligned(32)));
  memset(dl_ls_est, 0, sizeof(dl_ls_est));

  for (int pilot_cnt = 0; pilot_cnt < 6 * nb_rb_pdsch; pilot_cnt++) {
    if (pilot_cnt % 2 == 0) {
      c16_t ch = c16mulShift(*pil, rxF[re_offset], 15);
      pil++;
      re_offset = (re_offset + 2) % frame_parms->ofdm_symbol_size;
      ch = c16maddShift(*pil, rxF[re_offset], ch, 15);
      ch = c16Shift(ch, 1);
      pil++;
      re_offset = (re_offset + 2) % frame_parms->ofdm_symbol_size;
      for (int k = pilot_cnt << 1; k < (pilot_cnt << 1) + 4; k++) {
        dl_ls_est[k] = ch;
      }
    }

#ifdef DEBUG_PDSCH
    printf("pilot %3d: pil -> (%6d,%6d), rxF -> (%4d,%4d), ch -> (%4d,%4d) \n",
           pilot_cnt,
           pil->r,
           pil->i,
           rxF[re_offset].r,
           rxF[re_offset].i,
           dl_ls_est[pilot_cnt << 1].r,
           dl_ls_est[pilot_cnt << 1].i);
#endif
  }

  c16_t ch_estimates_time[frame_parms->ofdm_symbol_size] __attribute__((aligned(32)));
  memset(ch_estimates_time, 0, sizeof(ch_estimates_time));
  nr_est_delay(frame_parms->ofdm_symbol_size, dl_ls_est, ch_estimates_time, delay);
  int delay_idx = get_delay_idx(delay->est_delay, MAX_DELAY_COMP);
  c16_t *dl_delay_table = frame_parms->delay_table[delay_idx];

  for (int pilot_cnt = 0; pilot_cnt < 6 * nb_rb_pdsch; pilot_cnt++) {
    int k = pilot_cnt << 1;
    c16_t ch = c16mulShift(dl_ls_est[k], dl_delay_table[k], 8);
    if (pilot_cnt == 0) { // Treat first pilot
      c16multaddVectRealComplex(filt16_ul_p0, &ch, dl_ch, 16);
    } else if (pilot_cnt == 1 || pilot_cnt == 2) {
      c16multaddVectRealComplex(filt16_ul_p1p2, &ch, dl_ch, 16);
    } else if (pilot_cnt == 6 * nb_rb_pdsch - 1) { // Treat last pilot
      c16multaddVectRealComplex(filt16_ul_last, &ch, dl_ch, 16);
    } else { // Treat middle pilots
      c16multaddVectRealComplex(filt16_ul_middle, &ch, dl_ch, 16);
      if (pilot_cnt % 2 == 0) {
        dl_ch += 4;
      }
    }
  }

  // Revert delay
  dl_ch = dl_ch0;
  int inv_delay_idx = get_delay_idx(-delay->est_delay, MAX_DELAY_COMP);
  c16_t *dl_inv_delay_table = frame_parms->delay_table[inv_delay_idx];
  for (int k = 0; k < 12 * nb_rb_pdsch; k++) {
    dl_ch[k] = c16mulShift(dl_ch[k], dl_inv_delay_table[k], 8);
  }
}

void NFAPI_NR_DMRS_TYPE1_average_prb(NR_DL_FRAME_PARMS *frame_parms,
                                     c16_t *rxF,
                                     c16_t *pil,
                                     c16_t *dl_ch,
                                     unsigned short bwp_start_subcarrier,
                                     unsigned short nb_rb_pdsch)
{
  int re_offset = bwp_start_subcarrier % frame_parms->ofdm_symbol_size;
  c16_t ch = {0};
  int P_average = 6;

  c32_t ch32 = {0};
  for (int p_av = 0; p_av < P_average; p_av++) {
    ch32 = c32x16maddShift(*pil, rxF[re_offset], ch32, 15);
    pil++;
    re_offset = (re_offset + 2) % frame_parms->ofdm_symbol_size;
  }
  ch = c16x32div(ch32, P_average);

#if NO_INTERP
  for (int i = 0; i < 2 * P_average; i++) {
    dl_ch[i] = ch;
  }
  dl_ch += 2 * P_average;
#else
  c16multaddVectRealComplex(filt8_avlip0, &ch, dl_ch, 8);
  dl_ch += 16;
  c16multaddVectRealComplex(filt8_avlip1, &ch, dl_ch, 8);
  dl_ch += 16;
  c16multaddVectRealComplex(filt8_avlip2, &ch, dl_ch, 8);
  dl_ch -= 24;
#endif

  for (int pilot_cnt = P_average; pilot_cnt < 6 * (nb_rb_pdsch - 1); pilot_cnt += P_average) {
    ch32.r = 0;
    ch32.i = 0;
    for (int p_av = 0; p_av < P_average; p_av++) {
      ch32 = c32x16maddShift(*pil, rxF[re_offset], ch32, 15);
      pil++;
      re_offset = (re_offset + 2) % frame_parms->ofdm_symbol_size;
    }
    ch = c16x32div(ch32, P_average);

#if NO_INTERP
    for (int i = 0; i < 2 * P_average; i++) {
      dl_ch[i] = ch;
    }
    dl_ch += 2 * P_average;
#else
    dl_ch[3].r += (ch.r * 1365) >> 15; // 1/12*16384
    dl_ch[3].i += (ch.i * 1365) >> 15; // 1/12*16384
    dl_ch += 4;
    c16multaddVectRealComplex(filt8_avlip3, &ch, dl_ch, 8);
    dl_ch += 8;
    c16multaddVectRealComplex(filt8_avlip4, &ch, dl_ch, 8);
    dl_ch += 8;
    c16multaddVectRealComplex(filt8_avlip5, &ch, dl_ch, 8);
    dl_ch -= 8;
#endif
  }

  ch32.r = 0;
  ch32.i = 0;
  for (int p_av = 0; p_av < P_average; p_av++) {
    ch32 = c32x16maddShift(*pil, rxF[re_offset], ch32, 15);
    pil++;
    re_offset = (re_offset + 2) % frame_parms->ofdm_symbol_size;
  }
  ch = c16x32div(ch32, P_average);

#if NO_INTERP
  for (int i = 0; i < 2 * P_average; i++) {
    dl_ch[i] = ch;
  }
  dl_ch += 2 * P_average;
#else
  dl_ch[3].r += (ch.r * 1365) >> 15; // 1/12*16384
  dl_ch[3].i += (ch.i * 1365) >> 15; // 1/12*16384
  dl_ch += 4;
  c16multaddVectRealComplex(filt8_avlip3, &ch, dl_ch, 8);
  dl_ch += 8;
  c16multaddVectRealComplex(filt8_avlip6, &ch, dl_ch, 8);
#endif
}

void NFAPI_NR_DMRS_TYPE2_linear_interp(NR_DL_FRAME_PARMS *frame_parms,
                                       c16_t *rxF,
                                       c16_t *pil,
                                       c16_t *dl_ch,
                                       unsigned short bwp_start_subcarrier,
                                       unsigned short nb_rb_pdsch,
                                       int8_t delta,
                                       unsigned short p,
                                       delay_t *delay)
{
  int re_offset = bwp_start_subcarrier % frame_parms->ofdm_symbol_size;
  c16_t *dl_ch0 = dl_ch;

  c16_t dl_ls_est[frame_parms->ofdm_symbol_size] __attribute__((aligned(32)));
  memset(dl_ls_est, 0, sizeof(dl_ls_est));

  for (int pilot_cnt = 0; pilot_cnt < 4 * nb_rb_pdsch; pilot_cnt += 2) {
    c16_t ch_l = c16mulShift(*pil, rxF[re_offset], 15);
#ifdef DEBUG_PDSCH
    printf("pilot %3d: pil -> (%6d,%6d), rxF -> (%4d,%4d), ch -> (%4d,%4d) \n", pilot_cnt, pil->r, pil->i, rxF[re_offset].r, rxF[re_offset].i, ch_l.r, ch_l.i);
#endif
    pil++;
    re_offset = (re_offset + 1) % frame_parms->ofdm_symbol_size;
    c16_t ch_r = c16mulShift(*pil, rxF[re_offset], 15);
#ifdef DEBUG_PDSCH
    printf("pilot %3d: pil -> (%6d,%6d), rxF -> (%4d,%4d), ch -> (%4d,%4d) \n", pilot_cnt, pil->r, pil->i, rxF[re_offset].r, rxF[re_offset].i, ch_r.r, ch_r.i);
#endif
    c16_t ch = c16addShift(ch_l, ch_r, 1);
    for (int k = 3 * pilot_cnt; k < (3 * pilot_cnt) + 6; k++) {
      dl_ls_est[k] = ch;
    }
    pil++;
    re_offset = (re_offset + 5) % frame_parms->ofdm_symbol_size;
  }

  c16_t ch_estimates_time[frame_parms->ofdm_symbol_size] __attribute__((aligned(32)));
  memset(ch_estimates_time, 0, sizeof(ch_estimates_time));
  nr_est_delay(frame_parms->ofdm_symbol_size, dl_ls_est, ch_estimates_time, delay);
  int delay_idx = get_delay_idx(delay->est_delay, MAX_DELAY_COMP);
  c16_t *dl_delay_table = frame_parms->delay_table[delay_idx];

  for (int pilot_cnt = 0; pilot_cnt < 6 * nb_rb_pdsch; pilot_cnt++) {
    int k = (pilot_cnt / 3) * 6;
    c16_t ch = c16mulShift(dl_ls_est[k], dl_delay_table[k], 8);
    if (pilot_cnt == 0) { // Treat first pilot
      c16multaddVectRealComplex(filt16_ul_p0, &ch, dl_ch, 16);
    } else if (pilot_cnt == 1 || pilot_cnt == 2) {
      c16multaddVectRealComplex(filt16_ul_p1p2, &ch, dl_ch, 16);
    } else if (pilot_cnt == 6 * nb_rb_pdsch - 1) { // Treat last pilot
      c16multaddVectRealComplex(filt16_ul_last, &ch, dl_ch, 16);
    } else { // Treat middle pilots
      c16multaddVectRealComplex(filt16_ul_middle, &ch, dl_ch, 16);
      if (pilot_cnt % 2 == 0) {
        dl_ch += 4;
      }
    }
  }

  // Revert delay
  dl_ch = dl_ch0;
  int inv_delay_idx = get_delay_idx(-delay->est_delay, MAX_DELAY_COMP);
  c16_t *dl_inv_delay_table = frame_parms->delay_table[inv_delay_idx];
  for (int k = 0; k < 12 * nb_rb_pdsch; k++) {
    dl_ch[k] = c16mulShift(dl_ch[k], dl_inv_delay_table[k], 8);
  }
}

void NFAPI_NR_DMRS_TYPE2_average_prb(NR_DL_FRAME_PARMS *frame_parms,
                                     c16_t *rxF,
                                     c16_t *pil,
                                     c16_t *dl_ch,
                                     unsigned short bwp_start_subcarrier,
                                     unsigned short nb_rb_pdsch)
{
  int re_offset = bwp_start_subcarrier % frame_parms->ofdm_symbol_size;
  c16_t ch = {0};
  int P_average = 4;

  c32_t ch32 = {0};
  for (int p_av = 0; p_av < P_average; p_av++) {
    ch32 = c32x16maddShift(*pil, rxF[re_offset], ch32, 15);
    pil++;
    re_offset = (re_offset + 1) % frame_parms->ofdm_symbol_size;
  }
  ch = c16x32div(ch32, P_average);

#if NO_INTERP
  for (int i = 0; i < 3 * P_average; i++) {
    dl_ch[i] = ch;
  }
  dl_ch += 3 * P_average;
#else
  c16multaddVectRealComplex(filt8_avlip0, &ch, dl_ch, 8);
  dl_ch += 8;
  c16multaddVectRealComplex(filt8_avlip1, &ch, dl_ch, 8);
  dl_ch += 8;
  c16multaddVectRealComplex(filt8_avlip2, &ch, dl_ch, 8);
  dl_ch -= 12;
#endif

  for (int pilot_cnt = P_average; pilot_cnt < 4 * (nb_rb_pdsch - 1); pilot_cnt += P_average) {
    ch32.r = 0;
    ch32.i = 0;
    for (int p_av = 0; p_av < P_average; p_av++) {
      ch32 = c32x16maddShift(*pil, rxF[re_offset], ch32, 15);
      pil++;
      re_offset = (re_offset + 5) % frame_parms->ofdm_symbol_size;
    }
    ch = c16x32div(ch32, P_average);

#if NO_INTERP
    for (int i = 0; i < 3 * P_average; i++) {
      dl_ch[i] = ch;
    }
    dl_ch += 3 * P_average;
#else
    dl_ch[3].r += (ch.r * 1365) >> 15; // 1/12*16384
    dl_ch[3].i += (ch.i * 1365) >> 15; // 1/12*16384
    dl_ch += 4;
    c16multaddVectRealComplex(filt8_avlip3, &ch, dl_ch, 8);
    dl_ch += 8;
    c16multaddVectRealComplex(filt8_avlip4, &ch, dl_ch, 8);
    dl_ch += 8;
    c16multaddVectRealComplex(filt8_avlip5, &ch, dl_ch, 8);
    dl_ch -= 8;
#endif
  }

  ch32.r = 0;
  ch32.i = 0;
  for (int p_av = 0; p_av < P_average; p_av++) {
    ch32 = c32x16maddShift(*pil, rxF[re_offset], ch32, 15);
    pil++;
    re_offset = (re_offset + 5) % frame_parms->ofdm_symbol_size;
  }
  ch = c16x32div(ch32, P_average);

#if NO_INTERP
  for (int i = 0; i < 3 * P_average; i++) {
    dl_ch[i] = ch;
  }
  dl_ch += 3 * P_average;
#else
  dl_ch[3].r += (ch.r * 1365) >> 15; // 1/12*16384
  dl_ch[3].i += (ch.i * 1365) >> 15; // 1/12*16384
  dl_ch += 4;
  c16multaddVectRealComplex(filt8_avlip3, &ch, dl_ch, 8);
  dl_ch += 8;
  c16multaddVectRealComplex(filt8_avlip6, &ch, dl_ch, 8);
#endif
}
int nr_pdsch_channel_estimation(PHY_VARS_NR_UE *ue,
                                const UE_nr_rxtx_proc_t *proc,
                                unsigned short p,
                                unsigned char symbol,
                                unsigned char nscid,
                                unsigned short scrambling_id,
                                unsigned short BWPStart,
                                uint8_t config_type,
                                uint16_t rb_offset,
                                unsigned short bwp_start_subcarrier,
                                unsigned short nb_rb_pdsch,
                                uint32_t pdsch_est_size,
                                int32_t dl_ch_estimates[][pdsch_est_size],
                                int rxdataFsize,
                                c16_t rxdataF[][rxdataFsize])
{
  int gNB_id = proc->gNB_id;
  int Ns = proc->nr_slot_rx;
  const int ch_offset = ue->frame_parms.ofdm_symbol_size * symbol;
  const int symbol_offset = ue->frame_parms.ofdm_symbol_size * symbol;

#ifdef DEBUG_PDSCH
  printf("PDSCH Channel Estimation : gNB_id %d ch_offset %d, symbol_offset %d OFDM size %d, Ncp=%d, Ns=%d, bwp_start_subcarrier=%d symbol %d\n",
         gNB_id,
         ch_offset,
         symbol_offset,
         ue->frame_parms.ofdm_symbol_size,
         ue->frame_parms.Ncp,
         Ns,
         bwp_start_subcarrier,
         symbol);
#endif

  // generate pilot for gNB port number 1000+p
  int8_t delta = get_delta(p, config_type);

  // checking if re-initialization of scrambling IDs is needed
  if (scrambling_id != ue->scramblingID_dlsch[nscid]) {
    ue->scramblingID_dlsch[nscid] = scrambling_id;
    nr_gold_pdsch(ue, nscid, scrambling_id);
  }

  c16_t pilot[3280] __attribute__((aligned(16)));
  nr_pdsch_dmrs_rx(ue, Ns, ue->nr_gold_pdsch[gNB_id][Ns][symbol][nscid], (int32_t *)pilot, 1000 + p, 0, nb_rb_pdsch + rb_offset, config_type);

  uint8_t nushift = 0;
  if (config_type == NFAPI_NR_DMRS_TYPE1) {
    nushift = (p >> 1) & 1;
  } else { // NFAPI_NR_DMRS_TYPE2
    nushift = delta;
  }

  delay_t delay = {0};

  for (int aarx = 0; aarx < ue->frame_parms.nb_antennas_rx; aarx++) {

#ifdef DEBUG_PDSCH
    printf("\n============================================\n");
    printf("==== Tx port %i, Rx antenna %i, Symbol %i ====\n", p, aarx, symbol);
    printf("============================================\n");
#endif

    c16_t *rxF = &rxdataF[aarx][symbol_offset + nushift];
    c16_t *dl_ch = (c16_t *)&dl_ch_estimates[p * ue->frame_parms.nb_antennas_rx + aarx][ch_offset];
    memset(dl_ch, 0, sizeof(*dl_ch) * ue->frame_parms.ofdm_symbol_size);

    if (config_type == NFAPI_NR_DMRS_TYPE1 && ue->chest_freq == 0) {
      NFAPI_NR_DMRS_TYPE1_linear_interp(&ue->frame_parms,
                                        rxF,
                                        &pilot[6 * rb_offset],
                                        dl_ch,
                                        bwp_start_subcarrier,
                                        nb_rb_pdsch,
                                        delta,
                                        &delay);

    } else if (config_type == NFAPI_NR_DMRS_TYPE2 && ue->chest_freq == 0) {
      NFAPI_NR_DMRS_TYPE2_linear_interp(&ue->frame_parms,
                                        rxF,
                                        &pilot[4 * rb_offset],
                                        dl_ch,
                                        bwp_start_subcarrier,
                                        nb_rb_pdsch,
                                        delta,
                                        p,
                                        &delay);

    } else if (config_type == NFAPI_NR_DMRS_TYPE1) {
      NFAPI_NR_DMRS_TYPE1_average_prb(&ue->frame_parms,
                                      rxF,
                                      &pilot[6 * rb_offset],
                                      dl_ch,
                                      bwp_start_subcarrier,
                                      nb_rb_pdsch);

    } else {
      NFAPI_NR_DMRS_TYPE2_average_prb(&ue->frame_parms,
                                      rxF,
                                      &pilot[4 * rb_offset],
                                      dl_ch,
                                      bwp_start_subcarrier,
                                      nb_rb_pdsch);
    }

#ifdef DEBUG_PDSCH
    dl_ch = (c16_t *)&dl_ch_estimates[p * ue->frame_parms.nb_antennas_rx + aarx][ch_offset];
    for (uint16_t idxP = 0; idxP < ceil((float)nb_rb_pdsch * 12 / 8); idxP++) {
      for (uint8_t idxI = 0; idxI < 8; idxI++) {
        printf("%4d\t%4d\t", dl_ch[idxP * 8 + idxI].r, dl_ch[idxP * 8 + idxI].i);
      }
      printf("%2d\n", idxP);
    }
#endif
  }
  return 0;
}

/*******************************************************************
 *
 * NAME :         nr_pdsch_ptrs_processing
 *
 * PARAMETERS :   PHY_VARS_NR_UE    : ue data structure
 *                c16_t             : ptrs_phase_per_slot array
 *                int32_t           : ptrs_re_per_slot array
 *                uint32_t          : rx_size,
 *                int32_t           : rxdataF_comp, array
 *                NR_DL_FRAME_PARMS : frame_parms pointer
 *                NR_DL_UE_HARQ_t   : dlsch0_harq pointer
 *                NR_DL_UE_HARQ_t   : dlsch1_harq pointer
 *                uint8_t           : gNB_id,
 *                uint8_t           : nr_slot_rx,
 *                unsigned char     : symbol,
 *                uint32_t          : nb_re_pdsch,
 *                uint16_t          : rnti
 *                RX_type_t         : rx_type
 * RETURN : Nothing
 *
 * DESCRIPTION :
 *  If ptrs is enabled process the symbol accordingly
 *  1) Estimate common phase error per PTRS symbol
 *  2) Interpolate PTRS estimated value in TD after all PTRS symbols
 *  3) Compensate signal with PTRS estimation for slot
 *********************************************************************/
void nr_pdsch_ptrs_processing(PHY_VARS_NR_UE *ue,
                              int nbRx,
                              c16_t ptrs_phase_per_slot[][14],
                              int32_t ptrs_re_per_slot[][14],
                              uint32_t rx_size_symbol,
                              int32_t rxdataF_comp[][nbRx][rx_size_symbol * NR_SYMBOLS_PER_SLOT],
                              NR_DL_FRAME_PARMS *frame_parms,
                              NR_DL_UE_HARQ_t *dlsch0_harq,
                              NR_DL_UE_HARQ_t *dlsch1_harq,
                              uint8_t gNB_id,
                              uint8_t nr_slot_rx,
                              unsigned char symbol,
                              uint32_t nb_re_pdsch,
                              uint16_t rnti,
                              NR_UE_DLSCH_t dlsch[2])
{
  //#define DEBUG_DL_PTRS 1
  int32_t *ptrs_re_symbol = NULL;
  int8_t   ret = 0;
  /* harq specific variables */
  uint8_t  symbInSlot       = 0;
  uint16_t *startSymbIndex  = NULL;
  uint16_t *nbSymb          = NULL;
  uint8_t  *L_ptrs          = NULL;
  uint8_t  *K_ptrs          = NULL;
  uint16_t *dmrsSymbPos     = NULL;
  uint16_t *ptrsSymbPos     = NULL;
  uint8_t  *ptrsSymbIdx     = NULL;
  uint8_t  *ptrsReOffset    = NULL;
  uint16_t *nb_rb           = NULL;
  int nscid = 0;

  if(dlsch0_harq->status == ACTIVE) {
    symbInSlot      = dlsch[0].dlsch_config.start_symbol + dlsch[0].dlsch_config.number_symbols;
    startSymbIndex  = &dlsch[0].dlsch_config.start_symbol;
    nbSymb          = &dlsch[0].dlsch_config.number_symbols;
    L_ptrs          = &dlsch[0].dlsch_config.PTRSTimeDensity;
    K_ptrs          = &dlsch[0].dlsch_config.PTRSFreqDensity;
    dmrsSymbPos     = &dlsch[0].dlsch_config.dlDmrsSymbPos;
    ptrsReOffset    = &dlsch[0].dlsch_config.PTRSReOffset;
    nb_rb           = &dlsch[0].dlsch_config.number_rbs;
    ptrsSymbPos     = &dlsch[0].ptrs_symbols;
    ptrsSymbIdx     = &dlsch[0].ptrs_symbol_index;
    nscid = dlsch[0].dlsch_config.nscid;
  }
  if(dlsch1_harq) {
    symbInSlot      = dlsch[1].dlsch_config.start_symbol + dlsch[1].dlsch_config.number_symbols;
    startSymbIndex  = &dlsch[1].dlsch_config.start_symbol;
    nbSymb          = &dlsch[1].dlsch_config.number_symbols;
    L_ptrs          = &dlsch[1].dlsch_config.PTRSTimeDensity;
    K_ptrs          = &dlsch[1].dlsch_config.PTRSFreqDensity;
    dmrsSymbPos     = &dlsch[1].dlsch_config.dlDmrsSymbPos;
    ptrsReOffset    = &dlsch[1].dlsch_config.PTRSReOffset;
    nb_rb           = &dlsch[1].dlsch_config.number_rbs;
    ptrsSymbPos     = &dlsch[1].ptrs_symbols;
    ptrsSymbIdx     = &dlsch[1].ptrs_symbol_index;
    nscid = dlsch[1].dlsch_config.nscid;
  }
  /* loop over antennas */
  for (int aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
    c16_t *phase_per_symbol = (c16_t*)ptrs_phase_per_slot[aarx];
    ptrs_re_symbol = (int32_t*)ptrs_re_per_slot[aarx];
    ptrs_re_symbol[symbol] = 0;
    phase_per_symbol[symbol].i = 0; // Imag
    /* set DMRS estimates to 0 angle with magnitude 1 */
    if(is_dmrs_symbol(symbol,*dmrsSymbPos)) {
      /* set DMRS real estimation to 32767 */
      phase_per_symbol[symbol].r=INT16_MAX; // 32767
#ifdef DEBUG_DL_PTRS
      printf("[PHY][PTRS]: DMRS Symbol %d -> %4d + j*%4d\n", symbol, phase_per_symbol[symbol].r,phase_per_symbol[symbol].i);
#endif
    }
    else { // real ptrs value is set to 0
      phase_per_symbol[symbol].r = 0; // Real
    }

    if(dlsch0_harq->status == ACTIVE) {
      if(symbol == *startSymbIndex) {
        *ptrsSymbPos = 0;
        set_ptrs_symb_idx(ptrsSymbPos,
                          *nbSymb,
                          *startSymbIndex,
                          1<< *L_ptrs,
                          *dmrsSymbPos);
      }
      /* if not PTRS symbol set current ptrs symbol index to zero*/
      *ptrsSymbIdx = 0;
      /* Check if current symbol contains PTRS */
      if(is_ptrs_symbol(symbol, *ptrsSymbPos)) {
        *ptrsSymbIdx = symbol;
        /*------------------------------------------------------------------------------------------------------- */
        /* 1) Estimate common phase error per PTRS symbol                                                                */
        /*------------------------------------------------------------------------------------------------------- */
        nr_ptrs_cpe_estimation(*K_ptrs,
                               *ptrsReOffset,
                               *nb_rb,
                               rnti,
                               nr_slot_rx,
                               symbol,
                               frame_parms->ofdm_symbol_size,
                               (int16_t *)(rxdataF_comp[0][aarx] + symbol * nb_re_pdsch),
                               ue->nr_gold_pdsch[gNB_id][nr_slot_rx][symbol][nscid],
                               (int16_t*)&phase_per_symbol[symbol],
                               &ptrs_re_symbol[symbol]);
      }
    }// HARQ 0

    /* For last OFDM symbol at each antenna perform interpolation and compensation for the slot*/
    if(symbol == (symbInSlot -1)) {
      /*------------------------------------------------------------------------------------------------------- */
      /* 2) Interpolate PTRS estimated value in TD */
      /*------------------------------------------------------------------------------------------------------- */
      /* If L-PTRS is > 0 then we need interpolation */
      if(*L_ptrs > 0) {
        ret = nr_ptrs_process_slot(*dmrsSymbPos, *ptrsSymbPos, (int16_t*)phase_per_symbol, *startSymbIndex, *nbSymb);
        if(ret != 0) {
          LOG_W(PHY,"[PTRS] Compensation is skipped due to error in PTRS slot processing !!\n");
        }
      }
#ifdef DEBUG_DL_PTRS
      LOG_M("ptrsEst.m","est",ptrs_phase_per_slot[aarx],frame_parms->symbols_per_slot,1,1 );
      LOG_M("rxdataF_bf_ptrs_comp.m", "bf_ptrs_cmp", rxdataF_comp[0][aarx] + (*startSymbIndex) * NR_NB_SC_PER_RB * (*nb_rb), (*nb_rb) * NR_NB_SC_PER_RB * (*nbSymb), 1, 1);
#endif
      /*------------------------------------------------------------------------------------------------------- */
      /* 3) Compensated DMRS based estimated signal with PTRS estimation                                        */
      /*--------------------------------------------------------------------------------------------------------*/
      for(uint8_t i = *startSymbIndex; i< symbInSlot ;i++) {
        /* DMRS Symbol has 0 phase so no need to rotate the respective symbol */
        /* Skip rotation if the slot processing is wrong */
        if((!is_dmrs_symbol(i,*dmrsSymbPos)) && (ret == 0)) {
#ifdef DEBUG_DL_PTRS
          printf("[PHY][DL][PTRS]: Rotate Symbol %2d with  %d + j* %d\n", i, phase_per_symbol[i].r,phase_per_symbol[i].i);
#endif
          rotate_cpx_vector((c16_t *)&rxdataF_comp[0][aarx][(i * (*nb_rb) * NR_NB_SC_PER_RB)],
                            &phase_per_symbol[i],
                            (c16_t *)&rxdataF_comp[0][aarx][(i * (*nb_rb) * NR_NB_SC_PER_RB)],
                            ((*nb_rb) * NR_NB_SC_PER_RB),
                            15);
        }// if not DMRS Symbol
      }// symbol loop
    }// last symbol check
  }//Antenna loop
}//main function
