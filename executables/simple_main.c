/*
 * Author: Laurent Thomas
 * Copyright: Open Cells Project company
 */

#include <common/utils/LOG/log.h>
#include <common/utils/system.h>
static int DEFBANDS[] = {7};
static int DEFENBS[] = {0};
#include <common/config/config_userapi.h>
#include <targets/RT/USER/lte-softmodem.h>
#include <openair1/PHY/defs_eNB.h>
#include <openair1/PHY/phy_extern.h>
#include <nfapi/oai_integration/vendor_ext.h>
#include <openair1/SCHED/fapi_l1.h>
#include <openair1/PHY/INIT/phy_init.h>
#include <openair2/LAYER2/MAC/mac_extern.h>
extern uint16_t sf_ahead;
extern void oai_subframe_ind(uint16_t sfn, uint16_t sf);
extern void fep_full(RU_t *ru);
extern void feptx_ofdm(RU_t *ru);
extern void feptx_prec(RU_t *ru);
extern void  phy_init_RU(RU_t *);

static void *ru_thread( void *param );
void kill_RU_proc(RU_t *ru) {
}
void kill_eNB_proc(int inst) {
}
void free_transport(PHY_VARS_eNB *eNB) {
}
void reset_opp_meas(void) {
}

void init_eNB_proc(int inst) {
  /*int i=0;*/
  int CC_id;
  PHY_VARS_eNB *eNB;
  L1_proc_t *proc;
  L1_rxtx_proc_t *L1_proc;

  for (CC_id=0; CC_id<RC.nb_CC[inst]; CC_id++) {
    eNB = RC.eNB[inst][CC_id];
    proc = &eNB->proc;
    L1_proc                        = &proc->L1_proc;
    L1_proc->instance_cnt          = -1;
    L1_proc->instance_cnt_RUs      = 0;
    proc->instance_cnt_prach       = -1;
    proc->instance_cnt_synch       = -1;
    proc->CC_id                    = CC_id;
    proc->first_rx                 =1;
    proc->first_tx                 =1;
    proc->RU_mask_tx               = (1<<eNB->num_RU)-1;
    proc->RU_mask                  =0;
    proc->RU_mask_prach            =0;
    pthread_mutex_init( &eNB->UL_INFO_mutex, NULL);
    pthread_mutex_init( &L1_proc->mutex, NULL);
    pthread_cond_init( &L1_proc->cond, NULL);
    pthread_mutex_init( &proc->mutex_RU,NULL);
  }

  //for multiple CCs: setup master and slaves
  /*
     for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
     eNB = PHY_vars_eNB_g[inst][CC_id];

     if (eNB->node_timing == synch_to_ext_device) { //master
     eNB->proc.num_slaves = MAX_NUM_CCs-1;
     eNB->proc.slave_proc = (L1_proc_t**)malloc(eNB->proc.num_slaves*sizeof(L1_proc_t*));

     for (i=0; i< eNB->proc.num_slaves; i++) {
     if (i < CC_id)  eNB->proc.slave_proc[i] = &(PHY_vars_eNB_g[inst][i]->proc);
     if (i >= CC_id)  eNB->proc.slave_proc[i] = &(PHY_vars_eNB_g[inst][i+1]->proc);
     }
     }
     }
  */
}

void init_RU_proc(RU_t *ru) {
  int i=0;
  RU_proc_t *proc;
  proc = &ru->proc;
  memset((void *)proc,0,sizeof(RU_proc_t));
  proc->ru = ru;
  proc->instance_cnt_synch       = -1;
  proc->instance_cnt_eNBs        = -1;
  proc->first_rx                 = 1;
  proc->first_tx                 = 1;
  proc->frame_offset             = 0;
  proc->num_slaves               = 0;
  proc->frame_tx_unwrap          = 0;

  for (i=0; i<10; i++) proc->symbol_mask[i]=0;

  pthread_mutex_init( &proc->mutex_synch,NULL);

  pthread_cond_init( &proc->cond_synch,NULL);
  pthread_cond_init( &proc->cond_eNBs, NULL);

  pthread_t t;
  threadCreate(&t,  ru_thread, (void *)ru, "MainRu", -1, OAI_PRIORITY_RT_MAX);
}

void wakeup_prach_eNB(PHY_VARS_eNB *eNB,RU_t *ru,int frame,int subframe) {
  L1_proc_t *proc = &eNB->proc;
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;

  // check if we have to detect PRACH first
  if (is_prach_subframe(fp,frame,subframe)>0) {
    // set timing for prach thread
    proc->frame_prach = frame;
    proc->subframe_prach = subframe;
    prach_procedures(eNB,0);
  }
}

void wakeup_prach_eNB_br(PHY_VARS_eNB *eNB,RU_t *ru,int frame,int subframe) {
  L1_proc_t *proc = &eNB->proc;
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;

  // check if we have to detect PRACH first
  if (is_prach_subframe(fp,frame,subframe)>0) {
    LOG_D(PHY,"Triggering prach br processing, frame %d, subframe %d\n",frame,subframe);
    // set timing for prach thread
    proc->frame_prach_br = frame;
    proc->subframe_prach_br = subframe;
    prach_procedures(eNB,1);
  }
}

static inline int rxtx(PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc, char *thread_name) {

  AssertFatal( eNB !=NULL, "");

  if (NFAPI_MODE==NFAPI_MODE_PNF) {
    // I am a PNF and I need to let nFAPI know that we have a (sub)frame tick
    //add_subframe(&frame, &subframe, 4);
    //oai_subframe_ind(proc->frame_tx, proc->subframe_tx);
    oai_subframe_ind(proc->frame_rx, proc->subframe_rx);
  }

  AssertFatal( !(NFAPI_MODE==NFAPI_MODE_PNF &&
		 eNB->pdcch_vars[proc->subframe_tx&1].num_pdcch_symbols == 0), "");
  
  wakeup_prach_eNB(eNB,NULL,proc->frame_rx,proc->subframe_rx);
  wakeup_prach_eNB_br(eNB,NULL,proc->frame_rx,proc->subframe_rx);
  release_UE_in_freeList(eNB->Mod_id);

  // UE-specific RX processing for subframe n
  if (NFAPI_MODE==NFAPI_MONOLITHIC || NFAPI_MODE==NFAPI_MODE_PNF) {
    phy_procedures_eNB_uespec_RX(eNB, proc);
  }

  pthread_mutex_lock(&eNB->UL_INFO_mutex);
  eNB->UL_INFO.frame     = proc->frame_rx;
  eNB->UL_INFO.subframe  = proc->subframe_rx;
  eNB->UL_INFO.module_id = eNB->Mod_id;
  eNB->UL_INFO.CC_id     = eNB->CC_id;
  eNB->if_inst->UL_indication(&eNB->UL_INFO);
  pthread_mutex_unlock(&eNB->UL_INFO_mutex);
  phy_procedures_eNB_TX(eNB, proc, 1);
  
  return(0);
}

