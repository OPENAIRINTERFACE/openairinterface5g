#include <string.h>
#include <math.h>
#include <unistd.h>
#include <execinfo.h>
#include <signal.h>

#include "SIMULATION/TOOLS/defs.h"
#include "PHY/types.h"
#include "PHY/defs.h"
#include "PHY/extern.h"

extern PHY_VARS_eNB *PHY_vars_eNB;
extern PHY_VARS_UE *PHY_vars_UE;

void lte_param_init(unsigned char N_tx, 
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

  LTE_DL_FRAME_PARMS *lte_frame_parms;
  int i;


  printf("Start lte_param_init\n");
  PHY_vars_eNB = malloc(sizeof(PHY_VARS_eNB));
  PHY_vars_UE = malloc(sizeof(PHY_VARS_UE));
  //PHY_config = malloc(sizeof(PHY_CONFIG));
  mac_xface = malloc(sizeof(MAC_xface));

  srand(0);
  randominit(0);
  set_taus_seed(0);

  lte_frame_parms = &(PHY_vars_eNB->lte_frame_parms);

  lte_frame_parms->N_RB_DL            = N_RB_DL;   //50 for 10MHz and 25 for 5 MHz
  lte_frame_parms->N_RB_UL            = N_RB_DL;
  lte_frame_parms->threequarter_fs    = threequarter_fs;
  lte_frame_parms->Ncp                = extended_prefix_flag;
  lte_frame_parms->Ncp_UL             = extended_prefix_flag;
  lte_frame_parms->Nid_cell           = Nid_cell;
  lte_frame_parms->nushift            = Nid_cell%6;
  lte_frame_parms->nb_antennas_tx     = N_tx;
  lte_frame_parms->nb_antennas_rx     = N_rx;
  lte_frame_parms->nb_antennas_tx_eNB = (transmission_mode == 1)? 1 : 2;
  lte_frame_parms->phich_config_common.phich_resource = oneSixth;
  lte_frame_parms->phich_config_common.phich_duration = normal;

  lte_frame_parms->tdd_config         = tdd_config;
  lte_frame_parms->frame_type         = frame_type;
  //  lte_frame_parms->Csrs = 2;
  //  lte_frame_parms->Bsrs = 0;
  //  lte_frame_parms->kTC = 0;44
  //  lte_frame_parms->n_RRC = 0;
  lte_frame_parms->mode1_flag = (transmission_mode == 1)? 1 : 0;

  init_frame_parms(lte_frame_parms,osf);

  //copy_lte_parms_to_phy_framing(lte_frame_parms, &(PHY_config->PHY_framing));

  //  phy_init_top(lte_frame_parms); //allocation

  PHY_vars_UE->is_secondary_ue = 0;
  PHY_vars_UE->lte_frame_parms = *lte_frame_parms;
  PHY_vars_eNB->lte_frame_parms = *lte_frame_parms;

  phy_init_lte_top(lte_frame_parms);
  dump_frame_parms(lte_frame_parms);

  PHY_vars_UE->PHY_measurements.n_adj_cells=0;
  PHY_vars_UE->PHY_measurements.adj_cell_id[0] = Nid_cell+1;
  PHY_vars_UE->PHY_measurements.adj_cell_id[1] = Nid_cell+2;

  for (i=0; i<3; i++)
    lte_gold(lte_frame_parms,PHY_vars_UE->lte_gold_table[i],Nid_cell+i);

  phy_init_lte_ue(PHY_vars_UE,1,0);
  phy_init_lte_eNB(PHY_vars_eNB,0,0,0);

  generate_pcfich_reg_mapping(&PHY_vars_UE->lte_frame_parms);
  generate_phich_reg_mapping(&PHY_vars_UE->lte_frame_parms);

  // DL power control init
  if (transmission_mode == 1) {
    PHY_vars_eNB->pdsch_config_dedicated->p_a  = dB0; // 4 = 0dB
    ((PHY_vars_eNB->lte_frame_parms).pdsch_config_common).p_b = 0;
    PHY_vars_UE->pdsch_config_dedicated->p_a  = dB0; // 4 = 0dB
    ((PHY_vars_UE->lte_frame_parms).pdsch_config_common).p_b = 0;
  } else { // rho_a = rhob
    PHY_vars_eNB->pdsch_config_dedicated->p_a  = dB0; // 4 = 0dB
    ((PHY_vars_eNB->lte_frame_parms).pdsch_config_common).p_b = 1;
    PHY_vars_UE->pdsch_config_dedicated->p_a  = dB0; // 4 = 0dB
    ((PHY_vars_UE->lte_frame_parms).pdsch_config_common).p_b = 1;
  }

  PHY_vars_UE->perfect_ce = perfect_ce;

  printf("Done lte_param_init\n");


}
