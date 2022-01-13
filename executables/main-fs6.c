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
* Author and copyright: Laurent Thomas, open-cells.com
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

#include <stdint.h>
#include <common/utils/LOG/log.h>
#include <common/utils/system.h>
#include <common/config/config_userapi.h>
#include <targets/RT/USER/lte-softmodem.h>
#include <openair1/PHY/defs_eNB.h>
#include <openair1/PHY/phy_extern.h>
#include <nfapi/oai_integration/vendor_ext.h>
#include <openair1/SCHED/fapi_l1.h>
#include <openair1/PHY/INIT/phy_init.h>
#include <openair2/LAYER2/MAC/mac_extern.h>
#include <openair1/PHY/LTE_REFSIG/lte_refsig.h>
#include <nfapi/oai_integration/nfapi_pnf.h>
#include <executables/split_headers.h>
#include <nfapi/oai_integration/vendor_ext.h>
#include <openair1/PHY/INIT/lte_init.c>
#include <openair1/PHY/LTE_ESTIMATION/lte_estimation.h>
#include <executables/split_headers.h>
#include <openair1/PHY/CODING/coding_extern.h>
#include <threadPool/thread-pool.h>
#include <emmintrin.h>

#define FS6_BUF_SIZE 1000*1000
static UDPsock_t sockFS6;

int sum(uint8_t *b, int s) {
  int sum=0;

  for (int i=0; i < s; i++)
    sum+=b[i];

  return sum;
}

static inline int cmpintRev(const void *a, const void *b) {
  uint64_t *aa=(uint64_t *)a;
  uint64_t *bb=(uint64_t *)b;
  return (int)(*bb-*aa);
}

static inline void printMeas2(char *txt, Meas *M, int period, bool MaxMin) {
  if (M->iterations%period == 0 ) {
    char txt2[512];
    sprintf(txt2,"%s avg=%" PRIu64 " iterations=%" PRIu64 " %s=%"
            PRIu64 ":%" PRIu64 ":%" PRIu64 ":%" PRIu64 ":%" PRIu64 ":%" PRIu64 ":%" PRIu64 ":%" PRIu64 ":%" PRIu64 ":%" PRIu64 "\n",
            txt,
            M->sum/M->iterations,
            M->iterations,
            MaxMin?"max":"min",
            M->maxArray[1],M->maxArray[2], M->maxArray[3],M->maxArray[4], M->maxArray[5],
            M->maxArray[6],M->maxArray[7], M->maxArray[8],M->maxArray[9],M->maxArray[10]);
#if T_TRACER
    LOG_W(PHY,"%s",txt2);
#else
    printf("%s",txt2);
#endif
  }
}

static inline void updateTimesReset(uint64_t start, Meas *M, int period, bool MaxMin, char *txt) {
  if (start!=0) {
    uint64_t end=rdtsc();
    long long diff=(end-start)/(cpuf*1000);
    M->maxArray[0]=diff;
    M->sum+=diff;
    M->iterations++;

    if ( MaxMin)
      qsort(M->maxArray, 11, sizeof(uint64_t), cmpint);
    else
      qsort(M->maxArray, 11, sizeof(uint64_t), cmpintRev);

    printMeas2(txt,M,period, MaxMin);

    if (M->iterations%period == 0 ) {
      bzero(M,sizeof(*M));

      if (!MaxMin)
        for (int i=0; i<11; i++)
          M->maxArray[i]=INT_MAX;
    }
  }
}

static inline void measTransportTime(uint64_t DuSend, uint64_t CuMicroSec, Meas *M, int period, bool MaxMin, char *txt) {
  if (DuSend!=0) {
    uint64_t end=rdtsc();
    long long diff=(end-DuSend)/(cpuf*1000)-CuMicroSec;
    M->maxArray[0]=diff;
    M->sum+=diff;
    M->iterations++;

    if ( MaxMin)
      qsort(M->maxArray, 11, sizeof(uint64_t), cmpint);
    else
      qsort(M->maxArray, 11, sizeof(uint64_t), cmpintRev);

    printMeas2(txt,M,period, MaxMin);

    if (M->iterations%period == 0 ) {
      bzero(M,sizeof(*M));

      if (!MaxMin)
        for (int i=0; i<11; i++)
          M->maxArray[i]=INT_MAX;
    }
  }
}

#define ceil16_bytes(a) ((((a+15)/16)*16)/8)

static void fs6Dlunpack(void *out, void *in, int szUnpacked) {
  static uint64_t *lut=NULL;

  if (!lut) {
    lut=(uint64_t *) malloc(sizeof(*lut)*256);

    for (int i=0; i <256; i++)
      for (int j=0; j<8; j++)
        ((uint8_t *)(lut+i))[7-j]=(i>>j)&1;
  }

  int64_t *out_64 = (int64_t *)out;
  int sz=ceil16_bytes(szUnpacked);

  for (int i=0; i<sz; i++)
    out_64[i]=lut[((uint8_t *)in)[i]];

  return;
}


static void fs6Dlpack(void *out, void *in, int szUnpacked) {
  __m128i zeros=_mm_set1_epi8(0);
  __m128i shuffle=_mm_set_epi8(8,9,10,11,12,13,14,15,0,1,2,3,4,5,6,7);
  const int loop=ceil16_bytes(szUnpacked)/sizeof(uint16_t);
  __m128i *iter=(__m128i *)in;

  for (int i=0; i < loop; i++) {
    __m128i tmp=_mm_shuffle_epi8(_mm_cmpgt_epi8(*iter++,zeros),shuffle);
    ((uint16_t *)out)[i]=(uint16_t)_mm_movemask_epi8(tmp);
  }
}

void prach_eNB_tosplit(uint8_t *bufferZone, int bufSize, PHY_VARS_eNB *eNB, L1_rxtx_proc_t *proc) {
  fs6_ul_t *header=(fs6_ul_t *) commonUDPdata(bufferZone);

  if (is_prach_subframe(&eNB->frame_parms, proc->frame_prach,proc->subframe_prach)<=0)
    return;

  RU_t *ru;
  int aa=0;
  int ru_aa;

  for (int i=0; i<eNB->num_RU; i++) {
    ru=eNB->RU_list[i];

    for (ru_aa=0,aa=0; ru_aa<ru->nb_rx; ru_aa++,aa++) {
      eNB->prach_vars.rxsigF[0][aa] = eNB->RU_list[i]->prach_rxsigF[0][ru_aa];
      int ce_level;

      for (ce_level=0; ce_level<4; ce_level++)
        eNB->prach_vars_br.rxsigF[ce_level][aa] = eNB->RU_list[i]->prach_rxsigF_br[ce_level][ru_aa];
    }
  }

  ocp_rx_prach(eNB,
               proc,
               eNB->RU_list[0],
               header->max_preamble,
               header->max_preamble_energy,
               header->max_preamble_delay,
               header->avg_preamble_energy,
               proc->frame_prach,
               0,
               false
              );
  // run PRACH detection for CE-level 0 only for now when br_flag is set
  /* fixme: seems not operational and may overwrite regular LTE prach detection
   * OAI code can call is sequence
  rx_prach(eNB,
           eNB->RU_list[0],
           header->max_preamble,
           header->max_preamble_energy,
           header->max_preamble_delay,
           header->avg_preamble_energy,
           frame,
           0,
           true
          );
  */
  LOG_D(PHY,"RACH detection index 0: max preamble: %u, energy: %u, delay: %u, avg energy: %u\n",
        header->max_preamble[0],
        header->max_preamble_energy[0],
        header->max_preamble_delay[0],
        header->avg_preamble_energy[0]
       );
  return;
}