void eNB_top(PHY_VARS_eNB *eNB, int frame_rx, int subframe_rx, char *string,RU_t *ru) {
  L1_proc_t *proc           = &eNB->proc;
  L1_rxtx_proc_t *L1_proc = &proc->L1_proc;
  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  RU_proc_t *ru_proc=&ru->proc;
  proc->frame_rx    = frame_rx;
  proc->subframe_rx = subframe_rx;

  if (!oai_exit) {
    L1_proc->timestamp_tx = ru_proc->timestamp_rx + (sf_ahead*fp->samples_per_tti);
    L1_proc->frame_rx     = ru_proc->frame_rx;
    L1_proc->subframe_rx  = ru_proc->subframe_rx;
    L1_proc->frame_tx     = (L1_proc->subframe_rx > (9-sf_ahead)) ?
      (L1_proc->frame_rx+1)&1023 :
      L1_proc->frame_rx;
    L1_proc->subframe_tx  = (L1_proc->subframe_rx + sf_ahead)%10;

    if (rxtx(eNB,L1_proc,string) < 0)
      LOG_E(PHY,"eNB %d CC_id %d failed during execution\n",eNB->Mod_id,eNB->CC_id);

    ru_proc->timestamp_tx = L1_proc->timestamp_tx;
    ru_proc->subframe_tx  = L1_proc->subframe_tx;
    ru_proc->frame_tx     = L1_proc->frame_tx;
  }
}

void init_transport(PHY_VARS_eNB *eNB) {
  int i;
  int j;
  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  LOG_I(PHY, "Initialise transport\n");

  for (i=0; i<NUMBER_OF_UE_MAX; i++) {
    LOG_D(PHY,"Allocating Transport Channel Buffers for DLSCH, UE %d\n",i);

    for (j=0; j<2; j++) {
      eNB->dlsch[i][j] = new_eNB_dlsch(1,8,NSOFT,fp->N_RB_DL,0,fp);

      if (!eNB->dlsch[i][j]) {
        LOG_E(PHY,"Can't get eNB dlsch structures for UE %d \n", i);
        exit(-1);
      } else {
        eNB->dlsch[i][j]->rnti=0;
        LOG_D(PHY,"dlsch[%d][%d] => %p rnti:%d\n",i,j,eNB->dlsch[i][j], eNB->dlsch[i][j]->rnti);
      }
    }

    LOG_D(PHY,"Allocating Transport Channel Buffer for ULSCH, UE %d\n",i);
    eNB->ulsch[1+i] = new_eNB_ulsch(MAX_TURBO_ITERATIONS,fp->N_RB_UL, 0);

    if (!eNB->ulsch[1+i]) {
      LOG_E(PHY,"Can't get eNB ulsch structures\n");
      exit(-1);
    }

    // this is the transmission mode for the signalling channels
    // this will be overwritten with the real transmission mode by the RRC once the UE is connected
    eNB->transmission_mode[i] = fp->nb_antenna_ports_eNB==1 ? 1 : 2;
  }

  // ULSCH for RA
  eNB->ulsch[0] = new_eNB_ulsch(MAX_TURBO_ITERATIONS, fp->N_RB_UL, 0);

  if (!eNB->ulsch[0]) {
    LOG_E(PHY,"Can't get eNB ulsch structures\n");
    exit(-1);
  }

  eNB->dlsch_SI  = new_eNB_dlsch(1,8,NSOFT,fp->N_RB_DL, 0, fp);
  LOG_D(PHY,"eNB %d.%d : SI %p\n",eNB->Mod_id,eNB->CC_id,eNB->dlsch_SI);
  eNB->dlsch_ra  = new_eNB_dlsch(1,8,NSOFT,fp->N_RB_DL, 0, fp);
  LOG_D(PHY,"eNB %d.%d : RA %p\n",eNB->Mod_id,eNB->CC_id,eNB->dlsch_ra);
  eNB->dlsch_MCH = new_eNB_dlsch(1,8,NSOFT,fp->N_RB_DL, 0, fp);
  LOG_D(PHY,"eNB %d.%d : MCH %p\n",eNB->Mod_id,eNB->CC_id,eNB->dlsch_MCH);
  eNB->rx_total_gain_dB=130;

  for(i=0; i<NUMBER_OF_UE_MAX; i++)
    eNB->mu_mimo_mode[i].dl_pow_off = 2;

  eNB->check_for_total_transmissions = 0;
  eNB->check_for_MUMIMO_transmissions = 0;
  eNB->FULL_MUMIMO_transmissions = 0;
  eNB->check_for_SUMIMO_transmissions = 0;
  fp->pucch_config_common.deltaPUCCH_Shift = 1;
}

