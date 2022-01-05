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

/*! \file PHY/NR_TRANSPORT/nr_ulsch.c
* \brief Top-level routines for the reception of the PUSCH TS 38.211 v 15.4.0
* \author Ahmed Hussein
* \date 2019
* \version 0.1
* \company Fraunhofer IIS
* \email: ahmed.hussein@iis.fraunhofer.de
* \note
* \warning
*/

#include <stdint.h>
#include "PHY/NR_TRANSPORT/nr_transport_common_proto.h"
#include "PHY/NR_TRANSPORT/nr_ulsch.h"
#include "PHY/NR_REFSIG/nr_refsig.h"

int16_t find_nr_ulsch(uint16_t rnti, PHY_VARS_gNB *gNB,find_type_t type) {

  uint16_t i;
  int16_t first_free_index=-1;

  AssertFatal(gNB!=NULL,"gNB is null\n");
  for (i=0; i<gNB->number_of_nr_ulsch_max; i++) {
    AssertFatal(gNB->ulsch[i]!=NULL,"gNB->ulsch[%d] is null\n",i);
    AssertFatal(gNB->ulsch[i][0]!=NULL,"gNB->ulsch[%d][0] is null\n",i);
    LOG_D(PHY,"searching for rnti %x : ulsch_index %d=> harq_mask %x, rnti %x, first_free_index %d\n", rnti,i,gNB->ulsch[i][0]->harq_mask,gNB->ulsch[i][0]->rnti,first_free_index);
    if ((gNB->ulsch[i][0]->harq_mask >0) &&
        (gNB->ulsch[i][0]->rnti==rnti))       return i;
    else if ((gNB->ulsch[i][0]->harq_mask == 0) && (first_free_index==-1)) first_free_index=i;
  }
  if (type == SEARCH_EXIST) return -1;
  if (first_free_index != -1)
    gNB->ulsch[first_free_index][0]->rnti = 0;
  return first_free_index;
}

void nr_fill_ulsch(PHY_VARS_gNB *gNB,
                   int frame,
                   int slot,
                   nfapi_nr_pusch_pdu_t *ulsch_pdu) {

 
  int ulsch_id = find_nr_ulsch(ulsch_pdu->rnti,gNB,SEARCH_EXIST_OR_FREE);
  AssertFatal( (ulsch_id>=0) && (ulsch_id<gNB->number_of_nr_ulsch_max),
              "illegal or no ulsch_id found!!! rnti %04x ulsch_id %d\n",ulsch_pdu->rnti,ulsch_id);

  NR_gNB_ULSCH_t  *ulsch = gNB->ulsch[ulsch_id][0];
  int harq_pid = ulsch_pdu->pusch_data.harq_process_id;
  ulsch->rnti = ulsch_pdu->rnti;
  //ulsch->rnti_type;
  ulsch->harq_mask |= 1<<harq_pid;
  ulsch->harq_process_id[slot] = harq_pid;

  ulsch->harq_processes[harq_pid]->frame=frame;
  ulsch->harq_processes[harq_pid]->slot=slot;
  ulsch->harq_processes[harq_pid]->handled= 0;
  ulsch->harq_processes[harq_pid]->status= NR_ACTIVE;
  memcpy((void*)&ulsch->harq_processes[harq_pid]->ulsch_pdu, (void*)ulsch_pdu, sizeof(nfapi_nr_pusch_pdu_t));

  LOG_D(PHY,"Initializing nFAPI for ULSCH, UE %d, harq_pid %d\n",ulsch_id,harq_pid);

}

void nr_ulsch_unscrambling(int16_t* llr,
                           uint32_t size,
                           uint8_t q,
                           uint32_t Nid,
                           uint32_t n_RNTI) {

  uint8_t reset;
  uint32_t x1, x2, s=0;

  reset = 1;
  x2 = (n_RNTI<<15) + Nid;

  for (uint32_t i=0; i<size; i++) {
    if ((i&0x1f)==0) {
      s = lte_gold_generic(&x1, &x2, reset);
      reset = 0;
    }
    if (((s>>(i&0x1f))&1)==1)
      llr[i] = -llr[i];
  }
}