void prach_eNB_fromsplit(uint8_t *bufferZone, int bufSize, PHY_VARS_eNB *eNB, L1_rxtx_proc_t *proc) {
  fs6_ul_t *header=(fs6_ul_t *) commonUDPdata(bufferZone);
  uint16_t *max_preamble=header->max_preamble;
  uint16_t *max_preamble_energy=header->max_preamble_energy;
  uint16_t *max_preamble_delay=header->max_preamble_delay;
  uint16_t *avg_preamble_energy=header->avg_preamble_energy;
  int subframe=proc->subframe_prach;
  int frame=proc->frame_prach;
  // Fixme: not clear why we call twice with "br" and without
  int br_flag=0;

  if (br_flag==1) {
    int             prach_mask;
    prach_mask = is_prach_subframe (&eNB->frame_parms, proc->frame_prach_br, proc->subframe_prach_br);
    eNB->UL_INFO.rach_ind_br.rach_indication_body.preamble_list = eNB->preamble_list_br;
    int             ind = 0;
    int             ce_level = 0;
    /* Save for later, it doesn't work
       for (int ind=0,ce_level=0;ce_level<4;ce_level++) {

       if ((eNB->frame_parms.prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[ce_level]==1)&&
       (prach_mask&(1<<(1+ce_level)) > 0) && // prach is active and CE level has finished its repetitions
       (eNB->prach_vars_br.repetition_number[ce_level]==
       eNB->frame_parms.prach_emtc_config_common.prach_ConfigInfo.prach_numRepetitionPerPreambleAttempt[ce_level])) {

    */

    if (eNB->frame_parms.prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[0] == 1) {
      if ((eNB->prach_energy_counter == 100) && (max_preamble_energy[0] > eNB->measurements.prach_I0 + eNB->prach_DTX_threshold_emtc[0])) {
        eNB->UL_INFO.rach_ind_br.rach_indication_body.number_of_preambles++;
        eNB->preamble_list_br[ind].preamble_rel8.timing_advance = max_preamble_delay[ind];      //
        eNB->preamble_list_br[ind].preamble_rel8.preamble = max_preamble[ind];
        // note: fid is implicitly 0 here, this is the rule for eMTC RA-RNTI from 36.321, Section 5.1.4
        eNB->preamble_list_br[ind].preamble_rel8.rnti = 1 + subframe + (60*(eNB->prach_vars_br.first_frame[ce_level] % 40));
        eNB->preamble_list_br[ind].instance_length = 0; //don't know exactly what this is
        eNB->preamble_list_br[ind].preamble_rel13.rach_resource_type = 1 + ce_level;    // CE Level
        LOG_I (PHY, "Filling NFAPI indication for RACH %d CELevel %d (mask %x) : TA %d, Preamble %d, rnti %x, rach_resource_type %d\n",
               ind,
               ce_level,
               prach_mask,
               eNB->preamble_list_br[ind].preamble_rel8.timing_advance,
               eNB->preamble_list_br[ind].preamble_rel8.preamble, eNB->preamble_list_br[ind].preamble_rel8.rnti, eNB->preamble_list_br[ind].preamble_rel13.rach_resource_type);
      }
    }

    /*
      ind++;
      }
      } */// ce_level
  } else if ((eNB->prach_energy_counter == 100) &&
             (max_preamble_energy[0] > eNB->measurements.prach_I0+eNB->prach_DTX_threshold)) {
    LOG_I(PHY,"[eNB %d/%d][RAPROC] Frame %d, subframe %d Initiating RA procedure with preamble %d, energy %d.%d dB, delay %d\n",
          eNB->Mod_id,
          eNB->CC_id,
          frame,
          subframe,
          max_preamble[0],
          max_preamble_energy[0]/10,
          max_preamble_energy[0]%10,
          max_preamble_delay[0]);
    pthread_mutex_lock(&eNB->UL_INFO_mutex);
    eNB->UL_INFO.rach_ind.rach_indication_body.number_of_preambles  = 1;
    eNB->UL_INFO.rach_ind.rach_indication_body.preamble_list        = &eNB->preamble_list[0];
    eNB->UL_INFO.rach_ind.rach_indication_body.tl.tag               = NFAPI_RACH_INDICATION_BODY_TAG;
    eNB->UL_INFO.rach_ind.header.message_id                         = NFAPI_RACH_INDICATION;
    eNB->UL_INFO.rach_ind.sfn_sf                                    = frame<<4 | subframe;
    eNB->preamble_list[0].preamble_rel8.tl.tag                = NFAPI_PREAMBLE_REL8_TAG;
    eNB->preamble_list[0].preamble_rel8.timing_advance        = max_preamble_delay[0];
    eNB->preamble_list[0].preamble_rel8.preamble              = max_preamble[0];
    eNB->preamble_list[0].preamble_rel8.rnti                  = 1+subframe;  // note: fid is implicitly 0 here
    eNB->preamble_list[0].preamble_rel13.rach_resource_type   = 0;
    eNB->preamble_list[0].instance_length                     = 0; //don't know exactly what this is

    if (NFAPI_MODE==NFAPI_MODE_PNF) {  // If NFAPI PNF then we need to send the message to the VNF
      LOG_D(PHY,"Filling NFAPI indication for RACH : SFN_SF:%d TA %d, Preamble %d, rnti %x, rach_resource_type %d\n",
            NFAPI_SFNSF2DEC(eNB->UL_INFO.rach_ind.sfn_sf),
            eNB->preamble_list[0].preamble_rel8.timing_advance,
            eNB->preamble_list[0].preamble_rel8.preamble,
            eNB->preamble_list[0].preamble_rel8.rnti,
            eNB->preamble_list[0].preamble_rel13.rach_resource_type);
      oai_nfapi_rach_ind(&eNB->UL_INFO.rach_ind);
      eNB->UL_INFO.rach_ind.rach_indication_body.number_of_preambles = 0;
    }

    pthread_mutex_unlock(&eNB->UL_INFO_mutex);
  } // max_preamble_energy > prach_I0 + 100
  else {
    eNB->measurements.prach_I0 = ((eNB->measurements.prach_I0*900)>>10) + ((avg_preamble_energy[0]*124)>>10);

    if (eNB->prach_energy_counter < 100)
      eNB->prach_energy_counter++;
  }
}

void sendFs6Ulharq(enum pckType type, int UEid, PHY_VARS_eNB *eNB, LTE_eNB_UCI *uci, int frame, int subframe, uint8_t *harq_ack, uint8_t tdd_mapping_mode, uint16_t tdd_multiplexing_mask,
                   uint16_t rnti,
                   int32_t stat) {
  static int current_fsf=-1;
  int fsf=frame*16+subframe;
  uint8_t *bufferZone=eNB->FS6bufferZone;
  commonUDP_t *FirstUDPheader=(commonUDP_t *) bufferZone;
  // move to the end
  uint8_t *firstFreeByte=bufferZone;
  int curBlock=0;

  if ( current_fsf != fsf ) {
    for (int i=0; i < FirstUDPheader->nbBlocks; i++) {
      AssertFatal( ((commonUDP_t *) firstFreeByte)->blockID==curBlock,"");
      firstFreeByte+=alignedSize(firstFreeByte);
      curBlock++;
    }

    commonUDP_t *newUDPheader=(commonUDP_t *) firstFreeByte;
    FirstUDPheader->nbBlocks++;
    newUDPheader->blockID=curBlock;
    newUDPheader->contentBytes=sizeof(fs6_ul_t)+sizeof(fs6_ul_uespec_uci_t);
    hULUEuci(newUDPheader)->type=fs6ULcch;
    hULUEuci(newUDPheader)->nb_active_ue=0;
  } else
    for (int i=0; i < FirstUDPheader->nbBlocks-1; i++) {
      AssertFatal( ((commonUDP_t *) firstFreeByte)->blockID==curBlock,"");
      firstFreeByte+=alignedSize(firstFreeByte);
      curBlock++;
    }

  LOG_D(PHY,"FS6 du, block: %d: adding ul harq/sr: %d, rnti: %d, ueid: %d\n",
        curBlock, type, rnti, UEid);
  commonUDP_t *newUDPheader=(commonUDP_t *) firstFreeByte;
  fs6_ul_uespec_uci_element_t *tmp=(fs6_ul_uespec_uci_element_t *)(hULUEuci(newUDPheader)+1);
  tmp+=hULUEuci(newUDPheader)->nb_active_ue;
  tmp->type=type;
  tmp->UEid=UEid;
  tmp->frame=frame;
  tmp->subframe=subframe;

  if (uci != NULL)
    memcpy(&tmp->uci, uci, sizeof(*uci));
  else
    tmp->uci.ue_id=0xFFFF;

  if (harq_ack != NULL)
    memcpy(tmp->harq_ack, harq_ack, 4);

  tmp->tdd_mapping_mode=tdd_mapping_mode;
  tmp->tdd_multiplexing_mask=tdd_multiplexing_mask;
  tmp->n0_subband_power_dB=eNB->measurements.n0_subband_power_dB[0][0];
  tmp->rnti=rnti;
  tmp->stat=stat;
  hULUEuci(newUDPheader)->nb_active_ue++;
  newUDPheader->contentBytes+=sizeof(fs6_ul_uespec_uci_element_t);
}


void sendFs6Ul(PHY_VARS_eNB *eNB, int UE_id, int harq_pid, int segmentID, int16_t *data, int dataLen, int r_offset) {
  uint8_t *bufferZone=eNB->FS6bufferZone;
  commonUDP_t *FirstUDPheader=(commonUDP_t *) bufferZone;
  // move to the end
  uint8_t *firstFreeByte=bufferZone;
  int curBlock=0;

  for (int i=0; i < FirstUDPheader->nbBlocks; i++) {
    AssertFatal( ((commonUDP_t *) firstFreeByte)->blockID==curBlock,"");
    firstFreeByte+=alignedSize(firstFreeByte);
    curBlock++;
  }

  commonUDP_t *newUDPheader=(commonUDP_t *) firstFreeByte;
  FirstUDPheader->nbBlocks++;
  newUDPheader->blockID=curBlock;
  newUDPheader->contentBytes=sizeof(fs6_ul_t)+sizeof(fs6_ul_uespec_t) + dataLen;
  hULUE(newUDPheader)->type=fs6ULsch;
  hULUE(newUDPheader)->UE_id=UE_id;
  hULUE(newUDPheader)->harq_id=harq_pid;
  memcpy(hULUE(newUDPheader)->ulsch_power,
         eNB->pusch_vars[UE_id]->ulsch_power,
         sizeof(int)*2);
  hULUE(newUDPheader)->cqi_crc_status=eNB->ulsch[UE_id]->harq_processes[harq_pid]->cqi_crc_status;
  hULUE(newUDPheader)->O_ACK=eNB->ulsch[UE_id]->harq_processes[harq_pid]->O_ACK;
  memcpy(hULUE(newUDPheader)->o_ACK, eNB->ulsch[UE_id]->harq_processes[harq_pid]->o_ACK,
         sizeof(eNB->ulsch[UE_id]->harq_processes[harq_pid]->o_ACK));
  hULUE(newUDPheader)->ta=lte_est_timing_advance_pusch(&eNB->frame_parms, eNB->pusch_vars[UE_id]->drs_ch_estimates_time);
  hULUE(newUDPheader)->segment=segmentID;
  memcpy(hULUE(newUDPheader)->o, eNB->ulsch[UE_id]->harq_processes[harq_pid]->o,
         sizeof(eNB->ulsch[UE_id]->harq_processes[harq_pid]->o));
  memcpy(hULUE(newUDPheader)+1, data, dataLen);
  hULUE(newUDPheader)->segLen=dataLen;
  hULUE(newUDPheader)->r_offset=r_offset;
  hULUE(newUDPheader)->G=eNB->ulsch[UE_id]->harq_processes[harq_pid]->G;
}