void init_eNB_afterRU(void) {
  int inst,CC_id,ru_id,i,aa;
  PHY_VARS_eNB *eNB;

  for (inst=0; inst<RC.nb_inst; inst++) {
    for (CC_id=0; CC_id<RC.nb_CC[inst]; CC_id++) {
      eNB = RC.eNB[inst][CC_id];
      phy_init_lte_eNB(eNB,0,0);
      eNB->frame_parms.nb_antennas_rx       = 0;
      eNB->prach_vars.rxsigF[0] = (int16_t **)malloc16(64*sizeof(int16_t *));

      for (int ce_level=0; ce_level<4; ce_level++) {
        LOG_I(PHY,"Overwriting eNB->prach_vars_br.rxsigF.rxsigF[0]:%p\n", eNB->prach_vars_br.rxsigF[ce_level]);
        eNB->prach_vars_br.rxsigF[ce_level] = (int16_t **)malloc16(64*sizeof(int16_t *));
      }

      for (ru_id=0,aa=0; ru_id<eNB->num_RU; ru_id++) {
        eNB->frame_parms.nb_antennas_rx    += eNB->RU_list[ru_id]->nb_rx;
        AssertFatal(eNB->RU_list[ru_id]->common.rxdataF!=NULL,
                    "RU %d : common.rxdataF is NULL\n",
                    eNB->RU_list[ru_id]->idx);
        AssertFatal(eNB->RU_list[ru_id]->prach_rxsigF!=NULL,
                    "RU %d : prach_rxsigF is NULL\n",
                    eNB->RU_list[ru_id]->idx);

        for (i=0; i<eNB->RU_list[ru_id]->nb_rx; aa++,i++) {
          LOG_I(PHY,"Attaching RU %d antenna %d to eNB antenna %d\n",eNB->RU_list[ru_id]->idx,i,aa);
          eNB->prach_vars.rxsigF[0][aa]    =  eNB->RU_list[ru_id]->prach_rxsigF[i];

          for (int ce_level=0; ce_level<4; ce_level++)
            eNB->prach_vars_br.rxsigF[ce_level][aa] = eNB->RU_list[ru_id]->prach_rxsigF_br[ce_level][i];

          eNB->common_vars.rxdataF[aa]     =  eNB->RU_list[ru_id]->common.rxdataF[i];
        }
      }

      /* TODO: review this code, there is something wrong.
       * In monolithic mode, we come here with nb_antennas_rx == 0
       * (not tested in other modes).
       */
      if (eNB->frame_parms.nb_antennas_rx < 1) {
        LOG_I(PHY, "%s() ************* DJP ***** eNB->frame_parms.nb_antennas_rx:%d - GOING TO HARD CODE TO 1", __FUNCTION__, eNB->frame_parms.nb_antennas_rx);
        eNB->frame_parms.nb_antennas_rx = 1;
      } else {
        //LOG_I(PHY," Delete code\n");
      }

      if (eNB->frame_parms.nb_antennas_tx < 1) {
        LOG_I(PHY, "%s() ************* DJP ***** eNB->frame_parms.nb_antennas_tx:%d - GOING TO HARD CODE TO 1", __FUNCTION__, eNB->frame_parms.nb_antennas_tx);
        eNB->frame_parms.nb_antennas_tx = 1;
      } else {
        //LOG_I(PHY," Delete code\n");
      }

      AssertFatal(eNB->frame_parms.nb_antennas_rx >0,
                  "inst %d, CC_id %d : nb_antennas_rx %d\n",inst,CC_id,eNB->frame_parms.nb_antennas_rx);
      LOG_I(PHY,"inst %d, CC_id %d : nb_antennas_rx %d\n",inst,CC_id,eNB->frame_parms.nb_antennas_rx);
      init_transport(eNB);
      //init_precoding_weights(RC.eNB[inst][CC_id]);
    }

    init_eNB_proc(inst);
  }
}

void init_eNB(int single_thread_flag,int wait_for_sync) {
  int CC_id;
  int inst;
  PHY_VARS_eNB *eNB;

  if (RC.eNB == NULL) RC.eNB = (PHY_VARS_eNB ** *) malloc(RC.nb_L1_inst*sizeof(PHY_VARS_eNB **));

  for (inst=0; inst<RC.nb_L1_inst; inst++) {
    if (RC.eNB[inst] == NULL) RC.eNB[inst] = (PHY_VARS_eNB **) malloc(RC.nb_CC[inst]*sizeof(PHY_VARS_eNB *));

    for (CC_id=0; CC_id<RC.nb_L1_CC[inst]; CC_id++) {
      if (RC.eNB[inst][CC_id] == NULL) RC.eNB[inst][CC_id] = (PHY_VARS_eNB *) malloc(sizeof(PHY_VARS_eNB));

      eNB                     = RC.eNB[inst][CC_id];
      eNB->abstraction_flag   = 0;
      eNB->single_thread_flag = single_thread_flag;
      AssertFatal((eNB->if_inst         = IF_Module_init(inst))!=NULL,"Cannot register interface");
      eNB->if_inst->schedule_response   = schedule_response;
      eNB->if_inst->PHY_config_req      = phy_config_request;
      memset((void *)&eNB->UL_INFO,0,sizeof(eNB->UL_INFO));
      memset((void *)&eNB->Sched_INFO,0,sizeof(eNB->Sched_INFO));
      LOG_I(PHY,"Setting indication lists\n");
      eNB->UL_INFO.rx_ind.rx_indication_body.rx_pdu_list   = eNB->rx_pdu_list;
      eNB->UL_INFO.crc_ind.crc_indication_body.crc_pdu_list = eNB->crc_pdu_list;
      eNB->UL_INFO.sr_ind.sr_indication_body.sr_pdu_list = eNB->sr_pdu_list;
      eNB->UL_INFO.harq_ind.harq_indication_body.harq_pdu_list = eNB->harq_pdu_list;
      eNB->UL_INFO.cqi_ind.cqi_pdu_list = eNB->cqi_pdu_list;
      eNB->UL_INFO.cqi_ind.cqi_raw_pdu_list = eNB->cqi_raw_pdu_list;
      eNB->prach_energy_counter = 0;
    }
  }
}

void stop_eNB(int nb_inst) {
  for (int inst=0; inst<nb_inst; inst++) {
    LOG_I(PHY,"Killing eNB %d processing threads\n",inst);
    kill_eNB_proc(inst);
  }
}

