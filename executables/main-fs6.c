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

#define FS6_BUF_SIZE 100*1000
static UDPsock_t sockFS6;

void prach_eNB_extract(uint8_t *bufferZone, int bufSize, PHY_VARS_eNB *eNB, int frame,int subframe) {
  //commonUDP_t *UDPheader=(commonUDP_t *) bufferZone;
  fs6_ul_t *header=(fs6_ul_t *) commonUDPdata(bufferZone);

  if (is_prach_subframe(&eNB->frame_parms, frame,subframe)<=0)
    return;

  eNB->proc.frame_prach = frame;
  eNB->proc.subframe_prach = subframe;
  eNB->proc.frame_prach_br = frame;
  eNB->proc.subframe_prach_br = subframe;
  RU_t *ru;
  int aa=0;
  int ru_aa;

  for (int i=0; i<eNB->num_RU; i++) {
    ru=eNB->RU_list[i];

    for (ru_aa=0,aa=0; ru_aa<ru->nb_rx; ru_aa++,aa++) {
      eNB->prach_vars.rxsigF[0][aa] = eNB->RU_list[i]->prach_rxsigF[ru_aa];
      int ce_level;

      for (ce_level=0; ce_level<4; ce_level++)
        eNB->prach_vars_br.rxsigF[ce_level][aa] = eNB->RU_list[i]->prach_rxsigF_br[ce_level][ru_aa];
    }
  }

  /* Fixme: the orgin code calls the two funtiosn, so one overwrite the second?
     rx_prach(eNB,
     eNB->RU_list[0],
     header->max_preamble,
     header->max_preamble_energy,
     header->max_preamble_delay,
     header->avg_preamble_energy,
     frame,
     0,
     false
     );
  */
  // run PRACH detection for CE-level 0 only for now when br_flag is set
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
  return;
}

void prach_eNB_process(uint8_t *bufferZone, int bufSize, PHY_VARS_eNB *eNB,RU_t *ru,int frame,int subframe) {
  //commonUDP_t *UDPheader=(commonUDP_t *) bufferZone;
  fs6_ul_t *header=(fs6_ul_t *) commonUDPdata(bufferZone);
  uint16_t *max_preamble=header->max_preamble;
  uint16_t *max_preamble_energy=header->max_preamble_energy;
  uint16_t *max_preamble_delay=header->max_preamble_delay;
  uint16_t *avg_preamble_energy=header->avg_preamble_energy;
  // Fixme: not clear why we call twice with "br" and without
  int br_flag=1;

  if (br_flag==1) {
    int             prach_mask;
    prach_mask = is_prach_subframe (&eNB->frame_parms, eNB->proc.frame_prach_br, eNB->proc.subframe_prach_br);
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

void pusch_procedures_extract(uint8_t *bufferZone, int bufSize, PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc) {
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
      rx_ulsch(eNB,proc, i);
      stop_meas(&eNB->ulsch_demodulation_stats);
      // TBD: add datablock for transmission
    }
  }
}

