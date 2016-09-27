#include "PHY/types.h"
#include "PHY/defs.h"

PHY_VARS_eNB* init_lte_eNB(LTE_DL_FRAME_PARMS *frame_parms,
                           uint8_t eNB_id,
                           uint8_t Nid_cell,
                           uint8_t abstraction_flag);

PHY_VARS_UE* init_lte_UE(LTE_DL_FRAME_PARMS *frame_parms,
                         uint8_t UE_id,
                         uint8_t abstraction_flag);

PHY_VARS_RN* init_lte_RN(LTE_DL_FRAME_PARMS *frame_parms,
                         uint8_t RN_id,
                         uint8_t eMBMS_active_state);

void init_lte_vars(LTE_DL_FRAME_PARMS *frame_parms[MAX_NUM_CCs],
                   uint8_t frame_type,
                   uint8_t tdd_config,
                   uint8_t tdd_config_S,
                   uint8_t extended_prefix_flag,
                   uint8_t N_RB_DL,
                   uint16_t Nid_cell,
                   uint8_t cooperation_flag,
                   uint8_t nb_antenna_ports,
                   uint8_t abstraction_flag,
                   int nb_antennas_rx,
                   int nb_antennas_tx,
                   int nb_antennas_rx_ue,
                   uint8_t eMBMS_active_state);