void rx_rf(RU_t *ru,int *frame,int *subframe) {
  RU_proc_t *proc = &ru->proc;
  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  void *rxp[ru->nb_rx];
  unsigned int rxs;
  int i;
  openair0_timestamp ts=0,old_ts=0;

  for (i=0; i<ru->nb_rx; i++)
    rxp[i] = (void *)&ru->common.rxdata[i][*subframe*fp->samples_per_tti];

  old_ts = proc->timestamp_rx;
  rxs = ru->rfdevice.trx_read_func(&ru->rfdevice,
                                   &ts,
                                   rxp,
                                   fp->samples_per_tti,
                                   ru->nb_rx);
  proc->timestamp_rx = ts-ru->ts_offset;

  //  AssertFatal(rxs == fp->samples_per_tti,
  //        "rx_rf: Asked for %d samples, got %d from SDR\n",fp->samples_per_tti,rxs);
  if(rxs != fp->samples_per_tti) {
    LOG_E(PHY,"rx_rf: Asked for %d samples, got %d from SDR\n",fp->samples_per_tti,rxs);
  }

  if (proc->first_rx == 1) {
    ru->ts_offset = proc->timestamp_rx;
    proc->timestamp_rx = 0;
  } else {
    if (proc->timestamp_rx - old_ts != fp->samples_per_tti) {
      ru->ts_offset += (proc->timestamp_rx - old_ts - fp->samples_per_tti);
      proc->timestamp_rx = ts-ru->ts_offset;
    }
  }

  proc->frame_rx     = (proc->timestamp_rx / (fp->samples_per_tti*10))&1023;
  proc->subframe_rx  = (proc->timestamp_rx / fp->samples_per_tti)%10;
  // synchronize first reception to frame 0 subframe 0
  proc->timestamp_tx = proc->timestamp_rx+(sf_ahead*fp->samples_per_tti);
  proc->subframe_tx  = (proc->subframe_rx+sf_ahead)%10;
  proc->frame_tx     = (proc->subframe_rx>(9-sf_ahead)) ? (proc->frame_rx+1)&1023 : proc->frame_rx;

  if (proc->first_rx == 0) {
    AssertFatal( proc->subframe_rx == *subframe && proc->frame_rx == *frame ,
		 "Received Timestamp (%llu) doesn't correspond to the time we think it is (proc->subframe_rx %d, subframe %d) (proc->frame_rx %d frame %d)\n",
		 (long long unsigned int)proc->timestamp_rx,proc->subframe_rx,*subframe, proc->frame_rx,*frame);
  } else {
    proc->first_rx = 0;
    *frame = proc->frame_rx;
    *subframe = proc->subframe_rx;
  }

  if (rxs != fp->samples_per_tti) {
#if defined(USRP_REC_PLAY)
    exit_fun("Exiting IQ record/playback");
#else
    //exit_fun( "problem receiving samples" );
    LOG_E(PHY, "problem receiving samples");
#endif
  }
}

void tx_rf(RU_t *ru) {
  RU_proc_t *proc = &ru->proc;
  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  void *txp[ru->nb_tx];
  int i;
  lte_subframe_t SF_type     = subframe_select(fp,proc->subframe_tx%10);
  lte_subframe_t prevSF_type = subframe_select(fp,(proc->subframe_tx+9)%10);
  int sf_extension = 0;

  if ((SF_type == SF_DL) ||
      (SF_type == SF_S)) {
    int siglen=fp->samples_per_tti,flags=1;

    if (SF_type == SF_S) {
      /* end_of_burst_delay is used to stop TX only "after a while".
       * If we stop right after effective signal, with USRP B210 and
       * B200mini, we observe a high EVM on the S subframe (on the
       * PSS).
       * A value of 400 (for 30.72MHz) solves this issue. This is
       * the default.
       */
      siglen = (fp->ofdm_symbol_size + fp->nb_prefix_samples0)
               + (fp->dl_symbols_in_S_subframe - 1) * (fp->ofdm_symbol_size + fp->nb_prefix_samples)
               + ru->end_of_burst_delay;
      flags=3; // end of burst
    }

    if (fp->frame_type == TDD &&
        SF_type == SF_DL &&
        prevSF_type == SF_UL) {
      flags = 2; // start of burst
      sf_extension = ru->sf_extension;
    }

#if defined(__x86_64) || defined(__i386__)
#ifdef __AVX2__
    sf_extension = (sf_extension)&0xfffffff8;
#else
    sf_extension = (sf_extension)&0xfffffffc;
#endif
#elif defined(__arm__)
    sf_extension = (sf_extension)&0xfffffffc;
#endif

    for (i=0; i<ru->nb_tx; i++)
      txp[i] = (void *)&ru->common.txdata[i][(proc->subframe_tx*fp->samples_per_tti)-sf_extension];

    /* add fail safe for late command end */
    // prepare tx buffer pointers
    ru->rfdevice.trx_write_func(&ru->rfdevice,
                                      proc->timestamp_tx+ru->ts_offset-ru->openair0_cfg.tx_sample_advance-sf_extension,
                                      txp,
                                      siglen+sf_extension,
                                      ru->nb_tx,
                                      flags);
    LOG_D(PHY,"[TXPATH] RU %d tx_rf, writing to TS %llu, frame %d, unwrapped_frame %d, subframe %d\n",ru->idx,
          (long long unsigned int)proc->timestamp_tx,proc->frame_tx,proc->frame_tx_unwrap,proc->subframe_tx);
  }
}

