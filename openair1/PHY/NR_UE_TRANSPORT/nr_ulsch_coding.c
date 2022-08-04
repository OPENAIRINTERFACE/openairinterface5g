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
#include "PHY/phy_extern_ue.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_proto_ue.h"
#include "PHY/CODING/coding_defs.h"
#include "PHY/CODING/coding_extern.h"
#include "PHY/CODING/lte_interleaver_inline.h"
#include "PHY/CODING/nrLDPC_extern.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_ue.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include <openair2/UTIL/OPT/opt.h>

//#define DEBUG_ULSCH_CODING

void free_nr_ue_ulsch(NR_UE_ULSCH_t **ulschptr,
                      uint16_t N_RB_UL,
                      NR_DL_FRAME_PARMS* frame_parms) {

  NR_UE_ULSCH_t *ulsch = *ulschptr;

  if (ulsch) {
#ifdef DEBUG_ULSCH_FREE
    printf("Freeing ulsch %p\n",ulsch);
#endif

    int max_layers = (frame_parms->nb_antennas_tx<NR_MAX_NB_LAYERS) ? frame_parms->nb_antennas_tx : NR_MAX_NB_LAYERS;
    uint16_t a_segments = MAX_NUM_NR_ULSCH_SEGMENTS_PER_LAYER*max_layers;  //number of segments to be allocated

    if (N_RB_UL != 273) {
      a_segments = a_segments*N_RB_UL;
      a_segments = a_segments/273 +1;
    }

    for (int i=0; i<NR_MAX_ULSCH_HARQ_PROCESSES; i++) {
      if (ulsch->harq_processes[i]) {

        if (ulsch->harq_processes[i]->a) {
          free16(ulsch->harq_processes[i]->a,a_segments*1056);
          ulsch->harq_processes[i]->a = NULL;
        }
        if (ulsch->harq_processes[i]->b) {
          free16(ulsch->harq_processes[i]->b,a_segments*1056);
          ulsch->harq_processes[i]->b = NULL;
        }
        if (ulsch->harq_processes[i]->e) {
          free16(ulsch->harq_processes[i]->e,14*N_RB_UL*12*8);
          ulsch->harq_processes[i]->e = NULL;
        }
        if (ulsch->harq_processes[i]->f) {
          free16(ulsch->harq_processes[i]->f,14*N_RB_UL*12*8);
          ulsch->harq_processes[i]->f = NULL;
        }
        for (int r=0; r<a_segments; r++) {
          if (ulsch->harq_processes[i]->c[r]) {
            free16(ulsch->harq_processes[i]->c[r],((r==0)?8:0) + 3+768);
            ulsch->harq_processes[i]->c[r] = NULL;
          }

          if (ulsch->harq_processes[i]->d[r]) {
            free16(ulsch->harq_processes[i]->d[r],68*384);
            ulsch->harq_processes[i]->d[r] = NULL;
          }

        }

        if (ulsch->harq_processes[i]->c) {
          free16(ulsch->harq_processes[i]->c,a_segments);
          ulsch->harq_processes[i]->c = NULL;
        }
        if (ulsch->harq_processes[i]->d) {
          free16(ulsch->harq_processes[i]->d,a_segments);
          ulsch->harq_processes[i]->d = NULL;
        }

        free16(ulsch->harq_processes[i],sizeof(NR_UL_UE_HARQ_t));
        ulsch->harq_processes[i] = NULL;
      }
    }
    free16(ulsch,sizeof(NR_UE_ULSCH_t));
    *ulschptr = NULL;
  }
}


