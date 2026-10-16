/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/*
 * \brief MAC Layer variables
 */

#include <stdint.h>
#include "LAYER2/NR_MAC_UE/mac_proto.h"

// table_7_3_1_1_2_2_3_4_5 contains values for number of layers and precoding information for tables 7.3.1.1.2-2/3/4/5 from TS 38.212 subclause 7.3.1.1.2
// the first 6 columns contain table 7.3.1.1.2-2: Precoding information and number of layers, for 4 antenna ports, if transformPrecoder=disabled and maxRank = 2 or 3 or 4
// next six columns contain table 7.3.1.1.2-3: Precoding information and number of layers for 4 antenna ports, if transformPrecoder= enabled, or if transformPrecoder=disabled and maxRank = 1
// next four columns contain table 7.3.1.1.2-4: Precoding information and number of layers, for 2 antenna ports, if transformPrecoder=disabled and maxRank = 2
// next four columns contain table 7.3.1.1.2-5: Precoding information and number of layers, for 2 antenna ports, if transformPrecoder= enabled, or if transformPrecoder= disabled and maxRank = 1
static const uint8_t table_7_3_1_1_2_2_3_4_5[64][20] = {
  {1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0,  1,  0},
  {1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1,  1},
  {1,  2,  1,  2,  1,  2,  1,  2,  1,  2,  1,  2,  2,  0,  2,  0,  1,  2,  0,  0},
  {1,  3,  1,  3,  1,  3,  1,  3,  1,  3,  1,  3,  1,  2,  0,  0,  1,  3,  0,  0},
  {2,  0,  2,  0,  2,  0,  1,  4,  1,  4,  0,  0,  1,  3,  0,  0,  1,  4,  0,  0},
  {2,  1,  2,  1,  2,  1,  1,  5,  1,  5,  0,  0,  1,  4,  0,  0,  1,  5,  0,  0},
  {2,  2,  2,  2,  2,  2,  1,  6,  1,  6,  0,  0,  1,  5,  0,  0,  0,  0,  0,  0},
  {2,  3,  2,  3,  2,  3,  1,  7,  1,  7,  0,  0,  2,  1,  0,  0,  0,  0,  0,  0},
  {2,  4,  2,  4,  2,  4,  1,  8,  1,  8,  0,  0,  2,  2,  0,  0,  0,  0,  0,  0},
  {2,  5,  2,  5,  2,  5,  1,  9,  1,  9,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {3,  0,  3,  0,  3,  0,  1,  10, 1,  10, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {4,  0,  4,  0,  4,  0,  1,  11, 1,  11, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  4,  1,  4,  0,  0,  1,  12, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  5,  1,  5,  0,  0,  1,  13, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  6,  1,  6,  0,  0,  1,  14, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  7,  1,  7,  0,  0,  1,  15, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  8,  1,  8,  0,  0,  1,  16, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  9,  1,  9,  0,  0,  1,  17, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  10, 1,  10, 0,  0,  1,  18, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  11, 1,  11, 0,  0,  1,  19, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  6,  2,  6,  0,  0,  1,  20, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  7,  2,  7,  0,  0,  1,  21, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  8,  2,  8,  0,  0,  1,  22, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  9,  2,  9,  0,  0,  1,  23, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  10, 2,  10, 0,  0,  1,  24, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  11, 2,  11, 0,  0,  1,  25, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  12, 2,  12, 0,  0,  1,  26, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  13, 2,  13, 0,  0,  1,  27, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {3,  1,  3,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {3,  2,  3,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {4,  1,  4,  1,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {4,  2,  4,  2,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  12, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  13, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  14, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  15, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  16, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  17, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  18, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  19, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  20, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  21, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  22, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  23, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  24, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  25, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  26, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {1,  27, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  14, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  15, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  16, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  17, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  18, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  19, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  20, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {2,  21, 0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {3,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {3,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {3,  5,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {3,  6,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {4,  3,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {4,  4,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0},
  {0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0}
};

static const uint8_t table_7_3_2_3_3_1[12][5] = {
  {1,1,0,0,0},
  {1,0,1,0,0},
  {1,1,1,0,0},
  {2,1,0,0,0},
  {2,0,1,0,0},
  {2,0,0,1,0},
  {2,0,0,0,1},
  {2,1,1,0,0},
  {2,0,0,1,1},
  {2,1,1,1,0},
  {2,1,1,1,1},
  {2,1,0,1,0}
};

static const uint8_t table_7_3_2_3_3_2_oneCodeword[31][10] = {
  {1,1,0,0,0,0,0,0,0,1},
  {1,0,1,0,0,0,0,0,0,1},
  {1,1,1,0,0,0,0,0,0,1},
  {2,1,0,0,0,0,0,0,0,1},
  {2,0,1,0,0,0,0,0,0,1},
  {2,0,0,1,0,0,0,0,0,1},
  {2,0,0,0,1,0,0,0,0,1},
  {2,1,1,0,0,0,0,0,0,1},
  {2,0,0,1,1,0,0,0,0,1},
  {2,1,1,1,0,0,0,0,0,1},
  {2,1,1,1,1,0,0,0,0,1},
  {2,1,0,1,0,0,0,0,0,1},
  {2,1,0,0,0,0,0,0,0,2},
  {2,0,1,0,0,0,0,0,0,2},
  {2,0,0,1,0,0,0,0,0,2},
  {2,0,0,0,1,0,0,0,0,2},
  {2,0,0,0,0,1,0,0,0,2},
  {2,0,0,0,0,0,1,0,0,2},
  {2,0,0,0,0,0,0,1,0,2},
  {2,0,0,0,0,0,0,0,1,2},
  {2,1,1,0,0,0,0,0,0,2},
  {2,0,0,1,1,0,0,0,0,2},
  {2,0,0,0,0,1,1,0,0,2},
  {2,0,0,0,0,0,0,1,1,2},
  {2,1,0,0,0,1,0,0,0,2},
  {2,0,0,1,0,0,0,1,0,2},
  {2,1,1,0,0,1,0,0,0,2},
  {2,0,0,1,1,0,0,1,0,2},
  {2,1,1,0,0,1,1,0,0,2},
  {2,0,0,1,1,0,0,1,1,2},
  {2,1,0,1,0,1,0,1,0,2}
};

static const uint8_t table_7_3_2_3_3_2_twoCodeword[4][10] = {
  {2,1,1,1,1,1,0,0,0,2},
  {2,1,1,1,1,1,0,1,0,2},
  {2,1,1,1,1,1,1,1,0,2},
  {2,1,1,1,1,1,1,1,1,2}
};

static const uint8_t table_7_3_2_3_3_3_oneCodeword[24][7] = {
  {1,1,0,0,0,0,0},
  {1,0,1,0,0,0,0},
  {1,1,1,0,0,0,0},
  {2,1,0,0,0,0,0},
  {2,0,1,0,0,0,0},
  {2,0,0,1,0,0,0},
  {2,0,0,0,1,0,0},
  {2,1,1,0,0,0,0},
  {2,0,0,1,1,0,0},
  {2,1,1,1,0,0,0},
  {2,1,1,1,1,0,0},
  {3,1,0,0,0,0,0},
  {3,0,1,0,0,0,0},
  {3,0,0,1,0,0,0},
  {3,0,0,0,1,0,0},
  {3,0,0,0,0,1,0},
  {3,0,0,0,0,0,1},
  {3,1,1,0,0,0,0},
  {3,0,0,1,1,0,0},
  {3,0,0,0,0,1,1},
  {3,1,1,1,0,0,0},
  {3,0,0,0,1,1,1},
  {3,1,1,1,1,0,0},
  {3,1,0,1,0,0,0}
};

static const uint8_t table_7_3_2_3_3_3_twoCodeword[2][7] = {
  {3,1,1,1,1,1,0},
  {3,1,1,1,1,1,1}
};

static const uint8_t table_7_3_2_3_3_4_oneCodeword[58][14] = {
  {1,1,0,0,0,0,0,0,0,0,0,0,0,1},
  {1,0,1,0,0,0,0,0,0,0,0,0,0,1},
  {1,1,1,0,0,0,0,0,0,0,0,0,0,1},
  {2,1,0,0,0,0,0,0,0,0,0,0,0,1},
  {2,0,1,0,0,0,0,0,0,0,0,0,0,1},
  {2,0,0,1,0,0,0,0,0,0,0,0,0,1},
  {2,0,0,0,1,0,0,0,0,0,0,0,0,1},
  {2,1,1,0,0,0,0,0,0,0,0,0,0,1},
  {2,0,0,1,1,0,0,0,0,0,0,0,0,1},
  {2,1,1,1,0,0,0,0,0,0,0,0,0,1},
  {2,1,1,1,1,0,0,0,0,0,0,0,0,1},
  {3,1,0,0,0,0,0,0,0,0,0,0,0,1},
  {3,0,1,0,0,0,0,0,0,0,0,0,0,1},
  {3,0,0,1,0,0,0,0,0,0,0,0,0,1},
  {3,0,0,0,1,0,0,0,0,0,0,0,0,1},
  {3,0,0,0,0,1,0,0,0,0,0,0,0,1},
  {3,0,0,0,0,0,1,0,0,0,0,0,0,1},
  {3,1,1,0,0,0,0,0,0,0,0,0,0,1},
  {3,0,0,1,1,0,0,0,0,0,0,0,0,1},
  {3,0,0,0,0,1,1,0,0,0,0,0,0,1},
  {3,1,1,1,0,0,0,0,0,0,0,0,0,1},
  {3,0,0,0,1,1,1,0,0,0,0,0,0,1},
  {3,1,1,1,1,0,0,0,0,0,0,0,0,1},
  {2,1,0,1,0,0,0,0,0,0,0,0,0,1},
  {3,1,0,0,0,0,0,0,0,0,0,0,0,2},
  {3,0,1,0,0,0,0,0,0,0,0,0,0,2},
  {3,0,0,1,0,0,0,0,0,0,0,0,0,2},
  {3,0,0,0,1,0,0,0,0,0,0,0,0,2},
  {3,0,0,0,0,1,0,0,0,0,0,0,0,2},
  {3,0,0,0,0,0,1,0,0,0,0,0,0,2},
  {3,0,0,0,0,0,0,1,0,0,0,0,0,2},
  {3,0,0,0,0,0,0,0,1,0,0,0,0,2},
  {3,0,0,0,0,0,0,0,0,1,0,0,0,2},
  {3,0,0,0,0,0,0,0,0,0,1,0,0,2},
  {3,0,0,0,0,0,0,0,0,0,0,1,0,2},
  {3,0,0,0,0,0,0,0,0,0,0,0,1,2},
  {3,1,1,0,0,0,0,0,0,0,0,0,0,2},
  {3,0,0,1,1,0,0,0,0,0,0,0,0,2},
  {3,0,0,0,0,1,1,0,0,0,0,0,0,2},
  {3,0,0,0,0,0,0,1,1,0,0,0,0,2},
  {3,0,0,0,0,0,0,0,0,1,1,0,0,2},
  {3,0,0,0,0,0,0,0,0,0,0,1,1,2},
  {3,1,1,0,0,0,0,1,0,0,0,0,0,2},
  {3,0,0,1,1,0,0,0,0,1,0,0,0,2},
  {3,0,0,0,0,1,1,0,0,0,0,1,0,2},
  {3,1,1,0,0,0,0,1,1,0,0,0,0,2},
  {3,0,0,1,1,0,0,0,0,1,1,0,0,2},
  {3,0,0,0,0,1,1,0,0,0,0,1,1,2},
  {1,1,0,0,0,0,0,0,0,0,0,0,0,2},
  {1,0,1,0,0,0,0,0,0,0,0,0,0,2},
  {1,0,0,0,0,0,0,1,0,0,0,0,0,2},
  {1,0,0,0,0,0,0,0,1,0,0,0,0,2},
  {1,1,1,0,0,0,0,0,0,0,0,0,0,2},
  {1,0,0,0,0,0,0,1,1,0,0,0,0,2},
  {2,1,1,0,0,0,0,0,0,0,0,0,0,2},
  {2,0,0,1,1,0,0,0,0,0,0,0,0,2},
  {2,0,0,0,0,0,0,1,1,0,0,0,0,2},
  {2,0,0,0,0,0,0,0,0,1,1,0,0,2}
};

static const uint8_t table_7_3_2_3_3_4_twoCodeword[6][14] = {
  {3,1,1,1,1,1,0,0,0,0,0,0,0,1},
  {3,1,1,1,1,1,1,0,0,0,0,0,0,1},
  {2,1,1,1,1,0,0,1,0,0,0,0,0,2},
  {2,1,1,1,1,0,0,1,0,1,0,0,0,2},
  {2,1,1,1,1,0,0,1,1,1,0,0,0,2},
  {2,1,1,1,1,0,0,1,1,1,1,0,0,2},
};

static inline uint16_t packBits(const uint8_t *toPack, const int nb)
{
  int res = 0;
  for (int i = 0; i < nb; i++)
    res += (*toPack++) << i;
  return res;
}

void set_antenna_port_parameters(fapi_nr_dl_config_dlsch_pdu_rel15_t *dlsch_pdu, int n_cw, long *max_length, long *dmrs, int ant)
{
  if (dmrs == NULL) {
    if (max_length == NULL) {
      // Table 7.3.1.2.2-1: Antenna port(s) (1000 + DMRS port), dmrs-Type=1, maxLength=1
      dlsch_pdu->n_dmrs_cdm_groups = table_7_3_2_3_3_1[ant][0];
      dlsch_pdu->dmrs_ports = packBits(&table_7_3_2_3_3_1[ant][1], 4);
    } else {
      // Table 7.3.1.2.2-2: Antenna port(s) (1000 + DMRS port), dmrs-Type=1, maxLength=2
      if (n_cw == 1) {
        dlsch_pdu->n_dmrs_cdm_groups = table_7_3_2_3_3_2_oneCodeword[ant][0];
        dlsch_pdu->dmrs_ports = packBits(&table_7_3_2_3_3_2_oneCodeword[ant][1], 8);
        dlsch_pdu->n_front_load_symb = table_7_3_2_3_3_2_oneCodeword[ant][9];
      }
      if (n_cw == 2) {
        dlsch_pdu->n_dmrs_cdm_groups = table_7_3_2_3_3_2_twoCodeword[ant][0];
        dlsch_pdu->dmrs_ports = packBits(&table_7_3_2_3_3_2_twoCodeword[ant][1], 8);
        dlsch_pdu->n_front_load_symb = table_7_3_2_3_3_2_twoCodeword[ant][9];
      }
    }
  } else {
    if (max_length == NULL) {
      // Table 7.3.1.2.2-3: Antenna port(s) (1000 + DMRS port), dmrs-Type=2, maxLength=1
      if (n_cw == 1) {
        dlsch_pdu->n_dmrs_cdm_groups = table_7_3_2_3_3_3_oneCodeword[ant][0];
        dlsch_pdu->dmrs_ports = packBits(&table_7_3_2_3_3_3_oneCodeword[ant][1], 6);
      }
      if (n_cw == 2) {
        dlsch_pdu->n_dmrs_cdm_groups = table_7_3_2_3_3_3_twoCodeword[ant][0];
        dlsch_pdu->dmrs_ports = packBits(&table_7_3_2_3_3_3_twoCodeword[ant][1], 6);
      }
    } else {
      // Table 7.3.1.2.2-4: Antenna port(s) (1000 + DMRS port), dmrs-Type=2, maxLength=2
      if (n_cw == 1) {
        dlsch_pdu->n_dmrs_cdm_groups = table_7_3_2_3_3_4_oneCodeword[ant][0];
        dlsch_pdu->dmrs_ports = packBits(&table_7_3_2_3_3_4_oneCodeword[ant][1], 12);
        dlsch_pdu->n_front_load_symb = table_7_3_2_3_3_4_oneCodeword[ant][13];
      }
      if (n_cw == 2) {
        dlsch_pdu->n_dmrs_cdm_groups = table_7_3_2_3_3_4_twoCodeword[ant][0];
        dlsch_pdu->dmrs_ports = packBits(&table_7_3_2_3_3_4_twoCodeword[ant][1], 12);
        dlsch_pdu->n_front_load_symb = table_7_3_2_3_3_4_twoCodeword[ant][13];
      }
    }
  }
}

void ul_ports_config(NR_UE_MAC_INST_t *mac,
                     int *n_front_load_symb,
                     nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu,
                     dci_pdu_rel15_t *dci)
{
  uint8_t rank = pusch_config_pdu->nrOfLayers;
  NR_PUSCH_Config_t *pusch_Config = mac->current_UL_BWP->pusch_Config;
  AssertFatal(pusch_Config != NULL, "pusch_Config shouldn't be null\n");

  long transformPrecoder = pusch_config_pdu->transform_precoding;
  LOG_D(NR_MAC,
        "transformPrecoder %s\n",
        transformPrecoder == NR_PUSCH_Config__transformPrecoder_disabled ? "disabled" : "enabled");

  long *max_length = NULL;
  long *dmrs_type = NULL;
  if (pusch_Config->dmrs_UplinkForPUSCH_MappingTypeA) {
    max_length = pusch_Config->dmrs_UplinkForPUSCH_MappingTypeA->choice.setup->maxLength;
    dmrs_type = pusch_Config->dmrs_UplinkForPUSCH_MappingTypeA->choice.setup->dmrs_Type;
  } else {
    max_length = pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->maxLength;
    dmrs_type = pusch_Config->dmrs_UplinkForPUSCH_MappingTypeB->choice.setup->dmrs_Type;
  }

  int val = dci->antenna_ports.val;
  LOG_D(NR_MAC,
        "MappingType%s max_length %s, dmrs_type %s, antenna_ports %d\n",
        pusch_Config->dmrs_UplinkForPUSCH_MappingTypeA ? "A" : "B",
        max_length ? "len2" : "len1",
        dmrs_type ? "type2" : "type1",
        val);

  int ret = decode_dci_antenna_ports_val(rank,
                                         dmrs_type,
                                         transformPrecoder,
                                         val,
                                         &pusch_config_pdu->num_dmrs_cdm_grps_no_data,
                                         &pusch_config_pdu->dmrs_ports,
                                         n_front_load_symb);
  if (ret < 0) {
    LOG_E(NR_MAC,
          "invalid DCI antenna_ports val %d (rank %d type %d tp %s)\n",
          val,
          rank,
          dmrs_type ? 2 : 1,
          transformPrecoder == NR_PUSCH_Config__transformPrecoder_disabled ? "disabled" : "enabled");
    return;
  }

  LOG_D(NR_MAC,
        "num_dmrs_cdm_grps_no_data %d, dmrs_ports %d\n",
        pusch_config_pdu->num_dmrs_cdm_grps_no_data,
        pusch_config_pdu->dmrs_ports);
}

void set_precoding_information_parameters(nfapi_nr_ue_pusch_pdu_t *pusch_config_pdu,
                                          int n_antenna_port,
                                          long transformPrecoder,
                                          int precoding_information,
                                          NR_PUSCH_Config_t *pusch_Config)
{
  // 1 antenna port and the higher layer parameter txConfig = codebook 0 bits
  if (n_antenna_port == 4) { // 4 antenna port and the higher layer parameter txConfig = codebook
    // Table 7.3.1.1.2-2: transformPrecoder=disabled and maxRank = 2 or 3 or 4
    if ((transformPrecoder == NR_PUSCH_Config__transformPrecoder_disabled)
        && ((*pusch_Config->maxRank == 2) || (*pusch_Config->maxRank == 3) || (*pusch_Config->maxRank == 4))) {
      if (*pusch_Config->codebookSubset == NR_PUSCH_Config__codebookSubset_fullyAndPartialAndNonCoherent) {
        pusch_config_pdu->nrOfLayers = table_7_3_1_1_2_2_3_4_5[precoding_information][0];
        pusch_config_pdu->Tpmi = table_7_3_1_1_2_2_3_4_5[precoding_information][1];
      }
      if (*pusch_Config->codebookSubset == NR_PUSCH_Config__codebookSubset_partialAndNonCoherent){
        pusch_config_pdu->nrOfLayers = table_7_3_1_1_2_2_3_4_5[precoding_information][2];
        pusch_config_pdu->Tpmi = table_7_3_1_1_2_2_3_4_5[precoding_information][3];
      }
      if (*pusch_Config->codebookSubset == NR_PUSCH_Config__codebookSubset_nonCoherent){
        pusch_config_pdu->nrOfLayers = table_7_3_1_1_2_2_3_4_5[precoding_information][4];
        pusch_config_pdu->Tpmi = table_7_3_1_1_2_2_3_4_5[precoding_information][5];
      }
    }
    // Table 7.3.1.1.2-3: transformPrecoder= enabled, or transformPrecoder=disabled and maxRank = 1
    if (((transformPrecoder == NR_PUSCH_Config__transformPrecoder_enabled)
        || (transformPrecoder == NR_PUSCH_Config__transformPrecoder_disabled))
        && (*pusch_Config->maxRank == 1)) {
      if (*pusch_Config->codebookSubset == NR_PUSCH_Config__codebookSubset_fullyAndPartialAndNonCoherent) {
        pusch_config_pdu->nrOfLayers = table_7_3_1_1_2_2_3_4_5[precoding_information][6];
        pusch_config_pdu->Tpmi = table_7_3_1_1_2_2_3_4_5[precoding_information][7];
      }
      if (*pusch_Config->codebookSubset == NR_PUSCH_Config__codebookSubset_partialAndNonCoherent){
        pusch_config_pdu->nrOfLayers = table_7_3_1_1_2_2_3_4_5[precoding_information][8];
        pusch_config_pdu->Tpmi = table_7_3_1_1_2_2_3_4_5[precoding_information][9];
      }
      if (*pusch_Config->codebookSubset == NR_PUSCH_Config__codebookSubset_nonCoherent){
        pusch_config_pdu->nrOfLayers = table_7_3_1_1_2_2_3_4_5[precoding_information][10];
        pusch_config_pdu->Tpmi = table_7_3_1_1_2_2_3_4_5[precoding_information][11];
      }
    }
  }
  if (n_antenna_port == 2) {
    // 2 antenna port and the higher layer parameter txConfig = codebook
    // Table 7.3.1.1.2-4: transformPrecoder=disabled and maxRank = 2
    if ((transformPrecoder == NR_PUSCH_Config__transformPrecoder_disabled) && (*pusch_Config->maxRank == 2)) {
      if (*pusch_Config->codebookSubset == NR_PUSCH_Config__codebookSubset_fullyAndPartialAndNonCoherent) {
        pusch_config_pdu->nrOfLayers = table_7_3_1_1_2_2_3_4_5[precoding_information][12];
        pusch_config_pdu->Tpmi = table_7_3_1_1_2_2_3_4_5[precoding_information][13];
      }
      if (*pusch_Config->codebookSubset == NR_PUSCH_Config__codebookSubset_nonCoherent){
        pusch_config_pdu->nrOfLayers = table_7_3_1_1_2_2_3_4_5[precoding_information][14];
        pusch_config_pdu->Tpmi = table_7_3_1_1_2_2_3_4_5[precoding_information][15];
      }
    }
    // Table 7.3.1.1.2-5: transformPrecoder= enabled, or transformPrecoder= disabled and maxRank = 1
    if (((transformPrecoder == NR_PUSCH_Config__transformPrecoder_enabled)
        || (transformPrecoder == NR_PUSCH_Config__transformPrecoder_disabled))
        && (*pusch_Config->maxRank == 1)) {
      if (*pusch_Config->codebookSubset == NR_PUSCH_Config__codebookSubset_fullyAndPartialAndNonCoherent) {
        pusch_config_pdu->nrOfLayers = table_7_3_1_1_2_2_3_4_5[precoding_information][16];
        pusch_config_pdu->Tpmi = table_7_3_1_1_2_2_3_4_5[precoding_information][17];
      }
      if (*pusch_Config->codebookSubset == NR_PUSCH_Config__codebookSubset_nonCoherent){
        pusch_config_pdu->nrOfLayers = table_7_3_1_1_2_2_3_4_5[precoding_information][18];
        pusch_config_pdu->Tpmi = table_7_3_1_1_2_2_3_4_5[precoding_information][19];
      }
    }
  }
}
