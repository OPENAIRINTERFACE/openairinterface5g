/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/***********************************************************************
*
* FILENAME    :  sss_nr.h
*
* MODULE      :  Secondary synchronisation signal
*
* DESCRIPTION :  variables related to sss
*
************************************************************************/

#ifndef SSS_NR_H
#define SSS_NR_H

#include "limits.h"
#include "pss_nr.h"

#define NUMBER_SSS_SEQUENCE (336)
#define LENGTH_SSS_NR (127)

#define  GET_NID2_SL(Nid_SL)          (Nid_SL/NUMBER_SSS_SEQUENCE)
#define  GET_NID1_SL(Nid_SL)          (Nid_SL%NUMBER_SSS_SEQUENCE)
#define SSS_METRIC_FLOOR_NR (3) // ratio signal sss sequence power against signal power

typedef struct {
  int nb_antennas_rx;
  int samples_per_slot_wCP;
  int ofdm_symbol_size;
  int first_carrier_offset;
  int ssb_start_subcarrier;
  int subcarrier_spacing;
  const uint16_t *exclude_nid_cells;
  int num_exclude_nid_cells;
} nr_sss_params_t;
sss_detection_result_t rx_sss_nr(nr_sss_params_t *params,
                                 pss_detection_result_t *pss,
                                 int target_Nid_cell,
                                 c16_t rxdataF[NR_N_SYMBOLS_SSB][params->nb_antennas_rx][params->ofdm_symbol_size]);

#endif /* SSS_NR_H */
