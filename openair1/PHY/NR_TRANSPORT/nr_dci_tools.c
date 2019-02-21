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

//#define DEBUG_FILL_DCI

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
  uint16_t n_RNTI = (pdcch_params->search_space_type == NFAPI_NR_SEARCH_SPACE_TYPE_UE_SPECIFIC)? pdcch_params->rnti:0;
  uint32_t A[3]={39827,39829,39839};

  if (pdcch_params->config_type == NFAPI_NR_CSET_CONFIG_MIB_SIB1)
    AssertFatal(L>=4, "Invalid aggregation level for SIB1 configured PDCCH %d\n", L);

  N_cce = N_reg / NR_NB_REG_PER_CCE;
  /*Max number of candidates per aggregation level -- SIB1 configured search space only*/
  M_s_max = (L==4)?4:(L==8)?2:1;

  if (pdcch_params->search_space_type == NFAPI_NR_SEARCH_SPACE_TYPE_COMMON)
    Y = 0;
  else { //NFAPI_NR_SEARCH_SPACE_TYPE_UE_SPECIFIC
    Y = (A[0]*n_RNTI)%65537; // Candidate 0, antenna port 0
  }

  if (pdcch_params->cr_mapping_type == NFAPI_NR_CCE_REG_MAPPING_INTERLEAVED) {
    AssertFatal((N_reg%(bsize*R))==0, "CCE to REG interleaving: Invalid configuration leading to non integer C (N_reg %d, bsize %d R %d)\n",
    N_reg, bsize, R);
    C = N_reg/(bsize*R);
  }

  tmp = L * (( Y + (m*N_cce)/(L*M_s_max) + n_CI ) % CEILIDIV(N_cce,L));

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

/*static inline uint64_t dci_field(uint64_t field, uint8_t size) {
  uint64_t ret = 0;
  for (int i=0; i<size; i++)
    ret |= ((field>>i)&1)<<(size-i-1);
  return ret;
}*/

