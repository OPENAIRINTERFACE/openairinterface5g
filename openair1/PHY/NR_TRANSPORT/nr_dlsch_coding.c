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

/*! \file PHY/LTE_TRANSPORT/dlsch_coding.c
* \brief Top-level routines for implementing LDPC-coded (DLSCH) transport channels from 38-212, 15.2
* \author H.Wang
* \date 2018
* \version 0.1
* \company Eurecom
* \email:
* \note
* \warning
*/

#include "PHY/defs_gNB.h"
#include "PHY/phy_extern.h"
#include "PHY/CODING/coding_extern.h"
#include "PHY/CODING/coding_defs.h"
#include "PHY/CODING/lte_interleaver_inline.h"
#include "PHY/CODING/nrLDPC_extern.h"
#include "PHY/NR_TRANSPORT/nr_transport.h"
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
#include "PHY/NR_TRANSPORT/nr_dlsch.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include "SCHED_NR/sched_nr.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "common/utils/LOG/log.h"
#include <syscall.h>

//#define DEBUG_DLSCH_CODING
//#define DEBUG_DLSCH_FREE 1


void free_gNB_dlsch(NR_gNB_DLSCH_t **dlschptr,uint16_t N_RB)
{
  int i;
  int r;

  NR_gNB_DLSCH_t *dlsch = *dlschptr;

  uint16_t a_segments = MAX_NUM_NR_DLSCH_SEGMENTS;  //number of segments to be allocated
  if (dlsch) {

    if (N_RB != 273) {
      a_segments = a_segments*N_RB;
      a_segments = a_segments/273;
    }  



#ifdef DEBUG_DLSCH_FREE
    LOG_D(PHY,"Freeing dlsch %p\n",dlsch);
#endif

    for (i=0; i<dlsch->Mdlharq; i++) {
#ifdef DEBUG_DLSCH_FREE
      LOG_D(PHY,"Freeing dlsch process %d\n",i);
#endif

      if (dlsch->harq_processes[i]) {
#ifdef DEBUG_DLSCH_FREE
        LOG_D(PHY,"Freeing dlsch process %d (%p)\n",i,dlsch->harq_processes[i]);
#endif

        if (dlsch->harq_processes[i]->b) {
          free16(dlsch->harq_processes[i]->b,a_segments*1056);
          dlsch->harq_processes[i]->b = NULL;
#ifdef DEBUG_DLSCH_FREE
          LOG_D(PHY,"Freeing dlsch process %d b (%p)\n",i,dlsch->harq_processes[i]->b);
#endif
        }

        if (dlsch->harq_processes[i]->e) {
          free16(dlsch->harq_processes[i]->e,14*N_RB*12*8);
          dlsch->harq_processes[i]->e = NULL;
#ifdef DEBUG_DLSCH_FREE
          printf("Freeing dlsch process %d e (%p)\n",i,dlsch->harq_processes[i]->e);
#endif
        }

        if (dlsch->harq_processes[i]->f) {
          free16(dlsch->harq_processes[i]->f,14*N_RB*12*8);
          dlsch->harq_processes[i]->f = NULL;
#ifdef DEBUG_DLSCH_FREE
          printf("Freeing dlsch process %d f (%p)\n",i,dlsch->harq_processes[i]->f);
#endif
        }

#ifdef DEBUG_DLSCH_FREE
        LOG_D(PHY,"Freeing dlsch process %d c (%p)\n",i,dlsch->harq_processes[i]->c);
#endif

        for (r=0; r<a_segments; r++) {

#ifdef DEBUG_DLSCH_FREE
          LOG_D(PHY,"Freeing dlsch process %d c[%d] (%p)\n",i,r,dlsch->harq_processes[i]->c[r]);
#endif

          if (dlsch->harq_processes[i]->c[r]) {
            free16(dlsch->harq_processes[i]->c[r],1056);
            dlsch->harq_processes[i]->c[r] = NULL;
          }
          if (dlsch->harq_processes[i]->d[r]) {
            free16(dlsch->harq_processes[i]->d[r],3*8448);
            dlsch->harq_processes[i]->d[r] = NULL;
          }

	    }
	free16(dlsch->harq_processes[i],sizeof(NR_DL_gNB_HARQ_t));
	dlsch->harq_processes[i] = NULL;
      }
    }

    free16(dlsch,sizeof(NR_gNB_DLSCH_t));
    dlsch = NULL;
  }

}

