/*
 * Author: Laurent Thomas
 * Copyright: Open Cells Project company
 */

/*
 * This file replaces
 * targets/RT/USER/lte-ru.c
 * targets/RT/USER/lte-enb.c
 * openair1/SCHED/prach_procedures.c
 * The merger of OpenAir central code to this branch
 * should check if these 3 files are modified and analyze if code code has to be copied in here
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
#include <openair1/PHY/LTE_REFSIG/lte_refsig.h>
#include <nfapi/oai_integration/nfapi_pnf.h>
#include <executables/split_headers.h>

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

  pthread_t t;
  char *fs6=getenv("fs6");

  if (fs6) {
    if ( strncasecmp(fs6,"cu", 2) == 0 )
      threadCreate(&t,  cu_fs6, (void *)ru, "MainCu", -1, OAI_PRIORITY_RT_MAX);
    else if ( strncasecmp(fs6,"du", 2) == 0 )
      threadCreate(&t,  du_fs6, (void *)ru, "MainDu", -1, OAI_PRIORITY_RT_MAX);
    else
      AssertFatal(false, "environement variable fs6 is not cu or du");
  } else
    threadCreate(&t,  ru_thread, (void *)ru, "MainRu", -1, OAI_PRIORITY_RT_MAX);
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
      eNB->frame_parms.nb_antennas_tx       = 0;
      eNB->prach_vars.rxsigF[0] = (int16_t **)malloc16(64*sizeof(int16_t *));

      for (int ce_level=0; ce_level<4; ce_level++) {
        LOG_I(PHY,"Overwriting eNB->prach_vars_br.rxsigF.rxsigF[0]:%p\n", eNB->prach_vars_br.rxsigF[ce_level]);
        eNB->prach_vars_br.rxsigF[ce_level] = (int16_t **)malloc16(64*sizeof(int16_t *));
      }

      for (ru_id=0,aa=0; ru_id<eNB->num_RU; ru_id++) {
        eNB->frame_parms.nb_antennas_rx    += eNB->RU_list[ru_id]->nb_rx;
        eNB->frame_parms.nb_antennas_tx    += eNB->RU_list[ru_id]->nb_tx;
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
      AssertFatal( eNB->frame_parms.nb_antennas_rx > 0 && eNB->frame_parms.nb_antennas_rx < 4, "");
      AssertFatal( eNB->frame_parms.nb_antennas_tx > 0 && eNB->frame_parms.nb_antennas_rx < 4, "");
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
      eNB->UL_INFO.cqi_ind.cqi_indication_body.cqi_pdu_list = eNB->cqi_pdu_list;
      eNB->UL_INFO.cqi_ind.cqi_indication_body.cqi_raw_pdu_list = eNB->cqi_raw_pdu_list;
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
      LOG_E(PHY,"Wrong input for numerology %d\n setting to 20MHz normal CP configuration",numerology);
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
    LOG_I(PHY,"channel %d, Setting tx_gain offset %f, rx_gain offset %f, tx_freq %f, rx_freq %f\n",
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
  //uint16_t N_TA_offset = 0;
  LTE_DL_FRAME_PARMS *frame_parms;
  AssertFatal(ru, "ru is NULL");
  frame_parms = &ru->frame_parms;
  LOG_I(PHY,"setup_RU_buffers: frame_parms = %p\n",frame_parms);

  if (frame_parms->frame_type == TDD) {
    if (frame_parms->N_RB_DL == 100) {
      ru->N_TA_offset = 624;
    } else if (frame_parms->N_RB_DL == 50) {
      ru->N_TA_offset = 624/2;
      ru->sf_extension       /= 2;
      ru->end_of_burst_delay /= 2;
    } else if (frame_parms->N_RB_DL == 25) {
      ru->N_TA_offset = 624/4;
      ru->sf_extension       /= 4;
      ru->end_of_burst_delay /= 4;
    } else {
      LOG_E(PHY,"not handled, todo\n");
      exit(1);
    }
  } else {
    ru->N_TA_offset = 0;
    ru->sf_extension = 0;
    ru->end_of_burst_delay = 0;
  }

  return(0);
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

void prach_procedures_ocp(PHY_VARS_eNB *eNB, int br_flag, int frame, int subframe) {
  uint16_t max_preamble[4],max_preamble_energy[4],max_preamble_delay[4],avg_preamble_energy[4];

  if (br_flag==0) {
    eNB->proc.frame_prach = frame;
    eNB->proc.subframe_prach = subframe;
  } else {
    eNB->proc.frame_prach_br = frame;
    eNB->proc.subframe_prach_br = subframe;
  }

  RU_t *ru;
  int aa=0;
  int ru_aa;

  for (int i=0; i<eNB->num_RU; i++) {
    ru=eNB->RU_list[i];

    for (ru_aa=0,aa=0; ru_aa<ru->nb_rx; ru_aa++,aa++) {
      eNB->prach_vars.rxsigF[0][aa] = eNB->RU_list[i]->prach_rxsigF[ru_aa];
      int ce_level;

      if (br_flag==1)
        for (ce_level=0; ce_level<4; ce_level++)
          eNB->prach_vars_br.rxsigF[ce_level][aa] = eNB->RU_list[i]->prach_rxsigF_br[ce_level][ru_aa];
    }
  }

  // run PRACH detection for CE-level 0 only for now when br_flag is set
  rx_prach(eNB,
           eNB->RU_list[0],
           &max_preamble[0],
           &max_preamble_energy[0],
           &max_preamble_delay[0],
           &avg_preamble_energy[0],
           frame,
           0
           ,br_flag
          );

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
} // else br_flag

void prach_eNB(PHY_VARS_eNB *eNB,RU_t *ru,int frame,int subframe) {
  // check if we have to detect PRACH first
  if (is_prach_subframe(&eNB->frame_parms, frame,subframe)>0) {
    prach_procedures_ocp(eNB, 0, frame, subframe);
    prach_procedures_ocp(eNB, 1, frame, subframe);
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
  prach_eNB(eNB,NULL,proc->frame_rx,proc->subframe_rx);
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
    AssertFatal( proc->subframe_rx == *subframe && proc->frame_rx == *frame,
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

static void *ru_thread( void *param ) {
  RU_t               *ru      = (RU_t *)param;
  RU_proc_t          *proc    = &ru->proc;
  int                subframe =9;
  int                frame    =1023;

  if (ru->if_south == LOCAL_RF) { // configure RF parameters only
    fill_rf_config(ru,ru->rf_config_file);
    init_frame_parms(&ru->frame_parms,1);
    phy_init_RU(ru);
    openair0_device_load(&ru->rfdevice,&ru->openair0_cfg);
  }

  AssertFatal(setup_RU_buffers(ru)==0, "Exiting, cannot initialize RU Buffers\n");
  LOG_I(PHY, "Signaling main thread that RU %d is ready\n",ru->idx);
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

  LOG_W(PHY,"Exiting ru_thread \n");

  if (ru->stop_rf != NULL) {
    if (ru->stop_rf(ru) != 0)
      LOG_E(HW,"Could not stop the RF device\n");
    else LOG_I(PHY,"RU %d rf device stopped\n",ru->idx);
  }

  return NULL;
}

int start_rf(RU_t *ru) {
  return(ru->rfdevice.trx_start_func(&ru->rfdevice));
}

int stop_rf(RU_t *ru) {
  ru->rfdevice.trx_end_func(&ru->rfdevice);
  return 0;
}

void set_function_spec_param(RU_t *ru) {
  switch (ru->if_south) {
    case LOCAL_RF: // this is an RU with integrated RF (RRU, eNB)
      ru->feprx                = fep_full;
      ru->feptx_ofdm           = feptx_ofdm;
      ru->feptx_prec           = feptx_prec;              // this is fep with idft and precoding
      ru->rfdevice.host_type   = RAU_HOST;
      ru->fh_south_in          = rx_rf;                               // local synchronous RF RX
      ru->fh_south_out         = tx_rf;                               // local synchronous RF TX
      ru->start_rf             = start_rf;                            // need to start the local RF interface
      ru->stop_rf              = stop_rf;
      ru->eNB_top              = eNB_top;
      break;

    default:
      LOG_E(PHY,"RU with invalid or unknown southbound interface type %d\n",ru->if_south);
      break;
  } // switch on interface type
}

//extern void RCconfig_RU(void);

void init_RU(char *rf_config_file, clock_source_t clock_source,clock_source_t time_source,int send_dmrssync) {
  int ru_id;
  RU_t *ru;
  PHY_VARS_eNB *eNB0= (PHY_VARS_eNB *)NULL;
  int i;
  int CC_id;
  // create status mask
  pthread_mutex_init(&RC.ru_mutex,NULL);
  pthread_cond_init(&RC.ru_cond,NULL);
  // read in configuration file)
  LOG_I(PHY,"configuring RU from file\n");
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

    if (ru->is_slave == 1) {
      ru->in_synch    = 0;
      ru->generate_dmrs_sync = 0;
    } else {
      ru->in_synch    = 1;
      ru->generate_dmrs_sync=send_dmrssync;
    }

    ru->cmd      = EMPTY;
    ru->south_out_cnt= 0;
    // use eNB_list[0] as a reference for RU frame parameters
    // NOTE: multiple CC_id are not handled here yet!
    ru->openair0_cfg.clock_source  = clock_source;
    ru->openair0_cfg.time_source = time_source;

    //    ru->generate_dmrs_sync = (ru->is_slave == 0) ? 1 : 0;
    if (ru->generate_dmrs_sync == 1) {
      generate_ul_ref_sigs();
      ru->dmrssync = (int16_t *)malloc16_clear(ru->frame_parms.ofdm_symbol_size*2*sizeof(int16_t));
    }

    ru->wakeup_L1_sleeptime = 2000;
    ru->wakeup_L1_sleep_cnt_max  = 3;

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
      LOG_I(PHY,"Copying frame parms from eNB %d to ru %d\n",eNB0->Mod_id,ru->idx);
      memcpy((void *)&ru->frame_parms,(void *)&eNB0->frame_parms,sizeof(LTE_DL_FRAME_PARMS));

      for (i=0; i<ru->num_eNB; i++) {
        eNB0 = ru->eNB_list[i];
        eNB0->RU_list[eNB0->num_RU++] = ru;
      }
    }

    LOG_I(PHY,"Initializing RRU descriptor %d : (%s,%s,%d)\n",
          ru_id,ru_if_types[ru->if_south],eNB_timing[ru->if_timing],ru->function);
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

  if ( RUParamList.numelt == 0 ) {
    RC.nb_RU = 0;
    return;
  } // setting != NULL

  RC.ru = (RU_t **)malloc(RC.nb_RU*sizeof(RU_t *));

  for (j = 0; j < RC.nb_RU; j++) {
    RC.ru[j] = (RU_t *)calloc(sizeof(RU_t), 1);
    RC.ru[j]->idx = j;
    LOG_I(PHY,"Creating RC.ru[%d]:%p\n", j, RC.ru[j]);
    RC.ru[j]->if_timing = synch_to_ext_device;
    paramdef_t *vals=RUParamList.paramarray[j];

    if (RC.nb_L1_inst >0)
      RC.ru[j]->num_eNB = vals[RU_ENB_LIST_IDX].numelt;
    else
      RC.ru[j]->num_eNB = 0;

    for (i=0; i<RC.ru[j]->num_eNB; i++)
      RC.ru[j]->eNB_list[i] = RC.eNB[vals[RU_ENB_LIST_IDX].iptr[i]][0];

    if (config_isparamset(vals, RU_SDR_ADDRS)) {
      RC.ru[j]->openair0_cfg.sdr_addrs = strdup(*(vals[RU_SDR_ADDRS].strptr));
    }

    if (config_isparamset(vals, RU_SDR_CLK_SRC)) {
      char *paramVal=*(vals[RU_SDR_CLK_SRC].strptr);
      LOG_D(PHY, "RU clock source set as %s\n", paramVal);

      if (strcmp(paramVal, "internal") == 0) {
        RC.ru[j]->openair0_cfg.clock_source = internal;
      } else if (strcmp(paramVal, "external") == 0) {
        RC.ru[j]->openair0_cfg.clock_source = external;
      } else if (strcmp(paramVal, "gpsdo") == 0) {
        RC.ru[j]->openair0_cfg.clock_source = gpsdo;
      } else {
        LOG_E(PHY, "Erroneous RU clock source in the provided configuration file: '%s'\n", paramVal);
      }
    }

    if (strcmp(*(vals[RU_LOCAL_RF_IDX].strptr), "yes") == 0) {
      if ( !(config_isparamset(vals,RU_LOCAL_IF_NAME_IDX)) ) {
        RC.ru[j]->if_south  = LOCAL_RF;
        RC.ru[j]->function  = eNodeB_3GPP;
        LOG_I(PHY, "Setting function for RU %d to eNodeB_3GPP\n",j);
      } else {
        RC.ru[j]->eth_params.local_if_name = strdup(*(vals[RU_LOCAL_IF_NAME_IDX].strptr));
        RC.ru[j]->eth_params.my_addr       = strdup(*(vals[RU_LOCAL_ADDRESS_IDX].strptr));
        RC.ru[j]->eth_params.remote_addr   = strdup(*(vals[RU_REMOTE_ADDRESS_IDX].strptr));
        RC.ru[j]->eth_params.my_portc      = *(vals[RU_LOCAL_PORTC_IDX].uptr);
        RC.ru[j]->eth_params.remote_portc  = *(vals[RU_REMOTE_PORTC_IDX].uptr);
        RC.ru[j]->eth_params.my_portd      = *(vals[RU_LOCAL_PORTD_IDX].uptr);
        RC.ru[j]->eth_params.remote_portd  = *(vals[RU_REMOTE_PORTD_IDX].uptr);
      }

      RC.ru[j]->max_pdschReferenceSignalPower = *(vals[RU_MAX_RS_EPRE_IDX].uptr);;
      RC.ru[j]->max_rxgain                    = *(vals[RU_MAX_RXGAIN_IDX].uptr);
      RC.ru[j]->num_bands                     = vals[RU_BAND_LIST_IDX].numelt;
      /* sf_extension is in unit of samples for 30.72MHz here, has to be scaled later */
      RC.ru[j]->sf_extension                  = *(vals[RU_SF_EXTENSION_IDX].uptr);
      RC.ru[j]->end_of_burst_delay            = *(vals[RU_END_OF_BURST_DELAY_IDX].uptr);

      for (i=0; i<RC.ru[j]->num_bands; i++) RC.ru[j]->band[i] = vals[RU_BAND_LIST_IDX].iptr[i];
    } else {
      LOG_I(PHY,"RU %d: Transport %s\n",j,*(vals[RU_TRANSPORT_PREFERENCE_IDX].strptr));
      RC.ru[j]->eth_params.local_if_name    = strdup(*(vals[RU_LOCAL_IF_NAME_IDX].strptr));
      RC.ru[j]->eth_params.my_addr          = strdup(*(vals[RU_LOCAL_ADDRESS_IDX].strptr));
      RC.ru[j]->eth_params.remote_addr      = strdup(*(vals[RU_REMOTE_ADDRESS_IDX].strptr));
      RC.ru[j]->eth_params.my_portc         = *(vals[RU_LOCAL_PORTC_IDX].uptr);
      RC.ru[j]->eth_params.remote_portc     = *(vals[RU_REMOTE_PORTC_IDX].uptr);
      RC.ru[j]->eth_params.my_portd         = *(vals[RU_LOCAL_PORTD_IDX].uptr);
      RC.ru[j]->eth_params.remote_portd     = *(vals[RU_REMOTE_PORTD_IDX].uptr);
    }  /* strcmp(local_rf, "yes") != 0 */

    RC.ru[j]->nb_tx                             = *(vals[RU_NB_TX_IDX].uptr);
    RC.ru[j]->nb_rx                             = *(vals[RU_NB_RX_IDX].uptr);
    RC.ru[j]->att_tx                            = *(vals[RU_ATT_TX_IDX].uptr);
    RC.ru[j]->att_rx                            = *(vals[RU_ATT_RX_IDX].uptr);
  }// j=0..num_rus

  return;
}
