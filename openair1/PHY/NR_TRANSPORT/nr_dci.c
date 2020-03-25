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

/*! \file PHY/NR_TRANSPORT/nr_dci.c
* \brief Implements DCI encoding and PDCCH TX procedures (38.212/38.213/38.214). V15.4.0 2019-01.
* \author Guy De Souza
* \date 2018
* \version 0.1
* \company Eurecom
* \email: desouza@eurecom.fr
* \note
* \warning
*/

#include "nr_dci.h"
#include "nr_dlsch.h"
#include "nr_sch_dmrs.h"
#include "PHY/MODULATION/nr_modulation.h"

//#define DEBUG_PDCCH_DMRS
//#define DEBUG_DCI
//#define DEBUG_CHANNEL_CODING


uint16_t nr_get_dci_size(nfapi_nr_dci_format_e format,
                         nfapi_nr_rnti_type_e rnti_type,
                         uint16_t N_RB) {
  uint16_t size = 0;

  switch(format) {
    /*Only sizes for 0_0 and 1_0 are correct at the moment*/
    case NFAPI_NR_UL_DCI_FORMAT_0_0:
      /// fixed: Format identifier 1, Hop flag 1, MCS 5, NDI 1, RV 2, HARQ PID 4, PUSCH TPC 2 Time Domain assgnmt 4 --20
      size += 20;
      size += (uint8_t)ceil( log2( (N_RB*(N_RB+1))>>1 ) ); // Freq domain assignment -- hopping scenario to be updated
      size += nr_get_dci_size(NFAPI_NR_DL_DCI_FORMAT_1_0, rnti_type, N_RB) - size; // Padding to match 1_0 size
      // UL/SUL indicator assumed to be 0
      break;

    case NFAPI_NR_UL_DCI_FORMAT_0_1:
      /// fixed: Format identifier 1, MCS 5, NDI 1, RV 2, HARQ PID 4, PUSCH TPC 2, SRS request 2 --17
      size += 17;
      // Carrier indicator
      // UL/SUL indicator
      // BWP Indicator
      // Freq domain assignment
      // Time domain assignment
      // VRB to PRB mapping
      // Frequency Hopping flag
      // 1st DAI
      // 2nd DAI
      // SRS resource indicator
      // Precoding info and number of layers
      // Antenna ports
      // CSI request
      // CBGTI
      // PTRS - DMRS association
      // beta offset indicator
      // DMRS sequence init
      break;

    case NFAPI_NR_DL_DCI_FORMAT_1_0:
      /// fixed: Format identifier 1, VRB2PRB 1, MCS 5, NDI 1, RV 2, HARQ PID 4, DAI 2, PUCCH TPC 2, PUCCH RInd 3, PDSCH to HARQ TInd 3 Time Domain assgnmt 4 -- 28
      size += 28;
      size += (uint8_t)ceil( log2( (N_RB*(N_RB+1))>>1 ) ); // Freq domain assignment
      break;

    case NFAPI_NR_DL_DCI_FORMAT_1_1:
      // Carrier indicator
      size += 1; // Format identifier
      // BWP Indicator
      // Freq domain assignment
      // Time domain assignment
      // VRB to PRB mapping
      // PRB bundling size indicator
      // Rate matching indicator
      // ZP CSI-RS trigger
      /// TB1- MCS 5, NDI 1, RV 2
      size += 8;
      // TB2
      size += 4 ;  // HARQ PID
      // DAI
      size += 2; // TPC PUCCH
      size += 3; // PUCCH resource indicator
      size += 3; // PDSCH to HARQ timing indicator
      // Antenna ports
      // Tx Config Indication
      size += 2; // SRS request
      // CBGTI
      // CBGFI
      size += 1; // DMRS sequence init
      break;

    case NFAPI_NR_DL_DCI_FORMAT_2_0:
      break;

    case NFAPI_NR_DL_DCI_FORMAT_2_1:
      break;

    case NFAPI_NR_DL_DCI_FORMAT_2_2:
      break;

    case NFAPI_NR_DL_DCI_FORMAT_2_3:
      break;

    default:
      AssertFatal(1==0, "Invalid NR DCI format %d\n", format);
  }

  return size;
}

