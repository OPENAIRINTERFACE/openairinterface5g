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
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file PHY/LTE_TRANSPORT/prach_common.c
 * \brief Common routines for UE/eNB PRACH physical channel V8.6 2009-03
 * \author R. Knopp
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */
#include "PHY/sse_intrin.h"
#include "PHY/defs_common.h"
#include "PHY/phy_extern.h"
#include "PHY/phy_extern_ue.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "common/utils/lte/prach_utils.h"

uint16_t NCS_unrestricted[16] = {0,13,15,18,22,26,32,38,46,59,76,93,119,167,279,419};
uint16_t NCS_restricted[15]   = {15,18,22,26,32,38,46,55,68,82,100,128,158,202,237}; // high-speed case
uint16_t NCS_4[7]             = {2,4,6,8,10,12,15};

int16_t ru[2*839]; // quantized roots of unity
uint32_t ZC_inv[839]; // multiplicative inverse for roots u
uint16_t du[838];

extern PRACH_TDD_PREAMBLE_MAP tdd_preamble_map[64][7];

/*
// This is table 5.7.1-4 from 36.211
PRACH_TDD_PREAMBLE_MAP tdd_preamble_map[64][7] = {
  // TDD Configuration Index 0
  { {1,{{0,1,0,2}}},{1,{{0,1,0,1}}}, {1,{{0,1,0,0}}}, {1,{{0,1,0,2}}}, {1,{{0,1,0,1}}}, {1,{{0,1,0,0}}}, {1,{{0,1,0,2}}}},
  // TDD Configuration Index 1
  { {1,{{0,2,0,2}}},{1,{{0,2,0,1}}}, {1,{{0,2,0,0}}}, {1,{{0,2,0,2}}}, {1,{{0,2,0,1}}}, {1,{{0,2,0,0}}}, {1,{{0,2,0,2}}}},
  // TDD Configuration Index 2
  { {1,{{0,1,1,2}}},{1,{{0,1,1,1}}}, {1,{{0,1,1,0}}}, {1,{{0,1,0,1}}}, {1,{{0,1,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,1,1,1}}}},
  // TDD Configuration Index 3
  { {1,{{0,0,0,2}}},{1,{{0,0,0,1}}}, {1,{{0,0,0,0}}}, {1,{{0,0,0,2}}}, {1,{{0,0,0,1}}}, {1,{{0,0,0,0}}}, {1,{{0,0,0,2}}}},
  // TDD Configuration Index 4
  { {1,{{0,0,1,2}}},{1,{{0,0,1,1}}}, {1,{{0,0,1,0}}}, {1,{{0,0,0,1}}}, {1,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,0,1,1}}}},
  // TDD Configuration Index 5
  { {1,{{0,0,0,1}}},{1,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,0,0,1}}}},
  // TDD Configuration Index 6
  { {2,{{0,0,0,2},{0,0,1,2}}}, {2,{{0,0,0,1},{0,0,1,1}}}, {2,{{0,0,0,0},{0,0,1,0}}}, {2,{{0,0,0,1},{0,0,0,2}}}, {2,{{0,0,0,0},{0,0,0,1}}}, {2,{{0,0,0,0},{1,0,0,0}}}, {2,{{0,0,0,2},{0,0,1,1}}}},
  // TDD Configuration Index 7
  { {2,{{0,0,0,1},{0,0,1,1}}}, {2,{{0,0,0,0},{0,0,1,0}}}, {0,{{0,0,0,0},{0,0,0,0}}}, {2,{{0,0,0,0},{0,0,0,2}}}, {0,{{0,0,0,0},{0,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0}}}, {2,{{0,0,0,1},{0,0,1,0}}}},
  // TDD Configuration Index 8
  { {2,{{0,0,0,0},{0,0,1,0}}}, {0,{{0,0,0,0},{0,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0}}}, {2,{{0,0,0,0},{0,0,0,1}}}, {0,{{0,0,0,0},{0,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0}}}, {2,{{0,0,0,0},{0,0,1,1}}}},
  // TDD Configuration Index 9
  { {3,{{0,0,0,1},{0,0,0,2},{0,0,1,2}}}, {3,{{0,0,0,0},{0,0,0,1},{0,0,1,1}}}, {3,{{0,0,0,0},{0,0,1,0},{1,0,0,0}}}, {3,{{0,0,0,0},{0,0,0,1},{0,0,0,2}}}, {3,{{0,0,0,0},{0,0,0,1},{1,0,0,1}}}, {3,{{0,0,0,0},{1,0,0,0},{2,0,0,0}}}, {3,{{0,0,0,1},{0,0,0,2},{0,0,1,1}}}},
  // TDD Configuration Index 10
  { {3,{{0,0,0,0},{0,0,1,0},{0,0,1,1}}}, {3,{{0,0,0,1},{0,0,1,0},{0,0,1,1}}}, {3,{{0,0,0,0},{0,0,1,0},{1,0,1,0}}}, {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0}}}, {3,{{0,0,0,0},{0,0,0,1},{1,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0}}}, {3,{{0,0,0,0},{0,0,0,2},{0,0,1,0}}}},
  // TDD Configuration Index 11
  { {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0}}}, {3,{{0,0,0,0},{0,0,0,1},{0,0,1,0}}}, {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0}}}, {3,{{0,0,0,1},{0,0,1,0},{0,0,1,1}}}},
  // TDD Configuration Index 12
  { {4,{{0,0,0,1},{0,0,0,2},{0,0,1,1},{0,0,1,2}}}, {4,{{0,0,0,0},{0,0,0,1},{0,0,1,0},{0,0,1,1}}},
    {4,{{0,0,0,0},{0,0,1,0},{1,0,0,0},{1,0,1,0}}},
    {4,{{0,0,0,0},{0,0,0,1},{0,0,0,2},{1,0,0,2}}},
    {4,{{0,0,0,0},{0,0,0,1},{1,0,0,0},{1,0,0,1}}},
    {4,{{0,0,0,0},{1,0,0,0},{2,0,0,0},{3,0,0,0}}},
    {4,{{0,0,0,1},{0,0,0,2},{0,0,1,0},{0,0,1,1}}}
  },
  // TDD Configuration Index 13
  { {4,{{0,0,0,0},{0,0,0,2},{0,0,1,0},{0,0,1,2}}}, {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {4,{{0,0,0,0},{0,0,0,1},{0,0,0,2},{1,0,0,1}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {4,{{0,0,0,0},{0,0,0,1},{0,0,0,2},{0,0,1,1}}}
  },
  // TDD Configuration Index 14
  { {4,{{0,0,0,0},{0,0,0,1},{0,0,1,0},{0,0,1,1}}}, {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {4,{{0,0,0,0},{0,0,0,1},{0,0,0,2},{1,0,0,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {4,{{0,0,0,0},{0,0,0,2},{0,0,1,0},{0,0,1,1}}}
  },
  // TDD Configuration Index 15
  { {5,{{0,0,0,0},{0,0,0,1},{0,0,0,2},{0,0,1,1},{0,0,1,2}}}, {5,{{0,0,0,0},{0,0,0,1},{0,0,1,0},{0,0,1,1},{1,0,0,1}}},
    {5,{{0,0,0,0},{0,0,1,0},{1,0,0,0},{1,0,1,0},{2,0,0,0}}}, {5,{{0,0,0,0},{0,0,0,1},{0,0,0,2},{1,0,0,1},{1,0,0,2}}},
    {5,{{0,0,0,0},{0,0,0,1},{1,0,0,0},{1,0,0,1},{2,0,0,1}}}, {5,{{0,0,0,0},{1,0,0,0},{2,0,0,0},{3,0,0,0},{4,0,0,0}}},
    {5,{{0,0,0,0},{0,0,0,1},{0,0,0,2},{0,0,1,0},{0,0,1,1}}}
  },
  // TDD Configuration Index 16
  { {5,{{0,0,0,1},{0,0,0,2},{0,0,1,0},{0,0,1,1},{0,0,1,2}}}, {5,{{0,0,0,0},{0,0,0,1},{0,0,1,0},{0,0,1,1},{1,0,1,1}}},
    {5,{{0,0,0,0},{0,0,1,0},{1,0,0,0},{1,0,1,0},{2,0,1,0}}}, {5,{{0,0,0,0},{0,0,0,1},{0,0,0,2},{1,0,0,0},{1,0,0,2}}},
    {5,{{0,0,0,0},{0,0,0,1},{1,0,0,0},{1,0,0,1},{2,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}}
  },
  // TDD Configuration Index 17
  { {5,{{0,0,0,0},{0,0,0,1},{0,0,0,2},{0,0,1,0},{0,0,1,2}}}, {5,{{0,0,0,0},{0,0,0,1},{0,0,1,0},{0,0,1,1},{1,0,0,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}}, {5,{{0,0,0,0},{0,0,0,1},{0,0,0,2},{1,0,0,0},{1,0,0,1}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}}
  },
  // TDD Configuration Index 18
  { {6,{{0,0,0,0},{0,0,0,1},{0,0,0,2},{0,0,1,0},{0,0,1,1},{0,0,1,2}}},
    {6,{{0,0,0,0},{0,0,0,1},{0,0,1,0},{0,0,1,1},{1,0,0,1},{1,0,1,1}}},
    {6,{{0,0,0,0},{0,0,1,0},{1,0,0,0},{1,0,1,0},{2,0,0,0},{2,0,1,0}}},
    {6,{{0,0,0,0},{0,0,0,1},{0,0,0,2},{1,0,0,0},{1,0,0,1},{1,0,0,2}}},
    {6,{{0,0,0,0},{0,0,0,1},{1,0,0,0},{1,0,0,1},{2,0,0,0},{2,0,0,1}}},
    {6,{{0,0,0,0},{1,0,0,0},{2,0,0,0},{3,0,0,0},{4,0,0,0},{5,0,0,0}}},
    {6,{{0,0,0,0},{0,0,0,1},{0,0,0,2},{0,0,1,0},{0,0,1,1},{1,0,0,2}}}
  },
  // TDD Configuration Index 19
  { {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {6,{{0,0,0,0},{0,0,0,1},{0,0,1,0},{0,0,1,1},{1,0,0,0},{1,0,1,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {6,{{0,0,0,0},{0,0,0,1},{0,0,0,2},{0,0,1,0},{0,0,1,1},{1,0,1,1}}}
  },
  // TDD Configuration Index 20
  { {1,{{0,1,0,1}}},{1,{{0,1,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,1,0,1}}}, {1,{{0,1,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,1,0,1}}}},
  // TDD Configuration Index 21
  { {1,{{0,2,0,1}}},{1,{{0,2,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,2,0,1}}}, {1,{{0,2,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,2,0,1}}}},

  // TDD Configuration Index 22
  { {1,{{0,1,1,1}}},{1,{{0,1,1,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,1,1,0}}}},

  // TDD Configuration Index 23
  { {1,{{0,0,0,1}}},{1,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,0,0,1}}}, {1,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,0,0,1}}}},

  // TDD Configuration Index 24
  { {1,{{0,0,1,1}}},{1,{{0,0,1,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,0,1,0}}}},

  // TDD Configuration Index 25
  { {2,{{0,0,0,1},{0,0,1,1}}}, {2,{{0,0,0,0},{0,0,1,0}}}, {0,{{0,0,0,0},{0,0,0,0}}}, {2,{{0,0,0,1},{1,0,0,1}}}, {2,{{0,0,0,0},{1,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0}}}, {2,{{0,0,0,1},{0,0,1,0}}}},

  // TDD Configuration Index 26
  { {3,{{0,0,0,1},{0,0,1,1},{1,0,0,1}}}, {3,{{0,0,0,0},{0,0,1,0},{1,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0}}}, {3,{{0,0,0,1},{1,0,0,1},{2,0,0,1}}}, {3,{{0,0,0,0},{1,0,0,0},{2,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0}}}, {3,{{0,0,0,1},{0,0,1,0},{1,0,0,1}}}},

  // TDD Configuration Index 27
  { {4,{{0,0,0,1},{0,0,1,1},{1,0,0,1},{1,0,1,1}}}, {4,{{0,0,0,0},{0,0,1,0},{1,0,0,0},{1,0,1,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {4,{{0,0,0,1},{1,0,0,1},{2,0,0,1},{3,0,0,1}}},
    {4,{{0,0,0,0},{1,0,0,0},{2,0,0,0},{3,0,0,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {4,{{0,0,0,1},{0,0,1,0},{1,0,0,1},{1,0,1,0}}}
  },

  // TDD Configuration Index 28
  { {5,{{0,0,0,1},{0,0,1,1},{1,0,0,1},{1,0,1,1},{2,0,0,1}}}, {5,{{0,0,0,0},{0,0,1,0},{1,0,0,0},{1,0,1,0},{2,0,0,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}}, {5,{{0,0,0,1},{1,0,0,1},{2,0,0,1},{3,0,0,1},{4,0,0,1}}},
    {5,{{0,0,0,0},{1,0,0,0},{2,0,0,0},{3,0,0,0},{4,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {5,{{0,0,0,1},{0,0,1,0},{1,0,0,1},{1,0,1,0},{2,0,0,1}}}
  },

  // TDD Configuration Index 29
  { {6,{{0,0,0,1},{0,0,1,1},{1,0,0,1},{1,0,1,1},{2,0,0,1},{2,0,1,1}}},
    {6,{{0,0,0,0},{0,0,1,0},{1,0,0,0},{1,0,1,0},{2,0,0,0},{2,0,1,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {6,{{0,0,0,1},{1,0,0,1},{2,0,0,1},{3,0,0,1},{4,0,0,1},{5,0,0,1}}},
    {6,{{0,0,0,0},{1,0,0,0},{2,0,0,0},{3,0,0,0},{4,0,0,0},{5,0,0,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {6,{{0,0,0,1},{0,0,1,0},{1,0,0,1},{1,0,1,0},{2,0,0,1},{2,0,1,0}}}
  },


  // TDD Configuration Index 30
  { {1,{{0,1,0,1}}},{1,{{0,1,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,1,0,1}}}, {1,{{0,1,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,1,0,1}}}},

  // TDD Configuration Index 31
  { {1,{{0,2,0,1}}},{1,{{0,2,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,2,0,1}}}, {1,{{0,2,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,2,0,1}}}},

  // TDD Configuration Index 32
  { {1,{{0,1,1,1}}},{1,{{0,1,1,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,1,1,0}}}},

  // TDD Configuration Index 33
  { {1,{{0,0,0,1}}},{1,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,0,0,1}}}, {1,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,0,0,1}}}},

  // TDD Configuration Index 34
  { {1,{{0,0,1,1}}},{1,{{0,0,1,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,0,1,0}}}},

  // TDD Configuration Index 35
  { {2,{{0,0,0,1},{0,0,1,1}}}, {2,{{0,0,0,0},{0,0,1,0}}}, {0,{{0,0,0,0},{0,0,0,0}}}, {2,{{0,0,0,1},{1,0,0,1}}}, {2,{{0,0,0,0},{1,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0}}}, {2,{{0,0,0,1},{0,0,1,0}}}},

  // TDD Configuration Index 36
  { {3,{{0,0,0,1},{0,0,1,1},{1,0,0,1}}}, {3,{{0,0,0,0},{0,0,1,0},{1,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0}}}, {3,{{0,0,0,1},{1,0,0,1},{2,0,0,1}}}, {3,{{0,0,0,0},{1,0,0,0},{2,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0}}}, {3,{{0,0,0,1},{0,0,1,0},{1,0,0,1}}}},

  // TDD Configuration Index 37
  { {4,{{0,0,0,1},{0,0,1,1},{1,0,0,1},{1,0,1,1}}}, {4,{{0,0,0,0},{0,0,1,0},{1,0,0,0},{1,0,1,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {4,{{0,0,0,1},{1,0,0,1},{2,0,0,1},{3,0,0,1}}},
    {4,{{0,0,0,0},{1,0,0,0},{2,0,0,0},{3,0,0,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {4,{{0,0,0,1},{0,0,1,0},{1,0,0,1},{1,0,1,0}}}
  },

  // TDD Configuration Index 38
  { {5,{{0,0,0,1},{0,0,1,1},{1,0,0,1},{1,0,1,1},{2,0,0,1}}}, {5,{{0,0,0,0},{0,0,1,0},{1,0,0,0},{1,0,1,0},{2,0,0,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}}, {5,{{0,0,0,1},{1,0,0,1},{2,0,0,1},{3,0,0,1},{4,0,0,1}}},
    {5,{{0,0,0,0},{1,0,0,0},{2,0,0,0},{3,0,0,0},{4,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {5,{{0,0,0,1},{0,0,1,0},{1,0,0,1},{1,0,1,0},{2,0,0,1}}}
  },

  // TDD Configuration Index 39
  { {6,{{0,0,0,1},{0,0,1,1},{1,0,0,1},{1,0,1,1},{2,0,0,1},{2,0,1,1}}},
    {6,{{0,0,0,0},{0,0,1,0},{1,0,0,0},{1,0,1,0},{2,0,0,0},{2,0,1,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {6,{{0,0,0,1},{1,0,0,1},{2,0,0,1},{3,0,0,1},{4,0,0,1},{5,0,0,1}}},
    {6,{{0,0,0,0},{1,0,0,0},{2,0,0,0},{3,0,0,0},{4,0,0,0},{5,0,0,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {6,{{0,0,0,1},{0,0,1,0},{1,0,0,1},{1,0,1,0},{2,0,0,1},{2,0,1,0}}}
  },

  // TDD Configuration Index 40
  { {1,{{0,1,0,0}}},{0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,1,0,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,1,0,0}}}},
  // TDD Configuration Index 41
  { {1,{{0,2,0,0}}},{0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,2,0,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,2,0,0}}}},

  // TDD Configuration Index 42
  { {1,{{0,1,1,0}}},{0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}},

  // TDD Configuration Index 43
  { {1,{{0,0,0,0}}},{0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {1,{{0,0,0,0}}}},

  // TDD Configuration Index 44
  { {1,{{0,0,1,0}}},{0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}, {0,{{0,0,0,0}}}},

  // TDD Configuration Index 45
  { {2,{{0,0,0,0},{0,0,1,0}}}, {0,{{0,0,0,0},{0,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0}}}, {2,{{0,0,0,0},{1,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0}}}, {2,{{0,0,0,0},{1,0,0,0}}}},

  // TDD Configuration Index 46
  { {3,{{0,0,0,0},{0,0,1,0},{1,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0}}}, {3,{{0,0,0,0},{1,0,0,0},{2,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0}}}, {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0}}}, {3,{{0,0,0,0},{1,0,0,0},{2,0,0,0}}}},

  // TDD Configuration Index 47
  { {4,{{0,0,0,0},{0,0,1,0},{1,0,0,0},{1,0,1,0}}}, {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {4,{{0,0,0,0},{1,0,0,0},{2,0,0,0},{3,0,0,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {0,{{0,0,0,0},{0,0,0,0},{0,0,0,0},{0,0,0,0}}},
    {4,{{0,0,0,0},{1,0,0,0},{2,0,0,0},{3,0,0,0}}}
  }
};

*/