void phy_procedures_eNB_uespec_RX_extract(uint8_t *bufferZone, int bufSize, PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc) {
  //RX processing for ue-specific resources (i
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
    pusch_procedures_extract(bufferZone, bufSize, eNB,proc);
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

void pusch_procedures_process(uint8_t *bufferZone, int bufSize, PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc) {
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
      int ret = ulsch_decoding(eNB,proc,
                               i,
                               0, // control_only_flag
                               ulsch_harq->V_UL_DAI,
                               ulsch_harq->nb_rb>20 ? 1 : 0);
      stop_meas(&eNB->ulsch_decoding_stats);
      LOG_D(PHY,
            "[eNB %d][PUSCH %d] frame %d subframe %d RNTI %x RX power (%d,%d) N0 (%d,%d) dB ACK (%d,%d), decoding iter %d ulsch_harq->cqi_crc_status:%d ackBits:%d ulsch_decoding_stats[t:%lld max:%lld]\n",
            eNB->Mod_id,harq_pid,
            frame,subframe,
            ulsch->rnti,
            dB_fixed(eNB->pusch_vars[i]->ulsch_power[0]),
            dB_fixed(eNB->pusch_vars[i]->ulsch_power[1]),
            30,//eNB->measurements.n0_power_dB[0],
            30,//eNB->measurements.n0_power_dB[1],
            ulsch_harq->o_ACK[0],
            ulsch_harq->o_ACK[1],
            ret,
            ulsch_harq->cqi_crc_status,
            ulsch_harq->O_ACK,
            eNB->ulsch_decoding_stats.p_time, eNB->ulsch_decoding_stats.max);
      //compute the expected ULSCH RX power (for the stats)
      ulsch_harq->delta_TF = get_hundred_times_delta_IF_eNB(eNB,i,harq_pid, 0); // 0 means bw_factor is not considered

      if (RC.mac != NULL) { /* ulsim does not use RC.mac context. */
        if (ulsch_harq->cqi_crc_status == 1) {
#ifdef DEBUG_PHY_PROC
          //if (((frame%10) == 0) || (frame < 50))
          print_CQI(ulsch_harq->o,ulsch_harq->uci_format,0,fp->N_RB_DL);
#endif
          fill_ulsch_cqi_indication(eNB,frame,subframe,ulsch_harq,ulsch->rnti);
          RC.mac[eNB->Mod_id]->UE_list.UE_sched_ctrl[i].cqi_req_flag &= (~(1 << subframe));
        } else {
          if(RC.mac[eNB->Mod_id]->UE_list.UE_sched_ctrl[i].cqi_req_flag & (1 << subframe) ) {
            RC.mac[eNB->Mod_id]->UE_list.UE_sched_ctrl[i].cqi_req_flag &= (~(1 << subframe));
            RC.mac[eNB->Mod_id]->UE_list.UE_sched_ctrl[i].cqi_req_timer=30;
            LOG_D(PHY,"Frame %d,Subframe %d, We're supposed to get a cqi here. Set cqi_req_timer to 30.\n",frame,subframe);
          }
        }
      }

      if (ret == (1+MAX_TURBO_ITERATIONS)) {
        T(T_ENB_PHY_ULSCH_UE_NACK, T_INT(eNB->Mod_id), T_INT(frame), T_INT(subframe), T_INT(ulsch->rnti),
          T_INT(harq_pid));
        fill_crc_indication(eNB,i,frame,subframe,1); // indicate NAK to MAC
        fill_rx_indication(eNB,i,frame,subframe);  // indicate SDU to MAC
        LOG_D(PHY,"[eNB %d][PUSCH %d] frame %d subframe %d UE %d Error receiving ULSCH, round %d/%d (ACK %d,%d)\n",
              eNB->Mod_id,harq_pid,
              frame,subframe, i,
              ulsch_harq->round,
              ulsch->Mlimit,
              ulsch_harq->o_ACK[0],
              ulsch_harq->o_ACK[1]);

        if (ulsch_harq->round >= 3)  {
          ulsch_harq->status  = SCH_IDLE;
          ulsch_harq->handled = 0;
          ulsch->harq_mask   &= ~(1 << harq_pid);
          ulsch_harq->round   = 0;
        }

        MSC_LOG_RX_DISCARDED_MESSAGE(
          MSC_PHY_ENB,MSC_PHY_UE,
          NULL,0,
          "%05u:%02u ULSCH received rnti %x harq id %u round %d",
          frame,subframe,
          ulsch->rnti,harq_pid,
          ulsch_harq->round-1
        );
        /* Mark the HARQ process to release it later if max transmission reached
         * (see below).
         * MAC does not send the max transmission count, we have to deal with it
         * locally in PHY.
         */
        ulsch_harq->handled = 1;
      }                         // ulsch in error
      else {
        fill_crc_indication(eNB,i,frame,subframe,0); // indicate ACK to MAC
        fill_rx_indication(eNB,i,frame,subframe);  // indicate SDU to MAC
        ulsch_harq->status = SCH_IDLE;
        ulsch->harq_mask &= ~(1 << harq_pid);
        T (T_ENB_PHY_ULSCH_UE_ACK, T_INT (eNB->Mod_id), T_INT (frame), T_INT (subframe), T_INT (ulsch->rnti), T_INT (harq_pid));
        MSC_LOG_RX_MESSAGE(
          MSC_PHY_ENB,MSC_PHY_UE,
          NULL,0,
          "%05u:%02u ULSCH received rnti %x harq id %u",
          frame,subframe,
          ulsch->rnti,harq_pid
        );
      }  // ulsch not in error

      if (ulsch_harq->O_ACK>0) fill_ulsch_harq_indication(eNB,ulsch_harq,ulsch->rnti,frame,subframe,ulsch->bundling);

      LOG_D(PHY,"[eNB %d] Frame %d subframe %d: received ULSCH harq_pid %d for UE %d, ret = %d, CQI CRC Status %d, ACK %d,%d, ulsch_errors %d/%d\n",
            eNB->Mod_id,frame,subframe,
            harq_pid,
            i,
            ret,
            ulsch_harq->cqi_crc_status,
            ulsch_harq->o_ACK[0],
            ulsch_harq->o_ACK[1],
            eNB->UE_stats[i].ulsch_errors[harq_pid],
            eNB->UE_stats[i].ulsch_decoding_attempts[harq_pid][0]);
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
}

void phy_procedures_eNB_uespec_RX_process(uint8_t *bufferZone, int nbBlocks, PHY_VARS_eNB *phy_vars_eNB,L1_rxtx_proc_t *proc) {
}

void phy_procedures_eNB_TX_process(uint8_t *bufferZone, int nbBlocks, PHY_VARS_eNB *eNB, L1_rxtx_proc_t *proc, int do_meas ) {
  //commonUDP_t *UDPheader=(commonUDP_t *) bufferZone;
  fs6_dl_t *header=(fs6_dl_t *) commonUDPdata(bufferZone);
  // TBD: read frame&subframe from received data
  int frame=proc->frame_tx=header->frame;
  int subframe=proc->subframe_tx=header->subframe;
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  //LTE_UL_eNB_HARQ_t *ulsch_harq;
  eNB->pdcch_vars[subframe&1].num_pdcch_symbols=header->num_pdcch_symbols;
  eNB->pdcch_vars[subframe&1].num_dci=header->num_dci;
  uint8_t num_mdci = eNB->mpdcch_vars[subframe&1].num_dci = header->num_mdci;

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

  void *bufPtr=bufferZone;

  for (int i=0; i < nbBlocks; i++) { //nbBlocks is the actual received blocks
    fs6_dl_uespec_t *dlPtr=(fs6_dl_uespec_t *)(commonUDPdata(bufPtr)+sizeof(fs6_dl_t));
#ifdef PHY_TX_THREAD
    eNB->dlsch[dlPtr->UE_id][0]->active[subframe] = 1;
#else
    eNB->dlsch[dlPtr->UE_id][0]->active = 1;
#endif
    eNB->dlsch[dlPtr->UE_id][0]->harq_ids[frame%2][subframe]=dlPtr->harq_pid;
    eNB->dlsch[dlPtr->UE_id][0]->rnti=dlPtr->rnti;
    memcpy(eNB->dlsch[dlPtr->UE_id][0]->harq_processes[dlPtr->harq_pid]->e,
           commonUDPdata(((uint8_t *)dlPtr)+sizeof(fs6_dl_t)), dlPtr->dataLen);
    bufPtr+=alignedSize(bufPtr);
  }

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
      pmch_procedures(eNB,proc);
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
    for (int i=0; i< header->num_dci; i++)
      eNB->pdcch_vars[subframe&1].dci_alloc[i]=header->dci_alloc[i];

    generate_dci_top(header->num_pdcch_symbols,
                     header->num_dci,
                     &eNB->pdcch_vars[subframe&1].dci_alloc[0],
                     0,
                     header->amp,
                     fp,
                     eNB->common_vars.txdataF,
                     header->subframe);

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
      int harq_pid=dlsch0->harq_ids[frame%2][subframe];
      pdsch_procedures(eNB,
                       proc,
                       harq_pid,
                       dlsch0,
                       dlsch1);
    }
  }

  eNB->phich_vars[subframe&1]=header->phich_vars;
  generate_phich_top(eNB,
                     proc,
                     AMP);
}