void pusch_procedures_tosplit(uint8_t *bufferZone, int bufSize, PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc) {
  uint32_t harq_pid;
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  const int subframe = proc->subframe_rx;
  const int frame    = proc->frame_rx;

  for (int i = 0; i < NUMBER_OF_UE_MAX; i++) {
    LTE_eNB_ULSCH_t *ulsch = eNB->ulsch[i];

    if (ulsch->ue_type > NOCE)
      harq_pid = 0;
    else
      harq_pid= subframe2harq_pid(&eNB->frame_parms,frame,subframe);

    LTE_UL_eNB_HARQ_t *ulsch_harq = ulsch->harq_processes[harq_pid];

    if (ulsch->rnti>0)
      LOG_D(PHY,"eNB->ulsch[%d]->harq_processes[harq_pid:%d] SFN/SF:%04d%d: PUSCH procedures, UE %d/%x ulsch_harq[status:%d SFN/SF:%04d%d active: %d handled:%d]\n",
            i, harq_pid, frame,subframe,i,ulsch->rnti,
            ulsch_harq->status, ulsch_harq->frame, ulsch_harq->subframe, ulsch_harq->status, ulsch_harq->handled);

    if ((ulsch) &&
        (ulsch->rnti>0) &&
        (ulsch_harq->status == ACTIVE) &&
        ((ulsch_harq->frame == frame)     || (ulsch_harq->repetition_number >1) ) &&
        ((ulsch_harq->subframe == subframe) || (ulsch_harq->repetition_number >1) ) &&
        (ulsch_harq->handled == 0)) {
      // UE has ULSCH scheduling
      for (int rb=0;
           rb<=ulsch_harq->nb_rb;
           rb++) {
        int rb2 = rb+ulsch_harq->first_rb;
        eNB->rb_mask_ul[rb2>>5] |= (1<<(rb2&31));
      }

      LOG_D(PHY,"[eNB %d] frame %d, subframe %d: Scheduling ULSCH Reception for UE %d \n",
            eNB->Mod_id, frame, subframe, i);
      uint8_t nPRS= fp->pusch_config_common.ul_ReferenceSignalsPUSCH.nPRS[subframe<<1];
      ulsch->cyclicShift = (ulsch_harq->n_DMRS2 +
                            fp->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift +
                            nPRS)%12;
      AssertFatal(ulsch_harq->TBS>0,"illegal TBS %d\n",ulsch_harq->TBS);
      LOG_D(PHY,
            "[eNB %d][PUSCH %d] Frame %d Subframe %d Demodulating PUSCH: dci_alloc %d, rar_alloc %d, round %d, first_rb %d, nb_rb %d, Qm %d, TBS %d, rv %d, cyclic_shift %d (n_DMRS2 %d, cyclicShift_common %d, ), O_ACK %d, beta_cqi %d \n",
            eNB->Mod_id,harq_pid,frame,subframe,
            ulsch_harq->dci_alloc,
            ulsch_harq->rar_alloc,
            ulsch_harq->round,
            ulsch_harq->first_rb,
            ulsch_harq->nb_rb,
            ulsch_harq->Qm,
            ulsch_harq->TBS,
            ulsch_harq->rvidx,
            ulsch->cyclicShift,
            ulsch_harq->n_DMRS2,
            fp->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift,
            ulsch_harq->O_ACK,
            ulsch->beta_offset_cqi_times8);
      start_meas(&eNB->ulsch_demodulation_stats);
      eNB->FS6bufferZone=bufferZone;
      rx_ulsch(eNB, proc, i);
      stop_meas(&eNB->ulsch_demodulation_stats);
      // TBD: add datablock for transmission
      start_meas(&eNB->ulsch_decoding_stats);
      ulsch_decoding(eNB,proc,
                     i,
                     0, // control_only_flag
                     ulsch_harq->V_UL_DAI,
                     ulsch_harq->nb_rb>20 ? 1 : 0);
      stop_meas(&eNB->ulsch_decoding_stats);
    }
  }
}

void phy_procedures_eNB_uespec_RX_tosplit(uint8_t *bufferZone, int bufSize, PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc) {
  //RX processing for ue-specific resources
  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  const int       subframe = proc->subframe_rx;
  const int       frame = proc->frame_rx;
  /* TODO: use correct rxdata */

  if ((fp->frame_type == TDD) && (subframe_select(fp,subframe)!=SF_UL)) return;

  LOG_D (PHY, "[eNB %d] Frame %d: Doing phy_procedures_eNB_uespec_RX(%d)\n", eNB->Mod_id, frame, subframe);
  eNB->rb_mask_ul[0] = 0;
  eNB->rb_mask_ul[1] = 0;
  eNB->rb_mask_ul[2] = 0;
  eNB->rb_mask_ul[3] = 0;
  // Fix me here, these should be locked
  eNB->UL_INFO.rx_ind.rx_indication_body.number_of_pdus  = 0;
  eNB->UL_INFO.crc_ind.crc_indication_body.number_of_crcs = 0;
  // Call SRS first since all others depend on presence of SRS or lack thereof
  srs_procedures (eNB, proc);
  eNB->first_run_I0_measurements = 0;
  uci_procedures (eNB, proc);

  if (NFAPI_MODE==NFAPI_MONOLITHIC || NFAPI_MODE==NFAPI_MODE_PNF) { // If PNF or monolithic
    pusch_procedures_tosplit(bufferZone, bufSize, eNB,proc);
  }

  lte_eNB_I0_measurements (eNB, subframe, 0, eNB->first_run_I0_measurements);
  int min_I0=1000,max_I0=0;

  if ((frame==0) && (subframe==4)) {
    for (int i=0; i<eNB->frame_parms.N_RB_UL; i++) {
      if (i==(eNB->frame_parms.N_RB_UL>>1) - 1) i+=2;

      if (eNB->measurements.n0_subband_power_tot_dB[i]<min_I0)
        min_I0 = eNB->measurements.n0_subband_power_tot_dB[i];

      if (eNB->measurements.n0_subband_power_tot_dB[i]>max_I0)
        max_I0 = eNB->measurements.n0_subband_power_tot_dB[i];
    }

    LOG_I (PHY, "max_I0 %d, min_I0 %d\n", max_I0, min_I0);
  }

  return;
}


void fill_rx_indication_from_split(uint8_t *bufferZone, PHY_VARS_eNB *eNB,int UE_id,int frame,int subframe, ul_propagation_t *ul_propa) {
  nfapi_rx_indication_pdu_t *pdu;
  int             timing_advance_update;
  uint32_t        harq_pid;

  if (eNB->ulsch[UE_id]->ue_type > 0)
    harq_pid = 0;
  else
    harq_pid = subframe2harq_pid (&eNB->frame_parms,
                                  frame, subframe);

  pthread_mutex_lock(&eNB->UL_INFO_mutex);
  eNB->UL_INFO.rx_ind.sfn_sf                    = frame<<4| subframe;
  eNB->UL_INFO.rx_ind.rx_indication_body.tl.tag = NFAPI_RX_INDICATION_BODY_TAG;
  pdu                                    = &eNB->UL_INFO.rx_ind.rx_indication_body.rx_pdu_list[eNB->UL_INFO.rx_ind.rx_indication_body.number_of_pdus];
  //  pdu->rx_ue_information.handle          = eNB->ulsch[UE_id]->handle;
  pdu->rx_ue_information.tl.tag          = NFAPI_RX_UE_INFORMATION_TAG;
  pdu->rx_ue_information.rnti            = eNB->ulsch[UE_id]->rnti;
  pdu->rx_indication_rel8.tl.tag         = NFAPI_RX_INDICATION_REL8_TAG;
  pdu->rx_indication_rel8.length         = eNB->ulsch[UE_id]->harq_processes[harq_pid]->TBS>>3;
  pdu->rx_indication_rel8.offset         = 1;   // DJP - I dont understand - but broken unless 1 ????  0;  // filled in at the end of the UL_INFO formation
  AssertFatal(pdu->rx_indication_rel8.length <= NFAPI_RX_IND_DATA_MAX, "Invalid PDU len %d\n",
              pdu->rx_indication_rel8.length);
  memcpy(pdu->rx_ind_data,
         eNB->ulsch[UE_id]->harq_processes[harq_pid]->decodedBytes,
         pdu->rx_indication_rel8.length);
  // estimate timing advance for MAC
  timing_advance_update                  = ul_propa[UE_id].ta;

  //  if (timing_advance_update > 10) { dump_ulsch(eNB,frame,subframe,UE_id); exit(-1);}
  //  if (timing_advance_update < -10) { dump_ulsch(eNB,frame,subframe,UE_id); exit(-1);}
  switch (eNB->frame_parms.N_RB_DL) {
    case 6:                      /* nothing to do */
      break;

    case 15:
      timing_advance_update /= 2;
      break;

    case 25:
      timing_advance_update /= 4;
      break;

    case 50:
      timing_advance_update /= 8;
      break;

    case 75:
      timing_advance_update /= 12;
      break;

    case 100:
      timing_advance_update /= 16;
      break;

    default:
      abort ();
  }

  // put timing advance command in 0..63 range
  timing_advance_update += 31;

  if (timing_advance_update < 0)
    timing_advance_update = 0;

  if (timing_advance_update > 63)
    timing_advance_update = 63;

  pdu->rx_indication_rel8.timing_advance = timing_advance_update;
  // estimate UL_CQI for MAC (from antenna port 0 only)
  int SNRtimes10 = dB_fixed_times10(eNB->pusch_vars[UE_id]->ulsch_power[0]) - 10 * eNB->measurements.n0_subband_power_dB[0][0];

  if (SNRtimes10 < -640)
    pdu->rx_indication_rel8.ul_cqi = 0;
  else if (SNRtimes10 > 635)
    pdu->rx_indication_rel8.ul_cqi = 255;
  else
    pdu->rx_indication_rel8.ul_cqi = (640 + SNRtimes10) / 5;

  LOG_D(PHY,"[PUSCH %d] Frame %d Subframe %d Filling RX_indication with SNR %d (%d), timing_advance %d (update %d)\n",
        harq_pid,frame,subframe,SNRtimes10,pdu->rx_indication_rel8.ul_cqi,pdu->rx_indication_rel8.timing_advance,
        timing_advance_update);
  eNB->UL_INFO.rx_ind.rx_indication_body.number_of_pdus++;
  eNB->UL_INFO.rx_ind.sfn_sf = frame<<4 | subframe;
  pthread_mutex_unlock(&eNB->UL_INFO_mutex);
}

