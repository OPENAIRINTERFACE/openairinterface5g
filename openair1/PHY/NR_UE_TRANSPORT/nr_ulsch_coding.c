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

/*! \file PHY/NR_UE_TRANSPORT/nr_ulsch_coding.c
* \brief Top-level routines for coding the ULSCH transport channel as described in 38.212 V15.4 2018-12
* \author Khalid Ahmed
* \date 2019
* \version 0.1
* \company Fraunhofer IIS
* \email: khalid.ahmed@iis.fraunhofer.de
* \note
* \warning
*/

#include "PHY/defs_UE.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "PHY/CODING/coding_defs.h"
#include "PHY/CODING/coding_extern.h"
#include "PHY/CODING/lte_interleaver_inline.h"
#include "PHY/CODING/nrLDPC_extern.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_ue.h"
#include "common/utils/LOG/vcd_signal_dumper.h"

//#define DEBUG_ULSCH_CODING

int nr_ulsch_encoding(PHY_VARS_NR_UE *ue,
                      NR_UE_ULSCH_t *ulsch,
                      NR_DL_FRAME_PARMS* frame_parms,
                      uint8_t harq_pid,
                      uint32_t tb_size,
                      unsigned int G)
{
  start_meas(&ue->ulsch_encoding_stats);

/////////////////////////parameters and variables initialization/////////////////////////
///////////

  unsigned int crc = 1;
  NR_UL_UE_HARQ_t *harq_process = &ue->ul_harq_processes[harq_pid];
  uint16_t nb_rb = ulsch->pusch_pdu.rb_size;
  uint32_t A = tb_size << 3;
  uint8_t mod_order = ulsch->pusch_pdu.qam_mod_order;
  uint16_t Kr = 0;
  uint32_t r_offset = 0;
  uint32_t F = 0;
  // target_code_rate is in 0.1 units
  float Coderate = (float) ulsch->pusch_pdu.target_code_rate / 10240.0f;

///////////
/////////////////////////////////////////////////////////////////////////////////////////  

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_UE_ULSCH_ENCODING, VCD_FUNCTION_IN);

  LOG_D(NR_PHY, "ulsch coding nb_rb %d, Nl = %d\n", nb_rb, ulsch->pusch_pdu.nrOfLayers);
  LOG_D(NR_PHY, "ulsch coding A %d G %d mod_order %d Coderate %f\n", A, G, mod_order, Coderate);
  LOG_D(NR_PHY, "harq_pid %d, pusch_data.new_data_indicator %d\n", harq_pid, ulsch->pusch_pdu.pusch_data.new_data_indicator);
  if (ulsch->pusch_pdu.pusch_data.new_data_indicator) {  // this is a new packet
#ifdef DEBUG_ULSCH_CODING
  printf("encoding thinks this is a new packet \n");
#endif
///////////////////////// a---->| add CRC |---->b /////////////////////////
///////////
   /* 
    int i;
    printf("ulsch (tx): \n");
    for (i=0;i<(A>>3);i++)
      printf("%02x.",harq_process->a[i]);
    printf("\n");
   */ 

    int max_payload_bytes = MAX_NUM_NR_ULSCH_SEGMENTS_PER_LAYER*ulsch->pusch_pdu.nrOfLayers*1056;
    int B;
    if (A > NR_MAX_PDSCH_TBS) {
      // Add 24-bit crc (polynomial A) to payload
      crc = crc24a(harq_process->a,A)>>8;
      harq_process->a[A>>3] = ((uint8_t*)&crc)[2];
      harq_process->a[1+(A>>3)] = ((uint8_t*)&crc)[1];
      harq_process->a[2+(A>>3)] = ((uint8_t*)&crc)[0];
      //printf("CRC %x (A %d)\n",crc,A);
      //printf("a0 %d a1 %d a2 %d\n", a[A>>3], a[1+(A>>3)], a[2+(A>>3)]);

      B = A + 24;

      AssertFatal((A/8)+4 <= max_payload_bytes,"A %d is too big (A/8+4 = %d > %d)\n",A,(A/8)+4,max_payload_bytes);

      memcpy(harq_process->b,harq_process->a,(A/8)+4);
    } else {
      // Add 16-bit crc (polynomial A) to payload
      crc = crc16(harq_process->a,A)>>16;
      harq_process->a[A>>3] = ((uint8_t*)&crc)[1];
      harq_process->a[1+(A>>3)] = ((uint8_t*)&crc)[0];
      //printf("CRC %x (A %d)\n",crc,A);
      //printf("a0 %d a1 %d \n", a[A>>3], a[1+(A>>3)]);

      B = A + 16;

      AssertFatal((A/8)+3 <= max_payload_bytes,"A %d is too big (A/8+3 = %d > %d)\n",A,(A/8)+3,max_payload_bytes);

      memcpy(harq_process->b,harq_process->a,(A/8)+3);  // using 3 bytes to mimic the case of 24 bit crc
    }
///////////
///////////////////////////////////////////////////////////////////////////

///////////////////////// b---->| block segmentation |---->c /////////////////////////
///////////

    if ((A <= 292) || ((A <= NR_MAX_PDSCH_TBS) && (Coderate <= 0.6667)) || Coderate <= 0.25) {
      harq_process->BG = 2;
    } else {
      harq_process->BG = 1;
    }

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_SEGMENTATION, VCD_FUNCTION_IN);
    start_meas(&ue->ulsch_segmentation_stats);
    uint32_t Kb = nr_segmentation(harq_process->b,
                                  harq_process->c,
                                  B,
                                  &harq_process->C,
                                  &harq_process->K,
                                  &harq_process->Z,
                                  &harq_process->F,
                                  harq_process->BG);

    if (harq_process->C>MAX_NUM_NR_DLSCH_SEGMENTS_PER_LAYER*ulsch->pusch_pdu.nrOfLayers) {
      LOG_E(PHY, "nr_segmentation.c: too many segments %d, B %d\n", harq_process->C, B);
      return(-1);
    }
    stop_meas(&ue->ulsch_segmentation_stats);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_SEGMENTATION, VCD_FUNCTION_OUT);

    F = harq_process->F;
    Kr = harq_process->K;
