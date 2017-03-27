/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.0  (the "License"); you may not use this file
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

/*! \file main.c
 * \brief top init of Layer 2
 * \author  Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \version 1.0
 * \email: navid.nikaein@eurecom.fr
 * @ingroup _mac

 */

#ifdef USER_MODE
#include "LAYER2/register.h"
#else
#ifdef KERNEL2_6
//#include <linux/config.h>
#include <linux/slab.h>
#endif

#ifdef KERNEL2_4
#include <linux/malloc.h>
#include <linux/wrapper.h>
#endif


#endif //USER_MODE

#include "defs.h"
#include "proto.h"
#include "extern.h"
#include "assertions.h"
#include "PHY_INTERFACE/extern.h"
#include "PHY_INTERFACE/defs.h"
#include "PHY/defs.h"
#include "SCHED/defs.h"
#include "LAYER2/PDCP_v10.1.0/pdcp.h"
#include "RRC/LITE/defs.h"
#include "UTIL/LOG/log.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#ifdef PHY_EMUL
#include "SIMULATION/simulation_defs.h"
#endif //PHY_EMUL

#include "SCHED/defs.h"
/* TODO: this abstraction_flag here is very hackish - find a proper solution */
extern uint8_t abstraction_flag;
void dl_phy_sync_success(module_id_t   module_idP,
                         frame_t       frameP,
                         unsigned char eNB_index,
                         uint8_t            first_sync)   //init as MR
{
  LOG_D(MAC,"[UE %d] Frame %d: PHY Sync to eNB_index %d successful \n", module_idP, frameP, eNB_index);
#if defined(ENABLE_USE_MME)
  int mme_enabled=1;
#else
  int mme_enabled=0;
#endif

  if (first_sync==1 && !(mme_enabled==1 && abstraction_flag==0)) {
    layer2_init_UE(module_idP);
    openair_rrc_ue_init(module_idP,eNB_index);
  } else
  {
    rrc_in_sync_ind(module_idP,frameP,eNB_index);
  }
}

void mrbch_phy_sync_failure(module_id_t module_idP, frame_t frameP, uint8_t free_eNB_index) //init as CH
{

  LOG_I(MAC,"[eNB %d] Frame %d: PHY Sync failure \n",module_idP,frameP);
  layer2_init_eNB(module_idP, free_eNB_index);
  openair_rrc_eNB_init(module_idP);
}

char layer2_init_eNB(module_id_t module_idP, unsigned char eNB_index)
{

  return 0;

}

char layer2_init_UE(module_id_t module_idP)
{

  return 0;
}

void mac_UE_out_of_sync_ind(module_id_t module_idP, frame_t frameP, uint16_t eNB_index)
{

  //  Mac_rlc_xface->mac_out_of_sync_ind(Mod_id, frameP, eNB_index);
}


