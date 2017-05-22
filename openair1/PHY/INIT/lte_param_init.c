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

#include <string.h>
#include <math.h>
#include <unistd.h>
#include <execinfo.h>
#include <signal.h>

#include "SIMULATION/TOOLS/defs.h"
#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/extern.h"

extern PHY_VARS_eNB *eNB;
extern PHY_VARS_UE *UE;

void lte_param_init(unsigned char N_tx_port_eNB, 
                    unsigned char N_tx_phy,
		    unsigned char N_rx,
		    unsigned char transmission_mode,
		    uint8_t extended_prefix_flag,
		    frame_t frame_type, 
		    uint16_t Nid_cell,
		    uint8_t tdd_config,
		    uint8_t N_RB_DL,
		    uint8_t threequarter_fs,
                    uint8_t osf,
		    uint32_t perfect_ce)
{

  LTE_DL_FRAME_PARMS *frame_parms;
  int i;

  printf("Start lte_param_init\n");
  eNB = malloc(sizeof(PHY_VARS_eNB));
  UE = malloc(sizeof(PHY_VARS_UE));
  memset((void*)eNB,0,sizeof(PHY_VARS_eNB));
  memset((void*)UE,0,sizeof(PHY_VARS_UE));
  //PHY_config = malloc(sizeof(PHY_CONFIG));
  mac_xface = malloc(sizeof(MAC_xface));

  srand(0);
  randominit(0);
  set_taus_seed(0);

  frame_parms = &(eNB->frame_parms);

  frame_parms->N_RB_DL            = N_RB_DL;   //50 for 10MHz and 25 for 5 MHz
  frame_parms->N_RB_UL            = N_RB_DL;
  frame_parms->threequarter_fs    = threequarter_fs;
  frame_parms->Ncp                = extended_prefix_flag;
  frame_parms->Ncp_UL             = extended_prefix_flag;
  frame_parms->Nid_cell           = Nid_cell;
  frame_parms->nushift            = Nid_cell%6;
  frame_parms->nb_antennas_tx     = N_tx_phy;
  frame_parms->nb_antennas_rx     = N_rx;
  frame_parms->nb_antenna_ports_eNB = N_tx_port_eNB;
  frame_parms->phich_config_common.phich_resource         = oneSixth;
  frame_parms->phich_config_common.phich_duration         = normal;
  frame_parms->tdd_config         = tdd_config;
  frame_parms->frame_type         = frame_type;
  //  frame_parms->Csrs = 2;
  //  frame_parms->Bsrs = 0;
  //  frame_parms->kTC = 0;44
  //  frame_parms->n_RRC = 0;
  frame_parms->mode1_flag = (transmission_mode == 1 || transmission_mode ==7)? 1 : 0;

  init_frame_parms(frame_parms,osf);

  //copy_lte_parms_to_phy_framing(frame_parms, &(PHY_config->PHY_framing));

  //  phy_init_top(frame_parms); //allocation

  UE->is_secondary_ue = 0;
  UE->frame_parms = *frame_parms;
  eNB->frame_parms = *frame_parms;

  eNB->transmission_mode[0] = transmission_mode;
  UE->transmission_mode[0] = transmission_mode;

  phy_init_lte_top(frame_parms);
  dump_frame_parms(frame_parms);

  UE->measurements.n_adj_cells=0;
  UE->measurements.adj_cell_id[0] = Nid_cell+1;
  UE->measurements.adj_cell_id[1] = Nid_cell+2;

  for (i=0; i<3; i++)
    lte_gold(frame_parms,UE->lte_gold_table[i],Nid_cell+i);

  phy_init_lte_ue(UE,1,0);
  phy_init_lte_eNB(eNB,0,0);

  generate_pcfich_reg_mapping(&UE->frame_parms);
  generate_phich_reg_mapping(&UE->frame_parms);

  // DL power control init
  //if (transmission_mode == 1) {
  if (transmission_mode == 1 || transmission_mode ==7) {
    eNB->pdsch_config_dedicated->p_a  = dB0; // 4 = 0dB
    ((eNB->frame_parms).pdsch_config_common).p_b = 0;
    UE->pdsch_config_dedicated->p_a  = dB0; // 4 = 0dB
    ((UE->frame_parms).pdsch_config_common).p_b = 0;
  } else { // rho_a = rhob
    eNB->pdsch_config_dedicated->p_a  = dBm3; // 4 = 0dB
    ((eNB->frame_parms).pdsch_config_common).p_b = 1;
    UE->pdsch_config_dedicated->p_a  = dBm3; // 4 = 0dB
    ((UE->frame_parms).pdsch_config_common).p_b = 1;
  }

  UE->perfect_ce = perfect_ce;

  printf("Done lte_param_init\n");


}