NR_UE_ULSCH_t *new_nr_ue_ulsch(uint16_t N_RB_UL, int number_of_harq_pids, NR_DL_FRAME_PARMS* frame_parms) {

  int max_layers = (frame_parms->nb_antennas_tx<NR_MAX_NB_LAYERS) ? frame_parms->nb_antennas_tx : NR_MAX_NB_LAYERS;
  uint16_t a_segments = MAX_NUM_NR_ULSCH_SEGMENTS_PER_LAYER*max_layers;  //number of segments to be allocated

  if (N_RB_UL != 273) {
    a_segments = a_segments*N_RB_UL;
    a_segments = a_segments/273 +1;
  }  

  uint32_t ulsch_bytes = a_segments*1056;  // allocated bytes per segment

  NR_UE_ULSCH_t *ulsch = malloc16(sizeof(NR_UE_ULSCH_t));
  DevAssert(ulsch);
  memset(ulsch, 0, sizeof(*ulsch));

  ulsch->number_harq_processes_for_pusch = NR_MAX_ULSCH_HARQ_PROCESSES;

  for (int i = 0; i < number_of_harq_pids; i++) {

    ulsch->harq_processes[i] = malloc16(sizeof(NR_UL_UE_HARQ_t));
    DevAssert(ulsch->harq_processes[i]);
    memset(ulsch->harq_processes[i], 0, sizeof(NR_UL_UE_HARQ_t));

    ulsch->harq_processes[i]->a = malloc16(ulsch_bytes);
    DevAssert(ulsch->harq_processes[i]->a);
    bzero(ulsch->harq_processes[i]->a,ulsch_bytes);

    ulsch->harq_processes[i]->b = malloc16(ulsch_bytes);
    DevAssert(ulsch->harq_processes[i]->b);
    bzero(ulsch->harq_processes[i]->b,ulsch_bytes);

    ulsch->harq_processes[i]->c = malloc16(a_segments*sizeof(uint8_t *));
    ulsch->harq_processes[i]->d = malloc16(a_segments*sizeof(uint16_t *));
    for (int r = 0; r < a_segments; r++) {
      // account for filler in first segment and CRCs for multiple segment case
      ulsch->harq_processes[i]->c[r] = malloc16(8448);
      DevAssert(ulsch->harq_processes[i]->c[r]);
      bzero(ulsch->harq_processes[i]->c[r],8448);

      ulsch->harq_processes[i]->d[r] = malloc16(68*384); //max size for coded output
      DevAssert(ulsch->harq_processes[i]->d[r]);
      bzero(ulsch->harq_processes[i]->d[r],(68*384));
    }

    ulsch->harq_processes[i]->e = malloc16(14*N_RB_UL*12*8);
    DevAssert(ulsch->harq_processes[i]->e);
    bzero(ulsch->harq_processes[i]->e,14*N_RB_UL*12*8);

    ulsch->harq_processes[i]->f = malloc16(14*N_RB_UL*12*8);
    DevAssert(ulsch->harq_processes[i]->f);
    bzero(ulsch->harq_processes[i]->f,14*N_RB_UL*12*8);

    ulsch->harq_processes[i]->subframe_scheduling_flag = 0;
    ulsch->harq_processes[i]->first_tx = 1;
  }

  return(ulsch);
}


