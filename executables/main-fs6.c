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

#define FS6_BUF_SIZE 100*1000
static UDPsock_t sockFS6;


void prach_eNB_tosplit(uint8_t *bufferZone, int bufSize, PHY_VARS_eNB *eNB) {
  fs6_ul_t *header=(fs6_ul_t *) commonUDPdata(bufferZone);

  if (is_prach_subframe(&eNB->frame_parms, eNB->proc.frame_prach,eNB->proc.subframe_prach)<=0)
    return;

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

  rx_prach(eNB,
           eNB->RU_list[0],
           header->max_preamble,
           header->max_preamble_energy,
           header->max_preamble_delay,
           header->avg_preamble_energy,
           eNB->proc.frame_prach,
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

void prach_eNB_fromsplit(uint8_t *bufferZone, int bufSize, PHY_VARS_eNB *eNB) {
  fs6_ul_t *header=(fs6_ul_t *) commonUDPdata(bufferZone);
  uint16_t *max_preamble=header->max_preamble;
  uint16_t *max_preamble_energy=header->max_preamble_energy;
  uint16_t *max_preamble_delay=header->max_preamble_delay;
  uint16_t *avg_preamble_energy=header->avg_preamble_energy;
  int subframe=eNB->proc.subframe_prach;
  int frame=eNB->proc.frame_prach;
  // Fixme: not clear why we call twice with "br" and without
  int br_flag=0;

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

void sendFs6Ulharq(enum pckType type, int UEid, PHY_VARS_eNB *eNB, int frame, int subframe, uint8_t *harq_ack, uint8_t tdd_mapping_mode, uint16_t tdd_multiplexing_mask,  uint16_t rnti,  int32_t stat) {
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

void sendFs6Ul(PHY_VARS_eNB *eNB, int UE_id, int harq_pid, int segmentID, int16_t *data, int dataLen) {
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
  hULUE(newUDPheader)->segment=segmentID;
  memcpy(hULUE(newUDPheader)+1, data, dataLen);
  hULUE(newUDPheader)->segLen=dataLen;
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

int ulsch_decoding_process(PHY_VARS_eNB *eNB, int UE_id, int llr8_flag) {
  int harq_pid;
  LTE_eNB_ULSCH_t *ulsch = eNB->ulsch[UE_id];

  if (ulsch->ue_type>0)
    harq_pid = 0;
  else
    harq_pid = subframe2harq_pid(&eNB->frame_parms,eNB->proc.frame_rx,eNB->proc.subframe_rx);

  LTE_UL_eNB_HARQ_t *ulsch_harq = ulsch->harq_processes[harq_pid];
  decoder_if_t *tc;
  int offset, ret=-1;

  if (llr8_flag == 0)
    tc = *decoder16;
  else
    tc = *decoder8;

  // This is a new packet, so compute quantities regarding segmentation
  //Fixme: very dirty: all the variables produced by let_segmentation are only used localy
  // Furthermore, variables as 1 letter and global is "time consuming" for everybody !!!
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

  for (int r=0; r<ulsch_harq->C; r++) {
    //    printf("before subblock deinterleaving c[%d] = %p\n",r,ulsch_harq->c[r]);
    // Get Turbo interleaver parameters
    int Kr;

    if (r<ulsch_harq->Cminus)
      Kr = ulsch_harq->Kminus;
    else
      Kr = ulsch_harq->Kplus;

    int Kr_bytes = Kr>>3;
    int crc_type;

    if (ulsch_harq->C == 1)
      crc_type = CRC24_A;
    else
      crc_type = CRC24_B;

    start_meas(&eNB->ulsch_turbo_decoding_stats);
    ret = tc(&ulsch_harq->d[r][96],
             NULL,
             ulsch_harq->c[r],
             NULL,
             Kr,
             ulsch->max_turbo_iterations,//MAX_TURBO_ITERATIONS,
             crc_type,
             (r==0) ? ulsch_harq->F : 0,
             &eNB->ulsch_tc_init_stats,
             &eNB->ulsch_tc_alpha_stats,
             &eNB->ulsch_tc_beta_stats,
             &eNB->ulsch_tc_gamma_stats,
             &eNB->ulsch_tc_ext_stats,
             &eNB->ulsch_tc_intl1_stats,
             &eNB->ulsch_tc_intl2_stats);
    stop_meas(&eNB->ulsch_turbo_decoding_stats);

    // Reassembly of Transport block here

    if (ret != (1+ulsch->max_turbo_iterations)) {
      if (r<ulsch_harq->Cminus)
        Kr = ulsch_harq->Kminus;
      else
        Kr = ulsch_harq->Kplus;

      Kr_bytes = Kr>>3;

      if (r==0) {
        memcpy(ulsch_harq->bb,
               &ulsch_harq->c[0][(ulsch_harq->F>>3)],
               Kr_bytes - (ulsch_harq->F>>3) - ((ulsch_harq->C>1)?3:0));
        offset = Kr_bytes - (ulsch_harq->F>>3) - ((ulsch_harq->C>1)?3:0);
      } else {
        memcpy(ulsch_harq->bb+offset,
               ulsch_harq->c[r],
               Kr_bytes - ((ulsch_harq->C>1)?3:0));
        offset += (Kr_bytes- ((ulsch_harq->C>1)?3:0));
      }
    } else {
      break;
    }
  }

  return(ret);
}

void pusch_procedures_fromsplit(uint8_t *bufferZone, int bufSize, PHY_VARS_eNB *eNB) {
  //LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  const int subframe = eNB->proc.subframe_rx;
  const int frame    = eNB->proc.frame_rx;
  uint32_t harq_pid;
  uint32_t harq_pid0 = subframe2harq_pid(&eNB->frame_parms,frame,subframe);

  // TBD: read UL data

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
      /*
      int ret = ulsch_decoding(eNB,&eNB->proc.L1_proc,
                               i,
                               0, // control_only_flag
                               ulsch_harq->V_UL_DAI,
                               ulsch_harq->nb_rb>20 ? 1 : 0);
      */
      int ret = ulsch_decoding_process(eNB,
                                       i,
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

void recvFs6Ul(uint8_t *bufferZone, int nbBlocks, PHY_VARS_eNB *eNB) {
  void *bufPtr=bufferZone;

  for (int i=0; i < nbBlocks; i++) { //nbBlocks is the actual received blocks
    if ( ((commonUDP_t *)bufPtr)->contentBytes > sizeof(fs6_ul_t) ) {
      int type=hULUE(bufPtr)->type;

      if ( type == fs6ULsch)  {
        LTE_eNB_ULSCH_t *ulsch =eNB->ulsch[hULUE(bufPtr)->UE_id];
        LTE_UL_eNB_HARQ_t *ulsch_harq=ulsch->harq_processes[hULUE(bufPtr)->harq_id];
        memcpy(&ulsch_harq->d[hULUE(bufPtr)->segment][96],
               hULUE(bufPtr)+1,
               hULUE(bufPtr)->segLen);
        LOG_W(PHY,"Received ulsch data for: rnti:%d, fsf: %d/%d\n", ulsch->rnti, eNB->proc.frame_rx, eNB->proc.subframe_rx);
      } else if ( type == fs6ULcch ) {
        int nb_uci=hULUEuci(bufPtr)->nb_active_ue;
        fs6_ul_uespec_uci_element_t *tmp=(fs6_ul_uespec_uci_element_t *)(hULUEuci(bufPtr)+1);
        for (int j=0; j < nb_uci ; j++) {
	  LOG_D(PHY,"FS6 cu, block: %d/%d: received ul harq/sr: %d, rnti: %d, ueid: %d\n",
		i, j, type, tmp->rnti, tmp->UEid);
          eNB->measurements.n0_subband_power_dB[0][0]=tmp->n0_subband_power_dB;
	  
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

void phy_procedures_eNB_uespec_RX_fromsplit(uint8_t *bufferZone, int nbBlocks,PHY_VARS_eNB *eNB) {
  // The configuration arrived in Dl, so we can extract the UL data
  recvFs6Ul(bufferZone, nbBlocks, eNB);
  
  // dirty memory allocation in OAI...
  for (int i = 0; i < NUMBER_OF_UCI_VARS_MAX; i++)
    if ( eNB->uci_vars[i].frame == eNB->proc.frame_rx &&
	 eNB->uci_vars[i].subframe == eNB->proc.subframe_rx )
      eNB->uci_vars[i].active=0;

  pusch_procedures_fromsplit(bufferZone, nbBlocks, eNB);
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
        dlsch_harq->i0=hDLUE(bufPtr)->i0;
        dlsch_harq->sib1_br_flag=hDLUE(bufPtr)->sib1_br_flag;
#else
        dlsch0->i0=hDLUE(bufPtr)->i0;
        dlsch0->sib1_br_flag=hDLUE(bufPtr)->sib1_br_flag;
#endif
        memcpy(dlsch_harq->e,
               hDLUE(bufPtr)+1, hDLUE(bufPtr)->dataLen);
      } else if (type == fs6UlConfig) {
        int nbUE=(((commonUDP_t *)bufPtr)->contentBytes - sizeof(fs6_dl_t)) / sizeof( fs6_dl_ulsched_t ) ;

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
          ulsch_harq->V_UL_DAI=hTxULUE(bufPtr)->V_UL_DAI;
          ulsch_harq->Qm=hTxULUE(bufPtr)->Qm;
          ulsch_harq->srs_active=hTxULUE(bufPtr)->srs_active;
          ulsch_harq->TBS=hTxULUE(bufPtr)->TBS;
          ulsch_harq->Nsymb_pusch=hTxULUE(bufPtr)->Nsymb_pusch;
          LOG_W(PHY,"Received request to perform ulsch for: rnti:%d, fsf: %d/%d\n", ulsch->rnti, frame, subframe);
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
  cpyToDuHarq(first_rb);
  cpyToDuHarq(V_UL_DAI);
  cpyToDuHarq(Qm);
  cpyToDuHarq(srs_active);
  cpyToDuHarq(TBS);
  cpyToDuHarq(Nsymb_pusch);
  LOG_W(PHY,"Added request to perform ulsch for: rnti:%d, fsf: %d/%d\n", ulsch->rnti, frame, subframe);
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
  AssertFatal(firstFreeByte+UEdataLen+sizeof(fs6_dl_t) <= bufferZone+FS6_BUF_SIZE, "");
  commonUDP_t *newUDPheader=(commonUDP_t *) firstFreeByte;
  FirstUDPheader->nbBlocks++;
  newUDPheader->blockID=curBlock;
  newUDPheader->contentBytes=sizeof(fs6_dl_t)+sizeof(fs6_dl_uespec_t) + UEdataLen;
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
  hDLUE(newUDPheader)->i0=harqData->i0;
  hDLUE(newUDPheader)->sib1_br_flag=harqData->sib1_br_flag;
#else
  hDLUE(newUDPheader)->i0=dlsch0->i0;
  hDLUE(newUDPheader)->sib1_br_flag=dlsch0->sib1_br_flag;
#endif
  hDLUE(newUDPheader)->dataLen=UEdataLen;
  memcpy(hDLUE(newUDPheader)+1, harqData->e, UEdataLen);
  //for (int i=0; i < UEdataLen; i++)
  //LOG_D(PHY,"buffer e:%hhx\n", ( (uint8_t *)(hDLUE(newUDPheader)+1) )[i]);
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

  for (int i = 0; i < NUMBER_OF_UCI_VARS_MAX; i++) {
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

      if (ulsch &&
          (ulsch->rnti>0) &&
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
  LOG_D(PHY,"num_pdcch_symbols %"PRIu8",number dci %"PRIu8"\n",num_pdcch_symbols, num_dci);

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

void DL_du_fs6(RU_t *ru) {
  for (int i=0; i<ru->num_eNB; i++) {
    initBufferZone(bufferZone);
    int nb_blocks=receiveSubFrame(&sockFS6, bufferZone, sizeof(bufferZone), CTsentCUv0 );

    if (nb_blocks > 0) {
      L1_proc_t *L1_proc = &ru->eNB_list[i]->proc;

      if ( L1_proc->timestamp_tx != hUDP(bufferZone)->timestamp) {
        LOG_E(HW,"Missed a subframe: expecting: %lu, received %lu\n",
              L1_proc->timestamp_tx,
              hUDP(bufferZone)->timestamp);
      }

      setAllfromTS(hUDP(bufferZone)->timestamp - sf_ahead*ru->eNB_list[i]->frame_parms.samples_per_tti);
      phy_procedures_eNB_TX_fromsplit( bufferZone, nb_blocks, ru->eNB_list[i], &ru->eNB_list[i]->proc.L1_proc, 1);
    } else
      LOG_E(PHY,"DL not received for subframe\n");
  }

  feptx_prec(ru);
  feptx_ofdm(ru);
  tx_rf(ru);
}

void UL_du_fs6(RU_t *ru, int frame, int subframe) {
  RU_proc_t *ru_proc=&ru->proc;
  rx_rf(ru);
  setAllfromTS(ru_proc->timestamp_rx);
  // front end processing: convert from time domain to frequency domain
  // fills rxdataF buffer
  fep_full(ru);
  // Fixme: datamodel issue
  PHY_VARS_eNB *eNB = RC.eNB[0][0];
  L1_proc_t *proc   = &eNB->proc;

  if (NFAPI_MODE==NFAPI_MODE_PNF) {
    // I am a PNF and I need to let nFAPI know that we have a (sub)frame tick
    //add_subframe(&frame, &subframe, 4);
    //oai_subframe_ind(proc->frame_tx, proc->subframe_tx);
    oai_subframe_ind(proc->frame_rx, proc->subframe_rx);
  }

  initBufferZone(bufferZone);
  hUDP(bufferZone)->timestamp=ru->proc.timestamp_rx;
  prach_eNB_tosplit(bufferZone, FS6_BUF_SIZE, eNB);

  if (NFAPI_MODE==NFAPI_MONOLITHIC || NFAPI_MODE==NFAPI_MODE_PNF) {
    phy_procedures_eNB_uespec_RX_tosplit(bufferZone, FS6_BUF_SIZE, eNB, &proc->L1_proc );
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

void DL_cu_fs6(RU_t *ru) {
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
  phy_procedures_eNB_TX_tosplit(bufferZone, eNB, &proc->L1_proc, 1, bufferZone, FS6_BUF_SIZE);
  hUDP(bufferZone)->timestamp=proc->L1_proc.timestamp_tx;

  if (hUDP(bufferZone)->nbBlocks==0) {
    hUDP(bufferZone)->nbBlocks=1; // We have to send the signaling, even is there is no user plan data (no UE)
    hUDP(bufferZone)->blockID=0;
    hUDP(bufferZone)->contentBytes=sizeof(fs6_dl_t);
  }

  sendSubFrame(&sockFS6, bufferZone, sizeof(fs6_dl_t), CTsentCUv0 );
}

void UL_cu_fs6(RU_t *ru, uint64_t *TS) {
  initBufferZone(bufferZone);
  int nb_blocks=receiveSubFrame(&sockFS6, bufferZone, sizeof(bufferZone), CTsentDUv0 );

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

  setAllfromTS(hUDP(bufferZone)->timestamp);
  PHY_VARS_eNB *eNB = RC.eNB[0][0];
  prach_eNB_fromsplit(bufferZone, sizeof(bufferZone), eNB);
  release_UE_in_freeList(eNB->Mod_id);

  if (NFAPI_MODE==NFAPI_MONOLITHIC || NFAPI_MODE==NFAPI_MODE_PNF) {
    phy_procedures_eNB_uespec_RX_fromsplit(bufferZone, nb_blocks, eNB);
  }
}

void *cu_fs6(void *arg) {
  RU_t               *ru      = (RU_t *)arg;
  //RU_proc_t          *proc    = &ru->proc;
  fill_rf_config(ru,ru->rf_config_file);
  init_frame_parms(&ru->frame_parms,1);
  phy_init_RU(ru);
  wait_sync("ru_thread");
  AssertFatal(createUDPsock(NULL, CU_PORT, CU_IP, DU_PORT, &sockFS6), "");
  uint64_t timeStamp=0;

  while(1) {
    timeStamp+=ru->frame_parms.samples_per_tti;
    UL_cu_fs6(ru, &timeStamp);
    DL_cu_fs6(ru);
  }

  return NULL;
}

void *du_fs6(void *arg) {
  RU_t               *ru      = (RU_t *)arg;
  //RU_proc_t          *proc    = &ru->proc;
  fill_rf_config(ru,ru->rf_config_file);
  init_frame_parms(&ru->frame_parms,1);
  phy_init_RU(ru);
  openair0_device_load(&ru->rfdevice,&ru->openair0_cfg);
  wait_sync("ru_thread");
  AssertFatal(createUDPsock(NULL, DU_PORT, DU_IP, CU_PORT, &sockFS6),"");

  if (ru->start_rf) {
    if (ru->start_rf(ru) != 0)
      LOG_E(HW,"Could not start the RF device\n");
    else LOG_I(PHY,"RU %d rf device ready\n",ru->idx);
  } else LOG_I(PHY,"RU %d no rf device\n",ru->idx);

  while(1) {
    L1_proc_t *proc = &ru->eNB_list[0]->proc;
    UL_du_fs6(ru, proc->frame_rx,proc->subframe_rx);
    DL_du_fs6(ru);
  }

  return NULL;
}