#ifdef DEBUG_ULSCH_CODING
    uint16_t Kr_bytes;
    Kr_bytes = Kr>>3;
#endif

///////////////////////// c---->| LDCP coding |---->d /////////////////////////
///////////

    // printf("segment Z %d k %d Kr %d BG %d\n", harq_process->Z,harq_process->K,Kr,BG);

    //start_meas(te_stats);
    for (int r=0; r<harq_process->C; r++) {
      //channel_input[r] = &harq_process->d[r][0];
#ifdef DEBUG_ULSCH_CODING
      printf("Encoder: B %d F %d \n", B, harq_process->F);
      printf("start ldpc encoder segment %d/%d\n",r,harq_process->C);
      printf("input %d %d %d %d %d \n", harq_process->c[r][0], harq_process->c[r][1], harq_process->c[r][2],harq_process->c[r][3], harq_process->c[r][4]);
      for (int cnt = 0; cnt < 22 * harq_process->Z / 8; cnt++) {
        printf("%d ", harq_process->c[r][cnt]);
      }
      printf("\n");

#endif
      //ldpc_encoder_orig((unsigned char*)harq_process->c[r],harq_process->d[r],Kr,BG,0);
      //ldpc_encoder_optim((unsigned char*)harq_process->c[r],(unsigned char*)&harq_process->d[r][0],Kr,BG,NULL,NULL,NULL,NULL);
    }

    //for (int i=0;i<68*384;i++)
      //        printf("channel_input[%d]=%d\n",i,channel_input[i]);

    /*printf("output %d %d %d %d %d \n", harq_process->d[0][0], harq_process->d[0][1], harq_process->d[r][2],harq_process->d[0][3],
      harq_process->d[0][4]); for (int cnt =0 ; cnt < 66*harq_process->Z; cnt ++){ printf("%d \n",  harq_process->d[0][cnt]);
      }
      printf("\n");*/
    encoder_implemparams_t impp = {.n_segments = harq_process->C,
                                   .macro_num = 0,
                                   .K = harq_process->K,
                                   .Kb = Kb,
                                   .Zc = harq_process->Z,
                                   .BG = harq_process->BG,
                                   .tinput = NULL,
                                   .tprep = NULL,
                                   .tparity = NULL,
                                   .toutput = NULL};

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_LDPC_ENCODER_OPTIM, VCD_FUNCTION_IN);

    start_meas(&ue->ulsch_ldpc_encoding_stats);
    if (ldpc_interface_offload.LDPCencoder)
      for (int j = 0; j < harq_process->C; j++) {
        impp.macro_num = j;
        impp.E = nr_get_E(G, harq_process->C, mod_order, ulsch->pusch_pdu.nrOfLayers, j);
        impp.Kr = Kr;
        ldpc_interface_offload.LDPCencoder(&harq_process->c[j], &harq_process->d[j], &impp);
      }
    else
      for (int j = 0; j < (harq_process->C / 8 + 1); j++) {
        impp.macro_num = j;
        impp.E = nr_get_E(G, harq_process->C, mod_order, ulsch->pusch_pdu.nrOfLayers, j);
        impp.Kr = Kr;
        ldpc_interface.LDPCencoder(harq_process->c, harq_process->d, &impp);
      }
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_LDPC_ENCODER_OPTIM, VCD_FUNCTION_OUT);

    //stop_meas(te_stats);
    //printf("end ldpc encoder -- output\n");
