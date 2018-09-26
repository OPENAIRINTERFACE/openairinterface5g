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

/*! \file PHY/NR_UE_TRANSPORT/nr_dlsch_decoding.c
* \brief Top-level routines for decoding  Turbo-coded (DLSCH) transport channels from 36-212, V8.6 2009-03
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr
* \note
* \warning
*/

#include "PHY/defs_nr_UE.h"
#include "PHY/phy_extern_nr_ue.h"
#include "PHY/CODING/coding_extern.h"
#include "PHY/CODING/coding_defs.h"
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
//#include "SCHED/extern.h"
#include "SIMULATION/TOOLS/sim.h"
#include "targets/RT/USER/nr-uesoftmodem.h"
#include "PHY/CODING/nrLDPC_decoder/nrLDPC_decoder.h"
#include "PHY/CODING/nrLDPC_decoder/nrLDPC_types.h"
//#define DEBUG_DLSCH_DECODING

#define OAI_LDPC_MAX_NUM_LLR 27000//26112 // NR_LDPC_NCOL_BG1*NR_LDPC_ZMAX

static int8_t llrProcBuf[OAI_LDPC_MAX_NUM_LLR] __attribute__ ((aligned(32)));

static uint64_t nb_total_decod =0;
static uint64_t nb_error_decod =0;

//extern double cpuf;

void free_nr_ue_dlsch(NR_UE_DLSCH_t *dlsch)
{

  int i,r;

  if (dlsch) {
    for (i=0; i<dlsch->Mdlharq; i++) {
      if (dlsch->harq_processes[i]) {
        if (dlsch->harq_processes[i]->b) {
          free16(dlsch->harq_processes[i]->b,MAX_DLSCH_PAYLOAD_BYTES);
          dlsch->harq_processes[i]->b = NULL;
        }

        for (r=0; r<MAX_NUM_NR_DLSCH_SEGMENTS; r++) {
          free16(dlsch->harq_processes[i]->c[r],((r==0)?8:0) + 3+1056);
          dlsch->harq_processes[i]->c[r] = NULL;
        }

        for (r=0; r<MAX_NUM_NR_DLSCH_SEGMENTS; r++)
          if (dlsch->harq_processes[i]->d[r]) {
            free16(dlsch->harq_processes[i]->d[r],((3*8*8448)+12+96)*sizeof(short));
            dlsch->harq_processes[i]->d[r] = NULL;
          }

        free16(dlsch->harq_processes[i],sizeof(NR_DL_UE_HARQ_t));
        dlsch->harq_processes[i] = NULL;
      }
    }

    free16(dlsch,sizeof(NR_UE_DLSCH_t));
    dlsch = NULL;
  }
}

NR_UE_DLSCH_t *new_nr_ue_dlsch(uint8_t Kmimo,uint8_t Mdlharq,uint32_t Nsoft,uint8_t max_ldpc_iterations,uint8_t N_RB_DL, uint8_t abstraction_flag)
{

  NR_UE_DLSCH_t *dlsch;
  uint8_t exit_flag = 0,i,r;

  unsigned char bw_scaling =1;

  switch (N_RB_DL) {
  case 6:
    bw_scaling =16;
    break;

  case 25:
    bw_scaling =4;
    break;

  case 50:
    bw_scaling =2;
    break;

  default:
    bw_scaling =1;
    break;
  }

  dlsch = (NR_UE_DLSCH_t *)malloc16(sizeof(NR_UE_DLSCH_t));

  if (dlsch) {
    memset(dlsch,0,sizeof(NR_UE_DLSCH_t));
    dlsch->Kmimo = Kmimo;
    dlsch->Mdlharq = Mdlharq;
    dlsch->Nsoft = Nsoft;
    dlsch->max_ldpc_iterations = max_ldpc_iterations;

    for (i=0; i<Mdlharq; i++) {
      //      printf("new_ue_dlsch: Harq process %d\n",i);
      dlsch->harq_processes[i] = (NR_DL_UE_HARQ_t *)malloc16(sizeof(NR_DL_UE_HARQ_t));

      if (dlsch->harq_processes[i]) {
        memset(dlsch->harq_processes[i],0,sizeof(NR_DL_UE_HARQ_t));
        dlsch->harq_processes[i]->first_tx=1;
        dlsch->harq_processes[i]->b = (uint8_t*)malloc16(MAX_DLSCH_PAYLOAD_BYTES/bw_scaling);

        if (dlsch->harq_processes[i]->b)
          memset(dlsch->harq_processes[i]->b,0,MAX_DLSCH_PAYLOAD_BYTES/bw_scaling);
        else
          exit_flag=3;

        if (abstraction_flag == 0) {
          for (r=0; r<MAX_NUM_DLSCH_SEGMENTS/bw_scaling; r++) {
            dlsch->harq_processes[i]->c[r] = (uint8_t*)malloc16(1056);

            if (dlsch->harq_processes[i]->c[r])
              memset(dlsch->harq_processes[i]->c[r],0,1056);
            else
              exit_flag=2;

            dlsch->harq_processes[i]->d[r] = (short*)malloc16((3*8448)*sizeof(short));

            if (dlsch->harq_processes[i]->d[r])
              memset(dlsch->harq_processes[i]->d[r],0,(3*8448)*sizeof(short));
            else
              exit_flag=2;
          }
        }
      } else {
        exit_flag=1;
      }
    }

    if (exit_flag==0)
      return(dlsch);
  }

  printf("new_ue_dlsch with size %zu: exit_flag = %u\n",sizeof(NR_DL_UE_HARQ_t), exit_flag);
  free_nr_ue_dlsch(dlsch);

  return(NULL);
}

uint32_t  nr_dlsch_decoding(PHY_VARS_NR_UE *phy_vars_ue,
                         short *dlsch_llr,
                         NR_DL_FRAME_PARMS *frame_parms,
                         NR_UE_DLSCH_t *dlsch,
                         NR_DL_UE_HARQ_t *harq_process,
                         uint32_t frame,
                         uint8_t nr_tti_rx,
                         uint8_t harq_pid,
                         uint8_t is_crnti,
                         uint8_t llr8_flag)
{

#if UE_TIMING_TRACE
  time_stats_t *dlsch_rate_unmatching_stats=&phy_vars_ue->dlsch_rate_unmatching_stats;
  time_stats_t *dlsch_turbo_decoding_stats=&phy_vars_ue->dlsch_turbo_decoding_stats;
  time_stats_t *dlsch_deinterleaving_stats=&phy_vars_ue->dlsch_deinterleaving_stats;
#endif
  uint32_t A,E;
  uint32_t G;
  uint32_t ret,offset;
  int32_t no_iteration_ldpc;
  //short dummy_w[MAX_NUM_DLSCH_SEGMENTS][3*(8448+64)];
  uint32_t r,r_offset=0,Kr,Kr_bytes,K_bytes_F,err_flag=0;
  uint8_t crc_type;
  t_nrLDPC_dec_params decParams;
  t_nrLDPC_dec_params* p_decParams = &decParams;
  t_nrLDPC_time_stats procTime;
  t_nrLDPC_time_stats* p_procTime =&procTime ;
  int16_t z [68*384];
  int8_t l [68*384];
  //__m128i l;
  int16_t inv_d [68*384];
//  int16_t *p_invd =&inv_d;
  uint8_t kb, kc;
  uint8_t Ilbrm = 0;
  uint32_t Tbslbrm = 950984; //to compute tbs
  uint16_t nb_prb = 106; //to update

  uint32_t i,j;
//  uint32_t k;

  __m128i *pv = (__m128i*)&z;
  __m128i *pl = (__m128i*)&l;

   if (!dlsch_llr) {
    printf("dlsch_decoding.c: NULL dlsch_llr pointer\n");
    return(dlsch->max_ldpc_iterations);
  }

  if (!harq_process) {
    printf("dlsch_decoding.c: NULL harq_process pointer\n");
    return(dlsch->max_ldpc_iterations);
  }

  if (!frame_parms) {
    printf("dlsch_decoding.c: NULL frame_parms pointer\n");
    return(dlsch->max_ldpc_iterations);
  }

  if (nr_tti_rx> (10*frame_parms->ttis_per_subframe-1)) {
    printf("dlsch_decoding.c: Illegal subframe index %d\n",nr_tti_rx);
    return(dlsch->max_ldpc_iterations);
  }

  if (harq_process->harq_ack.ack != 2) {
    LOG_D(PHY, "[UE %d] DLSCH @ SF%d : ACK bit is %d instead of DTX even before PDSCH is decoded!\n",
        phy_vars_ue->Mod_id, nr_tti_rx, harq_process->harq_ack.ack);
  }

  //  nb_rb = dlsch->nb_rb;

  /*
  if (nb_rb > frame_parms->N_RB_DL) {
    printf("dlsch_decoding.c: Illegal nb_rb %d\n",nb_rb);
    return(max_ldpc_iterations);
    }*/

  /*harq_pid = dlsch->current_harq_pid[phy_vars_ue->current_thread_id[subframe]];
  if (harq_pid >= 8) {
    printf("dlsch_decoding.c: Illegal harq_pid %d\n",harq_pid);
    return(max_ldpc_iterations);
  }
  */

  harq_process->trials[harq_process->round]++;

  A = harq_process->TBS;
  ret = dlsch->max_ldpc_iterations;


  G = harq_process->G;
  //get_G(frame_parms,nb_rb,dlsch->rb_alloc,mod_order,num_pdcch_symbols,phy_vars_ue->frame,subframe);

  //  printf("DLSCH Decoding, harq_pid %d Ndi %d\n",harq_pid,harq_process->Ndi);

  if (harq_process->round == 0) {
    // This is a new packet, so compute quantities regarding segmentation
    harq_process->B = A+24;
    nr_segmentation(NULL,
    	                    NULL,
    	                    harq_process->B,
    	                    &harq_process->C,
    	                    &harq_process->K,
     						&harq_process->Z,
    	                    &harq_process->F);
    						p_decParams->Z = harq_process->Z;
    	//printf("dlsch decoding nr segmentation Z %d\n", p_decParams->Z);
	//if (!frame%100)
    	//printf("K %d C %d Z %d nl %d \n", harq_process->K, harq_process->C, p_decParams->Z, harq_process->Nl);

  }

      kb = harq_process->K/harq_process->Z;
  	  if ( kb==22){
  		  p_decParams->BG = 1;
  		  p_decParams->R = 13;
  		  kc = 68;
  	  }
  	  else{
  		  p_decParams->BG = 2;
  		  p_decParams->R = 13;
  		  kc = 52;
  	  	  }

      p_decParams->numMaxIter = 2;
      Kr = p_decParams->Z*kb;
      p_decParams->outMode= 0;


  err_flag = 0;
  r_offset = 0;

  unsigned char bw_scaling =1;

  switch (frame_parms->N_RB_DL) {

  case 106:
    bw_scaling =2;
    break;

  default:
    bw_scaling =1;
    break;
  }

  if (harq_process->C > MAX_NUM_NR_DLSCH_SEGMENTS/bw_scaling) {
    LOG_E(PHY,"Illegal harq_process->C %d > %d\n",harq_process->C,MAX_NUM_NR_DLSCH_SEGMENTS/bw_scaling);
    return((1+dlsch->max_ldpc_iterations));
  }
#ifdef DEBUG_DLSCH_DECODING
  printf("Segmentation: C %d, Cminus %d, Kminus %d, K %d\n",harq_process->C,harq_process->Cminus,harq_process->Kminus,harq_process->K);
#endif

  opp_enabled=1;

  Kr = harq_process->K;
  Kr_bytes = Kr>>3;

  K_bytes_F = Kr_bytes-(harq_process->F>>3);

  Tbslbrm = nr_compute_tbs(harq_process->mcs,nb_prb,frame_parms->symbols_per_slot,0,0, harq_process->Nl);

  for (r=0; r<harq_process->C; r++) {

#if UE_TIMING_TRACE
    start_meas(dlsch_rate_unmatching_stats);
#endif

#ifdef DEBUG_DLSCH_DECODING
    LOG_D(PHY,"HARQ_PID %d Rate Matching Segment %d (coded bits %d,unpunctured/repeated bits %d, TBS %d, mod_order %d, nb_rb %d, Nl %d, rv %d, round %d)...\n",
          harq_pid,r, G,
          Kr*3,
          harq_process->TBS,
          harq_process->Qm,
          harq_process->nb_rb,
          harq_process->Nl,
          harq_process->rvidx,
          harq_process->round);
#endif

    if (nr_rate_matching_ldpc_rx(Ilbrm,
    		 	 	 	 	 	 Tbslbrm,
								 p_decParams->BG,
								 p_decParams->Z,
    							 G,
								 harq_process->w[r],
								 dlsch_llr+r_offset,
								 harq_process->C,
								 harq_process->rvidx,
								 (harq_process->round==0)?1:0,
								 harq_process->Qm,
								 harq_process->Nl,
								 r,
								 &E)==-1) {

#if UE_TIMING_TRACE
      stop_meas(dlsch_rate_unmatching_stats);
#endif
      LOG_E(PHY,"dlsch_decoding.c: Problem in rate_matching\n");
      return(dlsch->max_ldpc_iterations);
    } else
    {
#if UE_TIMING_TRACE
      stop_meas(dlsch_rate_unmatching_stats);
#endif
    }
    r_offset += E;

    /*
    printf("Subblock deinterleaving, d %p w %p\n",
     harq_process->d[r],
     harq_process->w);
    */
#if UE_TIMING_TRACE
    start_meas(dlsch_deinterleaving_stats);
#endif
    nr_deinterleaving_ldpc(E,
    					   harq_process->Qm,
                           harq_process->d[r],
                           harq_process->w[r]);

#if UE_TIMING_TRACE
    stop_meas(dlsch_deinterleaving_stats);
#endif
#ifdef DEBUG_DLSCH_DECODING

    if (r==0) {
              write_output("decoder_llr.m","decllr",dlsch_llr,G,1,0);
              write_output("decoder_in.m","dec",&harq_process->d[0][0],(3*8*Kr_bytes)+12,1,0);
    }

    printf("decoder input(segment %d) :",r);
    int i; for (i=0;i<(3*8*Kr_bytes)+12;i++)
      printf("%d : %d\n",i,harq_process->d[r][i]);
      printf("\n");
#endif


    //    printf("Clearing c, %p\n",harq_process->c[r]);
    memset(harq_process->c[r],0,Kr_bytes);

    //    printf("done\n");
    if (harq_process->C == 1)
      crc_type = CRC24_A;
    else
      crc_type = CRC24_B;

    if (err_flag == 0) {
/*
        LOG_I(PHY, "turbo algo Kr=%d cb_cnt=%d C=%d nbRB=%d crc_type %d TBSInput=%d TBSHarq=%d TBSplus24=%d mcs=%d Qm=%d RIV=%d round=%d maxIter %d\n",
                            Kr,r,harq_process->C,harq_process->nb_rb,crc_type,A,harq_process->TBS,
                            harq_process->B,harq_process->mcs,harq_process->Qm,harq_process->rvidx,harq_process->round,dlsch->max_ldpc_iterations);
*/

#if UE_TIMING_TRACE
        start_meas(dlsch_turbo_decoding_stats);
#endif

      //LOG_E(PHY,"AbsSubframe %d.%d Start turbo segment %d/%d A %d ",frame%1024,nr_tti_rx,r,harq_process->C-1, A);

      //printf("harq process dr iteration %d\n", p_decParams->numMaxIter);
      //66*p_decParams->Z

      //if (A < 1000){
      for (int cnt =0; cnt < (kc-2)*p_decParams->Z; cnt++){
            inv_d[cnt] = (-1)*harq_process->d[r][cnt];
            }
      //}

      /*for (int cnt =0; cnt < 8; cnt++){
      printf("dr %d inv_d %d \n", harq_process->d[r][96+cnt], inv_d[cnt]);
      }

      printf(" \n");*/

      /*printf("end dr \n");
      for (int cnt =(50*p_decParams->Z-16) ; cnt < 50*p_decParams->Z; cnt++){
            printf("%d ", harq_process->d[r][96+cnt]);
            }
      printf(" \n");*/

		memset(pv,0,2*harq_process->Z*sizeof(int16_t));
        //memset(pl,0,2*p_decParams->Z*sizeof(int8_t));
    	memset((pv+K_bytes_F),127,harq_process->F*sizeof(int16_t));

      	for (i=((2*p_decParams->Z)>>3), j = 0; i < K_bytes_F; i++, j++)
      	{
      		pv[i]= _mm_loadu_si128((__m128i*)(&inv_d[8*j]));
      	}

		for (i=Kr_bytes, j = K_bytes_F; i < ((kc*p_decParams->Z)>>3); i++, j++)
		      	{
		      		pv[i]= _mm_loadu_si128((__m128i*)(&inv_d[8*j]));
		      	}
      	
		for (i=0, j=0; j < ((kc*p_decParams->Z)>>4);  i+=2, j++)
      	      	{
      				
      	      		pl[j] = _mm_packs_epi16(pv[i],pv[i+1]);
      	      		
      	      	}

		no_iteration_ldpc = nrLDPC_decoder(p_decParams,
						   (int8_t*)&pl[0],
						   llrProcBuf,
						   p_procTime);
		/*
		if (check_crc(llrProcBuf,Kr,crc_type)) {
		  printf("CRC OK\n");
		  ret = 2;
		}
		else {
		  printf("CRC NOK\n");
		  ret = 1+dlsch->max_ldpc_iterations;
		}
		*/

		nb_total_decod++;
		if (no_iteration_ldpc > 10){
		  nb_error_decod++;
		  ret = 1+dlsch->max_ldpc_iterations;
		}
		else {
		  ret=2;
		}
		if (!nb_total_decod%10000){
				printf("Error number of iteration LPDC %d %ld/%ld \n", no_iteration_ldpc, nb_error_decod,nb_total_decod);fflush(stdout);
		}

		//else
			//printf("OK number of iteration LPDC %d\n", no_iteration_ldpc);

		for (int m=0; m < Kr>>3; m ++)
		      	      	{
		      				harq_process->c[r][m]= (uint8_t) llrProcBuf[m];
		      	      	}

#ifdef DEBUG_DLSCH_DECODING
      printf("output decoder %d %d %d %d %d \n", harq_process->c[r][0], harq_process->c[r][1], harq_process->c[r][2],harq_process->c[r][3], harq_process->c[r][4]);
      printf("no_iterations_ldpc %d (ret %d)\n",no_iteration_ldpc,ret);
      write_output("dec_output.m","dec0",harq_process->c[0],Kr_bytes,1,4);
#endif


#if UE_TIMING_TRACE
      stop_meas(dlsch_turbo_decoding_stats);
#endif
    }

    /*printf("Segmentation: C %d r %d, dlsch_rate_unmatching_stats %5.3f dlsch_deinterleaving_stats %5.3f  dlsch_turbo_decoding_stats %5.3f \n",
                  harq_process->C,
                  r,
                  dlsch_rate_unmatching_stats->p_time/(cpuf*1000.0),
                  dlsch_deinterleaving_stats->p_time/(cpuf*1000.0),
                  dlsch_turbo_decoding_stats->p_time/(cpuf*1000.0));*/


    if ((err_flag == 0) && (ret>=(1+dlsch->max_ldpc_iterations))) {// a Code segment is in error so break;
      LOG_W(PHY,"AbsSubframe %d.%d CRC failed, segment %d/%d \n",frame%1024,nr_tti_rx,r,harq_process->C-1);
      err_flag = 1;
    }
  }

  int32_t frame_rx_prev = frame;
  int32_t tti_rx_prev = nr_tti_rx - 1;
  if (tti_rx_prev < 0) {
    frame_rx_prev--;
    tti_rx_prev += 10*frame_parms->ttis_per_subframe;
  }
  frame_rx_prev = frame_rx_prev%1024;

  if (err_flag == 1) {
#if UE_DEBUG_TRACE
    LOG_I(PHY,"[UE %d] DLSCH: Setting NAK for SFN/SF %d/%d (pid %d, status %d, round %d, TBS %d, mcs %d) Kr %d r %d harq_process->round %d\n",
        phy_vars_ue->Mod_id, frame, nr_tti_rx, harq_pid,harq_process->status, harq_process->round,harq_process->TBS,harq_process->mcs,Kr,r,harq_process->round);
#endif
    harq_process->harq_ack.ack = 0;
    harq_process->harq_ack.harq_id = harq_pid;
    harq_process->harq_ack.send_harq_status = 1;
    harq_process->errors[harq_process->round]++;
    harq_process->round++;


    //    printf("Rate: [UE %d] DLSCH: Setting NACK for subframe %d (pid %d, round %d)\n",phy_vars_ue->Mod_id,subframe,harq_pid,harq_process->round);
    if (harq_process->round >= dlsch->Mdlharq) {
      harq_process->status = SCH_IDLE;
      harq_process->round  = 0;
    }
    if(is_crnti)
    {
    LOG_D(PHY,"[UE %d] DLSCH: Setting NACK for nr_tti_rx %d (pid %d, pid status %d, round %d/Max %d, TBS %d)\n",
               phy_vars_ue->Mod_id,nr_tti_rx,harq_pid,harq_process->status,harq_process->round,dlsch->Mdlharq,harq_process->TBS);
    }

    return((1+dlsch->max_ldpc_iterations));
  } else {
#if UE_DEBUG_TRACE
      LOG_I(PHY,"[UE %d] DLSCH: Setting ACK for nr_tti_rx %d TBS %d mcs %d nb_rb %d harq_process->round %d\n",
           phy_vars_ue->Mod_id,nr_tti_rx,harq_process->TBS,harq_process->mcs,harq_process->nb_rb, harq_process->round);
#endif

    harq_process->status = SCH_IDLE;
    harq_process->round  = 0;
    harq_process->harq_ack.ack = 1;
    harq_process->harq_ack.harq_id = harq_pid;
    harq_process->harq_ack.send_harq_status = 1;
    //LOG_I(PHY,"[UE %d] DLSCH: Setting ACK for SFN/SF %d/%d (pid %d, status %d, round %d, TBS %d, mcs %d)\n",
      //  phy_vars_ue->Mod_id, frame, subframe, harq_pid, harq_process->status, harq_process->round,harq_process->TBS,harq_process->mcs);

    if(is_crnti)
    {
    LOG_D(PHY,"[UE %d] DLSCH: Setting ACK for nr_tti_rx %d (pid %d, round %d, TBS %d)\n",phy_vars_ue->Mod_id,nr_tti_rx,harq_pid,harq_process->round,harq_process->TBS);
    }
    //LOG_D(PHY,"[UE %d] DLSCH: Setting ACK for subframe %d (pid %d, round %d)\n",phy_vars_ue->Mod_id,subframe,harq_pid,harq_process->round);

  }

  // Reassembly of Transport block here
  offset = 0;
  Kr = harq_process->K;
  Kr_bytes = Kr>>3;

  /*
  printf("harq_pid %d\n",harq_pid);
  printf("F %d, Fbytes %d\n",harq_process->F,harq_process->F>>3);
  printf("C %d\n",harq_process->C);
  */
  for (r=0; r<harq_process->C; r++) {

    memcpy(harq_process->b+offset,
             harq_process->c[r],
             Kr_bytes- - (harq_process->F>>3) -((harq_process->C>1)?3:0));
    offset += (Kr_bytes - (harq_process->F>>3) - ((harq_process->C>1)?3:0));

#ifdef DEBUG_DLSCH_DECODING
    printf("Segment %d : Kr= %d bytes\n",r,Kr_bytes);
    printf("copied %d bytes to b sequence (harq_pid %d)\n",
              (Kr_bytes - (harq_process->F>>3)-((harq_process->C>1)?3:0)),harq_pid);
              printf("b[0] = %x,c[%d] = %x\n",
              harq_process->b[offset],
              harq_process->F>>3,
              harq_process->c[r]);
#endif

  }

  dlsch->last_iteration_cnt = ret;

  return(ret);
}