void pusch_procedures_fromsplit(uint8_t *bufferZone, int bufSize, PHY_VARS_eNB *eNB, L1_rxtx_proc_t *proc, ul_propagation_t *ul_propa) {
  //LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  const int subframe = proc->subframe_rx;
  const int frame    = proc->frame_rx;
  uint32_t harq_pid;
  uint32_t harq_pid0 = subframe2harq_pid(&eNB->frame_parms,frame,subframe);

  for (int i = 0; i < NUMBER_OF_UE_MAX; i++) {
    LTE_eNB_ULSCH_t *ulsch = eNB->ulsch[i];

    if (ulsch->ue_type > NOCE) harq_pid = 0;
    else harq_pid=harq_pid0;

    LTE_UL_eNB_HARQ_t *ulsch_harq = ulsch->harq_processes[harq_pid];

    if (ulsch->rnti>0)
      LOG_D(PHY,"eNB->ulsch[%d]->harq_processes[harq_pid:%d] SFN/SF:%04d%d: PUSCH procedures, UE %d/%x ulsch_harq[status:%d SFN/SF:%04d%d handled:%d]\n",
            i, harq_pid, frame,subframe,i,ulsch->rnti,
            ulsch_harq->status, ulsch_harq->frame, ulsch_harq->subframe, ulsch_harq->handled);

    if ((ulsch) &&
        (ulsch->rnti>0) &&
        (ulsch_harq->status == ACTIVE) &&
        (ulsch_harq->frame == frame) &&
        (ulsch_harq->subframe == subframe) &&
        (ulsch_harq->handled == 0)) {
      // UE has ULSCH scheduling
      for (int rb=0;
           rb<=ulsch_harq->nb_rb;
           rb++) {
        int rb2 = rb+ulsch_harq->first_rb;
        eNB->rb_mask_ul[rb2>>5] |= (1<<(rb2&31));
      }

      start_meas(&eNB->ulsch_decoding_stats);
      // This is a new packet, so compute quantities regarding segmentation
      ulsch_harq->B = ulsch_harq->TBS+24;
      lte_segmentation(NULL,
                       NULL,
                       ulsch_harq->B,
                       &ulsch_harq->C,
                       &ulsch_harq->Cplus,
                       &ulsch_harq->Cminus,
                       &ulsch_harq->Kplus,
                       &ulsch_harq->Kminus,
                       &ulsch_harq->F);
      ulsch_decoding_data(eNB, proc, i, harq_pid,
                          ulsch_harq->nb_rb>20 ? 1 : 0);
      stop_meas(&eNB->ulsch_decoding_stats);
    } //     if ((ulsch) &&
    //         (ulsch->rnti>0) &&
    //         (ulsch_harq->status == ACTIVE))
    else if ((ulsch) &&
             (ulsch->rnti>0) &&
             (ulsch_harq->status == ACTIVE) &&
             (ulsch_harq->frame == frame) &&
             (ulsch_harq->subframe == subframe) &&
             (ulsch_harq->handled == 1)) {
      // this harq process is stale, kill it, this 1024 frames later (10s), consider reducing that
      ulsch_harq->status = SCH_IDLE;
      ulsch_harq->handled = 0;
      ulsch->harq_mask &= ~(1 << harq_pid);
      LOG_W (PHY, "Removing stale ULSCH config for UE %x harq_pid %d (harq_mask is now 0x%2.2x)\n", ulsch->rnti, harq_pid, ulsch->harq_mask);
    }
  }   //   for (i=0; i<NUMBER_OF_UE_MAX; i++)

  while (proc->nbDecode > 0) {
    notifiedFIFO_elt_t *req=pullTpool(proc->respDecode, proc->threadPool);
    postDecode(proc, req);
    delNotifiedFIFO_elt(req);
  }
}

void recvFs6Ul(uint8_t *bufferZone, int nbBlocks, PHY_VARS_eNB *eNB, ul_propagation_t *ul_propa) {
  void *bufPtr=bufferZone;

  for (int i=0; i < nbBlocks; i++) { //nbBlocks is the actual received blocks
    if ( ((commonUDP_t *)bufPtr)->contentBytes > sizeof(fs6_ul_t) ) {
      int type=hULUE(bufPtr)->type;

      if ( type == fs6ULsch)  {
        LTE_eNB_ULSCH_t *ulsch =eNB->ulsch[hULUE(bufPtr)->UE_id];
        LTE_UL_eNB_HARQ_t *ulsch_harq=ulsch->harq_processes[hULUE(bufPtr)->harq_id];
        memcpy(ulsch_harq->eUL+hULUE(bufPtr)->r_offset,
               hULUE(bufPtr)+1,
               hULUE(bufPtr)->segLen);
        memcpy(eNB->pusch_vars[hULUE(bufPtr)->UE_id]->ulsch_power,
               hULUE(bufPtr)->ulsch_power,
               sizeof(int)*2);
        ulsch_harq->G=hULUE(bufPtr)->G;
        ulsch_harq->cqi_crc_status=hULUE(bufPtr)->cqi_crc_status;
        //ulsch_harq->O_ACK= hULUE(bufPtr)->O_ACK;
        memcpy(ulsch_harq->o_ACK, hULUE(bufPtr)->o_ACK,
               sizeof(ulsch_harq->o_ACK));
        memcpy(ulsch_harq->o,hULUE(bufPtr)->o, sizeof(ulsch_harq->o));
        ul_propa[hULUE(bufPtr)->UE_id].ta=hULUE(bufPtr)->ta;
        LOG_D(PHY,"Received ulsch data for: rnti:%x, cqi_crc_status %d O_ACK: %d, segment: %d, seglen: %d  \n",
              ulsch->rnti, ulsch_harq->cqi_crc_status, ulsch_harq->O_ACK,hULUE(bufPtr)->segment, hULUE(bufPtr)->segLen);
      } else if ( type == fs6ULcch ) {
        int nb_uci=hULUEuci(bufPtr)->nb_active_ue;
        fs6_ul_uespec_uci_element_t *tmp=(fs6_ul_uespec_uci_element_t *)(hULUEuci(bufPtr)+1);

        for (int j=0; j < nb_uci ; j++) {
          LOG_D(PHY,"FS6 cu, block: %d/%d: received ul harq/sr: %d, rnti: %d, ueid: %d\n",
                i, j, type, tmp->rnti, tmp->UEid);
          eNB->measurements.n0_subband_power_dB[0][0]=tmp->n0_subband_power_dB;

          if (tmp->uci.ue_id != 0xFFFF)
            memcpy(&eNB->uci_vars[tmp->UEid],&tmp->uci, sizeof(tmp->uci));

          if ( tmp->type == fs6ULindicationHarq )
            fill_uci_harq_indication (tmp->UEid, eNB, &eNB->uci_vars[tmp->UEid],
                                      tmp->frame, tmp->subframe, tmp->harq_ack,
                                      tmp->tdd_mapping_mode, tmp->tdd_multiplexing_mask);
          else if ( tmp->type == fs6ULindicationSr )
            fill_sr_indication(tmp->UEid, eNB,tmp->rnti,tmp->frame,tmp->subframe,tmp->stat);
          else
            LOG_E(PHY, "Split FS6: impossible UL harq type\n");

          tmp++;
        }
      } else
        LOG_E(PHY, "FS6 ul packet type impossible\n" );
    }

    bufPtr+=alignedSize(bufPtr);
  }
}

void phy_procedures_eNB_uespec_RX_fromsplit(uint8_t *bufferZone, int nbBlocks,PHY_VARS_eNB *eNB, L1_rxtx_proc_t *proc) {
  // The configuration arrived in Dl, so we can extract the UL data
  ul_propagation_t ul_propa[NUMBER_OF_UE_MAX];
  recvFs6Ul(bufferZone, nbBlocks, eNB, ul_propa);

  // dirty memory allocation in OAI...
  for (int i = 0; i < NUMBER_OF_UCI_MAX; i++)
    if ( eNB->uci_vars[i].frame == proc->frame_rx &&
         eNB->uci_vars[i].subframe == proc->subframe_rx )
      eNB->uci_vars[i].active=0;

  pusch_procedures_fromsplit(bufferZone, nbBlocks, eNB, proc, ul_propa);
}

