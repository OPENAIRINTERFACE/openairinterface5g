#include <stdint.h>
#include <nfapi/oai_integration/vendor_ext.h>
#include <openair1/PHY/INIT/lte_init.c>
#include <executables/split_headers.h>

#define FS6_BUF_SIZE 100*1000
static UDPsock_t sockFS6;
#if 0

void pdsch_procedures(PHY_VARS_eNB *eNB,
                      L1_rxtx_proc_t *proc,
                      int harq_pid,
                      LTE_eNB_DLSCH_t *dlsch,
                      LTE_eNB_DLSCH_t *dlsch1,
                      LTE_eNB_UE_stats *ue_stats,
                      int ra_flag) {
  int frame=proc->frame_tx;
  int subframe=proc->subframe_tx;
  LTE_DL_eNB_HARQ_t *dlsch_harq=dlsch->harq_processes[harq_pid];
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;

  // 36-212
  if (NFAPI_MODE==NFAPI_MONOLITHIC || NFAPI_MODE==NFAPI_MODE_PNF) { // monolthic OR PNF - do not need turbo encoding on VNF
    // Replace dlsch_encoding
    // data is in
    // dlsch->harq_processes[harq_pid]->e
    feedDlschBuffers(eNB,
                     dlsch_harq->pdu,
                     dlsch_harq->pdsch_start,
                     dlsch,
                     frame,
                     subframe,
                     &eNB->dlsch_rate_matching_stats,
                     &eNB->dlsch_turbo_encoding_stats,
                     &eNB->dlsch_turbo_encoding_waiting_stats,
                     &eNB->dlsch_turbo_encoding_main_stats,
                     &eNB->dlsch_turbo_encoding_wakeup_stats0,
                     &eNB->dlsch_turbo_encoding_wakeup_stats1,
                     &eNB->dlsch_interleaving_stats);
    // 36-211
    dlsch_scrambling(fp,
                     0,
                     dlsch,
                     harq_pid,
                     get_G(fp,
                           dlsch_harq->nb_rb,
                           dlsch_harq->rb_alloc,
                           dlsch_harq->Qm,
                           dlsch_harq->Nl,
                           dlsch_harq->pdsch_start,
                           frame,subframe,
                           0),
                     0,
                     frame,
                     subframe<<1);
    dlsch_modulation(eNB,
                     eNB->common_vars.txdataF,
                     AMP,
                     frame,
                     subframe,
                     dlsch_harq->pdsch_start,
                     dlsch,
                     dlsch->ue_type==0 ? dlsch1 : (LTE_eNB_DLSCH_t *)NULL);
  }

  dlsch->active = 0;
  dlsch_harq->round++;
}
#endif

void phy_procedures_eNB_TX_process(uint8_t *bufferZone, int bufSize, PHY_VARS_eNB *eNB, L1_rxtx_proc_t *proc, int do_meas ) {
  // We got
  // subframe number
  //
  int frame=proc->frame_tx;
  int subframe=proc->subframe_tx;
  uint32_t i,aa;
  uint8_t harq_pid;
  int16_t UE_id=0;
  uint8_t num_pdcch_symbols=0;
  uint8_t num_dci=0;
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))
  uint8_t         num_mdci = 0;