uint16_t prach_root_sequence_map0_3[838] = { 129, 710, 140, 699, 120, 719, 210, 629, 168, 671, 84, 755, 105, 734, 93, 746, 70, 769, 60, 779,
                                             2, 837, 1, 838,
                                             56, 783, 112, 727, 148, 691,
                                             80, 759, 42, 797, 40, 799,
                                             35, 804, 73, 766, 146, 693,
                                             31, 808, 28, 811, 30, 809, 27, 812, 29, 810,
                                             24, 815, 48, 791, 68, 771, 74, 765, 178, 661, 136, 703,
                                             86, 753, 78, 761, 43, 796, 39, 800, 20, 819, 21, 818,
                                             95, 744, 202, 637, 190, 649, 181, 658, 137, 702, 125, 714, 151, 688,
                                             217, 622, 128, 711, 142, 697, 122, 717, 203, 636, 118, 721, 110, 729, 89, 750, 103, 736, 61,
                                             778, 55, 784, 15, 824, 14, 825,
                                             12, 827, 23, 816, 34, 805, 37, 802, 46, 793, 207, 632, 179, 660, 145, 694, 130, 709, 223, 616,
                                             228, 611, 227, 612, 132, 707, 133, 706, 143, 696, 135, 704, 161, 678, 201, 638, 173, 666, 106,
                                             733, 83, 756, 91, 748, 66, 773, 53, 786, 10, 829, 9, 830,
                                             7, 832, 8, 831, 16, 823, 47, 792, 64, 775, 57, 782, 104, 735, 101, 738, 108, 731, 208, 631, 184,
                                             655, 197, 642, 191, 648, 121, 718, 141, 698, 149, 690, 216, 623, 218, 621,
                                             152, 687, 144, 695, 134, 705, 138, 701, 199, 640, 162, 677, 176, 663, 119, 720, 158, 681, 164,
                                             675, 174, 665, 171, 668, 170, 669, 87, 752, 169, 670, 88, 751, 107, 732, 81, 758, 82, 757, 100,
                                             739, 98, 741, 71, 768, 59, 780, 65, 774, 50, 789, 49, 790, 26, 813, 17, 822, 13, 826, 6, 833,
                                             5, 834, 33, 806, 51, 788, 75, 764, 99, 740, 96, 743, 97, 742, 166, 673, 172, 667, 175, 664, 187,
                                             652, 163, 676, 185, 654, 200, 639, 114, 725, 189, 650, 115, 724, 194, 645, 195, 644, 192, 647,
                                             182, 657, 157, 682, 156, 683, 211, 628, 154, 685, 123, 716, 139, 700, 212, 627, 153, 686, 213,
                                             626, 215, 624, 150, 689,
                                             225, 614, 224, 615, 221, 618, 220, 619, 127, 712, 147, 692, 124, 715, 193, 646, 205, 634, 206,
                                             633, 116, 723, 160, 679, 186, 653, 167, 672, 79, 760, 85, 754, 77, 762, 92, 747, 58, 781, 62,
                                             777, 69, 770, 54, 785, 36, 803, 32, 807, 25, 814, 18, 821, 11, 828, 4, 835,
                                             3, 836, 19, 820, 22, 817, 41, 798, 38, 801, 44, 795, 52, 787, 45, 794, 63, 776, 67, 772, 72,
                                             767, 76, 763, 94, 745, 102, 737, 90, 749, 109, 730, 165, 674, 111, 728, 209, 630, 204, 635, 117,
                                             722, 188, 651, 159, 680, 198, 641, 113, 726, 183, 656, 180, 659, 177, 662, 196, 643, 155, 684,
                                             214, 625, 126, 713, 131, 708, 219, 620, 222, 617, 226, 613,
                                             230, 609, 232, 607, 262, 577, 252, 587, 418, 421, 416, 423, 413, 426, 411, 428, 376, 463, 395,
                                             444, 283, 556, 285, 554, 379, 460, 390, 449, 363, 476, 384, 455, 388, 451, 386, 453, 361, 478,
                                             387, 452, 360, 479, 310, 529, 354, 485, 328, 511, 315, 524, 337, 502, 349, 490, 335, 504, 324,
                                             515,
                                             323, 516, 320, 519, 334, 505, 359, 480, 295, 544, 385, 454, 292, 547, 291, 548, 381, 458, 399,
                                             440, 380, 459, 397, 442, 369, 470, 377, 462, 410, 429, 407, 432, 281, 558, 414, 425, 247, 592,
                                             277, 562, 271, 568, 272, 567, 264, 575, 259, 580,
                                             237, 602, 239, 600, 244, 595, 243, 596, 275, 564, 278, 561, 250, 589, 246, 593, 417, 422, 248,
                                             591, 394, 445, 393, 446, 370, 469, 365, 474, 300, 539, 299, 540, 364, 475, 362, 477, 298, 541,
                                             312, 527, 313, 526, 314, 525, 353, 486, 352, 487, 343, 496, 327, 512, 350, 489, 326, 513, 319,
                                             520, 332, 507, 333, 506, 348, 491, 347, 492, 322, 517,
                                             330, 509, 338, 501, 341, 498, 340, 499, 342, 497, 301, 538, 366, 473, 401, 438, 371, 468, 408,
                                             431, 375, 464, 249, 590, 269, 570, 238, 601, 234, 605,
                                             257, 582, 273, 566, 255, 584, 254, 585, 245, 594, 251, 588, 412, 427, 372, 467, 282, 557, 403,
                                             436, 396, 443, 392, 447, 391, 448, 382, 457, 389, 450, 294, 545, 297, 542, 311, 528, 344, 495,
                                             345, 494, 318, 521, 331, 508, 325, 514, 321, 518,
                                             346, 493, 339, 500, 351, 488, 306, 533, 289, 550, 400, 439, 378, 461, 374, 465, 415, 424, 270,
                                             569, 241, 598,
                                             231, 608, 260, 579, 268, 571, 276, 563, 409, 430, 398, 441, 290, 549, 304, 535, 308, 531, 358,
                                             481, 316, 523,
                                             293, 546, 288, 551, 284, 555, 368, 471, 253, 586, 256, 583, 263, 576,
                                             242, 597, 274, 565, 402, 437, 383, 456, 357, 482, 329, 510,
                                             317, 522, 307, 532, 286, 553, 287, 552, 266, 573, 261, 578,
                                             236, 603, 303, 536, 356, 483,
                                             355, 484, 405, 434, 404, 435, 406, 433,
                                             235, 604, 267, 572, 302, 537,
                                             309, 530, 265, 574, 233, 606,
                                             367, 472, 296, 543,
                                             336, 503, 305, 534, 373, 466, 280, 559, 279, 560, 419, 420, 240, 599, 258, 581, 229, 610
                                           };

