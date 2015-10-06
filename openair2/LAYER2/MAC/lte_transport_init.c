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

#define UL_RB_ALLOC mac_xface->computeRIV(mac_xface->lte_frame_parms->N_RB_UL,0,24)
#define BCCH_RB_ALLOC mac_xface->computeRIV(mac_xface->lte_frame_parms->N_RB_UL,0,4)
#define RA_RB_ALLOC mac_xface->computeRIV(mac_xface->lte_frame_parms->N_RB_UL,0,4)
#define DLSCH_RB_ALLOC 0x1fff
#include "extern.h"


void init_transport_channels(unsigned char transmission_mode)
{

  // init DCI structures for testing

  UL_alloc_pdu.type    = 0;
  UL_alloc_pdu.hopping = 0;
  UL_alloc_pdu.rballoc = UL_RB_ALLOC;
  UL_alloc_pdu.mcs     = 2;
  UL_alloc_pdu.ndi     = 1;
  UL_alloc_pdu.TPC     = 0;
  UL_alloc_pdu.cqi_req = 1;

  /*
  BCCH_alloc_pdu.type               = 1;
  BCCH_alloc_pdu.vrb_type           = 0;
  BCCH_alloc_pdu.rballoc            = BCCH_RB_ALLOC;
  BCCH_alloc_pdu.ndi      = 1;
  BCCH_alloc_pdu.rv       = 1;
  BCCH_alloc_pdu.mcs      = 1;
  BCCH_alloc_pdu.harq_pid = 0;
  BCCH_alloc_pdu.TPC      = 1;      // set to 3 PRB

  // for FDD mode
  BCCH_alloc_pdu_fdd.type               = 1;
  BCCH_alloc_pdu_fdd.vrb_type           = 0;
  BCCH_alloc_pdu_fdd.rballoc            = BCCH_RB_ALLOC;
  BCCH_alloc_pdu_fdd.ndi      = 1;
  BCCH_alloc_pdu_fdd.rv       = 1;
  BCCH_alloc_pdu_fdd.mcs      = 1;
  BCCH_alloc_pdu_fdd.harq_pid = 0;
  BCCH_alloc_pdu_fdd.TPC      = 1;      // set to 3 PRB
  */

  DLSCH_alloc_pdu1A.type               = 1;
  DLSCH_alloc_pdu1A.vrb_type           = 0;
  DLSCH_alloc_pdu1A.rballoc            = BCCH_RB_ALLOC;
  DLSCH_alloc_pdu1A.ndi      = 1;
  DLSCH_alloc_pdu1A.rv       = 1;
  DLSCH_alloc_pdu1A.mcs      = 2;
  DLSCH_alloc_pdu1A.harq_pid = 0;
  DLSCH_alloc_pdu1A.TPC      = 1;   // set to 3 PRB

  DLSCH_alloc_pdu1A_fdd.type               = 1;
  DLSCH_alloc_pdu1A_fdd.vrb_type           = 0;
  DLSCH_alloc_pdu1A_fdd.rballoc            = BCCH_RB_ALLOC;
  DLSCH_alloc_pdu1A_fdd.ndi      = 1;
  DLSCH_alloc_pdu1A_fdd.rv       = 1;
  DLSCH_alloc_pdu1A_fdd.mcs      = 2;
  DLSCH_alloc_pdu1A_fdd.harq_pid = 0;
  DLSCH_alloc_pdu1A_fdd.TPC      = 1;   // set to 3 PRB

  RA_alloc_pdu.type               = 1;
  RA_alloc_pdu.vrb_type           = 0;
  RA_alloc_pdu.rballoc            = RA_RB_ALLOC;
  RA_alloc_pdu.ndi      = 1;
  RA_alloc_pdu.rv       = 0;
  RA_alloc_pdu.mcs      = 0;
  RA_alloc_pdu.harq_pid = 0;
  RA_alloc_pdu.TPC      = 1;

  RA_alloc_pdu_fdd.type               = 1;
  RA_alloc_pdu_fdd.vrb_type           = 0;
  RA_alloc_pdu_fdd.rballoc            = RA_RB_ALLOC;
  RA_alloc_pdu_fdd.ndi      = 1;
  RA_alloc_pdu_fdd.rv       = 1;
  RA_alloc_pdu_fdd.mcs      = 1;
  RA_alloc_pdu_fdd.harq_pid = 0;
  RA_alloc_pdu_fdd.TPC      = 1;


  DLSCH_alloc_pdu1.rballoc          = 0xf;
  DLSCH_alloc_pdu1.TPC              = 0;
  DLSCH_alloc_pdu1.dai              = 0;
  DLSCH_alloc_pdu1.harq_pid         = 0;
  DLSCH_alloc_pdu1.tb_swap          = 0;
  DLSCH_alloc_pdu1.mcs1             = 4;
  DLSCH_alloc_pdu1.ndi1             = 1;
  DLSCH_alloc_pdu1.rv1              = 0;

  // Forget second codeword
  if (transmission_mode == 6) {
    DLSCH_alloc_pdu1.tpmi           = 5;  // PUSCH_PRECODING0
  } else {
    DLSCH_alloc_pdu1.tpmi             = 0;
  }

  DLSCH_alloc_pdu2.rah              = 0;
  DLSCH_alloc_pdu2.rballoc          = DLSCH_RB_ALLOC;
  DLSCH_alloc_pdu2.TPC              = 0;
  DLSCH_alloc_pdu2.dai              = 0;
  DLSCH_alloc_pdu2.harq_pid         = 0;
  DLSCH_alloc_pdu2.tb_swap          = 0;
  DLSCH_alloc_pdu2.mcs1             = 4;
  DLSCH_alloc_pdu2.ndi1             = 1;
  DLSCH_alloc_pdu2.rv1              = 0;

  // Forget second codeword
  if (transmission_mode == 6) {
    DLSCH_alloc_pdu2.tpmi           = 5;  // PUSCH_PRECODING0
  } else {
    DLSCH_alloc_pdu2.tpmi             = 0;
  }
}
