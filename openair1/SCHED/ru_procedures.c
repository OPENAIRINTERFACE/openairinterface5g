
// RU OFDM Modulator, used in IF4p5 RRU, RCC/RAU with IF5, eNodeB

void do_OFDM_mod_rt(int subframe,RU_t *ru) {
     
  LTE_DL_FRAME_PARMS *fp=&ru->frame_parms;

  unsigned int aa,slot_offset, slot_offset_F;
  int dummy_tx_b[7680*4] __attribute__((aligned(32)));
  int i,j, tx_offset;
  int slot_sizeF = (fp->ofdm_symbol_size)*
                   ((fp->Ncp==1) ? 6 : 7);
  int len,len2;
  int16_t *txdata;
//  int CC_id = ru->proc.CC_id;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_SFGEN , 1 );

  slot_offset_F = (subframe<<1)*slot_sizeF;

  slot_offset = subframe*fp->samples_per_tti;

  if ((subframe_select(fp,subframe)==SF_DL)||
      ((subframe_select(fp,subframe)==SF_S))) {
    //    LOG_D(HW,"Frame %d: Generating slot %d\n",frame,next_slot);

    for (aa=0; aa<ru->nb_tx; aa++) {
      if (fp->Ncp == EXTENDED) {
        PHY_ofdm_mod(&ru->ru_time.txdataF[aa][slot_offset_F],
                     dummy_tx_b,
                     fp->ofdm_symbol_size,
                     6,
                     fp->nb_prefix_samples,
                     CYCLIC_PREFIX);
        PHY_ofdm_mod(&ru->ru_time.txdataF[aa][slot_offset_F+slot_sizeF],
                     dummy_tx_b+(fp->samples_per_tti>>1),
                     fp->ofdm_symbol_size,
                     6,
                     fp->nb_prefix_samples,
                     CYCLIC_PREFIX);
      } else {
        normal_prefix_mod(&ru->ru_time.txdataF[aa][slot_offset_F],
                          dummy_tx_b,
                          7,
                          fp);
	// if S-subframe generate first slot only
	if (subframe_select(fp,subframe) == SF_DL) 
	  normal_prefix_mod(&ru->ru_time.txdataF[aa][slot_offset_F+slot_sizeF],
			    dummy_tx_b+(fp->samples_per_tti>>1),
			    7,
			    fp);
      }

      // if S-subframe generate first slot only
      if (subframe_select(fp,subframe) == SF_S)
	len = fp->samples_per_tti>>1;
      else
	len = fp->samples_per_tti;
      /*
      for (i=0;i<len;i+=4) {
	dummy_tx_b[i] = 0x100;
	dummy_tx_b[i+1] = 0x01000000;
	dummy_tx_b[i+2] = 0xff00;
	dummy_tx_b[i+3] = 0xff000000;
	}*/
      
      if (slot_offset+time_offset[aa]<0) {
	txdata = (int16_t*)&ru->ru_time.txdata[aa][(LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti)+tx_offset];
        len2 = -(slot_offset+time_offset[aa]);
	len2 = (len2>len) ? len : len2;
	for (i=0; i<(len2<<1); i++) {
	  txdata[i] = ((int16_t*)dummy_tx_b)[i]<<openair0_cfg[0].iq_txshift;
	}
	if (len2<len) {
	  txdata = (int16_t*)&ru->ru_time.txdata[aa][0];
	  for (j=0; i<(len<<1); i++,j++) {
	    txdata[j++] = ((int16_t*)dummy_tx_b)[i]<<openair0_cfg[0].iq_txshift;
	  }
	}
      }  
      else if ((slot_offset+time_offset[aa]+len)>(LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti)) {
	tx_offset = (int)slot_offset+time_offset[aa];
	txdata = (int16_t*)&ru->ru_time.txdata[aa][tx_offset];
	len2 = -tx_offset+LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti;
	for (i=0; i<(len2<<1); i++) {
	  txdata[i] = ((int16_t*)dummy_tx_b)[i]<<openair0_cfg[0].iq_txshift;
	}
	txdata = (int16_t*)&ru->ru_time.txdata[aa][0];
	for (j=0; i<(len<<1); i++,j++) {
	  txdata[j++] = ((int16_t*)dummy_tx_b)[i]<<openair0_cfg[0].iq_txshift;
	}
      }
      else {
	tx_offset = (int)slot_offset+time_offset[aa];
	txdata = (int16_t*)&ru->ru_time.txdata[aa][tx_offset];

	for (i=0; i<(len<<1); i++) {
	  txdata[i] = ((int16_t*)dummy_tx_b)[i]<<openair0_cfg[0].iq_txshift;
	}
      }
      
     // if S-subframe switch to RX in second subframe
      /*
     if (subframe_select(fp,subframe) == SF_S) {
       for (i=0; i<len; i++) {
	 ru->common_vars.txdata[0][aa][tx_offset++] = 0x00010001;
       }
     }
      */
     if ((((fp->tdd_config==0) ||
	   (fp->tdd_config==1) ||
	   (fp->tdd_config==2) ||
	   (fp->tdd_config==6)) && 
	   (subframe==0)) || (subframe==5)) {
       // turn on tx switch N_TA_offset before
       //LOG_D(HW,"subframe %d, time to switch to tx (N_TA_offset %d, slot_offset %d) \n",subframe,ru->N_TA_offset,slot_offset);
       for (i=0; i<ru->N_TA_offset; i++) {
         tx_offset = (int)slot_offset+time_offset[aa]+i-ru->N_TA_offset/2;
         if (tx_offset<0)
           tx_offset += LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti;
	 
         if (tx_offset>=(LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti))
           tx_offset -= LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*fp->samples_per_tti;
	 
         ru->ru_time.txdata[aa][tx_offset] = 0x00000000;
       }
     }
    }
  }
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_ENB_SFGEN , 0 );
}