uint16_t prach_root_sequence_map4[138] = {  1,138,2,137,3,136,4,135,5,134,6,133,7,132,8,131,9,130,10,129,
                                            11,128,12,127,13,126,14,125,15,124,16,123,17,122,18,121,19,120,20,119,
                                            21,118,22,117,23,116,24,115,25,114,26,113,27,112,28,111,29,110,30,109,
                                            31,108,32,107,33,106,34,105,35,104,36,103,37,102,38,101,39,100,40,99,
                                            41,98,42,97,43,96,44,95,45,94,46,93,47,92,48,91,49,90,50,89,
                                            51,88,52,87,53,86,54,85,55,84,56,83,57,82,58,81,59,80,60,79,
                                            61,78,62,77,63,76,64,75,65,74,66,73,67,72,68,71,69,70
                                         };

void dump_prach_config(LTE_DL_FRAME_PARMS *frame_parms,uint8_t subframe) {
  FILE *fd;
  fd = fopen("prach_config.txt","w");
  fprintf(fd,"prach_config: subframe          = %d\n",subframe);
  fprintf(fd,"prach_config: N_RB_UL           = %d\n",frame_parms->N_RB_UL);
  fprintf(fd,"prach_config: frame_type        = %s\n",(frame_parms->frame_type==1) ? "TDD":"FDD");

  if(frame_parms->frame_type==1) fprintf(fd,"prach_config: tdd_config        = %d\n",frame_parms->tdd_config);

  fprintf(fd,"prach_config: rootSequenceIndex = %d\n",frame_parms->prach_config_common.rootSequenceIndex);
  fprintf(fd,"prach_config: prach_ConfigIndex = %d\n",frame_parms->prach_config_common.prach_ConfigInfo.prach_ConfigIndex);
  fprintf(fd,"prach_config: Ncs_config        = %d\n",frame_parms->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig);
  fprintf(fd,"prach_config: highSpeedFlag     = %d\n",frame_parms->prach_config_common.prach_ConfigInfo.highSpeedFlag);
  fprintf(fd,"prach_config: n_ra_prboffset    = %d\n",frame_parms->prach_config_common.prach_ConfigInfo.prach_FreqOffset);
  fclose(fd);
}