#ifdef UE_DLSCH_PARALLELISATION
uint32_t  dlsch_decoding_mthread(PHY_VARS_NR_UE *phy_vars_ue,
			 UE_rxtx_proc_t *proc,
                         int eNB_id,
                         short *dlsch_llr,
                         NR_DL_FRAME_PARMS *frame_parms,
                         NR_UE_DLSCH_t *dlsch,
                         NR_DL_UE_HARQ_t *harq_process,
                         uint32_t frame,
                         uint8_t nr_tti_rx,
                         uint8_t harq_pid,
                         uint8_t is_crnti,
                         uint8_t llr8_flag)
{

#if UE_TIMING_TRACE
  time_stats_t *dlsch_rate_unmatching_stats=&phy_vars_ue->dlsch_rate_unmatching_stats;
  time_stats_t *dlsch_turbo_decoding_stats=&phy_vars_ue->dlsch_turbo_decoding_stats;
  time_stats_t *dlsch_deinterleaving_stats=&phy_vars_ue->dlsch_deinterleaving_stats;
#endif
  uint32_t A,E;
  uint32_t G;
  uint32_t ret,offset;
  uint16_t iind;
  //  uint8_t dummy_channel_output[(3*8*block_length)+12];
  short dummy_w[MAX_NUM_DLSCH_SEGMENTS][3*(8448+64)];
  uint32_t r,r_offset=0,Kr,Kr_bytes,err_flag=0,Kr_int,kb,kc;
  uint8_t crc_type;
  //UE_rxtx_proc_t *proc = &phy_vars_ue->proc;
  int32_t no_iteration_ldpc;
  int Cby2;
  /*uint8_t C;
  uint8_t Qm;
  uint8_t Nl;
  uint8_t r_thread;
  uint32_t Er, Gp,GpmodC;*/
  t_nrLDPC_dec_params decParams;
    t_nrLDPC_dec_params* p_decParams = &decParams;
    t_nrLDPC_time_stats procTime;
    t_nrLDPC_time_stats* p_procTime =&procTime ;
    int16_t z [68*384];
    int8_t l [68*384];
    //__m128i l;
    int16_t inv_d [68*384];
    int16_t *p_invd =&inv_d;

    uint32_t i,j;
    uint32_t k;

    __m128i *pv = (__m128i*)&z;
    __m128i *pl = (__m128i*)&l;


#ifdef DEBUG_DLSCH_DECODING
  uint16_t i;
#endif
  //#ifdef __AVX2__
#if 0
  int Kr_last,skipped_last=0;
  uint8_t (*tc_2cw)(int16_t *y,
		    int16_t *y2,
		    uint8_t *,
		    uint8_t *,
		    uint16_t,
		    uint16_t,
		    uint16_t,
		    uint8_t,
		    uint8_t,
		    uint8_t,
		    time_stats_t *,
		    time_stats_t *,
		    time_stats_t *,
		    time_stats_t *,
		    time_stats_t *,
		    time_stats_t *,
		    time_stats_t *);

#endif
  uint8_t (*tc)(int16_t *y,
                uint8_t *,
                uint16_t,
                uint16_t,
                uint16_t,
                uint8_t,
                uint8_t,
                uint8_t,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *);




  if (!dlsch_llr) {
    printf("dlsch_decoding.c: NULL dlsch_llr pointer\n");
    return(dlsch->max_ldpc_iterations);
  }

  if (!harq_process) {
    printf("dlsch_decoding.c: NULL harq_process pointer\n");
    return(dlsch->max_ldpc_iterations);
  }

  if (!frame_parms) {
    printf("dlsch_decoding.c: NULL frame_parms pointer\n");
    return(dlsch->max_ldpc_iterations);
  }

  if (nr_tti_rx> (10*frame_parms->ttis_per_subframe-1)) {
    printf("dlsch_decoding.c: Illegal subframe index %d\n",nr_tti_rx);
    return(dlsch->max_ldpc_iterations);
  }

  if (dlsch->harq_ack[nr_tti_rx].ack != 2) {
    LOG_D(PHY, "[UE %d] DLSCH @ SF%d : ACK bit is %d instead of DTX even before PDSCH is decoded!\n",
        phy_vars_ue->Mod_id, nr_tti_rx, dlsch->harq_ack[nr_tti_rx].ack);
  }

  if (llr8_flag == 0) {
    //#ifdef __AVX2__
#if 0
    tc_2cw = phy_threegpplte_turbo_decoder16avx2;
#endif
    tc = phy_threegpplte_turbo_decoder16;
  }
  else
  {
	  AssertFatal (harq_process->TBS >= 256 , "Mismatch flag nbRB=%d TBS=%d mcs=%d Qm=%d RIV=%d round=%d \n",
			  harq_process->nb_rb, harq_process->TBS,harq_process->mcs,harq_process->Qm,harq_process->rvidx,harq_process->round);
	    tc = phy_threegpplte_turbo_decoder8;
  }


  //  nb_rb = dlsch->nb_rb;

  /*
  if (nb_rb > frame_parms->N_RB_DL) {
    printf("dlsch_decoding.c: Illegal nb_rb %d\n",nb_rb);
    return(max_ldpc_iterations);
    }*/

  /*harq_pid = dlsch->current_harq_pid[phy_vars_ue->current_thread_id[subframe]];
  if (harq_pid >= 8) {
    printf("dlsch_decoding.c: Illegal harq_pid %d\n",harq_pid);
    return(max_ldpc_iterations);
  }
  */


  harq_process->trials[harq_process->round]++;

  A = harq_process->TBS; //2072 for QPSK 1/3

  ret = dlsch->max_ldpc_iterations;


  G = harq_process->G;

  proc->decoder_main_available = 1;
  proc->decoder_thread_available = 0;
  proc->decoder_thread_available1 = 0;
  //get_G(frame_parms,nb_rb,dlsch->rb_alloc,mod_order,num_pdcch_symbols,phy_vars_ue->frame,subframe);

  //  printf("DLSCH Decoding, harq_pid %d Ndi %d\n",harq_pid,harq_process->Ndi);

  if (harq_process->round == 0) {
      // This is a new packet, so compute quantities regarding segmentation
      harq_process->B = A+24;
 #ifdef TD_DECODING
      lte_segmentation(NULL,
                       NULL,
                       harq_process->B,
                       &harq_process->C,
                       &harq_process->Cplus,
                       &harq_process->Cminus,
                       &harq_process->Kplus,
                       &harq_process->Kminus,
                       &harq_process->F);
      //  CLEAR LLR's HERE for first packet in process
  #else
      nr_segmentation(NULL,
      	                    NULL,
      	                    harq_process->B,
      	                    &harq_process->C,
      	                    &harq_process->Kplus,
      	                    &harq_process->Kminus,
      						&harq_process->Z,
      	                    &harq_process->F);
      	p_decParams->Z = harq_process->Z;
  #endif

    }

 	kb = harq_process->Kplus/harq_process->Z;
  	  if ( kb==22){
  		  p_decParams->BG = 1;
  		  p_decParams->R = 89;
  		  kc = 68;
  	  }
  	  else{
  		  p_decParams->BG = 2;
  		  p_decParams->R = 13;
  		  kc = 52;
  	  	  }

      p_decParams->numMaxIter = 2;
      Kr = p_decParams->Z*kb;
      p_decParams->outMode= 0;

  /*
  else {
    printf("dlsch_decoding.c: Ndi>0 not checked yet!!\n");
    return(max_ldpc_iterations);
  }
  */
  err_flag = 0;
  r_offset = 0;

  unsigned char bw_scaling =1;

  switch (frame_parms->N_RB_DL) {
  case 6:
    bw_scaling =16;
    break;

  case 25:
    bw_scaling =4;
    break;

  case 50:
    bw_scaling =2;
    break;

  default:
    bw_scaling =1;
    break;
  }

  if (harq_process->C > MAX_NUM_DLSCH_SEGMENTS/bw_scaling) {
    LOG_E(PHY,"Illegal harq_process->C %d > %d\n",harq_process->C,MAX_NUM_DLSCH_SEGMENTS/bw_scaling);
    return((1+dlsch->max_ldpc_iterations));
  }
#ifdef DEBUG_DLSCH_DECODING
  printf("Segmentation: C %d, Cminus %d, Kminus %d, Kplus %d\n",harq_process->C,harq_process->Cminus,harq_process->Kminus,harq_process->Kplus);
#endif

  opp_enabled=1;
if (harq_process->C>1) { // wakeup worker if more than 1 segment
     if (pthread_mutex_lock(&proc->mutex_dlsch_td) != 0) {
         LOG_E( PHY, "[SCHED][UE %d][Slot0] error locking mutex for UE dlsch td\n",phy_vars_ue->Mod_id );
         exit_fun("nothing to add");
     }

     /*Qm= harq_process->Qm;
     Nl=harq_process->Nl;
     r_thread = harq_process->C/2-1;
     C= harq_process->C;

     Gp = G/Nl/Qm;
      GpmodC = Gp%C;



      if (r_thread < (C-(GpmodC)))
        Er = Nl*Qm * (Gp/C);
      else
        Er = Nl*Qm * ((GpmodC==0?0:1) + (Gp/C));
     printf("mthread Er %d\n", Er);

     printf("mthread instance_cnt_dlsch_td %d\n",  proc->instance_cnt_dlsch_td);*/

     proc->instance_cnt_dlsch_td++;
               proc->eNB_id    = eNB_id;
               proc->harq_pid  = harq_pid;
               proc->llr8_flag = llr8_flag;
               //proc->r[0] = 1;

     if (proc->instance_cnt_dlsch_td == 0)
     {
     LOG_D(PHY,"unblock dlsch td processing thread blocked on instance_cnt_dlsch_td : %d \n", proc->instance_cnt_dlsch_td );
         if (pthread_cond_signal(&proc->cond_dlsch_td) != 0) {
             LOG_E( PHY, "[SCHED][UE %d][Slot0] ERROR pthread_cond_signal for UE dlsch td\n", phy_vars_ue->Mod_id);
             exit_fun("nothing to add");
         }
         if (pthread_mutex_unlock(&proc->mutex_dlsch_td) != 0) {
             LOG_E( PHY, "[SCHED][UE %d][Slot0] error unlocking mutex for UE dlsch td \n",phy_vars_ue->Mod_id );
             exit_fun("nothing to add");
         }

     } else
     {
         LOG_E( PHY, "[SCHED][UE %d] UE dlsch td thread busy (IC %d)!!\n", phy_vars_ue->Mod_id, proc->instance_cnt_dlsch_td);
         if (proc->instance_cnt_dlsch_td > 4)
             exit_fun("instance_cnt_dlsch_td > 4");
     }
     //AssertFatal(pthread_cond_signal(&proc->cond_slot1_dl_processing) ==0 ,"");
     AssertFatal(pthread_mutex_unlock(&proc->mutex_dlsch_td) ==0,"");

     if (harq_process->C>2) {
    	 if (pthread_mutex_lock(&proc->mutex_dlsch_td1) != 0) {
    	          LOG_E( PHY, "[SCHED][UE %d][Slot0] error locking mutex for UE dlsch td\n",phy_vars_ue->Mod_id );
    	          exit_fun("nothing to add");
    	      }

    	 proc->instance_cnt_dlsch_td1++;
                    proc->eNB_id    = eNB_id;
                    proc->harq_pid  = harq_pid;
                    proc->llr8_flag = llr8_flag;
                   // proc->Er = Er;

          if (proc->instance_cnt_dlsch_td1 == 0)
          {
          LOG_D(PHY,"unblock slot1 dl processing thread blocked on instance_cnt_dlsch_td : %d \n", proc->instance_cnt_dlsch_td1 );
              if (pthread_cond_signal(&proc->cond_dlsch_td1) != 0) {
                  LOG_E( PHY, "[SCHED][UE %d][Slot0] ERROR pthread_cond_signal for UE dlsch td\n", phy_vars_ue->Mod_id);
                  exit_fun("nothing to add");
              }
              if (pthread_mutex_unlock(&proc->mutex_dlsch_td1) != 0) {
                  LOG_E( PHY, "[SCHED][UE %d][Slot0] error unlocking mutex for UE dlsch td \n",phy_vars_ue->Mod_id );
                  exit_fun("nothing to add");
              }

          } else
          {
              LOG_E( PHY, "[SCHED][UE %d] UE dlsch td thread 1 busy (IC %d)!!\n", phy_vars_ue->Mod_id, proc->instance_cnt_dlsch_td1);
              if (proc->instance_cnt_dlsch_td1 > 4)
                  exit_fun("instance_cnt_dlsch_td1 > 4");
          }

          AssertFatal(pthread_mutex_unlock(&proc->mutex_dlsch_td1) ==0,"");

     }
	/*
	if (pthread_mutex_timedlock(&proc->mutex_td,&wait) != 0) {
       printf("[eNB] ERROR pthread_mutex_lock for TD thread (IC %d)\n", proc->instance_cnt_td);
       exit_fun( "error locking mutex_fep" );
       return -1;
     }

     if (proc->instance_cnt_td==0) {
       printf("[UE] TD thread busy\n");
       exit_fun("TD thread busy");
       pthread_mutex_unlock( &proc->mutex_td );
       return -1;
     }

     ++proc->instance_cnt_td;

     proc->tdp.UE        = phy_vars_ue;
     proc->tdp.eNB_id    = eNB_id;
     proc->tdp.harq_pid  = harq_pid;
     proc->tdp.llr8_flag = llr8_flag;

     printf("----- 2thread llr flag %d tdp flag %d\n",llr8_flag, proc->tdp.llr8_flag);


     // wakeup worker to do second half segments
     if (pthread_cond_signal(&proc->cond_td) != 0) {
       printf("[UE] ERROR pthread_cond_signal for td thread exit\n");
       exit_fun( "ERROR pthread_cond_signal" );
       return (1+dlsch->last_iteration_cnt);
     }

     pthread_mutex_unlock( &proc->mutex_td );*/


     Cby2 = 1; //harq_process->C/2;
     //proc->decoder_main_available = 1;
   }
   else {
     Cby2 = 1;
   }

  for (r=0; r<Cby2; r++) {

    // Get Turbo interleaver parameters
#ifdef TD_DECODING
	  if (r<harq_process->Cminus)
      Kr = harq_process->Kminus;
    else
      Kr = harq_process->Kplus;
#else
	  Kr = harq_process->Kplus;
#endif

    Kr_bytes = Kr>>3;

     //workaround for nr ldpc using lte interleaving
  if (dlsch->harq_processes[harq_pid]->C >= 2)
      	Kr_int = G/(3*dlsch->harq_processes[harq_pid]->C);
  else
  	Kr_int = Kr;
  
    if (Kr_bytes<=64)
      iind = (Kr_bytes-5);
    else if (Kr_bytes <=128)
      iind = 59 + ((Kr_bytes-64)>>1);
    else if (Kr_bytes <= 256)
      iind = 91 + ((Kr_bytes-128)>>2);
    else if (Kr_bytes <= 768)
      iind = 123 + ((Kr_bytes-256)>>3);
    else {
      //printf("dlsch_decoding: Illegal codeword size %d!!!\n",Kr_bytes);
      //return(dlsch->max_ldpc_iterations);
    }

#ifdef DEBUG_DLSCH_DECODING
    printf("f1 %d, f2 %d, F %d\n",f1f2mat_old[2*iind],f1f2mat_old[1+(2*iind)],(r==0) ? harq_process->F : 0);
#endif

#if UE_TIMING_TRACE
    start_meas(dlsch_rate_unmatching_stats);
#endif
    memset(&dummy_w[r][0],0,3*(8448+64)*sizeof(short));
    harq_process->RTC[r] = generate_dummy_w(Kr_int,
                                            (uint8_t*) &dummy_w[r][0],
                                            (r==0) ? harq_process->F : 0);

#ifdef DEBUG_DLSCH_DECODING
    LOG_D(PHY,"HARQ_PID %d Rate Matching Segment %d (coded bits %d,unpunctured/repeated bits %d, TBS %d, mod_order %d, nb_rb %d, Nl %d, rv %d, round %d)...\n",
          harq_pid,r, G,
          Kr*3,
          harq_process->TBS,
          harq_process->Qm,
          harq_process->nb_rb,
          harq_process->Nl,
          harq_process->rvidx,
          harq_process->round);
#endif

#ifdef DEBUG_DLSCH_DECODING
    printf(" in decoding dlsch->harq_processes[harq_pid]->rvidx = %d\n", dlsch->harq_processes[harq_pid]->rvidx);
#endif
    if (lte_rate_matching_turbo_rx(harq_process->RTC[r],
                                   G,
                                   harq_process->w[r],
                                   (uint8_t*)&dummy_w[r][0],
                                   dlsch_llr+r_offset,
                                   harq_process->C,
                                   dlsch->Nsoft,
                                   dlsch->Mdlharq,
                                   dlsch->Kmimo,
                                   harq_process->rvidx,
                                   (harq_process->round==0)?1:0,
                                   harq_process->Qm,
                                   harq_process->Nl,
                                   r,
                                   &E)==-1) {
#if UE_TIMING_TRACE
      stop_meas(dlsch_rate_unmatching_stats);
#endif
      LOG_E(PHY,"dlsch_decoding.c: Problem in rate_matching\n");
      return(dlsch->max_ldpc_iterations);
    } else
    {
#if UE_TIMING_TRACE
      stop_meas(dlsch_rate_unmatching_stats);
#endif
    }
    r_offset += E;
    //printf("main thread r_offset %d\n",r_offset);

    /*
    printf("Subblock deinterleaving, d %p w %p\n",
     harq_process->d[r],
     harq_process->w);
    */
#if UE_TIMING_TRACE
    start_meas(dlsch_deinterleaving_stats);
#endif
    sub_block_deinterleaving_turbo(4+Kr,
                                   &harq_process->d[r][96],

                                   harq_process->w[r]);
#if UE_TIMING_TRACE
    stop_meas(dlsch_deinterleaving_stats);
#endif
#ifdef DEBUG_DLSCH_DECODING
    /*
    if (r==0) {
              write_output("decoder_llr.m","decllr",dlsch_llr,G,1,0);
              write_output("decoder_in.m","dec",&harq_process->d[0][96],(3*8*Kr_bytes)+12,1,0);
    }

    printf("decoder input(segment %d) :",r);
    int i; for (i=0;i<(3*8*Kr_bytes)+12;i++)
      printf("%d : %d\n",i,harq_process->d[r][96+i]);
      printf("\n");*/
#endif


    //    printf("Clearing c, %p\n",harq_process->c[r]);
    memset(harq_process->c[r],0,Kr_bytes);

    //    printf("done\n");
    if (harq_process->C == 1)
      crc_type = CRC24_A;
    else
      crc_type = CRC24_B;

    /*
    printf("decoder input(segment %d)\n",r);
    for (i=0;i<(3*8*Kr_bytes)+12;i++)
      if ((harq_process->d[r][96+i]>7) ||
    (harq_process->d[r][96+i] < -8))
    printf("%d : %d\n",i,harq_process->d[r][96+i]);
    printf("\n");
    */

    //#ifndef __AVX2__
#if 1
    if (err_flag == 0) {
/*
        LOG_I(PHY, "turbo algo Kr=%d cb_cnt=%d C=%d nbRB=%d crc_type %d TBSInput=%d TBSHarq=%d TBSplus24=%d mcs=%d Qm=%d RIV=%d round=%d maxIter %d\n",
                            Kr,r,harq_process->C,harq_process->nb_rb,crc_type,A,harq_process->TBS,
                            harq_process->B,harq_process->mcs,harq_process->Qm,harq_process->rvidx,harq_process->round,dlsch->max_ldpc_iterations);
*/
    	if (llr8_flag) {
    		AssertFatal (Kr >= 256, "turbo algo issue Kr=%d cb_cnt=%d C=%d nbRB=%d TBSInput=%d TBSHarq=%d TBSplus24=%d mcs=%d Qm=%d RIV=%d round=%d\n",
    				Kr,r,harq_process->C,harq_process->nb_rb,A,harq_process->TBS,harq_process->B,harq_process->mcs,harq_process->Qm,harq_process->rvidx,harq_process->round);
    	}
#if UE_TIMING_TRACE
        start_meas(dlsch_turbo_decoding_stats);
#endif
      LOG_D(PHY,"mthread AbsSubframe %d.%d Start turbo segment %d/%d \n",frame%1024,nr_tti_rx,r,harq_process->C-1);
#ifdef TD_DECODING
      ret = tc
            (&harq_process->d[r][96],
             harq_process->c[r],
             Kr,
             f1f2mat_old[iind*2],
             f1f2mat_old[(iind*2)+1],
             dlsch->max_ldpc_iterations,
             crc_type,
             (r==0) ? harq_process->F : 0,
             &phy_vars_ue->dlsch_tc_init_stats,
             &phy_vars_ue->dlsch_tc_alpha_stats,
             &phy_vars_ue->dlsch_tc_beta_stats,
             &phy_vars_ue->dlsch_tc_gamma_stats,
             &phy_vars_ue->dlsch_tc_ext_stats,
             &phy_vars_ue->dlsch_tc_intl1_stats,
             &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);

#else

		memset(pv,0,2*p_decParams->Z*sizeof(int16_t));
        //memset(pl,0,2*p_decParams->Z*sizeof(int8_t));

		//if (A < 1000){

      	for (i=2*p_decParams->Z/8, j = 0; i < (68*p_decParams->Z/8+1); i++, j++)
      	{
      		pv[i]= _mm_loadu_si128((__m128i*)(&inv_d[8*j]));
      	}
      	/*}
		else{
      	for (i=2*p_decParams->Z/8, j = 0; i < (68*p_decParams->Z/8+1); i++, j++)
      	      	{
      	      		pv[i]= _mm_loadu_si128((__m128i*)&harq_process->d[r][96+8*j]);
      	      	}
		}*/
		for (i=0, j=0; j < (68*p_decParams->Z/16);  i+=2, j++)
      	      	{
      				//printf("mm packs i %d j %d\n", i, j);
      				//print128_num(pv[i]);
      				//print128_num(pv[i+1]);
      	      		pl[j] = _mm_packs_epi16(pv[i],pv[i+1]);
      	      		//print128_num2bytes(pl[j]);
      	      	}

		no_iteration_ldpc = nrLDPC_decoder(p_decParams,
      			&pl[0],
				llrProcBuf,
          		p_procTime);

		if (no_iteration_ldpc > 2)
			printf("Error number of iteration LPDC %d\n", no_iteration_ldpc);
		//else
			//printf("OK number of iteration LPDC %d\n", no_iteration_ldpc);

		for (int m=0; m < Kr>>3; m ++)
		      	      	{
		      				harq_process->c[r][m]= (uint8_t) llrProcBuf[m];
		      	      	}

		      	/*for (int u=0; u < Kr>>3; u ++)
		      	      	      	{
		      						ullrProcBuf[u]= (uint8_t) llrProcBuf[u];
		      	      	      	}


		      	printf("output unsigned ullrProcBuf \n");

		      	for (int j=0; j < Kr>>3; j ++)
		      	      	       	      	{

		      	      						printf(" %d \n", ullrProcBuf[j]);

		      	      	      	      	}
		     	printf(" \n");*/
#endif
		//printf("output channel decoder %d %d %d %d %d \n", harq_process->c[r][0], harq_process->c[r][1], harq_process->c[r][2],harq_process->c[r][3], harq_process->c[r][4]);

		 //printf("output decoder %d %d %d %d %d \n", harq_process->c[r][0], harq_process->c[r][1], harq_process->c[r][2],harq_process->c[r][3], harq_process->c[r][4]);


#if UE_TIMING_TRACE
      stop_meas(dlsch_turbo_decoding_stats);
#endif
    }
#else
    if ((harq_process->C == 1) ||
	((r==harq_process->C-1) && (skipped_last==0))) { // last segment with odd number of segments

#if UE_TIMING_TRACE
        start_meas(dlsch_turbo_decoding_stats);
#endif
      ret = tc
            (&harq_process->d[r][96],
             harq_process->c[r],
             Kr,
             f1f2mat_old[iind*2],
             f1f2mat_old[(iind*2)+1],
             dlsch->max_ldpc_iterations,
             crc_type,
             (r==0) ? harq_process->F : 0,
             &phy_vars_ue->dlsch_tc_init_stats,
             &phy_vars_ue->dlsch_tc_alpha_stats,
             &phy_vars_ue->dlsch_tc_beta_stats,
             &phy_vars_ue->dlsch_tc_gamma_stats,
             &phy_vars_ue->dlsch_tc_ext_stats,
             &phy_vars_ue->dlsch_tc_intl1_stats,
             &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);
 #if UE_TIMING_TRACE
      stop_meas(dlsch_turbo_decoding_stats);
#endif
      //      printf("single decode, exit\n");
      //      exit(-1);
    }
    else {
    // we can merge code segments
      if ((skipped_last == 0) && (r<harq_process->C-1)) {
	skipped_last = 1;
	Kr_last = Kr;
      }
      else {
	skipped_last=0;

	if (Kr_last == Kr) { // decode 2 code segments with AVX2 version
#ifdef DEBUG_DLSCH_DECODING
	  printf("single decoding segment %d (%p)\n",r-1,&harq_process->d[r-1][96]);
#endif
#if UE_TIMING_TRACE
	  start_meas(dlsch_turbo_decoding_stats);
#endif
#ifdef DEBUG_DLSCH_DECODING
	  printf("double decoding segments %d,%d (%p,%p)\n",r-1,r,&harq_process->d[r-1][96],&harq_process->d[r][96]);
#endif
	  ret = tc_2cw
            (&harq_process->d[r-1][96],
	     &harq_process->d[r][96],
             harq_process->c[r-1],
             harq_process->c[r],
             Kr,
             f1f2mat_old[iind*2],
             f1f2mat_old[(iind*2)+1],
             dlsch->max_ldpc_iterations,
             crc_type,
             (r==0) ? harq_process->F : 0,
             &phy_vars_ue->dlsch_tc_init_stats,
             &phy_vars_ue->dlsch_tc_alpha_stats,
             &phy_vars_ue->dlsch_tc_beta_stats,
             &phy_vars_ue->dlsch_tc_gamma_stats,
             &phy_vars_ue->dlsch_tc_ext_stats,
             &phy_vars_ue->dlsch_tc_intl1_stats,
             &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);
	  /*
	  ret = tc
            (&harq_process->d[r-1][96],
             harq_process->c[r-1],
             Kr_last,
             f1f2mat_old[iind*2],
             f1f2mat_old[(iind*2)+1],
             dlsch->max_ldpc_iterations,
             crc_type,
             (r==0) ? harq_process->F : 0,
             &phy_vars_ue->dlsch_tc_init_stats,
             &phy_vars_ue->dlsch_tc_alpha_stats,
             &phy_vars_ue->dlsch_tc_beta_stats,
             &phy_vars_ue->dlsch_tc_gamma_stats,
             &phy_vars_ue->dlsch_tc_ext_stats,
             &phy_vars_ue->dlsch_tc_intl1_stats,
             &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);

	     exit(-1);*/
#if UE_TIMING_TRACE
      stop_meas(dlsch_turbo_decoding_stats);
#endif
	}
	else { // Kr_last != Kr
#if UE_TIMING_TRACE
	  start_meas(dlsch_turbo_decoding_stats);
#endif
	  ret = tc
            (&harq_process->d[r-1][96],
             harq_process->c[r-1],
             Kr_last,
             f1f2mat_old[iind*2],
             f1f2mat_old[(iind*2)+1],
             dlsch->max_ldpc_iterations,
             crc_type,
             (r==0) ? harq_process->F : 0,
             &phy_vars_ue->dlsch_tc_init_stats,
             &phy_vars_ue->dlsch_tc_alpha_stats,
             &phy_vars_ue->dlsch_tc_beta_stats,
             &phy_vars_ue->dlsch_tc_gamma_stats,
             &phy_vars_ue->dlsch_tc_ext_stats,
             &phy_vars_ue->dlsch_tc_intl1_stats,
             &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);
#if UE_TIMING_TRACE
      stop_meas(dlsch_turbo_decoding_stats);

	  start_meas(dlsch_turbo_decoding_stats);
#endif

	  ret = tc
            (&harq_process->d[r][96],
             harq_process->c[r],
             Kr,
             f1f2mat_old[iind*2],
             f1f2mat_old[(iind*2)+1],
             dlsch->max_ldpc_iterations,
             crc_type,
             (r==0) ? harq_process->F : 0,
             &phy_vars_ue->dlsch_tc_init_stats,
             &phy_vars_ue->dlsch_tc_alpha_stats,
             &phy_vars_ue->dlsch_tc_beta_stats,
             &phy_vars_ue->dlsch_tc_gamma_stats,
             &phy_vars_ue->dlsch_tc_ext_stats,
             &phy_vars_ue->dlsch_tc_intl1_stats,
             &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);

#if UE_TIMING_TRACE

	  stop_meas(dlsch_turbo_decoding_stats);

	  /*printf("Segmentation: C %d r %d, dlsch_rate_unmatching_stats %5.3f dlsch_deinterleaving_stats %5.3f  dlsch_turbo_decoding_stats %5.3f \n",
              harq_process->C,
              r,
              dlsch_rate_unmatching_stats->p_time/(cpuf*1000.0),
              dlsch_deinterleaving_stats->p_time/(cpuf*1000.0),
              dlsch_turbo_decoding_stats->p_time/(cpuf*1000.0));*/
#endif
	}
      }
    }
#endif


    if ((err_flag == 0) && (ret>=(1+dlsch->max_ldpc_iterations))) {// a Code segment is in error so break;
      LOG_D(PHY,"AbsSubframe %d.%d CRC failed, segment %d/%d \n",frame%1024,nr_tti_rx,r,harq_process->C-1);
      err_flag = 1;
    }
  }

  int32_t frame_rx_prev = frame;
  int32_t tti_rx_prev = nr_tti_rx - 1;
  if (tti_rx_prev < 0) {
    frame_rx_prev--;
    tti_rx_prev += 10*frame_parms->ttis_per_subframe;
  }
  frame_rx_prev = frame_rx_prev%1024;

  if (err_flag == 1) {
#if UE_DEBUG_TRACE
    LOG_I(PHY,"[UE %d] DLSCH: Setting NAK for SFN/SF %d/%d (pid %d, status %d, round %d, TBS %d, mcs %d) Kr %d r %d harq_process->round %d\n",
        phy_vars_ue->Mod_id, frame, nr_tti_rx, harq_pid,harq_process->status, harq_process->round,harq_process->TBS,harq_process->mcs,Kr,r,harq_process->round);
#endif
    dlsch->harq_ack[nr_tti_rx].ack = 0;
    dlsch->harq_ack[nr_tti_rx].harq_id = harq_pid;
    dlsch->harq_ack[nr_tti_rx].send_harq_status = 1;
    harq_process->errors[harq_process->round]++;
    harq_process->round++;


    //    printf("Rate: [UE %d] DLSCH: Setting NACK for subframe %d (pid %d, round %d)\n",phy_vars_ue->Mod_id,subframe,harq_pid,harq_process->round);
    if (harq_process->round >= dlsch->Mdlharq) {
      harq_process->status = SCH_IDLE;
      harq_process->round  = 0;
    }
    if(is_crnti)
    {
    LOG_D(PHY,"[UE %d] DLSCH: Setting NACK for nr_tti_rx %d (pid %d, pid status %d, round %d/Max %d, TBS %d)\n",
               phy_vars_ue->Mod_id,nr_tti_rx,harq_pid,harq_process->status,harq_process->round,dlsch->Mdlharq,harq_process->TBS);
    }

    return((1+dlsch->max_ldpc_iterations));
  } else {
#if UE_DEBUG_TRACE
      LOG_I(PHY,"[UE %d] DLSCH: Setting ACK for nr_tti_rx %d TBS %d mcs %d nb_rb %d\n",
           phy_vars_ue->Mod_id,nr_tti_rx,harq_process->TBS,harq_process->mcs,harq_process->nb_rb);
#endif

    harq_process->status = SCH_IDLE;
    harq_process->round  = 0;
    dlsch->harq_ack[nr_tti_rx].ack = 1;
    dlsch->harq_ack[nr_tti_rx].harq_id = harq_pid;
    dlsch->harq_ack[nr_tti_rx].send_harq_status = 1;
    //LOG_I(PHY,"[UE %d] DLSCH: Setting ACK for SFN/SF %d/%d (pid %d, status %d, round %d, TBS %d, mcs %d)\n",
      //  phy_vars_ue->Mod_id, frame, subframe, harq_pid, harq_process->status, harq_process->round,harq_process->TBS,harq_process->mcs);

    if(is_crnti)
    {
    LOG_D(PHY,"[UE %d] DLSCH: Setting ACK for nr_tti_rx %d (pid %d, round %d, TBS %d)\n",phy_vars_ue->Mod_id,nr_tti_rx,harq_pid,harq_process->round,harq_process->TBS);
    }
    //LOG_D(PHY,"[UE %d] DLSCH: Setting ACK for subframe %d (pid %d, round %d)\n",phy_vars_ue->Mod_id,subframe,harq_pid,harq_process->round);

  }

  // Reassembly of Transport block here
  offset = 0;

  /*
  printf("harq_pid %d\n",harq_pid);
  printf("F %d, Fbytes %d\n",harq_process->F,harq_process->F>>3);
  printf("C %d\n",harq_process->C);
  */
  uint32_t wait = 0;
  if (harq_process->C==2){
	  while((proc->decoder_thread_available == 0) )
  {
          usleep(1);
          wait++;
  }
  }
  else if ((harq_process->C==3) ){
	  while((proc->decoder_thread_available == 0) || (proc->decoder_thread_available1 == 0))
			  {
			            usleep(1);
			            wait++;
			    }
  }

  proc->decoder_main_available = 0;
  
  for (r=0; r<harq_process->C; r++) {
    if (r<harq_process->Cminus)
      Kr = harq_process->Kminus;
    else
      Kr = harq_process->Kplus;

    Kr_bytes = Kr>>3;

    //    printf("Segment %d : Kr= %d bytes\n",r,Kr_bytes);
    if (r==0) {
      memcpy(harq_process->b,
             &harq_process->c[0][(harq_process->F>>3)],
             Kr_bytes - (harq_process->F>>3)- ((harq_process->C>1)?3:0));
      offset = Kr_bytes - (harq_process->F>>3) - ((harq_process->C>1)?3:0);
      //            printf("copied %d bytes to b sequence (harq_pid %d)\n",
      //          Kr_bytes - (harq_process->F>>3),harq_pid);
      //          printf("b[0] = %x,c[%d] = %x\n",
      //      harq_process->b[0],
      //      harq_process->F>>3,
      //      harq_process->c[0][(harq_process->F>>3)]);
    } else {
      memcpy(harq_process->b+offset,
             harq_process->c[r],
             Kr_bytes- ((harq_process->C>1)?3:0));
      offset += (Kr_bytes - ((harq_process->C>1)?3:0));
    }
  }

  dlsch->last_iteration_cnt = ret;
  //proc->decoder_thread_available = 0;
  //proc->decoder_main_available = 0;
  
  //wait for worker to finish
  //wait_on_busy_condition(&proc->mutex_td,&proc->cond_td,&proc->instance_cnt_dlsch td,"dlsch td thread");

  //return( (ret>proc->tdp.ret) ? ret : proc->tdp.ret );

  return(ret);
}
#endif

