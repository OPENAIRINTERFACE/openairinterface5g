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

/*! \file phy_mac_stub.c
 * \brief stimulates the phy without mac
 * \author R. Knopp, F. Kaltenberger, N. Nikaein
 * \date 2011
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr,florian.kaltenberger@eurecom.fr,navid.nikaein@eurecom.fr
 * \note
 * \warning
 */

#include "PHY/defs.h"
#include "PHY/extern.h"
#include "SCHED/defs.h"
#include "SCHED/extern.h"
#include "LAYER2/MAC/extern.h"

#ifdef EMOS
#include "SCHED/phy_procedures_emos.h"
#endif

void fill_dci(DCI_PDU *DCI_pdu, uint8_t sched_subframe, PHY_VARS_eNB *phy_vars_eNB)
{

  //uint8_t cooperation_flag = phy_vars_eNB->cooperation_flag;
  uint8_t transmission_mode = phy_vars_eNB->transmission_mode[0];

  uint32_t rballoc = 0x7FFF;
  //uint32_t rballoc2 = 0x000F;
  uint32_t rballoc_test = 0xFFFF;
  int subframe = phy_vars_eNB->proc[sched_subframe].subframe_tx;

  LTE_eNB_DLSCH_t *DLSCH_ptr = phy_vars_eNB->dlsch_eNB[0][0];

  /*
    uint32_t rand = taus();
    if ((subframe==8) || (subframe==9) || (subframe==0))
    rand = (rand%5)+5;
    else
    rand = (rand%4)+5;
  */
  uint32_t bcch_pdu;
  uint64_t dlsch_pdu;

  DCI_pdu->Num_common_dci = 0;
  DCI_pdu->Num_ue_spec_dci=0;

  switch (subframe) {
  case 5:
    DCI_pdu->Num_common_dci = 1;
    DCI_pdu->dci_alloc[0].L          = 2;
    DCI_pdu->dci_alloc[0].rnti       = SI_RNTI;
    DCI_pdu->dci_alloc[0].format     = format1A;
    DCI_pdu->dci_alloc[0].ra_flag    = 0;

    switch (phy_vars_eNB->lte_frame_parms.N_RB_DL) {
    case 6:
      if (phy_vars_eNB->lte_frame_parms.frame_type == FDD) {
        DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1A_1_5MHz_FDD_t;
        ((DCI1A_1_5MHz_FDD_t*)&bcch_pdu)->type              = 1;
        ((DCI1A_1_5MHz_FDD_t*)&bcch_pdu)->vrb_type          = 0;
        ((DCI1A_1_5MHz_FDD_t*)&bcch_pdu)->rballoc           = computeRIV(25,10,3);
        ((DCI1A_1_5MHz_FDD_t*)&bcch_pdu)->ndi               = phy_vars_eNB->proc[sched_subframe].frame_tx&1;
        ((DCI1A_1_5MHz_FDD_t*)&bcch_pdu)->rv                = 1;
        ((DCI1A_1_5MHz_FDD_t*)&bcch_pdu)->mcs               = 1;
        ((DCI1A_1_5MHz_FDD_t*)&bcch_pdu)->harq_pid          = 0;
        ((DCI1A_1_5MHz_FDD_t*)&bcch_pdu)->TPC               = 1;      // set to 3 PRB
        memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],&bcch_pdu,sizeof(DCI1A_1_5MHz_TDD_1_6_t));
      } else {
        DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1A_1_5MHz_TDD_1_6_t;
        ((DCI1A_1_5MHz_TDD_1_6_t*)&bcch_pdu)->type              = 1;
        ((DCI1A_1_5MHz_TDD_1_6_t*)&bcch_pdu)->vrb_type          = 0;
        ((DCI1A_1_5MHz_TDD_1_6_t*)&bcch_pdu)->rballoc           = computeRIV(25,10,3);
        ((DCI1A_1_5MHz_TDD_1_6_t*)&bcch_pdu)->ndi               = phy_vars_eNB->proc[sched_subframe].frame_tx&1;
        ((DCI1A_1_5MHz_TDD_1_6_t*)&bcch_pdu)->rv                = 1;
        ((DCI1A_1_5MHz_TDD_1_6_t*)&bcch_pdu)->mcs               = 1;
        ((DCI1A_1_5MHz_TDD_1_6_t*)&bcch_pdu)->harq_pid          = 0;
        ((DCI1A_1_5MHz_TDD_1_6_t*)&bcch_pdu)->TPC               = 1;      // set to 3 PRB
        memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],&bcch_pdu,sizeof(DCI1A_1_5MHz_TDD_1_6_t));
      }

      break;

    case 25:
    default:
      if (phy_vars_eNB->lte_frame_parms.frame_type == FDD) {
        DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1A_5MHz_FDD_t;
        ((DCI1A_5MHz_FDD_t*)&bcch_pdu)->type              = 1;
        ((DCI1A_5MHz_FDD_t*)&bcch_pdu)->vrb_type          = 0;
        ((DCI1A_5MHz_FDD_t*)&bcch_pdu)->rballoc           = computeRIV(25,10,3);
        ((DCI1A_5MHz_FDD_t*)&bcch_pdu)->ndi               = phy_vars_eNB->proc[sched_subframe].frame_tx&1;
        ((DCI1A_5MHz_FDD_t*)&bcch_pdu)->rv                = 1;
        ((DCI1A_5MHz_FDD_t*)&bcch_pdu)->mcs               = 1;
        ((DCI1A_5MHz_FDD_t*)&bcch_pdu)->harq_pid          = 0;
        ((DCI1A_5MHz_FDD_t*)&bcch_pdu)->TPC               = 1;      // set to 3 PRB
        memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],&bcch_pdu,sizeof(DCI1A_5MHz_TDD_1_6_t));
      } else {
        DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1A_5MHz_TDD_1_6_t;
        ((DCI1A_5MHz_TDD_1_6_t*)&bcch_pdu)->type              = 1;
        ((DCI1A_5MHz_TDD_1_6_t*)&bcch_pdu)->vrb_type          = 0;
        ((DCI1A_5MHz_TDD_1_6_t*)&bcch_pdu)->rballoc           = computeRIV(25,10,3);
        ((DCI1A_5MHz_TDD_1_6_t*)&bcch_pdu)->ndi               = phy_vars_eNB->proc[sched_subframe].frame_tx&1;
        ((DCI1A_5MHz_TDD_1_6_t*)&bcch_pdu)->rv                = 1;
        ((DCI1A_5MHz_TDD_1_6_t*)&bcch_pdu)->mcs               = 1;
        ((DCI1A_5MHz_TDD_1_6_t*)&bcch_pdu)->harq_pid          = 0;
        ((DCI1A_5MHz_TDD_1_6_t*)&bcch_pdu)->TPC               = 1;      // set to 3 PRB
        memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],&bcch_pdu,sizeof(DCI1A_5MHz_TDD_1_6_t));
      }

      break;

    case 50:
      if (phy_vars_eNB->lte_frame_parms.frame_type == FDD) {
        DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1A_10MHz_FDD_t;
        ((DCI1A_10MHz_FDD_t*)&bcch_pdu)->type              = 1;
        ((DCI1A_10MHz_FDD_t*)&bcch_pdu)->vrb_type          = 0;
        ((DCI1A_10MHz_FDD_t*)&bcch_pdu)->rballoc           = computeRIV(50,10,3);
        ((DCI1A_10MHz_FDD_t*)&bcch_pdu)->ndi               = phy_vars_eNB->proc[sched_subframe].frame_tx&1;
        ((DCI1A_10MHz_FDD_t*)&bcch_pdu)->rv                = 1;
        ((DCI1A_10MHz_FDD_t*)&bcch_pdu)->mcs               = 1;
        ((DCI1A_10MHz_FDD_t*)&bcch_pdu)->harq_pid          = 0;
        ((DCI1A_10MHz_FDD_t*)&bcch_pdu)->TPC               = 1;      // set to 3 PRB
        memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],&bcch_pdu,sizeof(DCI1A_10MHz_TDD_1_6_t));
      } else {
        DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1A_10MHz_TDD_1_6_t;
        ((DCI1A_10MHz_TDD_1_6_t*)&bcch_pdu)->type              = 1;
        ((DCI1A_10MHz_TDD_1_6_t*)&bcch_pdu)->vrb_type          = 0;
        ((DCI1A_10MHz_TDD_1_6_t*)&bcch_pdu)->rballoc           = computeRIV(50,10,3);
        ((DCI1A_10MHz_TDD_1_6_t*)&bcch_pdu)->ndi               = subframe / 5;
        ((DCI1A_10MHz_TDD_1_6_t*)&bcch_pdu)->rv                = 1;
        ((DCI1A_10MHz_TDD_1_6_t*)&bcch_pdu)->mcs               = 1;
        ((DCI1A_10MHz_TDD_1_6_t*)&bcch_pdu)->harq_pid          = subframe % 5;
        ((DCI1A_10MHz_TDD_1_6_t*)&bcch_pdu)->TPC               = 1;      // set to 3 PRB
        memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],&bcch_pdu,sizeof(DCI1A_10MHz_TDD_1_6_t));
      }

      break;

    case 100:
      if (phy_vars_eNB->lte_frame_parms.frame_type == FDD) {
        DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1A_20MHz_FDD_t;
        ((DCI1A_20MHz_FDD_t*)&bcch_pdu)->type              = 1;
        ((DCI1A_20MHz_FDD_t*)&bcch_pdu)->vrb_type          = 0;
        ((DCI1A_20MHz_FDD_t*)&bcch_pdu)->rballoc           = computeRIV(100,10,3);
        ((DCI1A_20MHz_FDD_t*)&bcch_pdu)->ndi               = phy_vars_eNB->proc[sched_subframe].frame_tx&1;
        ((DCI1A_20MHz_FDD_t*)&bcch_pdu)->rv                = 1;
        ((DCI1A_20MHz_FDD_t*)&bcch_pdu)->mcs               = 1;
        ((DCI1A_20MHz_FDD_t*)&bcch_pdu)->harq_pid          = 0;
        ((DCI1A_20MHz_FDD_t*)&bcch_pdu)->TPC               = 1;      // set to 3 PRB
        memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],&bcch_pdu,sizeof(DCI1A_20MHz_TDD_1_6_t));
      } else {
        DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1A_20MHz_TDD_1_6_t;
        ((DCI1A_20MHz_TDD_1_6_t*)&bcch_pdu)->type              = 1;
        ((DCI1A_20MHz_TDD_1_6_t*)&bcch_pdu)->vrb_type          = 0;
        ((DCI1A_20MHz_TDD_1_6_t*)&bcch_pdu)->rballoc           = computeRIV(100,10,3);
        ((DCI1A_20MHz_TDD_1_6_t*)&bcch_pdu)->ndi               = phy_vars_eNB->proc[sched_subframe].frame_tx&1;
        ((DCI1A_20MHz_TDD_1_6_t*)&bcch_pdu)->rv                = 1;
        ((DCI1A_20MHz_TDD_1_6_t*)&bcch_pdu)->mcs               = 1;
        ((DCI1A_20MHz_TDD_1_6_t*)&bcch_pdu)->harq_pid          = 0;
        ((DCI1A_20MHz_TDD_1_6_t*)&bcch_pdu)->TPC               = 1;      // set to 3 PRB
        memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],&bcch_pdu,sizeof(DCI1A_20MHz_TDD_1_6_t));
      }

      break;
    }
    break; //subframe switch

    /*
  case 6:
      DCI_pdu->Num_ue_spec_dci = 1;
      DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI2_5MHz_2A_M10PRB_TDD_t;
      DCI_pdu->dci_alloc[0].L          = 2;
      DCI_pdu->dci_alloc[0].rnti       = 0x1236;
      DCI_pdu->dci_alloc[0].format     = format2_2A_M10PRB;
      DCI_pdu->dci_alloc[0].ra_flag    = 0;

      DLSCH_alloc_pdu1.rballoc          = 0x00ff;
      DLSCH_alloc_pdu1.TPC              = 0;
      DLSCH_alloc_pdu1.dai              = 0;
      DLSCH_alloc_pdu1.harq_pid         = 0;
      DLSCH_alloc_pdu1.tb_swap          = 0;
      DLSCH_alloc_pdu1.mcs1             = 0;
      DLSCH_alloc_pdu1.ndi1             = 1;
      DLSCH_alloc_pdu1.rv1              = 0;
      DLSCH_alloc_pdu1.tpmi             = 0;
      memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],(void *)&DLSCH_alloc_pdu1,sizeof(DCI2_5MHz_2A_M10PRB_TDD_t));
    break;
    */

  default:
  case 7:
    DCI_pdu->Num_ue_spec_dci = 1;
    DCI_pdu->dci_alloc[0].L          = 2;
    DCI_pdu->dci_alloc[0].rnti       = 0x1235;
    DCI_pdu->dci_alloc[0].format     = format1;
    DCI_pdu->dci_alloc[0].ra_flag    = 0;

    if (transmission_mode<3) {
      //user 1
      switch (phy_vars_eNB->lte_frame_parms.N_RB_DL) {
      case 25:
        if (phy_vars_eNB->lte_frame_parms.frame_type == FDD) {
          DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1_5MHz_FDD_t;

          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->rballoc          = rballoc; //computeRIV(25,10,3);
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->TPC              = 0;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->harq_pid         = subframe % 5;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->mcs              = openair_daq_vars.target_ue_dl_mcs;
          //((DCI1_5MHz_FDD_t *)&dlsch_pdu)->mcs            = (unsigned char) ((phy_vars_eNB->frame%1024)%28);
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->ndi              = subframe / 5;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->rv               = 0;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->rah              = 0;

          memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],(void *)&dlsch_pdu,sizeof(DCI1_5MHz_FDD_t));

          /*
          //user2
          DCI_pdu->dci_alloc[1].dci_length = sizeof_DCI1_5MHz_TDD_t;
          DCI_pdu->dci_alloc[1].L          = 2;
          DCI_pdu->dci_alloc[1].rnti       = 0x1236;
          DCI_pdu->dci_alloc[1].format     = format1;
          DCI_pdu->dci_alloc[1].ra_flag    = 0;

          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->rballoc          = rballoc2;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->TPC              = 0;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->dai              = 0;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->harq_pid         = 1;
          //((DCI1_5MHz_FDD_t *)&dlsch_pdu)->mcs              = (unsigned char) ((phy_vars_eNB->proc[subframe].frame%1024)%28);
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->mcs              = openair_daq_vars.target_ue_dl_mcs;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->ndi              = 1;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->rv               = 0;
          memcpy((void*)&DCI_pdu->dci_alloc[1].dci_pdu[0],(void *)&((DCI1_5MHz_FDD_t *)&dlsch_pdu)->,sizeof(DCI1_5MHz_TDD_t));
          */
        } else {
          DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1_5MHz_TDD_t;

          ((DCI1_5MHz_TDD_t *)&dlsch_pdu)->rballoc          = rballoc; //computeRIV(25,10,3);
          ((DCI1_5MHz_TDD_t *)&dlsch_pdu)->TPC              = 0;
          ((DCI1_5MHz_TDD_t *)&dlsch_pdu)->dai              = 0;
          ((DCI1_5MHz_TDD_t *)&dlsch_pdu)->harq_pid         = subframe % 5;
          ((DCI1_5MHz_TDD_t *)&dlsch_pdu)->mcs              = openair_daq_vars.target_ue_dl_mcs;
          //((DCI1_5MHz_TDD_t *)&dlsch_pdu)->mcs            = (unsigned char) ((phy_vars_eNB->frame%1024)%28);
          ((DCI1_5MHz_TDD_t *)&dlsch_pdu)->ndi              = subframe / 5;
          ((DCI1_5MHz_TDD_t *)&dlsch_pdu)->rv               = 0;
          ((DCI1_5MHz_TDD_t *)&dlsch_pdu)->rah              = 0;
          memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],(void *)&dlsch_pdu,sizeof(DCI1_5MHz_TDD_t));

          /*
          //user2
          DCI_pdu->dci_alloc[1].dci_length = sizeof_DCI1_5MHz_TDD_t;
          DCI_pdu->dci_alloc[1].L          = 2;
          DCI_pdu->dci_alloc[1].rnti       = 0x1236;
          DCI_pdu->dci_alloc[1].format     = format1;
          DCI_pdu->dci_alloc[1].ra_flag    = 0;

          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->rballoc          = rballoc2;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->TPC              = 0;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->dai              = 0;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->harq_pid         = 1;
          //((DCI1_5MHz_FDD_t *)&dlsch_pdu)->mcs              = (unsigned char) ((phy_vars_eNB->proc[subframe].frame%1024)%28);
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->mcs              = openair_daq_vars.target_ue_dl_mcs;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->ndi              = 1;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->rv               = 0;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->rah              = 0;
          memcpy((void*)&DCI_pdu->dci_alloc[1].dci_pdu[0],(void *)&((DCI1_5MHz_FDD_t *)&dlsch_pdu)->,sizeof(DCI1_5MHz_TDD_t));
          */
        }

        break;

      case 50:

        if (phy_vars_eNB->lte_frame_parms.frame_type == FDD) {
          DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1_10MHz_FDD_t;

          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->rballoc          = rballoc; //computeRIV(50,10,3);
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->TPC              = 0;
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->harq_pid         = subframe % 5;
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->mcs              = openair_daq_vars.target_ue_dl_mcs;
          //((DCI1_10MHz_FDD_t *)&dlsch_pdu)->mcs            = (unsigned char) ((phy_vars_eNB->frame%1024)%28);
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->ndi              = subframe / 5;
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->rv               = 0;
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->rah              = 0;

          memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],(void *)&dlsch_pdu,sizeof(DCI1_10MHz_FDD_t));

          /*
          //user2
          DCI_pdu->dci_alloc[1].dci_length = sizeof_DCI1_10MHz_TDD_t;
          DCI_pdu->dci_alloc[1].L          = 2;
          DCI_pdu->dci_alloc[1].rnti       = 0x1236;
          DCI_pdu->dci_alloc[1].format     = format1;
          DCI_pdu->dci_alloc[1].ra_flag    = 0;

          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->rballoc          = rballoc2;
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->TPC              = 0;
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->dai              = 0;
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->harq_pid         = 1;
          //((DCI1_10MHz_FDD_t *)&dlsch_pdu)->mcs              = (unsigned char) ((phy_vars_eNB->proc[subframe].frame%1024)%28);
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->mcs              = openair_daq_vars.target_ue_dl_mcs;
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->ndi              = 1;
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->rv               = 0;
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->rah              = 0;
          memcpy((void*)&DCI_pdu->dci_alloc[1].dci_pdu[0],(void *)&((DCI1_10MHz_FDD_t *)&dlsch_pdu)->,sizeof(DCI1_10MHz_TDD_t));
          */
        } else {
          DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1_10MHz_TDD_t;

          ((DCI1_10MHz_TDD_t *)&dlsch_pdu)->rballoc          = rballoc; //computeRIV(50,10,3);
          ((DCI1_10MHz_TDD_t *)&dlsch_pdu)->TPC              = 0;
          ((DCI1_10MHz_TDD_t *)&dlsch_pdu)->dai              = 0;
          ((DCI1_10MHz_TDD_t *)&dlsch_pdu)->harq_pid         = subframe % 5;
          ((DCI1_10MHz_TDD_t *)&dlsch_pdu)->mcs              = openair_daq_vars.target_ue_dl_mcs;
          //((DCI1_10MHz_TDD_t *)&dlsch_pdu)->mcs            = (unsigned char) ((phy_vars_eNB->frame%1024)%28);
          ((DCI1_10MHz_TDD_t *)&dlsch_pdu)->ndi              = subframe / 5;
          ((DCI1_10MHz_TDD_t *)&dlsch_pdu)->rv               = 0;
          ((DCI1_10MHz_TDD_t *)&dlsch_pdu)->rah              = 0;
          memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],(void *)&dlsch_pdu,sizeof(DCI1_10MHz_TDD_t));

          /*
          //user2
          DCI_pdu->dci_alloc[1].dci_length = sizeof_DCI1_10MHz_TDD_t;
          DCI_pdu->dci_alloc[1].L          = 2;
          DCI_pdu->dci_alloc[1].rnti       = 0x1236;
          DCI_pdu->dci_alloc[1].format     = format1;
          DCI_pdu->dci_alloc[1].ra_flag    = 0;

          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->rballoc          = rballoc2;
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->TPC              = 0;
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->dai              = 0;
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->harq_pid         = 1;
          //((DCI1_10MHz_FDD_t *)&dlsch_pdu)->mcs              = (unsigned char) ((phy_vars_eNB->proc[subframe].frame%1024)%28);
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->mcs              = openair_daq_vars.target_ue_dl_mcs;
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->ndi              = 1;
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->rv               = 0;
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->rah              = 0;
          memcpy((void*)&DCI_pdu->dci_alloc[1].dci_pdu[0],(void *)&((DCI1_10MHz_FDD_t *)&dlsch_pdu)->,sizeof(DCI1_10MHz_TDD_t));
          */
        }

        break;

      case 100:
        if (phy_vars_eNB->lte_frame_parms.frame_type == FDD) {
          DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1_20MHz_FDD_t;

          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->rballoc          = rballoc; //computeRIV(100,10,3);
          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->TPC              = 0;
          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->harq_pid         = subframe % 5;
          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->mcs              = openair_daq_vars.target_ue_dl_mcs;
          //((DCI1_5MHz_FDD_t *)&dlsch_pdu)->mcs             = (unsigned char) ((phy_vars_eNB->frame%1024)%28);
          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->ndi              = subframe / 5;
          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->rv               = 0;
          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->rah              = 0;

          memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],(void *)&dlsch_pdu,sizeof(DCI1_20MHz_FDD_t));

          /*
          //user2
          DCI_pdu->dci_alloc[1].dci_length = sizeof_DCI1_5MHz_TDD_t;
          DCI_pdu->dci_alloc[1].L          = 2;
          DCI_pdu->dci_alloc[1].rnti       = 0x1236;
          DCI_pdu->dci_alloc[1].format     = format1;
          DCI_pdu->dci_alloc[1].ra_flag    = 0;

          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->rballoc          = rballoc2;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->TPC              = 0;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->dai              = 0;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->harq_pid         = 1;
          //((DCI1_5MHz_FDD_t *)&dlsch_pdu)->mcs              = (unsigned char) ((phy_vars_eNB->proc[subframe].frame%1024)%28);
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->mcs              = openair_daq_vars.target_ue_dl_mcs;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->ndi              = 1;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->rv               = 0;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->rah              = 0;
          memcpy((void*)&DCI_pdu->dci_alloc[1].dci_pdu[0],(void *)&((DCI1_5MHz_FDD_t *)&dlsch_pdu)->,sizeof(DCI1_5MHz_TDD_t));
          */
        } else {
          DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1_20MHz_TDD_t;

          ((DCI1_20MHz_TDD_t *)&dlsch_pdu)->rballoc          = rballoc; //computeRIV(100,10,3);
          ((DCI1_20MHz_TDD_t *)&dlsch_pdu)->TPC              = 0;
          ((DCI1_20MHz_TDD_t *)&dlsch_pdu)->dai              = 0;
          ((DCI1_20MHz_TDD_t *)&dlsch_pdu)->harq_pid         = subframe % 5;
          ((DCI1_20MHz_TDD_t *)&dlsch_pdu)->mcs              = openair_daq_vars.target_ue_dl_mcs;
          //((DCI1_20MHz_TDD_t *)&dlsch_pdu)->mcs            = (unsigned char) ((phy_vars_eNB->frame%1024)%28);
          ((DCI1_20MHz_TDD_t *)&dlsch_pdu)->ndi              = subframe / 5;
          ((DCI1_20MHz_TDD_t *)&dlsch_pdu)->rv               = 0;
          ((DCI1_20MHz_TDD_t *)&dlsch_pdu)->rah              = 0;
          memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],(void *)&dlsch_pdu,sizeof(DCI1_20MHz_TDD_t));

          /*
          //user2
          DCI_pdu->dci_alloc[1].dci_length = sizeof_DCI1_20MHz_TDD_t;
          DCI_pdu->dci_alloc[1].L          = 2;
          DCI_pdu->dci_alloc[1].rnti       = 0x1236;
          DCI_pdu->dci_alloc[1].format     = format1;
          DCI_pdu->dci_alloc[1].ra_flag    = 0;

          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->rballoc          = rballoc2;
          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->TPC              = 0;
          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->dai              = 0;
          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->harq_pid         = 1;
          //((DCI1_20MHz_FDD_t *)&dlsch_pdu)->mcs              = (unsigned char) ((phy_vars_eNB->proc[subframe].frame%1024)%28);
          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->mcs              = openair_daq_vars.target_ue_dl_mcs;
          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->ndi              = 1;
          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->rv               = 0;
          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->rah              = 0;
          memcpy((void*)&DCI_pdu->dci_alloc[1].dci_pdu[0],(void *)&((DCI1_20MHz_FDD_t *)&dlsch_pdu)->,sizeof(DCI1_5MHz_TDD_t));
          */
        }

        break;
      }
    } else if (transmission_mode==4) {
      DCI_pdu->Num_ue_spec_dci = 1;
      // user 1
      DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI2_5MHz_2A_FDD_t;
      DCI_pdu->dci_alloc[0].L          = 3;
      DCI_pdu->dci_alloc[0].rnti       = 0x1235;
      DCI_pdu->dci_alloc[0].format     = format2;
      DCI_pdu->dci_alloc[0].ra_flag    = 0;

      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->tpmi     = 0;
      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->rv1      = 0;
      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->ndi1     = subframe / 5;
      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->mcs1     = openair_daq_vars.target_ue_dl_mcs;
      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->rv2      = 0;
      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->ndi2     = subframe / 5;
      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->mcs2     = openair_daq_vars.target_ue_dl_mcs;
      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->tb_swap  = 0;
      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->harq_pid = subframe % 5;
      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->TPC      = 0;
      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->rballoc  = openair_daq_vars.ue_dl_rb_alloc;
      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->rah      = 0;

    } else if (transmission_mode==5) {
      DCI_pdu->Num_ue_spec_dci = 2;
      // user 1
      DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1E_5MHz_2A_M10PRB_TDD_t;
      DCI_pdu->dci_alloc[0].L          = 3;
      DCI_pdu->dci_alloc[0].rnti       = 0x1235;
      DCI_pdu->dci_alloc[0].format     = format1E_2A_M10PRB;
      DCI_pdu->dci_alloc[0].ra_flag    = 0;

      DLSCH_alloc_pdu1E.tpmi             = 5; //5=use feedback
      DLSCH_alloc_pdu1E.rv               = 0;
      DLSCH_alloc_pdu1E.ndi              = subframe / 5;
      //DLSCH_alloc_pdu1E.mcs            = cqi_to_mcs[phy_vars_eNB->eNB_UE_stats->DL_cqi[0]];
      //DLSCH_alloc_pdu1E.mcs            = (unsigned char) (taus()%28);
      DLSCH_alloc_pdu1E.mcs              = openair_daq_vars.target_ue_dl_mcs;
      //DLSCH_alloc_pdu1E.mcs            = (unsigned char) ((phy_vars_eNB->proc[subframe].frame%1024)%28);
      phy_vars_eNB->eNB_UE_stats[0].dlsch_mcs1 = DLSCH_alloc_pdu1E.mcs;
      DLSCH_alloc_pdu1E.harq_pid         = subframe % 5;
      DLSCH_alloc_pdu1E.dai              = 0;
      DLSCH_alloc_pdu1E.TPC              = 0;
      DLSCH_alloc_pdu1E.rballoc          = openair_daq_vars.ue_dl_rb_alloc;
      DLSCH_alloc_pdu1E.rah              = 0;
      DLSCH_alloc_pdu1E.dl_power_off     = 0; //0=second user present
      memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],(void *)&DLSCH_alloc_pdu1E,sizeof(DCI1E_5MHz_2A_M10PRB_TDD_t));

      //user 2
      DCI_pdu->dci_alloc[1].dci_length = sizeof_DCI1E_5MHz_2A_M10PRB_TDD_t;
      DCI_pdu->dci_alloc[1].L          = 0;
      DCI_pdu->dci_alloc[1].rnti       = 0x1236;
      DCI_pdu->dci_alloc[1].format     = format1E_2A_M10PRB;
      DCI_pdu->dci_alloc[1].ra_flag    = 0;
      //DLSCH_alloc_pdu1E.mcs            = openair_daq_vars.target_ue_dl_mcs;
      //DLSCH_alloc_pdu1E.mcs            = (unsigned char) (taus()%28);
      //DLSCH_alloc_pdu1E.mcs            = (unsigned char) ((phy_vars_eNB->frame%1024)%28);
      DLSCH_alloc_pdu1E.mcs            = (unsigned char) (((phy_vars_eNB->proc[sched_subframe].frame_tx%1024)/3)%28);
      phy_vars_eNB->eNB_UE_stats[1].dlsch_mcs1 = DLSCH_alloc_pdu1E.mcs;

      memcpy((void*)&DCI_pdu->dci_alloc[1].dci_pdu[0],(void *)&DLSCH_alloc_pdu1E,sizeof(DCI1E_5MHz_2A_M10PRB_TDD_t));

      // set the precoder of the second UE orthogonal to the first
      phy_vars_eNB->eNB_UE_stats[1].DL_pmi_single = (phy_vars_eNB->eNB_UE_stats[0].DL_pmi_single ^ 0x1555);
    }

    break; //subframe switch

    /*
      case 8:
      DCI_pdu->Num_common_dci = 1;
      DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1A_5MHz_TDD_1_6_t;
      DCI_pdu->dci_alloc[0].L          = 2;
      DCI_pdu->dci_alloc[0].rnti       = 0xbeef;
      DCI_pdu->dci_alloc[0].format     = format1A;
      DCI_pdu->dci_alloc[0].ra_flag    = 1;

      RA_alloc_pdu.type                = 1;
      RA_alloc_pdu.vrb_type            = 0;
      RA_alloc_pdu.rballoc             = computeRIV(25,12,3);
      RA_alloc_pdu.ndi      = 1;
      RA_alloc_pdu.rv       = 1;
      RA_alloc_pdu.mcs      = 4;
      RA_alloc_pdu.harq_pid = 0;
      RA_alloc_pdu.TPC      = 1;

      memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],&RA_alloc_pdu,sizeof(DCI1A_5MHz_TDD_1_6_t));
      break;
    */
    /*
  case 9:
    DCI_pdu->Num_ue_spec_dci = 1;

    //user 1
    if (phy_vars_eNB->lte_frame_parms.frame_type == FDD)
      DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI0_5MHz_FDD_t ;
    else
      DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI0_5MHz_TDD_1_6_t ;

    DCI_pdu->dci_alloc[0].L          = 2;
    DCI_pdu->dci_alloc[0].rnti       = 0x1235;
    DCI_pdu->dci_alloc[0].format     = format0;
    DCI_pdu->dci_alloc[0].ra_flag    = 0;

    UL_alloc_pdu.type    = 0;
    UL_alloc_pdu.hopping = 0;
    UL_alloc_pdu.rballoc = computeRIV(25,2,openair_daq_vars.ue_ul_nb_rb);
    UL_alloc_pdu.mcs     = openair_daq_vars.target_ue_ul_mcs;
    UL_alloc_pdu.ndi     = phy_vars_eNB->proc[sched_subframe].frame_tx&1;
    UL_alloc_pdu.TPC     = 0;
    UL_alloc_pdu.cshift  = 0;
    UL_alloc_pdu.dai     = 0;
    UL_alloc_pdu.cqi_req = 1;
    memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],(void *)&UL_alloc_pdu,sizeof(DCI0_5MHz_TDD_1_6_t));
    */
    // user 2
    /*
    DCI_pdu->dci_alloc[1].dci_length = sizeof_DCI0_5MHz_TDD_1_6_t ;
    DCI_pdu->dci_alloc[1].L          = 2;
    DCI_pdu->dci_alloc[1].rnti       = 0x1236;
    DCI_pdu->dci_alloc[1].format     = format0;
    DCI_pdu->dci_alloc[1].ra_flag    = 0;

    UL_alloc_pdu.type    = 0;
    UL_alloc_pdu.hopping = 0;
    if (cooperation_flag==0)
      UL_alloc_pdu.rballoc = computeRIV(25,2+openair_daq_vars.ue_ul_nb_rb,openair_daq_vars.ue_ul_nb_rb);
    else
      UL_alloc_pdu.rballoc = computeRIV(25,0,openair_daq_vars.ue_ul_nb_rb);
    UL_alloc_pdu.mcs     = openair_daq_vars.target_ue_ul_mcs;
    UL_alloc_pdu.ndi     = phy_vars_eNB->proc[sched_subframe].frame_tx&1;
    UL_alloc_pdu.TPC     = 0;
    if ((cooperation_flag==0) || (cooperation_flag==1))
      UL_alloc_pdu.cshift  = 0;
    else
      UL_alloc_pdu.cshift  = 1;
    UL_alloc_pdu.dai     = 0;
    UL_alloc_pdu.cqi_req = 1;
    memcpy((void*)&DCI_pdu->dci_alloc[1].dci_pdu[0],(void *)&UL_alloc_pdu,sizeof(DCI0_5MHz_TDD_1_6_t));
    break;
    */

    /*default:
      break;*/
  }

  /*
  DCI_pdu->nCCE = 0;

  for (i=0; i<DCI_pdu->Num_common_dci+DCI_pdu->Num_ue_spec_dci; i++) {
    DCI_pdu->nCCE += (1<<(DCI_pdu->dci_alloc[i].L));
  }
  */
}