// this is for RU with local RF unit
void fill_rf_config(RU_t *ru, char *rf_config_file) {
  int i;
  LTE_DL_FRAME_PARMS *fp   = &ru->frame_parms;
  openair0_config_t *cfg   = &ru->openair0_cfg;
  //printf("////////////////numerology in config = %d\n",numerology);
  int numerology = get_softmodem_params()->numerology;

  if(fp->N_RB_DL == 100) {
    if(numerology == 0) {
      if (fp->threequarter_fs) {
        cfg->sample_rate=23.04e6;
        cfg->samples_per_frame = 230400;
        cfg->tx_bw = 10e6;
        cfg->rx_bw = 10e6;
      } else {
        cfg->sample_rate=30.72e6;
        cfg->samples_per_frame = 307200;
        cfg->tx_bw = 10e6;
        cfg->rx_bw = 10e6;
      }
    } else if(numerology == 1) {
      cfg->sample_rate=61.44e6;
      cfg->samples_per_frame = 307200;
      cfg->tx_bw = 20e6;
      cfg->rx_bw = 20e6;
    } else if(numerology == 2) {
      cfg->sample_rate=122.88e6;
      cfg->samples_per_frame = 307200;
      cfg->tx_bw = 40e6;
      cfg->rx_bw = 40e6;
    } else {
      printf("Wrong input for numerology %d\n setting to 20MHz normal CP configuration",numerology);
      cfg->sample_rate=30.72e6;
      cfg->samples_per_frame = 307200;
      cfg->tx_bw = 10e6;
      cfg->rx_bw = 10e6;
    }
  } else if(fp->N_RB_DL == 50) {
    cfg->sample_rate=15.36e6;
    cfg->samples_per_frame = 153600;
    cfg->tx_bw = 5e6;
    cfg->rx_bw = 5e6;
  } else if (fp->N_RB_DL == 25) {
    cfg->sample_rate=7.68e6;
    cfg->samples_per_frame = 76800;
    cfg->tx_bw = 2.5e6;
    cfg->rx_bw = 2.5e6;
  } else if (fp->N_RB_DL == 6) {
    cfg->sample_rate=1.92e6;
    cfg->samples_per_frame = 19200;
    cfg->tx_bw = 1.5e6;
    cfg->rx_bw = 1.5e6;
  } else AssertFatal(1==0,"Unknown N_RB_DL %d\n",fp->N_RB_DL);

  if (fp->frame_type==TDD)
    cfg->duplex_mode = duplex_mode_TDD;
  else //FDD
    cfg->duplex_mode = duplex_mode_FDD;

  cfg->Mod_id = 0;
  cfg->num_rb_dl=fp->N_RB_DL;
  cfg->tx_num_channels=ru->nb_tx;
  cfg->rx_num_channels=ru->nb_rx;
  cfg->clock_source=get_softmodem_params()->clock_source;

  for (i=0; i<ru->nb_tx; i++) {
    cfg->tx_freq[i] = (double)fp->dl_CarrierFreq;
    cfg->rx_freq[i] = (double)fp->ul_CarrierFreq;
    cfg->tx_gain[i] = (double)ru->att_tx;
    cfg->rx_gain[i] = ru->max_rxgain-(double)ru->att_rx;
    cfg->configFilename = rf_config_file;
    printf("channel %d, Setting tx_gain offset %f, rx_gain offset %f, tx_freq %f, rx_freq %f\n",
           i, cfg->tx_gain[i],
           cfg->rx_gain[i],
           cfg->tx_freq[i],
           cfg->rx_freq[i]);
  }
}

/* this function maps the RU tx and rx buffers to the available rf chains.
   Each rf chain is is addressed by the card number and the chain on the card. The
   rf_map specifies for each antenna port, on which rf chain the mapping should start. Multiple
   antennas are mapped to successive RF chains on the same card. */
int setup_RU_buffers(RU_t *ru) {
  int i,j;
  int card,ant;
  //uint16_t N_TA_offset = 0;
  LTE_DL_FRAME_PARMS *frame_parms;

  if (ru) {
    frame_parms = &ru->frame_parms;
    printf("setup_RU_buffers: frame_parms = %p\n",frame_parms);
  } else {
    printf("RU not initialized (NULL pointer)\n");
    return(-1);
  }

  if (frame_parms->frame_type == TDD) {
    if      (frame_parms->N_RB_DL == 100)
      ru->N_TA_offset = 624;
    else if (frame_parms->N_RB_DL == 50)
      ru->N_TA_offset = 624/2;
    else if (frame_parms->N_RB_DL == 25)
      ru->N_TA_offset = 624/4;

    if(IS_SOFTMODEM_BASICSIM)
      /* this is required for the basic simulator in TDD mode
       * TODO: find a proper cleaner solution
       */
      ru->N_TA_offset = 0;

    if      (frame_parms->N_RB_DL == 100) /* no scaling to do */;
    else if (frame_parms->N_RB_DL == 50) {
      ru->sf_extension       /= 2;
      ru->end_of_burst_delay /= 2;
    } else if (frame_parms->N_RB_DL == 25) {
      ru->sf_extension       /= 4;
      ru->end_of_burst_delay /= 4;
    } else {
      printf("not handled, todo\n");
      exit(1);
    }
  } else {
    ru->N_TA_offset = 0;
    ru->sf_extension = 0;
    ru->end_of_burst_delay = 0;
  }

  if (ru->openair0_cfg.mmapped_dma == 1) {
    // replace RX signal buffers with mmaped HW versions
    for (i=0; i<ru->nb_rx; i++) {
      card = i/4;
      ant = i%4;
      printf("Mapping RU id %d, rx_ant %d, on card %d, chain %d\n",ru->idx,i,ru->rf_map.card+card, ru->rf_map.chain+ant);
      free(ru->common.rxdata[i]);
      ru->common.rxdata[i] = ru->openair0_cfg.rxbase[ru->rf_map.chain+ant];
      printf("rxdata[%d] @ %p\n",i,ru->common.rxdata[i]);

      for (j=0; j<16; j++) {
        printf("rxbuffer %d: %x\n",j,ru->common.rxdata[i][j]);
        ru->common.rxdata[i][j] = 16-j;
      }
    }

    for (i=0; i<ru->nb_tx; i++) {
      card = i/4;
      ant = i%4;
      printf("Mapping RU id %d, tx_ant %d, on card %d, chain %d\n",ru->idx,i,ru->rf_map.card+card, ru->rf_map.chain+ant);
      free(ru->common.txdata[i]);
      ru->common.txdata[i] = ru->openair0_cfg.txbase[ru->rf_map.chain+ant];
      printf("txdata[%d] @ %p\n",i,ru->common.txdata[i]);

      for (j=0; j<16; j++) {
        printf("txbuffer %d: %x\n",j,ru->common.txdata[i][j]);
        ru->common.txdata[i][j] = 16-j;
      }
    }
  } else { // not memory-mapped DMA
    //nothing to do, everything already allocated in lte_init
  }

  return(0);
}

