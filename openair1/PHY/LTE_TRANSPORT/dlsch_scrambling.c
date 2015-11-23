/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@lists.eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

/*! \file PHY/LTE_TRANSPORT/dlsch_scrambling.c
* \brief Routines for the scrambling procedure of the PDSCH physical channel from 36-211, V8.6 2009-03
* \author R. Knopp
* \date 2011
* \version 0.1
* \company Eurecom
* \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr
* \note
* \warning
*/

//#define DEBUG_SCRAMBLING 1

#include "PHY/defs.h"
#include "PHY/CODING/extern.h"
#include "PHY/CODING/lte_interleaver_inline.h"
#include "defs.h"
#include "extern.h"
#include "PHY/extern.h"
#include "UTIL/LOG/vcd_signal_dumper.h"

void dlsch_scrambling(LTE_DL_FRAME_PARMS *frame_parms,
                      int mbsfn_flag,
                      LTE_eNB_DLSCH_t *dlsch,
                      int G,
                      uint8_t q,
                      uint8_t Ns)
{

  int i,j,k=0;
  //  uint8_t reset;
  uint32_t x1, x2, s=0;
  uint8_t *e=dlsch->harq_processes[dlsch->current_harq_pid]->e;

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_SCRAMBLING, VCD_FUNCTION_IN);

  //  reset = 1;
  // x1 is set in lte_gold_generic
  if (mbsfn_flag == 0) {
    x2 = (dlsch->rnti<<14) + (q<<13) + ((Ns>>1)<<9) + frame_parms->Nid_cell; //this is c_init in 36.211 Sec 6.3.1
  } else {
    x2 = ((Ns>>1)<<9) + frame_parms->Nid_cell_mbsfn; //this is c_init in 36.211 Sec 6.3.1
  }

#ifdef DEBUG_SCRAMBLING
  printf("scrambling: rnti %x, q %d, Ns %d, Nid_cell %d, length %d\n",dlsch->rnti,q,Ns,frame_parms->Nid_cell, G);
#endif
  s = lte_gold_generic(&x1, &x2, 1);

  for (i=0; i<(1+(G>>5)); i++) {

    for (j=0; j<32; j++,k++) {
#ifdef DEBUG_SCRAMBLING
      printf("scrambling %d : %d => ",k,e[k]);
#endif
      e[k] = (e[k]&1) ^ ((s>>j)&1);
#ifdef DEBUG_SCRAMBLING
      printf("%d\n",e[k]);
#endif
    }

    s = lte_gold_generic(&x1, &x2, 0);
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_ENB_DLSCH_SCRAMBLING, VCD_FUNCTION_OUT);

}


void dlsch_unscrambling(LTE_DL_FRAME_PARMS *frame_parms,
                        int mbsfn_flag,
                        LTE_UE_DLSCH_t *dlsch,
                        int G,
                        int16_t* llr,
                        uint8_t q,
                        uint8_t Ns)
{

  int i,j,k=0;
  //  uint8_t reset;
  uint32_t x1, x2, s=0;

  //  reset = 1;
  // x1 is set in first call to lte_gold_generic

  if (mbsfn_flag == 0)
    x2 = (dlsch->rnti<<14) + (q<<13) + ((Ns>>1)<<9) + frame_parms->Nid_cell; //this is c_init in 36.211 Sec 6.3.1
  else
    x2 = ((Ns>>1)<<9) + frame_parms->Nid_cell_mbsfn; //this is c_init in 36.211 Sec 6.3.1

#ifdef DEBUG_SCRAMBLING
  printf("unscrambling: rnti %x, q %d, Ns %d, Nid_cell %d length %d\n",dlsch->rnti,q,Ns,frame_parms->Nid_cell,G);
#endif
  s = lte_gold_generic(&x1, &x2, 1);

  for (i=0; i<(1+(G>>5)); i++) {
    for (j=0; j<32; j++,k++) {
#ifdef DEBUG_SCRAMBLING
      printf("unscrambling %d : %d => ",k,llr[k]);
#endif
      llr[k] = ((2*((s>>j)&1))-1)*llr[k];
#ifdef DEBUG_SCRAMBLING
      printf("%d\n",llr[k]);
#endif
    }

    s = lte_gold_generic(&x1, &x2, 0);
  }
}