// This function computes the du
void fill_du(uint8_t prach_fmt) {
  uint16_t iu,u,p;
  uint16_t N_ZC;
  uint16_t *prach_root_sequence_map;

  if (prach_fmt<4) {
    N_ZC = 839;
    prach_root_sequence_map = prach_root_sequence_map0_3;
  } else {
    N_ZC = 139;
    prach_root_sequence_map = prach_root_sequence_map4;
  }

  for (iu=0; iu<(N_ZC-1); iu++) {
    u=prach_root_sequence_map[iu];
    p=1;

    while (((u*p)%N_ZC)!=1)
      p++;

    du[u] = ((p<(N_ZC>>1)) ? p : (N_ZC-p));
  }
}

uint8_t get_num_prach_tdd(module_id_t Mod_id) {
  LTE_DL_FRAME_PARMS *fp = &PHY_vars_UE_g[Mod_id][0]->frame_parms;
  return(tdd_preamble_map[fp->prach_config_common.prach_ConfigInfo.prach_ConfigIndex][fp->tdd_config].num_prach);
}

uint8_t get_fid_prach_tdd(module_id_t Mod_id,uint8_t tdd_map_index) {
  LTE_DL_FRAME_PARMS *fp = &PHY_vars_UE_g[Mod_id][0]->frame_parms;
  return(tdd_preamble_map[fp->prach_config_common.prach_ConfigInfo.prach_ConfigIndex][fp->tdd_config].map[tdd_map_index].f_ra);
}