static void *ru_thread( void *param ) {
  static int ru_thread_status;
  RU_t               *ru      = (RU_t *)param;
  RU_proc_t          *proc    = &ru->proc;
  int                subframe =9;
  int                frame    =1023;
  // set default return value
  ru_thread_status = 0;

  if (ru->if_south == LOCAL_RF) { // configure RF parameters only
    fill_rf_config(ru,ru->rf_config_file);
    init_frame_parms(&ru->frame_parms,1);
    phy_init_RU(ru);
    openair0_device_load(&ru->rfdevice,&ru->openair0_cfg);
  }

  if (setup_RU_buffers(ru)!=0) {
    printf("Exiting, cannot initialize RU Buffers\n");
    exit(-1);
  }

  LOG_I(PHY, "Signaling main thread that RU %d is ready\n",ru->idx);
  pthread_mutex_lock(&RC.ru_mutex);
  RC.ru_mask &= ~(1<<ru->idx);
  pthread_cond_signal(&RC.ru_cond);
  pthread_mutex_unlock(&RC.ru_mutex);
  wait_sync("ru_thread");

  // Start RF device if any
  if (ru->start_rf) {
    if (ru->start_rf(ru) != 0)
      LOG_E(HW,"Could not start the RF device\n");
    else LOG_I(PHY,"RU %d rf device ready\n",ru->idx);
  } else LOG_I(PHY,"RU %d no rf device\n",ru->idx);

  // This is a forever while loop, it loops over subframes which are scheduled by incoming samples from HW devices
  while (!oai_exit) {
    // these are local subframe/frame counters to check that we are in synch with the fronthaul timing.
    // They are set on the first rx/tx in the underly FH routines.
    if (subframe==9) {
      subframe=0;
      frame++;
      frame&=1023;
    } else {
      subframe++;
    }

    // synchronization on input FH interface, acquire signals/data and block
    AssertFatal(ru->fh_south_in, "No fronthaul interface at south port");
    ru->fh_south_in(ru,&frame,&subframe);

    // adjust for timing offset between RU
    if (ru->idx!=0)
      proc->frame_tx = (proc->frame_tx+proc->frame_offset)&1023;

    // do RX front-end processing (frequency-shift, dft) if needed
    if (ru->feprx)
      ru->feprx(ru);

    // At this point, all information for subframe has been received on FH interface
    // If this proc is to provide synchronization, do so
    // Fixme: not used
    // wakeup_slaves(proc);
    for (int i=0; i<ru->num_eNB; i++) {
      char string[20];
      sprintf(string,"Incoming RU %d",ru->idx);
      ru->eNB_top(ru->eNB_list[i],ru->proc.frame_rx,ru->proc.subframe_rx,string,ru);
    }

    // do TX front-end processing if needed (precoding and/or IDFTs)
    if (ru->feptx_prec)
      ru->feptx_prec(ru);

    // do OFDM if needed
    if ((ru->fh_north_asynch_in == NULL) && (ru->feptx_ofdm))
      ru->feptx_ofdm(ru);

    // do outgoing fronthaul (south) if needed
    if ((ru->fh_north_asynch_in == NULL) && (ru->fh_south_out))
      ru->fh_south_out(ru);
  }

  printf( "Exiting ru_thread \n");

  if (ru->stop_rf != NULL) {
    if (ru->stop_rf(ru) != 0)
      LOG_E(HW,"Could not stop the RF device\n");
    else LOG_I(PHY,"RU %d rf device stopped\n",ru->idx);
  }

  ru_thread_status = 0;
  return &ru_thread_status;
}

int start_rf(RU_t *ru) {
  return(ru->rfdevice.trx_start_func(&ru->rfdevice));
}

int stop_rf(RU_t *ru) {
  ru->rfdevice.trx_end_func(&ru->rfdevice);
  return 0;
}

void init_precoding_weights(PHY_VARS_eNB *eNB) {
  int layer,ru_id,aa,re,ue,tb;
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  RU_t *ru;
  LTE_eNB_DLSCH_t *dlsch;

  // init precoding weigths
  for (ue=0; ue<NUMBER_OF_UE_MAX; ue++) {
    for (tb=0; tb<2; tb++) {
      dlsch = eNB->dlsch[ue][tb];

      for (layer=0; layer<4; layer++) {
        int nb_tx=0;

        for (ru_id=0; ru_id<RC.nb_RU; ru_id++) {
          ru = RC.ru[ru_id];
          nb_tx+=ru->nb_tx;
        }

        dlsch->ue_spec_bf_weights[layer] = (int32_t **)malloc16(nb_tx*sizeof(int32_t *));

        for (aa=0; aa<nb_tx; aa++) {
          dlsch->ue_spec_bf_weights[layer][aa] = (int32_t *)malloc16(fp->ofdm_symbol_size*sizeof(int32_t));

          for (re=0; re<fp->ofdm_symbol_size; re++) {
            dlsch->ue_spec_bf_weights[layer][aa][re] = 0x00007fff;
          }
        }
      }
    }
  }
}

void set_function_spec_param(RU_t *ru) {

  switch (ru->if_south) {
    case LOCAL_RF: { // this is an RU with integrated RF (RRU, eNB)
      ru->do_prach             = 0;                       // no prach processing in RU
      ru->feprx                = fep_full;
      ru->feptx_ofdm           = feptx_ofdm;
      ru->feptx_prec           = feptx_prec;              // this is fep with idft and precoding
      ru->fh_north_in          = NULL;                    // no incoming fronthaul from north
      ru->start_if             = NULL;                    // no if interface
      ru->rfdevice.host_type   = RAU_HOST;
    }

    ru->fh_south_in            = rx_rf;                               // local synchronous RF RX
    ru->fh_south_out           = tx_rf;                               // local synchronous RF TX
    ru->start_rf               = start_rf;                            // need to start the local RF interface
    ru->stop_rf                = stop_rf;
    ru->eNB_top=eNB_top;
    printf("configuring ru_id %d (start_rf %p)\n", ru->idx, start_rf);
      /*
          if (ru->function == eNodeB_3GPP) { // configure RF parameters only for 3GPP eNodeB, we need to get them from RAU otherwise
            fill_rf_config(ru,rf_config_file);
            init_frame_parms(&ru->frame_parms,1);
            phy_init_RU(ru);
          }

          ret = openair0_device_load(&ru->rfdevice,&ru->openair0_cfg);
          if (setup_RU_buffers(ru)!=0) {
            printf("Exiting, cannot initialize RU Buffers\n");
            exit(-1);
          }*/
    break;

    default:
      LOG_E(PHY,"RU with invalid or unknown southbound interface type %d\n",ru->if_south);
      break;
  } // switch on interface type
}

