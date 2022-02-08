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


/*
 * This file replaces
 * targets/RT/USER/lte-softmodem.c
 * targets/RT/USER/rt_wrapper.c
 * targets/RT/USER/lte-ru.c
 * targets/RT/USER/lte-enb.c
 * targets/RT/USER/ru_control.c
 * openair1/SCHED/prach_procedures.c
 * The merger of OpenAir central code to this branch
 * should check if these 3 files are modified and analyze if code code has to be copied in here
 */
#define  _GNU_SOURCE
#include <pthread.h>

#include <common/utils/LOG/log.h>
#include <common/utils/system.h>
#include <common/utils/assertions.h>
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
#include <common/utils/threadPool/thread-pool.h>
#include <openair2/ENB_APP/NB_IoT_interface.h>
#include <common/utils/load_module_shlib.h>
#include <targets/COMMON/create_tasks.h>
#include <openair1/PHY/TOOLS/phy_scope_interface.h>
#include <openair2/UTIL/OPT/opt.h>
#include <openair1/SIMULATION/TOOLS/sim.h>
#include <openair1/PHY/phy_vars.h>
#include <openair1/SCHED/sched_common_vars.h>
#include <openair2/LAYER2/MAC/mac_vars.h>
#include <openair2/RRC/LTE/rrc_vars.h>

pthread_cond_t nfapi_sync_cond;
pthread_mutex_t nfapi_sync_mutex;
int nfapi_sync_var=-1; //!< protected by mutex \ref nfapi_sync_mutex
pthread_cond_t sync_cond;
pthread_mutex_t sync_mutex;
int sync_var=-1; //!< protected by mutex \ref sync_mutex.
int config_sync_var=-1;
volatile int oai_exit = 0;
double cpuf;
THREAD_STRUCT thread_struct;

uint16_t sf_ahead=4;
//uint16_t slot_ahead=6;
int otg_enabled;
uint64_t  downlink_frequency[MAX_NUM_CCs][4];
int32_t   uplink_frequency_offset[MAX_NUM_CCs][4];
int split73;
char *split73_config;
int split73;
AGENT_RRC_xface *agent_rrc_xface[NUM_MAX_ENB]= {0};
AGENT_MAC_xface *agent_mac_xface[NUM_MAX_ENB]= {0};
void flexran_agent_slice_update(mid_t module_idP) {
}
int proto_agent_start(mod_id_t mod_id, const cudu_params_t *p) {
  return 0;
}
void proto_agent_stop(mod_id_t mod_id) {
}

static void *ru_thread( void *param );
void kill_RU_proc(RU_t *ru) {
}
void kill_eNB_proc(int inst) {
}
void free_transport(PHY_VARS_eNB *eNB) {
}
void reset_opp_meas(void) {
}
extern void  phy_free_RU(RU_t *);

void exit_function(const char *file, const char *function, const int line, const char *s) {
  if (s != NULL) {
    printf("%s:%d %s() Exiting OAI softmodem: %s\n",file,line, function, s);
  }

  close_log_mem();
  oai_exit = 1;
  sleep(1); //allow lte-softmodem threads to exit first
  exit(1);
}

// Fixme: there are many mistakes in the datamodel and in redondant variables
// TDD is also mode complex
void setAllfromTS(uint64_t TS, L1_rxtx_proc_t *proc) {
  for (int i=0; i < RC.nb_inst; i++) {
    for (int j=0; j<RC.nb_CC[i]; j++) {
      LTE_DL_FRAME_PARMS *fp=&RC.eNB[i][j]->frame_parms;
      uint64_t TStx=TS+(sf_ahead)*fp->samples_per_tti;
      uint64_t TSrach=TS;//-fp->samples_per_tti;
      proc->timestamp_rx=  TS;
      proc->timestamp_tx=  TStx;
      proc->subframe_rx=   (TS    / fp->samples_per_tti)%10;
      proc->subframe_prach=(TSrach    / fp->samples_per_tti)%10;
      proc->subframe_prach_br=(TSrach / fp->samples_per_tti)%10;
      proc->frame_rx=      (TS    / (fp->samples_per_tti*10))&1023;
      proc->frame_prach=   (TSrach    / (fp->samples_per_tti*10))&1023;
      proc->frame_prach_br=(TSrach    / (fp->samples_per_tti*10))&1023;
      proc->frame_tx=      (TStx  / (fp->samples_per_tti*10))&1023;
      proc->subframe_tx=  (TStx  / fp->samples_per_tti)%10;
    }
  }

  return;
}

void init_RU_proc(RU_t *ru) {
  pthread_t t;

  switch(split73) {
    case SPLIT73_CU:
      threadCreate(&t, cu_fs6, (void *)ru, "MainCu", -1, OAI_PRIORITY_RT_MAX);
      break;

    case SPLIT73_DU:
      threadCreate(&t, du_fs6, (void *)ru, "MainDuRx", -1, OAI_PRIORITY_RT_MAX);
      break;

    default:
      threadCreate(&t,  ru_thread, (void *)ru, "MainRu", -1, OAI_PRIORITY_RT_MAX);
  }
}

