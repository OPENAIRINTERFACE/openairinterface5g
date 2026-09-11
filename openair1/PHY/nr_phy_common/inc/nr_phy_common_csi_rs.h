/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#ifndef __NR_PHY_COMMON_CSI_RS__H__
#define __NR_PHY_COMMON_CSI_RS__H__

#include "common/platform_types.h"

// forward declarations
typedef struct NR_DL_FRAME_PARMS_s NR_DL_FRAME_PARMS;

typedef struct {
  int size;
  int ports;
  int kprime;
  int lprime;
  int j[16];
  int koverline[16];
  int loverline[16];
} csi_mapping_parms_t;

csi_mapping_parms_t get_csi_mapping_parms(int row, int b, int l0, int l1);
int get_cdm_group_size(int cdm_type);
void nr_generate_csi_rs(const NR_DL_FRAME_PARMS *frame_parms,
                        const csi_mapping_parms_t *phy_csi_parms,
                        const int16_t amp,
                        const int slot,
                        const uint8_t freq_density,
                        const uint16_t start_rb,
                        const uint16_t nr_of_rbs,
                        const uint8_t symb_l0,
                        const uint8_t symb_l1,
                        const uint8_t row,
                        const uint16_t scramb_id,
                        const uint8_t power_control_offset_ss,
                        const uint8_t cdm_type,
                        c16_t **dataF);

#endif /* __NR_PHY_COMMON_CSI_RS__H__ */