int nr_ulsch_encoding(PHY_VARS_NR_UE *ue,
                      NR_UE_ULSCH_t *ulsch,
                      NR_DL_FRAME_PARMS* frame_parms,
                      uint8_t harq_pid,
                      unsigned int G) {

  start_meas(&ue->ulsch_encoding_stats);

/////////////////////////parameters and variables initialization/////////////////////////
///////////

  unsigned int crc = 1;
  NR_UL_UE_HARQ_t *harq_process = ulsch->harq_processes[harq_pid];
  uint16_t nb_rb = harq_process->pusch_pdu.rb_size;
  uint32_t A = harq_process->pusch_pdu.pusch_data.tb_size<<3;
  uint32_t *pz = &harq_process->Z;
  uint8_t mod_order = harq_process->pusch_pdu.qam_mod_order;
  uint16_t Kr=0;
  uint32_t r_offset=0;
  uint32_t F=0;
  // target_code_rate is in 0.1 units
  float Coderate = (float) harq_process->pusch_pdu.target_code_rate / 10240.0f;

///////////
/////////////////////////////////////////////////////////////////////////////////////////  

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_UE_ULSCH_ENCODING, VCD_FUNCTION_IN);

  LOG_D(NR_PHY, "ulsch coding nb_rb %d, Nl = %d\n", nb_rb, harq_process->pusch_pdu.nrOfLayers);
  LOG_D(NR_PHY, "ulsch coding A %d G %d mod_order %d Coderate %f\n", A, G, mod_order, Coderate);
  LOG_D(NR_PHY, "harq_pid %d harq_process->ndi %d, pusch_data.new_data_indicator %d\n",
        harq_pid,harq_process->ndi,harq_process->pusch_pdu.pusch_data.new_data_indicator);

  if (harq_process->first_tx == 1 ||
      harq_process->ndi != harq_process->pusch_pdu.pusch_data.new_data_indicator) {  // this is a new packet
#ifdef DEBUG_ULSCH_CODING
  printf("encoding thinks this is a new packet \n");
#endif
    harq_process->first_tx = 0;
///////////////////////// a---->| add CRC |---->b /////////////////////////
///////////
   /* 
    int i;
    printf("ulsch (tx): \n");
    for (i=0;i<(A>>3);i++)
      printf("%02x.",harq_process->a[i]);
    printf("\n");
   */ 

    int max_payload_bytes = MAX_NUM_NR_ULSCH_SEGMENTS_PER_LAYER*harq_process->pusch_pdu.nrOfLayers*1056;

    if (A > 3824) {
      // Add 24-bit crc (polynomial A) to payload
      crc = crc24a(harq_process->a,A)>>8;
      harq_process->a[A>>3] = ((uint8_t*)&crc)[2];
      harq_process->a[1+(A>>3)] = ((uint8_t*)&crc)[1];
      harq_process->a[2+(A>>3)] = ((uint8_t*)&crc)[0];
      //printf("CRC %x (A %d)\n",crc,A);
      //printf("a0 %d a1 %d a2 %d\n", a[A>>3], a[1+(A>>3)], a[2+(A>>3)]);

      harq_process->B = A+24;

      AssertFatal((A/8)+4 <= max_payload_bytes,"A %d is too big (A/8+4 = %d > %d)\n",A,(A/8)+4,max_payload_bytes);

      memcpy(harq_process->b,harq_process->a,(A/8)+4);
    }
    else {
      // Add 16-bit crc (polynomial A) to payload
      crc = crc16(harq_process->a,A)>>16;
      harq_process->a[A>>3] = ((uint8_t*)&crc)[1];
      harq_process->a[1+(A>>3)] = ((uint8_t*)&crc)[0];
      //printf("CRC %x (A %d)\n",crc,A);
      //printf("a0 %d a1 %d \n", a[A>>3], a[1+(A>>3)]);

      harq_process->B = A+16;

      AssertFatal((A/8)+3 <= max_payload_bytes,"A %d is too big (A/8+3 = %d > %d)\n",A,(A/8)+3,max_payload_bytes);

      memcpy(harq_process->b,harq_process->a,(A/8)+3);  // using 3 bytes to mimic the case of 24 bit crc
    }
///////////
///////////////////////////////////////////////////////////////////////////

///////////////////////// b---->| block segmentation |---->c /////////////////////////
///////////

    if ((A <=292) || ((A<=3824) && (Coderate <= 0.6667)) || Coderate <= 0.25){
      harq_process->BG = 2;
    }
    else{
      harq_process->BG = 1;
    }

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_SEGMENTATION, VCD_FUNCTION_IN);
    start_meas(&ue->ulsch_segmentation_stats);
    uint32_t  Kb=nr_segmentation(harq_process->b,
                                 harq_process->c,
                                 harq_process->B,
                                 &harq_process->C,
                                 &harq_process->K,
                                 pz,
                                 &harq_process->F,
                                 harq_process->BG);

    if (harq_process->C>MAX_NUM_NR_DLSCH_SEGMENTS_PER_LAYER*harq_process->pusch_pdu.nrOfLayers) {
      LOG_E(PHY,"nr_segmentation.c: too many segments %d, B %d\n",harq_process->C,harq_process->B);
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

    //printf("segment Z %d k %d Kr %d BG %d\n", *pz,harq_process->K,Kr,BG);

    //start_meas(te_stats);
    for (int r=0; r<harq_process->C; r++) {
      //channel_input[r] = &harq_process->d[r][0];
#ifdef DEBUG_ULSCH_CODING
      printf("Encoder: B %d F %d \n",harq_process->B, harq_process->F);
      printf("start ldpc encoder segment %d/%d\n",r,harq_process->C);
      printf("input %d %d %d %d %d \n", harq_process->c[r][0], harq_process->c[r][1], harq_process->c[r][2],harq_process->c[r][3], harq_process->c[r][4]);
      for (int cnt =0 ; cnt < 22*(*pz)/8; cnt ++){
        printf("%d ", harq_process->c[r][cnt]);
      }
      printf("\n");

#endif
      //ldpc_encoder_orig((unsigned char*)harq_process->c[r],harq_process->d[r],Kr,BG,0);
      //ldpc_encoder_optim((unsigned char*)harq_process->c[r],(unsigned char*)&harq_process->d[r][0],Kr,BG,NULL,NULL,NULL,NULL);
    }

    //for (int i=0;i<68*384;i++)
      //        printf("channel_input[%d]=%d\n",i,channel_input[i]);

    /*printf("output %d %d %d %d %d \n", harq_process->d[0][0], harq_process->d[0][1], harq_process->d[r][2],harq_process->d[0][3], harq_process->d[0][4]);
      for (int cnt =0 ; cnt < 66*(*pz); cnt ++){
      printf("%d \n",  harq_process->d[0][cnt]);
      }
      printf("\n");*/
    encoder_implemparams_t impp = {
      .n_segments=harq_process->C,
      .macro_num=0,
      .tinput  = NULL,
      .tprep   = NULL,
      .tparity = NULL,
      .toutput = NULL};

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_LDPC_ENCODER_OPTIM, VCD_FUNCTION_IN);

    start_meas(&ue->ulsch_ldpc_encoding_stats);
    for(int j = 0; j < (harq_process->C/8 + 1); j++) {
      impp.macro_num = j;
      nrLDPC_encoder(harq_process->c,harq_process->d,*pz,Kb,Kr,harq_process->BG,&impp);
    }
    stop_meas(&ue->ulsch_ldpc_encoding_stats);

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_LDPC_ENCODER_OPTIM, VCD_FUNCTION_OUT);

    //stop_meas(te_stats);
    //printf("end ldpc encoder -- output\n");