#ifdef DEBUG_ULSCH_CODING
    write_output("ulsch_enc_input0.m","enc_in0",&harq_process->c[0][0],Kr_bytes,1,4);
    write_output("ulsch_enc_output0.m","enc0",&harq_process->d[0][0],(3*8*Kr_bytes)+12,1,4);
#endif

///////////
///////////////////////////////////////////////////////////////////////////////
  }
  F = harq_process->F;
  Kr = harq_process->K;

  for (int r=0; r<harq_process->C; r++) { // looping over C segments
    if (harq_process->F>0) {
      for (int k = Kr - F - 2 * harq_process->Z; k < Kr - 2 * harq_process->Z; k++) {
        harq_process->d[r][k] = NR_NULL;
        //if (k<(Kr-F+8))
        //printf("r %d filler bits [%d] = %d \n", r,k, harq_process->d[r][k]);
      }
    }


    LOG_D(PHY,"Rate Matching, Code segment %d (coded bits (G) %u, unpunctured/repeated bits per code segment %d, mod_order %d, nb_rb %d, rvidx %d)...\n",
	  r,
	  G,
	  Kr*3,
	  mod_order,nb_rb,
	  ulsch->pusch_pdu.pusch_data.rv_index);

    //start_meas(rm_stats);
///////////////////////// d---->| Rate matching bit selection |---->e /////////////////////////
///////////

    uint32_t E = nr_get_E(G, harq_process->C, mod_order, ulsch->pusch_pdu.nrOfLayers, r);

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_RATE_MATCHING_LDPC, VCD_FUNCTION_IN);
    start_meas(&ue->ulsch_rate_matching_stats);
    if (nr_rate_matching_ldpc(ulsch->pusch_pdu.tbslbrm,
                              harq_process->BG,
                              harq_process->Z,
                              harq_process->d[r],
                              harq_process->e + r_offset,
                              harq_process->C,
                              F,
                              Kr - F - 2 * harq_process->Z,
                              ulsch->pusch_pdu.pusch_data.rv_index,
                              E)
        == -1)
      return -1;

    stop_meas(&ue->ulsch_rate_matching_stats);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_RATE_MATCHING_LDPC, VCD_FUNCTION_OUT);



#ifdef DEBUG_ULSCH_CODING
    for (int i =0; i<16; i++)
      printf("output ratematching e[%d]= %d r_offset %u\n", i,harq_process->e[i+r_offset], r_offset);
#endif

///////////
///////////////////////////////////////////////////////////////////////////////////////////////

    
///////////////////////// e---->| Rate matching bit interleaving |---->f /////////////////////////
///////////

    //stop_meas(rm_stats);

    //start_meas(i_stats);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_INTERLEAVING_LDPC, VCD_FUNCTION_IN);
    
    start_meas(&ue->ulsch_interleaving_stats);
    nr_interleaving_ldpc(E,
                         mod_order,
                         harq_process->e+r_offset,
                         harq_process->f+r_offset);
    stop_meas(&ue->ulsch_interleaving_stats);
    
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_INTERLEAVING_LDPC, VCD_FUNCTION_OUT);
    //stop_meas(i_stats);


#ifdef DEBUG_ULSCH_CODING
    for (int i =0; i<16; i++)
      printf("output interleaving f[%d]= %d r_offset %u\n", i,harq_process->f[i+r_offset], r_offset);

    if (r==harq_process->C-1)
      write_output("enc_output.m","enc",harq_process->f,G,1,4);
#endif

    r_offset += E;

///////////
///////////////////////////////////////////////////////////////////////////////////////////////

  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_UE_ULSCH_ENCODING, VCD_FUNCTION_OUT);

  stop_meas(&ue->ulsch_encoding_stats);
  return(0);
}
