#define MTU 65536
#define UDP_TIMEOUT 100000L // in nano second

receiveSubFrame(int sock) {
  //read all subframe data from the control unit
  char * buf[MTU];
  int ret=recv(sock, buf, sizeof(buf), 0);
  if ( ret==-1) {
    if ( errno == EWOULDBLOCK || errno== EINTR ) {
      finishSubframeRecv();
    } else {
      LOG_E(HW,"Critical issue in socket: %s\n", strerror(errno));
      return;
    }
  } else {
  }
      
}

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

phy_procedures_eNB_TX_fs6() {

  receiveSubFrame();

  // We got
  // subframe number
  //

  for (aa = 0; aa < fp->nb_antenna_ports_eNB; aa++) {
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
        pdsch_procedures_fs6(eNB,
			     proc,
			     harq_pid,
			     dlsch0,
			     dlsch1,
			     &eNB->UE_stats[(uint32_t)UE_id],
			     0);
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

DL_thread_fs6() {
  receiveSubFrame();
  phy_procedures_eNB_TX_fs6();
  ru->feptx_prec(ru);
  ru->feptx_ofdm(ru);
  ru->fh_south_out(ru);
}

int createListner (port) {
  int sock;
  AssertFatal((sock=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) >= 0, "");
    struct sockaddr_in addr = {
sin_family:
    AF_INET,
sin_port:
htons(port),
sin_addr:
    { s_addr: INADDR_ANY }
  };
 AssertFatal(bind(sock, const struct sockaddr *addr, socklen_t addrlen)==0,"");
 struct timeval tv={0,UDP_TIMEOUT};
 AssertFatal(setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) ==0,"");
}


DL_thread_frequency() {
  frequency_t header;
  
  full_read(&header, 