void rcvFs6DL(uint8_t *bufferZone, int nbBlocks, PHY_VARS_eNB *eNB, int frame, int subframe) {
  void *bufPtr=bufferZone;

  for (int i=0; i < nbBlocks; i++) { //nbBlocks is the actual received blocks
    if ( ((commonUDP_t *)bufPtr)->contentBytes > sizeof(fs6_dl_t) ) {
      int type=hDLUE(bufPtr)->type;

      if ( type == fs6DlConfig) {
        int curUE=hDLUE(bufPtr)->UE_id;
        LTE_eNB_DLSCH_t *dlsch0 = eNB->dlsch[curUE][0];
        LTE_DL_eNB_HARQ_t *dlsch_harq=dlsch0->harq_processes[hDLUE(bufPtr)->harq_pid];
#ifdef PHY_TX_THREAD
        dlsch0->active[subframe] = 1;
#else
        dlsch0->active = 1;
#endif
        dlsch0->harq_ids[frame%2][subframe]=hDLUE(bufPtr)->harq_pid;
        dlsch0->rnti=hDLUE(bufPtr)->rnti;
        dlsch0->sqrt_rho_a=hDLUE(bufPtr)->sqrt_rho_a;
        dlsch0->sqrt_rho_b=hDLUE(bufPtr)->sqrt_rho_b;
        dlsch_harq->nb_rb=hDLUE(bufPtr)->nb_rb;
        memcpy(dlsch_harq->rb_alloc, hDLUE(bufPtr)->rb_alloc, sizeof(hDLUE(bufPtr)->rb_alloc));
        dlsch_harq->Qm=hDLUE(bufPtr)->Qm;
        dlsch_harq->Nl=hDLUE(bufPtr)->Nl;
        dlsch_harq->pdsch_start=hDLUE(bufPtr)->pdsch_start;
#ifdef PHY_TX_THREAD
        dlsch_harq->CEmode = hDLUE(bufPtr)->CEmode;
        dlsch_harq->i0=hDLUE(bufPtr)->i0;
        dlsch_harq->sib1_br_flag=hDLUE(bufPtr)->sib1_br_flag;
#else
        dlsch0->i0=hDLUE(bufPtr)->i0;
        dlsch0->sib1_br_flag=hDLUE(bufPtr)->sib1_br_flag;
#endif
        fs6Dlunpack(dlsch_harq->eDL,
                    hDLUE(bufPtr)+1, hDLUE(bufPtr)->dataLen);
        LOG_D(PHY,"received %d bits, in harq id: %di fsf: %d.%d, sum %d\n",
              hDLUE(bufPtr)->dataLen, hDLUE(bufPtr)->harq_pid, frame, subframe, sum(dlsch_harq->eDL, hDLUE(bufPtr)->dataLen));
      } else if (type == fs6UlConfig) {
        int nbUE=(((commonUDP_t *)bufPtr)->contentBytes - sizeof(fs6_dl_t)) / sizeof( fs6_dl_ulsched_t ) ;
#define cpyVal(a) memcpy(&ulsch_harq->a,&hTxULUE(bufPtr)->a, sizeof(ulsch_harq->a))

        for ( int i=0; i < nbUE; i++ ) {
          int curUE=hTxULUE(bufPtr)->UE_id;
          LTE_eNB_ULSCH_t *ulsch = eNB->ulsch[curUE];
          LTE_UL_eNB_HARQ_t *ulsch_harq=ulsch->harq_processes[hTxULUE(bufPtr)->harq_pid];
          ulsch->ue_type=hTxULUE(bufPtr)->ue_type;
          ulsch->harq_mask=hTxULUE(bufPtr)->harq_mask;
          ulsch->Mlimit=hTxULUE(bufPtr)->Mlimit;
          ulsch->max_turbo_iterations=hTxULUE(bufPtr)->max_turbo_iterations;
          ulsch->bundling=hTxULUE(bufPtr)->bundling;
          ulsch->beta_offset_cqi_times8=hTxULUE(bufPtr)->beta_offset_cqi_times8;
          ulsch->beta_offset_ri_times8=hTxULUE(bufPtr)->beta_offset_ri_times8;
          ulsch->beta_offset_harqack_times8=hTxULUE(bufPtr)->beta_offset_harqack_times8;
          ulsch->Msg3_active=hTxULUE(bufPtr)->Msg3_active;
          ulsch->cyclicShift=hTxULUE(bufPtr)->cyclicShift;
          ulsch->cooperation_flag=hTxULUE(bufPtr)->cooperation_flag;
          ulsch->num_active_cba_groups=hTxULUE(bufPtr)->num_active_cba_groups;
          memcpy(ulsch->cba_rnti,hTxULUE(bufPtr)->cba_rnti,sizeof(ulsch->cba_rnti));//NUM_MAX_CBA_GROUP];
          ulsch->rnti=hTxULUE(bufPtr)->rnti;
          ulsch_harq->nb_rb=hTxULUE(bufPtr)->nb_rb;
          ulsch_harq->handled=0;
          ulsch_harq->status = ACTIVE;
          ulsch_harq->frame = frame;
          ulsch_harq->subframe = subframe;
          ulsch_harq->first_rb=hTxULUE(bufPtr)->first_rb;
          ulsch_harq->O_RI=hTxULUE(bufPtr)->O_RI;
          ulsch_harq->Or1=hTxULUE(bufPtr)->Or1;
          ulsch_harq->Msc_initial=hTxULUE(bufPtr)->Msc_initial;
          ulsch_harq->Nsymb_initial=hTxULUE(bufPtr)->Nsymb_initial;
          ulsch_harq->V_UL_DAI=hTxULUE(bufPtr)->V_UL_DAI;
          ulsch_harq->Qm=hTxULUE(bufPtr)->Qm;
          ulsch_harq->srs_active=hTxULUE(bufPtr)->srs_active;
          ulsch_harq->TBS=hTxULUE(bufPtr)->TBS;
          ulsch_harq->Nsymb_pusch=hTxULUE(bufPtr)->Nsymb_pusch;
          cpyVal(dci_alloc);
          cpyVal(rar_alloc);
          cpyVal(status);
          cpyVal(Msg3_flag);
          cpyVal(phich_active);
          cpyVal(phich_ACK);
          cpyVal(previous_first_rb);
          cpyVal(B);
          cpyVal(G);
          //cpyVal(o);
          cpyVal(uci_format);
          cpyVal(Or2);
          cpyVal(o_RI);
          cpyVal(o_ACK);
          cpyVal(O_ACK);
          //cpyVal(q);
          cpyVal(o_RCC);
          cpyVal(q_ACK);
          cpyVal(q_RI);
          cpyVal(RTC);
          cpyVal(ndi);
          cpyVal(round);
          cpyVal(rvidx);
          cpyVal(Nl);
          cpyVal(n_DMRS);
          cpyVal(previous_n_DMRS);
          cpyVal(n_DMRS2);
          cpyVal(delta_TF);
          cpyVal(repetition_number );
          cpyVal(total_number_of_repetitions);
          LOG_D(PHY,"Received request to perform ulsch for: rnti:%d, fsf: %d/%d, O_ACK: %d\n",
                ulsch->rnti, frame, subframe, ulsch_harq->O_ACK);
        }
      } else if ( type == fs6ULConfigCCH ) {
        fs6_dl_uespec_ulcch_element_t *tmp=(fs6_dl_uespec_ulcch_element_t *)(hTxULcch(bufPtr)+1);

        for (int i=0; i< hTxULcch(bufPtr)->nb_active_ue; i++ )
          memcpy(&eNB->uci_vars[tmp->UE_id], &tmp->cch_vars, sizeof(tmp->cch_vars));
      }  else
        LOG_E(PHY, "Impossible block in fs6 DL\n");
    }

    bufPtr+=alignedSize(bufPtr);
  }
}

void phy_procedures_eNB_TX_fromsplit(uint8_t *bufferZone, int nbBlocks, PHY_VARS_eNB *eNB, L1_rxtx_proc_t *proc, int do_meas ) {
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  int subframe=proc->subframe_tx;
  int frame=proc->frame_tx;
  //LTE_UL_eNB_HARQ_t *ulsch_harq;
  eNB->pdcch_vars[subframe&1].num_pdcch_symbols=hDL(bufferZone)->num_pdcch_symbols;
  eNB->pdcch_vars[subframe&1].num_dci=hDL(bufferZone)->num_dci;
  uint8_t num_mdci = eNB->mpdcch_vars[subframe&1].num_dci = hDL(bufferZone)->num_mdci;
  eNB->pbch_configured=true;
  memcpy(eNB->pbch_pdu,hDL(bufferZone)->pbch_pdu, 4);

  // Remove all scheduled DL, we will populate from the CU sending
  for (int UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) {
    LTE_eNB_DLSCH_t *dlsch0 = eNB->dlsch[UE_id][0];

    if ( dlsch0 && dlsch0->rnti>0 ) {
#ifdef PHY_TX_THREAD
      dlsch0->active[subframe] = 0;
#else
      dlsch0->active = 0;
#endif
    }
  }

  rcvFs6DL(bufferZone, nbBlocks, eNB, frame, subframe);

  if (do_meas==1) {
    start_meas(&eNB->phy_proc_tx);
    start_meas(&eNB->dlsch_common_and_dci);
  }

  // clear the transmit data array for the current subframe
  for (int aa = 0; aa < fp->nb_antenna_ports_eNB; aa++) {
    memset (&eNB->common_vars.txdataF[aa][subframe * fp->ofdm_symbol_size * (fp->symbols_per_tti)],
            0, fp->ofdm_symbol_size * (fp->symbols_per_tti) * sizeof (int32_t));
  }

  if (NFAPI_MODE==NFAPI_MONOLITHIC || NFAPI_MODE==NFAPI_MODE_PNF) {
    if (is_pmch_subframe(frame,subframe,fp)) {
      pmch_procedures(eNB,proc,0);
    } else {
      // this is not a pmch subframe, so generate PSS/SSS/PBCH
      common_signal_procedures(eNB,frame, subframe);
    }
  }

  // clear previous allocation information for all UEs
  for (int i = 0; i < NUMBER_OF_UE_MAX; i++) {
    //if (eNB->dlsch[i][0])
    //eNB->dlsch[i][0]->subframe_tx[subframe] = 0;
  }

  if (NFAPI_MODE==NFAPI_MONOLITHIC || NFAPI_MODE==NFAPI_MODE_PNF) {
    for (int i=0; i< hDL(bufferZone)->num_dci; i++)
      eNB->pdcch_vars[subframe&1].dci_alloc[i]=hDL(bufferZone)->dci_alloc[i];

    LOG_D (PHY, "Frame %d, subframe %d: Calling generate_dci_top (pdcch) (num_dci %" PRIu8 ")\n", frame, subframe, hDL(bufferZone)->num_dci);
    generate_dci_top(hDL(bufferZone)->num_pdcch_symbols,
                     hDL(bufferZone)->num_dci,
                     &eNB->pdcch_vars[subframe&1].dci_alloc[0],
                     0,
                     hDL(bufferZone)->amp,
                     fp,
                     eNB->common_vars.txdataF,
                     subframe);

    if (num_mdci > 0) {
      LOG_D (PHY, "[eNB %" PRIu8 "] Frame %d, subframe %d: Calling generate_mdci_top (mpdcch) (num_dci %" PRIu8 ")\n", eNB->Mod_id, frame, subframe, num_mdci);
      generate_mdci_top (eNB, frame, subframe, AMP, eNB->common_vars.txdataF);
    }
  }

  for (int UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) {
    LTE_eNB_DLSCH_t *dlsch0 = eNB->dlsch[UE_id][0];
    LTE_eNB_DLSCH_t *dlsch1 = eNB->dlsch[UE_id][1];

    if ((dlsch0)&&(dlsch0->rnti>0)&&
#ifdef PHY_TX_THREAD
        (dlsch0->active[subframe] == 1)
#else
        (dlsch0->active == 1)
#endif
       ) {
      uint64_t sum=0;

      for ( int i= subframe * fp->ofdm_symbol_size * (fp->symbols_per_tti);
            i< (subframe+1) * fp->ofdm_symbol_size * (fp->symbols_per_tti);
            i++)
        sum+=((int32_t *)(eNB->common_vars.txdataF[0]))[i];

      LOG_D(PHY,"frame: %d, subframe: %d, sum of dlsch mod v1: %lx\n", frame, subframe, sum);
      int harq_pid=dlsch0->harq_ids[frame%2][subframe];
      pdsch_procedures(eNB,
                       proc,
                       harq_pid,
                       dlsch0,
                       dlsch1);
    }
  }

  eNB->phich_vars[subframe&1]=hDL(bufferZone)->phich_vars;
  generate_phich_top(eNB,
                     proc,
                     AMP);
}

