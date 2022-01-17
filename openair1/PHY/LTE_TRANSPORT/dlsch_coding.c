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

/*! \file PHY/LTE_TRANSPORT/dlsch_coding.c
 * \brief Top-level routines for implementing Turbo-coded (DLSCH) transport channels from 36-212, V8.6 2009-03, V14.1 2017 (includes FeMBMS support)
 * \author R. Knopp, J. Morgade
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr, jaiver.morgade@ieee.org
 * \note
 * \warning
 */

#include "PHY/defs_eNB.h"
#include "PHY/phy_extern.h"
#include "PHY/CODING/coding_defs.h"
#include "PHY/CODING/coding_extern.h"
#include "PHY/CODING/lte_interleaver_inline.h"
#include "PHY/LTE_TRANSPORT/transport_eNB.h"
#include "PHY/LTE_TRANSPORT/transport_proto.h"
#include "SCHED/sched_eNB.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "common/utils/LOG/log.h"
#include "targets/RT/USER/lte-softmodem.h"
#include <syscall.h>
#include <common/utils/threadPool/thread-pool.h>

//#define DEBUG_DLSCH_CODING
//#define DEBUG_DLSCH_FREE 1

/*
  #define is_not_pilot(pilots,first_pilot,re) (pilots==0) || \
  ((pilots==1)&&(first_pilot==1)&&(((re>2)&&(re<6))||((re>8)&&(re<12)))) || \
  ((pilots==1)&&(first_pilot==0)&&(((re<3))||((re>5)&&(re<9)))) \
*/
#define is_not_pilot(pilots,first_pilot,re) (1)
/*extern void thread_top_init(char *thread_name,
  int affinity,
  uint64_t runtime,
  uint64_t deadline,
  uint64_t period);*/

extern volatile int oai_exit;

void free_eNB_dlsch(LTE_eNB_DLSCH_t *dlsch) {
  int i, r, aa, layer;

  if (dlsch) {
    for (layer=0; layer<4; layer++) {
      for (aa=0; aa<64; aa++) free16(dlsch->ue_spec_bf_weights[layer][aa], OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES*sizeof(int32_t));

      free16(dlsch->ue_spec_bf_weights[layer], 64*sizeof(int32_t *));
    }

    for (i=0; i<dlsch->Mdlharq; i++) {
      if (dlsch->harq_processes[i]) {
        if (dlsch->harq_processes[i]->b) {
          free16(dlsch->harq_processes[i]->b,MAX_DLSCH_PAYLOAD_BYTES);
          dlsch->harq_processes[i]->b = NULL;
        }

        for (r=0; r<MAX_NUM_DLSCH_SEGMENTS; r++) {
          if (dlsch->harq_processes[i]->c[r]) {
            free16(dlsch->harq_processes[i]->c[r],((r==0)?8:0) + 3+768);
            dlsch->harq_processes[i]->c[r] = NULL;
          }
        }

        free16(dlsch->harq_processes[i],sizeof(LTE_DL_eNB_HARQ_t));
        dlsch->harq_processes[i] = NULL;
      }
    }

    free16(dlsch,sizeof(LTE_eNB_DLSCH_t));
  }
}