// Create per UE structures
void init_transport(PHY_VARS_eNB *eNB) {
  LTE_DL_FRAME_PARMS *fp = &eNB->frame_parms;
  LOG_I(PHY, "Initialise transport\n");

  for (int i=0; i<NUMBER_OF_DLSCH_MAX; i++) {
    LOG_D(PHY,"Allocating Transport Channel Buffers for DLSCH, UE %d\n",i);

    for (int j=0; j<2; j++) {
      AssertFatal( (eNB->dlsch[i][j] = new_eNB_dlsch(1,8,NSOFT,fp->N_RB_DL,0,fp)) != NULL,
                   "Can't get eNB dlsch structures for UE %d \n", i);
      eNB->dlsch[i][j]->rnti=0;
      LOG_D(PHY,"dlsch[%d][%d] => %p rnti:%d\n",i,j,eNB->dlsch[i][j], eNB->dlsch[i][j]->rnti);
    }
  }

  for (int i=0; i<NUMBER_OF_ULSCH_MAX; i++) {
    LOG_D(PHY,"Allocating Transport Channel Buffer for ULSCH, UE %d\n",i);
    AssertFatal((eNB->ulsch[1+i] = new_eNB_ulsch(MAX_TURBO_ITERATIONS,fp->N_RB_UL, 0)) != NULL,
                "Can't get eNB ulsch structures\n");
    // this is the transmission mode for the signalling channels
    // this will be overwritten with the real transmission mode by the RRC once the UE is connected
    eNB->transmission_mode[i] = fp->nb_antenna_ports_eNB==1 ? 1 : 2;
  }

  // ULSCH for RA
  AssertFatal( (eNB->ulsch[0] = new_eNB_ulsch(MAX_TURBO_ITERATIONS, fp->N_RB_UL, 0)) !=NULL,
               "Can't get eNB ulsch structures\n");
  eNB->dlsch_SI  = new_eNB_dlsch(1,8,NSOFT,fp->N_RB_DL, 0, fp);
  LOG_D(PHY,"eNB %d.%d : SI %p\n",eNB->Mod_id,eNB->CC_id,eNB->dlsch_SI);
  eNB->dlsch_ra  = new_eNB_dlsch(1,8,NSOFT,fp->N_RB_DL, 0, fp);
  LOG_D(PHY,"eNB %d.%d : RA %p\n",eNB->Mod_id,eNB->CC_id,eNB->dlsch_ra);
  eNB->dlsch_MCH = new_eNB_dlsch(1,8,NSOFT,fp->N_RB_DL, 0, fp);
  LOG_D(PHY,"eNB %d.%d : MCH %p\n",eNB->Mod_id,eNB->CC_id,eNB->dlsch_MCH);
  eNB->rx_total_gain_dB=130;

  for(int i=0; i<NUMBER_OF_UE_MAX; i++)
    eNB->mu_mimo_mode[i].dl_pow_off = 2;

  eNB->check_for_total_transmissions = 0;
  eNB->check_for_MUMIMO_transmissions = 0;
  eNB->FULL_MUMIMO_transmissions = 0;
  eNB->check_for_SUMIMO_transmissions = 0;
  fp->pucch_config_common.deltaPUCCH_Shift = 1;

  if (eNB->use_DTX == 0)
    fill_subframe_mask(eNB);
}

void init_eNB_afterRU(void) {
  for (int inst=0; inst<RC.nb_inst; inst++) {
    for (int CC_id=0; CC_id<RC.nb_CC[inst]; CC_id++) {
      PHY_VARS_eNB *eNB = RC.eNB[inst][CC_id];
      eNB->frame_parms.nb_antennas_rx       = 0;
      eNB->frame_parms.nb_antennas_tx       = 0;
      eNB->prach_vars.rxsigF[0] = (int16_t **)malloc16(64*sizeof(int16_t *));

      for (int ce_level=0; ce_level<4; ce_level++) {
        eNB->prach_vars_br.rxsigF[ce_level] = (int16_t **)malloc16(64*sizeof(int16_t *));
      }

      for (int ru_id=0,aa=0; ru_id<eNB->num_RU; ru_id++) {
        eNB->frame_parms.nb_antennas_rx    += eNB->RU_list[ru_id]->nb_rx;
        eNB->frame_parms.nb_antennas_tx    += eNB->RU_list[ru_id]->nb_tx;
        AssertFatal(eNB->RU_list[ru_id]->common.rxdataF!=NULL,
                    "RU %d : common.rxdataF is NULL\n",
                    eNB->RU_list[ru_id]->idx);
        AssertFatal(eNB->RU_list[ru_id]->prach_rxsigF!=NULL,
                    "RU %d : prach_rxsigF is NULL\n",
                    eNB->RU_list[ru_id]->idx);

        for (int i=0; i<eNB->RU_list[ru_id]->nb_rx; aa++,i++) {
          LOG_I(PHY,"Attaching RU %d antenna %d to eNB antenna %d\n",eNB->RU_list[ru_id]->idx,i,aa);
          eNB->prach_vars.rxsigF[0][aa]    =  eNB->RU_list[ru_id]->prach_rxsigF[0][i];

          for (int ce_level=0; ce_level<4; ce_level++)
            eNB->prach_vars_br.rxsigF[ce_level][aa] = eNB->RU_list[ru_id]->prach_rxsigF_br[ce_level][i];
        }
      }

      AssertFatal( eNB->frame_parms.nb_antennas_rx > 0 && eNB->frame_parms.nb_antennas_rx < 5, "");
      AssertFatal( eNB->frame_parms.nb_antennas_tx > 0 && eNB->frame_parms.nb_antennas_rx < 5, "");
      phy_init_lte_eNB(eNB,0,0);

      // need to copy rxdataF after L1 variables are allocated
      for (int inst=0; inst<RC.nb_inst; inst++) {
        for (int CC_id=0; CC_id<RC.nb_CC[inst]; CC_id++) {
          PHY_VARS_eNB *eNB = RC.eNB[inst][CC_id];

          for (int ru_id=0,aa=0; ru_id<eNB->num_RU; ru_id++) {
            for (int i=0; i<eNB->RU_list[ru_id]->nb_rx; aa++,i++)
              eNB->common_vars.rxdataF[aa]     =  eNB->RU_list[ru_id]->common.rxdataF[i];
          }
        }
      }

      LOG_I(PHY,"inst %d, CC_id %d : nb_antennas_rx %d\n",inst,CC_id,eNB->frame_parms.nb_antennas_rx);
      init_transport(eNB);
      //init_precoding_weights(RC.eNB[inst][CC_id]);
    }
  }
}