typedef struct {
  RU_t *ru;
  int slot;
} fep_task;

void fep0(RU_t *ru,int slot) {

  eNB_proc_t *proc       = &ru->proc;
  LTE_DL_FRAME_PARMS *fp = ru->frame_parms;
  int l;

  //  printf("fep0: slot %d\n",slot);

  remove_7_5_kHz(ru,(slot&1)+(proc->subframe_rx<<1));
  for (l=0; l<fp->symbols_per_tti/2; l++) {
    slot_fep_ul(fp,
		&ru->ru_time,
		l,
		(slot&1)+(proc->subframe_rx<<1),
		0,
		0
		);
  }
}



extern int oai_exit;

static void *fep_thread(void *param) {

  RU_t *ru = (RU_t *)param;
  ru_proc_t *proc  = &ru->proc;
  while (!oai_exit) {

    if (wait_on_condition(&proc->mutex_fep,&proc->cond_fep,&proc->instance_cnt_fep,"fep thread")<0) break;  
    fep0(ru,0);
    if (release_thread(&proc->mutex_fep,&proc->instance_cnt_fep,"fep thread")<0) break;

    if (pthread_cond_signal(&proc->cond_fep) != 0) {
      printf("[eNB] ERROR pthread_cond_signal for fep thread exit\n");
      exit_fun( "ERROR pthread_cond_signal" );
      return NULL;
    }
  }



  return(NULL);
}

