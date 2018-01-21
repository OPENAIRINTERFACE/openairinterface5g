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

/*! \file rar_tools.c
 * \brief random access tools
 * \author Raymond Knopp and navid nikaein
 * \date 2011 - 2014
 * \version 1.0
 * @ingroup _mac

 */

#include "defs.h"
#include "proto.h"
#include "extern.h"
#include "SIMULATION/TOOLS/defs.h"
#include "UTIL/LOG/log.h"
#include "OCG.h"
#include "OCG_extern.h"
#include "UTIL/OPT/opt.h"
#include "common/ran_context.h"

#define DEBUG_RAR

extern unsigned int localRIV2alloc_LUT25[512];
extern unsigned int distRIV2alloc_LUT25[512];
extern unsigned short RIV2nb_rb_LUT25[512];
extern unsigned short RIV2first_rb_LUT25[512];
extern RAN_CONTEXT_t RC;

//------------------------------------------------------------------------------
unsigned short
fill_rar(const module_id_t module_idP,
	 const int CC_id,
	 RA_t * ra,
	 const frame_t frameP,
	 uint8_t * const dlsch_buffer,
	 const uint16_t N_RB_UL, const uint8_t input_buffer_length)
//------------------------------------------------------------------------------
{

    RA_HEADER_RAPID *rarh = (RA_HEADER_RAPID *) dlsch_buffer;
    uint8_t *rar = (uint8_t *) (dlsch_buffer + 1);


    // subheader fixed
    rarh->E = 0;		// First and last RAR
    rarh->T = 1;		// 0 for E/T/R/R/BI subheader, 1 for E/T/RAPID subheader
    rarh->RAPID = ra->preamble_index;	// Respond to Preamble 0 only for the moment
    rar[4] = (uint8_t) (ra->rnti >> 8);
    rar[5] = (uint8_t) (ra->rnti & 0xff);
    //ra->timing_offset = 0;
    ra->timing_offset /= 16;	//T_A = N_TA/16, where N_TA should be on a 30.72Msps
    rar[0] = (uint8_t) (ra->timing_offset >> (2 + 4));	// 7 MSBs of timing advance + divide by 4
    rar[1] = (uint8_t) (ra->timing_offset << (4 - 2)) & 0xf0;	// 4 LSBs of timing advance + divide by 4
    ra->msg3_first_rb = 6;
    ra->msg3_nb_rb = 1;
    uint16_t rballoc = mac_computeRIV(N_RB_UL, ra->msg3_first_rb, ra->msg3_nb_rb);	// first PRB only for UL Grant
    rar[1] |= (rballoc >> 7) & 7;	// Hopping = 0 (bit 3), 3 MSBs of rballoc
    rar[2] = ((uint8_t) (rballoc & 0xff)) << 1;	// 7 LSBs of rballoc
    ra->msg3_mcs = 10;
    ra->msg3_TPC = 3;
    ra->msg3_ULdelay = 0;
    ra->msg3_cqireq = 0;
    rar[2] |= ((ra->msg3_mcs & 0x8) >> 3);	// mcs 10
    rar[3] =
	(((ra->msg3_mcs & 0x7) << 5)) | ((ra->msg3_TPC & 7) << 2) |
	((ra->msg3_ULdelay & 1) << 1) | (ra->msg3_cqireq & 1);

    if (opt_enabled) {
	trace_pdu(1, dlsch_buffer, input_buffer_length, module_idP, 2, 1,
		  RC.mac[module_idP]->frame, RC.mac[module_idP]->subframe,
		  0, 0);
	LOG_D(OPT,
	      "[eNB %d][RAPROC] CC_id %d RAR Frame %d trace pdu for rnti %x and  rapid %d size %d\n",
	      module_idP, CC_id, frameP, ra->rnti, rarh->RAPID,
	      input_buffer_length);
    }

    return (ra->rnti);
}