void appendFs6DLUE(uint8_t *bufferZone, int UE_id, int8_t harq_pid, uint16_t rnti, uint8_t *UEdata, int UEdataLen) {
  commonUDP_t *FirstUDPheader=(commonUDP_t *) bufferZone;
  // move to the end
  uint8_t *firstFreeByte=bufferZone;
  int curBlock=0;

  for (int i=0; i < FirstUDPheader->nbBlocks; i++) {
    AssertFatal( ((commonUDP_t *) firstFreeByte)->blockID==curBlock,"");
    firstFreeByte+=alignedSize(firstFreeByte);
    curBlock++;
  }

  AssertFatal(firstFreeByte+UEdataLen+sizeof(fs6_dl_t) <= bufferZone+FS6_BUF_SIZE, "");
  commonUDP_t *newUDPheader=(commonUDP_t *) firstFreeByte;
  FirstUDPheader->nbBlocks++;
  newUDPheader->blockID=curBlock;
  newUDPheader->contentBytes=sizeof(fs6_dl_t)+sizeof(fs6_dl_uespec_t) + UEdataLen;
  // We skip the fs6 DL header, that is populated by caller
  // This header will be duplicated during sending
  fs6_dl_uespec_t *tmp=(fs6_dl_uespec_t *) firstFreeByte+sizeof(fs6_dl_t);
  tmp->UE_id=UE_id;
  tmp->harq_pid=harq_pid;
  tmp->rnti=rnti;
  memcpy(commonUDPdata(((uint8_t *)tmp)+sizeof(fs6_dl_uespec_t)), UEdata, UEdataLen);
}