void init_eNB(int single_thread_flag,int wait_for_sync) {
  AssertFatal(RC.eNB != NULL,"RC.eNB must have been allocated\n");

  for (int inst=0; inst<RC.nb_L1_inst; inst++) {
    AssertFatal(RC.eNB[inst] != NULL,"RC.eNB[%d] must have been allocated\n", inst);

    for (int CC_id=0; CC_id<RC.nb_L1_CC[inst]; CC_id++) {
      AssertFatal(RC.eNB[inst][CC_id] != NULL,"RC.eNB[%d][%d] must have been allocated\n", inst, CC_id);
      PHY_VARS_eNB *eNB = RC.eNB[inst][CC_id];
      eNB->abstraction_flag   = 0;
      eNB->single_thread_flag = single_thread_flag;
      AssertFatal((eNB->if_inst         = IF_Module_init(inst))!=NULL,"Cannot register interface");
      eNB->if_inst->schedule_response   = schedule_response;
      eNB->if_inst->PHY_config_req      = phy_config_request;
      memset((void *)&eNB->UL_INFO,0,sizeof(eNB->UL_INFO));
      memset((void *)&eNB->Sched_INFO,0,sizeof(eNB->Sched_INFO));
      pthread_mutex_init( &eNB->UL_INFO_mutex, NULL);
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

  SET_LOG_DEBUG(PRACH);
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
  LTE_DL_FRAME_PARMS *fp   = ru->frame_parms;
  openair0_config_t *cfg   = &ru->openair0_cfg;
  //printf("////////////////numerology in config = %d\n",numerology);
  int numerology = 0; //get_softmodem_params()->numerology;

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
  } else
    AssertFatal(1==0,"Unknown N_RB_DL %d\n",fp->N_RB_DL);

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
  frame_parms = ru->frame_parms;
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

#if 0
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
#endif

void ocp_rx_prach(PHY_VARS_eNB *eNB,
                  L1_rxtx_proc_t *proc,
                  RU_t *ru,
                  uint16_t *max_preamble,
                  uint16_t *max_preamble_energy,
                  uint16_t *max_preamble_delay,
                  uint16_t *avg_preamble_energy,
                  uint16_t Nf,
                  uint8_t tdd_mapindex,
                  uint8_t br_flag) {
  int i;
  int prach_mask=0;

  if (br_flag == 0) {
    rx_prach0(eNB,ru,proc->frame_prach, proc->subframe_prach,
              max_preamble,max_preamble_energy,max_preamble_delay,avg_preamble_energy,Nf,tdd_mapindex,0,0);
  } else { // This is procedure for eMTC, basically handling the repetitions
    prach_mask = is_prach_subframe(&eNB->frame_parms,proc->frame_prach_br,proc->subframe_prach_br);

    for (i=0; i<4; i++) {
      if ((eNB->frame_parms.prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[i]==1) &&
          ((prach_mask&(1<<(i+1))) > 0)) { // check that prach CE level is active now

        // if first reception in group of repetitions store frame for later (in RA-RNTI for Msg2)
        if (eNB->prach_vars_br.repetition_number[i]==0)
          eNB->prach_vars_br.first_frame[i]=proc->frame_prach_br;

        // increment repetition number
        eNB->prach_vars_br.repetition_number[i]++;
        // do basic PRACH reception
        rx_prach0(eNB,ru,proc->frame_prach, proc->subframe_prach_br,
                  max_preamble,max_preamble_energy,max_preamble_delay,avg_preamble_energy,Nf,tdd_mapindex,1,i);

        // if last repetition, clear counter
        if (eNB->prach_vars_br.repetition_number[i] == eNB->frame_parms.prach_emtc_config_common.prach_ConfigInfo.prach_numRepetitionPerPreambleAttempt[i]) {
          eNB->prach_vars_br.repetition_number[i]=0;
        }
      }
    } /* for i ... */
  } /* else br_flag == 0 */
}

void prach_procedures_ocp(PHY_VARS_eNB *eNB, L1_rxtx_proc_t *proc, int br_flag) {
  uint16_t max_preamble[4],max_preamble_energy[4],max_preamble_delay[4],avg_preamble_energy[4];
  RU_t *ru;
  int aa=0;
  int ru_aa;

  for (int i=0; i<eNB->num_RU; i++) {
    ru=eNB->RU_list[i];

    for (ru_aa=0,aa=0; ru_aa<ru->nb_rx; ru_aa++,aa++) {
      eNB->prach_vars.rxsigF[0][aa] = eNB->RU_list[i]->prach_rxsigF[0][ru_aa];
      int ce_level;

      if (br_flag==1)
        for (ce_level=0; ce_level<4; ce_level++)
          eNB->prach_vars_br.rxsigF[ce_level][aa] = eNB->RU_list[i]->prach_rxsigF_br[ce_level][ru_aa];
    }
  }

  // run PRACH detection for CE-level 0 only for now when br_flag is set
  ocp_rx_prach(eNB,
               proc,
               eNB->RU_list[0],
               &max_preamble[0],
               &max_preamble_energy[0],
               &max_preamble_delay[0],
               &avg_preamble_energy[0],
               proc->frame_prach,
               0
               ,br_flag
              );
  LOG_D(PHY,"RACH detection index 0: max preamble: %u, energy: %u, delay: %u, avg energy: %u\n",
        max_preamble[0],
        max_preamble_energy[0],
        max_preamble_delay[0],
        avg_preamble_energy[0]
       );

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
        eNB->preamble_list_br[ind].preamble_rel8.rnti = 1 + proc->subframe_prach + (60*(eNB->prach_vars_br.first_frame[ce_level] % 40));
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
          proc->frame_prach,
          proc->subframe_prach,
          max_preamble[0],
          max_preamble_energy[0]/10,
          max_preamble_energy[0]%10,
          max_preamble_delay[0]);
    pthread_mutex_lock(&eNB->UL_INFO_mutex);
    eNB->UL_INFO.rach_ind.rach_indication_body.number_of_preambles  = 1;
    eNB->UL_INFO.rach_ind.rach_indication_body.preamble_list        = &eNB->preamble_list[0];
    eNB->UL_INFO.rach_ind.rach_indication_body.tl.tag               = NFAPI_RACH_INDICATION_BODY_TAG;
    eNB->UL_INFO.rach_ind.header.message_id                         = NFAPI_RACH_INDICATION;
    eNB->UL_INFO.rach_ind.sfn_sf                                    = proc->frame_prach<<4 | proc->subframe_prach;
    eNB->preamble_list[0].preamble_rel8.tl.tag                = NFAPI_PREAMBLE_REL8_TAG;
    eNB->preamble_list[0].preamble_rel8.timing_advance        = max_preamble_delay[0];
    eNB->preamble_list[0].preamble_rel8.preamble              = max_preamble[0];
    eNB->preamble_list[0].preamble_rel8.rnti                  = 1+proc->subframe_prach;  // note: fid is implicitly 0 here
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

void prach_eNB(PHY_VARS_eNB *eNB, L1_rxtx_proc_t *proc, int frame,int subframe) {
  // check if we have to detect PRACH first
  if (is_prach_subframe(&eNB->frame_parms, frame,subframe)>0) {
    prach_procedures_ocp(eNB, proc, 0);
    prach_procedures_ocp(eNB, proc, 1);
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
  prach_eNB(eNB,proc,proc->frame_rx,proc->subframe_rx);
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
  eNB->if_inst->UL_indication(&eNB->UL_INFO, proc);
  pthread_mutex_unlock(&eNB->UL_INFO_mutex);
  phy_procedures_eNB_TX(eNB, proc, 1);
  return(0);
}

void rx_rf(RU_t *ru, L1_rxtx_proc_t *proc) {
  LTE_DL_FRAME_PARMS *fp = ru->frame_parms;
  void *rxp[ru->nb_rx];
  unsigned int rxs;
  int i;
  openair0_timestamp ts=0, timestamp_rx;
  static openair0_timestamp old_ts=0;

  for (i=0; i<ru->nb_rx; i++)
    //receive in the next slot
    rxp[i] = (void *)&ru->common.rxdata[i][((proc->subframe_rx+1)%10)*fp->samples_per_tti];

  rxs = ru->rfdevice.trx_read_func(&ru->rfdevice,
                                   &ts,
                                   rxp,
                                   fp->samples_per_tti,
                                   ru->nb_rx);
  timestamp_rx = ts-ru->ts_offset;

  //  AssertFatal(rxs == fp->samples_per_tti,
  //        "rx_rf: Asked for %d samples, got %d from SDR\n",fp->samples_per_tti,rxs);
  if(rxs != fp->samples_per_tti) {
    LOG_E(PHY,"rx_rf: Asked for %d samples, got %d from SDR\n",fp->samples_per_tti,rxs);
#if defined(USRP_REC_PLAY)
    exit_fun("Exiting IQ record/playback");
#else
    //exit_fun( "problem receiving samples" );
    LOG_E(PHY, "problem receiving samples");
#endif
  }

  if (old_ts != 0 && timestamp_rx - old_ts != fp->samples_per_tti) {
    LOG_E(HW,"impossible shift in rx stream, rx: %ld, previous rx distance: %ld, should be %d\n", timestamp_rx, proc->timestamp_rx - old_ts, fp->samples_per_tti);
    //ru->ts_offset += (proc->timestamp_rx - old_ts - fp->samples_per_tti);
    //proc->timestamp_rx = ts-ru->ts_offset;
  }

  old_ts=timestamp_rx;
  setAllfromTS(timestamp_rx, proc);
}

void ocp_tx_rf(RU_t *ru, L1_rxtx_proc_t *proc) {
  LTE_DL_FRAME_PARMS *fp = ru->frame_parms;
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
    LOG_D(PHY,"[TXPATH] RU %d tx_rf, writing to TS %llu, frame %d, subframe %d\n",ru->idx,
          (long long unsigned int)proc->timestamp_tx,proc->frame_tx,proc->subframe_tx);
  }

  return;
}

static void *ru_thread( void *param ) {
  setbuf(stdout, NULL);
  setbuf(stderr, NULL);
  RU_t *ru = (RU_t *)param;
  L1_rxtx_proc_t L1proc= {0};
  // We pick the global thread pool from the legacy code global vars
  L1proc.threadPool=RC.eNB[0][0]->proc.L1_proc.threadPool;
  L1proc.respEncode=RC.eNB[0][0]->proc.L1_proc.respEncode;
  L1proc.respDecode=RC.eNB[0][0]->proc.L1_proc.respDecode;

  if (ru->if_south == LOCAL_RF) { // configure RF parameters only
    fill_rf_config(ru,ru->rf_config_file);
    init_frame_parms(ru->frame_parms,1);
    phy_init_RU(ru);
    init_rf(ru);
  }

  AssertFatal(setup_RU_buffers(ru)==0, "Exiting, cannot initialize RU Buffers\n");
  LOG_I(PHY, "Signaling main thread that RU %d is ready\n",ru->idx);
  wait_sync("ru_thread");

  // Start RF device if any
  if (ru->rfdevice.trx_start_func(&ru->rfdevice) != 0)
    LOG_E(HW,"Could not start the RF device\n");
  else
    LOG_I(PHY,"RU %d rf device ready\n",ru->idx);

  // This is a forever while loop, it loops over subframes which are scheduled by incoming samples from HW devices
  while (!oai_exit) {
    // synchronization on input FH interface, acquire signals/data and block
    rx_rf(ru, &L1proc);
    // do RX front-end processing (frequency-shift, dft) if needed
    fep_full(ru, L1proc.subframe_rx);

    // At this point, all information for subframe has been received on FH interface
    // If this proc is to provide synchronization, do so
    // Fixme: not used
    // wakeup_slaves(proc);
    for (int i=0; i<ru->num_eNB; i++) {
      char string[20];
      sprintf(string,"Incoming RU %d",ru->idx);

      if (rxtx(ru->eNB_list[i],&L1proc,string) < 0)
        LOG_E(PHY,"eNB %d CC_id %d failed during execution\n",
              ru->eNB_list[i]->Mod_id,ru->eNB_list[i]->CC_id);
    }

    // do TX front-end processing if needed (precoding and/or IDFTs)
    feptx_prec(ru, L1proc.frame_tx, L1proc.subframe_tx);
    // do OFDM if needed
    feptx_ofdm(ru, L1proc.frame_tx, L1proc.subframe_tx);
    // do outgoing fronthaul (south) if needed
    ocp_tx_rf(ru, &L1proc);
  }

  LOG_W(PHY,"Exiting ru_thread \n");
  ru->rfdevice.trx_end_func(&ru->rfdevice);
  LOG_I(PHY,"RU %d rf device stopped\n",ru->idx);
  return NULL;
}

int init_rf(RU_t *ru) {
  char name[256];
  pthread_getname_np(pthread_self(),name, 255);
  pthread_setname_np(pthread_self(),"UHD for OAI");
  int ret=openair0_device_load(&ru->rfdevice,&ru->openair0_cfg);
  pthread_setname_np(pthread_self(),name);
  return ret;
}

void ocp_init_RU(RU_t *ru, char *rf_config_file, int send_dmrssync) {
  PHY_VARS_eNB *eNB0= (PHY_VARS_eNB *)NULL;
  int i;
  int CC_id;
  // read in configuration file)
  LOG_I(PHY,"configuring RU from file\n");
  LOG_I(PHY,"number of L1 instances %d, number of RU %d, number of CPU cores %d\n",
        RC.nb_L1_inst,RC.nb_RU,get_nprocs());

  if (RC.nb_CC != 0)
    for (i=0; i<RC.nb_L1_inst; i++)
      for (CC_id=0; CC_id<RC.nb_CC[i]; CC_id++)
        RC.eNB[i][CC_id]->num_RU=0;

  LOG_D(PHY,"Process RUs RC.nb_RU:%d\n",RC.nb_RU);
  ru->rf_config_file = rf_config_file;
  ru->idx          = 0;
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

  //    ru->generate_dmrs_sync = (ru->is_slave == 0) ? 1 : 0;
  if (ru->generate_dmrs_sync == 1) {
    generate_ul_ref_sigs();
    ru->dmrssync = (int16_t *)malloc16_clear(ru->frame_parms->ofdm_symbol_size*2*sizeof(int16_t));
  }

  ru->wakeup_L1_sleeptime = 2000;
  ru->wakeup_L1_sleep_cnt_max  = 3;

  if (ru->num_eNB > 0) {
    AssertFatal(ru->eNB_list[0], "ru->eNB_list is not initialized\n");
  } else {
    LOG_E(PHY,"Wrong data model, assigning eNB 0, carrier 0 to RU 0\n");
    ru->eNB_list[0] = RC.eNB[0][0];
    ru->num_eNB=1;
  }

  eNB0 = ru->eNB_list[0];
  // datamodel error in regular OAI: a RU uses one single eNB carrier parameters!
  ru->frame_parms = &eNB0->frame_parms;

  for (i=0; i<ru->num_eNB; i++) {
    eNB0 = ru->eNB_list[i];
    int ruIndex=eNB0->num_RU++;
    eNB0->RU_list[ruIndex] = ru;
  }
}

void stop_RU(int nb_ru) {
  for (int inst = 0; inst < nb_ru; inst++) {
    LOG_I(PHY, "Stopping RU %d processing threads\n", inst);
    //kill_RU_proc(RC.ru[inst]);
  }
}

/* --------------------------------------------------------*/
/* from here function to use configuration module          */
static int DEFBFW[] = {0x00007fff};
void ocpRCconfig_RU(RU_t *ru) {
  paramdef_t RUParams[] = RUPARAMS_DESC;
  paramlist_def_t RUParamList = {CONFIG_STRING_RU_LIST,NULL,0};
  config_getlist( &RUParamList,RUParams,sizeof(RUParams)/sizeof(paramdef_t), NULL);

  if ( RUParamList.numelt == 0  ) {
    LOG_W(PHY, "Calling RCconfig_RU while no ru\n");
    RC.nb_RU = 0;
    return;
  } // setting != NULL

  ru->idx = 0;
  ru->if_timing = synch_to_ext_device;
  paramdef_t *vals=RUParamList.paramarray[0];

  if (RC.nb_L1_inst >0)
    ru->num_eNB = vals[RU_ENB_LIST_IDX].numelt;
  else
    ru->num_eNB = 0;

  for (int i=0; i<ru->num_eNB; i++)
    ru->eNB_list[i] = RC.eNB[vals[RU_ENB_LIST_IDX].iptr[i]][0];

  if (config_isparamset(vals, RU_SDR_ADDRS)) {
    ru->openair0_cfg.sdr_addrs = strdup(*(vals[RU_SDR_ADDRS].strptr));
  }

  if (config_isparamset(vals, RU_SDR_CLK_SRC)) {
    char *paramVal=*(vals[RU_SDR_CLK_SRC].strptr);
    LOG_D(PHY, "RU clock source set as %s\n", paramVal);

    if (strcmp(paramVal, "internal") == 0) {
      ru->openair0_cfg.clock_source = internal;
    } else if (strcmp(paramVal, "external") == 0) {
      ru->openair0_cfg.clock_source = external;
    } else if (strcmp(paramVal, "gpsdo") == 0) {
      ru->openair0_cfg.clock_source = gpsdo;
    } else {
      LOG_E(PHY, "Erroneous RU clock source in the provided configuration file: '%s'\n", paramVal);
    }
  }

  if (strcmp(*(vals[RU_LOCAL_RF_IDX].strptr), "yes") == 0) {
    if ( !(config_isparamset(vals,RU_LOCAL_IF_NAME_IDX)) ) {
      ru->if_south  = LOCAL_RF;
      ru->function  = eNodeB_3GPP;
    } else {
      ru->eth_params.local_if_name = strdup(*(vals[RU_LOCAL_IF_NAME_IDX].strptr));
      ru->eth_params.my_addr       = strdup(*(vals[RU_LOCAL_ADDRESS_IDX].strptr));
      ru->eth_params.remote_addr   = strdup(*(vals[RU_REMOTE_ADDRESS_IDX].strptr));
      ru->eth_params.my_portc      = *(vals[RU_LOCAL_PORTC_IDX].uptr);
      ru->eth_params.remote_portc  = *(vals[RU_REMOTE_PORTC_IDX].uptr);
      ru->eth_params.my_portd      = *(vals[RU_LOCAL_PORTD_IDX].uptr);
      ru->eth_params.remote_portd  = *(vals[RU_REMOTE_PORTD_IDX].uptr);
    }

    ru->max_pdschReferenceSignalPower = *(vals[RU_MAX_RS_EPRE_IDX].uptr);;
    ru->max_rxgain                    = *(vals[RU_MAX_RXGAIN_IDX].uptr);
    ru->num_bands                     = vals[RU_BAND_LIST_IDX].numelt;
    /* sf_extension is in unit of samples for 30.72MHz here, has to be scaled later */
    ru->sf_extension                  = *(vals[RU_SF_EXTENSION_IDX].uptr);
    ru->end_of_burst_delay            = *(vals[RU_END_OF_BURST_DELAY_IDX].uptr);

    for (int i=0; i<ru->num_bands; i++)
      ru->band[i] = vals[RU_BAND_LIST_IDX].iptr[i];
  } else {
    ru->eth_params.local_if_name    = strdup(*(vals[RU_LOCAL_IF_NAME_IDX].strptr));
    ru->eth_params.my_addr          = strdup(*(vals[RU_LOCAL_ADDRESS_IDX].strptr));
    ru->eth_params.remote_addr      = strdup(*(vals[RU_REMOTE_ADDRESS_IDX].strptr));
    ru->eth_params.my_portc         = *(vals[RU_LOCAL_PORTC_IDX].uptr);
    ru->eth_params.remote_portc     = *(vals[RU_REMOTE_PORTC_IDX].uptr);
    ru->eth_params.my_portd         = *(vals[RU_LOCAL_PORTD_IDX].uptr);
    ru->eth_params.remote_portd     = *(vals[RU_REMOTE_PORTD_IDX].uptr);
  }  /* strcmp(local_rf, "yes") != 0 */

  ru->nb_tx                             = *(vals[RU_NB_TX_IDX].uptr);
  ru->nb_rx                             = *(vals[RU_NB_RX_IDX].uptr);
  ru->att_tx                            = *(vals[RU_ATT_TX_IDX].uptr);
  ru->att_rx                            = *(vals[RU_ATT_RX_IDX].uptr);
  return;
}


static void get_options(void) {
  CONFIG_SETRTFLAG(CONFIG_NOEXITONHELP);
  get_common_options(SOFTMODEM_ENB_BIT);
  CONFIG_CLEARRTFLAG(CONFIG_NOEXITONHELP);

  if ( !(CONFIG_ISFLAGSET(CONFIG_ABORT)) ) {
    memset((void *)&RC,0,sizeof(RC));
    /* Read RC configuration file */
    RCConfig();
    NB_eNB_INST = RC.nb_inst;
    printf("Configuration: nb_rrc_inst %d, nb_L1_inst %d, nb_ru %d\n",NB_eNB_INST,RC.nb_L1_inst,RC.nb_RU);

    if (!IS_SOFTMODEM_NONBIOT) {
      load_NB_IoT();
      printf("               nb_nbiot_rrc_inst %d, nb_nbiot_L1_inst %d, nb_nbiot_macrlc_inst %d\n",
             RC.nb_nb_iot_rrc_inst, RC.nb_nb_iot_L1_inst, RC.nb_nb_iot_macrlc_inst);
    } else {
      printf("All Nb-IoT instances disabled\n");
      RC.nb_nb_iot_rrc_inst=RC.nb_nb_iot_L1_inst=RC.nb_nb_iot_macrlc_inst=0;
    }
  }
}

void set_default_frame_parms(LTE_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs]) {
  int CC_id;

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    frame_parms[CC_id] = (LTE_DL_FRAME_PARMS *) malloc(sizeof(LTE_DL_FRAME_PARMS));
    /* Set some default values that may be overwritten while reading options */
    frame_parms[CC_id]->frame_type          = FDD;
    frame_parms[CC_id]->tdd_config          = 3;
    frame_parms[CC_id]->tdd_config_S        = 0;
    frame_parms[CC_id]->N_RB_DL             = 100;
    frame_parms[CC_id]->N_RB_UL             = 100;
    frame_parms[CC_id]->Ncp                 = NORMAL;
    frame_parms[CC_id]->Ncp_UL              = NORMAL;
    frame_parms[CC_id]->Nid_cell            = 0;
    frame_parms[CC_id]->num_MBSFN_config    = 0;
    frame_parms[CC_id]->nb_antenna_ports_eNB  = 1;
    frame_parms[CC_id]->nb_antennas_tx      = 1;
    frame_parms[CC_id]->nb_antennas_rx      = 1;
    frame_parms[CC_id]->nushift             = 0;
    frame_parms[CC_id]->phich_config_common.phich_resource = oneSixth;
    frame_parms[CC_id]->phich_config_common.phich_duration = normal;
    // UL RS Config
    frame_parms[CC_id]->pusch_config_common.ul_ReferenceSignalsPUSCH.cyclicShift = 0;//n_DMRS1 set to 0
    frame_parms[CC_id]->pusch_config_common.ul_ReferenceSignalsPUSCH.groupHoppingEnabled = 0;
    frame_parms[CC_id]->pusch_config_common.ul_ReferenceSignalsPUSCH.sequenceHoppingEnabled = 0;
    frame_parms[CC_id]->pusch_config_common.ul_ReferenceSignalsPUSCH.groupAssignmentPUSCH = 0;
    frame_parms[CC_id]->prach_config_common.rootSequenceIndex=22;
    frame_parms[CC_id]->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig=1;
    frame_parms[CC_id]->prach_config_common.prach_ConfigInfo.prach_ConfigIndex=0;
    frame_parms[CC_id]->prach_config_common.prach_ConfigInfo.highSpeedFlag=0;
    frame_parms[CC_id]->prach_config_common.prach_ConfigInfo.prach_FreqOffset=0;
    //    downlink_frequency[CC_id][0] = 2680000000; // Use float to avoid issue with frequency over 2^31.
    //    downlink_frequency[CC_id][1] = downlink_frequency[CC_id][0];
    //    downlink_frequency[CC_id][2] = downlink_frequency[CC_id][0];
    //    downlink_frequency[CC_id][3] = downlink_frequency[CC_id][0];
    //printf("Downlink for CC_id %d frequency set to %u\n", CC_id, downlink_frequency[CC_id][0]);
    frame_parms[CC_id]->dl_CarrierFreq=downlink_frequency[CC_id][0];
  }
}