NR_gNB_DLSCH_t *new_gNB_dlsch(NR_DL_FRAME_PARMS *frame_parms,
                              unsigned char Kmimo,
                              unsigned char Mdlharq,
                              uint32_t Nsoft,
                              uint8_t  abstraction_flag,
                              uint16_t N_RB)
{

  NR_gNB_DLSCH_t *dlsch;
  unsigned char exit_flag = 0,i,r,aa,layer;
  int re;
  uint16_t a_segments = MAX_NUM_NR_DLSCH_SEGMENTS;  //number of segments to be allocated

  if (N_RB != 273) {
    a_segments = a_segments*N_RB;
    a_segments = a_segments/273;
  }  

  uint16_t dlsch_bytes = a_segments*1056;  // allocated bytes per segment

  
  dlsch = (NR_gNB_DLSCH_t *)malloc16(sizeof(NR_gNB_DLSCH_t));

  if (dlsch) {
    bzero(dlsch,sizeof(NR_gNB_DLSCH_t));
    dlsch->Kmimo = Kmimo;
    dlsch->Mdlharq = Mdlharq;
    dlsch->Mlimit = 4;
    dlsch->Nsoft = Nsoft;

    for (layer=0; layer<NR_MAX_NB_LAYERS; layer++) {
      dlsch->ue_spec_bf_weights[layer] = (int32_t**)malloc16(64*sizeof(int32_t*));

      for (aa=0; aa<64; aa++) {
         dlsch->ue_spec_bf_weights[layer][aa] = (int32_t *)malloc16(OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES*sizeof(int32_t));
         for (re=0;re<OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES; re++) {
           dlsch->ue_spec_bf_weights[layer][aa][re] = 0x00007fff;
         }
      }

      dlsch->txdataF[layer] = (int32_t *)malloc16((NR_MAX_PDSCH_ENCODED_LENGTH/NR_MAX_NB_LAYERS)*sizeof(int32_t)); // NR_MAX_NB_LAYERS is already included in NR_MAX_PDSCH_ENCODED_LENGTH
    }

    for (int q=0; q<NR_MAX_NB_CODEWORDS; q++)
      dlsch->mod_symbs[q] = (int32_t *)malloc16(NR_MAX_PDSCH_ENCODED_LENGTH*sizeof(int32_t));

     dlsch->calib_dl_ch_estimates = (int32_t**)malloc16(64*sizeof(int32_t*));
     for (aa=0; aa<64; aa++) {
       dlsch->calib_dl_ch_estimates[aa] = (int32_t *)malloc16(OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES*sizeof(int32_t));

     }

    for (i=0; i<20; i++) {
      dlsch->harq_ids[0][i] = 0;
      dlsch->harq_ids[1][i] = 0;
    }

    for (i=0; i<Mdlharq; i++) {
      dlsch->harq_processes[i] = (NR_DL_gNB_HARQ_t *)malloc16(sizeof(NR_DL_gNB_HARQ_t));
      LOG_T(PHY, "Required mem size %d  dlsch->harq_processes[%d] %p\n",
    		  dlsch_bytes, i,dlsch->harq_processes[i]);

      if (dlsch->harq_processes[i]) {
        bzero(dlsch->harq_processes[i],sizeof(NR_DL_gNB_HARQ_t));
        //    dlsch->harq_processes[i]->first_tx=1;
        dlsch->harq_processes[i]->b = (unsigned char*)malloc16(dlsch_bytes);
        dlsch->harq_processes[i]->pdu = (uint8_t*)malloc16(dlsch_bytes);
        if (dlsch->harq_processes[i]->pdu) {
          bzero(dlsch->harq_processes[i]->pdu,dlsch_bytes);
          nr_emulate_dlsch_payload(dlsch->harq_processes[i]->pdu, (dlsch_bytes)>>3);
        } else {
          LOG_D(PHY,"Can't allocate PDU\n");
          exit_flag=1;
        }

        if (dlsch->harq_processes[i]->b) {
          bzero(dlsch->harq_processes[i]->b,dlsch_bytes);
        } else {
          LOG_D(PHY,"Can't get b\n");
          exit_flag=1;
        }

        if (abstraction_flag==0) {
          for (r=0; r<a_segments; r++) {
            // account for filler in first segment and CRCs for multiple segment case
            // [hna] 8448 is the maximum CB size in NR
            //       68*348 = 68*(maximum size of Zc)
            //       In section 5.3.2 in 38.212, the for loop is up to N + 2*Zc (maximum size of N is 66*Zc, therefore 68*Zc)
            dlsch->harq_processes[i]->c[r] = (uint8_t*)malloc16(8448);
            dlsch->harq_processes[i]->d[r] = (uint8_t*)malloc16(68*384); //max size for coded output
            if (dlsch->harq_processes[i]->c[r]) {
              bzero(dlsch->harq_processes[i]->c[r],8448);
            } else {
              LOG_D(PHY,"Can't get c\n");
              exit_flag=2;
            }
            if (dlsch->harq_processes[i]->d[r]) {
              bzero(dlsch->harq_processes[i]->d[r],(3*8448));
            } else {
              LOG_D(PHY,"Can't get d\n");
              exit_flag=2;
            }
          }
          dlsch->harq_processes[i]->e = (uint8_t*)malloc16(14*N_RB*12*8);
          if (dlsch->harq_processes[i]->e) {
            bzero(dlsch->harq_processes[i]->e,14*N_RB*12*8);
          } else {
            printf("Can't get e\n");
            exit_flag=1;
          }
          dlsch->harq_processes[i]->f = (uint8_t*)malloc16(14*N_RB*12*8);
          if (dlsch->harq_processes[i]->f) {
            bzero(dlsch->harq_processes[i]->f,14*N_RB*12*8);
          } else {
            printf("Can't get f\n");
            exit_flag=1;
          }
        }
      } else {
        LOG_D(PHY,"Can't get harq_p %d\n",i);
        exit_flag=3;
      }
    }

    if (exit_flag==0) {
      for (i=0; i<Mdlharq; i++) {
        dlsch->harq_processes[i]->round=0;
      }

      return(dlsch);
    }
  }

  LOG_D(PHY,"new_gNB_dlsch exit flag %d, size of  %ld\n",
	exit_flag, sizeof(NR_gNB_DLSCH_t));

  free_gNB_dlsch(&dlsch,N_RB);

  return(NULL);


}

