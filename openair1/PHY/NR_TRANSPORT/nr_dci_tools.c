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

/*! \file PHY/NR_TRANSPORT/nr_dci_tools.c
 * \brief
 * \author
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email:
 * \note
 * \warning
 */

#include "nr_dci.h"
#include "nr_dlsch.h"


void nr_fill_cce_list(NR_gNB_DCI_ALLOC_t* dci_alloc, uint16_t n_shift, uint8_t m) {

  nr_cce_t* cce;
  nr_reg_t* reg;
  nfapi_nr_dl_config_pdcch_parameters_rel15_t* pdcch_params = &dci_alloc->pdcch_params;

  uint8_t L = dci_alloc->L;
  uint8_t bsize = pdcch_params->reg_bundle_size;
  uint8_t R = pdcch_params->interleaver_size;
  uint16_t N_reg = pdcch_params->n_rb * pdcch_params->n_symb;
  uint16_t Y, N_cce, M_s_max, n_CI=0, tmp, C;

  if (pdcch_params->config_type == NFAPI_NR_CSET_CONFIG_MIB_SIB1)
    AssertFatal(L>=4, "Invalid aggregation level for SIB1 configured PDCCH %d\n", L);

  N_cce = N_reg / NR_NB_REG_PER_CCE;
  /*Max number of candidates per aggregation level -- SIB1 configured search space only*/
  M_s_max = (L==4)?4:(L==8)?2:1;

  if (pdcch_params->search_space_type == NFAPI_NR_SEARCH_SPACE_TYPE_COMMON)
    Y = 0;
  else { //NFAPI_NR_SEARCH_SPACE_TYPE_UE_SPECIFIC
  }

  uint8_t cond = N_reg%(bsize*R);
  AssertFatal(cond==0, "CCE to REG interleaving: Invalid configuration leading to non integer C\n");
  C = N_reg/(bsize*R);

  tmp = L * (( Y + (m*N_cce)/(L*M_s_max) + n_CI ) % (N_cce/L));

  LOG_I(PHY, "CCE list generation for candidate %d: bundle size %d ilv size %d tmp %d\n", m, bsize, R, tmp);
  for (uint8_t cce_idx=0; cce_idx<L; cce_idx++) {
    cce = &dci_alloc->cce_list[cce_idx];
    cce->cce_idx = tmp + cce_idx;
    LOG_D(PHY, "cce_idx %d\n", cce->cce_idx);

    if (pdcch_params->cr_mapping_type == NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED) {
      LOG_D(PHY, "Interleaved CCE to REG mapping\n");
      uint8_t j = cce->cce_idx, j_prime;
      uint8_t r,c,idx;

      for (uint8_t bundle_idx=0; bundle_idx<NR_NB_REG_PER_CCE/bsize; bundle_idx++) {
        j_prime = 6*j/bsize + bundle_idx;
        r = j_prime%R;
        c = (j_prime-r)/R;
        idx = (r*C + c + n_shift)%(N_reg/bsize);
        LOG_D(PHY, "bundle idx = %d \n j = %d \t j_prime = %d \t r = %d \t c = %d\n", idx, j , j_prime, r, c);

        for (uint8_t reg_idx=0; reg_idx<bsize; reg_idx++) {
          reg = &cce->reg_list[reg_idx];
          reg->reg_idx = bsize*idx + reg_idx;
          reg->start_sc_idx = (reg->reg_idx/pdcch_params->n_symb) * NR_NB_SC_PER_RB;
          reg->symb_idx = reg->reg_idx % pdcch_params->n_symb;
          LOG_D(PHY, "reg %d symbol %d start subcarrier %d\n", reg->reg_idx, reg->symb_idx, reg->start_sc_idx);
        }
      }
    }
    else { // NFAPI_NR_CCE_REG_MAPPING_NON_INTERLEAVED
      LOG_D(PHY, "Non interleaved CCE to REG mapping\n");
      for (uint8_t reg_idx=0; reg_idx<NR_NB_REG_PER_CCE; reg_idx++) {
        reg = &cce->reg_list[reg_idx];
        reg->reg_idx = cce->cce_idx*NR_NB_REG_PER_CCE + reg_idx;
        reg->start_sc_idx = (reg->reg_idx/pdcch_params->n_symb) * NR_NB_SC_PER_RB;
        reg->symb_idx = reg->reg_idx % pdcch_params->n_symb;
        LOG_D(PHY, "reg %d symbol %d start subcarrier %d\n", reg->reg_idx, reg->symb_idx, reg->start_sc_idx);
      }

    }

  }
}