//extern void RCconfig_RU(void);

void init_RU(char *rf_config_file) {
  int ru_id;
  RU_t *ru;
  PHY_VARS_eNB *eNB0= (PHY_VARS_eNB *)NULL;
  int i;
  int CC_id;
  // create status mask
  RC.ru_mask = 0;
  pthread_mutex_init(&RC.ru_mutex,NULL);
  pthread_cond_init(&RC.ru_cond,NULL);
  // read in configuration file)
  printf("configuring RU from file\n");
  RCconfig_RU();
  LOG_I(PHY,"number of L1 instances %d, number of RU %d, number of CPU cores %d\n",
	RC.nb_L1_inst,RC.nb_RU,get_nprocs());

  if (RC.nb_CC != 0)
    for (i=0; i<RC.nb_L1_inst; i++)
      for (CC_id=0; CC_id<RC.nb_CC[i]; CC_id++) RC.eNB[i][CC_id]->num_RU=0;

  LOG_D(PHY,"Process RUs RC.nb_RU:%d\n",RC.nb_RU);

  for (ru_id=0; ru_id<RC.nb_RU; ru_id++) {
    LOG_D(PHY,"Process RC.ru[%d]\n",ru_id);
    ru               = RC.ru[ru_id];
    ru->rf_config_file = rf_config_file;
    ru->idx          = ru_id;
    ru->ts_offset    = 0;
    // use eNB_list[0] as a reference for RU frame parameters
    // NOTE: multiple CC_id are not handled here yet!

    if (ru->num_eNB > 0) {
      LOG_D(PHY, "%s() RC.ru[%d].num_eNB:%d ru->eNB_list[0]:%p RC.eNB[0][0]:%p rf_config_file:%s\n",
	    __FUNCTION__, ru_id, ru->num_eNB, ru->eNB_list[0], RC.eNB[0][0], ru->rf_config_file);

      if (ru->eNB_list[0] == 0) {
        LOG_E(PHY,"%s() DJP - ru->eNB_list ru->num_eNB are not initialized - so do it manually\n",
	      __FUNCTION__);
        ru->eNB_list[0] = RC.eNB[0][0];
        ru->num_eNB=1;
        //
        // DJP - feptx_prec() / feptx_ofdm() parses the eNB_list (based on num_eNB) and copies the txdata_F to txdata in RU
        //
      } else {
        LOG_E(PHY,"DJP - delete code above this %s:%d\n", __FILE__, __LINE__);
      }
    }

    eNB0 = ru->eNB_list[0];
    LOG_D(PHY, "RU FUnction:%d ru->if_south:%d\n", ru->function, ru->if_south);
    LOG_D(PHY, "eNB0:%p\n", eNB0);

    if (eNB0) {
      if ((ru->function != NGFI_RRU_IF5) && (ru->function != NGFI_RRU_IF4p5))
        AssertFatal(eNB0!=NULL,"eNB0 is null!\n");

      if (eNB0) {
        LOG_I(PHY,"Copying frame parms from eNB %d to ru %d\n",eNB0->Mod_id,ru->idx);
        memcpy((void *)&ru->frame_parms,(void *)&eNB0->frame_parms,sizeof(LTE_DL_FRAME_PARMS));
        // attach all RU to all eNBs in its list/
        LOG_D(PHY,"ru->num_eNB:%d eNB0->num_RU:%d\n", ru->num_eNB, eNB0->num_RU);

        for (i=0; i<ru->num_eNB; i++) {
          eNB0 = ru->eNB_list[i];
          eNB0->RU_list[eNB0->num_RU++] = ru;
        }
      }
    }

    LOG_I(PHY,"Initializing RRU descriptor %d : (%s,%s,%d)\n",ru_id,ru_if_types[ru->if_south],eNB_timing[ru->if_timing],ru->function);
    set_function_spec_param(ru);
    LOG_I(PHY,"Starting ru_thread %d\n",ru_id);
    init_RU_proc(ru);
  } // for ru_id

  //  sleep(1);
  LOG_D(HW,"[lte-softmodem.c] RU threads created\n");
}

void stop_RU(int nb_ru) {
  for (int inst = 0; inst < nb_ru; inst++) {
    LOG_I(PHY, "Stopping RU %d processing threads\n", inst);
    kill_RU_proc(RC.ru[inst]);
  }
}