LTE_eNB_DLSCH_t *new_eNB_dlsch(unsigned char Kmimo,
                               unsigned char Mdlharq,
                               uint32_t Nsoft,
                               unsigned char N_RB_DL,
                               uint8_t abstraction_flag,
                               LTE_DL_FRAME_PARMS *frame_parms)
{
  LTE_eNB_DLSCH_t *dlsch;
  unsigned char exit_flag = 0,i,r,aa,layer;
  int re;
  unsigned char bw_scaling =1;

  switch (N_RB_DL) {
  case 6:
    bw_scaling = 4;
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

  dlsch = (LTE_eNB_DLSCH_t *)malloc16(sizeof(LTE_eNB_DLSCH_t));

  if (dlsch) {
    bzero(dlsch,sizeof(LTE_eNB_DLSCH_t));
    dlsch->Kmimo = Kmimo;
    dlsch->Mdlharq = Mdlharq;
    dlsch->Mlimit = 8;
    dlsch->Nsoft = Nsoft;

    for (layer=0; layer<4; layer++) {
      dlsch->ue_spec_bf_weights[layer] = (int32_t **)malloc16(64*sizeof(int32_t *));

      for (aa=0; aa<64; aa++) {
        dlsch->ue_spec_bf_weights[layer][aa] = (int32_t *)malloc16(OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES*sizeof(int32_t));

        for (re=0; re<OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES; re++) {
          dlsch->ue_spec_bf_weights[layer][aa][re] = 0x00007fff;
        }
      }
    }

    // NOTE: THIS HAS TO BE REVISED FOR RU, commenting to remove memory leak !!!!!
    /*
      dlsch->calib_dl_ch_estimates = (int32_t**)malloc16(frame_parms->nb_antennas_tx*sizeof(int32_t*));
      for (aa=0; aa<frame_parms->nb_antennas_tx; aa++) {
      dlsch->calib_dl_ch_estimates[aa] = (int32_t *)malloc16(OFDM_SYMBOL_SIZE_COMPLEX_SAMPLES*sizeof(int32_t));

      }*/

    for (i=0; i<20; i++)
      dlsch->harq_ids[i/10][i%10] = Mdlharq;

    for (i=0; i<Mdlharq; i++) {
      dlsch->harq_processes[i] = (LTE_DL_eNB_HARQ_t *)malloc16(sizeof(LTE_DL_eNB_HARQ_t));
      LOG_I(PHY, "Required DLSCH mem size %d (bw scaling %d), dlsch->harq_processes[%d] %p\n",
            MAX_DLSCH_PAYLOAD_BYTES/bw_scaling,bw_scaling, i,dlsch->harq_processes[i]);

      if (dlsch->harq_processes[i]) {
        bzero(dlsch->harq_processes[i],sizeof(LTE_DL_eNB_HARQ_t));
        //    dlsch->harq_processes[i]->first_tx=1;
        dlsch->harq_processes[i]->b = (unsigned char *)malloc16(MAX_DLSCH_PAYLOAD_BYTES/bw_scaling);

        if (dlsch->harq_processes[i]->b) {
          memset(dlsch->harq_processes[i]->b,0,MAX_DLSCH_PAYLOAD_BYTES/bw_scaling);
        } else {
          AssertFatal(1==0,"Can't get b\n");
          exit_flag=1;
        }

        if (abstraction_flag==0) {
          for (r=0; r<MAX_NUM_DLSCH_SEGMENTS/bw_scaling; r++) {
            // account for filler in first segment and CRCs for multiple segment case
            dlsch->harq_processes[i]->c[r] = (uint8_t *)malloc16(((r==0)?8:0) + 3+ 768);

            if (dlsch->harq_processes[i]->c[r]) {
              memset(dlsch->harq_processes[i]->c[r],0,((r==0)?8:0) + 3+ 768);
            } else {
              AssertFatal(1==0,"Can't get c\n");
              exit_flag=2;
            }
          }
        }
      } else {
        AssertFatal(1==0,"Can't get harq_p %d\n",i);
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

  LOG_I(PHY,"new_eNB_dlsch exit flag %d, size of  %ld\n",
        exit_flag, sizeof(LTE_eNB_DLSCH_t));
  free_eNB_dlsch(dlsch);
  return(NULL);
}


void clean_eNb_dlsch(LTE_eNB_DLSCH_t *dlsch) {
  unsigned char Mdlharq;
  unsigned char i;

  if (dlsch) {
    Mdlharq = dlsch->Mdlharq;
    dlsch->rnti = 0;
#ifdef PHY_TX_THREAD

    for (i=0; i<10; i++)
      dlsch->active[i] = 0;

#else
    dlsch->active = 0;
#endif
    dlsch->harq_mask = 0;

    for (i=0; i<20; i++)
      dlsch->harq_ids[i/10][i%10] = Mdlharq;

    for (i=0; i<Mdlharq; i++) {
      if (dlsch->harq_processes[i]) {
        //  dlsch->harq_processes[i]->Ndi    = 0;
        dlsch->harq_processes[i]->status = 0;
        dlsch->harq_processes[i]->round  = 0;

      }
    }
  }
}

void *te_thread(void *param) {
  return(NULL);
}

int dlsch_encoding_all(PHY_VARS_eNB *eNB,
		       L1_rxtx_proc_t *proc,
                       unsigned char *a,
                       uint8_t num_pdcch_symbols,
                       LTE_eNB_DLSCH_t *dlsch,
                       int frame,
                       uint8_t subframe,
                       time_stats_t *rm_stats,
                       time_stats_t *te_stats,
                       time_stats_t *te_wait_stats,
                       time_stats_t *te_main_stats,
                       time_stats_t *te_wakeup_stats0,
                       time_stats_t *te_wakeup_stats1,
                       time_stats_t *i_stats) {
  uint8_t harq_pid = dlsch->harq_ids[frame%2][subframe];
  if(harq_pid >= dlsch->Mdlharq) {
    LOG_E(PHY,"dlsch_encoding_all illegal harq_pid %d\n", harq_pid);
    return(-1);
  }
  
  LOG_D(PHY,"B %d, harq_pid %d\n",
	dlsch->harq_processes[harq_pid]->B,
	dlsch->harq_ids[frame%2][subframe]);

  return dlsch_encoding(eNB,
			proc,
			a,
			num_pdcch_symbols,
			dlsch,
			frame,
			subframe,
			rm_stats,
			te_stats,
			i_stats);

}

static void TPencode(void * arg) {
  turboEncode_t * rdata=(turboEncode_t *) arg;
  unsigned char harq_pid = rdata->harq_pid;
  LTE_DL_eNB_HARQ_t *hadlsch=rdata->dlsch->harq_processes[harq_pid];
  
  if ( rdata-> round == 0) {
    uint8_t tmp[96+12+3+3*6144];
    memset(tmp,LTE_NULL, TURBO_SIMD_SOFTBITS);
    start_meas(rdata->te_stats);
    encoder(rdata->input,
	    rdata->Kr_bytes,
	    tmp+96,//&dlsch->harq_processes[harq_pid]->d[r][96],
	    rdata->filler);
    stop_meas(rdata->te_stats);
    start_meas(rdata->i_stats);
    hadlsch->RTC[rdata->r] =
      sub_block_interleaving_turbo(4+(rdata->Kr_bytes*8),
				   tmp+96,
				   hadlsch->w[rdata->r]);
    stop_meas(rdata->i_stats);
  }
  
  // Fill in the "e"-sequence from 36-212, V8.6 2009-03, p. 16-17 (for each "e") and concatenate the
  // outputs for each code segment, see Section 5.1.5 p.20
    start_meas(rdata->rm_stats);
  lte_rate_matching_turbo(hadlsch->RTC[rdata->r],
			  rdata->G,  //G
			  hadlsch->w[rdata->r],
			  hadlsch->eDL+rdata->r_offset,
			  hadlsch->C, // C
			  rdata->dlsch->Nsoft,                    // Nsoft,
			  rdata->dlsch->Mdlharq,
			  rdata->dlsch->Kmimo,
			  hadlsch->rvidx,
			  hadlsch->Qm,
			  hadlsch->Nl,
			  rdata->r,
			  hadlsch->nb_rb);
    stop_meas(rdata->rm_stats);
}

int dlsch_encoding(PHY_VARS_eNB *eNB,
                   L1_rxtx_proc_t *proc,
                   unsigned char *a,
                   uint8_t num_pdcch_symbols,
                   LTE_eNB_DLSCH_t *dlsch,
                   int frame,
                   uint8_t subframe,
                   time_stats_t *rm_stats,
                   time_stats_t *te_stats,
                   time_stats_t *i_stats) {
  LTE_DL_FRAME_PARMS *frame_parms = &eNB->frame_parms;
  unsigned char harq_pid = dlsch->harq_ids[frame%2][subframe];
  if((harq_pid < 0) || (harq_pid >= dlsch->Mdlharq)) {
    LOG_E(PHY,"dlsch_encoding illegal harq_pid %d %s:%d\n", harq_pid, __FILE__, __LINE__);
    return(-1);
  }

  LTE_DL_eNB_HARQ_t *hadlsch=dlsch->harq_processes[harq_pid];
  uint8_t beamforming_mode=0;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ENCODING, VCD_FUNCTION_IN);

  if(hadlsch->mimo_mode == TM7)
    beamforming_mode = 7;
  else if(hadlsch->mimo_mode == TM8)
    beamforming_mode = 8;
  else if(hadlsch->mimo_mode == TM9_10)
    beamforming_mode = 9;

  unsigned int G = get_G(frame_parms,hadlsch->nb_rb,
	    hadlsch->rb_alloc,
	    hadlsch->Qm, // mod order
	    hadlsch->Nl,
	    num_pdcch_symbols,
	    frame,subframe,beamforming_mode);

  proc->nbEncode=0;


  //  if (hadlsch->Ndi == 1) {  // this is a new packet
  if (hadlsch->round == 0) {  // this is a new packet
    // Add 24-bit crc (polynomial A) to payload
    unsigned int A=hadlsch->TBS; //6228;
    unsigned int crc = crc24a(a,
                 A)>>8;
    a[A>>3] = ((uint8_t *)&crc)[2];
    a[1+(A>>3)] = ((uint8_t *)&crc)[1];
    a[2+(A>>3)] = ((uint8_t *)&crc)[0];
    //    printf("CRC %x (A %d)\n",crc,A);
    hadlsch->B = A+24;
    //    hadlsch->b = a;
    memcpy(hadlsch->b,a,(A/8)+4);
    
    if (lte_segmentation(hadlsch->b,
                         hadlsch->c,
                         hadlsch->B,
                         &hadlsch->C,
                         &hadlsch->Cplus,
                         &hadlsch->Cminus,
                         &hadlsch->Kplus,
                         &hadlsch->Kminus,
                         &hadlsch->F)<0)
      return(-1);
  }
 
  for (int r=0, r_offset=0; r<hadlsch->C; r++) {
    
    union turboReqUnion id= {.s={dlsch->rnti,frame,subframe,r,0}};
    notifiedFIFO_elt_t *req=newNotifiedFIFO_elt(sizeof(turboEncode_t), id.p, proc->respEncode, TPencode);
    turboEncode_t * rdata=(turboEncode_t *) NotifiedFifoData(req);
    rdata->input=hadlsch->c[r];
    rdata->Kr_bytes= ( r<hadlsch->Cminus ? hadlsch->Kminus : hadlsch->Kplus) >>3;
    rdata->filler=(r==0) ? hadlsch->F : 0;
    rdata->r=r;
    rdata->harq_pid=harq_pid;
    rdata->dlsch=dlsch;
    rdata->rm_stats=rm_stats;
    rdata->te_stats=te_stats;
    rdata->i_stats=i_stats;
    rdata->round=hadlsch->round;
    rdata->r_offset=r_offset;
    rdata->G=G;
    
    if (  proc->threadPool->activated ) {
      pushTpool(proc->threadPool,req);
      proc->nbEncode++;
    } else {
      TPencode(rdata);
      delNotifiedFIFO_elt(req);
    }
    
    int Qm=hadlsch->Qm;
    int C=hadlsch->C;
    int Nl=hadlsch->Nl;
    int Gp = G/Nl/Qm;
    int GpmodC = Gp%C;
    if (r < (C-(GpmodC)))
      r_offset += Nl*Qm * (Gp/C);
    else
      r_offset += Nl*Qm * ((GpmodC==0?0:1) + (Gp/C));
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ENCODING, VCD_FUNCTION_OUT);
  return(0);
}

int dlsch_encoding_fembms_pmch(PHY_VARS_eNB *eNB,
                   L1_rxtx_proc_t *proc,
                   unsigned char *a,
                   uint8_t num_pdcch_symbols,
                   LTE_eNB_DLSCH_t *dlsch,
                   int frame,
                   uint8_t subframe,
                   time_stats_t *rm_stats,
                   time_stats_t *te_stats,
                   time_stats_t *i_stats) {
  LTE_DL_FRAME_PARMS *frame_parms = &eNB->frame_parms;
  unsigned char harq_pid = dlsch->harq_ids[frame%2][subframe];
  if((harq_pid < 0) || (harq_pid >= dlsch->Mdlharq)) {
    LOG_E(PHY,"dlsch_encoding illegal harq_pid %d %s:%d\n", harq_pid, __FILE__, __LINE__);
    return(-1);
  }

  LTE_DL_eNB_HARQ_t *hadlsch=dlsch->harq_processes[harq_pid];
  uint8_t beamforming_mode=0;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_ENCODING, VCD_FUNCTION_IN);

  if(hadlsch->mimo_mode == TM7)
    beamforming_mode = 7;
  else if(hadlsch->mimo_mode == TM8)
    beamforming_mode = 8;
  else if(hadlsch->mimo_mode == TM9_10)
    beamforming_mode = 9;

  unsigned int G = get_G_khz_1dot25(frame_parms,hadlsch->nb_rb,
	    hadlsch->rb_alloc,
	    hadlsch->Qm, // mod order
	    hadlsch->Nl,
	    num_pdcch_symbols,
	    frame,subframe,beamforming_mode);

  proc->nbEncode=0;

  //  if (hadlsch->Ndi == 1) {  // this is a new packet
  if (hadlsch->round == 0) {  // this is a new packet
    // Add 24-bit crc (polynomial A) to payload
    unsigned int A=hadlsch->TBS; //6228;
    unsigned int crc = crc24a(a,
                 A)>>8;
    a[A>>3] = ((uint8_t *)&crc)[2];
    a[1+(A>>3)] = ((uint8_t *)&crc)[1];
    a[2+(A>>3)] = ((uint8_t *)&crc)[0];
    //    printf("CRC %x (A %d)\n",crc,A);
    hadlsch->B = A+24;
    //    hadlsch->b = a;
    memcpy(hadlsch->b,a,(A/8)+4);
    
    if (lte_segmentation(hadlsch->b,
                         hadlsch->c,
                         hadlsch->B,
                         &hadlsch->C,
                         &hadlsch->Cplus,
                         &hadlsch->Cminus,
                         &hadlsch->Kplus,
                         &hadlsch->Kminus,
                         &hadlsch->F)<0)
      return(-1);
  }
 
  for (int r=0, r_offset=0; r<hadlsch->C; r++) {
    
    union turboReqUnion id= {.s={dlsch->rnti,frame,subframe,r,0}};
    notifiedFIFO_elt_t *req=newNotifiedFIFO_elt(sizeof(turboEncode_t), id.p, proc->respEncode, TPencode);
    turboEncode_t * rdata=(turboEncode_t *) NotifiedFifoData(req);
    rdata->input=hadlsch->c[r];
    rdata->Kr_bytes= ( r<hadlsch->Cminus ? hadlsch->Kminus : hadlsch->Kplus) >>3;
    rdata->filler=(r==0) ? hadlsch->F : 0;
    rdata->r=r;
    rdata->harq_pid=harq_pid;
    rdata->dlsch=dlsch;
    rdata->rm_stats=rm_stats;
    rdata->te_stats=te_stats;
    rdata->i_stats=i_stats;
    rdata->round=hadlsch->round;
    rdata->r_offset=r_offset;
    rdata->G=G;
    
    if (  proc->threadPool->activated ) {
      pushTpool(proc->threadPool,req);
      proc->nbEncode++;
    } else {
      TPencode(rdata);
      delNotifiedFIFO_elt(req);
    }
    
    int Qm=hadlsch->Qm;
    int C=hadlsch->C;
    int Nl=hadlsch->Nl;
    int Gp = G/Nl/Qm;
    int GpmodC = Gp%C;
    if (r < (C-(GpmodC)))
      r_offset += Nl*Qm * (Gp/C);
    else
      r_offset += Nl*Qm * ((GpmodC==0?0:1) + (Gp/C));
  }

  return(0);
}