#ifdef UE_DLSCH_PARALLELISATION
#define FIFO_PRIORITY   39
uint32_t  dlsch_decoding_2thread0(void *arg)
{
	static __thread int UE_dlsch_td_retval;
    struct rx_tx_thread_data *rtd = arg;
    UE_rxtx_proc_t *proc = rtd->proc;
    PHY_VARS_NR_UE    *phy_vars_ue   = rtd->UE;

    int llr8_flag1;
    int32_t no_iteration_ldpc;
    t_nrLDPC_dec_params decParams;
    t_nrLDPC_dec_params* p_decParams = &decParams;
    t_nrLDPC_time_stats procTime;
    t_nrLDPC_time_stats* p_procTime =&procTime ;
    int16_t z [68*384];
    int8_t l [68*384];
    //__m128i l;
    int16_t inv_d [68*384];
    int16_t *p_invd =&inv_d;

    uint32_t i,j;
    uint32_t k;

    __m128i *pv = (__m128i*)&z;
    __m128i *pl = (__m128i*)&l;

    proc->instance_cnt_dlsch_td=-1;
    proc->nr_tti_rx=proc->sub_frame_start;

    proc->decoder_thread_available = 0;
    
    char threadname[256];
    sprintf(threadname,"UE_thread_dlsch_td_%d", proc->sub_frame_start);

	cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    if ( (proc->sub_frame_start+1)%RX_NB_TH == 0 && threads.dlsch_td_one != -1 )
    	CPU_SET(threads.dlsch_td_one, &cpuset);
    if ( (proc->sub_frame_start+1)%RX_NB_TH == 1 && threads.dlsch_td_two != -1 )
    	CPU_SET(threads.dlsch_td_two, &cpuset);
    if ( (proc->sub_frame_start+1)%RX_NB_TH == 2 && threads.dlsch_td_three != -1 )
    	CPU_SET(threads.dlsch_td_three, &cpuset);



#if UE_TIMING_TRACE
  time_stats_t *dlsch_rate_unmatching_stats=&phy_vars_ue->dlsch_rate_unmatching_stats;
  time_stats_t *dlsch_turbo_decoding_stats=&phy_vars_ue->dlsch_turbo_decoding_stats;
  time_stats_t *dlsch_deinterleaving_stats=&phy_vars_ue->dlsch_deinterleaving_stats;
#endif
  uint32_t A,E;
  uint32_t G;
  uint32_t ret,offset;
  uint16_t iind;
  //  uint8_t dummy_channel_output[(3*8*block_length)+12];
  short dummy_w[MAX_NUM_DLSCH_SEGMENTS][3*(8448+64)];
  uint32_t r,r_offset=0,Kr,Kr_bytes,err_flag=0,Kr_int;
  uint8_t crc_type;
  uint8_t C;
   uint8_t Qm;
   uint8_t Nl;
   uint32_t Er, Gp,GpmodC;
#ifdef DEBUG_DLSCH_DECODING
  uint16_t i;
#endif
  //#ifdef __AVX2__

  uint8_t (*tc)(int16_t *y,
                uint8_t *,
                uint16_t,
                uint16_t,
                uint16_t,
                uint8_t,
                uint8_t,
                uint8_t,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *);
  if (llr8_flag1 == 0) {
    //#ifdef __AVX2__
#if 0
    tc_2cw = phy_threegpplte_turbo_decoder16avx2;
#endif
    tc = phy_threegpplte_turbo_decoder16;
  }
  else
  {
	  //AssertFatal (harq_process->TBS >= 256 , "Mismatch flag nbRB=%d TBS=%d mcs=%d Qm=%d RIV=%d round=%d \n",
	  //		  harq_process->nb_rb, harq_process->TBS,harq_process->mcs,harq_process->Qm,harq_process->rvidx,harq_process->round);
	    tc = phy_threegpplte_turbo_decoder8;
  }

	init_thread(900000,1000000 , FIFO_PRIORITY-1, &cpuset, threadname);
	while (!oai_exit) {

		//proc->decoder_thread_available = 1;
		
	        if (pthread_mutex_lock(&proc->mutex_dlsch_td) != 0) {
	            LOG_E( PHY, "[SCHED][UE] error locking mutex for UE dlsch td\n" );
	            exit_fun("nothing to add");
	        }
	        while (proc->instance_cnt_dlsch_td < 0) {
	            // most of the time, the thread is waiting here
	            pthread_cond_wait( &proc->cond_dlsch_td, &proc->mutex_dlsch_td );
	        }
	        if (pthread_mutex_unlock(&proc->mutex_dlsch_td) != 0) {
	            LOG_E( PHY, "[SCHED][UE] error unlocking mutex for UE dlsch_td \n" );
	            exit_fun("nothing to add");
	        }

	        uint32_t wait = 0;
	        	          while(proc->decoder_main_available == 0)
	        	          {
	        	                  usleep(1);
	        	                  wait++;
	        	          }

	        	          //proc->decoder_thread_available = 0;
	        //PHY_VARS_NR_UE *phy_vars_ue   		= tdp->UE;
	        	int eNB_id         				= proc->eNB_id;
	        	int harq_pid      				= proc->harq_pid;
	        	llr8_flag1     					= proc->llr8_flag;
	        	//r_offset						= proc->Er;
	        	//UE_rxtx_proc_t *proc    		= tdp->proc;
	        	int frame                       = proc->frame_rx;
	        	int subframe      				= proc->nr_tti_rx;
	        	NR_UE_DLSCH_t *dlsch 			= phy_vars_ue->dlsch[phy_vars_ue->current_thread_id[subframe]][eNB_id][0];
	        	NR_DL_UE_HARQ_t *harq_process  = dlsch->harq_processes[harq_pid];
	        	short *dlsch_llr 				= phy_vars_ue->pdsch_vars[phy_vars_ue->current_thread_id[subframe]][eNB_id]->llr[0];
	        	//printf("2thread0 llr flag %d tdp flag %d\n",llr8_flag1, tdp->llr8_flag);
	        	
  //  nb_rb = dlsch->nb_rb;

  /*
  if (nb_rb > frame_parms->N_RB_DL) {
    printf("dlsch_decoding.c: Illegal nb_rb %d\n",nb_rb);
    return(max_ldpc_iterations);
    }*/

  /*harq_pid = dlsch->current_harq_pid[phy_vars_ue->current_thread_id[subframe]];
  if (harq_pid >= 8) {
    printf("dlsch_decoding.c: Illegal harq_pid %d\n",harq_pid);
    return(max_ldpc_iterations);
  }
  */



  harq_process->trials[harq_process->round]++;

  A = harq_process->TBS; //2072 for QPSK 1/3

  ret = dlsch->max_ldpc_iterations;


  G = harq_process->G;
  //get_G(frame_parms,nb_rb,dlsch->rb_alloc,mod_order,num_pdcch_symbols,phy_vars_ue->frame,subframe);
    
  if (harq_process->round == 0) {
    // This is a new packet, so compute quantities regarding segmentation
    harq_process->B = A+24;
 #ifdef TD_DECODING
      lte_segmentation(NULL,
                       NULL,
                       harq_process->B,
                       &harq_process->C,
                       &harq_process->Cplus,
                       &harq_process->Cminus,
                       &harq_process->Kplus,
                       &harq_process->Kminus,
                       &harq_process->F);
      //  CLEAR LLR's HERE for first packet in process
  #else
      nr_segmentation(NULL,
      	                    NULL,
      	                    harq_process->B,
      	                    &harq_process->C,
      	                    &harq_process->Kplus,
      	                    &harq_process->Kminus,
      						&harq_process->Z,
      	                    &harq_process->F);
      	p_decParams->Z = harq_process->Z;
  #endif

    }

    //	  p_decParams->Z = 128;
        p_decParams->BG = 1;
        p_decParams->R = 89;
        p_decParams->numMaxIter = 2;
        Kr = p_decParams->Z*22;
        p_decParams->outMode= 0;

  /*
  else {
    printf("dlsch_decoding.c: Ndi>0 not checked yet!!\n");
    return(max_ldpc_iterations);
  }
  */
  err_flag = 0;
  //r_offset = 0;

  /*
  unsigned char bw_scaling =1;

  switch (frame_parms->N_RB_DL) {
  case 6:
    bw_scaling =16;
    break;

  case 25:
    bw_scaling =4;
    break;

  case 50:
    bw_scaling =2;
    break;

  default:
    bw_scaling =1;
    break;
  }

  if (harq_process->C > MAX_NUM_DLSCH_SEGMENTS/bw_scaling) {
    LOG_E(PHY,"Illegal harq_process->C %d > %d\n",harq_process->C,MAX_NUM_DLSCH_SEGMENTS/bw_scaling);
    return((1+dlsch->max_ldpc_iterations));
  }*/
#ifdef DEBUG_DLSCH_DECODING
  printf("Segmentation: C %d, Cminus %d, Kminus %d, Kplus %d\n",harq_process->C,harq_process->Cminus,harq_process->Kminus,harq_process->Kplus);
#endif

  opp_enabled=1;
  
  Qm= harq_process->Qm;
  	       Nl=harq_process->Nl;
  	       //r_thread = harq_process->C/2-1;
  	       C= harq_process->C;

  	       Gp = G/Nl/Qm;
  	        GpmodC = Gp%C;



  	        if ((C/2-1) < (C-(GpmodC)))
  	        	r_offset = Nl*Qm * (Gp/C);
  	        else
  	        	r_offset = Nl*Qm * ((GpmodC==0?0:1) + (Gp/C));

		//	printf("thread0 r_offset %d\n",r_offset);
  	       
  //for (r=(harq_process->C/2); r<harq_process->C; r++) {
  	     r=1; //(harq_process->C/2);

    // Get Turbo interleaver parameters
#ifdef TD_DECODING
	  if (r<harq_process->Cminus)
      Kr = harq_process->Kminus;
    else
      Kr = harq_process->Kplus;
#else
	  Kr = harq_process->Kplus;
#endif

    Kr_bytes = Kr>>3;

     //workaround for nr ldpc using lte interleaving
  if (dlsch->harq_processes[harq_pid]->C >= 2)
      	Kr_int = G/(3*dlsch->harq_processes[harq_pid]->C);
  else
  	Kr_int = Kr;

    if (Kr_bytes<=64)
      iind = (Kr_bytes-5);
    else if (Kr_bytes <=128)
      iind = 59 + ((Kr_bytes-64)>>1);
    else if (Kr_bytes <= 256)
      iind = 91 + ((Kr_bytes-128)>>2);
    else if (Kr_bytes <= 768)
      iind = 123 + ((Kr_bytes-256)>>3);
    else {
      //printf("dlsch_decoding: Illegal codeword size %d!!!\n",Kr_bytes);
      //return(dlsch->max_ldpc_iterations);
    }

#ifdef DEBUG_DLSCH_DECODING
    printf("f1 %d, f2 %d, F %d\n",f1f2mat_old[2*iind],f1f2mat_old[1+(2*iind)],(r==0) ? harq_process->F : 0);
#endif

#if UE_TIMING_TRACE
    start_meas(dlsch_rate_unmatching_stats);
#endif
    memset(&dummy_w[r][0],0,3*(8448+64)*sizeof(short));
    harq_process->RTC[r] = generate_dummy_w(Kr_int,
                                            (uint8_t*) &dummy_w[r][0],
                                            (r==0) ? harq_process->F : 0);

#ifdef DEBUG_DLSCH_DECODING
    LOG_D(PHY,"HARQ_PID %d Rate Matching Segment %d (coded bits %d,unpunctured/repeated bits %d, TBS %d, mod_order %d, nb_rb %d, Nl %d, rv %d, round %d)...\n",
          harq_pid,r, G,
          Kr*3,
          harq_process->TBS,
          harq_process->Qm,
          harq_process->nb_rb,
          harq_process->Nl,
          harq_process->rvidx,
          harq_process->round);
#endif

#ifdef DEBUG_DLSCH_DECODING
    printf(" in decoding dlsch->harq_processes[harq_pid]->rvidx = %d\n", dlsch->harq_processes[harq_pid]->rvidx);
#endif
    if (lte_rate_matching_turbo_rx(harq_process->RTC[r],
                                   G,
                                   harq_process->w[r],
                                   (uint8_t*)&dummy_w[r][0],
                                   dlsch_llr+r_offset,
                                   harq_process->C,
                                   dlsch->Nsoft,
                                   dlsch->Mdlharq,
                                   dlsch->Kmimo,
                                   harq_process->rvidx,
                                   (harq_process->round==0)?1:0,
                                   harq_process->Qm,
                                   harq_process->Nl,
                                   r,
                                   &E)==-1) {
#if UE_TIMING_TRACE
      stop_meas(dlsch_rate_unmatching_stats);
#endif
      LOG_E(PHY,"dlsch_decoding.c: Problem in rate_matching\n");
      //return(dlsch->max_ldpc_iterations);
    } else
    {
#if UE_TIMING_TRACE
      stop_meas(dlsch_rate_unmatching_stats);
#endif
    }
    r_offset += E;

    /*
    printf("Subblock deinterleaving, d %p w %p\n",
     harq_process->d[r],
     harq_process->w);
    */
#if UE_TIMING_TRACE
    start_meas(dlsch_deinterleaving_stats);
#endif
    sub_block_deinterleaving_turbo(4+Kr,
                                   &harq_process->d[r][96],

                                   harq_process->w[r]);
#if UE_TIMING_TRACE
    stop_meas(dlsch_deinterleaving_stats);
#endif
#ifdef DEBUG_DLSCH_DECODING
    /*
    if (r==0) {
              write_output("decoder_llr.m","decllr",dlsch_llr,G,1,0);
              write_output("decoder_in.m","dec",&harq_process->d[0][96],(3*8*Kr_bytes)+12,1,0);
    }

    printf("decoder input(segment %d) :",r);
    int i; for (i=0;i<(3*8*Kr_bytes)+12;i++)
      printf("%d : %d\n",i,harq_process->d[r][96+i]);
      printf("\n");*/
#endif


    //    printf("Clearing c, %p\n",harq_process->c[r]);
    memset(harq_process->c[r],0,Kr_bytes);

    //    printf("done\n");
    if (harq_process->C == 1)
      crc_type = CRC24_A;
    else
      crc_type = CRC24_B;

    /*
    printf("decoder input(segment %d)\n",r);
    for (i=0;i<(3*8*Kr_bytes)+12;i++)
      if ((harq_process->d[r][96+i]>7) ||
    (harq_process->d[r][96+i] < -8))
    printf("%d : %d\n",i,harq_process->d[r][96+i]);
    printf("\n");
    */

    //#ifndef __AVX2__
#if 1
    if (err_flag == 0) {
/*
        LOG_I(PHY, "turbo algo Kr=%d cb_cnt=%d C=%d nbRB=%d crc_type %d TBSInput=%d TBSHarq=%d TBSplus24=%d mcs=%d Qm=%d RIV=%d round=%d maxIter %d\n",
                            Kr,r,harq_process->C,harq_process->nb_rb,crc_type,A,harq_process->TBS,
                            harq_process->B,harq_process->mcs,harq_process->Qm,harq_process->rvidx,harq_process->round,dlsch->max_ldpc_iterations);
*/
    	if (llr8_flag1) {
    		AssertFatal (Kr >= 256, "turbo algo issue Kr=%d cb_cnt=%d C=%d nbRB=%d TBSInput=%d TBSHarq=%d TBSplus24=%d mcs=%d Qm=%d RIV=%d round=%d\n",
    				Kr,r,harq_process->C,harq_process->nb_rb,A,harq_process->TBS,harq_process->B,harq_process->mcs,harq_process->Qm,harq_process->rvidx,harq_process->round);
    	}
#if UE_TIMING_TRACE
        start_meas(dlsch_turbo_decoding_stats);
#endif
//      LOG_D(PHY,"AbsSubframe %d.%d Start turbo segment %d/%d \n",frame%1024,subframe,r,harq_process->C-1);
#ifdef TD_DECODING
        ret = tc
            (&harq_process->d[r][96],
             harq_process->c[r],
             Kr,
             f1f2mat_old[iind*2],
             f1f2mat_old[(iind*2)+1],
             dlsch->max_ldpc_iterations,
             crc_type,
             (r==0) ? harq_process->F : 0,
             &phy_vars_ue->dlsch_tc_init_stats,
             &phy_vars_ue->dlsch_tc_alpha_stats,
             &phy_vars_ue->dlsch_tc_beta_stats,
             &phy_vars_ue->dlsch_tc_gamma_stats,
             &phy_vars_ue->dlsch_tc_ext_stats,
             &phy_vars_ue->dlsch_tc_intl1_stats,
             &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);

      
#else

		memset(pv,0,2*p_decParams->Z*sizeof(int16_t));
        //memset(pl,0,2*p_decParams->Z*sizeof(int8_t));

		//if (A < 1000){

      	for (i=2*p_decParams->Z/8, j = 0; i < (68*p_decParams->Z/8+1); i++, j++)
      	{
      		pv[i]= _mm_loadu_si128((__m128i*)(&inv_d[8*j]));
      	}
      	/*}
		else{
      	for (i=2*p_decParams->Z/8, j = 0; i < (68*p_decParams->Z/8+1); i++, j++)
      	      	{
      	      		pv[i]= _mm_loadu_si128((__m128i*)&harq_process->d[r][96+8*j]);
      	      	}
		}*/
		for (i=0, j=0; j < (68*p_decParams->Z/16);  i+=2, j++)
      	      	{
      				//printf("mm packs i %d j %d\n", i, j);
      				//print128_num(pv[i]);
      				//print128_num(pv[i+1]);
      	      		pl[j] = _mm_packs_epi16(pv[i],pv[i+1]);
      	      		//print128_num2bytes(pl[j]);
      	      	}

		no_iteration_ldpc = nrLDPC_decoder(p_decParams,
      			&pl[0],
				llrProcBuf,
          		p_procTime);

		if (no_iteration_ldpc > 2)
			printf("Error number of iteration LPDC %d\n", no_iteration_ldpc);
		//else
			//printf("OK number of iteration LPDC %d\n", no_iteration_ldpc);

		for (int m=0; m < Kr>>3; m ++)
		      	      	{
		      				harq_process->c[r][m]= (uint8_t) llrProcBuf[m];
		      	      	}

		      	/*for (int u=0; u < Kr>>3; u ++)
		      	      	      	{
		      						ullrProcBuf[u]= (uint8_t) llrProcBuf[u];
		      	      	      	}


		      	printf("output unsigned ullrProcBuf \n");

		      	for (int j=0; j < Kr>>3; j ++)
		      	      	       	      	{

		      	      						printf(" %d \n", ullrProcBuf[j]);

		      	      	      	      	}
		     	printf(" \n");*/
#endif
		
#if UE_TIMING_TRACE
      stop_meas(dlsch_turbo_decoding_stats);
#endif
    }
#else
    if ((harq_process->C == 1) ||
	((r==harq_process->C-1) && (skipped_last==0))) { // last segment with odd number of segments

#if UE_TIMING_TRACE
        start_meas(dlsch_turbo_decoding_stats);
#endif
      ret = tc
            (&harq_process->d[r][96],
             harq_process->c[r],
             Kr,
             f1f2mat_old[iind*2],
             f1f2mat_old[(iind*2)+1],
             dlsch->max_ldpc_iterations,
             crc_type,
             (r==0) ? harq_process->F : 0,
             &phy_vars_ue->dlsch_tc_init_stats,
             &phy_vars_ue->dlsch_tc_alpha_stats,
             &phy_vars_ue->dlsch_tc_beta_stats,
             &phy_vars_ue->dlsch_tc_gamma_stats,
             &phy_vars_ue->dlsch_tc_ext_stats,
             &phy_vars_ue->dlsch_tc_intl1_stats,
             &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);
 #if UE_TIMING_TRACE
      stop_meas(dlsch_turbo_decoding_stats);
#endif
      //      printf("single decode, exit\n");
      //      exit(-1);
    }
    else {
    // we can merge code segments
      if ((skipped_last == 0) && (r<harq_process->C-1)) {
	skipped_last = 1;
	Kr_last = Kr;
      }
      else {
	skipped_last=0;

	if (Kr_last == Kr) { // decode 2 code segments with AVX2 version
#ifdef DEBUG_DLSCH_DECODING
	  printf("single decoding segment %d (%p)\n",r-1,&harq_process->d[r-1][96]);
#endif
#if UE_TIMING_TRACE
	  start_meas(dlsch_turbo_decoding_stats);
#endif
#ifdef DEBUG_DLSCH_DECODING
	  printf("double decoding segments %d,%d (%p,%p)\n",r-1,r,&harq_process->d[r-1][96],&harq_process->d[r][96]);
#endif
	  ret = tc_2cw
            (&harq_process->d[r-1][96],
	     &harq_process->d[r][96],
             harq_process->c[r-1],
             harq_process->c[r],
             Kr,
             f1f2mat_old[iind*2],
             f1f2mat_old[(iind*2)+1],
             dlsch->max_ldpc_iterations,
             crc_type,
             (r==0) ? harq_process->F : 0,
             &phy_vars_ue->dlsch_tc_init_stats,
             &phy_vars_ue->dlsch_tc_alpha_stats,
             &phy_vars_ue->dlsch_tc_beta_stats,
             &phy_vars_ue->dlsch_tc_gamma_stats,
             &phy_vars_ue->dlsch_tc_ext_stats,
             &phy_vars_ue->dlsch_tc_intl1_stats,
             &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);
	  /*
	  ret = tc
            (&harq_process->d[r-1][96],
             harq_process->c[r-1],
             Kr_last,
             f1f2mat_old[iind*2],
             f1f2mat_old[(iind*2)+1],
             dlsch->max_ldpc_iterations,
             crc_type,
             (r==0) ? harq_process->F : 0,
             &phy_vars_ue->dlsch_tc_init_stats,
             &phy_vars_ue->dlsch_tc_alpha_stats,
             &phy_vars_ue->dlsch_tc_beta_stats,
             &phy_vars_ue->dlsch_tc_gamma_stats,
             &phy_vars_ue->dlsch_tc_ext_stats,
             &phy_vars_ue->dlsch_tc_intl1_stats,
             &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);

	     exit(-1);*/
#if UE_TIMING_TRACE
      stop_meas(dlsch_turbo_decoding_stats);
#endif
	}
	else { // Kr_last != Kr
#if UE_TIMING_TRACE
	  start_meas(dlsch_turbo_decoding_stats);
#endif
	  ret = tc
            (&harq_process->d[r-1][96],
             harq_process->c[r-1],
             Kr_last,
             f1f2mat_old[iind*2],
             f1f2mat_old[(iind*2)+1],
             dlsch->max_ldpc_iterations,
             crc_type,
             (r==0) ? harq_process->F : 0,
             &phy_vars_ue->dlsch_tc_init_stats,
             &phy_vars_ue->dlsch_tc_alpha_stats,
             &phy_vars_ue->dlsch_tc_beta_stats,
             &phy_vars_ue->dlsch_tc_gamma_stats,
             &phy_vars_ue->dlsch_tc_ext_stats,
             &phy_vars_ue->dlsch_tc_intl1_stats,
             &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);
#if UE_TIMING_TRACE
      stop_meas(dlsch_turbo_decoding_stats);

	  start_meas(dlsch_turbo_decoding_stats);
#endif

	  ret = tc
            (&harq_process->d[r][96],
             harq_process->c[r],
             Kr,
             f1f2mat_old[iind*2],
             f1f2mat_old[(iind*2)+1],
             dlsch->max_ldpc_iterations,
             crc_type,
             (r==0) ? harq_process->F : 0,
             &phy_vars_ue->dlsch_tc_init_stats,
             &phy_vars_ue->dlsch_tc_alpha_stats,
             &phy_vars_ue->dlsch_tc_beta_stats,
             &phy_vars_ue->dlsch_tc_gamma_stats,
             &phy_vars_ue->dlsch_tc_ext_stats,
             &phy_vars_ue->dlsch_tc_intl1_stats,
             &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);

#if UE_TIMING_TRACE

	  stop_meas(dlsch_turbo_decoding_stats);

	  /*printf("Segmentation: C %d r %d, dlsch_rate_unmatching_stats %5.3f dlsch_deinterleaving_stats %5.3f  dlsch_turbo_decoding_stats %5.3f \n",
              harq_process->C,
              r,
              dlsch_rate_unmatching_stats->p_time/(cpuf*1000.0),
              dlsch_deinterleaving_stats->p_time/(cpuf*1000.0),
              dlsch_turbo_decoding_stats->p_time/(cpuf*1000.0));*/
#endif
	}
      }
    }
#endif


    if ((err_flag == 0) && (ret>=(1+dlsch->max_ldpc_iterations))) {// a Code segment is in error so break;
//      LOG_D(PHY,"AbsSubframe %d.%d CRC failed, segment %d/%d \n",frame%1024,subframe,r,harq_process->C-1);
      err_flag = 1;
    }
  //}

  /*int32_t frame_rx_prev = frame;
  int32_t subframe_rx_prev = subframe - 1;
  if (subframe_rx_prev < 0) {
    frame_rx_prev--;
    subframe_rx_prev += 10;
  }
  frame_rx_prev = frame_rx_prev%1024;*/
#if 0
  if (err_flag == 1) {
//#if UE_DEBUG_TRACE
    LOG_I(PHY,"[UE %d] THREAD 0 DLSCH: Setting NAK for SFN/SF %d/%d (pid %d, status %d, round %d, TBS %d, mcs %d) Kr %d r %d harq_process->round %d\n",
        phy_vars_ue->Mod_id, frame, subframe, harq_pid,harq_process->status, harq_process->round,harq_process->TBS,harq_process->mcs,Kr,r,harq_process->round);
//#endif
    dlsch->harq_ack[subframe].ack = 0;
    dlsch->harq_ack[subframe].harq_id = harq_pid;
    dlsch->harq_ack[subframe].send_harq_status = 1;
    harq_process->errors[harq_process->round]++;
    harq_process->round++;


    //    printf("Rate: [UE %d] DLSCH: Setting NACK for subframe %d (pid %d, round %d)\n",phy_vars_ue->Mod_id,subframe,harq_pid,harq_process->round);
    if (harq_process->round >= dlsch->Mdlharq) {
      harq_process->status = SCH_IDLE;
      harq_process->round  = 0;
    }
/*    if(is_crnti)
    {
    LOG_D(PHY,"[UE %d] DLSCH: Setting NACK for subframe %d (pid %d, pid status %d, round %d/Max %d, TBS %d)\n",
               phy_vars_ue->Mod_id,subframe,harq_pid,harq_process->status,harq_process->round,dlsch->Mdlharq,harq_process->TBS);
    }*/


    //return((1+dlsch->max_ldpc_iterations));
  } else {
#if UE_DEBUG_TRACE
      LOG_I(PHY,"[UE %d] THREAD 0 DLSCH: Setting ACK for subframe %d TBS %d mcs %d nb_rb %d\n",
           phy_vars_ue->Mod_id,subframe,harq_process->TBS,harq_process->mcs,harq_process->nb_rb);
#endif

    harq_process->status = SCH_IDLE;
    harq_process->round  = 0;
    dlsch->harq_ack[subframe].ack = 1;
    dlsch->harq_ack[subframe].harq_id = harq_pid;
    dlsch->harq_ack[subframe].send_harq_status = 1;
    //LOG_I(PHY,"[UE %d] DLSCH: Setting ACK for SFN/SF %d/%d (pid %d, status %d, round %d, TBS %d, mcs %d)\n",
      //  phy_vars_ue->Mod_id, frame, subframe, harq_pid, harq_process->status, harq_process->round,harq_process->TBS,harq_process->mcs);

/*    if(is_crnti)
    {
    LOG_D(PHY,"[UE %d] DLSCH: Setting ACK for subframe %d (pid %d, round %d, TBS %d)\n",phy_vars_ue->Mod_id,subframe,harq_pid,harq_process->round,harq_process->TBS);
    }
    LOG_D(PHY,"[UE %d] DLSCH: Setting ACK for subframe %d (pid %d, round %d)\n",phy_vars_ue->Mod_id,subframe,harq_pid,harq_process->round);

  }*/

  // Reassembly of Transport block here
  offset = 0;

  /*
  printf("harq_pid %d\n",harq_pid);
  printf("F %d, Fbytes %d\n",harq_process->F,harq_process->F>>3);
  printf("C %d\n",harq_process->C);
  */
  for (r=0; r<harq_process->C; r++) {
    if (r<harq_process->Cminus)
      Kr = harq_process->Kminus;
    else
      Kr = harq_process->Kplus;

    Kr_bytes = Kr>>3;

    //    printf("Segment %d : Kr= %d bytes\n",r,Kr_bytes);
    if (r==0) {
      memcpy(harq_process->b,
             &harq_process->c[0][(harq_process->F>>3)],
             Kr_bytes - (harq_process->F>>3)- ((harq_process->C>1)?3:0));
      offset = Kr_bytes - (harq_process->F>>3) - ((harq_process->C>1)?3:0);
      //            printf("copied %d bytes to b sequence (harq_pid %d)\n",
      //          Kr_bytes - (harq_process->F>>3),harq_pid);
      //          printf("b[0] = %x,c[%d] = %x\n",
      //      harq_process->b[0],
      //      harq_process->F>>3,
      //      harq_process->c[0][(harq_process->F>>3)]);
    } else {
      memcpy(harq_process->b+offset,
             harq_process->c[r],
             Kr_bytes- ((harq_process->C>1)?3:0));
      offset += (Kr_bytes - ((harq_process->C>1)?3:0));
    }
  }

  dlsch->last_iteration_cnt = ret;

  //return(ret);
  }
#endif

  proc->decoder_thread_available = 1;
  //proc->decoder_main_available = 0;
  
  if (pthread_mutex_lock(&proc->mutex_dlsch_td) != 0) {
              LOG_E( PHY, "[SCHED][UE] error locking mutex for UE RXTX\n" );
              exit_fun("noting to add");
          }
          proc->instance_cnt_dlsch_td--;
          if (pthread_mutex_unlock(&proc->mutex_dlsch_td) != 0) {
              LOG_E( PHY, "[SCHED][UE] error unlocking mutex for UE td1\n" );
              exit_fun("noting to add");
          }
      }

      // thread finished
          free(arg);
          return &UE_dlsch_td_retval;
}
#endif