void nr_fill_dci_and_dlsch(PHY_VARS_gNB *gNB,
                           int frame,
                           int subframe,
                           gNB_rxtx_proc_t *proc,
                           NR_gNB_DCI_ALLOC_t *dci_alloc,
                           nfapi_nr_dl_config_dci_dl_pdu *pdcch_pdu,
                           nfapi_nr_dl_config_dlsch_pdu *dlsch_pdu)
{
	NR_DL_FRAME_PARMS *fp = &gNB->frame_parms;
  uint8_t n_shift;
	uint32_t *dci_pdu = dci_alloc->dci_pdu;
  memset((void*)dci_pdu,0,4*sizeof(uint32_t));
	nfapi_nr_dl_config_dci_dl_pdu_rel15_t *pdu_rel15 = &pdcch_pdu->dci_dl_pdu_rel15;
  nfapi_nr_dl_config_pdcch_parameters_rel15_t *params_rel15 = &pdcch_pdu->pdcch_params_rel15;
	nfapi_nr_config_request_t *cfg = &gNB->gNB_config;
  NR_gNB_DLSCH_t *dlsch = &gNB->dlsch[0][0];
  NR_DL_gNB_HARQ_t **harq = dlsch->harq_processes;

  uint16_t N_RB = fp->initial_bwp_dl.N_RB;
  uint16_t N_RB_UL = fp->initial_bwp_ul.N_RB;
  uint8_t fsize=0, pos=0, pos2=0,cand_idx=0;

  /// Payload generation
  switch(params_rel15->dci_format) {

    case NFAPI_NR_DL_DCI_FORMAT_1_0:
      switch(params_rel15->rnti_type) {
        case NFAPI_NR_RNTI_RA:
        	// Freq domain assignment
        	fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
          printf("fsize = %d\n",fsize);
        	for (int i=0; i<fsize; i++)
        		*dci_pdu |= ((pdu_rel15->frequency_domain_assignment>>(fsize-i-1))&1)<<pos++;
        	// Time domain assignment
        	for (int i=0; i<4; i++)
        		*dci_pdu |= ((pdu_rel15->time_domain_assignment>>(3-i))&1)<<pos++;
        	// VRB to PRB mapping
        	*dci_pdu |= (pdu_rel15->vrb_to_prb_mapping&1)<<pos++;
        	// MCS
        	for (int i=0; i<5; i++)
        		*dci_pdu |= ((pdu_rel15->mcs>>(4-i))&1)<<pos++;
        	// TB scaling
        	for (int i=0; i<2; i++)
        		*dci_pdu |= ((pdu_rel15->tb_scaling>>(1-i))&1)<<pos++;

          printf("***************************\n");
        	break;

         case NFAPI_NR_RNTI_C:  
      // indicating a DL DCI format 1bit
        *dci_pdu |= (pdu_rel15->format_indicator&1)<<pos++;
      // Freq domain assignment (275rb >> fsize = 16)
        fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
        for (int i=0; i<fsize; i++) 
            *dci_pdu |= ((pdu_rel15->frequency_domain_assignment>>(fsize-i-1))&1)<<pos++;

      if ((pdu_rel15->frequency_domain_assignment+1)&1 ==0) //fsize are all 1  38.212 p86
      {
          printf("***************************\n");
          
          // ra_preamble_index 6bit
          for (int i=0; i<6; i++)
            *dci_pdu |= ((pdu_rel15->ra_preamble_index>>(5-i-1))&1)<<pos++;

          // UL/SUL indicator  1bit
            *dci_pdu |= (pdu_rel15->ul_sul_indicator&1)<<pos++;
        
          // SS/PBCH index  6bit
          for (int i=0; i<6; i++)
            *dci_pdu |= ((pdu_rel15->ss_pbch_index>>(5-i))&1)<<pos++;
      
      //  prach_mask_index  "2"+2bit // cause it 32bit and  bit over 32 ,so dci_pdu ++
          for (int i=0; i<2; i++)
            *dci_pdu |= ((pdu_rel15->prach_mask_index>>(3-i))&1)<<pos++;
      
      //--------------------------dci_pdu ++------------------------------
      
      //  prach_mask_index  2+"2"bit //
          for (int i=2; i<4; i++)
      {
        if (pos>31)
            *(dci_pdu+1) |= ((pdu_rel15->prach_mask_index>>(3-i))&1)<<pos2++;
        else
        *dci_pdu |= ((pdu_rel15->prach_mask_index>>(3-i))&1)<<pos++;
      }
      
      
      }  //end if
      else 
      
      {
      // Time domain assignment 4bit
          for (int i=0; i<4; i++)
            *dci_pdu |= ((pdu_rel15->time_domain_assignment>>(3-i))&1)<<pos++;
      
      // VRB to PRB mapping  1bit
           *dci_pdu |= (pdu_rel15->vrb_to_prb_mapping&1)<<pos++;
      
      // MCS 5bit  //bit over 32, so dci_pdu ++
          for (int i=0; i<5; i++)
            *dci_pdu |= ((pdu_rel15->mcs>>(4-i))&1)<<pos++;
    
      // New data indicator 1bit
        *dci_pdu |= (pdu_rel15->ndi&1)<<pos++;
      
      // Redundancy version  2bit
          for (int i=0; i<2; i++)
            *dci_pdu |= ((pdu_rel15->rv>>(1-i))&1)<<pos++;
      
      // HARQ process number  4bit  "2"+2
          for (int i=0; i<2; i++)
            *dci_pdu  |= ((pdu_rel15->harq_pid>>(3-i))&1)<<pos++;
      
      //--------------------------dci_pdu ++------------------------------
      
      // HARQ process number  4bit  2+"2"
          for (int i=2; i<4; i++)
      { 
        if (pos>31)
            *(dci_pdu+1)  |= ((pdu_rel15->harq_pid>>(3-i))&1)<<pos2++;
        else 
        *dci_pdu  |= ((pdu_rel15->harq_pid>>(3-i))&1)<<pos++;
      }
      
      // Downlink assignment index  2bit
      for (int i=0; i<2; i++)
      { 
        if (pos>31)
            *(dci_pdu+1) |= ((pdu_rel15->dai>>(1-i))&1)<<pos2++;
        else 
        *dci_pdu |= ((pdu_rel15->dai>>(1-i))&1)<<pos++;
      }
      // TPC command for scheduled PUCCH  2bit
      for (int i=0; i<2; i++)
      { 
        if (pos>31)
            *(dci_pdu+1) |= ((pdu_rel15->tpc>>(1-i))&1)<<pos2++;
        else 
        *dci_pdu |= ((pdu_rel15->tpc>>(1-i))&1)<<pos++;  
      }
      // PUCCH resource indicator  3bit
      for (int i=0; i<3; i++)
      { 
        if (pos>31)
            *(dci_pdu+1) |= ((pdu_rel15->pucch_resource_indicator>>(2-i))&1)<<pos2++;
        else
        *dci_pdu |= ((pdu_rel15->pucch_resource_indicator>>(2-i))&1)<<pos++;      
      }
      // PDSCH-to-HARQ_feedback timing indicator 3bit
      for (int i=0; i<3; i++)
      {
        if (pos>31)
            *(dci_pdu+1) |= ((pdu_rel15->pdsch_to_harq_feedback_timing_indicator>>(2-i))&1)<<pos2++;
        else
        *dci_pdu |= ((pdu_rel15->pdsch_to_harq_feedback_timing_indicator>>(2-i))&1)<<pos++; 
      }
  
      } //end else

      break;

      case NFAPI_NR_RNTI_P:
      
      // Short Messages Indicator – 2 bits
          for (int i=0; i<2; i++)
            *dci_pdu |= ((pdu_rel15->short_messages_indicator>>(1-i))&1)<<pos++;
      // Short Messages – 8 bits
          for (int i=0; i<8; i++)
            *dci_pdu |= ((pdu_rel15->short_messages>>(7-i))&1)<<pos++;
      // Freq domain assignment 0-16 bit
        fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
          for (int i=0; i<fsize; i++)
            *dci_pdu |= ((pdu_rel15->frequency_domain_assignment>>(fsize-i-1))&1)<<pos++;
          // Time domain assignment 4 bit
          for (int i=0; i<4; i++)
            *dci_pdu |= ((pdu_rel15->time_domain_assignment>>(3-i))&1)<<pos++;
          // VRB to PRB mapping 1 bit
        *dci_pdu |= (pdu_rel15->vrb_to_prb_mapping&1)<<pos++;
          // MCS "1"+4 = 5 bit
          for (int i=0; i<1; i++) 
            *dci_pdu |= ((pdu_rel15->mcs>>(4-i))&1)<<pos++;
      // MCS 1+"4" = 5 bit
      
          for (int i=1; i<4; i++) 
      {
        if (pos>31)
            *(dci_pdu+1) |= ((pdu_rel15->mcs>>(4-i))&1)<<pos2++;
        else
        *dci_pdu |= ((pdu_rel15->mcs>>(4-i))&1)<<pos++;
      }
          // TB scaling 2 bit
          for (int i=0; i<2; i++)
      { 
        if (pos>31)
            *(dci_pdu+1) |= ((pdu_rel15->tb_scaling>>(1-i))&1)<<pos2++;
        else
        *dci_pdu |= ((pdu_rel15->tb_scaling>>(1-i))&1)<<pos++;
      }
      break;
      
      case NFAPI_NR_RNTI_SI:
      // Freq domain assignment 0-16 bit
        fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
          for (int i=0; i<fsize; i++)
            *dci_pdu |= ((pdu_rel15->frequency_domain_assignment>>(fsize-i-1))&1)<<pos++;
          // Time domain assignment 4 bit
          for (int i=0; i<4; i++)
            *dci_pdu |= ((pdu_rel15->time_domain_assignment>>(3-i))&1)<<pos++;
          // VRB to PRB mapping 1 bit
        *dci_pdu |= (pdu_rel15->vrb_to_prb_mapping&1)<<pos++;
      // MCS 5bit  //bit over 32, so dci_pdu ++
          for (int i=0; i<5; i++)
            *dci_pdu |= ((pdu_rel15->mcs>>(4-i))&1)<<pos++;
      // Redundancy version  2bit
          for (int i=0; i<2; i++)
            *dci_pdu |= ((pdu_rel15->rv>>(1-i))&1)<<pos++;
      
      break;
      
      case NFAPI_NR_RNTI_TC:
      // indicating a DL DCI format 1bit
        *dci_pdu |= (pdu_rel15->format_indicator&1)<<pos++;
      // Freq domain assignment 0-16 bit
          fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
          for (int i=0; i<fsize; i++)
            *dci_pdu |= ((pdu_rel15->frequency_domain_assignment>>(fsize-i-1))&1)<<pos++;
      // Time domain assignment 4 bit
          for (int i=0; i<4; i++)
            *dci_pdu |= ((pdu_rel15->time_domain_assignment>>(3-i))&1)<<pos++;
      // VRB to PRB mapping 1 bit
        *dci_pdu |= (pdu_rel15->vrb_to_prb_mapping&1)<<pos++;
      // MCS 5bit  //bit over 32, so dci_pdu ++
          for (int i=0; i<5; i++)
            *dci_pdu |= ((pdu_rel15->mcs>>(4-i))&1)<<pos++;
      // New data indicator 1bit
        *dci_pdu |= (pdu_rel15->ndi&1)<<pos++;
      // Redundancy version  2bit
          for (int i=0; i<2; i++)
            *dci_pdu |= ((pdu_rel15->rv>>(1-i))&1)<<pos++;
      // HARQ process number  4bit  
          for (int i=0; i<2; i++)
            *dci_pdu  |= ((pdu_rel15->harq_pid>>(3-i))&1)<<pos++; 
      
      // HARQ process number  4bit  
          for (int i=2; i<4; i++)
      { 
        if (pos>31)
            *(dci_pdu+1)  |= ((pdu_rel15->harq_pid>>(3-i))&1)<<pos2++;  
        else
        *dci_pdu  |= ((pdu_rel15->harq_pid>>(3-i))&1)<<pos++; 
      } 
        
      // Downlink assignment index – 2 bits 
          for (int i=0; i<2; i++)
      { 
        if (pos>31)
            *(dci_pdu+1)  |= ((pdu_rel15->dai>>(1-i))&1)<<pos2++;
        else
        *dci_pdu  |= ((pdu_rel15->dai>>(1-i))&1)<<pos++;
      }     
      // TPC command for scheduled PUCCH – 2 bits
          for (int i=0; i<2; i++)
      { 
        if (pos>31)
            *(dci_pdu+1)  |= ((pdu_rel15->tpc>>(1-i))&1)<<pos2++; 
        else
        *dci_pdu  |= ((pdu_rel15->tpc>>(1-i))&1)<<pos++;    
      } 
      // PUCCH resource indicator – 3 bits 
          for (int i=0; i<3; i++)
      { 
        if (pos>31)
            *(dci_pdu+1)  |= ((pdu_rel15->pucch_resource_indicator>>(2-i))&1)<<pos2++;
        else
        *dci_pdu  |= ((pdu_rel15->pucch_resource_indicator>>(2-i))&1)<<pos++; 
      }
      // PDSCH-to-HARQ_feedback timing indicator – 3 bits
          for (int i=0; i<3; i++)
      {
        if (pos>31)
            *(dci_pdu+1)  |= ((pdu_rel15->pdsch_to_harq_feedback_timing_indicator>>(2-i))&1)<<pos2++; 
        else
        *dci_pdu  |= ((pdu_rel15->pdsch_to_harq_feedback_timing_indicator>>(2-i))&1)<<pos++;      
      }
      
      ///-----------------------------------?????????????????????------------------------
      break;
      }
      break;

    case NFAPI_NR_UL_DCI_FORMAT_0_0:
    switch(params_rel15->rnti_type) 
    {
      case NFAPI_NR_RNTI_C:
      // indicating a DL DCI format 1bit
         *dci_pdu |= (pdu_rel15->format_indicator&1)<<pos++;
          // Freq domain assignment  max 16 bit
          fsize = (int)ceil( log2( (N_RB_UL*(N_RB_UL+1))>>1 ) );
          for (int i=0; i<fsize; i++)
            *dci_pdu |= ((pdu_rel15->frequency_domain_assignment>>(fsize-i-1))&1)<<pos++;
          // Time domain assignment 4bit
          for (int i=0; i<4; i++)
            *dci_pdu |= ((pdu_rel15->time_domain_assignment>>(3-i))&1)<<pos++;
      // Frequency hopping flag – 1 bit
         *dci_pdu |= (pdu_rel15->frequency_hopping_flag&1)<<pos++;
          // MCS  5 bit
          for (int i=0; i<5; i++)
          *dci_pdu |= ((pdu_rel15->mcs>>(4-i))&1)<<pos++;
      // New data indicator 1bit
         *dci_pdu |= (pdu_rel15->ndi&1)<<pos++;
      // Redundancy version  2bit
          for (int i=0; i<2; i++)
         *dci_pdu |= ((pdu_rel15->rv>>(1-i))&1)<<pos++;
      // HARQ process number  4bit  
          for (int i=0; i<2; i++)
         *dci_pdu  |= ((pdu_rel15->harq_pid>>(3-i))&1)<<pos++;
      
      // HARQ process number  4bit  
          for (int i=2; i<4; i++)
      {
        if (pos>31)
            *(dci_pdu+1)  |= ((pdu_rel15->harq_pid>>(3-i))&1)<<pos2++;
        else 
        *dci_pdu  |= ((pdu_rel15->harq_pid>>(3-i))&1)<<pos++;
      }
      // TPC command for scheduled PUSCH – 2 bits
        for (int i=0; i<2; i++)
        {
        if (pos>31)
            *(dci_pdu+1) |= ((pdu_rel15->tpc>>(1-i))&1)<<pos2++;  
        else
        *dci_pdu |= ((pdu_rel15->tpc>>(1-i))&1)<<pos++;
        }
      // Padding bits
        if (pos<32)
        {
        for(int a = pos;a<32;a++)
          *dci_pdu |= (pdu_rel15->padding&1)<<pos++;
        }
      // UL/SUL indicator – 1 bit
        if (cfg->pucch_config.pucch_GroupHopping.value)
        { 
        if (pos>31) 
        *(dci_pdu+1) |= (pdu_rel15->ul_sul_indicator&1)<<pos2++;  
        else
        *dci_pdu |= (pdu_rel15->ul_sul_indicator&1)<<pos++; 
        }
   
          break;
      
      case NFAPI_NR_RNTI_TC:
      
          // indicating a DL DCI format 1bit
          *dci_pdu |= (pdu_rel15->format_indicator&1)<<pos++;
          // Freq domain assignment  max 16 bit
          fsize = (int)ceil( log2( (N_RB_UL*(N_RB_UL+1))>>1 ) );
          for (int i=0; i<fsize; i++)
            *dci_pdu |= ((pdu_rel15->frequency_domain_assignment>>(fsize-i-1))&1)<<pos++;
          // Time domain assignment 4bit
          for (int i=0; i<4; i++)
            *dci_pdu |= ((pdu_rel15->time_domain_assignment>>(3-i))&1)<<pos++;
         // Frequency hopping flag – 1 bit
          *dci_pdu |= (pdu_rel15->frequency_hopping_flag&1)<<pos++;
          // MCS  5 bit
          for (int i=0; i<5; i++)
          *dci_pdu |= ((pdu_rel15->mcs>>(4-i))&1)<<pos++;
          // New data indicator 1bit
         *dci_pdu |= (pdu_rel15->ndi&1)<<pos++;
          // Redundancy version  2bit
          for (int i=0; i<2; i++)
          *dci_pdu |= ((pdu_rel15->rv>>(1-i))&1)<<pos++;
          // HARQ process number  4bit  
          for (int i=0; i<2; i++)
          *dci_pdu  |= ((pdu_rel15->harq_pid>>(3-i))&1)<<pos++;
      
        // HARQ process number  4bit  
        for (int i=2; i<4; i++)
        {
        if (pos>31)
        *(dci_pdu+1)  |= ((pdu_rel15->harq_pid>>(3-i))&1)<<pos2++;
        else 
        *dci_pdu  |= ((pdu_rel15->harq_pid>>(3-i))&1)<<pos++;
        }
        // TPC command for scheduled PUSCH – 2 bits
        for (int i=0; i<2; i++)
        {
        if (pos>31)
        *(dci_pdu+1) |= ((pdu_rel15->tpc>>(1-i))&1)<<pos2++;  
        else
        *dci_pdu |= ((pdu_rel15->tpc>>(1-i))&1)<<pos++;
        }
        // Padding bits
        if (pos<32)
        {
        for(int a = pos;a<32;a++)
        *dci_pdu |= (pdu_rel15->padding&1)<<pos++;
        }
        // UL/SUL indicator – 1 bit
        if (cfg->pucch_config.pucch_GroupHopping.value)
        { 
        if (pos>31) 
        *(dci_pdu+1) |= (pdu_rel15->ul_sul_indicator&1)<<pos2++;  
        else
        *dci_pdu |= (pdu_rel15->ul_sul_indicator&1)<<pos++; 
        } 
        break;
      } 
      break;
  }


  LOG_I(PHY, "DCI PDU: [0]->0x%08x \t [1]->0x%08x \t [2]->0x%08x \t [3]->0x%08x\n",
              dci_pdu[0], dci_pdu[1], dci_pdu[2], dci_pdu[3]);

  /// rest of DCI alloc
  dci_alloc->L = 8;
  memcpy((void*)&dci_alloc->pdcch_params, (void*)params_rel15, sizeof(nfapi_nr_dl_config_pdcch_parameters_rel15_t));
  dci_alloc->size = nr_get_dci_size(dci_alloc->pdcch_params.dci_format,
                        dci_alloc->pdcch_params.rnti_type,
                        &fp->initial_bwp_dl,
                        cfg);
  n_shift = (dci_alloc->pdcch_params.config_type == NFAPI_NR_CSET_CONFIG_MIB_SIB1)?
                      cfg->sch_config.physical_cell_id.value : dci_alloc->pdcch_params.shift_index;
  nr_fill_cce_list(dci_alloc, n_shift, cand_idx);
  LOG_I(PHY, "DCI type %d payload (size %d) generated on candidate %d\n", dci_alloc->pdcch_params.dci_format, dci_alloc->size, cand_idx);

  /// DLSCH struct
  memcpy((void*)&harq[dci_alloc->harq_pid]->dlsch_pdu, (void*)dlsch_pdu, sizeof(nfapi_nr_dl_config_dlsch_pdu));

}