#endif
  uint8_t ul_subframe;
  uint32_t ul_frame;
  LTE_DL_FRAME_PARMS *fp=&eNB->frame_parms;
  LTE_UL_eNB_HARQ_t *ulsch_harq;

  for (int aa = 0; aa < fp->nb_antenna_ports_eNB; aa++) {
    memset (&eNB->common_vars.txdataF[aa][subframe * fp->ofdm_symbol_size * fp->symbols_per_tti],
            0,
            fp->ofdm_symbol_size * (fp->symbols_per_tti) * sizeof (int32_t));
  }

  if (NFAPI_MODE==NFAPI_MONOLITHIC || NFAPI_MODE==NFAPI_MODE_PNF) {
    if (is_pmch_subframe(frame,subframe,fp)) {
      pmch_procedures(eNB,proc);
    } else {
      // this is not a pmch subframe, so generate PSS/SSS/PBCH
      common_signal_procedures(eNB,proc->frame_tx, proc->subframe_tx);
    }
  }

  if (ul_subframe < 10)if (ul_subframe < 10) { // This means that there is a potential UL subframe that will be scheduled here
      for (i=0; i<NUMBER_OF_UE_MAX; i++) {
#if (LTE_RRC_VERSION >= MAKE_VERSION(14, 0, 0))

        if (eNB->ulsch[i] && eNB->ulsch[i]->ue_type >0) harq_pid = 0;
        else
#endif
          harq_pid = subframe2harq_pid(fp,ul_frame,ul_subframe);

        if (eNB->ulsch[i]) {
          ulsch_harq = eNB->ulsch[i]->harq_processes[harq_pid];
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
          ulsch_harq->previous_first_rb = ulsch_harq->first_rb;
          ulsch_harq->previous_n_DMRS = ulsch_harq->n_DMRS;
        }
      }
    }

  num_pdcch_symbols = eNB->pdcch_vars[subframe&1].num_pdcch_symbols;
  num_dci           = eNB->pdcch_vars[subframe&1].num_dci;

  if (num_dci > 0)
    if (NFAPI_MODE==NFAPI_MONOLITHIC || NFAPI_MODE==NFAPI_MODE_PNF) {
      generate_dci_top(num_pdcch_symbols,
                       num_dci,
                       &eNB->pdcch_vars[subframe&1].dci_alloc[0],
                       0,
                       AMP,
                       fp,
                       eNB->common_vars.txdataF,
                       subframe);
      num_mdci = eNB->mpdcch_vars[subframe &1].num_dci;

      if (num_mdci > 0) {
        generate_mdci_top (eNB, frame, subframe, AMP, eNB->common_vars.txdataF);
      }
    }

  // Now scan UE specific DLSCH
  LTE_eNB_DLSCH_t *dlsch0,*dlsch1;

  for (UE_id=0; UE_id<NUMBER_OF_UE_MAX; UE_id++) {
    dlsch0 = eNB->dlsch[(uint8_t)UE_id][0];
    dlsch1 = eNB->dlsch[(uint8_t)UE_id][1];

    if ((dlsch0)&&(dlsch0->rnti>0)&&
        (dlsch0->active == 1)
       ) {
      // get harq_pid
      harq_pid = dlsch0->harq_ids[frame%2][subframe];
      AssertFatal(harq_pid>=0,"harq_pid is negative\n");

      if (harq_pid>=8) {
        if (dlsch0->ue_type==0)
          LOG_E(PHY,"harq_pid:%d corrupt must be 0-7 UE_id:%d frame:%d subframe:%d rnti:%x \n",
                harq_pid,UE_id,frame,subframe,dlsch0->rnti);
      } else {
        // generate pdsch
        /*
              pdsch_procedures_fs6(eNB,
                                   proc,
                                   harq_pid,
                                   dlsch0,
                                   dlsch1,
                                   &eNB->UE_stats[(uint32_t)UE_id],
                                   0);
        */
      }
    } else if ((dlsch0)&&(dlsch0->rnti>0)&&
               (dlsch0->active == 0)
              ) {
      // clear subframe TX flag since UE is not scheduled for PDSCH in this subframe (so that we don't look for PUCCH later)
      dlsch0->subframe_tx[subframe]=0;
    }
  }

  generate_phich_top(eNB,
                     proc,
                     AMP);
}

void prach_eNB_extract(PHY_VARS_eNB *eNB,RU_t *ru,int frame,int subframe, uint8_t *buf, int bufSize) {
  commonUDP_t *header=(commonUDP_t *) buf;
  header->contentBytes=1000;
  header->nbBlocks=1;
  return;
}

void prach_eNB_process(PHY_VARS_eNB *eNB,RU_t *ru,int frame,int subframe) {
}