void init_pdcp(void) {
  if (!NODE_IS_DU(RC.rrc[0]->node_type)) {
    pdcp_layer_init();
    uint32_t pdcp_initmask = (IS_SOFTMODEM_NOS1) ?
                             (PDCP_USE_NETLINK_BIT | LINK_ENB_PDCP_TO_IP_DRIVER_BIT) : LINK_ENB_PDCP_TO_GTPV1U_BIT;

    if (IS_SOFTMODEM_NOS1)
      pdcp_initmask = pdcp_initmask | ENB_NAS_USE_TUN_BIT | SOFTMODEM_NOKRNMOD_BIT  ;

    pdcp_initmask = pdcp_initmask | ENB_NAS_USE_TUN_W_MBMS_BIT;

    if ( split73!=SPLIT73_DU)
      pdcp_module_init(pdcp_initmask, 0);

    if (NODE_IS_CU(RC.rrc[0]->node_type)) {
      //pdcp_set_rlc_data_req_func(proto_agent_send_rlc_data_req);
    } else {
      pdcp_set_rlc_data_req_func(rlc_data_req);
      pdcp_set_pdcp_data_ind_func(pdcp_data_ind);
    }
  } else {
    //pdcp_set_pdcp_data_ind_func(proto_agent_send_pdcp_data_ind);
  }
}