void clean_gNB_dlsch(NR_gNB_DLSCH_t *dlsch)
{

  unsigned char Mdlharq;
  unsigned char i,j,r;

  if (dlsch) {
    Mdlharq = dlsch->Mdlharq;
    dlsch->rnti = 0;
    dlsch->active = 0;

    for (i=0; i<10; i++) {
      dlsch->harq_ids[0][i] = Mdlharq;
      dlsch->harq_ids[1][i] = Mdlharq;
    }
    for (i=0; i<Mdlharq; i++) {
      if (dlsch->harq_processes[i]) {
        //  dlsch->harq_processes[i]->Ndi    = 0;
        //dlsch->harq_processes[i]->status = 0;
        dlsch->harq_processes[i]->round  = 0;

	for (j=0; j<96; j++)
	  for (r=0; r<MAX_NUM_NR_DLSCH_SEGMENTS; r++)
	    if (dlsch->harq_processes[i]->d[r])
	      dlsch->harq_processes[i]->d[r][j] = NR_NULL;

      }
    }
  }
}

int nr_dlsch_encoding(unsigned char *a,
                      int frame,
                      uint8_t slot,
                      NR_gNB_DLSCH_t *dlsch,
                      NR_DL_FRAME_PARMS* frame_parms,
		      time_stats_t *tinput,time_stats_t *tprep,time_stats_t *tparity,time_stats_t *toutput,
		      time_stats_t *dlsch_rate_matching_stats,time_stats_t *dlsch_interleaving_stats,
		      time_stats_t *dlsch_segmentation_stats)
{

  unsigned int G;
  unsigned int crc=1;
  uint8_t harq_pid = dlsch->harq_ids[frame%2][slot];
  AssertFatal(harq_pid<8 && harq_pid>=0,"illegal harq_pid %d\b",harq_pid);
  nfapi_nr_dl_tti_pdsch_pdu_rel15_t *rel15 = &dlsch->harq_processes[harq_pid]->pdsch_pdu.pdsch_pdu_rel15;
  uint16_t nb_rb = rel15->rbSize;
  uint8_t nb_symb_sch = rel15->NrOfSymbols;
  uint32_t A, Z, Kb, F=0;
  uint32_t *Zc = &Z;
  uint8_t mod_order = rel15->qamModOrder[0];
  uint16_t Kr=0,r;
  uint32_t r_offset=0;
  uint8_t BG=1;
  uint32_t E;
  uint8_t Ilbrm = 1;
  uint32_t Tbslbrm = 950984; //max tbs
  uint8_t nb_re_dmrs = rel15->dmrsConfigType==NFAPI_NR_DMRS_TYPE1 ? 6:4;
  uint16_t length_dmrs = get_num_dmrs(rel15->dlDmrsSymbPos);
  uint16_t R=rel15->targetCodeRate[0];
  float Coderate = 0.0;
  uint8_t Nl = 4;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ENCODING, VCD_FUNCTION_IN);

  A = rel15->TBSize[0]<<3;

  G = nr_get_G(nb_rb, nb_symb_sch, nb_re_dmrs, length_dmrs,mod_order,rel15->nrOfLayers);

  LOG_D(PHY,"dlsch coding A %d G %d mod_order %d\n", A,G, mod_order);

  //  if (dlsch->harq_processes[harq_pid]->Ndi == 1) {  // this is a new packet
  if (dlsch->harq_processes[harq_pid]->round == 0) {  // this is a new packet
#ifdef DEBUG_DLSCH_CODING
  LOG_D(PHY,"encoding thinks this is a new packet \n");
#endif
  /*    
    int i;
    LOG_D(PHY,"dlsch (tx): \n");
    for (i=0;i<(A>>3);i++)
      LOG_D(PHY,"%02x\n",a[i]);
    LOG_D(PHY,"\n");
  */

    if (A > 3824) {
      // Add 24-bit crc (polynomial A) to payload
      crc = crc24a(a,A)>>8;
      a[A>>3] = ((uint8_t*)&crc)[2];
      a[1+(A>>3)] = ((uint8_t*)&crc)[1];
      a[2+(A>>3)] = ((uint8_t*)&crc)[0];
      //printf("CRC %x (A %d)\n",crc,A);
      //printf("a0 %d a1 %d a2 %d\n", a[A>>3], a[1+(A>>3)], a[2+(A>>3)]);
  
      dlsch->harq_processes[harq_pid]->B = A+24;
      //    dlsch->harq_processes[harq_pid]->b = a;
   
      AssertFatal((A/8)+4 <= MAX_NR_DLSCH_PAYLOAD_BYTES,"A %d is too big (A/8+4 = %d > %d)\n",A,(A/8)+4,MAX_NR_DLSCH_PAYLOAD_BYTES);

      memcpy(dlsch->harq_processes[harq_pid]->b,a,(A/8)+4);  // why is this +4 if the CRC is only 3 bytes?
    }
    else {
      // Add 16-bit crc (polynomial A) to payload
      crc = crc16(a,A)>>16;
      a[A>>3] = ((uint8_t*)&crc)[1];
      a[1+(A>>3)] = ((uint8_t*)&crc)[0];
      //printf("CRC %x (A %d)\n",crc,A);
      //printf("a0 %d a1 %d \n", a[A>>3], a[1+(A>>3)]);
  
      dlsch->harq_processes[harq_pid]->B = A+16;
      //    dlsch->harq_processes[harq_pid]->b = a;
   
      AssertFatal((A/8)+3 <= MAX_NR_DLSCH_PAYLOAD_BYTES,"A %d is too big (A/8+3 = %d > %d)\n",A,(A/8)+3,MAX_NR_DLSCH_PAYLOAD_BYTES);

      memcpy(dlsch->harq_processes[harq_pid]->b,a,(A/8)+3);  // using 3 bytes to mimic the case of 24 bit crc
    }
    if (R<1000)
      Coderate = (float) R /(float) 1024;
    else  // to scale for mcs 20 and 26 in table 5.1.3.1-2 which are decimal and input 2* in nr_tbs_tools
      Coderate = (float) R /(float) 2048;

    if ((A <=292) || ((A<=3824) && (Coderate <= 0.6667)) || Coderate <= 0.25)
		BG = 2;
    else
		BG = 1;

    start_meas(dlsch_segmentation_stats);
    Kb = nr_segmentation(dlsch->harq_processes[harq_pid]->b,
		         dlsch->harq_processes[harq_pid]->c,
		         dlsch->harq_processes[harq_pid]->B,
		         &dlsch->harq_processes[harq_pid]->C,
		         &dlsch->harq_processes[harq_pid]->K,
		         Zc, 
		         &dlsch->harq_processes[harq_pid]->F,
                         BG);
    stop_meas(dlsch_segmentation_stats);
    F = dlsch->harq_processes[harq_pid]->F;

    Kr = dlsch->harq_processes[harq_pid]->K;
#ifdef DEBUG_DLSCH_CODING
    uint16_t Kr_bytes;
    Kr_bytes = Kr>>3;
#endif

    //printf("segment Z %d k %d Kr %d BG %d C %d\n", *Zc,dlsch->harq_processes[harq_pid]->K,Kr,BG,dlsch->harq_processes[harq_pid]->C);

    for (r=0; r<dlsch->harq_processes[harq_pid]->C; r++) {
      //d_tmp[r] = &dlsch->harq_processes[harq_pid]->d[r][0];
      //channel_input[r] = &dlsch->harq_processes[harq_pid]->d[r][0];
#ifdef DEBUG_DLSCH_CODING
      LOG_D(PHY,"Encoder: B %d F %d \n",dlsch->harq_processes[harq_pid]->B, dlsch->harq_processes[harq_pid]->F);
      LOG_D(PHY,"start ldpc encoder segment %d/%d\n",r,dlsch->harq_processes[harq_pid]->C);
      LOG_D(PHY,"input %d %d %d %d %d \n", dlsch->harq_processes[harq_pid]->c[r][0], dlsch->harq_processes[harq_pid]->c[r][1], dlsch->harq_processes[harq_pid]->c[r][2],dlsch->harq_processes[harq_pid]->c[r][3], dlsch->harq_processes[harq_pid]->c[r][4]);
      for (int cnt =0 ; cnt < 22*(*Zc)/8; cnt ++){
      LOG_D(PHY,"%d ", dlsch->harq_processes[harq_pid]->c[r][cnt]);
      }
      LOG_D(PHY,"\n");

#endif
      //ldpc_encoder_orig((unsigned char*)dlsch->harq_processes[harq_pid]->c[r],dlsch->harq_processes[harq_pid]->d[r],*Zc,Kb,Kr,BG,0);
      //ldpc_encoder_optim((unsigned char*)dlsch->harq_processes[harq_pid]->c[r],(unsigned char*)&dlsch->harq_processes[harq_pid]->d[r][0],*Zc,Kb,Kr,BG,NULL,NULL,NULL,NULL);
    }
    encoder_implemparams_t impp;
    impp.n_segments=dlsch->harq_processes[harq_pid]->C;
    impp.tprep = tprep;
    impp.tinput = tinput;
    impp.tparity = tparity;
    impp.toutput = toutput;

    for(int j=0;j<(dlsch->harq_processes[harq_pid]->C/8+1);j++) {
      impp.macro_num=j;
      nrLDPC_encoder(dlsch->harq_processes[harq_pid]->c,dlsch->harq_processes[harq_pid]->d,*Zc,Kb,Kr,BG,&impp);
    }


#ifdef DEBUG_DLSCH_CODING
      write_output("enc_input0.m","enc_in0",&dlsch->harq_processes[harq_pid]->c[0][0],Kr_bytes,1,4);
      write_output("enc_output0.m","enc0",&dlsch->harq_processes[harq_pid]->d[0][0],(3*8*Kr_bytes)+12,1,4);
#endif

  }

  for (r=0; r<dlsch->harq_processes[harq_pid]->C; r++) {

    if (F>0) {
      for (int k=(Kr-F-2*(*Zc)); k<Kr-2*(*Zc); k++) {
	// writing into positions d[r][k-2Zc] as in clause 5.3.2 step 2) in 38.212
        dlsch->harq_processes[harq_pid]->d[r][k] = NR_NULL;
	//if (k<(Kr-F+8))
	//printf("r %d filler bits [%d] = %d \n", r,k, dlsch->harq_processes[harq_pid]->d[r][k]);
      }
    }



#ifdef DEBUG_DLSCH_CODING
  LOG_D(PHY,"rvidx in encoding = %d\n", rel15->rvIndex[0]);
#endif

    E = nr_get_E(G, dlsch->harq_processes[harq_pid]->C, mod_order, rel15->nrOfLayers, r);

    //#ifdef DEBUG_DLSCH_CODING
    LOG_D(PHY,"Rate Matching, Code segment %d/%d (coded bits (G) %u, E %d, Filler bits %d, Filler offset %d mod_order %d, nb_rb %d)...\n",
	  r,
	  dlsch->harq_processes[harq_pid]->C,
	  G,
	  E,
	  F,
	  Kr-F-2*(*Zc),
	  mod_order,nb_rb);

    // for tbslbrm calculation according to 5.4.2.1 of 38.212
    if (rel15->nrOfLayers < Nl)
      Nl = rel15->nrOfLayers;

    Tbslbrm = nr_compute_tbslbrm(rel15->mcsTable[0],nb_rb,Nl,dlsch->harq_processes[harq_pid]->C);

    start_meas(dlsch_rate_matching_stats);
    nr_rate_matching_ldpc(Ilbrm,
                          Tbslbrm,
                          BG,
                          *Zc,
                          dlsch->harq_processes[harq_pid]->d[r],
                          dlsch->harq_processes[harq_pid]->e+r_offset,
                          dlsch->harq_processes[harq_pid]->C,
                          F,
                          Kr-F-2*(*Zc),
                          rel15->rvIndex[0],
                          E);
    stop_meas(dlsch_rate_matching_stats);
#ifdef DEBUG_DLSCH_CODING
    for (int i =0; i<16; i++)
      printf("output ratematching e[%d]= %d r_offset %u\n", i,dlsch->harq_processes[harq_pid]->e[i+r_offset], r_offset);
#endif

    start_meas(dlsch_interleaving_stats);
    nr_interleaving_ldpc(E,
			 mod_order,
			 dlsch->harq_processes[harq_pid]->e+r_offset,
			 dlsch->harq_processes[harq_pid]->f+r_offset);
    stop_meas(dlsch_interleaving_stats);

#ifdef DEBUG_DLSCH_CODING
    for (int i =0; i<16; i++)
      printf("output interleaving f[%d]= %d r_offset %u\n", i,dlsch->harq_processes[harq_pid]->f[i+r_offset], r_offset);

    if (r==dlsch->harq_processes[harq_pid]->C-1)
      write_output("enc_output.m","enc",dlsch->harq_processes[harq_pid]->f,G,1,4);
#endif

    r_offset += E;
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ENCODING, VCD_FUNCTION_OUT);

  return 0;
}
