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

#define UL_RB_ALLOC mac_xface->computeRIV(mac_xface->frame_parms->N_RB_UL,0,24)
#define BCCH_RB_ALLOC mac_xface->computeRIV(mac_xface->frame_parms->N_RB_UL,0,4)
#define RA_RB_ALLOC mac_xface->computeRIV(mac_xface->frame_parms->N_RB_UL,0,4)
#define DLSCH_RB_ALLOC 0x1fff
#include "extern.h"


void init_transport_channels(unsigned char transmission_mode)
{

    // init DCI structures for testing

    UL_alloc_pdu.type = 0;
    UL_alloc_pdu.hopping = 0;
    UL_alloc_pdu.rballoc = UL_RB_ALLOC;
    UL_alloc_pdu.mcs = 2;
    UL_alloc_pdu.ndi = 1;
    UL_alloc_pdu.TPC = 0;
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

    DLSCH_alloc_pdu1A.type = 1;
    DLSCH_alloc_pdu1A.vrb_type = 0;
    DLSCH_alloc_pdu1A.rballoc = BCCH_RB_ALLOC;
    DLSCH_alloc_pdu1A.ndi = 1;
    DLSCH_alloc_pdu1A.rv = 1;
    DLSCH_alloc_pdu1A.mcs = 2;
    DLSCH_alloc_pdu1A.harq_pid = 0;
    DLSCH_alloc_pdu1A.TPC = 1;	// set to 3 PRB

    DLSCH_alloc_pdu1A_fdd.type = 1;
    DLSCH_alloc_pdu1A_fdd.vrb_type = 0;
    DLSCH_alloc_pdu1A_fdd.rballoc = BCCH_RB_ALLOC;
    DLSCH_alloc_pdu1A_fdd.ndi = 1;
    DLSCH_alloc_pdu1A_fdd.rv = 1;
    DLSCH_alloc_pdu1A_fdd.mcs = 2;
    DLSCH_alloc_pdu1A_fdd.harq_pid = 0;
    DLSCH_alloc_pdu1A_fdd.TPC = 1;	// set to 3 PRB

    RA_alloc_pdu.type = 1;
    RA_alloc_pdu.vrb_type = 0;
    RA_alloc_pdu.rballoc = RA_RB_ALLOC;
    RA_alloc_pdu.ndi = 1;
    RA_alloc_pdu.rv = 0;
    RA_alloc_pdu.mcs = 0;
    RA_alloc_pdu.harq_pid = 0;
    RA_alloc_pdu.TPC = 1;

    RA_alloc_pdu_fdd.type = 1;
    RA_alloc_pdu_fdd.vrb_type = 0;
    RA_alloc_pdu_fdd.rballoc = RA_RB_ALLOC;
    RA_alloc_pdu_fdd.ndi = 1;
    RA_alloc_pdu_fdd.rv = 1;
    RA_alloc_pdu_fdd.mcs = 1;
    RA_alloc_pdu_fdd.harq_pid = 0;
    RA_alloc_pdu_fdd.TPC = 1;


    DLSCH_alloc_pdu1.rballoc = 0xf;
    DLSCH_alloc_pdu1.TPC = 0;
    DLSCH_alloc_pdu1.dai = 0;
    DLSCH_alloc_pdu1.harq_pid = 0;
    DLSCH_alloc_pdu1.tb_swap = 0;
    DLSCH_alloc_pdu1.mcs1 = 4;
    DLSCH_alloc_pdu1.ndi1 = 1;
    DLSCH_alloc_pdu1.rv1 = 0;

    // Forget second codeword
    if (transmission_mode == 6) {
	DLSCH_alloc_pdu1.tpmi = 5;	// PUSCH_PRECODING0
    } else {
	DLSCH_alloc_pdu1.tpmi = 0;
    }

    DLSCH_alloc_pdu2.rah = 0;
    DLSCH_alloc_pdu2.rballoc = DLSCH_RB_ALLOC;
    DLSCH_alloc_pdu2.TPC = 0;
    DLSCH_alloc_pdu2.dai = 0;
    DLSCH_alloc_pdu2.harq_pid = 0;
    DLSCH_alloc_pdu2.tb_swap = 0;
    DLSCH_alloc_pdu2.mcs1 = 4;
    DLSCH_alloc_pdu2.ndi1 = 1;
    DLSCH_alloc_pdu2.rv1 = 0;

    // Forget second codeword
    if (transmission_mode == 6) {
	DLSCH_alloc_pdu2.tpmi = 5;	// PUSCH_PRECODING0
    } else {
	DLSCH_alloc_pdu2.tpmi = 0;
    }
}