void phy_procedures_eNB_TX_extract(uint8_t *bufferZone, PHY_VARS_eNB *eNB, L1_rxtx_proc_t *proc, int do_meas, uint8_t *buf, int bufSize) {
  fs6_dl_t *header=(fs6_dl_t *) commonUDPdata(bufferZone);
  int frame=proc->frame_tx;
  int subframe=proc->subframe_tx;
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;

  if ((fp->frame_type == TDD) && (subframe_select (fp, subframe) == SF_UL))
    return;

  header->frame=frame;
  header->subframe=subframe;
  // clear existing ulsch dci allocations before applying info from MAC  (this is table
  // returns -1 (or 255) if there is no ul (tdd cases)
  header->ul_subframe = pdcch_alloc2ul_subframe (fp, subframe);
  header->ul_frame = pdcch_alloc2ul_frame (fp, frame, subframe);

  // clear previous allocation information for all UEs
  for (int i = 0; i < NUMBER_OF_UE_MAX; i++) {
    //if (eNB->dlsch[i][0])
    //eNB->dlsch[i][0]->subframe_tx[subframe] = 0;
  }

  /* TODO: check the following test - in the meantime it is put back as it was before */
  //if ((ul_subframe < 10)&&
  //    (subframe_select(fp,ul_subframe)==SF_UL)) { // This means that there is a potential UL subframe that will be scheduled here
  if (header->ul_subframe < 10) { // This means that there is a potential UL subframe that will be scheduled here
    for (int i=0; i<NUMBER_OF_UE_MAX; i++) {
      int harq_pid;

      if (eNB->ulsch[i] && eNB->ulsch[i]->ue_type > NOCE)
        // LTE-M case
        harq_pid = 0;
      else
        header->UE_ul_active[i] = harq_pid = subframe2harq_pid(fp,header->ul_frame,header->ul_subframe);

      if (eNB->ulsch[i]) {
        //LTE_UL_eNB_HARQ_t *ulsch_harq = eNB->ulsch[i]->harq_processes[harq_pid];
        /* Store first_rb and n_DMRS for correct PHICH generation below.
         * For PHICH generation we need "old" values of last scheduling
         * for this HARQ process. 'generate_eNB_dlsch_params' below will
         * overwrite first_rb and n_DMRS and 'generate_phich_top', done
         * after 'generate_eNB_dlsch_params', would use the "new" values
         * instead of the "old" ones.
         *
         * This has been tested for FDD only, may be wrong for TDD.
         *
         * TODO: maybe we should restructure the code to be sure it
         *       is done correctly. The main concern is if the code
         *       changes and first_rb and n_DMRS are modified before
         *       we reach here, then the PHICH processing will be wrong,
         *       using wrong first_rb and n_DMRS values to compute
         *       ngroup_PHICH and nseq_PHICH.
         *
         * TODO: check if that works with TDD.
         */
        /* seems useless: unused variables
        header->previous_first_rb=
          ulsch_harq->previous_first_rb = ulsch_harq->first_rb;
        header->previous_n_DMRS =
          ulsch_harq->previous_n_DMRS = ulsch_harq->n_DMRS;
        */
        header->UE_ul_active[i] = harq_pid;
      } else
        header->UE_ul_active[i] = -1;
    }
  }

  uint8_t num_pdcch_symbols = eNB->pdcch_vars[subframe&1].num_pdcch_symbols;
  uint8_t num_dci           = eNB->pdcch_vars[subframe&1].num_dci;
  uint8_t num_mdci          = eNB->mpdcch_vars[subframe&1].num_dci;
  LOG_D(PHY,"num_pdcch_symbols %"PRIu8",number dci %"PRIu8"\n",num_pdcch_symbols, num_dci);

  if (NFAPI_MODE==NFAPI_MONOLITHIC || NFAPI_MODE==NFAPI_MODE_PNF) {
    header->num_pdcch_symbols=num_pdcch_symbols;
    header->num_dci=num_dci;
    header->num_mdci=num_mdci;
    header->amp=AMP;
  }

  if (do_meas==1) stop_meas(&eNB->dlsch_common_and_dci);

  if (do_meas==1) start_meas(&eNB->dlsch_ue_specific);

  // Now scan UE specific DLSCH
  LTE_eNB_DLSCH_t *dlsch0;

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
                        UE_id,
                        harq_pid,
                        dlsch0->rnti,
                        dlsch0->harq_processes[harq_pid]->e,
                        get_G(fp,
                              dlsch_harq->nb_rb,
                              dlsch_harq->rb_alloc,
                              dlsch_harq->Qm,
                              dlsch_harq->Nl,
                              dlsch_harq->pdsch_start,
                              frame,subframe,
                              0)
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

  header->phich_vars=eNB->phich_vars[subframe&1];

  if (do_meas==1) stop_meas(&eNB->dlsch_ue_specific);

  if (do_meas==1) stop_meas(&eNB->phy_proc_tx);

  return;
}

