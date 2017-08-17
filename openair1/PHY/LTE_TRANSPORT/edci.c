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

/*! \file PHY/LTE_TRANSPORT/dci.c
* \brief Implements PDCCH physical channel TX/RX procedures (36.211) and DCI encoding/decoding (36.212/36.213). Current LTE compliance V8.6 2009-03.
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr
* \note
* \warning
*/
#ifdef USER_MODE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif
#include "PHY/defs.h"
#include "PHY/extern.h"
#include "SCHED/defs.h"
#include "SIMULATION/TOOLS/defs.h" // for taus 
#include "PHY/sse_intrin.h"

#include "assertions.h" 
#include "T.h"
#include "UTIL/LOG/log.h"

//#define DEBUG_DCI_ENCODING 1
//#define DEBUG_DCI_DECODING 1
//#define DEBUG_PHY

typedef struct {
  uint8_t     num_dci;
  mDCI_ALLOC_t mdci_alloc[32];
} LTE_eNB_MPDCCH;

typedef struct {
  /// Length of DCI in bits
  uint8_t dci_length;
  /// Aggregation level
  uint8_t L;
  /// Position of first CCE of the dci
  int firstCCE;
  /// flag to indicate that this is a RA response
  boolean_t ra_flag;
  /// rnti
  rnti_t rnti;
  /// Format
  DCI_format_t format;
  /// harq process index
  uint8_t harq_pid;
  /// Narrowband index
  uint8_t narrowband;
  /// number of PRB pairs for MPDCCH
  uint8_t number_of_prb_pairs;
  /// mpdcch resource assignement (0=localized,1=distributed) 
  uint8_t resource_block_assignment;
  /// transmission type
  uint8_t transmission_type;
  /// mpdcch start symbol
  uint8_t start_symbol;
  /// CE mode (1=ModeA,2=ModeB)
  uint8_t ce_mode;
  /// 0-503 n_EPDCCHid_i
  uint16_t dmrs_scrambling_init;
  /// Absolute subframe of the initial transmission (0-10239)
  uint16_t initial_transmission_sf_io;
  /// DCI pdu
  uint8_t dci_pdu[8];
} mDCI_ALLOC_t;

void generate_edci_top(PHY_VARS_eNB *eNB, int frame, int subframe) {

}

void generate_mdci_top(PHY_VARS_eNB *eNB, int frame, int subframe) {

  LTE_eNB_MPDCCH *mpdcch= &eNB->mpdcch_vars[subframe&2];
  mDCI_ALLOC_t *mdci;
  uint8_t e[DCI_BITS_MAX];

  for (i=0;i<mpdcch->num_dci;i++) {
    mdci = &mdpcch->mdci_alloc[i];

    // Assume 4 EREG/ECCE case (normal prefix)
    AssertFatal(eNB->frame_parms.frame_type==FDD,"TDD is not yet supported for MPDCCH\n");

    // 9 REs/EREG => 36 REs/ECCE => 72 bits/ECCE
    generate_dci0(mdci->dci_pdu,
		  e+(72*mdci->firstCCE),
		  mdci_length,
		  mdci->L,
		  mdci->rnti);

    // compute MPDCCH format based on parameters from NFAPI
    AssertFatal(mdci->reps>0,"mdci->reps==0\n");
    if (reps==0) { // table 6.8A.1-1
      AssertFatal(eNB->frame_parms->Ncp == NORMAL,"Extended Prefix not yet supported for MPDCCH\n");
      N_ECCE_EREG = 4;
    }
  
  }
} 