int is_prach_subframe(LTE_DL_FRAME_PARMS *frame_parms, uint32_t frame, uint8_t subframe) {
  uint8_t prach_ConfigIndex  = frame_parms->prach_config_common.prach_ConfigInfo.prach_ConfigIndex;
  int prach_mask             = is_prach_subframe0(frame_parms->tdd_config,frame_parms->frame_type, prach_ConfigIndex, frame, subframe);

  for (int i=0; i<4; i++) {
    if (frame_parms->prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[i] == 1)
      prach_mask |= (is_prach_subframe0(frame_parms->tdd_config,frame_parms->frame_type, frame_parms->prach_emtc_config_common.prach_ConfigInfo.prach_ConfigIndex[i],
                                        frame, subframe) << (i+1));
  }

  return(prach_mask);
}


void compute_prach_seq(uint16_t rootSequenceIndex,
                       uint8_t prach_ConfigIndex,
                       uint8_t zeroCorrelationZoneConfig,
                       uint8_t highSpeedFlag,
                       lte_frame_type_t frame_type,
                       uint32_t X_u[64][839]) {
  // Compute DFT of x_u => X_u[k] = x_u(inv(u)*k)^* X_u[k] = exp(j\pi u*inv(u)*k*(inv(u)*k+1)/N_ZC)
  unsigned int k,inv_u,i,NCS=0,num_preambles;
  int N_ZC;
  uint8_t prach_fmt = get_prach_fmt(prach_ConfigIndex,frame_type);
  uint16_t *prach_root_sequence_map;
  uint16_t u, preamble_offset;
  uint16_t n_shift_ra,n_shift_ra_bar, d_start,numshift;
  uint8_t not_found;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_UE_COMPUTE_PRACH, VCD_FUNCTION_IN);