#ifdef UE_DLSCH_PARALLELISATION
#define FIFO_PRIORITY   39
uint32_t  dlsch_decoding_2thread1(void *arg)
{
	static __thread int UE_dlsch_td_retval1;
    struct rx_tx_thread_data *rtd = arg;
    UE_rxtx_proc_t *proc = rtd->proc;
    PHY_VARS_NR_UE    *phy_vars_ue   = rtd->UE;

    int llr8_flag1;
    int32_t no_iteration_ldpc;
    t_nrLDPC_dec_params decParams;
    t_nrLDPC_dec_params* p_decParams = &decParams;
    t_nrLDPC_time_stats procTime;
    t_nrLDPC_time_stats* p_procTime =&procTime ;
    int16_t z [68*384];
    int8_t l [68*384];
    //__m128i l;
    int16_t inv_d [68*384];
    int16_t *p_invd =&inv_d;

    uint32_t i,j;
    uint32_t k;

    __m128i *pv = (__m128i*)&z;
    __m128i *pl = (__m128i*)&l;

    proc->instance_cnt_dlsch_td1=-1;
    proc->nr_tti_rx=proc->sub_frame_start;

    printf("start thread 1\n");
    proc->decoder_thread_available1 = 0;

    char threadname[256];
    sprintf(threadname,"UE_thread_dlsch_td1_%d", proc->sub_frame_start);

	cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    if ( (proc->sub_frame_start+1)%RX_NB_TH == 0 && threads.dlsch_td_one != -1 )
    	CPU_SET(threads.dlsch_td_one, &cpuset);
    if ( (proc->sub_frame_start+1)%RX_NB_TH == 1 && threads.dlsch_td_two != -1 )
    	CPU_SET(threads.dlsch_td_two, &cpuset);
    if ( (proc->sub_frame_start+1)%RX_NB_TH == 2 && threads.dlsch_td_three != -1 )
    	CPU_SET(threads.dlsch_td_three, &cpuset);



#if UE_TIMING_TRACE
  time_stats_t *dlsch_rate_unmatching_stats=&phy_vars_ue->dlsch_rate_unmatching_stats;
  time_stats_t *dlsch_turbo_decoding_stats=&phy_vars_ue->dlsch_turbo_decoding_stats;
  time_stats_t *dlsch_deinterleaving_stats=&phy_vars_ue->dlsch_deinterleaving_stats;
#endif
  uint32_t A,E;
  uint32_t G;
  uint32_t ret,offset;
  uint16_t iind;
  //  uint8_t dummy_channel_output[(3*8*block_length)+12];
  short dummy_w[MAX_NUM_DLSCH_SEGMENTS][3*(8448+64)];
  uint32_t r,r_offset=0,Kr,Kr_bytes,err_flag=0,Kr_int;
  uint8_t crc_type;
  uint8_t C;
   uint8_t Qm;
   uint8_t Nl;
   uint32_t Er, Gp,GpmodC;
#ifdef DEBUG_DLSCH_DECODING
  uint16_t i;
#endif
  //#ifdef __AVX2__

  uint8_t (*tc)(int16_t *y,
                uint8_t *,
                uint16_t,
                uint16_t,
                uint16_t,
                uint8_t,
                uint8_t,
                uint8_t,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *,
                time_stats_t *);
  if (llr8_flag1 == 0) {
    //#ifdef __AVX2__
#if 0
    tc_2cw = phy_threegpplte_turbo_decoder16avx2;
#endif
    tc = phy_threegpplte_turbo_decoder16;
  }
  else
  {
	  //AssertFatal (harq_process->TBS >= 256 , "Mismatch flag nbRB=%d TBS=%d mcs=%d Qm=%d RIV=%d round=%d \n",
	  //		  harq_process->nb_rb, harq_process->TBS,harq_process->mcs,harq_process->Qm,harq_process->rvidx,harq_process->round);
	    tc = phy_threegpplte_turbo_decoder8;
  }

	init_thread(900000,1000000 , FIFO_PRIORITY-1, &cpuset, threadname);
	printf("2thread1 oai_exit %d\n", oai_exit);
	while (!oai_exit) {
	        if (pthread_mutex_lock(&proc->mutex_dlsch_td1) != 0) {
	            LOG_E( PHY, "[SCHED][UE] error locking mutex for UE dlsch td\n" );
	            exit_fun("nothing to add");
	        }
	        while (proc->instance_cnt_dlsch_td1 < 0) {
	            // most of the time, the thread is waiting here
	            pthread_cond_wait( &proc->cond_dlsch_td1, &proc->mutex_dlsch_td1 );
	        }
	        if (pthread_mutex_unlock(&proc->mutex_dlsch_td1) != 0) {
	            LOG_E( PHY, "[SCHED][UE] error unlocking mutex for UE dlsch_td \n" );
	            exit_fun("nothing to add");
	        }

	        //printf("2thread1 main available %d\n", proc->decoder_main_available);

	        uint32_t wait = 0;
	        	          while(proc->decoder_main_available == 0)
	        	          {
	        	                  usleep(1);
	        	                  wait++;
	        	          }
	        	
	        	          //proc->decoder_thread_available1 = 0;

	        //PHY_VARS_NR_UE *phy_vars_ue   		= tdp->UE;
	        	int eNB_id         				= proc->eNB_id;
	        	int harq_pid      				= proc->harq_pid;
	        	llr8_flag1     					= proc->llr8_flag;
	        	//r_offset						= proc->Er;
	        	//UE_rxtx_proc_t *proc    		= tdp->proc;
	        	int frame                       = proc->frame_rx;
	        	int subframe      				= proc->nr_tti_rx;
	        	NR_UE_DLSCH_t *dlsch 			= phy_vars_ue->dlsch[phy_vars_ue->current_thread_id[subframe]][eNB_id][0];
	        	NR_DL_UE_HARQ_t *harq_process  = dlsch->harq_processes[harq_pid];
	        	short *dlsch_llr 				= phy_vars_ue->pdsch_vars[phy_vars_ue->current_thread_id[subframe]][eNB_id]->llr[0];
	        	//printf("2thread0 llr flag %d tdp flag %d\n",llr8_flag1, tdp->llr8_flag);
	        	//printf("2thread1 nr_tti_tx %d subframe %d SF thread id %d r_offset %d\n", proc->nr_tti_rx, subframe, phy_vars_ue->current_thread_id[subframe], r_offset);

  //  nb_rb = dlsch->nb_rb;

  /*
  if (nb_rb > frame_parms->N_RB_DL) {
    printf("dlsch_decoding.c: Illegal nb_rb %d\n",nb_rb);
    return(max_ldpc_iterations);
    }*/

  /*harq_pid = dlsch->current_harq_pid[phy_vars_ue->current_thread_id[subframe]];
  if (harq_pid >= 8) {
    printf("dlsch_decoding.c: Illegal harq_pid %d\n",harq_pid);
    return(max_ldpc_iterations);
  }
  */



  harq_process->trials[harq_process->round]++;

  A = harq_process->TBS; //2072 for QPSK 1/3

  ret = dlsch->max_ldpc_iterations;


  G = harq_process->G;
  //get_G(frame_parms,nb_rb,dlsch->rb_alloc,mod_order,num_pdcch_symbols,phy_vars_ue->frame,subframe);

    //printf("DLSCH Decoding,  A %d harq_pid %d G %d\n",A, harq_pid,harq_process->G);

  if (harq_process->round == 0) {
    // This is a new packet, so compute quantities regarding segmentation
    harq_process->B = A+24;
#ifdef TD_DECODING
      lte_segmentation(NULL,
                       NULL,
                       harq_process->B,
                       &harq_process->C,
                       &harq_process->Cplus,
                       &harq_process->Cminus,
                       &harq_process->Kplus,
                       &harq_process->Kminus,
                       &harq_process->F);
      //  CLEAR LLR's HERE for first packet in process
  #else
      nr_segmentation(NULL,
      	                    NULL,
      	                    harq_process->B,
      	                    &harq_process->C,
      	                    &harq_process->Kplus,
      	                    &harq_process->Kminus,
      						&harq_process->Z,
      	                    &harq_process->F);
      	p_decParams->Z = harq_process->Z;      	

  #endif

    }

    //	  p_decParams->Z = 128;
        p_decParams->BG = 1;
        p_decParams->R = 89;
        p_decParams->numMaxIter = 2;
        Kr = p_decParams->Z*22;
        p_decParams->outMode= 0;
  /*
  else {
    printf("dlsch_decoding.c: Ndi>0 not checked yet!!\n");
    return(max_ldpc_iterations);
  }
  */
  err_flag = 0;
  //r_offset = 0;

  /*
  unsigned char bw_scaling =1;

  switch (frame_parms->N_RB_DL) {
  case 6:
    bw_scaling =16;
    break;

  case 25:
    bw_scaling =4;
    break;

  case 50:
    bw_scaling =2;
    break;

  default:
    bw_scaling =1;
    break;
  }

  if (harq_process->C > MAX_NUM_DLSCH_SEGMENTS/bw_scaling) {
    LOG_E(PHY,"Illegal harq_process->C %d > %d\n",harq_process->C,MAX_NUM_DLSCH_SEGMENTS/bw_scaling);
    return((1+dlsch->max_ldpc_iterations));
  }*/
#ifdef DEBUG_DLSCH_DECODING
  printf("Segmentation: C %d, Cminus %d, Kminus %d, Kplus %d\n",harq_process->C,harq_process->Cminus,harq_process->Kminus,harq_process->Kplus);
#endif

  opp_enabled=1;
  
  Qm= harq_process->Qm;
  	       Nl=harq_process->Nl;
  	       //r_thread = harq_process->C/2-1;
  	       C= harq_process->C;

  	       Gp = G/Nl/Qm;
  	        GpmodC = Gp%C;



  	        if ((C/2-1) < (C-(GpmodC)))
  	        	r_offset = Nl*Qm * (Gp/C);
  	        else
  	        	r_offset = Nl*Qm * ((GpmodC==0?0:1) + (Gp/C));

  	       //printf("sub thread r_offset %d\n", r_offset);

  //for (r=(harq_process->C/2); r<harq_process->C; r++) {
  	     r=2; //(harq_process->C/2);
  	   r_offset = r*r_offset;
	 //printf("thread1 r=%d r_offset %d \n",r, r_offset);


    // Get Turbo interleaver parameters
#ifdef TD_DECODING
	  if (r<harq_process->Cminus)
      Kr = harq_process->Kminus;
    else
      Kr = harq_process->Kplus;
#else
	  Kr = harq_process->Kplus;
#endif

    Kr_bytes = Kr>>3;

     //workaround for nr ldpc using lte interleaving
  if (dlsch->harq_processes[harq_pid]->C >= 2)
      	Kr_int = G/(3*dlsch->harq_processes[harq_pid]->C);
  else
  	Kr_int = Kr;

    if (Kr_bytes<=64)
      iind = (Kr_bytes-5);
    else if (Kr_bytes <=128)
      iind = 59 + ((Kr_bytes-64)>>1);
    else if (Kr_bytes <= 256)
      iind = 91 + ((Kr_bytes-128)>>2);
    else if (Kr_bytes <= 768)
      iind = 123 + ((Kr_bytes-256)>>3);
    else {
      //printf("dlsch_decoding: Illegal codeword size %d!!!\n",Kr_bytes);
      //return(dlsch->max_ldpc_iterations);
    }

#ifdef DEBUG_DLSCH_DECODING
    printf("f1 %d, f2 %d, F %d\n",f1f2mat_old[2*iind],f1f2mat_old[1+(2*iind)],(r==0) ? harq_process->F : 0);
#endif

#if UE_TIMING_TRACE
    start_meas(dlsch_rate_unmatching_stats);
#endif
    memset(&dummy_w[r][0],0,3*(8448+64)*sizeof(short));
    harq_process->RTC[r] = generate_dummy_w(Kr_int,
                                            (uint8_t*) &dummy_w[r][0],
                                            (r==0) ? harq_process->F : 0);

#ifdef DEBUG_DLSCH_DECODING
    LOG_D(PHY,"HARQ_PID %d Rate Matching Segment %d (coded bits %d,unpunctured/repeated bits %d, TBS %d, mod_order %d, nb_rb %d, Nl %d, rv %d, round %d)...\n",
          harq_pid,r, G,
          Kr*3,
          harq_process->TBS,
          harq_process->Qm,
          harq_process->nb_rb,
          harq_process->Nl,
          harq_process->rvidx,
          harq_process->round);
#endif

#ifdef DEBUG_DLSCH_DECODING
    printf(" in decoding dlsch->harq_processes[harq_pid]->rvidx = %d\n", dlsch->harq_processes[harq_pid]->rvidx);
#endif
    if (lte_rate_matching_turbo_rx(harq_process->RTC[r],
                                   G,
                                   harq_process->w[r],
                                   (uint8_t*)&dummy_w[r][0],
                                   dlsch_llr+r_offset,
                                   harq_process->C,
                                   dlsch->Nsoft,
                                   dlsch->Mdlharq,
                                   dlsch->Kmimo,
                                   harq_process->rvidx,
                                   (harq_process->round==0)?1:0,
                                   harq_process->Qm,
                                   harq_process->Nl,
                                   r,
                                   &E)==-1) {
#if UE_TIMING_TRACE
      stop_meas(dlsch_rate_unmatching_stats);
#endif
      LOG_E(PHY,"dlsch_decoding.c: Problem in rate_matching\n");
      //return(dlsch->max_ldpc_iterations);
    } else
    {
#if UE_TIMING_TRACE
      stop_meas(dlsch_rate_unmatching_stats);
#endif
    }
    r_offset += E;

    /*
    printf("Subblock deinterleaving, d %p w %p\n",
     harq_process->d[r],
     harq_process->w);
    */
#if UE_TIMING_TRACE
    start_meas(dlsch_deinterleaving_stats);
#endif
    sub_block_deinterleaving_turbo(4+Kr,
                                   &harq_process->d[r][96],

                                   harq_process->w[r]);
#if UE_TIMING_TRACE
    stop_meas(dlsch_deinterleaving_stats);
#endif
#ifdef DEBUG_DLSCH_DECODING
    /*
    if (r==0) {
              write_output("decoder_llr.m","decllr",dlsch_llr,G,1,0);
              write_output("decoder_in.m","dec",&harq_process->d[0][96],(3*8*Kr_bytes)+12,1,0);
    }

    printf("decoder input(segment %d) :",r);
    int i; for (i=0;i<(3*8*Kr_bytes)+12;i++)
      printf("%d : %d\n",i,harq_process->d[r][96+i]);
      printf("\n");*/
#endif


    //    printf("Clearing c, %p\n",harq_process->c[r]);
    memset(harq_process->c[r],0,Kr_bytes);

    //    printf("done\n");
    if (harq_process->C == 1)
      crc_type = CRC24_A;
    else
      crc_type = CRC24_B;

    /*
    printf("decoder input(segment %d)\n",r);
    for (i=0;i<(3*8*Kr_bytes)+12;i++)
      if ((harq_process->d[r][96+i]>7) ||
    (harq_process->d[r][96+i] < -8))
    printf("%d : %d\n",i,harq_process->d[r][96+i]);
    printf("\n");
    */

    //#ifndef __AVX2__
#if 1
    if (err_flag == 0) {
/*
        LOG_I(PHY, "turbo algo Kr=%d cb_cnt=%d C=%d nbRB=%d crc_type %d TBSInput=%d TBSHarq=%d TBSplus24=%d mcs=%d Qm=%d RIV=%d round=%d maxIter %d\n",
                            Kr,r,harq_process->C,harq_process->nb_rb,crc_type,A,harq_process->TBS,
                            harq_process->B,harq_process->mcs,harq_process->Qm,harq_process->rvidx,harq_process->round,dlsch->max_ldpc_iterations);
*/
    	if (llr8_flag1) {
    		AssertFatal (Kr >= 256, "turbo algo issue Kr=%d cb_cnt=%d C=%d nbRB=%d TBSInput=%d TBSHarq=%d TBSplus24=%d mcs=%d Qm=%d RIV=%d round=%d\n",
    				Kr,r,harq_process->C,harq_process->nb_rb,A,harq_process->TBS,harq_process->B,harq_process->mcs,harq_process->Qm,harq_process->rvidx,harq_process->round);
    	}
#if UE_TIMING_TRACE
        start_meas(dlsch_turbo_decoding_stats);
#endif
//      LOG_D(PHY,"AbsSubframe %d.%d Start turbo segment %d/%d \n",frame%1024,subframe,r,harq_process->C-1);
#ifdef TD_DECODING
        ret = tc
            (&harq_process->d[r][96],
             harq_process->c[r],
             Kr,
             f1f2mat_old[iind*2],
             f1f2mat_old[(iind*2)+1],
             dlsch->max_ldpc_iterations,
             crc_type,
             (r==0) ? harq_process->F : 0,
             &phy_vars_ue->dlsch_tc_init_stats,
             &phy_vars_ue->dlsch_tc_alpha_stats,
             &phy_vars_ue->dlsch_tc_beta_stats,
             &phy_vars_ue->dlsch_tc_gamma_stats,
             &phy_vars_ue->dlsch_tc_ext_stats,
             &phy_vars_ue->dlsch_tc_intl1_stats,
             &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);

#else

		memset(pv,0,2*p_decParams->Z*sizeof(int16_t));
        //memset(pl,0,2*p_decParams->Z*sizeof(int8_t));

	
      	for (i=2*p_decParams->Z/8, j = 0; i < (68*p_decParams->Z/8+1); i++, j++)
      	{
      		pv[i]= _mm_loadu_si128((__m128i*)(&inv_d[8*j]));
      	}
      	
		for (i=0, j=0; j < (68*p_decParams->Z/16);  i+=2, j++)
      	      	{
      				//printf("mm packs i %d j %d\n", i, j);
      				//print128_num(pv[i]);
      				//print128_num(pv[i+1]);
      	      		pl[j] = _mm_packs_epi16(pv[i],pv[i+1]);
      	      		//print128_num2bytes(pl[j]);
      	      	}

		no_iteration_ldpc = nrLDPC_decoder(p_decParams,
      			&pl[0],
				llrProcBuf,
          		p_procTime);

		if (no_iteration_ldpc > 2)
			printf("Error number of iteration LPDC %d\n", no_iteration_ldpc);
		//else
		//	printf("OK number of iteration LPDC %d\n", no_iteration_ldpc);

		for (int m=0; m < Kr>>3; m ++)
		      	      	{
		      				harq_process->c[r][m]= (uint8_t) llrProcBuf[m];
		      	      	}

		      	/*for (int u=0; u < Kr>>3; u ++)
		      	      	      	{
		      						ullrProcBuf[u]= (uint8_t) llrProcBuf[u];
		      	      	      	}


		      	printf("output unsigned ullrProcBuf \n");

		      	for (int j=0; j < Kr>>3; j ++)
		      	      	       	      	{

		      	      						printf(" %d \n", ullrProcBuf[j]);

		      	      	      	      	}
		     	printf(" \n");*/
#endif
		//printf("output channel decoder %d %d %d %d %d \n", harq_process->c[r][0], harq_process->c[r][1], harq_process->c[r][2],harq_process->c[r][3], harq_process->c[r][4]);

		 //printf("output decoder %d %d %d %d %d \n", harq_process->c[r][0], harq_process->c[r][1], harq_process->c[r][2],harq_process->c[r][3], harq_process->c[r][4]);

#if UE_TIMING_TRACE
      stop_meas(dlsch_turbo_decoding_stats);
#endif
    }
#else
    if ((harq_process->C == 1) ||
	((r==harq_process->C-1) && (skipped_last==0))) { // last segment with odd number of segments

#if UE_TIMING_TRACE
        start_meas(dlsch_turbo_decoding_stats);
#endif
      ret = tc
            (&harq_process->d[r][96],
             harq_process->c[r],
             Kr,
             f1f2mat_old[iind*2],
             f1f2mat_old[(iind*2)+1],
             dlsch->max_ldpc_iterations,
             crc_type,
             (r==0) ? harq_process->F : 0,
             &phy_vars_ue->dlsch_tc_init_stats,
             &phy_vars_ue->dlsch_tc_alpha_stats,
             &phy_vars_ue->dlsch_tc_beta_stats,
             &phy_vars_ue->dlsch_tc_gamma_stats,
             &phy_vars_ue->dlsch_tc_ext_stats,
             &phy_vars_ue->dlsch_tc_intl1_stats,
             &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);
 #if UE_TIMING_TRACE
      stop_meas(dlsch_turbo_decoding_stats);
#endif
      //      printf("single decode, exit\n");
      //      exit(-1);
    }
    else {
    // we can merge code segments
      if ((skipped_last == 0) && (r<harq_process->C-1)) {
	skipped_last = 1;
	Kr_last = Kr;
      }
      else {
	skipped_last=0;

	if (Kr_last == Kr) { // decode 2 code segments with AVX2 version
#ifdef DEBUG_DLSCH_DECODING
	  printf("single decoding segment %d (%p)\n",r-1,&harq_process->d[r-1][96]);
#endif
#if UE_TIMING_TRACE
	  start_meas(dlsch_turbo_decoding_stats);
#endif
#ifdef DEBUG_DLSCH_DECODING
	  printf("double decoding segments %d,%d (%p,%p)\n",r-1,r,&harq_process->d[r-1][96],&harq_process->d[r][96]);
#endif
	  ret = tc_2cw
            (&harq_process->d[r-1][96],
	     &harq_process->d[r][96],
             harq_process->c[r-1],
             harq_process->c[r],
             Kr,
             f1f2mat_old[iind*2],
             f1f2mat_old[(iind*2)+1],
             dlsch->max_ldpc_iterations,
             crc_type,
             (r==0) ? harq_process->F : 0,
             &phy_vars_ue->dlsch_tc_init_stats,
             &phy_vars_ue->dlsch_tc_alpha_stats,
             &phy_vars_ue->dlsch_tc_beta_stats,
             &phy_vars_ue->dlsch_tc_gamma_stats,
             &phy_vars_ue->dlsch_tc_ext_stats,
             &phy_vars_ue->dlsch_tc_intl1_stats,
             &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);
	  /*
	  ret = tc
            (&harq_process->d[r-1][96],
             harq_process->c[r-1],
             Kr_last,
             f1f2mat_old[iind*2],
             f1f2mat_old[(iind*2)+1],
             dlsch->max_ldpc_iterations,
             crc_type,
             (r==0) ? harq_process->F : 0,
             &phy_vars_ue->dlsch_tc_init_stats,
             &phy_vars_ue->dlsch_tc_alpha_stats,
             &phy_vars_ue->dlsch_tc_beta_stats,
             &phy_vars_ue->dlsch_tc_gamma_stats,
             &phy_vars_ue->dlsch_tc_ext_stats,
             &phy_vars_ue->dlsch_tc_intl1_stats,
             &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);

	     exit(-1);*/
#if UE_TIMING_TRACE
      stop_meas(dlsch_turbo_decoding_stats);
#endif
	}
	else { // Kr_last != Kr
#if UE_TIMING_TRACE
	  start_meas(dlsch_turbo_decoding_stats);
#endif
	  ret = tc
            (&harq_process->d[r-1][96],
             harq_process->c[r-1],
             Kr_last,
             f1f2mat_old[iind*2],
             f1f2mat_old[(iind*2)+1],
             dlsch->max_ldpc_iterations,
             crc_type,
             (r==0) ? harq_process->F : 0,
             &phy_vars_ue->dlsch_tc_init_stats,
             &phy_vars_ue->dlsch_tc_alpha_stats,
             &phy_vars_ue->dlsch_tc_beta_stats,
             &phy_vars_ue->dlsch_tc_gamma_stats,
             &phy_vars_ue->dlsch_tc_ext_stats,
             &phy_vars_ue->dlsch_tc_intl1_stats,
             &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);
#if UE_TIMING_TRACE
      stop_meas(dlsch_turbo_decoding_stats);

	  start_meas(dlsch_turbo_decoding_stats);
#endif

	  ret = tc
            (&harq_process->d[r][96],
             harq_process->c[r],
             Kr,
             f1f2mat_old[iind*2],
             f1f2mat_old[(iind*2)+1],
             dlsch->max_ldpc_iterations,
             crc_type,
             (r==0) ? harq_process->F : 0,
             &phy_vars_ue->dlsch_tc_init_stats,
             &phy_vars_ue->dlsch_tc_alpha_stats,
             &phy_vars_ue->dlsch_tc_beta_stats,
             &phy_vars_ue->dlsch_tc_gamma_stats,
             &phy_vars_ue->dlsch_tc_ext_stats,
             &phy_vars_ue->dlsch_tc_intl1_stats,
             &phy_vars_ue->dlsch_tc_intl2_stats); //(is_crnti==0)?harq_pid:harq_pid+1);

#if UE_TIMING_TRACE

	  stop_meas(dlsch_turbo_decoding_stats);

	  /*printf("Segmentation: C %d r %d, dlsch_rate_unmatching_stats %5.3f dlsch_deinterleaving_stats %5.3f  dlsch_turbo_decoding_stats %5.3f \n",
              harq_process->C,
              r,
              dlsch_rate_unmatching_stats->p_time/(cpuf*1000.0),
              dlsch_deinterleaving_stats->p_time/(cpuf*1000.0),
              dlsch_turbo_decoding_stats->p_time/(cpuf*1000.0));*/
#endif
	}
      }
    }
#endif


    if ((err_flag == 0) && (ret>=(1+dlsch->max_ldpc_iterations))) {// a Code segment is in error so break;
//      LOG_D(PHY,"AbsSubframe %d.%d CRC failed, segment %d/%d \n",frame%1024,subframe,r,harq_process->C-1);
      err_flag = 1;
    }
  //}

  /*int32_t frame_rx_prev = frame;
  int32_t subframe_rx_prev = subframe - 1;
  if (subframe_rx_prev < 0) {
    frame_rx_prev--;
    subframe_rx_prev += 10;
  }
  frame_rx_prev = frame_rx_prev%1024;*/
#if 0
  if (err_flag == 1) {
//#if UE_DEBUG_TRACE
    LOG_I(PHY,"[UE %d] THREAD 0 DLSCH: Setting NAK for SFN/SF %d/%d (pid %d, status %d, round %d, TBS %d, mcs %d) Kr %d r %d harq_process->round %d\n",
        phy_vars_ue->Mod_id, frame, subframe, harq_pid,harq_process->status, harq_process->round,harq_process->TBS,harq_process->mcs,Kr,r,harq_process->round);
//#endif
    dlsch->harq_ack[subframe].ack = 0;
    dlsch->harq_ack[subframe].harq_id = harq_pid;
    dlsch->harq_ack[subframe].send_harq_status = 1;
    harq_process->errors[harq_process->round]++;
    harq_process->round++;


    //    printf("Rate: [UE %d] DLSCH: Setting NACK for subframe %d (pid %d, round %d)\n",phy_vars_ue->Mod_id,subframe,harq_pid,harq_process->round);
    if (harq_process->round >= dlsch->Mdlharq) {
      harq_process->status = SCH_IDLE;
      harq_process->round  = 0;
    }
/*    if(is_crnti)
    {
    LOG_D(PHY,"[UE %d] DLSCH: Setting NACK for subframe %d (pid %d, pid status %d, round %d/Max %d, TBS %d)\n",
               phy_vars_ue->Mod_id,subframe,harq_pid,harq_process->status,harq_process->round,dlsch->Mdlharq,harq_process->TBS);
    }*/


    //return((1+dlsch->max_ldpc_iterations));
  } else {
#if UE_DEBUG_TRACE
      LOG_I(PHY,"[UE %d] THREAD 0 DLSCH: Setting ACK for subframe %d TBS %d mcs %d nb_rb %d\n",
           phy_vars_ue->Mod_id,subframe,harq_process->TBS,harq_process->mcs,harq_process->nb_rb);
#endif

    harq_process->status = SCH_IDLE;
    harq_process->round  = 0;
    dlsch->harq_ack[subframe].ack = 1;
    dlsch->harq_ack[subframe].harq_id = harq_pid;
    dlsch->harq_ack[subframe].send_harq_status = 1;
    //LOG_I(PHY,"[UE %d] DLSCH: Setting ACK for SFN/SF %d/%d (pid %d, status %d, round %d, TBS %d, mcs %d)\n",
      //  phy_vars_ue->Mod_id, frame, subframe, harq_pid, harq_process->status, harq_process->round,harq_process->TBS,harq_process->mcs);

/*    if(is_crnti)
    {
    LOG_D(PHY,"[UE %d] DLSCH: Setting ACK for subframe %d (pid %d, round %d, TBS %d)\n",phy_vars_ue->Mod_id,subframe,harq_pid,harq_process->round,harq_process->TBS);
    }
    LOG_D(PHY,"[UE %d] DLSCH: Setting ACK for subframe %d (pid %d, round %d)\n",phy_vars_ue->Mod_id,subframe,harq_pid,harq_process->round);

  }*/

  // Reassembly of Transport block here
  offset = 0;

  /*
  printf("harq_pid %d\n",harq_pid);
  printf("F %d, Fbytes %d\n",harq_process->F,harq_process->F>>3);
  printf("C %d\n",harq_process->C);
  */
  for (r=0; r<harq_process->C; r++) {
    if (r<harq_process->Cminus)
      Kr = harq_process->Kminus;
    else
      Kr = harq_process->Kplus;

    Kr_bytes = Kr>>3;

    //    printf("Segment %d : Kr= %d bytes\n",r,Kr_bytes);
    if (r==0) {
      memcpy(harq_process->b,
             &harq_process->c[0][(harq_process->F>>3)],
             Kr_bytes - (harq_process->F>>3)- ((harq_process->C>1)?3:0));
      offset = Kr_bytes - (harq_process->F>>3) - ((harq_process->C>1)?3:0);
      //            printf("copied %d bytes to b sequence (harq_pid %d)\n",
      //          Kr_bytes - (harq_process->F>>3),harq_pid);
      //          printf("b[0] = %x,c[%d] = %x\n",
      //      harq_process->b[0],
      //      harq_process->F>>3,
      //      harq_process->c[0][(harq_process->F>>3)]);
    } else {
      memcpy(harq_process->b+offset,
             harq_process->c[r],
             Kr_bytes- ((harq_process->C>1)?3:0));
      offset += (Kr_bytes - ((harq_process->C>1)?3:0));
    }
  }

  dlsch->last_iteration_cnt = ret;

  //return(ret);
  }
#endif

  proc->decoder_thread_available1 = 1;
  //proc->decoder_main_available = 0;

  //printf("2thread1 proc->instance_cnt_dlsch_td1 %d\n", proc->instance_cnt_dlsch_td1);

  if (pthread_mutex_lock(&proc->mutex_dlsch_td1) != 0) {
              LOG_E( PHY, "[SCHED][UE] error locking mutex for UE RXTX\n" );
              exit_fun("noting to add");
          }
          proc->instance_cnt_dlsch_td1--;
          if (pthread_mutex_unlock(&proc->mutex_dlsch_td1) != 0) {
              LOG_E( PHY, "[SCHED][UE] error unlocking mutex for UE td1\n" );
              exit_fun("noting to add");
          }
          //printf("end 2thread1 proc->instance_cnt_dlsch_td1 %d\n", proc->instance_cnt_dlsch_td1);
      }

	//printf("after 2thread1 after oai exit proc->instance_cnt_dlsch_td %d\n", proc->instance_cnt_dlsch_td1);
      // thread finished
          free(arg);
          return &UE_dlsch_td_retval1;
}
#endif

