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

/*! \file PHY/LTE_TRANSPORT/prach.c
 * \brief Top-level routines for generating and decoding the PRACH physical channel V8.6 2009-03
 * \author R. Knopp
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */
#include "PHY/sse_intrin.h"
#include "PHY/defs.h"
#include "PHY/extern.h"
//#include "prach.h"
#include "PHY/LTE_TRANSPORT/if4_tools.h"
#include "SCHED/defs.h"
#include "SCHED/extern.h"
#include "UTIL/LOG/vcd_signal_dumper.h"

//#define PRACH_DEBUG 1
//#define PRACH_WRITE_OUTPUT_DEBUG 1

uint16_t NCS_unrestricted[16] = {0,13,15,18,22,26,32,38,46,59,76,93,119,167,279,419};
uint16_t NCS_restricted[15]   = {15,18,22,26,32,38,46,55,68,82,100,128,158,202,237}; // high-speed case
uint16_t NCS_4[7]             = {2,4,6,8,10,12,15};

int16_t ru[2*839]; // quantized roots of unity
uint32_t ZC_inv[839]; // multiplicative inverse for roots u
uint16_t du[838];

typedef struct {
  uint8_t f_ra;
  uint8_t t0_ra;
  uint8_t t1_ra;
  uint8_t t2_ra;
} PRACH_TDD_PREAMBLE_MAP_elem;
typedef struct {
  uint8_t num_prach;
  PRACH_TDD_PREAMBLE_MAP_elem map[6];
} PRACH_TDD_PREAMBLE_MAP;

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