static  void wait_nfapi_init(char *thread_name) {
  LOG_I(ENB_APP, "waiting for NFAPI PNF connection and population of global structure (%s)\n",thread_name);
  pthread_mutex_lock( &nfapi_sync_mutex );

  while (nfapi_sync_var<0)
    pthread_cond_wait( &nfapi_sync_cond, &nfapi_sync_mutex );

  pthread_mutex_unlock(&nfapi_sync_mutex);
  LOG_I(ENB_APP, "NFAPI: got sync (%s)\n", thread_name);
}

void terminate_task(module_id_t mod_id, task_id_t from, task_id_t to) {
  LOG_I(ENB_APP, "sending TERMINATE_MESSAGE from task %s (%d) to task %s (%d)\n",
        itti_get_task_name(from), from, itti_get_task_name(to), to);
  MessageDef *msg;
  msg = itti_alloc_new_message (from, 0, TERMINATE_MESSAGE);
  itti_send_msg_to_task (to, ENB_MODULE_ID_TO_INSTANCE(mod_id), msg);
}

int stop_L1L2(module_id_t enb_id) {
  LOG_W(ENB_APP, "stopping lte-softmodem\n");
  /* these tasks need to pick up new configuration */
  terminate_task(enb_id, TASK_ENB_APP, TASK_RRC_ENB);
  oai_exit = 1;
  LOG_I(ENB_APP, "calling kill_RU_proc() for instance %d\n", enb_id);
  //kill_RU_proc(RC.ru[enb_id]);
  LOG_I(ENB_APP, "calling kill_eNB_proc() for instance %d\n", enb_id);
  kill_eNB_proc(enb_id);
  oai_exit = 0;

  for (int cc_id = 0; cc_id < RC.nb_CC[enb_id]; cc_id++) {
    free_transport(RC.eNB[enb_id][cc_id]);
    phy_free_lte_eNB(RC.eNB[enb_id][cc_id]);
  }

  //phy_free_RU(RC.ru[enb_id]);
  free_lte_top();
  return 0;
}