#ifdef PRACH_DEBUG
  LOG_I(PHY,"compute_prach_seq: NCS_config %d, prach_fmt %d\n",zeroCorrelationZoneConfig, prach_fmt);
#endif
  AssertFatal(prach_fmt<4,
              "PRACH sequence is only precomputed for prach_fmt<4 (have %"PRIu8")\n", prach_fmt );
  N_ZC = (prach_fmt < 4) ? 839 : 139;
  //init_prach_tables(N_ZC); //moved to phy_init_lte_ue/eNB, since it takes to long in real-time

  if (prach_fmt < 4) {
    prach_root_sequence_map = prach_root_sequence_map0_3;
  } else {
    // FIXME cannot be reached
    prach_root_sequence_map = prach_root_sequence_map4;
  }

#ifdef PRACH_DEBUG
  LOG_I( PHY, "compute_prach_seq: done init prach_tables\n" );
#endif

  if (highSpeedFlag== 0) {
#ifdef PRACH_DEBUG
    LOG_I(PHY,"Low speed prach : NCS_config %d\n",zeroCorrelationZoneConfig);
#endif
    AssertFatal(zeroCorrelationZoneConfig<=15,
                "FATAL, Illegal Ncs_config for unrestricted format %"PRIu8"\n", zeroCorrelationZoneConfig );
    NCS = NCS_unrestricted[zeroCorrelationZoneConfig];
    num_preambles = (NCS==0) ? 64 : ((64*NCS)/N_ZC);

    if (NCS>0) num_preambles++;

    preamble_offset = 0;
  } else {
#ifdef PRACH_DEBUG
    LOG_I( PHY, "high speed prach : NCS_config %"PRIu8"\n", zeroCorrelationZoneConfig );
#endif
    AssertFatal(zeroCorrelationZoneConfig<=14,
                "FATAL, Illegal Ncs_config for restricted format %"PRIu8"\n", zeroCorrelationZoneConfig );
    NCS = NCS_restricted[zeroCorrelationZoneConfig];
    fill_du(prach_fmt);
    num_preambles = 64; // compute ZC sequence for 64 possible roots
    // find first non-zero shift root (stored in preamble_offset)
    not_found = 1;
    preamble_offset = 0;

    while (not_found == 1) {
      // current root depending on rootSequenceIndex
      int index = (rootSequenceIndex + preamble_offset) % N_ZC;

      if (prach_fmt<4) {
        // prach_root_sequence_map points to prach_root_sequence_map0_3
        DevAssert( index < sizeof(prach_root_sequence_map0_3) / sizeof(prach_root_sequence_map0_3[0]) );
      } else {
        // prach_root_sequence_map points to prach_root_sequence_map4
        DevAssert( index < sizeof(prach_root_sequence_map4) / sizeof(prach_root_sequence_map4[0]) );
      }

      u = prach_root_sequence_map[index];
      uint16_t n_group_ra = 0;

      if ( (du[u]<(N_ZC/3)) && (du[u]>=NCS) ) {
        n_shift_ra     = du[u]/NCS;
        d_start        = (du[u]<<1) + (n_shift_ra * NCS);
        n_group_ra     = N_ZC/d_start;
        n_shift_ra_bar = max(0,(N_ZC-(du[u]<<1)-(n_group_ra*d_start))/N_ZC);
      } else if  ( (du[u]>=(N_ZC/3)) && (du[u]<=((N_ZC - NCS)>>1)) ) {
        n_shift_ra     = (N_ZC - (du[u]<<1))/NCS;
        d_start        = N_ZC - (du[u]<<1) + (n_shift_ra * NCS);
        n_group_ra     = du[u]/d_start;
        n_shift_ra_bar = min(n_shift_ra,max(0,(du[u]- (n_group_ra*d_start))/NCS));
      } else {
        n_shift_ra     = 0;
        n_shift_ra_bar = 0;
      }

      // This is the number of cyclic shifts for the current root u
      numshift = (n_shift_ra*n_group_ra) + n_shift_ra_bar;

      // skip to next root and recompute parameters if numshift==0
      if (numshift>0)
        not_found = 0;
      else
        preamble_offset++;
    }
  }

