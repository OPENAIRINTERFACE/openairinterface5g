/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef _ORAN_ISOLATE_H_
#define _ORAN_ISOLATE_H_

#include <stdio.h>

#include <pthread.h>
#include <stdint.h>

#include "xran_fh_o_du.h"
#include "oran-init.h"
#include "openair1/PHY/impl_defs_nr.h"
#include "openair1/PHY/TOOLS/tools_defs.h"
#include "openair1/PHY/defs_nr_common.h"
#include "openair1/PHY/NR_TRANSPORT/nr_transport_proto.h"

/*
 * Structure added to bear the information needed from OAI RU
 */
typedef struct ru_info_s {
  // Needed for UL
  int nb_rx;
  int32_t **rxdataF;

  // Needed for Prach
  c16_t (*prach_buf)[NUMBER_OF_NR_RU_PRACH_OCCASIONS_MAX][NR_PRACH_SEQ_LEN_L];
  int nb_prach_rx;
  int start_prach_rx;
} ru_info_t;

void print_fhi_counters(ru_info_t *ru, const int frame, const int slot);

bool is_tdd_dl_guard_slot(const struct xran_frame_config *frame_conf, int slot);
bool is_tdd_ul_guard_slot(const struct xran_frame_config *frame_conf, int slot);

/** @brief Reads RX data PUSCH of next slot.
 *
 * @param ru pointer to structure keeping pointers to OAI data.
 * @param frame output of the frame which has been read.
 * @param slot output of the slot which has been read. */
int xran_fh_rx_read_slot(ru_info_t *ru, int *frame, int *slot);
/** @brief Reads RX data PRACH of next slot.
 *
 * @param ru pointer to structure keeping pointers to OAI data.
 * @param frame output of the frame which has been read.
 * @param slot output of the slot which has been read. */
int xran_fh_rx_prach_read_slot(PHY_VARS_gNB *gNB, ru_info_t *ru, int *frame, int *slot);
/** @brief Writes CP UL data for given slot. */
int xran_send_cp_slot(const int tti, const int slot, const int xran_port, const uint8_t nb_ant, uint16_t **beams, struct xran_buffer_list buf[XRAN_MAX_ANTENNA_NR][XRAN_N_FE_BUF_LEN]);
/** @brief Writes TX data (PDSCH) of given slot. */
int xran_fh_tx_send_slot(const int tti, const int xran_port, const uint8_t nb_ant, const int fft_size, int32_t **txdataF_BF, oran_buf_list_t *bufs);

#endif /* _ORAN_ISOLATE_H_ */