/*
 * Restart the lte-softmodem after it has been soft-stopped with stop_L1L2()
 */
int restart_L1L2(module_id_t enb_id) {
  RU_t *ru = NULL; //RC.ru[enb_id];
  MessageDef *msg_p = NULL;
  LOG_W(ENB_APP, "restarting lte-softmodem\n");
  /* block threads */
  pthread_mutex_lock(&sync_mutex);
  sync_var = -1;
  pthread_mutex_unlock(&sync_mutex);
  /* copy the changed frame parameters to the RU */
  /* TODO this should be done for all RUs associated to this eNB */
  memcpy(&ru->frame_parms, &RC.eNB[enb_id][0]->frame_parms, sizeof(LTE_DL_FRAME_PARMS));
  /* reset the list of connected UEs in the MAC, since in this process with
   * loose all UEs (have to reconnect) */
  init_UE_info(&RC.mac[enb_id]->UE_info);
  LOG_I(ENB_APP, "attempting to create ITTI tasks\n");
  // No more rrc thread, as many race conditions are hidden behind
  rrc_enb_init();
  itti_mark_task_ready(TASK_RRC_ENB);
  /* pass a reconfiguration request which will configure everything down to
   * RC.eNB[i][j]->frame_parms, too */
  msg_p = itti_alloc_new_message(TASK_ENB_APP, 0, RRC_CONFIGURATION_REQ);
  RRC_CONFIGURATION_REQ(msg_p) = RC.rrc[enb_id]->configuration;
  itti_send_msg_to_task(TASK_RRC_ENB, ENB_MODULE_ID_TO_INSTANCE(enb_id), msg_p);
  init_eNB_afterRU();
  printf("Sending sync to all threads\n");
  pthread_mutex_lock(&sync_mutex);
  sync_var=0;
  pthread_cond_broadcast(&sync_cond);
  pthread_mutex_unlock(&sync_mutex);
  return 0;
}