void phy_procedures_eNB_uespec_RX_extract(PHY_VARS_eNB *phy_vars_eNB,L1_rxtx_proc_t *proc, uint8_t *buf, int bufSize) {
  commonUDP_t *header=(commonUDP_t *) buf;
  header->contentBytes=1000;
  header->nbBlocks=1;
  return;
}

void phy_procedures_eNB_uespec_RX_process(PHY_VARS_eNB *phy_vars_eNB,L1_rxtx_proc_t *proc) {
}

void phy_procedures_eNB_TX_extract(PHY_VARS_eNB *eNB, L1_rxtx_proc_t *proc, int do_meas, uint8_t *buf, int bufSize) {
  commonUDP_t *header=(commonUDP_t *) buf;
  header->contentBytes=1000;
  header->nbBlocks=1;
  return;
}

void DL_du_fs6(RU_t *ru, int frame, int subframe, uint64_t TS) {
  RU_proc_t *ru_proc=&ru->proc;

  for (int i=0; i<ru->num_eNB; i++) {
    uint8_t bufferZone[FS6_BUF_SIZE];
    receiveSubFrame(&sockFS6, TS, bufferZone, sizeof(bufferZone) );
    phy_procedures_eNB_TX_process( bufferZone, sizeof(bufferZone), ru->eNB_list[i], &ru->eNB_list[i]->proc.L1_proc, 1);
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
  L1_rxtx_proc_t *L1_proc = &proc->L1_proc;
  LTE_DL_FRAME_PARMS *fp = &ru->frame_parms;
  proc->frame_rx    = frame;
  proc->subframe_rx = subframe;

  if (NFAPI_MODE==NFAPI_MODE_PNF) {
    // I am a PNF and I need to let nFAPI know that we have a (sub)frame tick
    //add_subframe(&frame, &subframe, 4);
    //oai_subframe_ind(proc->frame_tx, proc->subframe_tx);
    oai_subframe_ind(proc->frame_rx, proc->subframe_rx);
  }

  uint8_t bufferZone[FS6_BUF_SIZE];
  prach_eNB_extract(eNB,NULL,proc->frame_rx,proc->subframe_rx, bufferZone, FS6_BUF_SIZE);

  if (NFAPI_MODE==NFAPI_MONOLITHIC || NFAPI_MODE==NFAPI_MODE_PNF) {
    phy_procedures_eNB_uespec_RX_extract(eNB, &proc->L1_proc, bufferZone, FS6_BUF_SIZE);
  }

  for (int i=0; i<ru->num_eNB; i++) {
    sendSubFrame(&sockFS6,bufferZone);
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
  uint8_t bufferZone[FS6_BUF_SIZE];
  phy_procedures_eNB_TX_extract(eNB, &proc->L1_proc, 1, bufferZone, FS6_BUF_SIZE);
  sendSubFrame(&sockFS6, bufferZone );
}

void UL_cu_fs6(RU_t *ru,int frame, int subframe, uint64_t TS) {
  uint8_t bufferZone[FS6_BUF_SIZE];
  receiveSubFrame(&sockFS6, TS, bufferZone, sizeof(bufferZone) );
  // Fixme: datamodel issue
  PHY_VARS_eNB *eNB = RC.eNB[0][0];
  L1_proc_t *proc           = &eNB->proc;
  prach_eNB_process(eNB,NULL,proc->frame_rx,proc->subframe_rx);
  release_UE_in_freeList(eNB->Mod_id);

  if (NFAPI_MODE==NFAPI_MONOLITHIC || NFAPI_MODE==NFAPI_MODE_PNF) {
    phy_procedures_eNB_uespec_RX_process(eNB, &proc->L1_proc);
  }
}

void *cu_fs6(void *arg) {
  RU_t               *ru      = (RU_t *)arg;
  RU_proc_t          *proc    = &ru->proc;
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
  RU_proc_t          *proc    = &ru->proc;
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