#define cpyToDu(a) hTxULUE(newUDPheader)->a=ulsch->a
#define cpyToDuHarq(a) hTxULUE(newUDPheader)->a=ulsch_harq->a
#define memcpyToDuHarq(a) memcpy(&hTxULUE(newUDPheader)->a,&ulsch_harq->a, sizeof(ulsch_harq->a));

void appendFs6TxULUE(uint8_t *bufferZone, LTE_DL_FRAME_PARMS *fp, int curUE, LTE_eNB_ULSCH_t *ulsch, int frame, int subframe) {
  commonUDP_t *FirstUDPheader=(commonUDP_t *) bufferZone;
  // move to the end
  uint8_t *firstFreeByte=bufferZone;
  int curBlock=0;

  for (int i=0; i < FirstUDPheader->nbBlocks; i++) {
    AssertFatal( ((commonUDP_t *) firstFreeByte)->blockID==curBlock,"");
    firstFreeByte+=alignedSize(firstFreeByte);
    curBlock++;
  }

  commonUDP_t *newUDPheader=(commonUDP_t *) firstFreeByte;
  FirstUDPheader->nbBlocks++;
  newUDPheader->blockID=curBlock;
  newUDPheader->contentBytes=sizeof(fs6_dl_t)+sizeof(fs6_dl_ulsched_t);
  // We skip the fs6 DL header, that is populated by caller
  // This header will be duplicated during sending
  hTxULUE(newUDPheader)->type=fs6UlConfig;
  hTxULUE(newUDPheader)->UE_id=curUE;
  int harq_pid;

  if (ulsch->ue_type > NOCE)
    // LTE-M case
    harq_pid = 0;
  else
    harq_pid = subframe2harq_pid(fp, frame, subframe);

  LTE_UL_eNB_HARQ_t *ulsch_harq=ulsch->harq_processes[harq_pid];
  hTxULUE(newUDPheader)->harq_pid=harq_pid;
  cpyToDu(ue_type);
  cpyToDu(harq_mask);
  cpyToDu(Mlimit);
  cpyToDu(max_turbo_iterations);
  cpyToDu(bundling);
  cpyToDu(beta_offset_cqi_times8);
  cpyToDu(beta_offset_ri_times8);
  cpyToDu(beta_offset_harqack_times8);
  cpyToDu(Msg3_active);
  cpyToDu(cyclicShift);
  cpyToDu(cooperation_flag);
  cpyToDu(num_active_cba_groups);
  memcpy(hTxULUE(newUDPheader)->cba_rnti,ulsch->cba_rnti,sizeof(ulsch->cba_rnti));//NUM_MAX_CBA_GROUP];
  cpyToDu(rnti);
  cpyToDuHarq(nb_rb);
  cpyToDuHarq(Msc_initial);
  cpyToDuHarq(Nsymb_initial);
  cpyToDuHarq(O_RI);
  cpyToDuHarq(Or1);
  cpyToDuHarq(first_rb);
  cpyToDuHarq(V_UL_DAI);
  cpyToDuHarq(Qm);
  cpyToDuHarq(srs_active);
  cpyToDuHarq(TBS);
  cpyToDuHarq(Nsymb_pusch);
  memcpyToDuHarq(dci_alloc);
  memcpyToDuHarq(rar_alloc);
  memcpyToDuHarq(status);
  memcpyToDuHarq(Msg3_flag);
  memcpyToDuHarq(phich_active);
  memcpyToDuHarq(phich_ACK);
  memcpyToDuHarq(previous_first_rb);
  memcpyToDuHarq(B);
  memcpyToDuHarq(G);
  //memcpyToDuHarq(o);
  memcpyToDuHarq(uci_format);
  memcpyToDuHarq(Or2);
  memcpyToDuHarq(o_RI);
  memcpyToDuHarq(o_ACK);
  memcpyToDuHarq(O_ACK);
  //memcpyToDuHarq(q);
  memcpyToDuHarq(o_RCC);
  memcpyToDuHarq(q_ACK);
  memcpyToDuHarq(q_RI);
  memcpyToDuHarq(RTC);
  memcpyToDuHarq(ndi);
  memcpyToDuHarq(round);
  memcpyToDuHarq(rvidx);
  memcpyToDuHarq(Nl);
  memcpyToDuHarq(n_DMRS);
  memcpyToDuHarq(previous_n_DMRS);
  memcpyToDuHarq(n_DMRS2);
  memcpyToDuHarq(delta_TF);
  memcpyToDuHarq(repetition_number );
  memcpyToDuHarq(total_number_of_repetitions);
  LOG_D(PHY,"Added request to perform ulsch for: rnti:%x, fsf: %d/%d\n", ulsch->rnti, frame, subframe);
}

void appendFs6DLUE(uint8_t *bufferZone, LTE_DL_FRAME_PARMS *fp, int UE_id, int8_t harq_pid, LTE_eNB_DLSCH_t *dlsch0, LTE_DL_eNB_HARQ_t *harqData, int frame, int subframe) {
  commonUDP_t *FirstUDPheader=(commonUDP_t *) bufferZone;
  // move to the end
  uint8_t *firstFreeByte=bufferZone;
  int curBlock=0;

  for (int i=0; i < FirstUDPheader->nbBlocks; i++) {
    AssertFatal( ((commonUDP_t *) firstFreeByte)->blockID==curBlock,"");
    firstFreeByte+=alignedSize(firstFreeByte);
    curBlock++;
  }

  int UEdataLen= get_G(fp,
                       harqData->nb_rb,
                       harqData->rb_alloc,
                       harqData->Qm,
                       harqData->Nl,
                       harqData->pdsch_start,
                       frame,subframe,
                       0);
  AssertFatal(firstFreeByte+ceil16_bytes(UEdataLen)+sizeof(fs6_dl_t) <= bufferZone+FS6_BUF_SIZE, "");
  commonUDP_t *newUDPheader=(commonUDP_t *) firstFreeByte;
  FirstUDPheader->nbBlocks++;
  newUDPheader->blockID=curBlock;
  newUDPheader->contentBytes=sizeof(fs6_dl_t)+sizeof(fs6_dl_uespec_t) + ceil16_bytes(UEdataLen);
  // We skip the fs6 DL header, that is populated by caller
  // This header will be duplicated during sending
  hDLUE(newUDPheader)->type=fs6DlConfig;
  hDLUE(newUDPheader)->UE_id=UE_id;
  hDLUE(newUDPheader)->harq_pid=harq_pid;
  hDLUE(newUDPheader)->rnti=dlsch0->rnti;
  hDLUE(newUDPheader)->sqrt_rho_a=dlsch0->sqrt_rho_a;
  hDLUE(newUDPheader)->sqrt_rho_b=dlsch0->sqrt_rho_b;
  hDLUE(newUDPheader)->nb_rb=harqData->nb_rb;
  memcpy(hDLUE(newUDPheader)->rb_alloc, harqData->rb_alloc, sizeof(harqData->rb_alloc));
  hDLUE(newUDPheader)->Qm=harqData->Qm;
  hDLUE(newUDPheader)->Nl=harqData->Nl;
  hDLUE(newUDPheader)->pdsch_start=harqData->pdsch_start;
#ifdef PHY_TX_THREAD
  hDLUE(newUDPheader)->CEmode=harqData->CEmode;
  hDLUE(newUDPheader)->i0=harqData->i0;
  hDLUE(newUDPheader)->sib1_br_flag=harqData->sib1_br_flag;
#else
  hDLUE(newUDPheader)->i0=dlsch0->i0;
  hDLUE(newUDPheader)->sib1_br_flag=dlsch0->sib1_br_flag;
#endif
  hDLUE(newUDPheader)->dataLen=UEdataLen;
  fs6Dlpack(hDLUE(newUDPheader)+1, harqData->eDL, UEdataLen);
  LOG_D(PHY,"sending %d bits, in harq id: %di fsf: %d.%d, sum %d\n",
        UEdataLen, harq_pid, frame, subframe, sum(harqData->eDL, UEdataLen));
  //for (int i=0; i < UEdataLen; i++)
  //LOG_D(PHY,"buffer ei[%d]:%hhx\n", i, ( (uint8_t *)(hDLUE(newUDPheader)+1) )[i]);
}

void appendFs6DLUEcch(uint8_t *bufferZone, PHY_VARS_eNB *eNB, int frame, int subframe) {
  commonUDP_t *FirstUDPheader=(commonUDP_t *) bufferZone;
  // move to the end
  uint8_t *firstFreeByte=bufferZone;
  int curBlock=0;

  for (int i=0; i < FirstUDPheader->nbBlocks; i++) {
    AssertFatal( ((commonUDP_t *) firstFreeByte)->blockID==curBlock,"");
    firstFreeByte+=alignedSize(firstFreeByte);
    curBlock++;
  }

  commonUDP_t *newUDPheader=(commonUDP_t *) firstFreeByte;
  bool first_UE=true;

  for (int i = 0; i < NUMBER_OF_UCI_MAX; i++) {
    LTE_eNB_UCI *uci = &(eNB->uci_vars[i]);

    if ((uci->active == 1) && (uci->frame == frame) && (uci->subframe == subframe)) {
      LOG_D(PHY,"Frame %d, subframe %d: adding uci procedures (type %d) for %d \n",
            frame,
            subframe,
            uci->type,
            i);

      if ( first_UE ) {
        FirstUDPheader->nbBlocks++;
        newUDPheader->blockID=curBlock;
        newUDPheader->contentBytes=sizeof(fs6_dl_t)+sizeof(fs6_dl_uespec_ulcch_t);
        hTxULcch(newUDPheader)->type=fs6ULConfigCCH;
        hTxULcch(newUDPheader)->nb_active_ue=0;
        first_UE=false;
      }

      fs6_dl_uespec_ulcch_element_t *tmp=(fs6_dl_uespec_ulcch_element_t *)(hTxULcch(newUDPheader)+1);
      tmp+=hTxULcch(newUDPheader)->nb_active_ue;
      tmp->UE_id=i;
      memcpy(&tmp->cch_vars,uci, sizeof(tmp->cch_vars));
      hTxULcch(newUDPheader)->nb_active_ue++;
      newUDPheader->contentBytes+=sizeof(fs6_dl_uespec_ulcch_element_t);
    }
  }
}