int main ( int argc, char **argv ) {
  //mtrace();
  int i;
  int CC_id = 0;
  int node_type = ngran_eNB;
  AssertFatal(load_configmodule(argc,argv,0), "[SOFTMODEM] Error, configuration module init failed\n");
  logInit();
  printf("Reading in command-line options\n");
  get_options ();
  AssertFatal(!CONFIG_ISFLAGSET(CONFIG_ABORT),"Getting configuration failed\n");
  EPC_MODE_ENABLED = !IS_SOFTMODEM_NOS1;
#if T_TRACER
  T_Config_Init();
#endif
  set_latency_target();
  set_softmodem_sighandler();
  cpuf=get_cpu_freq_GHz();
  set_taus_seed (0);

  if (opp_enabled ==1)
    reset_opp_meas();

  itti_init(TASK_MAX, tasks_info);
  init_opt();
#ifndef PACKAGE_VERSION
#  define PACKAGE_VERSION "UNKNOWN-EXPERIMENTAL"
#endif
  LOG_I(HW, "Version: %s\n", PACKAGE_VERSION);

  /* Read configuration */
  if (RC.nb_inst > 0) {
    // Allocate memory from RC variable
    read_config_and_init();
  } else {
    printf("RC.nb_inst = 0, Initializing L1\n");
    RCconfig_L1();
  }

  // RU thread and some L1 procedure aren't necessary in VNF or L2 FAPI simulator.
  // but RU thread deals with pre_scd and this is necessary in VNF and simulator.
  // some initialization is necessary and init_ru_vnf do this.
  RU_t ru;

  /* We need to read RU configuration before FlexRAN starts so it knows what
   * splits to report. Actual RU start comes later. */
  if  ( NFAPI_MODE != NFAPI_MODE_VNF) {
    ocpRCconfig_RU(&ru);
    LOG_I(PHY,
          "number of L1 instances %d, number of RU %d, number of CPU cores %d\n",
          RC.nb_L1_inst, RC.nb_RU, get_nprocs());
  }

  if ( strlen(get_softmodem_params()->split73) > 0 ) {
    char tmp[1024]= {0};
    strncpy(tmp,get_softmodem_params()->split73, 1023);
    tmp[2]=0;

    if ( strncasecmp(tmp,"cu", 2)==0 )
      split73=SPLIT73_CU;
    else if ( strncasecmp(tmp,"du", 2)==0 )
      split73=SPLIT73_DU;
    else
      AssertFatal(false,"split73 syntax: <cu|du>:<remote ip addr>[:<ip port>] (string found: %s) \n",get_softmodem_params()->split73);
  }

  if (RC.nb_inst > 0) {
    /* Start the agent. If it is turned off in the configuration, it won't start */
    for (i = 0; i < RC.nb_inst; i++) {
      //flexran_agent_start(i);
    }

    /* initializes PDCP and sets correct RLC Request/PDCP Indication callbacks
     * for monolithic/F1 modes */
    init_pdcp();
    AssertFatal(create_tasks(1)==0,"cannot create ITTI tasks\n");

    for (int enb_id = 0; enb_id < RC.nb_inst; enb_id++) {
      MessageDef *msg_p = itti_alloc_new_message (TASK_ENB_APP, 0, RRC_CONFIGURATION_REQ);
      RRC_CONFIGURATION_REQ(msg_p) = RC.rrc[enb_id]->configuration;
      itti_send_msg_to_task (TASK_RRC_ENB, ENB_MODULE_ID_TO_INSTANCE(enb_id), msg_p);
    }

    node_type = RC.rrc[0]->node_type;
  }

  if (RC.nb_inst > 0 && NODE_IS_CU(node_type)) {
    protocol_ctxt_t ctxt;
    ctxt.module_id = 0 ;
    ctxt.instance = 0;
    ctxt.rnti = 0;
    ctxt.enb_flag = 1;
    ctxt.frame = 0;
    ctxt.subframe = 0;
    pdcp_run(&ctxt);
  }

  /* start threads if only L1 or not a CU */
  if (RC.nb_inst == 0 || !NODE_IS_CU(node_type) || NFAPI_MODE == NFAPI_MODE_PNF || NFAPI_MODE == NFAPI_MODE_VNF) {
    // init UE_PF_PO and mutex lock
    pthread_mutex_init(&ue_pf_po_mutex, NULL);
    memset (&UE_PF_PO[0][0], 0, sizeof(UE_PF_PO_t)*MAX_MOBILES_PER_ENB*MAX_NUM_CCs);
    pthread_cond_init(&sync_cond,NULL);
    pthread_mutex_init(&sync_mutex, NULL);

    if (NFAPI_MODE!=NFAPI_MONOLITHIC) {
      LOG_I(ENB_APP,"NFAPI*** - mutex and cond created - will block shortly for completion of PNF connection\n");
      pthread_cond_init(&sync_cond,NULL);
      pthread_mutex_init(&sync_mutex, NULL);
    }

    if (NFAPI_MODE==NFAPI_MODE_VNF) {// VNF
#if defined(PRE_SCD_THREAD)
      init_ru_vnf();  // ru pointer is necessary for pre_scd.
#endif
      wait_nfapi_init("main?");
    }

    LOG_I(ENB_APP,"START MAIN THREADS\n");
    // start the main threads
    number_of_cards = 1;
    printf("RC.nb_L1_inst:%d\n", RC.nb_L1_inst);

    if (RC.nb_L1_inst > 0) {
      printf("Initializing eNB threads single_thread_flag:%d wait_for_sync:%d\n",
             get_softmodem_params()->single_thread_flag,
             get_softmodem_params()->wait_for_sync);
      init_eNB(get_softmodem_params()->single_thread_flag,
               get_softmodem_params()->wait_for_sync);
    }

    for (int x=0; x < RC.nb_L1_inst; x++)
      for (int CC_id=0; CC_id<RC.nb_L1_CC[x]; CC_id++) {
        L1_rxtx_proc_t *L1proc= &RC.eNB[x][CC_id]->proc.L1_proc;
        L1proc->threadPool=(tpool_t *)malloc(sizeof(tpool_t));
        L1proc->respEncode=(notifiedFIFO_t *) malloc(sizeof(notifiedFIFO_t));
        L1proc->respDecode=(notifiedFIFO_t *) malloc(sizeof(notifiedFIFO_t));

        if ( strlen(get_softmodem_params()->threadPoolConfig) > 0 )
          initTpool(get_softmodem_params()->threadPoolConfig, L1proc->threadPool, true);
        else
          initTpool("n", L1proc->threadPool, true);

        initNotifiedFIFO(L1proc->respEncode);
        initNotifiedFIFO(L1proc->respDecode);
      }
  }

  printf("About to Init RU threads RC.nb_RU:%d\n", RC.nb_RU);

  if (RC.nb_RU >0 && NFAPI_MODE!=NFAPI_MODE_VNF) {
    printf("Initializing RU threads\n");
    ocp_init_RU(&ru,
                get_softmodem_params()->rf_config_file,
                get_softmodem_params()->send_dmrs_sync);
    ru.rf_map.card=0;
    ru.rf_map.chain=CC_id+(get_softmodem_params()->chain_offset);
    init_RU_proc(&ru);
    config_sync_var=0;

    if (NFAPI_MODE==NFAPI_MODE_PNF) { // PNF
      wait_nfapi_init("main?");
    }

    LOG_I(ENB_APP,"RC.nb_RU:%d\n", RC.nb_RU);
    // once all RUs are ready intiailize the rest of the eNBs ((dependence on final RU parameters after configuration)
    printf("ALL RUs ready - init eNBs\n");
    sleep(1);

    if (NFAPI_MODE!=NFAPI_MODE_PNF && NFAPI_MODE!=NFAPI_MODE_VNF) {
      LOG_I(ENB_APP,"Not NFAPI mode - call init_eNB_afterRU()\n");
      init_eNB_afterRU();
    } else {
      LOG_I(ENB_APP,"NFAPI mode - DO NOT call init_eNB_afterRU()\n");
    }

    LOG_UI(ENB_APP,"ALL RUs ready - ALL eNBs ready\n");
    // connect the TX/RX buffers
    sleep(1); /* wait for thread activation */
    LOG_I(ENB_APP,"Sending sync to all threads\n");
    pthread_mutex_lock(&sync_mutex);
    sync_var=0;
    pthread_cond_broadcast(&sync_cond);
    pthread_mutex_unlock(&sync_mutex);
    config_check_unknown_cmdlineopt(CONFIG_CHECKALLSECTIONS);
  }

  create_tasks_mbms(1);
  // wait for end of program
  LOG_UI(ENB_APP,"TYPE <CTRL-C> TO TERMINATE\n");
  // CI -- Flushing the std outputs for the previous marker to show on the eNB / DU / CU log file
  fflush(stdout);
  fflush(stderr);

  // end of CI modifications
  //getchar();
  if(IS_SOFTMODEM_DOSCOPE)
    load_softscope("enb", NULL);

  itti_wait_tasks_end();
  oai_exit=1;
  LOG_I(ENB_APP,"oai_exit=%d\n",oai_exit);
  // stop threads

  if (RC.nb_inst == 0 || !NODE_IS_CU(node_type)) {
    if(IS_SOFTMODEM_DOSCOPE)
      end_forms();

    LOG_I(ENB_APP,"stopping MODEM threads\n");
    stop_eNB(NB_eNB_INST);
    stop_RU(RC.nb_RU);

    /* release memory used by the RU/eNB threads (incomplete), after all
     * threads have been stopped (they partially use the same memory) */
    for (int inst = 0; inst < NB_eNB_INST; inst++) {
      for (int cc_id = 0; cc_id < RC.nb_CC[inst]; cc_id++) {
        free_transport(RC.eNB[inst][cc_id]);
        phy_free_lte_eNB(RC.eNB[inst][cc_id]);
      }
    }

    free_lte_top();
    end_configmodule();
    pthread_cond_destroy(&sync_cond);
    pthread_mutex_destroy(&sync_mutex);
    pthread_cond_destroy(&nfapi_sync_cond);
    pthread_mutex_destroy(&nfapi_sync_mutex);
    pthread_mutex_destroy(&ue_pf_po_mutex);
  }

  terminate_opt();
  logClean();
  printf("Bye.\n");
  return 0;
}