void nr_pdcch_scrambling(uint32_t *in,
                         uint32_t size,
                         uint32_t Nid,
                         uint32_t n_RNTI,
                         uint32_t *out) {
  uint8_t reset;
  uint32_t x1, x2, s=0;
  reset = 1;
  x2 = (n_RNTI<<16) + Nid;
  for (int i=0; i<size; i++) {
    if ((i&0x1f)==0) {
      s = lte_gold_generic(&x1, &x2, reset);
      reset = 0;

      if (i) {
        in++;
        out++;
      }
    }

    (*out) ^= ((((*in)>>(i&0x1f))&1) ^ ((s>>(i&0x1f))&1))<<(i&0x1f);
  }
}


uint8_t nr_generate_dci_top(nfapi_nr_dl_tti_pdcch_pdu *pdcch_pdu,
			    nfapi_nr_ul_dci_request_pdus_t    *ul_dci_pdu,
                            uint32_t **gold_pdcch_dmrs,
                            int32_t *txdataF,
                            int16_t amp,
                            NR_DL_FRAME_PARMS frame_parms) {

  int16_t mod_dmrs[NR_MAX_CSET_DURATION][NR_MAX_PDCCH_DMRS_LENGTH>>1]; // 3 for the max coreset duration
  uint16_t cset_start_sc;
  uint8_t cset_start_symb, cset_nsymb;
  int k,l,k_prime,dci_idx, dmrs_idx;
  /*First iteration: single DCI*/

  nfapi_nr_dl_tti_pdcch_pdu_rel15_t *pdcch_pdu_rel15=NULL;


  // find coreset descriptor
    
  int rb_offset;
  int n_rb;

  AssertFatal(pdcch_pdu!=NULL || ul_dci_pdu!=NULL,"At least one pointer has to be !NULL\n");
  AssertFatal(pdcch_pdu==NULL || ul_dci_pdu==NULL,"Can't handle both DL and UL DCI in same slot\n");


  if (pdcch_pdu) pdcch_pdu_rel15 = &pdcch_pdu->pdcch_pdu_rel15;
  else if (ul_dci_pdu) pdcch_pdu_rel15 = &ul_dci_pdu->pdcch_pdu.pdcch_pdu_rel15;

  get_coreset_rballoc(pdcch_pdu_rel15->FreqDomainResource,&n_rb,&rb_offset);

  // compute rb_offset and n_prb based on frequency allocation

  if (pdcch_pdu_rel15->CoreSetType == NFAPI_NR_CSET_CONFIG_MIB_SIB1) {
    cset_start_sc = frame_parms.first_carrier_offset + (frame_parms.ssb_start_subcarrier/NR_NB_SC_PER_RB +
							rb_offset)*NR_NB_SC_PER_RB;
  } else
    cset_start_sc = frame_parms.first_carrier_offset + rb_offset*NR_NB_SC_PER_RB;

  for (int d=0;d<pdcch_pdu_rel15->numDlDci;d++) {
    /*The coreset is initialised
     * in frequency: the first subcarrier is obtained by adding the first CRB overlapping the SSB and the rb_offset for coreset 0
     * or the rb_offset for other coresets
     * in time: by its first slot and its first symbol*/

    cset_start_symb = pdcch_pdu_rel15->StartSymbolIndex;
    cset_nsymb = pdcch_pdu_rel15->DurationSymbols;
    dci_idx = 0;
    LOG_D(PHY, "Coreset rb_offset %d, nb_rb %d\n",rb_offset,n_rb);
    LOG_D(PHY, "Coreset starting subcarrier %d on symbol %d (%d symbols)\n", cset_start_sc, cset_start_symb, cset_nsymb);
    // DMRS length is per OFDM symbol
    AssertFatal(pdcch_pdu_rel15->CceRegMappingType == NFAPI_NR_CCE_REG_MAPPING_NON_INTERLEAVED,
		"Interleaved CCE REG MAPPING not supported\n");
    uint32_t dmrs_length = (pdcch_pdu_rel15->CceRegMappingType == NFAPI_NR_CCE_REG_MAPPING_NON_INTERLEAVED)?
      (n_rb*6) : (pdcch_pdu_rel15->AggregationLevel[d]*36/cset_nsymb); //2(QPSK)*3(per RB)*6(REG per CCE)
    uint32_t encoded_length = pdcch_pdu_rel15->AggregationLevel[d]*108; //2(QPSK)*9(per RB)*6(REG per CCE)
    LOG_D(PHY, "DMRS length per symbol %d\t DCI encoded length %d (precoder_granularity %d,reg_mapping %d)\n", dmrs_length, encoded_length,pdcch_pdu_rel15->precoderGranularity,pdcch_pdu_rel15->CceRegMappingType);
    dmrs_length += rb_offset*6; // To accommodate more DMRS symbols in case of rb offset
      
    /// DMRS QPSK modulation
    for (int symb=cset_start_symb; symb<cset_start_symb + pdcch_pdu_rel15->DurationSymbols; symb++) {
      
      nr_modulation(gold_pdcch_dmrs[symb], dmrs_length, DMRS_MOD_ORDER, mod_dmrs[symb]); //Qm = 2 as DMRS is QPSK modulated
      
#ifdef DEBUG_PDCCH_DMRS
      
      for (int i=0; i<dmrs_length>>1; i++)
	printf("symb %d i %d gold seq 0x%08x mod_dmrs %d %d\n", symb, i,
	       gold_pdcch_dmrs[symb][i>>5], mod_dmrs[symb][i<<1], mod_dmrs[symb][(i<<1)+1] );
      
#endif
    }
    
    /// DCI payload processing
    // CRC attachment + Scrambling + Channel coding + Rate matching
    uint32_t encoder_output[NR_MAX_DCI_SIZE_DWORD];
    uint16_t n_RNTI = pdcch_pdu_rel15->RNTI[d];
    uint16_t Nid    = pdcch_pdu_rel15->ScramblingId[d];
    
    t_nrPolar_params *currentPtr = nr_polar_params(NR_POLAR_DCI_MESSAGE_TYPE, 
						   pdcch_pdu_rel15->PayloadSizeBits[d], 
						   pdcch_pdu_rel15->AggregationLevel[d],
						   0,NULL);
    polar_encoder_fast((uint64_t*)pdcch_pdu_rel15->Payload[d], encoder_output, n_RNTI,1,currentPtr);
#ifdef DEBUG_CHANNEL_CODING
    printf("polar rnti %x,length %d, L %d\n",n_RNTI, pdcch_pdu_rel15->PayloadSizeBits[d],pdcch_pdu_rel15->AggregationLevel[d]);
    printf("DCI PDU: [0]->0x%lx \t [1]->0x%lx\n",
	   ((uint64_t*)pdcch_pdu_rel15->Payload[d])[0], ((uint64_t*)pdcch_pdu_rel15->Payload[d])[1]);
    printf("Encoded Payload (length:%d dwords):\n", encoded_length>>5);
    
    for (int i=0; i<encoded_length>>5; i++)
      printf("[%d]->0x%08x \t", i,encoder_output[i]);    

    printf("\n");
#endif
    /// Scrambling
    uint32_t scrambled_output[NR_MAX_DCI_SIZE_DWORD]= {0};
    nr_pdcch_scrambling(encoder_output, encoded_length, Nid, n_RNTI, scrambled_output);
#ifdef DEBUG_CHANNEL_CODING
    printf("scrambled output: [0]->0x%08x \t [1]->0x%08x \t [2]->0x%08x \t [3]->0x%08x\t [4]->0x%08x\t [5]->0x%08x\t \
[6]->0x%08x \t [7]->0x%08x \t [8]->0x%08x \t [9]->0x%08x\t [10]->0x%08x\t [11]->0x%08x\n",
	   scrambled_output[0], scrambled_output[1], scrambled_output[2], scrambled_output[3], scrambled_output[4],scrambled_output[5],
	   scrambled_output[6], scrambled_output[7], scrambled_output[8], scrambled_output[9], scrambled_output[10],scrambled_output[11] );
#endif
    /// QPSK modulation
    int16_t mod_dci[NR_MAX_DCI_SIZE>>1];
    nr_modulation(scrambled_output, encoded_length, DMRS_MOD_ORDER, mod_dci); //Qm = 2 as DMRS is QPSK modulated
#ifdef DEBUG_DCI
    
    for (int i=0; i<encoded_length>>1; i++)
      printf("i %d mod_dci %d %d\n", i, mod_dci[i<<1], mod_dci[(i<<1)+1] );
    
#endif
    
    /// Resource mapping
    
    if (cset_start_sc >= frame_parms.ofdm_symbol_size)
      cset_start_sc -= frame_parms.ofdm_symbol_size;
    
    /*Reorder REG list for a freq first mapping*/
    uint8_t nb_regs = pdcch_pdu_rel15->AggregationLevel[d]*NR_NB_REG_PER_CCE;
    uint8_t reg_idx0 = pdcch_pdu_rel15->CceIndex[d]*NR_NB_REG_PER_CCE;

    /*Mapping the encoded DCI along with the DMRS */
    for (int reg_idx=reg_idx0; reg_idx<(nb_regs+reg_idx0); reg_idx++) {
      k = cset_start_sc + (12*reg_idx/cset_nsymb);
      
      if (k >= frame_parms.ofdm_symbol_size)
	k -= frame_parms.ofdm_symbol_size;
      
      l = cset_start_symb + ((reg_idx/cset_nsymb)%cset_nsymb);
      
      // dmrs index depends on reference point for k according to 38.211 7.4.1.3.2
      if (pdcch_pdu_rel15->CoreSetType == NFAPI_NR_CSET_CONFIG_PDCCH_CONFIG)
	dmrs_idx = (reg_idx/cset_nsymb)*3;
      else
	dmrs_idx = ((reg_idx/cset_nsymb)+rb_offset)*3;
      
      k_prime = 0;
      
      for (int m=0; m<NR_NB_SC_PER_RB; m++) {
	if ( m == (k_prime<<2)+1) { // DMRS if not already mapped
	  if (pdcch_pdu_rel15->CceRegMappingType == NFAPI_NR_CCE_REG_MAPPING_NON_INTERLEAVED) {
	    ((int16_t *)txdataF)[(l*frame_parms.ofdm_symbol_size + k)<<1]       = (2*amp * mod_dmrs[l][dmrs_idx<<1]) >> 15;
	    ((int16_t *)txdataF)[((l*frame_parms.ofdm_symbol_size + k)<<1) + 1] = (2*amp * mod_dmrs[l][(dmrs_idx<<1) + 1]) >> 15;
#ifdef DEBUG_PDCCH_DMRS
	    printf("PDCCH DMRS: l %d position %d => (%d,%d)\n",l,k,((int16_t *)txdataF)[(l*frame_parms.ofdm_symbol_size + k)<<1],
		   ((int16_t *)txdataF)[((l*frame_parms.ofdm_symbol_size + k)<<1)+1]);
#endif
	    dmrs_idx++;
	  }
	  
	  k_prime++;
	} else { // DCI payload
	  ((int16_t *)txdataF)[(l*frame_parms.ofdm_symbol_size + k)<<1]       = (amp * mod_dci[dci_idx<<1]) >> 15;
	  ((int16_t *)txdataF)[((l*frame_parms.ofdm_symbol_size + k)<<1) + 1] = (amp * mod_dci[(dci_idx<<1) + 1]) >> 15;
#ifdef DEBUG_DCI
	  printf("PDCCH: l %d position %d => (%d,%d)\n",l,k,((int16_t *)txdataF)[(l*frame_parms.ofdm_symbol_size + k)<<1],
		 ((int16_t *)txdataF)[((l*frame_parms.ofdm_symbol_size + k)<<1)+1]);
#endif
	  dci_idx++;
	}
	
	k++;
	
	if (k >= frame_parms.ofdm_symbol_size)
	  k -= frame_parms.ofdm_symbol_size;
      } // m
    } // reg_idx
  } // for (int d=0;d<pdcch_pdu_rel15->numDlDci;d++)
  return 0;
}