void fill_dci_emos(DCI_PDU *DCI_pdu, uint8_t subframe, PHY_VARS_eNB *phy_vars_eNB)
{

  //uint8_t cooperation_flag = phy_vars_eNB->cooperation_flag;
  uint8_t transmission_mode = phy_vars_eNB->transmission_mode[0];

  //uint32_t rballoc = 0x00F0;
  //uint32_t rballoc2 = 0x000F;
  /*
    uint32_t rand = taus();
    if ((subframe==8) || (subframe==9) || (subframe==0))
    rand = (rand%5)+5;
    else
    rand = (rand%4)+5;
  */

  DCI_pdu->Num_common_dci = 0;
  DCI_pdu->Num_ue_spec_dci=0;

  switch (subframe) {
  case 5:
    DCI_pdu->Num_ue_spec_dci = 1;

    if (transmission_mode<3) {
      //user 1
      DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1_5MHz_TDD_t;
      DCI_pdu->dci_alloc[0].L          = 2;
      DCI_pdu->dci_alloc[0].rnti       = 0x1235;
      DCI_pdu->dci_alloc[0].format     = format1;
      DCI_pdu->dci_alloc[0].ra_flag    = 0;

      DLSCH_alloc_pdu.rballoc          = openair_daq_vars.ue_dl_rb_alloc;
      DLSCH_alloc_pdu.TPC              = 0;
      DLSCH_alloc_pdu.dai              = 0;
      DLSCH_alloc_pdu.harq_pid         = 1;
      DLSCH_alloc_pdu.mcs              = openair_daq_vars.target_ue_dl_mcs;
      DLSCH_alloc_pdu.ndi              = 1;
      DLSCH_alloc_pdu.rv               = 0;
      memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],(void *)&DLSCH_alloc_pdu,sizeof(DCI1_5MHz_TDD_t));

      /*
      //user2
      DCI_pdu->dci_alloc[1].dci_length = sizeof_DCI1_5MHz_TDD_t;
      DCI_pdu->dci_alloc[1].L          = 2;
      DCI_pdu->dci_alloc[1].rnti       = 0x1236;
      DCI_pdu->dci_alloc[1].format     = format1;
      DCI_pdu->dci_alloc[1].ra_flag    = 0;

      DLSCH_alloc_pdu.rballoc          = rballoc2;
      DLSCH_alloc_pdu.TPC              = 0;
      DLSCH_alloc_pdu.dai              = 0;
      DLSCH_alloc_pdu.harq_pid         = 1;
      DLSCH_alloc_pdu.mcs              = openair_daq_vars.target_ue_dl_mcs;
      DLSCH_alloc_pdu.ndi              = 1;
      DLSCH_alloc_pdu.rv               = 0;
      memcpy((void*)&DCI_pdu->dci_alloc[1].dci_pdu[0],(void *)&DLSCH_alloc_pdu,sizeof(DCI1_5MHz_TDD_t));
      */
    } else if (transmission_mode==5) {
      DCI_pdu->Num_ue_spec_dci = 2;
      // user 1
      DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1E_5MHz_2A_M10PRB_TDD_t;
      DCI_pdu->dci_alloc[0].L          = 2;
      DCI_pdu->dci_alloc[0].rnti       = 0x1235;
      DCI_pdu->dci_alloc[0].format     = format1E_2A_M10PRB;
      DCI_pdu->dci_alloc[0].ra_flag    = 0;

      DLSCH_alloc_pdu1E.tpmi             = 5; //5=use feedback
      DLSCH_alloc_pdu1E.rv               = 0;
      DLSCH_alloc_pdu1E.ndi              = 1;
      DLSCH_alloc_pdu1E.mcs              = openair_daq_vars.target_ue_dl_mcs;
      DLSCH_alloc_pdu1E.harq_pid         = 1;
      DLSCH_alloc_pdu1E.dai              = 0;
      DLSCH_alloc_pdu1E.TPC              = 0;
      DLSCH_alloc_pdu1E.rballoc          = openair_daq_vars.ue_dl_rb_alloc;
      DLSCH_alloc_pdu1E.rah              = 0;
      DLSCH_alloc_pdu1E.dl_power_off     = 0; //0=second user present
      memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],(void *)&DLSCH_alloc_pdu1E,sizeof(DCI1E_5MHz_2A_M10PRB_TDD_t));

      //user 2
      DCI_pdu->dci_alloc[1].dci_length = sizeof_DCI1E_5MHz_2A_M10PRB_TDD_t;
      DCI_pdu->dci_alloc[1].L          = 2;
      DCI_pdu->dci_alloc[1].rnti       = 0x1236;
      DCI_pdu->dci_alloc[1].format     = format1E_2A_M10PRB;
      DCI_pdu->dci_alloc[1].ra_flag    = 0;

      memcpy((void*)&DCI_pdu->dci_alloc[1].dci_pdu[0],(void *)&DLSCH_alloc_pdu1E,sizeof(DCI1E_5MHz_2A_M10PRB_TDD_t));

      // set the precoder of the second UE orthogonal to the first
      phy_vars_eNB->eNB_UE_stats[1].DL_pmi_single = (phy_vars_eNB->eNB_UE_stats[0].DL_pmi_single ^ 0x1555);
    }

    break;

  case 7:
    DCI_pdu->Num_common_dci = 1;
    DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1A_5MHz_TDD_1_6_t;
    DCI_pdu->dci_alloc[0].L          = 2;
    DCI_pdu->dci_alloc[0].rnti       = 0xbeef;
    DCI_pdu->dci_alloc[0].format     = format1A;
    DCI_pdu->dci_alloc[0].ra_flag    = 1;

    RA_alloc_pdu.type                = 1;
    RA_alloc_pdu.vrb_type            = 0;
    RA_alloc_pdu.rballoc             = computeRIV(25,12,3);
    RA_alloc_pdu.ndi      = 1;
    RA_alloc_pdu.rv       = 1;
    RA_alloc_pdu.mcs      = 4;
    RA_alloc_pdu.harq_pid = 0;
    RA_alloc_pdu.TPC      = 1;

    memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],&RA_alloc_pdu,sizeof(DCI1A_5MHz_TDD_1_6_t));
    break;

  case 9:
    DCI_pdu->Num_ue_spec_dci = 1;

    //user 1
    DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI0_5MHz_TDD_1_6_t ;
    DCI_pdu->dci_alloc[0].L          = 2;
    DCI_pdu->dci_alloc[0].rnti       = 0x1235;
    DCI_pdu->dci_alloc[0].format     = format0;
    DCI_pdu->dci_alloc[0].ra_flag    = 0;

    UL_alloc_pdu.type    = 0;
    UL_alloc_pdu.hopping = 0;
    UL_alloc_pdu.rballoc = computeRIV(25,0,openair_daq_vars.ue_ul_nb_rb);
    UL_alloc_pdu.mcs     = openair_daq_vars.target_ue_ul_mcs;
    UL_alloc_pdu.ndi     = 1;
    UL_alloc_pdu.TPC     = 0;
    UL_alloc_pdu.cshift  = 0;
    UL_alloc_pdu.dai     = 0;
    UL_alloc_pdu.cqi_req = 1;
    memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],(void *)&UL_alloc_pdu,sizeof(DCI0_5MHz_TDD_1_6_t));

    /*
    //user 2
    DCI_pdu->dci_alloc[1].dci_length = sizeof_DCI0_5MHz_TDD_1_6_t ;
    DCI_pdu->dci_alloc[1].L          = 2;
    DCI_pdu->dci_alloc[1].rnti       = 0x1236;
    DCI_pdu->dci_alloc[1].format     = format0;
    DCI_pdu->dci_alloc[1].ra_flag    = 0;

    UL_alloc_pdu.type    = 0;
    UL_alloc_pdu.hopping = 0;
    if (cooperation_flag==0)
    UL_alloc_pdu.rballoc = computeRIV(25,2+openair_daq_vars.ue_ul_nb_rb,openair_daq_vars.ue_ul_nb_rb);
    else
    UL_alloc_pdu.rballoc = computeRIV(25,0,openair_daq_vars.ue_ul_nb_rb);
    UL_alloc_pdu.mcs     = openair_daq_vars.target_ue_ul_mcs;
    UL_alloc_pdu.ndi     = 1;
    UL_alloc_pdu.TPC     = 0;
    if ((cooperation_flag==0) || (cooperation_flag==1))
    UL_alloc_pdu.cshift  = 0;
    else
    UL_alloc_pdu.cshift  = 1;
    UL_alloc_pdu.dai     = 0;
    UL_alloc_pdu.cqi_req = 1;
    memcpy((void*)&DCI_pdu->dci_alloc[1].dci_pdu[0],(void *)&UL_alloc_pdu,sizeof(DCI0_5MHz_TDD_1_6_t));
    */
    break;

  default:
    break;
  }
  /*
  DCI_pdu->nCCE = 0;

  for (i=0; i<DCI_pdu->Num_common_dci+DCI_pdu->Num_ue_spec_dci; i++) {
    DCI_pdu->nCCE += (1<<(DCI_pdu->dci_alloc[i].L));
  }
  */
}