int mac_top_init(int eMBMS_active, char *uecap_xer, uint8_t cba_group_active, uint8_t HO_active)
{

  module_id_t    Mod_id,i,j;
  RA_TEMPLATE *RA_template;
  UE_TEMPLATE *UE_template;
  int size_bytes1,size_bytes2,size_bits1,size_bits2;
  int CC_id;
  int list_el;
  UE_list_t *UE_list;

  LOG_I(MAC,"[MAIN] Init function start:Nb_UE_INST=%d\n",NB_UE_INST);

  if (NB_UE_INST>0) {
    UE_mac_inst = (UE_MAC_INST*)malloc16(NB_UE_INST*sizeof(UE_MAC_INST));

    if (UE_mac_inst == NULL) {
      LOG_C(MAC,"[MAIN] Can't ALLOCATE %zu Bytes for %d UE_MAC_INST with size %zu \n",NB_UE_INST*sizeof(UE_MAC_INST),NB_UE_INST,sizeof(UE_MAC_INST));
      mac_xface->macphy_exit("[MAC][MAIN] not enough memory for UEs \n");
    }

    LOG_D(MAC,"[MAIN] ALLOCATE %zu Bytes for %d UE_MAC_INST @ %p\n",NB_UE_INST*sizeof(UE_MAC_INST),NB_UE_INST,UE_mac_inst);

    bzero(UE_mac_inst,NB_UE_INST*sizeof(UE_MAC_INST));

    for(i=0; i<NB_UE_INST; i++) {
      ue_init_mac(i);
    }
  } else {
    UE_mac_inst = NULL;
  }

  LOG_I(MAC,"[MAIN] Init function start:Nb_eNB_INST=%d\n",NB_eNB_INST);

  if (NB_eNB_INST>0) {
    eNB_mac_inst = (eNB_MAC_INST*)malloc16(NB_eNB_INST*sizeof(eNB_MAC_INST));

    if (eNB_mac_inst == NULL) {
      LOG_D(MAC,"[MAIN] can't ALLOCATE %zu Bytes for %d eNB_MAC_INST with size %zu \n",NB_eNB_INST*sizeof(eNB_MAC_INST*),NB_eNB_INST,sizeof(eNB_MAC_INST));
      mac_xface->macphy_exit("[MAC][MAIN] not enough memory for eNB \n");
    } else {
      LOG_D(MAC,"[MAIN] ALLOCATE %zu Bytes for %d eNB_MAC_INST @ %p\n",sizeof(eNB_MAC_INST),NB_eNB_INST,eNB_mac_inst);
      bzero(eNB_mac_inst,NB_eNB_INST*sizeof(eNB_MAC_INST));
    }
  } else {
    eNB_mac_inst = NULL;
  }

  // Initialize Linked-List for Active UEs
  for(Mod_id=0; Mod_id<NB_eNB_INST; Mod_id++) {
    UE_list = &eNB_mac_inst[Mod_id].UE_list;

    UE_list->num_UEs=0;
    UE_list->head=-1;
    UE_list->head_ul=-1;
    UE_list->avail=0;

    for (list_el=0; list_el<NUMBER_OF_UE_MAX-1; list_el++) {
      UE_list->next[list_el]=list_el+1;
      UE_list->next_ul[list_el]=list_el+1;
    }

    UE_list->next[list_el]=-1;
    UE_list->next_ul[list_el]=-1;

#ifdef PHY_EMUL
    Mac_rlc_xface->Is_cluster_head[Mod_id]=2;//0: MR, 1: CH, 2: not CH neither MR
#endif
    /*#if defined(Rel10) || defined(Rel14)
    int n;
    for (n=0;n<4096;n++)
    eNB_mac_inst[Mod_id].MCH_pdu.payload[n] = taus();
    //    Mac_rlc_xface->Node_id[Mod_id]=NODE_ID[Mod_id];
    #endif*/
  }

  //  Mac_rlc_xface->frame=Mac_rlc_xface->frame;


  if (Is_rrc_registered == 1) {
    LOG_I(MAC,"[MAIN] calling RRC\n");
#ifndef CELLULAR //nothing to be done yet for cellular
    openair_rrc_top_init(eMBMS_active, uecap_xer, cba_group_active,HO_active);
#endif
  } else {
    LOG_I(MAC,"[MAIN] Running without an RRC\n");
  }

#ifndef USER_MODE
#ifndef PHY_EMUL
  LOG_I(MAC,"[MAIN] add openair2 proc\n");
  ////  add_openair2_stats();
#endif
#endif

  //init_transport_channels(2);

  // Set up DCIs for TDD 5MHz Config 1..6

  for (i=0; i<NB_eNB_INST; i++)
    for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
      LOG_D(MAC,"[MAIN][eNB %d] CC_id %d initializing RA_template\n",i, CC_id);
      LOG_D(MAC, "[MSC_NEW][FRAME 00000][MAC_eNB][MOD %02d][]\n", i);

      RA_template = (RA_TEMPLATE *)&eNB_mac_inst[i].common_channels[CC_id].RA_template[0];

      for (j=0; j<NB_RA_PROC_MAX; j++) {
        if (mac_xface->frame_parms->frame_type == TDD) {
          switch (mac_xface->frame_parms->N_RB_DL) {
          case 6:
            size_bytes1 = sizeof(DCI1A_1_5MHz_TDD_1_6_t);
            size_bytes2 = sizeof(DCI1A_1_5MHz_TDD_1_6_t);
            size_bits1 = sizeof_DCI1A_1_5MHz_TDD_1_6_t;
            size_bits2 = sizeof_DCI1A_1_5MHz_TDD_1_6_t;
            break;

          case 25:
            size_bytes1 = sizeof(DCI1A_5MHz_TDD_1_6_t);
            size_bytes2 = sizeof(DCI1A_5MHz_TDD_1_6_t);
            size_bits1 = sizeof_DCI1A_5MHz_TDD_1_6_t;
            size_bits2 = sizeof_DCI1A_5MHz_TDD_1_6_t;
            break;

          case 50:
            size_bytes1 = sizeof(DCI1A_10MHz_TDD_1_6_t);
            size_bytes2 = sizeof(DCI1A_10MHz_TDD_1_6_t);
            size_bits1 = sizeof_DCI1A_10MHz_TDD_1_6_t;
            size_bits2 = sizeof_DCI1A_10MHz_TDD_1_6_t;
            break;

          case 100:
            size_bytes1 = sizeof(DCI1A_20MHz_TDD_1_6_t);
            size_bytes2 = sizeof(DCI1A_20MHz_TDD_1_6_t);
            size_bits1 = sizeof_DCI1A_20MHz_TDD_1_6_t;
            size_bits2 = sizeof_DCI1A_20MHz_TDD_1_6_t;
            break;

          default:
            size_bytes1 = sizeof(DCI1A_1_5MHz_TDD_1_6_t);
            size_bytes2 = sizeof(DCI1A_1_5MHz_TDD_1_6_t);
            size_bits1 = sizeof_DCI1A_1_5MHz_TDD_1_6_t;
            size_bits2 = sizeof_DCI1A_1_5MHz_TDD_1_6_t;
            break;
          }

        } else {
          switch (mac_xface->frame_parms->N_RB_DL) {
          case 6:
            size_bytes1 = sizeof(DCI1A_1_5MHz_FDD_t);
            size_bytes2 = sizeof(DCI1A_1_5MHz_FDD_t);
            size_bits1 = sizeof_DCI1A_1_5MHz_FDD_t;
            size_bits2 = sizeof_DCI1A_1_5MHz_FDD_t;
            break;

          case 25:
            size_bytes1 = sizeof(DCI1A_5MHz_FDD_t);
            size_bytes2 = sizeof(DCI1A_5MHz_FDD_t);
            size_bits1 = sizeof_DCI1A_5MHz_FDD_t;
            size_bits2 = sizeof_DCI1A_5MHz_FDD_t;
            break;

          case 50:
            size_bytes1 = sizeof(DCI1A_10MHz_FDD_t);
            size_bytes2 = sizeof(DCI1A_10MHz_FDD_t);
            size_bits1 = sizeof_DCI1A_10MHz_FDD_t;
            size_bits2 = sizeof_DCI1A_10MHz_FDD_t;
            break;

          case 100:
            size_bytes1 = sizeof(DCI1A_20MHz_FDD_t);
            size_bytes2 = sizeof(DCI1A_20MHz_FDD_t);
            size_bits1 = sizeof_DCI1A_20MHz_FDD_t;
            size_bits2 = sizeof_DCI1A_20MHz_FDD_t;
            break;

          default:
            size_bytes1 = sizeof(DCI1A_1_5MHz_FDD_t);
            size_bytes2 = sizeof(DCI1A_1_5MHz_FDD_t);
            size_bits1 = sizeof_DCI1A_1_5MHz_FDD_t;
            size_bits2 = sizeof_DCI1A_1_5MHz_FDD_t;
            break;
          }
        }

        memcpy((void *)&RA_template[j].RA_alloc_pdu1[0],(void *)&RA_alloc_pdu,size_bytes1);
        memcpy((void *)&RA_template[j].RA_alloc_pdu2[0],(void *)&DLSCH_alloc_pdu1A,size_bytes2);
        RA_template[j].RA_dci_size_bytes1 = size_bytes1;
        RA_template[j].RA_dci_size_bytes2 = size_bytes2;
        RA_template[j].RA_dci_size_bits1  = size_bits1;
        RA_template[j].RA_dci_size_bits2  = size_bits2;

        RA_template[j].RA_dci_fmt1        = format1A;
        RA_template[j].RA_dci_fmt2        = format1A;
      }

      memset (&eNB_mac_inst[i].eNB_stats,0,sizeof(eNB_STATS));
      UE_template = (UE_TEMPLATE *)&eNB_mac_inst[i].UE_list.UE_template[CC_id][0];

      for (j=0; j<NUMBER_OF_UE_MAX; j++) {
        UE_template[j].rnti=0;
        // initiallize the eNB to UE statistics
        memset (&eNB_mac_inst[i].UE_list.eNB_UE_stats[CC_id][j],0,sizeof(eNB_UE_STATS));
      }
    }


  //ICIC init param