#ifdef Rel14
//------------------------------------------------------------------------------
unsigned short
fill_rar_br(eNB_MAC_INST * eNB,
	    int CC_id,
	    RA_t * ra,
	    const frame_t frameP,
	    const sub_frame_t subframeP,
	    uint8_t * const dlsch_buffer, const uint8_t ce_level)
//------------------------------------------------------------------------------
{

    RA_HEADER_RAPID *rarh = (RA_HEADER_RAPID *) dlsch_buffer;
    COMMON_channels_t *cc = &eNB->common_channels[CC_id];
    uint8_t *rar = (uint8_t *) (dlsch_buffer + 1);
    //  uint8_t nb,reps;
    uint8_t rballoc;
    uint8_t mcs, TPC, ULdelay, cqireq;
    int input_buffer_length;


    AssertFatal(ra != NULL, "RA is null \n");

    // subheader fixed
    rarh->E = 0;		// First and last RAR
    rarh->T = 1;		// 0 for E/T/R/R/BI subheader, 1 for E/T/RAPID subheader
    rarh->RAPID = ra->preamble_index;	// Respond to Preamble 0 only for the moment
    ra->timing_offset /= 16;	//T_A = N_TA/16, where N_TA should be on a 30.72Msps
    rar[0] = (uint8_t) (ra->timing_offset >> (2 + 4));	// 7 MSBs of timing advance + divide by 4
    rar[1] = (uint8_t) (ra->timing_offset << (4 - 2)) & 0xf0;	// 4 LSBs of timing advance + divide by 4

    int N_NB_index;

    AssertFatal(1 == 0, "RAR for BL/CE Still to be finished ...\n");

    // Copy the Msg2 narrowband
    ra->msg34_narrowband = ra->msg2_narrowband;

    if (ce_level < 2) {		//CE Level 0,1, CEmodeA
	input_buffer_length = 6;

	N_NB_index = get_numnarrowbandbits(cc->mib->message.dl_Bandwidth);

	rar[4] = (uint8_t) (ra->rnti >> 8);
	rar[5] = (uint8_t) (ra->rnti & 0xff);
	//cc->ra[ra_idx].timing_offset = 0;
	//    nb      = 0;
	rballoc = mac_computeRIV(6, 1 + ce_level, 1);	// one PRB only for UL Grant in position 1+ce_level within Narrowband
	rar[1] |= (rballoc & 15) << (4 - N_NB_index);	// Hopping = 0 (bit 3), 3 MSBs of rballoc

	//    reps    = 4;
	mcs = 7;
	TPC = 3;		// no power increase
	ULdelay = 0;
	cqireq = 0;
	rar[2] |= ((mcs & 0x8) >> 3);	// mcs 10
	rar[3] =
	    (((mcs & 0x7) << 5)) | ((TPC & 7) << 2) | ((ULdelay & 1) << 1)
	    | (cqireq & 1);
    } else {			// CE level 2,3 => CEModeB

	input_buffer_length = 5;

	rar[3] = (uint8_t) (ra->rnti >> 8);
	rar[4] = (uint8_t) (ra->rnti & 0xff);
    }
    LOG_D(MAC,
	  "[RAPROC] Frame %d Generating RAR BR (%02x|%02x.%02x.%02x.%02x.%02x.%02x) for ce_level %d, CRNTI %x,preamble %d/%d,TIMING OFFSET %d\n",
	  frameP, *(uint8_t *) rarh, rar[0], rar[1], rar[2], rar[3],
	  rar[4], rar[5], ce_level, ra->rnti, rarh->RAPID,
	  ra->preamble_index, ra->timing_offset);

    if (opt_enabled) {
	trace_pdu(1, dlsch_buffer, input_buffer_length, eNB->Mod_id, 2, 1,
		  eNB->frame, eNB->subframe, 0, 0);
	LOG_D(OPT,
	      "[RAPROC] RAR Frame %d trace pdu for rnti %x and  rapid %d size %d\n",
	      frameP, ra->rnti, rarh->RAPID, input_buffer_length);
    }

    return (ra->rnti);
}
#endif