void init_fep_thread(PHY_VARS_eNB *eNB,pthread_attr_t *attr_fep) {

  eNB_proc_t *proc = &eNB->proc;

  proc->instance_cnt_fep         = -1;
    
  pthread_mutex_init( &proc->mutex_fep, NULL);
  pthread_cond_init( &proc->cond_fep, NULL);

  pthread_create(&proc->pthread_fep, attr_fep, fep_thread, (void*)eNB);


}
void ru_fep_full_2thread(RU_t *ru) {

  ru_proc_t *proc = &ru->proc;

  struct timespec wait;

  wait.tv_sec=0;
  wait.tv_nsec=5000000L;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_SLOT_FEP,1);
  start_meas(&ru->ofdm_demod_stats);

  if (pthread_mutex_timedlock(&proc->mutex_fep,&wait) != 0) {
    printf("[RU] ERROR pthread_mutex_lock for fep thread (IC %d)\n", proc->instance_cnt_fep);
    exit_fun( "error locking mutex_fep" );
    return;
  }

  if (proc->instance_cnt_fep==0) {
    printf("[RU] FEP thread busy\n");
    exit_fun("FEP thread busy");
    pthread_mutex_unlock( &proc->mutex_fep );
    return;
  }
  
  ++proc->instance_cnt_fep;


  if (pthread_cond_signal(&proc->cond_fep) != 0) {
    printf("[RU] ERROR pthread_cond_signal for fep thread\n");
    exit_fun( "ERROR pthread_cond_signal" );
    return;
  }
  
  pthread_mutex_unlock( &proc->mutex_fep );

  // call second slot in this symbol
  fep0(ru,1);

  wait_on_busy_condition(&proc->mutex_fep,&proc->cond_fep,&proc->instance_cnt_fep,"fep thread");  

  stop_meas(&ru->ofdm_demod_stats);
}



void ru_fep_full(RU_t *ru) {

  ru_proc_t *proc = &ru->proc;
  int l;
  LTE_DL_FRAME_PARMS *fp=ru->frame_parms;

  start_meas(&ru->ofdm_demod_stats);

  if (ru == RC.ru_list[0]) VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_SLOT_FEP,1);

  remove_7_5_kHz(ru,proc->subframe_rx<<1);
  remove_7_5_kHz(ru,1+(proc->subframe_rx<<1));
  for (l=0; l<fp->symbols_per_tti/2; l++) {
    slot_fep_ul(fp,
		&ru->ru_time,
		l,
		proc->subframe_rx<<1,
		0,
		0
		);
    slot_fep_ul(fp,
		&ru->ru_time,
		l,
		1+(proc->subframe_rx<<1),
		0,
		0
		);
  }
  stop_meas(&ru->ofdm_demod_stats);
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_SLOT_FEP,0);
  
  if (ru->node_function == NGFI_RRU_IF4p5) {
    /// **** send_IF4 of rxdataF to RCC (no prach now) **** ///
    send_IF4p5(ru, proc->frame_rx, proc->subframe_rx, IF4p5_PULFFT, 0);
  }    
}


void do_prach_ru(RU_t *ru) {

  ru_proc_t *proc = &ru->proc;
  LTE_DL_FRAME_PARMS *fp->frame_parms;

  // check if we have to detect PRACH first
  if (is_prach_subframe(fp,proc->frame_rx,proc->subframe_rx)>0) { 
    /* accept some delay in processing - up to 5ms */
    int i;
    for (i = 0; i < 10 && proc->instance_cnt_prach == 0; i++) {
      LOG_W(PHY,"[eNB] Frame %d Subframe %d, eNB PRACH thread busy (IC %d)!!\n", proc->frame_rx,proc->subframe_rx,proc->instance_cnt_prach);
      usleep(500);
    }
    if (proc->instance_cnt_prach == 0) {
      exit_fun( "PRACH thread busy" );
      return;
    }
    
    // wake up thread for PRACH RX
    if (pthread_mutex_lock(&proc->mutex_prach) != 0) {
      LOG_E( PHY, "[eNB] ERROR pthread_mutex_lock for eNB PRACH thread %d (IC %d)\n", proc->instance_cnt_prach );
      exit_fun( "error locking mutex_prach" );
      return;
    }
    
    ++proc->instance_cnt_prach;
    // set timing for prach thread
    proc->frame_prach = proc->frame_rx;
    proc->subframe_prach = proc->subframe_rx;
    
    // the thread can now be woken up
    if (pthread_cond_signal(&proc->cond_prach) != 0) {
      LOG_E( PHY, "[eNB] ERROR pthread_cond_signal for eNB PRACH thread %d\n", proc->thread_index);
      exit_fun( "ERROR pthread_cond_signal" );
      return;
    }
    
    pthread_mutex_unlock( &proc->mutex_prach );
  }

}