void DL_du_fs6(RU_t *ru, int frame, int subframe, uint64_t TS) {
  //RU_proc_t *ru_proc=&ru->proc;
  for (int i=0; i<ru->num_eNB; i++) {
    initBufferZone(bufferZone);
    int nb_blocks=receiveSubFrame(&sockFS6, TS, bufferZone, sizeof(bufferZone) );

    if (nb_blocks>0)
      phy_procedures_eNB_TX_process( bufferZone, nb_blocks, ru->eNB_list[i], &ru->eNB_list[i]->proc.L1_proc, 1);
    else
      LOG_E(PHY,"DL not received for subframe: %d\n", subframe);
  }

  /* Fixme: datamodel issue: a ru is supposed to be connected to several eNB
     L1_rxtx_proc_t * L1_proc = &proc->L1_proc;
     ru_proc->timestamp_tx = L1_proc->timestamp_tx;
     ru_proc->subframe_tx  = L1_proc->subframe_tx;
     ru_proc->frame_tx     = L1_proc->frame_tx;
  */
  feptx_prec(ru);
  feptx_ofdm(ru);
  tx_rf(ru);
}

void UL_du_fs6(RU_t *ru, int frame, int subframe) {
  RU_proc_t *ru_proc=&ru->proc;
  int tmpf=frame, tmpsf=subframe;
  rx_rf(ru,&tmpf,&tmpsf);
  AssertFatal(tmpf==frame && tmpsf==subframe, "lost synchronization\n");
  ru_proc->frame_tx = (ru_proc->frame_tx+ru_proc->frame_offset)&1023;
  // Fixme: datamodel issue
  PHY_VARS_eNB *eNB = RC.eNB[0][0];
  L1_proc_t *proc           = &eNB->proc;
  //L1_rxtx_proc_t *L1_proc = &proc->L1_proc;
  //LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  proc->frame_rx    = frame;
  proc->subframe_rx = subframe;

  if (NFAPI_MODE==NFAPI_MODE_PNF) {
    // I am a PNF and I need to let nFAPI know that we have a (sub)frame tick
    //add_subframe(&frame, &subframe, 4);
    //oai_subframe_ind(proc->frame_tx, proc->subframe_tx);
    oai_subframe_ind(proc->frame_rx, proc->subframe_rx);
  }

  initBufferZone(bufferZone);
  prach_eNB_extract(bufferZone, FS6_BUF_SIZE, eNB, proc->frame_rx,proc->subframe_rx );

  if (NFAPI_MODE==NFAPI_MONOLITHIC || NFAPI_MODE==NFAPI_MODE_PNF) {
    phy_procedures_eNB_uespec_RX_extract(bufferZone, FS6_BUF_SIZE, eNB, &proc->L1_proc );
  }

  for (int i=0; i<ru->num_eNB; i++) {
    sendSubFrame(&sockFS6,bufferZone, sizeof(fs6_ul_t));
  }
}