#ifdef DEBUG_ULSCH_CODING
    write_output("ulsch_enc_input0.m","enc_in0",&harq_process->c[0][0],Kr_bytes,1,4);
    write_output("ulsch_enc_output0.m","enc0",&harq_process->d[0][0],(3*8*Kr_bytes)+12,1,4);
#endif

///////////
///////////////////////////////////////////////////////////////////////////////
    LOG_D(PHY,"setting ndi to %d from pusch_data\n", harq_process->pusch_pdu.pusch_data.new_data_indicator);
    harq_process->ndi = harq_process->pusch_pdu.pusch_data.new_data_indicator;
  }
  F = harq_process->F;
  Kr = harq_process->K;

  for (int r=0; r<harq_process->C; r++) { // looping over C segments
    if (harq_process->F>0) {
      for (int k=(Kr-F-2*(*pz)); k<Kr-2*(*pz); k++) {
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
	  harq_process->pusch_pdu.pusch_data.rv_index);

    //start_meas(rm_stats);
///////////////////////// d---->| Rate matching bit selection |---->e /////////////////////////
///////////

    uint32_t E = nr_get_E(G, harq_process->C, mod_order, harq_process->pusch_pdu.nrOfLayers, r);

    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_RATE_MATCHING_LDPC, VCD_FUNCTION_IN);
    start_meas(&ue->ulsch_rate_matching_stats);
    if (nr_rate_matching_ldpc(0,
                              harq_process->BG,
                              *pz,
                              harq_process->d[r],
                              harq_process->e+r_offset,
                              harq_process->C,
                              F,
                              Kr-F-2*(*pz),
                              harq_process->pusch_pdu.pusch_data.rv_index,
                              E) == -1)
      return -1;

    stop_meas(&ue->ulsch_rate_matching_stats);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_NR_RATE_MATCHING_LDPC, VCD_FUNCTION_OUT);



#ifdef DEBUG_ULSCH_CODING
    for (int i =0; i<16; i++)
      printf("output ratematching e[%d]= %d r_offset %d\n", i,harq_process->e[i+r_offset], r_offset);
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
      printf("output interleaving f[%d]= %d r_offset %d\n", i,harq_process->f[i+r_offset], r_offset);

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
