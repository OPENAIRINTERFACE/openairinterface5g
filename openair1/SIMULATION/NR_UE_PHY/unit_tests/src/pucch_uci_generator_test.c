#include "../../unit_tests/src/pss_util_test.h"
#include "PHY/defs_nr_UE.h"
#include "PHY/INIT/init_extern.h"
#include "PHY/phy_extern_nr_ue.h"

/*
#include "SCHED_NR_UE/defs.h"
#include "SCHED_NR/extern.h"

#include "SCHED_NR_UE/harq_nr.h"
*/
#include "SCHED_NR_UE/pucch_uci_ue_nr.h"

/**************** define **************************************/

#define TST_GNB_ID_0                       (0)         /* first index of gNB */
#define TST_THREAD_ID                      (0)


int test_pucch_generators(PHY_VARS_NR_UE *ue) {
  int gNB_id = TST_GNB_ID_0;
  int thread_number = TST_THREAD_ID;
  int TB_identifier = 0;
  int v_return = 0;
  pucch_format_nr_t format = pucch_format2_nr;
  uint8_t  starting_symbol_index;
  uint8_t nb_symbols_total = 4;
  uint16_t starting_prb = 0;;  /* it can be considered as first  hop on case of pucch hopping */
  uint16_t second_hop = 0;     /* second part for pucch for hopping */
  uint8_t  nb_of_prbs = 1;

  switch (format) {
    case pucch_format0_nr:
      nb_symbols_total    = 2;
      nb_of_prbs      = 1;
      starting_symbol_index = 0;
      break;

    case pucch_format1_nr:
      nb_symbols_total    = 5;
      nb_of_prbs      = 1;
      starting_symbol_index = 0;
      break;

    case pucch_format2_nr:
      nb_symbols_total    = 2;
      nb_of_prbs      = 16;
      starting_symbol_index = 0;
      break;
  }

  int m_0 = 0;                 /* format 0 only */
  int m_CS = 0;                /* for all format except for format 0 */
  int index_additional_dmrs = I_PUCCH_NO_ADDITIONAL_DMRS;
  int index_hopping = I_PUCCH_NO_HOPPING;
  int time_domain_occ = 0;
  int occ_length = 0;
  int occ_Index = 0;
  uint64_t  pucch_payload = 0;
  int tx_amp = 512;
  int nr_slot_tx = 0;
  int N_UCI = 0;      /* size in bits for Uplink Control Information */

  switch(format) {
    case pucch_format0_nr: {
      nr_generate_pucch0(ue,ue->common_vars.txdataF,
                         &ue->frame_parms,
                         &ue->pucch_config_dedicated_nr[gNB_id],
                         tx_amp,
                         nr_slot_tx,
                         (uint8_t)m_0,
                         (uint8_t)m_CS,
                         nb_symbols_total,
                         starting_symbol_index,
                         starting_prb);
      break;
    }

    case pucch_format1_nr: {
      nr_generate_pucch1(ue,ue->common_vars.txdataF,
                         &ue->frame_parms,
                         &ue->pucch_config_dedicated_nr[gNB_id],
                         pucch_payload,
                         tx_amp,
                         nr_slot_tx,
                         (uint8_t)m_0,
                         nb_symbols_total,
                         starting_symbol_index,
                         starting_prb,
                         second_hop,
                         (uint8_t)time_domain_occ,
                         (uint8_t)N_UCI);
      break;
    }

    case pucch_format2_nr: {
      nr_generate_pucch2(ue,
                         ue->pdcch_vars[thread_number][gNB_id]->crnti,
                         ue->common_vars.txdataF,
                         &ue->frame_parms,
                         &ue->pucch_config_dedicated_nr[gNB_id],
                         pucch_payload,
                         tx_amp,
                         nr_slot_tx,
                         nb_symbols_total,
                         starting_symbol_index,
                         nb_of_prbs,
                         starting_prb,
                         (uint8_t)N_UCI);
      break;
    }

    case pucch_format3_nr:
    case pucch_format4_nr: {
      nr_generate_pucch3_4(ue,
                           ue->pdcch_vars[thread_number][gNB_id]->crnti,
                           ue->common_vars.txdataF,
                           &ue->frame_parms,
                           format,
                           &ue->pucch_config_dedicated_nr[gNB_id],
                           pucch_payload,
                           tx_amp,
                           nr_slot_tx,
                           nb_symbols_total,
                           starting_symbol_index,
                           nb_of_prbs,
                           starting_prb,
                           second_hop,
                           (uint8_t)N_UCI,
                           (uint8_t)occ_length,
                           (uint8_t)occ_Index);
      break;
    }
  }

  return (v_return);
}


int main(int argc, char *argv[]) {
  uint8_t transmission_mode = 1;
  uint8_t nb_antennas_tx = 1;
  uint8_t nb_antennas_rx = 1;
  uint8_t frame_type = FDD;
  uint8_t N_RB_DL=106;
  lte_prefix_type_t extended_prefix_flag = NORMAL;
  int Nid_cell[] = {(3*1+3)};
  VOID_PARAMETER argc;
  VOID_PARAMETER argv;
  printf(" PUCCH TEST \n");
  printf("-----------\n");

  if (init_test(nb_antennas_tx, nb_antennas_rx, transmission_mode, extended_prefix_flag, frame_type, Nid_cell[0], N_RB_DL) != 0) {
    printf("Initialisation problem for test \n");
    exit(-1);;
  }

  if (test_pucch_generators(PHY_vars_UE) != 0) {
    printf("\nTest PUCCH is fail \n");
  } else {
    printf("\nTest PUCCH is pass \n");
  }

  free_context_synchro_nr();
  return(0);
}