#ifdef PRACH_DEBUG

  if (NCS>0)
    LOG_I( PHY, "Initializing %u preambles for PRACH (NCS_config %"PRIu8", NCS %u, N_ZC/NCS %u)\n",
           num_preambles, zeroCorrelationZoneConfig, NCS, N_ZC/NCS );

#endif

  for (i=0; i<num_preambles; i++) {
    int index = (rootSequenceIndex+i+preamble_offset) % N_ZC;

    if (prach_fmt<4) {
      // prach_root_sequence_map points to prach_root_sequence_map0_3
      DevAssert( index < sizeof(prach_root_sequence_map0_3) / sizeof(prach_root_sequence_map0_3[0]) );
    } else {
      // prach_root_sequence_map points to prach_root_sequence_map4
      DevAssert( index < sizeof(prach_root_sequence_map4) / sizeof(prach_root_sequence_map4[0]) );
    }

    u = prach_root_sequence_map[index];
    inv_u = ZC_inv[u]; // multiplicative inverse of u

    // X_u[0] stores the first ZC sequence where the root u has a non-zero number of shifts
    // for the unrestricted case X_u[0] is the first root indicated by the rootSequenceIndex

    for (k=0; k<N_ZC; k++) {
      // 420 is the multiplicative inverse of 2 (required since ru is exp[j 2\pi n])
      X_u[i][k] = ((uint32_t *)ru)[(((k*(1+(inv_u*k)))%N_ZC)*420)%N_ZC];
    }
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_UE_COMPUTE_PRACH, VCD_FUNCTION_OUT);
}

void init_prach_tables(int N_ZC) {
  int i,m;
  // Compute the modular multiplicative inverse 'iu' of u s.t. iu*u = 1 mod N_ZC
  ZC_inv[0] = 0;
  ZC_inv[1] = 1;

  for (i=2; i<N_ZC; i++) {
    for (m=2; m<N_ZC; m++)
      if (((i*m)%N_ZC) == 1) {
        ZC_inv[i] = m;
        break;
      }

#ifdef PRACH_DEBUG

    if (i<16)
      printf("i %d : inv %d\n",i,ZC_inv[i]);

#endif
  }

  // Compute quantized roots of unity
  for (i=0; i<N_ZC; i++) {
    ru[i<<1]     = (int16_t)(floor(32767.0*cos(2*M_PI*(double)i/N_ZC)));
    ru[1+(i<<1)] = (int16_t)(floor(32767.0*sin(2*M_PI*(double)i/N_ZC)));
#ifdef PRACH_DEBUG

    if (i<16)
      printf("i %d : runity %d,%d\n",i,ru[i<<1],ru[1+(i<<1)]);

#endif
  }
}

