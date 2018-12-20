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

/* SYRTEM */
uint8_t dci_ndi_toggle_even[8] = {0, 0, 0, 0, 0, 0, 0, 0};
uint8_t dci_ndi_toggle_odd[8]  = {0, 0, 0, 0, 0, 0, 0, 0};

void fill_dci(DCI_PDU *DCI_pdu,PHY_VARS_eNB *eNB,L1_rxtx_proc_t *proc)
{
/* SYRTEM */
  uint8_t *dci_ndi_toggle_tmp = NULL;
  uint8_t harq_pid_value      = 0;
/* End of SYRTEM */

  //uint8_t cooperation_flag = eNB->cooperation_flag;
  uint8_t transmission_mode = eNB->transmission_mode[0];

  uint32_t rballoc = 0x1FFFF;
  //uint32_t rballoc2 = 0x000F;

  int subframe = proc->subframe_tx;


  /*
    uint32_t rand = taus();
    if ((subframe==8) || (subframe==9) || (subframe==0))
    rand = (rand%5)+5;
    else
    rand = (rand%4)+5;
  */
  uint32_t bcch_pdu;
  uint64_t dlsch_pdu;

  LOG_D(PHY,"frame %d, subframe %d, transmission_mode %d\n",proc->frame_tx,proc->subframe_tx,transmission_mode);

  DCI_pdu->Num_dci = 0;

  switch (subframe) {
  case 5:
   if ( !(proc->frame_tx&1) ) // SI message on even frame only (SFN mod 2 == 0)
   {
	DCI_pdu->Num_dci = 1;
	DCI_pdu->dci_alloc[0].L            = 2;
	DCI_pdu->dci_alloc[0].firstCCE     = 0;
	DCI_pdu->dci_alloc[0].rnti         = SI_RNTI;
	DCI_pdu->dci_alloc[0].format       = format1A;
	DCI_pdu->dci_alloc[0].ra_flag      = 0;
	DCI_pdu->dci_alloc[0].search_space = DCI_COMMON_SPACE;

    switch (eNB->frame_parms.N_RB_DL) {
    case 6:
      if (eNB->frame_parms.frame_type == FDD) {
        DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1A_1_5MHz_FDD_t;
        ((DCI1A_1_5MHz_FDD_t*)&bcch_pdu)->type              = 1;
        ((DCI1A_1_5MHz_FDD_t*)&bcch_pdu)->vrb_type          = 0;
        ((DCI1A_1_5MHz_FDD_t*)&bcch_pdu)->rballoc           = computeRIV(25,10,3);
        ((DCI1A_1_5MHz_FDD_t*)&bcch_pdu)->ndi               = proc->frame_tx&1;
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
        ((DCI1A_1_5MHz_TDD_1_6_t*)&bcch_pdu)->ndi               = proc->frame_tx&1;
        ((DCI1A_1_5MHz_TDD_1_6_t*)&bcch_pdu)->rv                = 1;
        ((DCI1A_1_5MHz_TDD_1_6_t*)&bcch_pdu)->mcs               = 1;
        ((DCI1A_1_5MHz_TDD_1_6_t*)&bcch_pdu)->harq_pid          = 0;
        ((DCI1A_1_5MHz_TDD_1_6_t*)&bcch_pdu)->TPC               = 1;      // set to 3 PRB
        memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],&bcch_pdu,sizeof(DCI1A_1_5MHz_TDD_1_6_t));
      }

      break;

    case 25:
    default:
      if (eNB->frame_parms.frame_type == FDD) {
        DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1A_5MHz_FDD_t;
        ((DCI1A_5MHz_FDD_t*)&bcch_pdu)->type              = 1;
        ((DCI1A_5MHz_FDD_t*)&bcch_pdu)->vrb_type          = 0;
        ((DCI1A_5MHz_FDD_t*)&bcch_pdu)->rballoc           = computeRIV(25,10,3);
        ((DCI1A_5MHz_FDD_t*)&bcch_pdu)->ndi               = proc->frame_tx&1;
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
        ((DCI1A_5MHz_TDD_1_6_t*)&bcch_pdu)->ndi               = proc->frame_tx&1;
        ((DCI1A_5MHz_TDD_1_6_t*)&bcch_pdu)->rv                = 1;
        ((DCI1A_5MHz_TDD_1_6_t*)&bcch_pdu)->mcs               = 1;
        ((DCI1A_5MHz_TDD_1_6_t*)&bcch_pdu)->harq_pid          = 0;
        ((DCI1A_5MHz_TDD_1_6_t*)&bcch_pdu)->TPC               = 1;      // set to 3 PRB
        memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],&bcch_pdu,sizeof(DCI1A_5MHz_TDD_1_6_t));
      }

      break;

    case 50:
      if (eNB->frame_parms.frame_type == FDD) {
        DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1A_10MHz_FDD_t;
        ((DCI1A_10MHz_FDD_t*)&bcch_pdu)->type              = 1;
        ((DCI1A_10MHz_FDD_t*)&bcch_pdu)->vrb_type          = 0;
        ((DCI1A_10MHz_FDD_t*)&bcch_pdu)->rballoc           = computeRIV(50,10,3);
        ((DCI1A_10MHz_FDD_t*)&bcch_pdu)->ndi               = proc->frame_tx&1;
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
      if (eNB->frame_parms.frame_type == FDD) {
        DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1A_20MHz_FDD_t;
        ((DCI1A_20MHz_FDD_t*)&bcch_pdu)->type              = 1;
        ((DCI1A_20MHz_FDD_t*)&bcch_pdu)->vrb_type          = 0;
        ((DCI1A_20MHz_FDD_t*)&bcch_pdu)->rballoc           = computeRIV(100,10,3);
        ((DCI1A_20MHz_FDD_t*)&bcch_pdu)->ndi               = proc->frame_tx&1;
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
        ((DCI1A_20MHz_TDD_1_6_t*)&bcch_pdu)->ndi               = proc->frame_tx&1;
        ((DCI1A_20MHz_TDD_1_6_t*)&bcch_pdu)->rv                = 1;
        ((DCI1A_20MHz_TDD_1_6_t*)&bcch_pdu)->mcs               = 1;
        ((DCI1A_20MHz_TDD_1_6_t*)&bcch_pdu)->harq_pid          = 0;
        ((DCI1A_20MHz_TDD_1_6_t*)&bcch_pdu)->TPC               = 1;      // set to 3 PRB
        memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],&bcch_pdu,sizeof(DCI1A_20MHz_TDD_1_6_t));
      }

      break;
    }
    break; //subframe switch

    }  // if ( !(proc->frame_tx&1) )
    else // No SI message on odd frame (SFN mod 2 == 1)
    {
      /* warning: work around to send dlsch on subframe 5 odd frame !!! */
      /* todo: clean up, espacially if subframe cases are added next !!! */
    }
    /*
  case 6:
      DCI_pdu->Num_dci = 1;
      DCI_pdu->dci_alloc[0].dci_length   = sizeof_DCI2_5MHz_2A_M10PRB_TDD_t;
      DCI_pdu->dci_alloc[0].L            = 2;
      DCI_pdu->dci_alloc[0].rnti         = 0x1236;
      DCI_pdu->dci_alloc[0].format       = format2_2A_M10PRB;
      DCI_pdu->dci_alloc[0].ra_flag      = 0;
      DCI_pdu->dci_alloc[0].search_space = DCI_UE_SPACE;

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
    DCI_pdu->Num_dci = 1;
    DCI_pdu->dci_alloc[0].L            = 2;
    DCI_pdu->dci_alloc[0].firstCCE     = 0;
    DCI_pdu->dci_alloc[0].rnti         = 0x1235;
    DCI_pdu->dci_alloc[0].format       = format1;
    DCI_pdu->dci_alloc[0].ra_flag      = 0;
    DCI_pdu->dci_alloc[0].search_space = DCI_UE_SPACE;

    if (transmission_mode<3 || transmission_mode == 7) {
      //user 1
      switch (eNB->frame_parms.N_RB_DL) {
      case 25:
        if (eNB->frame_parms.frame_type == FDD) {
          DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1_5MHz_FDD_t;

          harq_pid_value = (  ((proc->frame_tx * 10) + subframe) % 8 );

          if (!(subframe&1)) // even subframe
            dci_ndi_toggle_tmp = &(dci_ndi_toggle_even[harq_pid_value]);
          else // odd subframe
            dci_ndi_toggle_tmp = &(dci_ndi_toggle_odd[harq_pid_value]);

          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->rballoc          = rballoc; //computeRIV(25,10,3);
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->TPC              = 0;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->harq_pid         = harq_pid_value;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->mcs              = eNB->target_ue_dl_mcs;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->ndi              = (*dci_ndi_toggle_tmp);
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->rv               = 0;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->rah              = 0;

          (*dci_ndi_toggle_tmp) = ((*dci_ndi_toggle_tmp) + 1) & 1;

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
          //((DCI1_5MHz_FDD_t *)&dlsch_pdu)->mcs              = (unsigned char) ((eNB->proc[subframe].frame%1024)%28);
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->mcs              = eNB->target_ue_dl_mcs;
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
          ((DCI1_5MHz_TDD_t *)&dlsch_pdu)->mcs              = eNB->target_ue_dl_mcs;
          //((DCI1_5MHz_TDD_t *)&dlsch_pdu)->mcs            = (unsigned char) ((eNB->frame%1024)%28);
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
          //((DCI1_5MHz_FDD_t *)&dlsch_pdu)->mcs              = (unsigned char) ((eNB->proc[subframe].frame%1024)%28);
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->mcs              = eNB->target_ue_dl_mcs;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->ndi              = 1;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->rv               = 0;
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->rah              = 0;
          memcpy((void*)&DCI_pdu->dci_alloc[1].dci_pdu[0],(void *)&((DCI1_5MHz_FDD_t *)&dlsch_pdu)->,sizeof(DCI1_5MHz_TDD_t));
          */
        }

        break;

      case 50:

        if (eNB->frame_parms.frame_type == FDD) {
          DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1_10MHz_FDD_t;

          harq_pid_value = (  ((proc->frame_tx * 10) + subframe) % 8 );

          if (!(subframe&1)) // even subframe
            dci_ndi_toggle_tmp = &(dci_ndi_toggle_even[harq_pid_value]);
          else // odd subframe
            dci_ndi_toggle_tmp = &(dci_ndi_toggle_odd[harq_pid_value]);

          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->rballoc          = rballoc; //computeRIV(50,10,3);
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->TPC              = 0;
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->harq_pid         = harq_pid_value;
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->mcs              = eNB->target_ue_dl_mcs;
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->ndi              = (*dci_ndi_toggle_tmp);
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->rv               = 0;
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->rah              = 0;

          (*dci_ndi_toggle_tmp) = ((*dci_ndi_toggle_tmp) + 1) & 1;

          memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],(void *)&dlsch_pdu,sizeof(DCI1_10MHz_FDD_t));
        } else {
          DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1_10MHz_TDD_t;

          ((DCI1_10MHz_TDD_t *)&dlsch_pdu)->rballoc          = rballoc; //computeRIV(50,10,3);
          ((DCI1_10MHz_TDD_t *)&dlsch_pdu)->TPC              = 0;
          ((DCI1_10MHz_TDD_t *)&dlsch_pdu)->dai              = 0;
          ((DCI1_10MHz_TDD_t *)&dlsch_pdu)->harq_pid         = subframe % 5;
          ((DCI1_10MHz_TDD_t *)&dlsch_pdu)->mcs              = eNB->target_ue_dl_mcs;
          //((DCI1_10MHz_TDD_t *)&dlsch_pdu)->mcs            = (unsigned char) ((eNB->frame%1024)%28);
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
          //((DCI1_10MHz_FDD_t *)&dlsch_pdu)->mcs              = (unsigned char) ((eNB->proc[subframe].frame%1024)%28);
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->mcs              = eNB->target_ue_dl_mcs;
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->ndi              = 1;
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->rv               = 0;
          ((DCI1_10MHz_FDD_t *)&dlsch_pdu)->rah              = 0;
          memcpy((void*)&DCI_pdu->dci_alloc[1].dci_pdu[0],(void *)&((DCI1_10MHz_FDD_t *)&dlsch_pdu)->,sizeof(DCI1_10MHz_TDD_t));
          */
        }

        break;

      case 100:
        if (eNB->frame_parms.frame_type == FDD) {
          DCI_pdu->dci_alloc[0].dci_length = sizeof_DCI1_20MHz_FDD_t;

          harq_pid_value = (  ((proc->frame_tx * 10) + subframe) % 8 );

          if (!(subframe&1)) // even subframe
            dci_ndi_toggle_tmp = &(dci_ndi_toggle_even[harq_pid_value]);
          else // odd subframe
            dci_ndi_toggle_tmp = &(dci_ndi_toggle_odd[harq_pid_value]);

          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->rballoc          = 0x1ffffff; //rballoc; //computeRIV(100,10,3);
          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->TPC              = 0;
          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->harq_pid         = harq_pid_value;
          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->mcs              = eNB->target_ue_dl_mcs;
          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->ndi              = (*dci_ndi_toggle_tmp);
          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->rv               = 0;
          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->rah              = 0;

          (*dci_ndi_toggle_tmp) = ((*dci_ndi_toggle_tmp) + 1) & 1;

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
          //((DCI1_5MHz_FDD_t *)&dlsch_pdu)->mcs              = (unsigned char) ((eNB->proc[subframe].frame%1024)%28);
          ((DCI1_5MHz_FDD_t *)&dlsch_pdu)->mcs              = eNB->target_ue_dl_mcs;
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
          ((DCI1_20MHz_TDD_t *)&dlsch_pdu)->mcs              = eNB->target_ue_dl_mcs;
          //((DCI1_20MHz_TDD_t *)&dlsch_pdu)->mcs            = (unsigned char) ((eNB->frame%1024)%28);
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
          //((DCI1_20MHz_FDD_t *)&dlsch_pdu)->mcs              = (unsigned char) ((eNB->proc[subframe].frame%1024)%28);
          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->mcs              = eNB->target_ue_dl_mcs;
          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->ndi              = 1;
          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->rv               = 0;
          ((DCI1_20MHz_FDD_t *)&dlsch_pdu)->rah              = 0;
          memcpy((void*)&DCI_pdu->dci_alloc[1].dci_pdu[0],(void *)&((DCI1_20MHz_FDD_t *)&dlsch_pdu)->,sizeof(DCI1_5MHz_TDD_t));
          */
        }

        break;
      }


    

    DCI_pdu->dci_alloc[1].L            = 2;
    DCI_pdu->dci_alloc[1].rnti         = 0x1235;
    DCI_pdu->dci_alloc[1].format       = format0;
    DCI_pdu->dci_alloc[1].ra_flag      = 0;
    DCI_pdu->dci_alloc[1].search_space = DCI_UE_SPACE;

    if (eNB->frame_parms.frame_type == FDD) {
      switch (eNB->frame_parms.N_RB_DL) {
      case 6:
	((DCI0_1_5MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->type    = 0;
	((DCI0_1_5MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->hopping = 0;
	((DCI0_1_5MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->rballoc = computeRIV(6,1,4);
	((DCI0_1_5MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->mcs     = eNB->target_ue_ul_mcs;
	((DCI0_1_5MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->ndi     = proc->frame_tx&1;
	((DCI0_1_5MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->TPC     = 0;
	((DCI0_1_5MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cshift  = 0;
	((DCI0_1_5MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cqi_req = 1;
	break;
	/*      case 15:
	((DCI0_3MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->type    = 0;
	((DCI0_3MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->hopping = 0;
	((DCI0_3MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->rballoc = computeRIV(25,2,eNB->ue_ul_nb_rb);
	((DCI0_3MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->mcs     = eNB->target_ue_ul_mcs;
	((DCI0_3MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->ndi     = proc->frame_tx&1;
	((DCI0_3MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->TPC     = 0;
	((DCI0_3MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cshift  = 0;
	((DCI0_3MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->dai     = 0;
	((DCI0_3MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cqi_req = 1;
	break;*/
      case 25:
	((DCI0_5MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->type    = 0;
	((DCI0_5MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->hopping = 0;
	((DCI0_5MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->rballoc = computeRIV(25,1,20);
	printf("rballoc %d\n",((DCI0_5MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->rballoc);
	((DCI0_5MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->mcs     = eNB->target_ue_ul_mcs;
	((DCI0_5MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->ndi     = proc->frame_tx&1;
	((DCI0_5MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->TPC     = 0;
	((DCI0_5MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cshift  = 0;
	((DCI0_5MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cqi_req = 1;
	break;
      case 50:
	((DCI0_10MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->type    = 0;
	((DCI0_10MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->hopping = 0;
	((DCI0_10MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->rballoc = computeRIV(50,1,48);
	((DCI0_10MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->mcs     = eNB->target_ue_ul_mcs;
	((DCI0_10MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->ndi     = proc->frame_tx&1;
	((DCI0_10MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->TPC     = 0;
	((DCI0_10MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cshift  = 0;
	((DCI0_10MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cqi_req = 1;
	break;
	/*      case 75:
	((DCI0_15MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->type    = 0;
	((DCI0_15MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->hopping = 0;
	((DCI0_15MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->rballoc = computeRIV(25,2,eNB->ue_ul_nb_rb);
	((DCI0_15MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->mcs     = eNB->target_ue_ul_mcs;
	((DCI0_15MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->ndi     = proc->frame_tx&1;
	((DCI0_15MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->TPC     = 0;
	((DCI0_15MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cshift  = 0;
	((DCI0_15MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cqi_req = 1;
	break;*/
      case 100:
	((DCI0_20MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->type    = 0;
	((DCI0_20MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->hopping = 0;
	((DCI0_20MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->rballoc = computeRIV(100,1,96);
	((DCI0_20MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->mcs     = eNB->target_ue_ul_mcs;
	((DCI0_20MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->ndi     = proc->frame_tx&1;
	((DCI0_20MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->TPC     = 0;
	((DCI0_20MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cshift  = 0;
	((DCI0_20MHz_FDD_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cqi_req = 1;
	break;
      }
    }
    else {
      switch (eNB->frame_parms.N_RB_DL) {
      case 6:
	((DCI0_1_5MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->type    = 0;
	((DCI0_1_5MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->hopping = 0;
	((DCI0_1_5MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->rballoc = computeRIV(6,1,5);
	((DCI0_1_5MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->mcs     = eNB->target_ue_ul_mcs;
	((DCI0_1_5MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->ndi     = proc->frame_tx&1;
	((DCI0_1_5MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->TPC     = 0;
	((DCI0_1_5MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cshift  = 0;
	((DCI0_1_5MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->dai     = 0;
	((DCI0_1_5MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cqi_req = 1;
	break;
	/*      case 15:
	((DCI0_3MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->type    = 0;
	((DCI0_3MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->hopping = 0;
	((DCI0_3MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->rballoc = computeRIV(25,2,eNB->ue_ul_nb_rb);
	((DCI0_3MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->mcs     = eNB->target_ue_ul_mcs;
	((DCI0_3MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->ndi     = proc->frame_tx&1;
	((DCI0_3MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->TPC     = 0;
	((DCI0_3MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cshift  = 0;
	((DCI0_3MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->dai     = 0;
	((DCI0_3MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cqi_req = 1;
	break;*/
      case 25:
	((DCI0_5MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->type    = 0;
	((DCI0_5MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->hopping = 0;
	((DCI0_5MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->rballoc = computeRIV(25,2,20);
	((DCI0_5MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->mcs     = eNB->target_ue_ul_mcs;
	((DCI0_5MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->ndi     = proc->frame_tx&1;
	((DCI0_5MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->TPC     = 0;
	((DCI0_5MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cshift  = 0;
	((DCI0_5MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->dai     = 0;
	((DCI0_5MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cqi_req = 1;
	break;
      case 50:
	((DCI0_10MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->type    = 0;
	((DCI0_10MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->hopping = 0;
	((DCI0_10MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->rballoc = computeRIV(50,1,48);
	((DCI0_10MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->mcs     = eNB->target_ue_ul_mcs;
	((DCI0_10MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->ndi     = proc->frame_tx&1;
	((DCI0_10MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->TPC     = 0;
	((DCI0_10MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cshift  = 0;
	((DCI0_10MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->dai     = 0;
	((DCI0_10MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cqi_req = 1;
	break;
	/*      case 75:
	((DCI0_15MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->type    = 0;
	((DCI0_15MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->hopping = 0;
	((DCI0_15MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->rballoc = computeRIV(25,2,eNB->ue_ul_nb_rb);
	((DCI0_15MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->mcs     = eNB->target_ue_ul_mcs;
	((DCI0_15MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->ndi     = proc->frame_tx&1;
	((DCI0_15MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->TPC     = 0;
	((DCI0_15MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cshift  = 0;
	((DCI0_15MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->dai     = 0;
	((DCI0_15MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cqi_req = 1;
	break;*/
      case 100:
	((DCI0_20MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->type    = 0;
	((DCI0_20MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->hopping = 0;
	((DCI0_20MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->rballoc = computeRIV(100,1,96);
	((DCI0_20MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->mcs     = eNB->target_ue_ul_mcs;
	((DCI0_20MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->ndi     = proc->frame_tx&1;
	((DCI0_20MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->TPC     = 0;
	((DCI0_20MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cshift  = 0;
	((DCI0_20MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->dai     = 0;
	((DCI0_20MHz_TDD_1_6_t*)&DCI_pdu->dci_alloc[1].dci_pdu[0])->cqi_req = 1;
	break;
      }
    }
    } else if (transmission_mode==4) {
      DCI_pdu->Num_dci = 1;
      // user 1
      DCI_pdu->dci_alloc[0].dci_length   = sizeof_DCI2_5MHz_2A_FDD_t;
      DCI_pdu->dci_alloc[0].L            = 3;
      DCI_pdu->dci_alloc[0].rnti         = 0x1235;
      DCI_pdu->dci_alloc[0].format       = format2;
      DCI_pdu->dci_alloc[0].ra_flag      = 0;
      DCI_pdu->dci_alloc[0].search_space = DCI_UE_SPACE;

      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->tpmi     = 0;
      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->rv1      = 0;
      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->ndi1     = subframe / 5;
      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->mcs1     = eNB->target_ue_dl_mcs;
      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->rv2      = 0;
      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->ndi2     = subframe / 5;
      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->mcs2     = eNB->target_ue_dl_mcs;
      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->tb_swap  = 0;
      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->harq_pid = subframe % 5;
      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->TPC      = 0;
      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->rballoc  = eNB->ue_dl_rb_alloc;
      ((DCI2_5MHz_2A_FDD_t*) (&DCI_pdu->dci_alloc[0].dci_pdu))->rah      = 0;

    } else if (transmission_mode==5) {
      DCI_pdu->Num_dci = 2;
      // user 1
      DCI_pdu->dci_alloc[0].dci_length   = sizeof_DCI1E_5MHz_2A_M10PRB_TDD_t;
      DCI_pdu->dci_alloc[0].L            = 3;
      DCI_pdu->dci_alloc[0].rnti         = 0x1235;
      DCI_pdu->dci_alloc[0].format       = format1E_2A_M10PRB;
      DCI_pdu->dci_alloc[0].ra_flag      = 0;
      DCI_pdu->dci_alloc[0].search_space = DCI_UE_SPACE;

      DLSCH_alloc_pdu1E.tpmi             = 5; //5=use feedback
      DLSCH_alloc_pdu1E.rv               = 0;
      DLSCH_alloc_pdu1E.ndi              = subframe / 5;
      //DLSCH_alloc_pdu1E.mcs            = cqi_to_mcs[eNB->UE_stats->DL_cqi[0]];
      //DLSCH_alloc_pdu1E.mcs            = (unsigned char) (taus()%28);
      DLSCH_alloc_pdu1E.mcs              = eNB->target_ue_dl_mcs;
      //DLSCH_alloc_pdu1E.mcs            = (unsigned char) ((eNB->proc[subframe].frame%1024)%28);
      eNB->UE_stats[0].dlsch_mcs1 = DLSCH_alloc_pdu1E.mcs;
      DLSCH_alloc_pdu1E.harq_pid         = subframe % 5;
      DLSCH_alloc_pdu1E.dai              = 0;
      DLSCH_alloc_pdu1E.TPC              = 0;
      DLSCH_alloc_pdu1E.rballoc          = eNB->ue_dl_rb_alloc;
      DLSCH_alloc_pdu1E.rah              = 0;
      DLSCH_alloc_pdu1E.dl_power_off     = 0; //0=second user present
      memcpy((void*)&DCI_pdu->dci_alloc[0].dci_pdu[0],(void *)&DLSCH_alloc_pdu1E,sizeof(DCI1E_5MHz_2A_M10PRB_TDD_t));

      //user 2
      DCI_pdu->dci_alloc[1].dci_length   = sizeof_DCI1E_5MHz_2A_M10PRB_TDD_t;
      DCI_pdu->dci_alloc[1].L            = 0;
      DCI_pdu->dci_alloc[1].rnti         = 0x1236;
      DCI_pdu->dci_alloc[1].format       = format1E_2A_M10PRB;
      DCI_pdu->dci_alloc[1].ra_flag      = 0;
      DCI_pdu->dci_alloc[1].search_space = DCI_UE_SPACE;
      //DLSCH_alloc_pdu1E.mcs            = eNB->target_ue_dl_mcs;
      //DLSCH_alloc_pdu1E.mcs            = (unsigned char) (taus()%28);
      //DLSCH_alloc_pdu1E.mcs            = (unsigned char) ((eNB->frame%1024)%28);
      DLSCH_alloc_pdu1E.mcs            = (unsigned char) (((proc->frame_tx%1024)/3)%28);
      eNB->UE_stats[1].dlsch_mcs1 = DLSCH_alloc_pdu1E.mcs;

      memcpy((void*)&DCI_pdu->dci_alloc[1].dci_pdu[0],(void *)&DLSCH_alloc_pdu1E,sizeof(DCI1E_5MHz_2A_M10PRB_TDD_t));

      // set the precoder of the second UE orthogonal to the first
      eNB->UE_stats[1].DL_pmi_single = (eNB->UE_stats[0].DL_pmi_single ^ 0x1555);
    }

    break; //subframe switch

    /*
      case 8:
      DCI_pdu->Num_dci = 1;
      DCI_pdu->dci_alloc[0].dci_length   = sizeof_DCI1A_5MHz_TDD_1_6_t;
      DCI_pdu->dci_alloc[0].L            = 2;
      DCI_pdu->dci_alloc[0].rnti         = 0xbeef;
      DCI_pdu->dci_alloc[0].format       = format1A;
      DCI_pdu->dci_alloc[0].ra_flag      = 1;
      DCI_pdu->dci_alloc[0].search_space = DCI_COMMON_SPACE;

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
      UL_alloc_pdu.rballoc = computeRIV(25,2+eNB->ue_ul_nb_rb,eNB->ue_ul_nb_rb);
    else
      UL_alloc_pdu.rballoc = computeRIV(25,0,eNB->ue_ul_nb_rb);
    UL_alloc_pdu.mcs     = eNB->target_ue_ul_mcs;
    UL_alloc_pdu.ndi     = proc->frame_tx&1;
    UL_alloc_pdu.TPC     = 0;
    if ((cooperation_flag==0) || (cooperation_flag==1))
      UL_alloc_pdu.cshift  = 0;
    else
      UL_alloc_pdu.cshift  = 1;
    UL_alloc_pdu.dai     = 0;
    UL_alloc_pdu.cqi_req = 1;
    memcpy((void*)&DCI_pdu->dci_alloc[1].dci_pdu[0],(void *)&UL_alloc_pdu,sizeof(DCI0_5MHz_TDD_1_6_t));
 
    */
  }

  /*
  DCI_pdu->nCCE = 0;

  for (i=0; i<DCI_pdu->Num_dci; i++) {
    DCI_pdu->nCCE += (1<<(DCI_pdu->dci_alloc[i].L));
  }
  */
}