void phy_procedures_eNB_TX_tosplit(uint8_t *bufferZone, PHY_VARS_eNB *eNB, L1_rxtx_proc_t *proc, int do_meas, uint8_t *buf, int bufSize) {
  int frame=proc->frame_tx;
  int subframe=proc->subframe_tx;
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;

  if ((fp->frame_type == TDD) && (subframe_select (fp, subframe) == SF_UL)) {
    LOG_W(HW,"no sending in eNB_TX\n");
    return;
  }

  // clear previous allocation information for all UEs
  for (int i = 0; i < NUMBER_OF_UE_MAX; i++) {
    //if (eNB->dlsch[i][0])
    //eNB->dlsch[i][0]->subframe_tx[subframe] = 0;
  }

  // Send to DU the UL scheduled for future UL subframe
  for (int i=0; i<NUMBER_OF_UE_MAX; i++) {
    int harq_pid;
    LTE_eNB_ULSCH_t *ulsch = eNB->ulsch[i];
    if (ulsch == NULL)
      continue;

    if (ulsch->ue_type > NOCE)
      harq_pid = 0;
    else
      harq_pid= subframe2harq_pid(&eNB->frame_parms,frame,subframe);

    LTE_UL_eNB_HARQ_t *ulsch_harq = ulsch->harq_processes[harq_pid];

    if (ulsch->rnti>0) {
      LOG_D(PHY,"check in UL scheduled harq %d: rnti %d, tx frame %d/%d, ulsch: %d, %d/%d (handled: %d)\n",
            harq_pid, ulsch->rnti, frame, subframe, ulsch_harq->status, ulsch_harq->frame, ulsch_harq->subframe, ulsch_harq->handled);
    }

    for (int k=0; k<8; k++) {
      ulsch_harq = ulsch->harq_processes[k];
      if (ulsch_harq == NULL)
        continue;

      if ((ulsch->rnti>0) &&
          (ulsch_harq->status == ACTIVE) &&
          (ulsch_harq->frame == frame) &&
          (ulsch_harq->subframe == subframe) &&
          (ulsch_harq->handled == 0)
         )
        appendFs6TxULUE(bufferZone,
                        fp,
                        i,
                        ulsch,
                        frame,
                        subframe
                       );
    }
  }

  appendFs6DLUEcch(bufferZone,
                   eNB,
                   frame,
                   subframe
                  );
  uint8_t num_pdcch_symbols = eNB->pdcch_vars[subframe&1].num_pdcch_symbols;
  uint8_t num_dci           = eNB->pdcch_vars[subframe&1].num_dci;
  uint8_t num_mdci          = eNB->mpdcch_vars[subframe&1].num_dci;
  memcpy(hDL(bufferZone)->pbch_pdu,eNB->pbch_pdu,4);

  if ( num_dci <= 8 )
    LOG_D(PHY,"num_pdcch_symbols %"PRIu8",number dci %"PRIu8"\n",num_pdcch_symbols, num_dci);
  else {
    LOG_E(PHY, "Num dci too large for current FS6 implementation, reducing to 8 dci (was %d)\n",  num_dci);
    num_dci=8;
  }

  if (NFAPI_MODE==NFAPI_MONOLITHIC || NFAPI_MODE==NFAPI_MODE_PNF) {
    hDL(bufferZone)->num_pdcch_symbols=num_pdcch_symbols;
    hDL(bufferZone)->num_dci=num_dci;
    hDL(bufferZone)->num_mdci=num_mdci;
    hDL(bufferZone)->amp=AMP;

    for (int i=0; i< hDL(bufferZone)->num_dci; i++)
      hDL(bufferZone)->dci_alloc[i]=eNB->pdcch_vars[subframe&1].dci_alloc[i];

    LOG_D(PHY, "pbch configured: %d\n", eNB->pbch_configured);
  }

  if (do_meas==1) stop_meas(&eNB->dlsch_common_and_dci);

  if (do_meas==1) start_meas(&eNB->dlsch_ue_specific);

  for (int UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) {
    LTE_eNB_DLSCH_t *dlsch0 = eNB->dlsch[UE_id][0];

    if ((dlsch0)&&(dlsch0->rnti>0)&&
#ifdef PHY_TX_THREAD
        (dlsch0->active[subframe] == 1)
#else
        (dlsch0->active == 1)
#endif
       ) {
      // get harq_pid
      int harq_pid = dlsch0->harq_ids[frame%2][subframe];
      AssertFatal(harq_pid>=0,"harq_pid is negative\n");

      if (harq_pid>=8) {
        if (dlsch0->ue_type == NOCE)
          LOG_E(PHY,"harq_pid:%d corrupt must be 0-7 UE_id:%d frame:%d subframe:%d rnti:%x [ %1d.%1d.%1d.%1d.%1d.%1d.%1d.%1d\n", harq_pid,UE_id,frame,subframe,dlsch0->rnti,
                dlsch0->harq_ids[frame%2][0],
                dlsch0->harq_ids[frame%2][1],
                dlsch0->harq_ids[frame%2][2],
                dlsch0->harq_ids[frame%2][3],
                dlsch0->harq_ids[frame%2][4],
                dlsch0->harq_ids[frame%2][5],
                dlsch0->harq_ids[frame%2][6],
                dlsch0->harq_ids[frame%2][7]);
      } else {
        if (dlsch_procedures(eNB,
                             proc,
                             harq_pid,
                             dlsch0,
                             &eNB->UE_stats[(uint32_t)UE_id])) {
          // data in: dlsch0 harq_processes[harq_pid]->e
          /* length
             get_G(fp,
             dlsch_harq->nb_rb,
             dlsch_harq->rb_alloc,
             dlsch_harq->Qm,
             dlsch_harq->Nl,
             dlsch_harq->pdsch_start,
             frame,subframe,
             0)
             need harq_pid
          */
          LTE_DL_eNB_HARQ_t *dlsch_harq=dlsch0->harq_processes[harq_pid];
          appendFs6DLUE(bufferZone,
                        fp,
                        UE_id,
                        harq_pid,
                        dlsch0,
                        dlsch_harq,
                        frame,
                        subframe
                       );
        }
      }
    } else if ((dlsch0)&&(dlsch0->rnti>0)&&
#ifdef PHY_TX_THREAD
               (dlsch0->active[subframe] == 0)
#else
               (dlsch0->active == 0)
#endif
              ) {
      // clear subframe TX flag since UE is not scheduled for PDSCH in this subframe (so that we don't look for PUCCH later)
      //dlsch0->subframe_tx[subframe]=0;
    }
  }

  hDL(bufferZone)->phich_vars=eNB->phich_vars[subframe&1];

  if (do_meas==1) stop_meas(&eNB->dlsch_ue_specific);

  if (do_meas==1) stop_meas(&eNB->phy_proc_tx);

  // MBMS is not working in OAI
  if (hDL(bufferZone)->num_mdci) abort();

  return;
}

void *DL_du_fs6(void *arg) {
  RU_t *ru=(RU_t *)arg;
  static uint64_t lastTS;
  L1_rxtx_proc_t L1proc= {0};
  // We pick the global thread pool from the legacy code global vars
  L1proc.threadPool=RC.eNB[0][0]->proc.L1_proc.threadPool;
  L1proc.respEncode=RC.eNB[0][0]->proc.L1_proc.respEncode;
  L1proc.respDecode=RC.eNB[0][0]->proc.L1_proc.respDecode;
  initStaticTime(begingWait);
  initStaticTime(begingProcessing);
  initRefTimes(fullLoop);
  initRefTimes(DuHigh);
  initRefTimes(DuLow);
  initRefTimes(transportTime);

  while (1) {
    for (int i=0; i<ru->num_eNB; i++) {
      initBufferZone(bufferZone);
      pickStaticTime(begingWait);
      int nb_blocks=receiveSubFrame(&sockFS6, bufferZone, sizeof(bufferZone), CTsentCUv0 );
      updateTimesReset(begingWait, &fullLoop, 1000, false, "DU wait CU");

      if (nb_blocks > 0) {
        if ( lastTS+ru->eNB_list[i]->frame_parms.samples_per_tti < hUDP(bufferZone)->timestamp) {
          LOG_E(HW,"Missed a subframe: expecting: %lu, received %lu\n",
                lastTS+ru->eNB_list[i]->frame_parms.samples_per_tti,
                hUDP(bufferZone)->timestamp);
        } else if ( lastTS+ru->eNB_list[i]->frame_parms.samples_per_tti > hUDP(bufferZone)->timestamp) {
          LOG_E(HW,"Received a subframe in past time from CU (dropping it): expecting: %lu, received %lu\n",
                lastTS+ru->eNB_list[i]->frame_parms.samples_per_tti,
                hUDP(bufferZone)->timestamp);
        }

        pickStaticTime(begingProcessing);
        lastTS=hUDP(bufferZone)->timestamp;
        setAllfromTS(hUDP(bufferZone)->timestamp - sf_ahead*ru->eNB_list[i]->frame_parms.samples_per_tti, &L1proc);
        measTransportTime(hDL(bufferZone)->DuClock, hDL(bufferZone)->CuSpentMicroSec,
                          &transportTime, 1000, false, "Transport time, to CU + from CU for one subframe");
        phy_procedures_eNB_TX_fromsplit( bufferZone, nb_blocks, ru->eNB_list[i], &L1proc, 1);
        updateTimesReset(begingProcessing, &DuHigh, 1000, false, "DU high layer1 processing for DL");
      } else
        LOG_E(PHY,"DL not received for subframe\n");
    }

    pickStaticTime(begingProcessing);
    feptx_prec(ru, L1proc.frame_tx,L1proc.subframe_tx );
    feptx_ofdm(ru, L1proc.frame_tx,L1proc.subframe_tx );
    ocp_tx_rf(ru, &L1proc);
    updateTimesReset(begingProcessing, &DuLow, 1000, false, "DU low layer1 processing for DL");

    if ( IS_SOFTMODEM_RFSIM )
      return NULL;
  }

  return NULL;
}

