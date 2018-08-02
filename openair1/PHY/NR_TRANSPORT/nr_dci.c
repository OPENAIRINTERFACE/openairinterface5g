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
* \brief Implements DCI encoding/decoding and PDCCH TX/RX procedures (38.212/38.213/38.214). V15.2.0 2018-06.
* \author Guy De Souza
* \date 2018
* \version 0.1
* \company Eurecom
* \email: desouza@eurecom.fr
* \note
* \warning
*/

#include "nr_dci.h"

#define DEBUG_PDCCH_DMRS
#define DEBUG_DCI

extern short nr_mod_table[NR_MOD_TABLE_SIZE_SHORT];

uint16_t nr_get_dci_size(nfapi_nr_dci_format_e format,
                        nfapi_nr_rnti_type_e rnti_type,
                        NR_BWP_PARMS* bwp,
                        nfapi_nr_config_request_t* config)
{
  uint16_t size = 0;
  uint16_t N_RB = bwp->N_RB;

  switch(format) {
/*Only sizes for 0_0 and 1_0 are correct at the moment*/
    case NFAPI_NR_UL_DCI_FORMAT_0_0:
      /// fixed: Format identifier 1, Hop flag 1, MCS 5, NDI 1, RV 2, HARQ PID 4, PUSCH TPC 2 Time Domain assgnmt 4 --20
      size += 20;
      size += (uint8_t)ceil( log2( (N_RB*(N_RB+1))>>1 ) ); // Freq domain assignment -- hopping scenario to be updated
      // UL/SUL indicator assumed to be 0
      // Padding
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
      // Time domain assignment
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
                         uint8_t size,
                         uint32_t Nid,
                         uint32_t n_RNTI,
                         uint32_t* out) {

  uint8_t reset;
  uint32_t x1, x2, s=0;

  reset = 1;
  x2 = (n_RNTI<<16) + Nid;

  for (int i=0; i<size; i++) {
    if ((i&0x1f)==0) {
      s = lte_gold_generic(&x1, &x2, reset);
      reset = 0;
    }
    *out ^= (((*in)>>i)&1) ^ ((s>>i)&1);
  }  

}

uint8_t nr_generate_dci_top(NR_gNB_PDCCH pdcch_vars,
                            uint32_t *gold_pdcch_dmrs,
                            int32_t** txdataF,
                            int16_t amp,
                            NR_DL_FRAME_PARMS frame_parms,
                            nfapi_nr_config_request_t config)
{

  uint16_t mod_dmrs[NR_MAX_PDCCH_DMRS_LENGTH<<1];
  uint8_t idx=0;
  uint16_t a;
  int k,l,k_prime,dci_idx, dmrs_idx;
  nr_cce_t cce;

  /*First iteration: single DCI*/
  NR_gNB_DCI_ALLOC_t dci_alloc = pdcch_vars.dci_alloc[0];
  nfapi_nr_dl_config_pdcch_parameters_rel15_t pdcch_params = dci_alloc.pdcch_params;

  /// DMRS QPSK modulation
    /*There is a need to shift from which index the pregenerated DMRS sequence is used
     * see 38211 r15.2.0 section 7.4.1.3.2: assumption is the reference point for k refers to the DMRS sequence*/
  if (pdcch_params.config_type == NFAPI_NR_CSET_CONFIG_PDCCH_CONFIG)
    gold_pdcch_dmrs += ((int)floor(frame_parms.ssb_start_subcarrier/NR_NB_SC_PER_RB)+pdcch_params.rb_offset)*3/32;

  for (int i=0; i<NR_MAX_PDCCH_DMRS_LENGTH>>1; i++) {
    idx = ((((gold_pdcch_dmrs[(i<<1)>>5])>>((i<<1)&0x1f))&1)<<1) ^ (((gold_pdcch_dmrs[((i<<1)+1)>>5])>>(((i<<1)+1)&0x1f))&1);
    mod_dmrs[i<<1] = nr_mod_table[(NR_MOD_TABLE_QPSK_OFFSET + idx)<<1];
    mod_dmrs[(i<<1)+1] = nr_mod_table[((NR_MOD_TABLE_QPSK_OFFSET + idx)<<1) + 1];
#ifdef DEBUG_PDCCH_DMRS
  printf("DMRS modulation\n");
  printf("i %d idx %d gold seq %d b0-b1 %d-%d mod_dmrs %d %d\n", i, idx, gold_pdcch_dmrs[(i<<1)>>5], (((gold_pdcch_dmrs[(i<<1)>>5])>>((i<<1)&0x1f))&1),
  (((gold_pdcch_dmrs[((i<<1)+1)>>5])>>(((i<<1)+1)&0x1f))&1), mod_dmrs[(i<<1)], mod_dmrs[(i<<1)+1]);
#endif
  }

  /// DCI payload processing
    //channel coding
  
    // scrambling
  uint32_t scrambled_payload[4];
  uint32_t Nid = (dci_alloc.search_space_type == NFAPI_NR_SEARCH_SPACE_TYPE_UE_SPECIFIC)? pdcch_params.scrambling_id : config.sch_config.physical_cell_id.value;
  uint32_t n_RNTI = (dci_alloc.search_space_type == NFAPI_NR_SEARCH_SPACE_TYPE_UE_SPECIFIC)? dci_alloc.rnti : 0;
  nr_pdcch_scrambling(dci_alloc.dci_pdu, dci_alloc.size, Nid, n_RNTI, scrambled_payload);

    // QPSK modulation
  uint32_t mod_dci[NR_MAX_DCI_SIZE>>1];
  for (int i=0; i<dci_alloc.size>>1; i++) {
    idx = (((scrambled_payload[i<<1]>>(i<<1))&1)<<1) ^ ((scrambled_payload[(i<<1)+1]>>((i<<1)+1))&1);
    mod_dci[i<<1] = nr_mod_table[(NR_MOD_TABLE_QPSK_OFFSET + idx)<<1];
    mod_dci[(i<<1)+1] = nr_mod_table[((NR_MOD_TABLE_QPSK_OFFSET + idx)<<1) + 1];
#ifdef DEBUG_DCI
  printf("DCI modulation\n");
  printf("i %d idx %d b0-b1 %d-%d mod_dmrs %d %d\n", i, idx, (((scrambled_payload[(i<<1)>>5])>>((i<<1)&0x1f))&1),
  (((scrambled_payload[((i<<1)+1)>>5])>>(((i<<1)+1)&0x1f))&1), mod_dci[(i<<1)], mod_dci[(i<<1)+1]);
#endif
  }

  /// Resource mapping
  a = (config.rf_config.tx_antenna_ports.value == 1) ? amp : (amp*ONE_OVER_SQRT2_Q15)>>15;

   /*The coreset is initialised
    * in frequency: the first subcarrier is obtained by adding the first CRB overlapping the SSB and the rb_offset
    * in time: by its first slot and its first symbol*/
  uint8_t cset_start_sc = frame_parms.first_carrier_offset + ((int)floor(frame_parms.ssb_start_subcarrier/NR_NB_SC_PER_RB)+pdcch_params.rb_offset)*NR_NB_SC_PER_RB;
  uint8_t cset_start_symb = pdcch_params.first_slot*frame_parms.symbols_per_slot + pdcch_params.first_symbol;
  dci_idx = 0;
  dmrs_idx = 0;

  for (int aa = 0; aa < config.rf_config.tx_antenna_ports.value; aa++)
  {
    if (cset_start_sc >= frame_parms.ofdm_symbol_size)
      cset_start_sc -= frame_parms.ofdm_symbol_size;

    if (pdcch_params.precoder_granularity == NFAPI_NR_CSET_SAME_AS_REG_BUNDLE) {

      for (int cce_idx=0; cce_idx<dci_alloc.L; cce_idx++){
        cce = dci_alloc.cce_list[cce_idx];
          for (int reg_idx=0; reg_idx<NR_NB_REG_PER_CCE; reg_idx++) {
            k = cset_start_sc + cce.reg_list[reg_idx].start_sc_idx;
            l = cset_start_symb + cce.reg_list[reg_idx].symb_idx;
            k_prime = 0;
            for (int m=0; m<NR_NB_SC_PER_RB; m++) {
              if ( m == (k_prime<<2)+1) { // DMRS
                ((int16_t*)txdataF[aa])[(l*frame_parms.ofdm_symbol_size + k)<<1] = (a * mod_dmrs[dmrs_idx<<1]) >> 15;
                ((int16_t*)txdataF[aa])[((l*frame_parms.ofdm_symbol_size + k)<<1) + 1] = (a * mod_dmrs[(dmrs_idx<<1) + 1]) >> 15;
                k_prime++;
                dmrs_idx++;
              }
              else { // DCI payload
                ((int16_t*)txdataF[aa])[(l*frame_parms.ofdm_symbol_size + k)<<1] = (a * mod_dci[dci_idx<<1]) >> 15;
                ((int16_t*)txdataF[aa])[((l*frame_parms.ofdm_symbol_size + k)<<1) + 1] = (a * mod_dci[(dci_idx<<1) + 1]) >> 15;
                dci_idx++;
              }
              k++;
              if (k >= frame_parms.ofdm_symbol_size)
                k -= frame_parms.ofdm_symbol_size;
            }
        }
      }
    }

    else { //NFAPI_NR_CSET_ALL_CONTIGUOUS_RBS
    }

  }

  return 0;
}