/* --------------------------------------------------------*/
/* from here function to use configuration module          */
void RCconfig_RU(void) {
  int               j                             = 0;
  int               i                             = 0;
  paramdef_t RUParams[] = RUPARAMS_DESC;
  paramlist_def_t RUParamList = {CONFIG_STRING_RU_LIST,NULL,0};
  config_getlist( &RUParamList,RUParams,sizeof(RUParams)/sizeof(paramdef_t), NULL);

  if ( RUParamList.numelt > 0) {
    RC.ru = (RU_t **)malloc(RC.nb_RU*sizeof(RU_t *));
    RC.ru_mask=(1<<RC.nb_RU) - 1;
    printf("Set RU mask to %lx\n",RC.ru_mask);

    for (j = 0; j < RC.nb_RU; j++) {
      RC.ru[j]                                    = (RU_t *)malloc(sizeof(RU_t));
      memset((void *)RC.ru[j],0,sizeof(RU_t));
      RC.ru[j]->idx                                 = j;
      printf("Creating RC.ru[%d]:%p\n", j, RC.ru[j]);
      RC.ru[j]->if_timing                           = synch_to_ext_device;

      if (RC.nb_L1_inst >0)
        RC.ru[j]->num_eNB                           = RUParamList.paramarray[j][RU_ENB_LIST_IDX].numelt;
      else
        RC.ru[j]->num_eNB                           = 0;

      for (i=0; i<RC.ru[j]->num_eNB; i++) RC.ru[j]->eNB_list[i] = RC.eNB[RUParamList.paramarray[j][RU_ENB_LIST_IDX].iptr[i]][0];

      if (config_isparamset(RUParamList.paramarray[j], RU_SDR_ADDRS)) {
        RC.ru[j]->openair0_cfg.sdr_addrs = strdup(*(RUParamList.paramarray[j][RU_SDR_ADDRS].strptr));
      }

      if (config_isparamset(RUParamList.paramarray[j], RU_SDR_CLK_SRC)) {
        if (strcmp(*(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr), "internal") == 0) {
          RC.ru[j]->openair0_cfg.clock_source = internal;
          LOG_D(PHY, "RU clock source set as internal\n");
        } else if (strcmp(*(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr), "external") == 0) {
          RC.ru[j]->openair0_cfg.clock_source = external;
          LOG_D(PHY, "RU clock source set as external\n");
        } else if (strcmp(*(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr), "gpsdo") == 0) {
          RC.ru[j]->openair0_cfg.clock_source = gpsdo;
          LOG_D(PHY, "RU clock source set as gpsdo\n");
        } else {
          LOG_E(PHY, "Erroneous RU clock source in the provided configuration file: '%s'\n", *(RUParamList.paramarray[j][RU_SDR_CLK_SRC].strptr));
        }
      }

      if (strcmp(*(RUParamList.paramarray[j][RU_LOCAL_RF_IDX].strptr), "yes") == 0) {
        if ( !(config_isparamset(RUParamList.paramarray[j],RU_LOCAL_IF_NAME_IDX)) ) {
          RC.ru[j]->if_south                        = LOCAL_RF;
          RC.ru[j]->function                        = eNodeB_3GPP;
          printf("Setting function for RU %d to eNodeB_3GPP\n",j);
        } else {
          RC.ru[j]->eth_params.local_if_name            = strdup(*(RUParamList.paramarray[j][RU_LOCAL_IF_NAME_IDX].strptr));
          RC.ru[j]->eth_params.my_addr                  = strdup(*(RUParamList.paramarray[j][RU_LOCAL_ADDRESS_IDX].strptr));
          RC.ru[j]->eth_params.remote_addr              = strdup(*(RUParamList.paramarray[j][RU_REMOTE_ADDRESS_IDX].strptr));
          RC.ru[j]->eth_params.my_portc                 = *(RUParamList.paramarray[j][RU_LOCAL_PORTC_IDX].uptr);
          RC.ru[j]->eth_params.remote_portc             = *(RUParamList.paramarray[j][RU_REMOTE_PORTC_IDX].uptr);
          RC.ru[j]->eth_params.my_portd                 = *(RUParamList.paramarray[j][RU_LOCAL_PORTD_IDX].uptr);
          RC.ru[j]->eth_params.remote_portd             = *(RUParamList.paramarray[j][RU_REMOTE_PORTD_IDX].uptr);
        }

        RC.ru[j]->max_pdschReferenceSignalPower     = *(RUParamList.paramarray[j][RU_MAX_RS_EPRE_IDX].uptr);;
        RC.ru[j]->max_rxgain                        = *(RUParamList.paramarray[j][RU_MAX_RXGAIN_IDX].uptr);
        RC.ru[j]->num_bands                         = RUParamList.paramarray[j][RU_BAND_LIST_IDX].numelt;
        /* sf_extension is in unit of samples for 30.72MHz here, has to be scaled later */
        RC.ru[j]->sf_extension                      = *(RUParamList.paramarray[j][RU_SF_EXTENSION_IDX].uptr);
        RC.ru[j]->end_of_burst_delay                = *(RUParamList.paramarray[j][RU_END_OF_BURST_DELAY_IDX].uptr);

        for (i=0; i<RC.ru[j]->num_bands; i++) RC.ru[j]->band[i] = RUParamList.paramarray[j][RU_BAND_LIST_IDX].iptr[i];
      } //strcmp(local_rf, "yes") == 0
      else {
        printf("RU %d: Transport %s\n",j,*(RUParamList.paramarray[j][RU_TRANSPORT_PREFERENCE_IDX].strptr));
        RC.ru[j]->eth_params.local_if_name        = strdup(*(RUParamList.paramarray[j][RU_LOCAL_IF_NAME_IDX].strptr));
        RC.ru[j]->eth_params.my_addr          = strdup(*(RUParamList.paramarray[j][RU_LOCAL_ADDRESS_IDX].strptr));
        RC.ru[j]->eth_params.remote_addr        = strdup(*(RUParamList.paramarray[j][RU_REMOTE_ADDRESS_IDX].strptr));
        RC.ru[j]->eth_params.my_portc         = *(RUParamList.paramarray[j][RU_LOCAL_PORTC_IDX].uptr);
        RC.ru[j]->eth_params.remote_portc       = *(RUParamList.paramarray[j][RU_REMOTE_PORTC_IDX].uptr);
        RC.ru[j]->eth_params.my_portd         = *(RUParamList.paramarray[j][RU_LOCAL_PORTD_IDX].uptr);
        RC.ru[j]->eth_params.remote_portd       = *(RUParamList.paramarray[j][RU_REMOTE_PORTD_IDX].uptr);
      }  /* strcmp(local_rf, "yes") != 0 */

      RC.ru[j]->nb_tx                             = *(RUParamList.paramarray[j][RU_NB_TX_IDX].uptr);
      RC.ru[j]->nb_rx                             = *(RUParamList.paramarray[j][RU_NB_RX_IDX].uptr);
      RC.ru[j]->att_tx                            = *(RUParamList.paramarray[j][RU_ATT_TX_IDX].uptr);
      RC.ru[j]->att_rx                            = *(RUParamList.paramarray[j][RU_ATT_RX_IDX].uptr);
    }// j=0..num_rus
  } else {
    RC.nb_RU = 0;
  } // setting != NULL

  return;
}