void DL_cu_fs6(RU_t *ru,int frame, int subframe) {
  // Fixme: datamodel issue
  PHY_VARS_eNB *eNB = RC.eNB[0][0];
  L1_proc_t *proc           = &eNB->proc;
  pthread_mutex_lock(&eNB->UL_INFO_mutex);
  eNB->UL_INFO.frame     = proc->frame_rx;
  eNB->UL_INFO.subframe  = proc->subframe_rx;
  eNB->UL_INFO.module_id = eNB->Mod_id;
  eNB->UL_INFO.CC_id     = eNB->CC_id;
  eNB->if_inst->UL_indication(&eNB->UL_INFO);
  pthread_mutex_unlock(&eNB->UL_INFO_mutex);
  initBufferZone(bufferZone);
  phy_procedures_eNB_TX_extract(bufferZone, eNB, &proc->L1_proc, 1, bufferZone, FS6_BUF_SIZE);
  commonUDP_t *firstHeader=(commonUDP_t *) bufferZone;

  if (firstHeader->nbBlocks==0) {
    firstHeader->nbBlocks=1; // We have to send the signaling, even is there is no user plan data (no UE)
    firstHeader->blockID=0;
    firstHeader->contentBytes=sizeof(fs6_dl_t);
  }

  sendSubFrame(&sockFS6, bufferZone, sizeof(fs6_dl_t) );
}

void UL_cu_fs6(RU_t *ru,int frame, int subframe, uint64_t TS) {
  initBufferZone(bufferZone);
  commonUDP_t *UDPheader=(commonUDP_t *) bufferZone;
  int nb_blocks=receiveSubFrame(&sockFS6, TS, bufferZone, sizeof(bufferZone) );

  if (nb_blocks ==0) {
    LOG_W(PHY, "CU lost a subframe\n");
    return;
  }

  if (nb_blocks != UDPheader->nbBlocks )
    LOG_W(PHY, "received %d blocks for %d expected\n", nb_blocks, UDPheader->nbBlocks);

  // Fixme: datamodel issue
  PHY_VARS_eNB *eNB = RC.eNB[0][0];
  L1_proc_t *proc   = &eNB->proc;
  prach_eNB_process(bufferZone, sizeof(bufferZone), eNB,NULL,proc->frame_rx,proc->subframe_rx);
  release_UE_in_freeList(eNB->Mod_id);

  if (NFAPI_MODE==NFAPI_MONOLITHIC || NFAPI_MODE==NFAPI_MODE_PNF) {
    phy_procedures_eNB_uespec_RX_process(bufferZone, sizeof(bufferZone), eNB, &proc->L1_proc);
  }
}

void *cu_fs6(void *arg) {
  RU_t               *ru      = (RU_t *)arg;
  //RU_proc_t          *proc    = &ru->proc;
  int64_t           AbsoluteSubframe=-1;
  fill_rf_config(ru,ru->rf_config_file);
  init_frame_parms(&ru->frame_parms,1);
  phy_init_RU(ru);
  wait_sync("ru_thread");
  AssertFatal(createUDPsock(NULL, "8888", "127.0.0.1", "7777", &sockFS6), "");

  while(1) {
    AbsoluteSubframe++;
    int subframe=AbsoluteSubframe%10;
    int frame=(AbsoluteSubframe/10)%1024;
    UL_cu_fs6(ru, frame,subframe, AbsoluteSubframe);
    DL_cu_fs6(ru, frame,subframe);
  }

  return NULL;
}

void *du_fs6(void *arg) {
  RU_t               *ru      = (RU_t *)arg;
  //RU_proc_t          *proc    = &ru->proc;
  int64_t           AbsoluteSubframe=-1;
  fill_rf_config(ru,ru->rf_config_file);
  init_frame_parms(&ru->frame_parms,1);
  phy_init_RU(ru);
  openair0_device_load(&ru->rfdevice,&ru->openair0_cfg);
  wait_sync("ru_thread");
  AssertFatal(createUDPsock(NULL, "7777", "127.0.0.1", "8888", &sockFS6),"");

  while(1) {
    AbsoluteSubframe++;
    int subframe=AbsoluteSubframe%10;
    int frame=(AbsoluteSubframe/10)%1024;
    UL_du_fs6(ru, frame,subframe);
    DL_du_fs6(ru, frame,subframe, AbsoluteSubframe);
  }

  return NULL;
}