#ifdef ICIC
  uint8_t SB_size;
  SB_size=mac_xface->get_SB_size(mac_xface->frame_parms->N_RB_DL);

  srand (time(NULL));

  for(j=0; j<NB_eNB_INST; j++)
    for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
      eNB_mac_inst[j][CC_id].sbmap_conf.first_subframe=0;
      eNB_mac_inst[j][CC_id].sbmap_conf.periodicity=10;
      eNB_mac_inst[j][CC_id].sbmap_conf.sb_size=SB_size;
      eNB_mac_inst[j][CC_id].sbmap_conf.nb_active_sb=1;

      for(i=0; i<NUMBER_OF_SUBBANDS; i++) {
        eNB_mac_inst[j][CC_id].sbmap_conf.sbmap[i]=1;
      }

      eNB_mac_inst[j][CC_id].sbmap_conf.sbmap[rand()%NUMBER_OF_SUBBANDS]=0;

    }

#endif
  //end ALU's algo

  LOG_I(MAC,"[MAIN][INIT] Init function finished\n");

  return(0);

}


int mac_init_global_param(void)
{


  Mac_rlc_xface = NULL;
  LOG_I(MAC,"[MAIN] CALLING RLC_MODULE_INIT...\n");

  if (rlc_module_init()!=0) {
    return(-1);
  }

  LOG_I(MAC,"[MAIN] RLC_MODULE_INIT OK, malloc16 for mac_rlc_xface...\n");

  Mac_rlc_xface = (MAC_RLC_XFACE*)malloc16(sizeof(MAC_RLC_XFACE));
  bzero(Mac_rlc_xface,sizeof(MAC_RLC_XFACE));

  if(Mac_rlc_xface == NULL) {
    LOG_E(MAC,"[MAIN] FATAL EROOR: Could not allocate memory for Mac_rlc_xface !!!\n");
    return (-1);

  }

  LOG_I(MAC,"[MAIN] malloc16 OK, mac_rlc_xface @ %p\n",(void *)Mac_rlc_xface);

  //  mac_xface->macphy_data_ind=macphy_data_ind;
  mac_xface->mrbch_phy_sync_failure=mrbch_phy_sync_failure;
  mac_xface->dl_phy_sync_success=dl_phy_sync_success;
  mac_xface->out_of_sync_ind=rrc_out_of_sync_ind;

  //  Mac_rlc_xface->macphy_exit=  mac_xface->macphy_exit;
  //  Mac_rlc_xface->frame = 0;
  //  Mac_rlc_xface->mac_config_req=mac_config_req;
  //  Mac_rlc_xface->mac_meas_req=mac_meas_req;
  //  Mac_rlc_xface->rrc_rlc_config_req=rrc_rlc_config_req;
  //  Mac_rlc_xface->rrc_rlc_data_req=rrc_rlc_data_req;
  //  Mac_rlc_xface->rrc_rlc_register_rrc=rrc_rlc_register_rrc;

  //  Mac_rlc_xface->rrc_mac_config_req=rrc_mac_config_req;

  //  LOG_I(MAC,"[MAIN] INIT_GLOBAL_PARAM: Mac_rlc_xface=%p,rrc_rlc_register_rrc =%p\n",Mac_rlc_xface,Mac_rlc_xface->rrc_rlc_register_rrc);

  //  Mac_rlc_xface->mac_rlc_data_req=mac_rlc_data_req;
  //  Mac_rlc_xface->mac_rlc_data_ind=mac_rlc_data_ind;
  //  Mac_rlc_xface->mac_rlc_status_ind=mac_rlc_status_ind;
  //  Mac_rlc_xface->pdcp_data_req=pdcp_data_req;
  //  Mac_rlc_xface->mrbch_phy_sync_failure=mrbch_phy_sync_failure;
  //  Mac_rlc_xface->dl_phy_sync_success=dl_phy_sync_success;

  LOG_I(MAC,"[MAIN] RLC interface setup and init\n");
  rrc_init_global_param();

#ifdef USER_MODE
  pdcp_layer_init ();
#else
  pdcp_module_init ();
#endif

  LOG_I(MAC,"[MAIN] Init Global Param Done\n");

  return 0;
}