void UL_du_fs6(RU_t *ru, L1_rxtx_proc_t *proc) {
  initStaticTime(begingWait);
  initRefTimes(fullLoop);
  pickStaticTime(begingWait);
  rx_rf(ru, proc);
  updateTimesReset(begingWait, &fullLoop, 1000, false, "DU wait USRP");
  // front end processing: convert from time domain to frequency domain
  // fills rxdataF buffer
  fep_full(ru, proc->subframe_rx);
  // Fixme: datamodel issue
  PHY_VARS_eNB *eNB = RC.eNB[0][0];

  if (NFAPI_MODE==NFAPI_MODE_PNF) {
    // I am a PNF and I need to let nFAPI know that we have a (sub)frame tick
    //add_subframe(&frame, &subframe, 4);
    //oai_subframe_ind(proc->frame_tx, proc->subframe_tx);
    oai_subframe_ind(proc->frame_rx, proc->subframe_rx);
  }

  initBufferZone(bufferZone);
  hUDP(bufferZone)->timestamp=proc->timestamp_rx;
  prach_eNB_tosplit(bufferZone, FS6_BUF_SIZE, eNB, proc );

  if (NFAPI_MODE==NFAPI_MONOLITHIC || NFAPI_MODE==NFAPI_MODE_PNF) {
    phy_procedures_eNB_uespec_RX_tosplit(bufferZone, FS6_BUF_SIZE, eNB, proc );
  }

  if (hUDP(bufferZone)->nbBlocks==0) {
    hUDP(bufferZone)->nbBlocks=1; // We have to send the signaling, even is there is no user plan data (no UE)
    hUDP(bufferZone)->blockID=0;
    hUDP(bufferZone)->contentBytes=sizeof(fs6_ul_t);
  }

  for (int i=0; i<ru->num_eNB; i++) {
    sendSubFrame(&sockFS6, bufferZone, sizeof(fs6_ul_t), CTsentDUv0);
  }
}

void DL_cu_fs6(RU_t *ru, L1_rxtx_proc_t *proc, uint64_t  DuClock, uint64_t startCycle) {
  initRefTimes(CUprocessing);
  // Fixme: datamodel issue
  PHY_VARS_eNB *eNB = RC.eNB[0][0];
  pthread_mutex_lock(&eNB->UL_INFO_mutex);
  eNB->UL_INFO.frame     = proc->frame_rx;
  eNB->UL_INFO.subframe  = proc->subframe_rx;
  eNB->UL_INFO.module_id = eNB->Mod_id;
  eNB->UL_INFO.CC_id     = eNB->CC_id;
  eNB->if_inst->UL_indication(&eNB->UL_INFO, proc);
  pthread_mutex_unlock(&eNB->UL_INFO_mutex);
  initBufferZone(bufferZone);
  phy_procedures_eNB_TX_tosplit(bufferZone, eNB, proc, 1, bufferZone, FS6_BUF_SIZE);
  hUDP(bufferZone)->timestamp=proc->timestamp_tx;

  if (hUDP(bufferZone)->nbBlocks==0) {
    hUDP(bufferZone)->nbBlocks=1; // We have to send the signaling, even is there is no user plan data (no UE)
    hUDP(bufferZone)->blockID=0;
    hUDP(bufferZone)->contentBytes=sizeof(fs6_dl_t);
  }

  hDL(bufferZone)->DuClock=DuClock;
  hDL(bufferZone)->CuSpentMicroSec=(rdtsc()-startCycle)/(cpuf*1000);
  updateTimesReset(startCycle, &CUprocessing, 1000,  true,"CU entire processing from recv to send");
  sendSubFrame(&sockFS6, bufferZone, sizeof(fs6_dl_t), CTsentCUv0 );
  return;
}

void UL_cu_fs6(RU_t *ru, L1_rxtx_proc_t *proc, uint64_t *TS, uint64_t *DuClock, uint64_t *startProcessing) {
  initBufferZone(bufferZone);
  initStaticTime(begingWait);
  initRefTimes(fullLoop);
  pickStaticTime(begingWait);
  int nb_blocks=receiveSubFrame(&sockFS6, bufferZone, sizeof(bufferZone), CTsentDUv0 );
  * DuClock=hUDP(bufferZone)->senderClock;
  * startProcessing=rdtsc();
  updateTimesReset(begingWait, &fullLoop, 1000, false, "CU wait DU");

  if (nb_blocks ==0) {
    LOG_W(PHY, "CU lost a subframe\n");
    return;
  }

  if (nb_blocks != hUDP(bufferZone)->nbBlocks )
    LOG_W(PHY, "received %d blocks for %d expected\n", nb_blocks, hUDP(bufferZone)->nbBlocks);

  if ( *TS != hUDP(bufferZone)->timestamp ) {
    LOG_W(HW, "CU received time: %lu instead of %lu expected\n", hUDP(bufferZone)->timestamp, *TS);
    *TS=hUDP(bufferZone)->timestamp;
  }

  setAllfromTS(hUDP(bufferZone)->timestamp, proc);
  PHY_VARS_eNB *eNB = RC.eNB[0][0];

  if (is_prach_subframe(&eNB->frame_parms, proc->frame_prach,proc->subframe_prach)>0)
    prach_eNB_fromsplit(bufferZone, sizeof(bufferZone), eNB, proc);

  release_UE_in_freeList(eNB->Mod_id);

  if (NFAPI_MODE==NFAPI_MONOLITHIC || NFAPI_MODE==NFAPI_MODE_PNF) {
    phy_procedures_eNB_uespec_RX_fromsplit(bufferZone, nb_blocks, eNB, proc);
  }
}

void *cu_fs6(void *arg) {
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);
  RU_t               *ru      = (RU_t *)arg;
  //RU_proc_t          *proc    = &ru->proc;
  fill_rf_config(ru,ru->rf_config_file);
  init_frame_parms(ru->frame_parms,1);
  phy_init_RU(ru);
  wait_sync("ru_thread");
  char remoteIP[1024];
  strncpy(remoteIP,get_softmodem_params()->split73+3, 1023); //three first char should be cu: or du:
  char port_def[256]=DU_PORT;

  for (int i=0; i <1000; i++)
    if (remoteIP[i]==':') {
      strncpy(port_def,remoteIP+i+1,255);
      remoteIP[i]=0;
      break;
    }

  AssertFatal(createUDPsock(NULL, CU_PORT, remoteIP, port_def, &sockFS6), "");
  L1_rxtx_proc_t L1proc= {0};
  // We pick the global thread pool from the legacy code global vars
  L1proc.threadPool=RC.eNB[0][0]->proc.L1_proc.threadPool;
  L1proc.respEncode=RC.eNB[0][0]->proc.L1_proc.respEncode;
  L1proc.respDecode=RC.eNB[0][0]->proc.L1_proc.respDecode;
  uint64_t timeStamp=0;
  initStaticTime(begingWait);
  initStaticTime(begingWait2);
  initRefTimes(waitDUAndProcessingUL);
  initRefTimes(makeSendDL);
  initRefTimes(fullLoop);
  uint64_t DuClock=0, startProcessing=0;

  while(1) {
    timeStamp+=ru->frame_parms->samples_per_tti;
    updateTimesReset(begingWait, &fullLoop, 1000, true, "CU for full SubFrame (must be less 1ms)");
    pickStaticTime(begingWait);
    UL_cu_fs6(ru, &L1proc, &timeStamp, &DuClock, &startProcessing);
    updateTimesReset(begingWait, &waitDUAndProcessingUL, 1000,  true,"CU Time in wait Rx + Ul processing");
    pickStaticTime(begingWait2);
    DL_cu_fs6(ru, &L1proc, DuClock, startProcessing);
    updateTimesReset(begingWait2, &makeSendDL, 1000,  true,"CU Time in DL build+send");
  }

  return NULL;
}

void *du_fs6(void *arg) {
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);
  RU_t               *ru      = (RU_t *)arg;
  //RU_proc_t          *proc    = &ru->proc;
  fill_rf_config(ru,ru->rf_config_file);
  init_frame_parms(ru->frame_parms,1);
  phy_init_RU(ru);
  init_rf(ru);
  wait_sync("ru_thread");
  char remoteIP[1024];
  strncpy(remoteIP,get_softmodem_params()->split73+3,1023); //three first char should be cu: or du:
  char port_def[256]=CU_PORT;

  for (int i=0; i <1000; i++)
    if (remoteIP[i]==':') {
      strncpy(port_def,remoteIP+i+1,255);
      remoteIP[i]=0;
      break;
    }

  AssertFatal(createUDPsock(NULL, DU_PORT, remoteIP, port_def, &sockFS6), "");

  if (ru->rfdevice.trx_start_func(&ru->rfdevice) != 0)
    LOG_E(HW,"Could not start the RF device\n");
  else
    LOG_I(PHY,"RU %d rf device ready\n",ru->idx);

  initStaticTime(begingWait);
  initRefTimes(waitRxAndProcessingUL);
  initRefTimes(fullLoop);
  pthread_t t;

  if ( !IS_SOFTMODEM_RFSIM )
    threadCreate(&t, DL_du_fs6, (void *)ru, "MainDuTx", -1, OAI_PRIORITY_RT_MAX);

  L1_rxtx_proc_t L1proc= {0};
  // We pick the global thread pool from the legacy code global vars
  L1proc.threadPool=RC.eNB[0][0]->proc.L1_proc.threadPool;
  L1proc.respEncode=RC.eNB[0][0]->proc.L1_proc.respEncode;
  L1proc.respDecode=RC.eNB[0][0]->proc.L1_proc.respDecode;

  while(!oai_exit) {
    updateTimesReset(begingWait, &fullLoop, 1000,  true,"DU for full SubFrame (must be less 1ms)");
    pickStaticTime(begingWait);
    UL_du_fs6(ru, &L1proc);

    if ( IS_SOFTMODEM_RFSIM )
      DL_du_fs6((void *)ru);

    updateTimesReset(begingWait, &waitRxAndProcessingUL, 1000,  true,"DU Time in wait Rx + Ul processing");
  }

  ru->rfdevice.trx_end_func(&ru->rfdevice);
  LOG_I(PHY,"RU %d rf device stopped\n",ru->idx);
  return NULL;
}