void nr_ulsch_unscrambling_optim(int16_t* llr,
				 uint32_t size,
				 uint8_t q,
				 uint32_t Nid,
				 uint32_t n_RNTI) {
  
#if defined(__x86_64__) || defined(__i386__)
  uint32_t x1, x2, s=0;

  x2 = (n_RNTI<<15) + Nid;

  uint8_t *s8=(uint8_t *)&s;
  __m128i *llr128 = (__m128i*)llr;
  int j=0;
  s = lte_gold_generic(&x1, &x2, 1);

  for (int i=0; i<((size>>5)+((size&0x1f) > 0 ? 1 : 0)); i++,j+=4) {
    llr128[j]   = _mm_mullo_epi16(llr128[j],byte2m128i[s8[0]]);
    llr128[j+1] = _mm_mullo_epi16(llr128[j+1],byte2m128i[s8[1]]);
    llr128[j+2] = _mm_mullo_epi16(llr128[j+2],byte2m128i[s8[2]]);
    llr128[j+3] = _mm_mullo_epi16(llr128[j+3],byte2m128i[s8[3]]);
    s = lte_gold_generic(&x1, &x2, 0);
  }
#else

    nr_ulsch_unscrambling(llr,
                          size,
                          q,
                          Nid,
                          n_RNTI);
#endif
}

void dump_pusch_stats(FILE *fd,PHY_VARS_gNB *gNB) {

  for (int i=0;i<gNB->number_of_nr_ulsch_max;i++) {

    if (gNB->ulsch_stats[i].rnti>0 && gNB->ulsch_stats[i].frame != gNB->ulsch_stats[i].dump_frame) {
      gNB->ulsch_stats[i].dump_frame = gNB->ulsch_stats[i].frame; 
      for (int aa=0;aa<gNB->frame_parms.nb_antennas_rx;aa++)
          if (aa==0) fprintf(fd,"ULSCH RNTI %4x, %d.%d: ulsch_power[%d] %d,%d ulsch_noise_power[%d] %d.%d, sync_pos %d\n",
                                     gNB->ulsch_stats[i].rnti,gNB->ulsch_stats[i].frame,gNB->ulsch_stats[i].dump_frame,
                                     aa,gNB->ulsch_stats[i].power[aa]/10,gNB->ulsch_stats[i].power[aa]%10,
                                     aa,gNB->ulsch_stats[i].noise_power[aa]/10,gNB->ulsch_stats[i].noise_power[aa]%10,
                                     gNB->ulsch_stats[i].sync_pos);
          else       fprintf(fd,"                  ulsch_power[%d] %d.%d, ulsch_noise_power[%d] %d.%d\n",
                                     aa,gNB->ulsch_stats[i].power[aa]/10,gNB->ulsch_stats[i].power[aa]%10,
                                     aa,gNB->ulsch_stats[i].noise_power[aa]/10,gNB->ulsch_stats[i].noise_power[aa]%10);


      fprintf(fd,"                 round_trials %d(%1.1e):%d(%1.1e):%d(%1.1e):%d, DTX %d, current_Qm %d, current_RI %d, total_bytes RX/SCHED %d/%d\n",
	    gNB->ulsch_stats[i].round_trials[0],
	    (double)gNB->ulsch_stats[i].round_trials[1]/gNB->ulsch_stats[i].round_trials[0],
	    gNB->ulsch_stats[i].round_trials[1],
	    (double)gNB->ulsch_stats[i].round_trials[2]/gNB->ulsch_stats[i].round_trials[0],
	    gNB->ulsch_stats[i].round_trials[2],
	    (double)gNB->ulsch_stats[i].round_trials[3]/gNB->ulsch_stats[i].round_trials[0],
	    gNB->ulsch_stats[i].round_trials[3],
            gNB->ulsch_stats[i].DTX,
	    gNB->ulsch_stats[i].current_Qm,
	    gNB->ulsch_stats[i].current_RI,
	    gNB->ulsch_stats[i].total_bytes_rx,
	    gNB->ulsch_stats[i].total_bytes_tx);
    }
 }
}

void clear_pusch_stats(PHY_VARS_gNB *gNB) {

  for (int i=0;i<gNB->number_of_nr_ulsch_max;i++)
    memset((void*)&gNB->ulsch_stats[i],0,sizeof(gNB->ulsch_stats[i]));
}

NR_gNB_SCH_STATS_t *get_ulsch_stats(PHY_VARS_gNB *gNB,NR_gNB_ULSCH_t *ulsch) {
   NR_gNB_SCH_STATS_t *stats=NULL;
   int first_free=-1;
   for (int i=0;i<gNB->number_of_nr_ulsch_max;i++) {
       if (gNB->ulsch_stats[i].rnti == 0 && first_free == -1) {
          first_free = i;
          stats=&gNB->ulsch_stats[i];
       }
       if (gNB->ulsch_stats[i].rnti == ulsch->rnti) {
           stats=&gNB->ulsch_stats[i];
           break;
       }
   }
   return(stats);
}