void nr_fill_dci_and_dlsch(PHY_VARS_gNB *gNB,
                           int frame,
                           int subframe,
                           gNB_L1_rxtx_proc_t *proc,
                           NR_gNB_DCI_ALLOC_t *dci_alloc,
                           nfapi_nr_dl_config_dci_dl_pdu *pdcch_pdu,
                           nfapi_nr_dl_config_dlsch_pdu *dlsch_pdu)
{

  uint8_t n_shift;


  uint64_t *dci_pdu = (uint64_t*)dci_alloc->dci_pdu;
  memset((void*)dci_pdu,0,2*sizeof(uint64_t));
  nfapi_nr_dl_config_dci_dl_pdu_rel15_t *pdu_rel15 = &pdcch_pdu->dci_dl_pdu_rel15;
  nfapi_nr_dl_config_pdcch_parameters_rel15_t *params_rel15 = &pdcch_pdu->pdcch_params_rel15;


  nfapi_nr_config_request_t *cfg = &gNB->gNB_config;
  NR_gNB_DLSCH_t *dlsch = gNB->dlsch[0][0];
  NR_DL_gNB_HARQ_t **harq = dlsch->harq_processes;

  uint16_t N_RB = params_rel15->n_RB_BWP;
  uint8_t fsize=0, pos=0, cand_idx=0;


  dci_alloc->L = 8;
  memcpy((void*)&dci_alloc->pdcch_params, (void*)params_rel15, sizeof(nfapi_nr_dl_config_pdcch_parameters_rel15_t));
  dci_alloc->size = nr_get_dci_size(dci_alloc->pdcch_params.dci_format,
				    dci_alloc->pdcch_params.rnti_type,
				    N_RB,
				    cfg);

  AssertFatal(dci_alloc->size<=64, "DCI sizes above 64 bits not yet supported");
  n_shift = (dci_alloc->pdcch_params.config_type == NFAPI_NR_CSET_CONFIG_MIB_SIB1)?
    cfg->sch_config.physical_cell_id.value : dci_alloc->pdcch_params.shift_index;
  nr_fill_cce_list(dci_alloc, n_shift, cand_idx);

  /// Payload generation
  switch(params_rel15->dci_format) {


  case NFAPI_NR_DL_DCI_FORMAT_1_0:
    switch(params_rel15->rnti_type) {
    case NFAPI_NR_RNTI_RA:

      // Freq domain assignment
      fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
      pos=fsize;
      *dci_pdu |= ((pdu_rel15->frequency_domain_assignment&((1<<fsize)-1)) << (dci_alloc->size-pos)); 
#ifdef DEBUG_FILL_DCI
      printf("frequency-domain assignment %d (%d bits)=> %d (0x%lx)\n",pdu_rel15->frequency_domain_assignment,fsize,dci_alloc->size-pos,*dci_pdu);
#endif
      // Time domain assignment
      pos+=4;		   
      *dci_pdu |= (((uint64_t)pdu_rel15->time_domain_assignment&0xf) << (dci_alloc->size-pos));
#ifdef DEBUG_FILL_DCI
      printf("time-domain assignment %d  (3 bits)=> %d (0x%lx)\n",pdu_rel15->time_domain_assignment,dci_alloc->size-pos,*dci_pdu);
#endif
      // VRB to PRB mapping

      pos++;
      *dci_pdu |= ((uint64_t)pdu_rel15->vrb_to_prb_mapping&0x1)<<(dci_alloc->size-pos);
#ifdef DEBUG_FILL_DCI
      printf("vrb to prb mapping %d  (1 bits)=> %d (0x%lx)\n",pdu_rel15->vrb_to_prb_mapping,dci_alloc->size-pos,*dci_pdu);
#endif
      // MCS
      pos+=5;
      *dci_pdu |= ((uint64_t)pdu_rel15->mcs&0x1f)<<(dci_alloc->size-pos);
#ifdef DEBUG_FILL_DCI
      printf("mcs %d  (5 bits)=> %d (0x%lx)\n",pdu_rel15->mcs,dci_alloc->size-pos,*dci_pdu);
#endif
      // TB scaling
      pos+=2;
      *dci_pdu |= ((uint64_t)pdu_rel15->tb_scaling&0x3)<<(dci_alloc->size-pos);
#ifdef DEBUG_FILL_DCI
      printf("tb_scaling %d  (2 bits)=> %d (0x%lx)\n",pdu_rel15->tb_scaling,dci_alloc->size-pos,*dci_pdu);
#endif
      break;

    case NFAPI_NR_RNTI_C:

      // indicating a DL DCI format 1bit
      pos++;
      *dci_pdu |= ((uint64_t)pdu_rel15->format_indicator&1)<<(dci_alloc->size-pos);
#ifdef DEBUG_FILL_DCI
      printf("Format indicator %d (%d bits)=> %d (0x%lx)\n",pdu_rel15->format_indicator,1,dci_alloc->size-pos,*dci_pdu);
#endif

      // Freq domain assignment (275rb >> fsize = 16)
      fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
      pos+=fsize;
      *dci_pdu |= (((uint64_t)pdu_rel15->frequency_domain_assignment&((1<<fsize)-1)) << (dci_alloc->size-pos));

#ifdef DEBUG_FILL_DCI
      printf("Freq domain assignment %d (%d bits)=> %d (0x%lx)\n",pdu_rel15->frequency_domain_assignment,fsize,dci_alloc->size-pos,*dci_pdu);
#endif

  uint16_t is_ra = 1;
  for (int i=0; i<fsize; i++)
    if (!((pdu_rel15->frequency_domain_assignment>>i)&1)) {
      is_ra = 0;
      break;
    }
  if (is_ra) //fsize are all 1  38.212 p86
	{
	  // ra_preamble_index 6 bits
	  pos+=6;
	  *dci_pdu |= ((pdu_rel15->ra_preamble_index&0x3f)<<(dci_alloc->size-pos));

	  // UL/SUL indicator  1 bit
	  pos++;
	  *dci_pdu |= (pdu_rel15->ul_sul_indicator&1)<<(dci_alloc->size-pos);
          
	  // SS/PBCH index  6 bits
	  pos+=6;
	  *dci_pdu |= ((pdu_rel15->ss_pbch_index&0x3f)<<(dci_alloc->size-pos));
        
	  //  prach_mask_index  4 bits
	  pos+=4;
	  *dci_pdu |= ((pdu_rel15->prach_mask_index&0xf)<<(dci_alloc->size-pos));
          
	}  //end if

      else {

	// Time domain assignment 4bit

	pos+=4;		   
	*dci_pdu |= ((pdu_rel15->time_domain_assignment&0xf) << (dci_alloc->size-pos));
#ifdef DEBUG_FILL_DCI
      printf("Time domain assignment %d (%d bits)=> %d (0x%lx)\n",pdu_rel15->time_domain_assignment,4,dci_alloc->size-pos,*dci_pdu);
#endif

	// VRB to PRB mapping  1bit
	pos++;
	*dci_pdu |= (pdu_rel15->vrb_to_prb_mapping&1)<<(dci_alloc->size-pos);
#ifdef DEBUG_FILL_DCI
      printf("VRB to PRB %d (%d bits)=> %d (0x%lx)\n",pdu_rel15->vrb_to_prb_mapping,1,dci_alloc->size-pos,*dci_pdu);
#endif

	// MCS 5bit  //bit over 32, so dci_pdu ++
	pos+=5;
	*dci_pdu |= (pdu_rel15->mcs&0x1f)<<(dci_alloc->size-pos);
#ifdef DEBUG_FILL_DCI
      printf("MCS %d (%d bits)=> %d (0x%lx)\n",pdu_rel15->mcs,5,dci_alloc->size-pos,*dci_pdu);
#endif

	// New data indicator 1bit
	pos++;
	*dci_pdu |= (pdu_rel15->ndi&1)<<(dci_alloc->size-pos);
#ifdef DEBUG_FILL_DCI
      printf("NDI %d (%d bits)=> %d (0x%lx)\n",pdu_rel15->ndi,1,dci_alloc->size-pos,*dci_pdu);
#endif      

	// Redundancy version  2bit
	pos+=2;
	*dci_pdu |= (pdu_rel15->rv&0x3)<<(dci_alloc->size-pos);
#ifdef DEBUG_FILL_DCI
      printf("RV %d (%d bits)=> %d (0x%lx)\n",pdu_rel15->rv,2,dci_alloc->size-pos,*dci_pdu);
#endif

	// HARQ process number  4bit
	pos+=4;
	*dci_pdu  |= ((pdu_rel15->harq_pid&0xf)<<(dci_alloc->size-pos));
#ifdef DEBUG_FILL_DCI
      printf("HARQ_PID %d (%d bits)=> %d (0x%lx)\n",pdu_rel15->harq_pid,4,dci_alloc->size-pos,*dci_pdu);
#endif

	// Downlink assignment index  2bit
	pos+=2;
	*dci_pdu |= ((pdu_rel15->dai&3)<<(dci_alloc->size-pos));
#ifdef DEBUG_FILL_DCI
      printf("DAI %d (%d bits)=> %d (0x%lx)\n",pdu_rel15->dai,2,dci_alloc->size-pos,*dci_pdu);
#endif

	// TPC command for scheduled PUCCH  2bit
	pos+=2;
	*dci_pdu |= ((pdu_rel15->tpc&3)<<(dci_alloc->size-pos));
#ifdef DEBUG_FILL_DCI
      printf("TPC %d (%d bits)=> %d (0x%lx)\n",pdu_rel15->tpc,2,dci_alloc->size-pos,*dci_pdu);
#endif

	// PUCCH resource indicator  3bit
	pos+=3;
	*dci_pdu |= ((pdu_rel15->pucch_resource_indicator&0x7)<<(dci_alloc->size-pos));
#ifdef DEBUG_FILL_DCI
      printf("PUCCH RI %d (%d bits)=> %d (0x%lx)\n",pdu_rel15->pucch_resource_indicator,3,dci_alloc->size-pos,*dci_pdu);
#endif

	// PDSCH-to-HARQ_feedback timing indicator 3bit
	pos+=3;
	*dci_pdu |= ((pdu_rel15->pdsch_to_harq_feedback_timing_indicator&0x7)<<(dci_alloc->size-pos));
#ifdef DEBUG_FILL_DCI
      printf("PDSCH to HARQ TI %d (%d bits)=> %d (0x%lx)\n",pdu_rel15->pdsch_to_harq_feedback_timing_indicator,3,dci_alloc->size-pos,*dci_pdu);
#endif

      } //end else
      break;

    case NFAPI_NR_RNTI_P:
      
      // Short Messages Indicator – 2 bits
      for (int i=0; i<2; i++)
	*dci_pdu |= (((uint64_t)pdu_rel15->short_messages_indicator>>(1-i))&1)<<(dci_alloc->size-pos++);
      // Short Messages – 8 bits
      for (int i=0; i<8; i++)
	*dci_pdu |= (((uint64_t)pdu_rel15->short_messages>>(7-i))&1)<<(dci_alloc->size-pos++);
      // Freq domain assignment 0-16 bit
      fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
      for (int i=0; i<fsize; i++)
	*dci_pdu |= (((uint64_t)pdu_rel15->frequency_domain_assignment>>(fsize-i-1))&1)<<(dci_alloc->size-pos++);
      // Time domain assignment 4 bit
      for (int i=0; i<4; i++)
	*dci_pdu |= (((uint64_t)pdu_rel15->time_domain_assignment>>(3-i))&1)<<(dci_alloc->size-pos++);
      // VRB to PRB mapping 1 bit
      *dci_pdu |= ((uint64_t)pdu_rel15->vrb_to_prb_mapping&1)<<(dci_alloc->size-pos++);
      // MCS 5 bit
      for (int i=0; i<5; i++) 
	*dci_pdu |= (((uint64_t)pdu_rel15->mcs>>(4-i))&1)<<(dci_alloc->size-pos++);

      // TB scaling 2 bit
      for (int i=0; i<2; i++)
	*dci_pdu |= (((uint64_t)pdu_rel15->tb_scaling>>(1-i))&1)<<(dci_alloc->size-pos++);


      break;
      
    case NFAPI_NR_RNTI_SI:
      // Freq domain assignment 0-16 bit
      fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
      for (int i=0; i<fsize; i++)
	*dci_pdu |= ((pdu_rel15->frequency_domain_assignment>>(fsize-i-1))&1)<<(dci_alloc->size-pos++);
      // Time domain assignment 4 bit
      for (int i=0; i<4; i++)
	*dci_pdu |= (((uint64_t)pdu_rel15->time_domain_assignment>>(3-i))&1)<<(dci_alloc->size-pos++);
      // VRB to PRB mapping 1 bit
      *dci_pdu |= ((uint64_t)pdu_rel15->vrb_to_prb_mapping&1)<<(dci_alloc->size-pos++);
      // MCS 5bit  //bit over 32, so dci_pdu ++
      for (int i=0; i<5; i++)
	*dci_pdu |= (((uint64_t)pdu_rel15->mcs>>(4-i))&1)<<(dci_alloc->size-pos++);
      // Redundancy version  2bit
      for (int i=0; i<2; i++)
	*dci_pdu |= (((uint64_t)pdu_rel15->rv>>(1-i))&1)<<(dci_alloc->size-pos++);
      
      break;
      
    case NFAPI_NR_RNTI_TC:
      // indicating a DL DCI format 1bit
      *dci_pdu |= ((uint64_t)pdu_rel15->format_indicator&1)<<(dci_alloc->size-pos++);
      // Freq domain assignment 0-16 bit
      fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
      for (int i=0; i<fsize; i++)
	*dci_pdu |= (((uint64_t)pdu_rel15->frequency_domain_assignment>>(fsize-i-1))&1)<<(dci_alloc->size-pos++);
      // Time domain assignment 4 bit
      for (int i=0; i<4; i++)
	*dci_pdu |= (((uint64_t)pdu_rel15->time_domain_assignment>>(3-i))&1)<<(dci_alloc->size-pos++);
      // VRB to PRB mapping 1 bit
      *dci_pdu |= ((uint64_t)pdu_rel15->vrb_to_prb_mapping&1)<<(dci_alloc->size-pos++);
      // MCS 5bit  //bit over 32, so dci_pdu ++
      for (int i=0; i<5; i++)
	*dci_pdu |= (((uint64_t)pdu_rel15->mcs>>(4-i))&1)<<(dci_alloc->size-pos++);
      // New data indicator 1bit
      *dci_pdu |= ((uint64_t)pdu_rel15->ndi&1)<<(dci_alloc->size-pos++);
      // Redundancy version  2bit
      for (int i=0; i<2; i++)
	*dci_pdu |= (((uint64_t)pdu_rel15->rv>>(1-i))&1)<<(dci_alloc->size-pos++);
      // HARQ process number  4bit  
      for (int i=0; i<4; i++)
	*dci_pdu  |= (((uint64_t)pdu_rel15->harq_pid>>(3-i))&1)<<(dci_alloc->size-pos++);
      
      // Downlink assignment index – 2 bits
      for (int i=0; i<2; i++)
	*dci_pdu  |= (((uint64_t)pdu_rel15->dai>>(1-i))&1)<<(dci_alloc->size-pos++);
    
      // TPC command for scheduled PUCCH – 2 bits
      for (int i=0; i<2; i++)
	*dci_pdu  |= (((uint64_t)pdu_rel15->tpc>>(1-i))&1)<<(dci_alloc->size-pos++);


      //      LOG_I(PHY, "DCI PDU: [0]->0x%08llx \t [1]->0x%08llx \t [2]->0x%08llx \t [3]->0x%08llx\n",
      //	    dci_pdu[0], dci_pdu[1], dci_pdu[2], dci_pdu[3]);


      // PDSCH-to-HARQ_feedback timing indicator – 3 bits
      for (int i=0; i<3; i++)
	*dci_pdu  |= (((uint64_t)pdu_rel15->pdsch_to_harq_feedback_timing_indicator>>(2-i))&1)<<(dci_alloc->size-pos++);
      
      break;
    }
    break;

  case NFAPI_NR_UL_DCI_FORMAT_0_0:
    switch(params_rel15->rnti_type)
      {
      case NFAPI_NR_RNTI_C:
	// indicating a DL DCI format 1bit
	*dci_pdu |= ((uint64_t)pdu_rel15->format_indicator&1)<<(dci_alloc->size-pos++);
	// Freq domain assignment  max 16 bit
	fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
	for (int i=0; i<fsize; i++)
	  *dci_pdu |= ((pdu_rel15->frequency_domain_assignment>>(fsize-i-1))&1)<<(dci_alloc->size-pos++);
	// Time domain assignment 4bit
	for (int i=0; i<4; i++)
	  *dci_pdu |= (((uint64_t)pdu_rel15->time_domain_assignment>>(3-i))&1)<<(dci_alloc->size-pos++);
	// Frequency hopping flag – 1 bit
	*dci_pdu |= ((uint64_t)pdu_rel15->frequency_hopping_flag&1)<<(dci_alloc->size-pos++);
	// MCS  5 bit
	for (int i=0; i<5; i++)
          *dci_pdu |= (((uint64_t)pdu_rel15->mcs>>(4-i))&1)<<(dci_alloc->size-pos++);
	// New data indicator 1bit
	*dci_pdu |= ((uint64_t)pdu_rel15->ndi&1)<<(dci_alloc->size-pos++);
	// Redundancy version  2bit
	for (int i=0; i<2; i++)
	  *dci_pdu |= (((uint64_t)pdu_rel15->rv>>(1-i))&1)<<(dci_alloc->size-pos++);
	// HARQ process number  4bit  
	for (int i=0; i<4; i++)
	  *dci_pdu  |= (((uint64_t)pdu_rel15->harq_pid>>(3-i))&1)<<(dci_alloc->size-pos++);
      
	// TPC command for scheduled PUSCH – 2 bits
        for (int i=0; i<2; i++)
          *dci_pdu |= (((uint64_t)pdu_rel15->tpc>>(1-i))&1)<<(dci_alloc->size-pos++);

	// Padding bits
        for(int a = pos;a<32;a++)
          *dci_pdu |= ((uint64_t)pdu_rel15->padding&1)<<(dci_alloc->size-pos++);

	// UL/SUL indicator – 1 bit
        if (cfg->pucch_config.pucch_GroupHopping.value)
          *dci_pdu |= ((uint64_t)pdu_rel15->ul_sul_indicator&1)<<(dci_alloc->size-pos++);
   
	break;
      
      case NFAPI_NR_RNTI_TC:
      
	// indicating a DL DCI format 1bit
	*dci_pdu |= (pdu_rel15->format_indicator&1)<<(dci_alloc->size-pos++);
	// Freq domain assignment  max 16 bit
	fsize = (int)ceil( log2( (N_RB*(N_RB+1))>>1 ) );
	for (int i=0; i<fsize; i++)
	  *dci_pdu |= ((pdu_rel15->frequency_domain_assignment>>(fsize-i-1))&1)<<(dci_alloc->size-pos++);
	// Time domain assignment 4bit
	for (int i=0; i<4; i++)
	  *dci_pdu |= (((uint64_t)pdu_rel15->time_domain_assignment>>(3-i))&1)<<(dci_alloc->size-pos++);
	// Frequency hopping flag – 1 bit
	*dci_pdu |= ((uint64_t)pdu_rel15->frequency_hopping_flag&1)<<(dci_alloc->size-pos++);
	// MCS  5 bit
	for (int i=0; i<5; i++)
          *dci_pdu |= (((uint64_t)pdu_rel15->mcs>>(4-i))&1)<<(dci_alloc->size-pos++);
	// New data indicator 1bit
	*dci_pdu |= ((uint64_t)pdu_rel15->ndi&1)<<(dci_alloc->size-pos++);
	// Redundancy version  2bit
	for (int i=0; i<2; i++)
          *dci_pdu |= (((uint64_t)pdu_rel15->rv>>(1-i))&1)<<(dci_alloc->size-pos++);
	// HARQ process number  4bit  
	for (int i=0; i<4; i++)
          *dci_pdu  |= (((uint64_t)pdu_rel15->harq_pid>>(3-i))&1)<<(dci_alloc->size-pos++);

        // TPC command for scheduled PUSCH – 2 bits
        for (int i=0; i<2; i++)
          *dci_pdu |= (((uint64_t)pdu_rel15->tpc>>(1-i))&1)<<(dci_alloc->size-pos++);

        // Padding bits
        for(int a = pos;a<32;a++)
	  *dci_pdu |= ((uint64_t)pdu_rel15->padding&1)<<(dci_alloc->size-pos++);

        // UL/SUL indicator – 1 bit
        if (cfg->pucch_config.pucch_GroupHopping.value)
	  *dci_pdu |= ((uint64_t)pdu_rel15->ul_sul_indicator&1)<<(dci_alloc->size-pos++);

        break;
      } 
    break;
  }

  LOG_I(PHY, "DCI PDU: [0]->0x%lx \t [1]->0x%lx \n",dci_pdu[0], dci_pdu[1]);
  LOG_I(PHY, "DCI type %d payload (size %d) generated on candidate %d\n", dci_alloc->pdcch_params.dci_format, dci_alloc->size, cand_idx);

  /// DLSCH struct
  memcpy((void*)&harq[dci_alloc->harq_pid]->dlsch_pdu, (void*)dlsch_pdu, sizeof(nfapi_nr_dl_config_dlsch_pdu));

}
