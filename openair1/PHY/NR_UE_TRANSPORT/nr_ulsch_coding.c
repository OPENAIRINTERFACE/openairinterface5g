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
#include "PHY/CODING/nrLDPC_encoder/defs.h"
#include "PHY/NR_UE_TRANSPORT/nr_transport_ue.h"
#include "common/utils/LOG/vcd_signal_dumper.h"





void free_nr_ue_ulsch(NR_UE_ULSCH_t *ulsch)
{
  int i;
  int r;

  if (ulsch) {
#ifdef DEBUG_ULSCH_FREE
    printf("Freeing ulsch %p\n",ulsch);
#endif

    for (i=0; i<NR_MAX_ULSCH_HARQ_PROCESSES; i++) {
      if (ulsch->harq_processes[i]) {
        if (ulsch->harq_processes[i]->b) {
          free16(ulsch->harq_processes[i]->b,MAX_NR_ULSCH_PAYLOAD_BYTES);
          ulsch->harq_processes[i]->b = NULL;
        }
        for (r=0; r<MAX_NUM_NR_ULSCH_SEGMENTS; r++) {
          if (ulsch->harq_processes[i]->c[r]) {
            free16(ulsch->harq_processes[i]->c[r],((r==0)?8:0) + 3+768);
            ulsch->harq_processes[i]->c[r] = NULL;
          }

          if (ulsch->harq_processes[i]->d[r]) {
            free16(ulsch->harq_processes[i]->d[r],68*384);
            ulsch->harq_processes[i]->d[r] = NULL;
          }

          if (ulsch->harq_processes[i]->e[r]) {
            free16(ulsch->harq_processes[i]->e[r],68*384);
            ulsch->harq_processes[i]->e[r] = NULL;
          }

          if (ulsch->harq_processes[i]->f[r]) {
            free16(ulsch->harq_processes[i]->f[r],68*384);
            ulsch->harq_processes[i]->f[r] = NULL;
          }

        }

        free16(ulsch->harq_processes[i],sizeof(NR_UL_UE_HARQ_t));
        ulsch->harq_processes[i] = NULL;
      }
    }
    free16(ulsch,sizeof(NR_UE_ULSCH_t));
    ulsch = NULL;
  }

}

NR_UE_ULSCH_t *new_nr_ue_ulsch(unsigned char N_RB_UL, int number_of_harq_pids, uint8_t abstraction_flag)
{

  NR_UE_ULSCH_t *ulsch;
  unsigned char exit_flag = 0,i,j,r;
  unsigned char bw_scaling =1;

  switch (N_RB_UL) {

  case 106:
    bw_scaling =2;
    break;

  default:
    bw_scaling =1;
    break;
  }

  ulsch = (NR_UE_ULSCH_t *)malloc16(sizeof(NR_UE_ULSCH_t));

  if (ulsch) {
    memset(ulsch,0,sizeof(NR_UE_ULSCH_t));

    ulsch->number_harq_processes_for_pusch = NR_MAX_ULSCH_HARQ_PROCESSES;
    ulsch->Mlimit = 4; // maximum harq retransmissions

    for (i=0; i<number_of_harq_pids; i++) {

      ulsch->harq_processes[i] = (NR_UL_UE_HARQ_t *)malloc16(sizeof(NR_UL_UE_HARQ_t));

      //      printf("ulsch->harq_processes[%d] %p\n",i,ulsch->harq_processes[i]);
      if (ulsch->harq_processes[i]) {
        memset(ulsch->harq_processes[i], 0, sizeof(NR_UL_UE_HARQ_t));
        ulsch->harq_processes[i]->b = (unsigned char*)malloc16(MAX_NR_ULSCH_PAYLOAD_BYTES/bw_scaling);
        ulsch->harq_processes[i]->a = (uint8_t*)malloc16(MAX_NR_ULSCH_PAYLOAD_BYTES/bw_scaling);

        if (ulsch->harq_processes[i]->a) {
          bzero(ulsch->harq_processes[i]->a,MAX_NR_ULSCH_PAYLOAD_BYTES/bw_scaling);
        } else {
          printf("Can't allocate PDU\n");
          exit_flag=1;
        }

        if (ulsch->harq_processes[i]->b)
          memset(ulsch->harq_processes[i]->b,0,MAX_NR_ULSCH_PAYLOAD_BYTES/bw_scaling);
        else {
          LOG_E(PHY,"Can't get b\n");
          exit_flag=1;
        }

        if (abstraction_flag==0) {
          for (r=0; r<MAX_NUM_NR_ULSCH_SEGMENTS/bw_scaling; r++) {
            // account for filler in first segment and CRCs for multiple segment case
            ulsch->harq_processes[i]->c[r] = (uint8_t*)malloc16(8448);
            ulsch->harq_processes[i]->d[r] = (uint8_t*)malloc16(68*384); //max size for coded output
            ulsch->harq_processes[i]->e[r] = (uint8_t*)malloc16(68*384);
            ulsch->harq_processes[i]->f[r] = (uint8_t*)malloc16(68*384);
            if (ulsch->harq_processes[i]->c[r]) {
              bzero(ulsch->harq_processes[i]->c[r],8448);
            } else {
              printf("Can't get c\n");
              exit_flag=2;
            }
            if (ulsch->harq_processes[i]->d[r]) {
              bzero(ulsch->harq_processes[i]->d[r],(68*384));
            } else {
              printf("Can't get d\n");
              exit_flag=2;
            }
            if (ulsch->harq_processes[i]->e[r]) {
              bzero(ulsch->harq_processes[i]->e[r],(68*384));
            } else {
              printf("Can't get e\n");
              exit_flag=2;
            }
            if (ulsch->harq_processes[i]->f[r]) {
              bzero(ulsch->harq_processes[i]->f[r],(68*384));
            } else {
              printf("Can't get f\n");
              exit_flag=2;
            }
          }
        }

        ulsch->harq_processes[i]->subframe_scheduling_flag = 0;
        ulsch->harq_processes[i]->first_tx = 1;
      } else {
        LOG_E(PHY,"Can't get harq_p %d\n",i);
        exit_flag=3;
      }
    }

    if ((abstraction_flag == 0) && (exit_flag==0)) {
      for (i=0; i<8; i++)
        for (j=0; j<96; j++)
          for (r=0; r<MAX_NUM_NR_ULSCH_SEGMENTS/bw_scaling; r++)
            ulsch->harq_processes[i]->d[r][j] = NR_NULL;

      return(ulsch);
    } else if (abstraction_flag==1)
      return(ulsch);
  }

  LOG_E(PHY,"new_ue_ulsch exit flag, size of  %d ,   %zu\n",exit_flag, sizeof(LTE_UE_ULSCH_t));
  free_nr_ue_ulsch(ulsch);
  return(NULL);


}