void dump_prach_config(LTE_DL_FRAME_PARMS *frame_parms,uint8_t subframe)
{

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
void fill_du(uint8_t prach_fmt)
{

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

uint8_t get_num_prach_tdd(module_id_t Mod_id)
{
  LTE_DL_FRAME_PARMS *fp = &PHY_vars_UE_g[Mod_id][0]->frame_parms;
  return(tdd_preamble_map[fp->prach_config_common.prach_ConfigInfo.prach_ConfigIndex][fp->tdd_config].num_prach);
}

uint8_t get_fid_prach_tdd(module_id_t Mod_id,uint8_t tdd_map_index)
{
  LTE_DL_FRAME_PARMS *fp = &PHY_vars_UE_g[Mod_id][0]->frame_parms;
  return(tdd_preamble_map[fp->prach_config_common.prach_ConfigInfo.prach_ConfigIndex][fp->tdd_config].map[tdd_map_index].f_ra);
}

uint8_t get_prach_fmt(uint8_t prach_ConfigIndex,lte_frame_type_t frame_type)
{

  if (frame_type == FDD) // FDD
    return(prach_ConfigIndex>>4);

  else {
    if (prach_ConfigIndex < 20)
      return (0);

    if (prach_ConfigIndex < 30)
      return (1);

    if (prach_ConfigIndex < 40)
      return (2);

    if (prach_ConfigIndex < 48)
      return (3);
    else
      return (4);
  }
}

uint8_t get_prach_prb_offset(LTE_DL_FRAME_PARMS *frame_parms, 
			     uint8_t prach_ConfigIndex, 
			     uint8_t n_ra_prboffset,
			     uint8_t tdd_mapindex, uint16_t Nf) 
{
  lte_frame_type_t frame_type         = frame_parms->frame_type;
  uint8_t tdd_config         = frame_parms->tdd_config;

  uint8_t n_ra_prb;
  uint8_t f_ra,t1_ra;
  uint8_t prach_fmt = get_prach_fmt(prach_ConfigIndex,frame_type);
  uint8_t Nsp=2;

  if (frame_type == TDD) { // TDD

    if (tdd_preamble_map[prach_ConfigIndex][tdd_config].num_prach==0) {
      LOG_E(PHY, "Illegal prach_ConfigIndex %"PRIu8"", prach_ConfigIndex);
      return(-1);
    }

    // adjust n_ra_prboffset for frequency multiplexing (p.36 36.211)
    f_ra = tdd_preamble_map[prach_ConfigIndex][tdd_config].map[tdd_mapindex].f_ra;

    if (prach_fmt < 4) {
      if ((f_ra&1) == 0) {
        n_ra_prb = n_ra_prboffset + 6*(f_ra>>1);
      } else {
        n_ra_prb = frame_parms->N_RB_UL - 6 - n_ra_prboffset + 6*(f_ra>>1);
      }
    } else {
      if ((tdd_config >2) && (tdd_config<6))
        Nsp = 2;

      t1_ra = tdd_preamble_map[prach_ConfigIndex][tdd_config].map[0].t1_ra;

      if ((((Nf&1)*(2-Nsp)+t1_ra)&1) == 0) {
        n_ra_prb = 6*f_ra;
      } else {
        n_ra_prb = frame_parms->N_RB_UL - 6*(f_ra+1);
      }
    }
  }
  else { //FDD
    n_ra_prb = n_ra_prboffset;
  }
  return(n_ra_prb);
}

int is_prach_subframe0(LTE_DL_FRAME_PARMS *frame_parms,uint8_t prach_ConfigIndex,uint32_t frame, uint8_t subframe)
{
  //  uint8_t prach_ConfigIndex  = frame_parms->prach_config_common.prach_ConfigInfo.prach_ConfigIndex;
  uint8_t tdd_config         = frame_parms->tdd_config;
  uint8_t t0_ra;
  uint8_t t1_ra;
  uint8_t t2_ra;

  int prach_mask = 0;

  if (frame_parms->frame_type == FDD) { //FDD
    //implement Table 5.7.1-2 from 36.211 (Rel-10, p.41)
    if ((((frame&1) == 1) && (subframe < 9)) ||
        (((frame&1) == 0) && (subframe == 9)))  // This is an odd frame, ignore even-only PRACH frames
      if (((prach_ConfigIndex&0xf)<3) || // 0,1,2,16,17,18,32,33,34,48,49,50
          ((prach_ConfigIndex&0x1f)==18) || // 18,50
          ((prach_ConfigIndex&0xf)==15))   // 15,47
        return(0);

    switch (prach_ConfigIndex&0x1f) {
    case 0:
    case 3:
      if (subframe==1) prach_mask = 1;
      break;

    case 1:
    case 4:
      if (subframe==4) prach_mask = 1;
      break;

    case 2:
    case 5:
      if (subframe==7) prach_mask = 1;
      break;

    case 6:
      if ((subframe==1) || (subframe==6)) prach_mask=1;
      break;

    case 7:
      if ((subframe==2) || (subframe==7)) prach_mask=1;
      break;

    case 8:
      if ((subframe==3) || (subframe==8)) prach_mask=1;
      break;

    case 9:
      if ((subframe==1) || (subframe==4) || (subframe==7)) prach_mask=1;
      break;

    case 10:
      if ((subframe==2) || (subframe==5) || (subframe==8)) prach_mask=1;
      break;

    case 11:
      if ((subframe==3) || (subframe==6) || (subframe==9)) prach_mask=1;
      break;

    case 12:
      if ((subframe&1)==0) prach_mask=1;
      break;

    case 13:
      if ((subframe&1)==1) prach_mask=1;
      break;

    case 14:
      prach_mask=1;
      break;

    case 15:
      if (subframe==9) prach_mask=1;
      break;
    }
  } else { // TDD

    AssertFatal(prach_ConfigIndex<64,
		"Illegal prach_ConfigIndex %d for ",prach_ConfigIndex);
    AssertFatal(tdd_preamble_map[prach_ConfigIndex][tdd_config].num_prach>0,
		"Illegal prach_ConfigIndex %d for ",prach_ConfigIndex);

    t0_ra = tdd_preamble_map[prach_ConfigIndex][tdd_config].map[0].t0_ra;
    t1_ra = tdd_preamble_map[prach_ConfigIndex][tdd_config].map[0].t1_ra;
    t2_ra = tdd_preamble_map[prach_ConfigIndex][tdd_config].map[0].t2_ra;
#ifdef PRACH_DEBUG
    LOG_I(PHY,"[PRACH] Checking for PRACH format (ConfigIndex %d) in TDD subframe %d (%d,%d,%d)\n",
          prach_ConfigIndex,
          subframe,
          t0_ra,t1_ra,t2_ra);
#endif

    if ((((t0_ra == 1) && ((frame &1)==0))||  // frame is even and PRACH is in even frames
         ((t0_ra == 2) && ((frame &1)==1))||  // frame is odd and PRACH is in odd frames
         (t0_ra == 0)) &&                                // PRACH is in all frames
        (((subframe<5)&&(t1_ra==0)) ||                   // PRACH is in 1st half-frame
         (((subframe>4)&&(t1_ra==1))))) {                // PRACH is in 2nd half-frame
      if ((prach_ConfigIndex<48) &&                          // PRACH only in normal UL subframe
	  (((subframe%5)-2)==t2_ra)) prach_mask=1;
      else if ((prach_ConfigIndex>47) && (((subframe%5)-1)==t2_ra)) prach_mask=1;      // PRACH can be in UpPTS
    }
  }

  return(prach_mask);
}

int is_prach_subframe(LTE_DL_FRAME_PARMS *frame_parms,uint32_t frame, uint8_t subframe) {
  
  uint8_t prach_ConfigIndex  = frame_parms->prach_config_common.prach_ConfigInfo.prach_ConfigIndex;
  int prach_mask             = is_prach_subframe0(frame_parms,prach_ConfigIndex,frame,subframe);

#ifdef Rel14
  int i;

  for (i=0;i<4;i++) {
    if (frame_parms->prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[i] == 1) 
      prach_mask|=(is_prach_subframe0(frame_parms,frame_parms->prach_emtc_config_common.prach_ConfigInfo.prach_ConfigIndex[i],frame,subframe)<<(i+1));
  }
#endif
  return(prach_mask);
}

int32_t generate_prach( PHY_VARS_UE *ue, uint8_t eNB_id, uint8_t subframe, uint16_t Nf )
{

  lte_frame_type_t frame_type         = ue->frame_parms.frame_type;
  //uint8_t tdd_config         = ue->frame_parms.tdd_config;
  uint16_t rootSequenceIndex = ue->frame_parms.prach_config_common.rootSequenceIndex;
  uint8_t prach_ConfigIndex  = ue->frame_parms.prach_config_common.prach_ConfigInfo.prach_ConfigIndex;
  uint8_t Ncs_config         = ue->frame_parms.prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig;
  uint8_t restricted_set     = ue->frame_parms.prach_config_common.prach_ConfigInfo.highSpeedFlag;
  //uint8_t n_ra_prboffset     = ue->frame_parms.prach_config_common.prach_ConfigInfo.prach_FreqOffset;
  uint8_t preamble_index     = ue->prach_resources[eNB_id]->ra_PreambleIndex;
  uint8_t tdd_mapindex       = ue->prach_resources[eNB_id]->ra_TDD_map_index;
  int16_t *prachF           = ue->prach_vars[eNB_id]->prachF;
  static int16_t prach_tmp[45600*2] __attribute__((aligned(32)));
  int16_t *prach            = prach_tmp;
  int16_t *prach2;
  int16_t amp               = ue->prach_vars[eNB_id]->amp;
  int16_t Ncp;
  uint8_t n_ra_prb;
  uint16_t NCS;
  uint16_t *prach_root_sequence_map;
  uint16_t preamble_offset,preamble_shift;
  uint16_t preamble_index0,n_shift_ra,n_shift_ra_bar;
  uint16_t d_start,numshift;

  uint8_t prach_fmt = get_prach_fmt(prach_ConfigIndex,frame_type);
  //uint8_t Nsp=2;
  //uint8_t f_ra,t1_ra;
  uint16_t N_ZC = (prach_fmt<4)?839:139;
  uint8_t not_found;
  int k;
  int16_t *Xu;
  uint16_t u;
  int32_t Xu_re,Xu_im;
  uint16_t offset,offset2;
  int prach_start;
  int i, prach_len;
  uint16_t first_nonzero_root_idx=0;

#if defined(EXMIMO) || defined(OAI_USRP)
  prach_start =  (ue->rx_offset+subframe*ue->frame_parms.samples_per_tti-ue->hw_timing_advance-ue->N_TA_offset);
#ifdef PRACH_DEBUG
    LOG_I(PHY,"[UE %d] prach_start %d, rx_offset %d, hw_timing_advance %d, N_TA_offset %d\n", ue->Mod_id,
        prach_start,
        ue->rx_offset,
        ue->hw_timing_advance,
        ue->N_TA_offset);
#endif

  if (prach_start<0)
    prach_start+=(ue->frame_parms.samples_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME);

  if (prach_start>=(ue->frame_parms.samples_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME))
    prach_start-=(ue->frame_parms.samples_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME);

#else //normal case (simulation)
  prach_start = subframe*ue->frame_parms.samples_per_tti-ue->N_TA_offset;
  LOG_I(PHY,"[UE %d] prach_start %d, rx_offset %d, hw_timing_advance %d, N_TA_offset %d\n", ue->Mod_id,
    prach_start,
    ue->rx_offset,
    ue->hw_timing_advance,
    ue->N_TA_offset);
  
#endif


  // First compute physical root sequence
  if (restricted_set == 0) {
    AssertFatal(Ncs_config <= 15,
		"[PHY] FATAL, Illegal Ncs_config for unrestricted format %"PRIu8"\n", Ncs_config );
    NCS = NCS_unrestricted[Ncs_config];
  } else {
    AssertFatal(Ncs_config <= 14,
		"[PHY] FATAL, Illegal Ncs_config for restricted format %"PRIu8"\n", Ncs_config );
    NCS = NCS_restricted[Ncs_config];
  }

  n_ra_prb = get_prach_prb_offset(&(ue->frame_parms),
				  ue->frame_parms.prach_config_common.prach_ConfigInfo.prach_ConfigIndex,
				  ue->frame_parms.prach_config_common.prach_ConfigInfo.prach_FreqOffset,
				  tdd_mapindex, Nf);
  prach_root_sequence_map = (prach_fmt<4) ? prach_root_sequence_map0_3 : prach_root_sequence_map4;

  /*
  // this code is not part of get_prach_prb_offset
  if (frame_type == TDD) { // TDD

    if (tdd_preamble_map[prach_ConfigIndex][tdd_config].num_prach==0) {
      LOG_E( PHY, "[PHY][UE %"PRIu8"] Illegal prach_ConfigIndex %"PRIu8" for ", ue->Mod_id, prach_ConfigIndex );
    }

    // adjust n_ra_prboffset for frequency multiplexing (p.36 36.211)
    f_ra = tdd_preamble_map[prach_ConfigIndex][tdd_config].map[tdd_mapindex].f_ra;

    if (prach_fmt < 4) {
      if ((f_ra&1) == 0) {
        n_ra_prb = n_ra_prboffset + 6*(f_ra>>1);
      } else {
        n_ra_prb = ue->frame_parms.N_RB_UL - 6 - n_ra_prboffset + 6*(f_ra>>1);
      }
    } else {
      if ((tdd_config >2) && (tdd_config<6))
        Nsp = 2;

      t1_ra = tdd_preamble_map[prach_ConfigIndex][tdd_config].map[0].t1_ra;

      if ((((Nf&1)*(2-Nsp)+t1_ra)&1) == 0) {
        n_ra_prb = 6*f_ra;
      } else {
        n_ra_prb = ue->frame_parms.N_RB_UL - 6*(f_ra+1);
      }
    }
  }
  */

  // This is the relative offset (for unrestricted case) in the root sequence table (5.7.2-4 from 36.211) for the given preamble index
  preamble_offset = ((NCS==0)? preamble_index : (preamble_index/(N_ZC/NCS)));

  if (restricted_set == 0) {
    // This is the \nu corresponding to the preamble index
    preamble_shift  = (NCS==0)? 0 : (preamble_index % (N_ZC/NCS));
    preamble_shift *= NCS;
  } else { // This is the high-speed case

#ifdef PRACH_DEBUG
    LOG_I(PHY,"[UE %d] High-speed mode, NCS_config %d\n",ue->Mod_id,Ncs_config);
#endif

    not_found = 1;
    preamble_index0 = preamble_index;
    // set preamble_offset to initial rootSequenceIndex and look if we need more root sequences for this
    // preamble index and find the corresponding cyclic shift
    preamble_offset = 0; // relative rootSequenceIndex;

    while (not_found == 1) {
      // current root depending on rootSequenceIndex and preamble_offset
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

      if (numshift>0 && preamble_index0==preamble_index)
        first_nonzero_root_idx = preamble_offset;

      if (preamble_index0 < numshift) {
        not_found      = 0;
        preamble_shift = (d_start * (preamble_index0/n_shift_ra)) + ((preamble_index0%n_shift_ra)*NCS);

      } else { // skip to next rootSequenceIndex and recompute parameters
        preamble_offset++;
        preamble_index0 -= numshift;
      }
    }
  }

  // now generate PRACH signal
#ifdef PRACH_DEBUG

  if (NCS>0)
    LOG_I(PHY,"Generate PRACH for RootSeqIndex %d, Preamble Index %d, NCS %d (NCS_config %d, N_ZC/NCS %d) n_ra_prb %d: Preamble_offset %d, Preamble_shift %d\n",
          rootSequenceIndex,preamble_index,NCS,Ncs_config,N_ZC/NCS,n_ra_prb,
          preamble_offset,preamble_shift);

#endif

  //  nsymb = (frame_parms->Ncp==0) ? 14:12;
  //  subframe_offset = (unsigned int)frame_parms->ofdm_symbol_size*subframe*nsymb;

  k = (12*n_ra_prb) - 6*ue->frame_parms.N_RB_UL;

  if (k<0)
    k+=ue->frame_parms.ofdm_symbol_size;

  k*=12;
  k+=13;

  Xu = (int16_t*)ue->X_u[preamble_offset-first_nonzero_root_idx];

  /*
    k+=(12*ue->frame_parms.first_carrier_offset);
    if (k>(12*ue->frame_parms.ofdm_symbol_size))
    k-=(12*ue->frame_parms.ofdm_symbol_size);
  */
  k*=2;

  switch (ue->frame_parms.N_RB_UL) {
  case 6:
    memset((void*)prachF,0,4*1536);
    break;

  case 15:
    memset((void*)prachF,0,4*3072);
    break;

  case 25:
    memset((void*)prachF,0,4*6144);
    break;

  case 50:
    memset((void*)prachF,0,4*12288);
    break;

  case 75:
    memset((void*)prachF,0,4*18432);
    break;

  case 100:
    if (ue->frame_parms.threequarter_fs == 0)
      memset((void*)prachF,0,4*24576);
    else
      memset((void*)prachF,0,4*18432);
    break;
  }

  for (offset=0,offset2=0; offset<N_ZC; offset++,offset2+=preamble_shift) {

    if (offset2 >= N_ZC)
      offset2 -= N_ZC;

    Xu_re = (((int32_t)Xu[offset<<1]*amp)>>15);
    Xu_im = (((int32_t)Xu[1+(offset<<1)]*amp)>>15);
    prachF[k++]= ((Xu_re*ru[offset2<<1]) - (Xu_im*ru[1+(offset2<<1)]))>>15;
    prachF[k++]= ((Xu_im*ru[offset2<<1]) + (Xu_re*ru[1+(offset2<<1)]))>>15;

    if (k==(12*2*ue->frame_parms.ofdm_symbol_size))
      k=0;
  }

  switch (prach_fmt) {
  case 0:
    Ncp = 3168;
    break;

  case 1:
  case 3:
    Ncp = 21024;
    break;

  case 2:
    Ncp = 6240;
    break;

  case 4:
    Ncp = 448;
    break;

  default:
    Ncp = 3168;
    break;
  }

  switch (ue->frame_parms.N_RB_UL) {
  case 6:
    Ncp>>=4;
    prach+=4; // makes prach2 aligned to 128-bit
    break;

  case 15:
    Ncp>>=3;
    break;

  case 25:
    Ncp>>=2;
    break;

  case 50:
    Ncp>>=1;
    break;

  case 75:
    Ncp=(Ncp*3)>>2;
    break;
  }

  if (ue->frame_parms.threequarter_fs == 1)
    Ncp=(Ncp*3)>>2;

  prach2 = prach+(Ncp<<1);

  // do IDFT
  switch (ue->frame_parms.N_RB_UL) {
  case 6:
    if (prach_fmt == 4) {
      idft256(prachF,prach2,1);
      memmove( prach, prach+512, Ncp<<2 );
      prach_len = 256+Ncp;
    } else {
      idft1536(prachF,prach2,1);
      memmove( prach, prach+3072, Ncp<<2 );
      prach_len = 1536+Ncp;

      if (prach_fmt>1) {
        memmove( prach2+3072, prach2, 6144 );
        prach_len = 2*1536+Ncp;
      }
    }

    break;

  case 15:
    if (prach_fmt == 4) {
      idft512(prachF,prach2,1);
      //TODO: account for repeated format in dft output
      memmove( prach, prach+1024, Ncp<<2 );
      prach_len = 512+Ncp;
    } else {
      idft3072(prachF,prach2);
      memmove( prach, prach+6144, Ncp<<2 );
      prach_len = 3072+Ncp;

      if (prach_fmt>1) {
        memmove( prach2+6144, prach2, 12288 );
        prach_len = 2*3072+Ncp;
      }
    }

    break;

  case 25:
  default:
    if (prach_fmt == 4) {
      idft1024(prachF,prach2,1);
      memmove( prach, prach+2048, Ncp<<2 );
      prach_len = 1024+Ncp;
    } else {
      idft6144(prachF,prach2);
      /*for (i=0;i<6144*2;i++)
      prach2[i]<<=1;*/
      memmove( prach, prach+12288, Ncp<<2 );
      prach_len = 6144+Ncp;

      if (prach_fmt>1) {
        memmove( prach2+12288, prach2, 24576 );
        prach_len = 2*6144+Ncp;
      }
    }

    break;

  case 50:
    if (prach_fmt == 4) {
      idft2048(prachF,prach2,1);
      memmove( prach, prach+4096, Ncp<<2 );
      prach_len = 2048+Ncp;
    } else {
      idft12288(prachF,prach2);
      memmove( prach, prach+24576, Ncp<<2 );
      prach_len = 12288+Ncp;

      if (prach_fmt>1) {
        memmove( prach2+24576, prach2, 49152 );
        prach_len = 2*12288+Ncp;
      }
    }

    break;

  case 75:
    if (prach_fmt == 4) {
      idft3072(prachF,prach2);
      //TODO: account for repeated format in dft output
      memmove( prach, prach+6144, Ncp<<2 );
      prach_len = 3072+Ncp;
    } else {
      idft18432(prachF,prach2);
      memmove( prach, prach+36864, Ncp<<2 );
      prach_len = 18432+Ncp;

      if (prach_fmt>1) {
        memmove( prach2+36834, prach2, 73728 );
        prach_len = 2*18432+Ncp;
      }
    }

    break;

  case 100:
    if (ue->frame_parms.threequarter_fs == 0) { 
      if (prach_fmt == 4) {
	idft4096(prachF,prach2,1);
	memmove( prach, prach+8192, Ncp<<2 );
	prach_len = 4096+Ncp;
      } else {
	idft24576(prachF,prach2);
	memmove( prach, prach+49152, Ncp<<2 );
	prach_len = 24576+Ncp;
	
	if (prach_fmt>1) {
	  memmove( prach2+49152, prach2, 98304 );
	  prach_len = 2* 24576+Ncp;
	}
      }
    }
    else {
      if (prach_fmt == 4) {
	idft3072(prachF,prach2);
	//TODO: account for repeated format in dft output
	memmove( prach, prach+6144, Ncp<<2 );
	prach_len = 3072+Ncp;
      } else {
	idft18432(prachF,prach2);
	memmove( prach, prach+36864, Ncp<<2 );
	prach_len = 18432+Ncp;
	printf("Generated prach for 100 PRB, 3/4 sampling\n");
	if (prach_fmt>1) {
	  memmove( prach2+36834, prach2, 73728 );
	  prach_len = 2*18432+Ncp;
	}
      } 
    }

    break;
  }

  //LOG_I(PHY,"prach_len=%d\n",prach_len);

  AssertFatal(prach_fmt<4,
	      "prach_fmt4 not fully implemented" );
#if defined(EXMIMO) || defined(OAI_USRP)
  int j;
  int overflow = prach_start + prach_len - LTE_NUMBER_OF_SUBFRAMES_PER_FRAME*ue->frame_parms.samples_per_tti;
  LOG_I( PHY, "prach_start=%d, overflow=%d\n", prach_start, overflow );
  
  for (i=prach_start,j=0; i<min(ue->frame_parms.samples_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME,prach_start+prach_len); i++,j++) {
    ((int16_t*)ue->common_vars.txdata[0])[2*i] = prach[2*j]<<4;
    ((int16_t*)ue->common_vars.txdata[0])[2*i+1] = prach[2*j+1]<<4;
  }
  
  for (i=0; i<overflow; i++,j++) {
    ((int16_t*)ue->common_vars.txdata[0])[2*i] = prach[2*j]<<4;
    ((int16_t*)ue->common_vars.txdata[0])[2*i+1] = prach[2*j+1]<<4;
  }
#if defined(EXMIMO)
  // handle switch before 1st TX subframe, guarantee that the slot prior to transmission is switch on
  for (k=prach_start - (ue->frame_parms.samples_per_tti>>1) ; k<prach_start ; k++) {
    if (k<0)
      ue->common_vars.txdata[0][k+ue->frame_parms.samples_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME] &= 0xFFFEFFFE;
    else if (k>(ue->frame_parms.samples_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME))
      ue->common_vars.txdata[0][k-ue->frame_parms.samples_per_tti*LTE_NUMBER_OF_SUBFRAMES_PER_FRAME] &= 0xFFFEFFFE;
    else
      ue->common_vars.txdata[0][k] &= 0xFFFEFFFE;
  }
#endif
#else
  
  for (i=0; i<prach_len; i++) {
    ((int16_t*)(&ue->common_vars.txdata[0][prach_start]))[2*i] = prach[2*i];
    ((int16_t*)(&ue->common_vars.txdata[0][prach_start]))[2*i+1] = prach[2*i+1];
  }
  
#endif
  

  
#if defined(PRACH_WRITE_OUTPUT_DEBUG)
  write_output("prach_txF0.m","prachtxF0",prachF,prach_len-Ncp,1,1);
  write_output("prach_tx0.m","prachtx0",prach+(Ncp<<1),prach_len-Ncp,1,1);
  write_output("txsig.m","txs",(int16_t*)(&ue->common_vars.txdata[0][0]),2*ue->frame_parms.samples_per_tti,1,1);
  exit(-1);
#endif

  return signal_energy( (int*)prach, 256 );
}
//__m128i mmtmpX0,mmtmpX1,mmtmpX2,mmtmpX3;

#ifndef Rel14
#define rx_prach0 rx_prach
#endif

void rx_prach0(PHY_VARS_eNB *eNB,
	       RU_t *ru,
	       uint16_t *max_preamble,
	       uint16_t *max_preamble_energy,
	       uint16_t *max_preamble_delay,
	       uint16_t Nf, 
	       uint8_t tdd_mapindex
#ifdef Rel14
	       ,uint8_t br_flag,
	       uint8_t ce_level
#endif
	       )
{

  int i;

  LTE_DL_FRAME_PARMS *fp;
  lte_frame_type_t   frame_type;
  uint16_t           rootSequenceIndex;  
  uint8_t            prach_ConfigIndex;   
  uint8_t            Ncs_config;          
  uint8_t            restricted_set;      
  uint8_t            n_ra_prb;

#ifdef PRACH_DEBUG
  int                frame;
#endif
  int                subframe;
  int16_t            *prachF=NULL;
  int16_t            **rxsigF=NULL;
  int                nb_rx;

  int16_t *prach2;
  uint8_t preamble_index;
  uint16_t NCS,NCS2;
  uint16_t preamble_offset=0,preamble_offset_old;
  int16_t preamble_shift=0;
  uint32_t preamble_shift2;
  uint16_t preamble_index0=0,n_shift_ra=0,n_shift_ra_bar;
  uint16_t d_start=0;
  uint16_t numshift=0;
  uint16_t *prach_root_sequence_map;
  uint8_t not_found;
  int k=0;
  uint16_t u;
  int16_t *Xu;
  uint16_t offset;
  int16_t Ncp;
  uint16_t first_nonzero_root_idx=0;
  uint8_t new_dft=0;
  uint8_t aa;
  int32_t lev;
  int16_t levdB;
  int fft_size,log2_ifft_size;
  int16_t prach_ifft_tmp[2048*2] __attribute__((aligned(32)));
  int32_t *prach_ifft=(int32_t*)NULL;
  int32_t **prach_ifftp=(int32_t **)NULL;
#ifdef Rel14
  int prach_ifft_cnt=0;
#endif
#ifdef PRACH_DEBUG
  int en0=0;
#endif

  if (ru) { 
    fp    = &ru->frame_parms;
    nb_rx = ru->nb_rx;
  }
  else if (eNB) {
    fp    = &eNB->frame_parms;
    nb_rx = fp->nb_antennas_rx;
  }
  else AssertFatal(1==0,"rx_prach called without valid RU or eNB descriptor\n");
  
  frame_type          = fp->frame_type;

#ifdef Rel14
  if (br_flag == 1) {
    AssertFatal(fp->prach_emtc_config_common.prach_Config_enabled==1,
		"emtc prach_Config is not enabled\n");
    AssertFatal(fp->prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[ce_level]==1,
		"ce_level %d is not active\n",ce_level);
    rootSequenceIndex   = fp->prach_emtc_config_common.rootSequenceIndex;
    prach_ConfigIndex   = fp->prach_emtc_config_common.prach_ConfigInfo.prach_ConfigIndex[ce_level];
    Ncs_config          = fp->prach_emtc_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig;
    restricted_set      = fp->prach_emtc_config_common.prach_ConfigInfo.highSpeedFlag;
    n_ra_prb            = get_prach_prb_offset(fp,prach_ConfigIndex,
					       fp->prach_emtc_config_common.prach_ConfigInfo.prach_FreqOffset[ce_level],
					       tdd_mapindex,Nf);
    // update pointers to results for ce_level
    max_preamble        += ce_level;
    max_preamble_energy += ce_level;
    max_preamble_delay  += ce_level;
  }
  else 
#endif
    {
      rootSequenceIndex   = fp->prach_config_common.rootSequenceIndex;
      prach_ConfigIndex   = fp->prach_config_common.prach_ConfigInfo.prach_ConfigIndex;
      Ncs_config          = fp->prach_config_common.prach_ConfigInfo.zeroCorrelationZoneConfig;
      restricted_set      = fp->prach_config_common.prach_ConfigInfo.highSpeedFlag;
      n_ra_prb            = get_prach_prb_offset(fp,prach_ConfigIndex,
						 fp->prach_config_common.prach_ConfigInfo.prach_FreqOffset,
						 tdd_mapindex,Nf);
    }

  int16_t *prach[nb_rx];
  uint8_t prach_fmt = get_prach_fmt(prach_ConfigIndex,frame_type);
  uint16_t N_ZC = (prach_fmt <4)?839:139;
  
  if (eNB) {
#ifdef Rel14
    if (br_flag == 1) {
      prach_ifftp         = eNB->prach_vars_br.prach_ifft[ce_level];
#ifdef PRACH_DEBUG
      frame               = eNB->proc.frame_prach_br;
#endif
      subframe            = eNB->proc.subframe_prach_br;
      prachF              = eNB->prach_vars_br.prachF;
      rxsigF              = eNB->prach_vars_br.rxsigF[ce_level];
#ifdef PRACH_DEBUG
      if ((frame&1023) < 20) LOG_I(PHY,"PRACH (eNB) : running rx_prach (br_flag %d, ce_level %d) for frame %d subframe %d, prach_FreqOffset %d, prach_ConfigIndex %d, rootSequenceIndex %d, repetition number %d,numRepetitionsPrePreambleAttempt %d\n",
				   br_flag,ce_level,frame,subframe,
				   fp->prach_emtc_config_common.prach_ConfigInfo.prach_FreqOffset[ce_level],
				   prach_ConfigIndex,rootSequenceIndex,
				   eNB->prach_vars_br.repetition_number[ce_level],
				   fp->prach_emtc_config_common.prach_ConfigInfo.prach_numRepetitionPerPreambleAttempt[ce_level]);
#endif
    }
    else
#endif
      {
        prach_ifftp       = eNB->prach_vars.prach_ifft[0];
#ifdef PRACH_DEBUG
        frame             = eNB->proc.frame_prach;
#endif
        subframe          = eNB->proc.subframe_prach;
        prachF            = eNB->prach_vars.prachF;
        rxsigF            = eNB->prach_vars.rxsigF[0];
#ifdef PRACH_DEBUG
        //if ((frame&1023) < 20) LOG_I(PHY,"PRACH (eNB) : running rx_prach for subframe %d, prach_FreqOffset %d, prach_ConfigIndex %d , rootSequenceIndex %d\n", subframe,fp->prach_config_common.prach_ConfigInfo.prach_FreqOffset,prach_ConfigIndex,rootSequenceIndex);
#endif
      }
  }
  else {
#ifdef Rel14
    if (br_flag == 1) {
#ifdef PRACH_DEBUG
        frame             = ru->proc.frame_prach_br;
#endif
        subframe          = ru->proc.subframe_prach_br;
        rxsigF            = ru->prach_rxsigF_br[ce_level];
#ifdef PRACH_DEBUG
        if ((frame&1023) < 20) LOG_I(PHY,"PRACH (RU) : running rx_prach (br_flag %d, ce_level %d) for frame %d subframe %d, prach_FreqOffset %d, prach_ConfigIndex %d\n",
				     br_flag,ce_level,frame,subframe,fp->prach_emtc_config_common.prach_ConfigInfo.prach_FreqOffset[ce_level],prach_ConfigIndex);
#endif
    }
    else
#endif
      {
#ifdef PRACH_DEBUG
        frame             = ru->proc.frame_prach;
#endif
        subframe          = ru->proc.subframe_prach;
        rxsigF            = ru->prach_rxsigF;
#ifdef PRACH_DEBUG
        if ((frame&1023) < 20) LOG_I(PHY,"PRACH (RU) : running rx_prach for subframe %d, prach_FreqOffset %d, prach_ConfigIndex %d\n",
	      subframe,fp->prach_config_common.prach_ConfigInfo.prach_FreqOffset,prach_ConfigIndex);
#endif
      }

  }

  AssertFatal(ru!=NULL,"ru is null\n");

  for (aa=0; aa<nb_rx; aa++) {
    if (ru->if_south == LOCAL_RF) { // set the time-domain signal if we have to use it in this node
      // DJP - indexing below in subframe zero takes us off the beginning of the array???
      prach[aa] = (int16_t*)&ru->common.rxdata[aa][(subframe*fp->samples_per_tti)-ru->N_TA_offset];
#ifdef PRACH_DEBUG
      int32_t en0=signal_energy((int32_t*)prach[aa],fp->samples_per_tti);
      int8_t dbEn0 = dB_fixed(en0);
      int8_t rach_dBm = dbEn0 - eNB->rx_total_gain_dB;

#ifdef PRACH_WRITE_OUTPUT_DEBUG
        if (dbEn0>32 && prach[0]!= NULL)
        {
          static int counter=0;

          char buffer[80];
          //counter++;
          sprintf(buffer, "%s%d", "/tmp/prach_rx",counter);
          write_output(buffer,"prach_rx",prach[0],fp->samples_per_tti,1,13);
        }
#endif

      if (dbEn0>32)
      {
#ifdef PRACH_WRITE_OUTPUT_DEBUG
        if (prach[0]!= NULL) write_output("prach_rx","prach_rx",prach[0],fp->samples_per_tti,1,1);
#endif
        LOG_I(PHY,"RU %d, br_flag %d ce_level %d frame %d subframe %d per_tti:%d prach:%p (energy %d) TA:%d rach_dBm:%d rxdata:%p index:%d\n",ru->idx,br_flag,ce_level,frame,subframe,fp->samples_per_tti,prach[aa],dbEn0,ru->N_TA_offset,rach_dBm,ru->common.rxdata[aa], (subframe*fp->samples_per_tti)-ru->N_TA_offset);
        }
#endif
    }
  }

  // First compute physical root sequence
  if (restricted_set == 0) {
    AssertFatal(Ncs_config<=15,
		"Illegal Ncs_config for unrestricted format %d\n",Ncs_config);
    NCS = NCS_unrestricted[Ncs_config];
  } else {
    AssertFatal(Ncs_config<=14,
		"FATAL, Illegal Ncs_config for restricted format %d\n",Ncs_config);
    NCS = NCS_restricted[Ncs_config];
  }

  if (eNB) start_meas(&eNB->rx_prach);


  prach_root_sequence_map = (prach_fmt < 4) ? prach_root_sequence_map0_3 : prach_root_sequence_map4;

  // PDP is oversampled, e.g. 1024 sample instead of 839
  // Adapt the NCS (zero-correlation zones) with oversampling factor e.g. 1024/839
  NCS2 = (N_ZC==839) ? ((NCS<<10)/839) : ((NCS<<8)/139);

  if (NCS2==0)
    NCS2 = N_ZC;

  switch (prach_fmt) {
  case 0:
    Ncp = 3168;
    break;

  case 1:
  case 3:
    Ncp = 21024;
    break;

  case 2:
    Ncp = 6240;
    break;

  case 4:
    Ncp = 448;
    break;

  default:
    Ncp = 3168;
    break;
  }

  // Adjust CP length based on UL bandwidth
  switch (fp->N_RB_UL) {
  case 6:
    Ncp>>=4;
    break;

  case 15:
    Ncp>>=3;
    break;

  case 25:
    Ncp>>=2;
    break;

  case 50:
    Ncp>>=1;
    break;

  case 75:
    Ncp=(Ncp*3)>>2;
    break;

  case 100:
    if (fp->threequarter_fs == 1)
      Ncp=(Ncp*3)>>2;
    break;
  }


  if (((eNB!=NULL) && (ru->function != NGFI_RAU_IF4p5))||
      ((eNB==NULL) && (ru->function == NGFI_RRU_IF4p5))) { // compute the DFTs of the PRACH temporal resources
    // Do forward transform
#ifdef PRACH_DEBUG
    LOG_D(PHY,"rx_prach: Doing FFT for N_RB_UL %d nb_rx:%d Ncp:%d\n",fp->N_RB_UL, nb_rx, Ncp);
#endif
    for (aa=0; aa<nb_rx; aa++) {
      AssertFatal(prach[aa]!=NULL,"prach[%d] is null\n",aa);
      prach2 = prach[aa] + (Ncp<<1);
  
      // do DFT
      switch (fp->N_RB_UL) {
      case 6:
	if (prach_fmt == 4) {
	  dft256(prach2,rxsigF[aa],1);
	} else {
	  dft1536(prach2,rxsigF[aa],1);
	  
	  if (prach_fmt>1)
	    dft1536(prach2+3072,rxsigF[aa]+3072,1);
	}
	
	break;
	
      case 15:
	if (prach_fmt == 4) {
	  dft256(prach2,rxsigF[aa],1);
	} else {
	  dft3072(prach2,rxsigF[aa]);
	  
	  if (prach_fmt>1)
	    dft3072(prach2+6144,rxsigF[aa]+6144);
	}
	
	break;
	
      case 25:
      default:
	if (prach_fmt == 4) {
	  dft1024(prach2,rxsigF[aa],1);
	  fft_size = 1024;
	} else {
	  dft6144(prach2,rxsigF[aa]);
	  
	  if (prach_fmt>1)
	    dft6144(prach2+12288,rxsigF[aa]+12288);
	  
	  fft_size = 6144;
	}
	
	break;
	
      case 50:
	if (prach_fmt == 4) {
	  dft2048(prach2,rxsigF[aa],1);
	} else {
	  dft12288(prach2,rxsigF[aa]);
	  
	  if (prach_fmt>1)
	    dft12288(prach2+24576,rxsigF[aa]+24576);
	}
	
	break;
	
      case 75:
	if (prach_fmt == 4) {
	  dft3072(prach2,rxsigF[aa]);
	} else {
	  dft18432(prach2,rxsigF[aa]);
	  
	  if (prach_fmt>1)
	    dft18432(prach2+36864,rxsigF[aa]+36864);
	}
	
	break;
	
      case 100:
	if (fp->threequarter_fs==0) {
	  if (prach_fmt == 4) {
	    dft4096(prach2,rxsigF[aa],1);
	  } else {
	    dft24576(prach2,rxsigF[aa]);
	    
	    if (prach_fmt>1)
	      dft24576(prach2+49152,rxsigF[aa]+49152);
	  }
	} else {
	  if (prach_fmt == 4) {
	    dft3072(prach2,rxsigF[aa]);
	  } else {
	    dft18432(prach2,rxsigF[aa]);
	    
	    if (prach_fmt>1)
	      dft18432(prach2+36864,rxsigF[aa]+36864);
	  }
	}
	
	break;
      }

      k = (12*n_ra_prb) - 6*fp->N_RB_UL;
      
      if (k<0) {
	k+=(fp->ofdm_symbol_size);
      }
      
      k*=12;
      k+=13; 
      k*=2;
      int dftsize_x2 = fp->ofdm_symbol_size*24;
      //LOG_D(PHY,"Shifting prach_rxF from %d to 0\n",k);

      if ((k+(839*2)) > dftsize_x2) { // PRACH signal is split around DC 
	memmove((void*)&rxsigF[aa][dftsize_x2-k],(void*)&rxsigF[aa][0],(k+(839*2)-dftsize_x2)*2);	
	memmove((void*)&rxsigF[aa][0],(void*)(&rxsigF[aa][k]),(dftsize_x2-k)*2);	
      }
      else  // PRACH signal is not split around DC
	memmove((void*)&rxsigF[aa][0],(void*)(&rxsigF[aa][k]),839*4);	
      
    }
	     
  }

  if ((eNB==NULL) && (ru!=NULL) && ru->function == NGFI_RRU_IF4p5) {

    /// **** send_IF4 of rxsigF to RAU **** ///
#ifdef Rel14
    if (br_flag == 1) send_IF4p5(ru, ru->proc.frame_prach, ru->proc.subframe_prach, IF4p5_PRACH+1+ce_level);      

    else
#endif
      send_IF4p5(ru, ru->proc.frame_prach, ru->proc.subframe_prach, IF4p5_PRACH);
    
    return;
  } else if (eNB!=NULL) {

#ifdef PRACH_DEBUG
    int en = dB_fixed(signal_energy((int32_t*)&rxsigF[0][0],840));
    if ((en > 60)&&(br_flag==1)) LOG_I(PHY,"PRACH (br_flag %d,ce_level %d, n_ra_prb %d, k %d): Frame %d, Subframe %d => %d dB\n",br_flag,ce_level,n_ra_prb,k,eNB->proc.frame_rx,eNB->proc.subframe_rx,en);
#endif
  }
  
  // in case of RAU and prach received rx_thread wakes up prach

  // here onwards is for eNodeB_3GPP or NGFI_RAU_IF4p5

  preamble_offset_old = 99;

  uint8_t update_TA  = 4;
  uint8_t update_TA2 = 1;
  switch (eNB->frame_parms.N_RB_DL) {
  case 6:
    update_TA = 16;
    break;
    
  case 25:
    update_TA = 4;
    break;
    
  case 50:
    update_TA = 2;
    break;
    
  case 75:
    update_TA  = 3;
    update_TA2 = 2;
  case 100:
    update_TA  = 1;
    break;
  }
  
  *max_preamble_energy=0;
  for (preamble_index=0 ; preamble_index<64 ; preamble_index++) {

#ifdef PRACH_DEBUG
    if (en>60) LOG_I(PHY,"frame %d, subframe %d : Trying preamble %d (br_flag %d)\n",frame,subframe,preamble_index,br_flag);
#endif
    if (restricted_set == 0) {
      // This is the relative offset in the root sequence table (5.7.2-4 from 36.211) for the given preamble index
      preamble_offset = ((NCS==0)? preamble_index : (preamble_index/(N_ZC/NCS)));
      
      if (preamble_offset != preamble_offset_old) {
        preamble_offset_old = preamble_offset;
        new_dft = 1;
        // This is the \nu corresponding to the preamble index
        preamble_shift  = 0;
      }
      
      else {
        preamble_shift  -= NCS;
	
        if (preamble_shift < 0)
          preamble_shift+=N_ZC;
      }
    } else { // This is the high-speed case
      new_dft = 0;

      // set preamble_offset to initial rootSequenceIndex and look if we need more root sequences for this
      // preamble index and find the corresponding cyclic shift
      // Check if all shifts for that root have been processed
      if (preamble_index0 == numshift) {
        not_found = 1;
        new_dft   = 1;
        preamble_index0 -= numshift;
        (preamble_offset==0 && numshift==0) ? (preamble_offset) : (preamble_offset++);

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
          (numshift>0) ? (not_found = 0) : (preamble_offset++);
        }
      }

      if (n_shift_ra>0)
        preamble_shift = -((d_start * (preamble_index0/n_shift_ra)) + ((preamble_index0%n_shift_ra)*NCS)); // minus because the channel is h(t -\tau + Cv)
      else
        preamble_shift = 0;

      if (preamble_shift < 0)
        preamble_shift+=N_ZC;

      preamble_index0++;

      if (preamble_index == 0)
        first_nonzero_root_idx = preamble_offset;
    }

    // Compute DFT of RX signal (conjugate input, results in conjugate output) for each new rootSequenceIndex
#ifdef PRACH_DEBUG
    if (en>60) LOG_I(PHY,"frame %d, subframe %d : preamble index %d: offset %d, preamble shift %d (br_flag %d, en %d)\n",
		     frame,subframe,preamble_index,preamble_offset,preamble_shift,br_flag,en);
#endif
    log2_ifft_size = 10;
    fft_size = 6144;

    if (new_dft == 1) {
      new_dft = 0;

#ifdef Rel14
      if (br_flag == 1) {
	Xu=(int16_t*)eNB->X_u_br[ce_level][preamble_offset-first_nonzero_root_idx];
	prach_ifft = prach_ifftp[prach_ifft_cnt++];
	if (eNB->prach_vars_br.repetition_number[ce_level]==1) memset(prach_ifft,0,((N_ZC==839)?2048:256)*sizeof(int32_t));
      }
      else
#endif
	{
	  Xu=(int16_t*)eNB->X_u[preamble_offset-first_nonzero_root_idx];
	  prach_ifft = prach_ifftp[0];
          memset(prach_ifft,0,((N_ZC==839) ? 2048 : 256)*sizeof(int32_t));
	}

      memset(prachF, 0, sizeof(int16_t)*2*1024 );
#if defined(PRACH_WRITE_OUTPUT_DEBUG)
      if (prach[0]!= NULL) write_output("prach_rx0.m","prach_rx0",prach[0],6144+792,1,1);
#endif
      // write_output("prach_rx1.m","prach_rx1",prach[1],6144+792,1,1);
      //       write_output("prach_rxF0.m","prach_rxF0",rxsigF[0],24576,1,1);
      // write_output("prach_rxF1.m","prach_rxF1",rxsigF[1],6144,1,1);

      for (aa=0;aa<nb_rx; aa++) {
      // Do componentwise product with Xu* on each antenna 

	k=0;	
	for (offset=0; offset<(N_ZC<<1); offset+=2) {
	  prachF[offset]   = (int16_t)(((int32_t)Xu[offset]*rxsigF[aa][k]   + (int32_t)Xu[offset+1]*rxsigF[aa][k+1])>>15);
	  prachF[offset+1] = (int16_t)(((int32_t)Xu[offset]*rxsigF[aa][k+1] - (int32_t)Xu[offset+1]*rxsigF[aa][k])>>15);
	  k+=2;
	  if (k==(12*2*fp->ofdm_symbol_size))
	    k=0;
	}
	
	// Now do IFFT of size 1024 (N_ZC=839) or 256 (N_ZC=139)
	if (N_ZC == 839) {
	  log2_ifft_size = 10;
	  idft1024(prachF,prach_ifft_tmp,1);
	  // compute energy and accumulate over receive antennas and repetitions for BR
	  for (i=0;i<2048;i++)
	    prach_ifft[i] += (prach_ifft_tmp[i<<1]*prach_ifft_tmp[i<<1] + prach_ifft_tmp[1+(i<<1)]*prach_ifft_tmp[1+(i<<1)])>>10;
	} else {
	  idft256(prachF,prach_ifft_tmp,1);
	  log2_ifft_size = 8;
	  // compute energy and accumulate over receive antennas and repetitions for BR
	  for (i=0;i<256;i++)
	    prach_ifft[i] += (prach_ifft_tmp[i<<1]*prach_ifft_tmp[(i<<1)] + prach_ifft_tmp[1+(i<<1)]*prach_ifft_tmp[1+(i<<1)])>>10;
	}
	
#if defined(PRACH_WRITE_OUTPUT_DEBUG)
	if (aa==0) write_output("prach_rxF_comp0.m","prach_rxF_comp0",prachF,1024,1,1);
#endif
      // if (aa=1) write_output("prach_rxF_comp1.m","prach_rxF_comp1",prachF,1024,1,1);
      }// antennas_rx
    } // new dft
    
    // check energy in nth time shift, for 
#ifdef Rel14
    if ((br_flag==0) ||
	(eNB->prach_vars_br.repetition_number[ce_level]==
	 eNB->frame_parms.prach_emtc_config_common.prach_ConfigInfo.prach_numRepetitionPerPreambleAttempt[ce_level]))
#endif
      {
#ifdef PRACH_DEBUG
	if (en>60) LOG_I(PHY,"frame %d, subframe %d: Checking for peak in time-domain (br_flag %d, en %d)\n",frame,subframe,br_flag,en);
#endif
	preamble_shift2 = ((preamble_shift==0) ? 0 : ((preamble_shift<<log2_ifft_size)/N_ZC));

    
	for (i=0; i<NCS2; i++) {
	  lev = (int32_t)prach_ifft[(preamble_shift2+i)];
	  levdB = dB_fixed_times10(lev);
	  
	  if (levdB>*max_preamble_energy) {
	    *max_preamble_energy  = levdB;
	    *max_preamble_delay   = ((i*fft_size)>>log2_ifft_size)*update_TA/update_TA2;
	    *max_preamble         = preamble_index;
#ifdef PRACH_DEBUG
	    if ((en>60) && (br_flag==1)) LOG_D(PHY,"frame %d, subframe %d : max_preamble_energy %d, max_preamble_delay %d, max_preamble %d (br_flag %d,ce_level %d, levdB %d, lev %d)\n",frame,subframe,*max_preamble_energy,*max_preamble_delay,*max_preamble,br_flag,ce_level,levdB,lev);
#endif
	  }
	}

      }
  }// preamble_index

#ifdef PRACH_DEBUG
  
  if (en>60) {
    k = (12*n_ra_prb) - 6*fp->N_RB_UL;
    
    if (k<0) k+=fp->ofdm_symbol_size;
    
    k*=12;
    k+=13;
    k*=2;
    
    if (br_flag == 0) {
#if defined(PRACH_WRITE_OUTPUT_DEBUG)
	write_output("rxsigF.m","prach_rxF",&rxsigF[0][0],12288,1,1);
	write_output("prach_rxF_comp0.m","prach_rxF_comp0",prachF,1024,1,1);
	write_output("Xu.m","xu",Xu,N_ZC,1,1);
	write_output("prach_ifft0.m","prach_t0",prach_ifft,1024,1,1);
#endif
    }
    else {
#if defined(PRACH_WRITE_OUTPUT_DEBUG)
      printf("Dumping prach (br_flag %d), k = %d (n_ra_prb %d)\n",br_flag,k,n_ra_prb);
      write_output("rxsigF_br.m","prach_rxF_br",&rxsigF[0][0],12288,1,1);
      write_output("prach_rxF_comp0_br.m","prach_rxF_comp0_br",prachF,1024,1,1);
      write_output("Xu_br.m","xu_br",Xu,N_ZC,1,1);
      write_output("prach_ifft0_br.m","prach_t0_br",prach_ifft,1024,1,1);
      exit(-1);      
#endif
    }

  }
#endif
  if (eNB) stop_meas(&eNB->rx_prach);

}



#ifdef Rel14

void rx_prach(PHY_VARS_eNB *eNB,
	      RU_t *ru,
	      uint16_t *max_preamble,
	      uint16_t *max_preamble_energy,
	      uint16_t *max_preamble_delay,
	      uint16_t Nf, 
	      uint8_t tdd_mapindex,
	      uint8_t br_flag) {

  int i;
  int prach_mask=0;

  if (br_flag == 0) { 
    rx_prach0(eNB,ru,max_preamble,max_preamble_energy,max_preamble_delay,Nf,tdd_mapindex,0,0);
  }
  else { // This is procedure for eMTC, basically handling the repetitions
    prach_mask = is_prach_subframe(&eNB->frame_parms,eNB->proc.frame_prach_br,eNB->proc.subframe_prach_br);
    for (i=0;i<4;i++) {
      if ((eNB->frame_parms.prach_emtc_config_common.prach_ConfigInfo.prach_CElevel_enable[i]==1) &&
	  ((prach_mask&(1<<(i+1))) > 0)) { // check that prach CE level is active now

	// if first reception in group of repetitions store frame for later (in RA-RNTI for Msg2) 
	if (eNB->prach_vars_br.repetition_number[i]==0) eNB->prach_vars_br.first_frame[i]=eNB->proc.frame_prach_br;

	// increment repetition number
	eNB->prach_vars_br.repetition_number[i]++;

	// do basic PRACH reception
	rx_prach0(eNB,ru,max_preamble,max_preamble_energy,max_preamble_delay,Nf,tdd_mapindex,1,i);
	
	// if last repetition, clear counter
	if (eNB->prach_vars_br.repetition_number[i] == eNB->frame_parms.prach_emtc_config_common.prach_ConfigInfo.prach_numRepetitionPerPreambleAttempt[i]) {
	  eNB->prach_vars_br.repetition_number[i]=0;

	}
      }
    }
  }
}

#endif /* Rel14 */

void init_prach_tables(int N_ZC)
{

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

void compute_prach_seq(uint16_t rootSequenceIndex,
		       uint8_t prach_ConfigIndex,
		       uint8_t zeroCorrelationZoneConfig,
		       uint8_t highSpeedFlag,
		       lte_frame_type_t frame_type,
		       uint32_t X_u[64][839])
{

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
      X_u[i][k] = ((uint32_t*)ru)[(((k*(1+(inv_u*k)))%N_ZC)*420)%N_ZC];
    }
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_PHY_UE_COMPUTE_PRACH, VCD_FUNCTION_OUT);

}