void mac_top_cleanup(void)
{

#ifndef USER_MODE
  pdcp_module_cleanup ();
#endif

  if (NB_UE_INST>0) {
    free (UE_mac_inst);
  }

  if (NB_eNB_INST>0) {
    free(eNB_mac_inst);
  }

  free( Mac_rlc_xface);
}

int l2_init(LTE_DL_FRAME_PARMS *frame_parms,int eMBMS_active, char *uecap_xer,uint8_t cba_group_active, uint8_t HO_active)
{



  LOG_I(MAC,"[MAIN] MAC_INIT_GLOBAL_PARAM IN...\n");
  //    NB_NODE=2;
  //    NB_INST=2;

  Is_rrc_registered=0;
  mac_init_global_param();
  Is_rrc_registered=1;

  mac_xface->macphy_init = mac_top_init;
#ifndef USER_MODE
  mac_xface->macphy_exit = openair_sched_exit;
#else
  mac_xface->macphy_exit=(void (*)(const char*)) exit;
#endif
  LOG_I(MAC,"[MAIN] init eNB MAC functions  \n");
  mac_xface->eNB_dlsch_ulsch_scheduler = eNB_dlsch_ulsch_scheduler;
  mac_xface->get_dci_sdu               = get_dci_sdu;
  mac_xface->fill_rar                  = fill_rar;
  mac_xface->initiate_ra_proc          = initiate_ra_proc;
  mac_xface->cancel_ra_proc            = cancel_ra_proc;
  mac_xface->set_msg3_subframe         = set_msg3_subframe;
  mac_xface->SR_indication             = SR_indication;
  mac_xface->UL_failure_indication     = UL_failure_indication;
  mac_xface->rx_sdu                    = rx_sdu;
  mac_xface->get_dlsch_sdu             = get_dlsch_sdu;
  mac_xface->get_eNB_UE_stats          = get_UE_stats;
  mac_xface->get_transmission_mode     = get_transmission_mode;
  mac_xface->get_rballoc               = get_rballoc;
  mac_xface->get_nb_rb                 = conv_nprb;
  mac_xface->get_prb                   = get_prb;
  //  mac_xface->get_SB_size               = Get_SB_size;
  mac_xface->get_subframe_direction    = get_subframe_direction;
  mac_xface->Msg3_transmitted          = Msg3_tx;
  mac_xface->Msg1_transmitted          = Msg1_tx;
  mac_xface->ra_failed                 = ra_failed;
  mac_xface->ra_succeeded              = ra_succeeded;
  mac_xface->mac_phy_remove_ue         = mac_phy_remove_ue;

  LOG_I(MAC,"[MAIN] init UE MAC functions \n");
  mac_xface->ue_decode_si              = ue_decode_si;
  mac_xface->ue_decode_p               = ue_decode_p;
  mac_xface->ue_send_sdu               = ue_send_sdu;
#if defined(Rel10) || defined(Rel14)
  mac_xface->ue_send_mch_sdu           = ue_send_mch_sdu;
  mac_xface->ue_query_mch              = ue_query_mch;
#endif
  mac_xface->ue_get_SR                 = ue_get_SR;
  mac_xface->ue_get_sdu                = ue_get_sdu;
  mac_xface->ue_get_rach               = ue_get_rach;
  mac_xface->ue_process_rar            = ue_process_rar;
  mac_xface->ue_scheduler              = ue_scheduler;
  mac_xface->process_timing_advance    = process_timing_advance;


  LOG_I(MAC,"[MAIN] PHY Frame configuration \n");
  mac_xface->frame_parms = frame_parms;

  mac_xface->get_ue_active_harq_pid = get_ue_active_harq_pid;
  mac_xface->get_PL                 = get_PL;
  mac_xface->get_RSRP               = get_RSRP;
  mac_xface->get_RSRQ               = get_RSRQ;
  mac_xface->get_RSSI               = get_RSSI;
  mac_xface->get_n_adj_cells        = get_n_adj_cells;
  mac_xface->get_rx_total_gain_dB   = get_rx_total_gain_dB;
  mac_xface->get_Po_NOMINAL_PUSCH   = get_Po_NOMINAL_PUSCH;
  mac_xface->get_num_prach_tdd      = get_num_prach_tdd;
  mac_xface->get_fid_prach_tdd      = get_fid_prach_tdd;
  mac_xface->get_deltaP_rampup      = get_deltaP_rampup;
  mac_xface->computeRIV             = computeRIV;
  mac_xface->get_TBS_DL             = get_TBS_DL;
  mac_xface->get_TBS_UL             = get_TBS_UL;
  mac_xface->get_nCCE_max           = get_nCCE_mac;
  mac_xface->get_nCCE_offset        = get_nCCE_offset;
  mac_xface->get_ue_mode            = get_ue_mode;
  mac_xface->phy_config_sib1_eNB    = phy_config_sib1_eNB;
  mac_xface->phy_config_sib1_ue     = phy_config_sib1_ue;

  mac_xface->phy_config_sib2_eNB        = phy_config_sib2_eNB;
  mac_xface->phy_config_sib2_ue         = phy_config_sib2_ue;
  mac_xface->phy_config_afterHO_ue      = phy_config_afterHO_ue;
#if defined(Rel10) || defined(Rel14)
  mac_xface->phy_config_sib13_eNB        = phy_config_sib13_eNB;
  mac_xface->phy_config_sib13_ue         = phy_config_sib13_ue;
#endif
#ifdef CBA
  mac_xface->phy_config_cba_rnti         = phy_config_cba_rnti ;
#endif
  mac_xface->estimate_ue_tx_power        = estimate_ue_tx_power;
  mac_xface->phy_config_meas_ue          = phy_config_meas_ue;
  mac_xface->phy_reset_ue    = phy_reset_ue;

  mac_xface->phy_config_dedicated_eNB    = phy_config_dedicated_eNB;
  mac_xface->phy_config_dedicated_ue     = phy_config_dedicated_ue;
  mac_xface->phy_config_harq_ue          = phy_config_harq_ue;

  mac_xface->get_lte_frame_parms        = get_lte_frame_parms;
  mac_xface->get_mu_mimo_mode           = get_mu_mimo_mode;

  mac_xface->get_hundred_times_delta_TF = get_hundred_times_delta_IF_mac;
  mac_xface->get_target_pusch_rx_power     = get_target_pusch_rx_power;
  mac_xface->get_target_pucch_rx_power     = get_target_pucch_rx_power;

  mac_xface->get_prach_prb_offset       = get_prach_prb_offset;
  mac_xface->is_prach_subframe          = is_prach_subframe;

#if defined(Rel10) || defined(Rel14)
  mac_xface->get_mch_sdu                 = get_mch_sdu;
  mac_xface->phy_config_dedicated_scell_eNB= phy_config_dedicated_scell_eNB;
  mac_xface->phy_config_dedicated_scell_ue= phy_config_dedicated_scell_ue;

#endif

  mac_xface->get_PHR = get_PHR;
  LOG_D(MAC,"[MAIN] ALL INIT OK\n");

  mac_xface->macphy_init(eMBMS_active,uecap_xer,cba_group_active,HO_active);

  //Mac_rlc_xface->Is_cluster_head[0] = 1;
  //Mac_rlc_xface->Is_cluster_head[1] = 0;


  return(1);
}

