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

/*! \file ue_procedures.c
 * \brief procedures related to UE
 * \author  Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \version 1
 * \email: navid.nikaein@eurecom.fr
 * @ingroup _mac

 */

#ifdef EXMIMO
#include <pthread.h>
#endif

#include "extern.h"
#include "defs.h"
#include "proto.h"
#ifdef PHY_EMUL
# include "SIMULATION/PHY_EMULATION/impl_defs.h"
#else
# include "SCHED/defs.h"
# include "PHY/impl_defs_top.h"
#endif
#include "PHY_INTERFACE/defs.h"
#include "PHY_INTERFACE/extern.h"
#include "COMMON/mac_rrc_primitives.h"

#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "RRC/LITE/extern.h"
#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"

#ifdef PHY_EMUL
# include "SIMULATION/simulation_defs.h"
#endif
#include "pdcp.h"

#if defined(ENABLE_ITTI)
# include "intertask_interface.h"
#endif

#include "assertions.h"

#include "SIMULATION/TOOLS/defs.h" // for taus

#define DEBUG_HEADER_PARSING 1
#define ENABLE_MAC_PAYLOAD_DEBUG 1

/*
#ifndef USER_MODE
#define msg debug_msg
#endif
 */
mapping BSR_names[] = {
  {"NONE", 0},
  {"SHORT BSR", 1},
  {"TRUNCATED BSR", 2},
  {"LONG BSR", 3},
  {"PADDING BSR", 4},
  {NULL, -1}
};


void ue_init_mac(module_id_t module_idP)
{
  int i;
  // default values as deined in 36.331 sec 9.2.2
  LOG_I(MAC,"[UE%d] Applying default macMainConfig\n",module_idP);
  //UE_mac_inst[module_idP].scheduling_info.macConfig=NULL;
  UE_mac_inst[module_idP].scheduling_info.retxBSR_Timer= MAC_MainConfig__ul_SCH_Config__retxBSR_Timer_sf2560;
  UE_mac_inst[module_idP].scheduling_info.periodicBSR_Timer=MAC_MainConfig__ul_SCH_Config__periodicBSR_Timer_infinity;
  UE_mac_inst[module_idP].scheduling_info.periodicPHR_Timer = MAC_MainConfig__phr_Config__setup__periodicPHR_Timer_sf20;
  UE_mac_inst[module_idP].scheduling_info.prohibitPHR_Timer = MAC_MainConfig__phr_Config__setup__prohibitPHR_Timer_sf20;
  UE_mac_inst[module_idP].scheduling_info.PathlossChange_db = MAC_MainConfig__phr_Config__setup__dl_PathlossChange_dB1;
  UE_mac_inst[module_idP].PHR_state = MAC_MainConfig__phr_Config_PR_setup;
  UE_mac_inst[module_idP].scheduling_info.SR_COUNTER=0;
  UE_mac_inst[module_idP].scheduling_info.sr_ProhibitTimer=0;
  UE_mac_inst[module_idP].scheduling_info.sr_ProhibitTimer_Running=0;
  UE_mac_inst[module_idP].scheduling_info.maxHARQ_Tx=MAC_MainConfig__ul_SCH_Config__maxHARQ_Tx_n5;
  UE_mac_inst[module_idP].scheduling_info.ttiBundling=0;
  UE_mac_inst[module_idP].scheduling_info.drx_config=NULL;
  UE_mac_inst[module_idP].scheduling_info.phr_config=NULL;
  UE_mac_inst[module_idP].scheduling_info.periodicBSR_SF  =  get_sf_periodicBSRTimer(UE_mac_inst[module_idP].scheduling_info.periodicBSR_Timer);
  UE_mac_inst[module_idP].scheduling_info.retxBSR_SF     =  get_sf_retxBSRTimer(UE_mac_inst[module_idP].scheduling_info.retxBSR_Timer);
  UE_mac_inst[module_idP].scheduling_info.periodicPHR_SF =  get_sf_perioidicPHR_Timer(UE_mac_inst[module_idP].scheduling_info.periodicPHR_Timer);
  UE_mac_inst[module_idP].scheduling_info.prohibitPHR_SF =  get_sf_prohibitPHR_Timer(UE_mac_inst[module_idP].scheduling_info.prohibitPHR_Timer);
  UE_mac_inst[module_idP].scheduling_info.PathlossChange_db =  get_db_dl_PathlossChange(UE_mac_inst[module_idP].scheduling_info.PathlossChange);

  for (i=0; i < MAX_NUM_LCID; i++) {
    LOG_D(MAC,"[UE%d] Applying default logical channel config for LCGID %d\n",module_idP,i);
    UE_mac_inst[module_idP].scheduling_info.Bj[i]=-1;
    UE_mac_inst[module_idP].scheduling_info.bucket_size[i]=-1;

    if (i < DTCH) { // initilize all control channels lcgid to 0
      UE_mac_inst[module_idP].scheduling_info.LCGID[i]=0;
    } else { // initialize all the data channels lcgid to 1
      UE_mac_inst[module_idP].scheduling_info.LCGID[i]=1;
    }

    UE_mac_inst[module_idP].scheduling_info.LCID_status[i]=0;
  }

#ifdef CBA

  for (i=0; i <NUM_MAX_CBA_GROUP; i++) {
    UE_mac_inst[module_idP].cba_last_access[i]= round(uniform_rngen(1,30));
  }

#endif
}


unsigned char *parse_header(unsigned char *mac_header,
                            unsigned char *num_ce,
                            unsigned char *num_sdu,
                            unsigned char *rx_ces,
                            unsigned char *rx_lcids,
                            unsigned short *rx_lengths,
                            unsigned short tb_length)
{

  unsigned char not_done=1,num_ces=0,num_sdus=0,lcid, num_sdu_cnt;
  unsigned char *mac_header_ptr = mac_header;
  unsigned short length,ce_len=0;

  while (not_done==1) {

    if (((SCH_SUBHEADER_FIXED *)mac_header_ptr)->E == 0) {
      //      printf("E=0\n");
      not_done = 0;
    }

    lcid = ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->LCID;

    if (lcid < UE_CONT_RES) {
      //printf("[MAC][UE] header %x.%x.%x\n",mac_header_ptr[0],mac_header_ptr[1],mac_header_ptr[2]);
      if (not_done==0) {// last MAC SDU, length is implicit
        mac_header_ptr++;
        length = tb_length-(mac_header_ptr-mac_header)-ce_len;

        for (num_sdu_cnt=0; num_sdu_cnt < num_sdus ; num_sdu_cnt++) {
          length -= rx_lengths[num_sdu_cnt];
        }
      } else {
        if (((SCH_SUBHEADER_LONG *)mac_header_ptr)->F == 1) {
          length = ((((SCH_SUBHEADER_LONG *)mac_header_ptr)->L_MSB & 0x7f ) << 8 ) | (((SCH_SUBHEADER_LONG *)mac_header_ptr)->L_LSB & 0xff);
          mac_header_ptr += 3;
#ifdef DEBUG_HEADER_PARSING
          LOG_D(MAC,"[UE] parse long sdu, size %x \n",length);
#endif

        }  else { //if (((SCH_SUBHEADER_SHORT *)mac_header_ptr)->F == 0) {
          length = ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->L;
          mac_header_ptr += 2;
        }
      }

#ifdef DEBUG_HEADER_PARSING
      LOG_D(MAC,"[UE] sdu %d lcid %d length %d (offset now %d)\n",
            num_sdus,lcid,length,mac_header_ptr-mac_header);
#endif
      rx_lcids[num_sdus] = lcid;
      rx_lengths[num_sdus] = length;
      num_sdus++;
    } else { // This is a control element subheader
      if (lcid == SHORT_PADDING) {
        mac_header_ptr++;
      } else {
        rx_ces[num_ces] = lcid;
        num_ces++;
        mac_header_ptr ++;

        if (lcid==TIMING_ADV_CMD) {
          ce_len++;
        } else if (lcid==UE_CONT_RES) {
          ce_len+=6;
        }
      }

#ifdef DEBUG_HEADER_PARSING
      LOG_D(MAC,"[UE] ce %d lcid %d (offset now %d)\n",num_ces,lcid,mac_header_ptr-mac_header);
#endif
    }
  }

  *num_ce = num_ces;
  *num_sdu = num_sdus;

  return(mac_header_ptr);
}

uint32_t ue_get_SR(module_id_t module_idP,int CC_id,frame_t frameP,uint8_t eNB_id,uint16_t rnti, sub_frame_t subframe)
{

  // no UL-SCH resources available for this tti && UE has a valid PUCCH resources for SR configuration for this tti
  //  int MGL=6;// measurement gap length in ms
  int MGRP       = 0; // measurement gap repetition period in ms
  int gapOffset  = -1;
  int T          = 0;

  DevCheck(module_idP < (int)NB_UE_INST, module_idP, NB_UE_INST, 0);

  if (CC_id>0) {
    LOG_E(MAC,"Transmission on secondary CCs is not supported yet\n");
    mac_xface->macphy_exit("MAC FATAL  CC_id>0");
    return 0;
  }

  // determin the measurement gap
  if (UE_mac_inst[module_idP].measGapConfig !=NULL) {
    if (UE_mac_inst[module_idP].measGapConfig->choice.setup.gapOffset.present == MeasGapConfig__setup__gapOffset_PR_gp0) {
      MGRP= 40;
      gapOffset= UE_mac_inst[module_idP].measGapConfig->choice.setup.gapOffset.choice.gp0;
    } else if (UE_mac_inst[module_idP].measGapConfig->choice.setup.gapOffset.present == MeasGapConfig__setup__gapOffset_PR_gp1) {
      MGRP= 80;
      gapOffset= UE_mac_inst[module_idP].measGapConfig->choice.setup.gapOffset.choice.gp1;
    } else {
      LOG_W(MAC, "Measurement GAP offset is unknown\n");
    }

    T=MGRP/10;
    DevAssert( T != 0 );

    //check the measurement gap and sr prohibit timer
    if ((subframe ==  gapOffset %10) && ((frameP %T) == (floor(gapOffset/10)))
        && (UE_mac_inst[module_idP].scheduling_info.sr_ProhibitTimer_Running ==0)) {
      UE_mac_inst[module_idP].scheduling_info.SR_pending=1;
      return(0);
    }
  }

  if ((UE_mac_inst[module_idP].physicalConfigDedicated != NULL) &&
      (UE_mac_inst[module_idP].scheduling_info.SR_pending==1) &&
      (UE_mac_inst[module_idP].scheduling_info.SR_COUNTER <
       (1<<(2+UE_mac_inst[module_idP].physicalConfigDedicated->schedulingRequestConfig->choice.setup.dsr_TransMax)))
     ) {
    LOG_D(MAC,"[UE %d][SR %x] Frame %d subframe %d PHY asks for SR (SR_COUNTER/dsr_TransMax %d/%d), SR_pending %d\n",
          module_idP,rnti,frameP,subframe,
          UE_mac_inst[module_idP].scheduling_info.SR_COUNTER,
          (1<<(2+UE_mac_inst[module_idP].physicalConfigDedicated->schedulingRequestConfig->choice.setup.dsr_TransMax)),
          UE_mac_inst[module_idP].scheduling_info.SR_pending);

    UE_mac_inst[module_idP].scheduling_info.SR_COUNTER++;

    // start the sr-prohibittimer : rel 9 and above
    if (UE_mac_inst[module_idP].scheduling_info.sr_ProhibitTimer > 0) { // timer configured
      UE_mac_inst[module_idP].scheduling_info.sr_ProhibitTimer--;
      UE_mac_inst[module_idP].scheduling_info.sr_ProhibitTimer_Running=1;
    } else {
      UE_mac_inst[module_idP].scheduling_info.sr_ProhibitTimer_Running=0;
    }

    LOG_D(MAC,"[UE %d][SR %x] Frame %d subframe %d send SR indication (SR_COUNTER/dsr_TransMax %d/%d), SR_pending %d\n",
          module_idP,rnti,frameP,subframe,
          UE_mac_inst[module_idP].scheduling_info.SR_COUNTER,
          (1<<(2+UE_mac_inst[module_idP].physicalConfigDedicated->schedulingRequestConfig->choice.setup.dsr_TransMax)),
          UE_mac_inst[module_idP].scheduling_info.SR_pending);

    //UE_mac_inst[module_idP].ul_active =1;
    return(1); //instruct phy to signal SR
  } else {
    // notify RRC to relase PUCCH/SRS
    // clear any configured dl/ul
    // initiate RA
    UE_mac_inst[module_idP].scheduling_info.SR_pending=0;
    UE_mac_inst[module_idP].scheduling_info.SR_COUNTER=0;
    return(0);
  }
}

//------------------------------------------------------------------------------
void
ue_send_sdu(module_id_t module_idP,
	    uint8_t CC_id,
	    frame_t frameP,
	    uint8_t* sdu,
	    uint16_t sdu_len,
	    uint8_t eNB_index) {

  unsigned char rx_ces[MAX_NUM_CE],num_ce,num_sdu,i,*payload_ptr;
  unsigned char rx_lcids[NB_RB_MAX];
  unsigned short rx_lengths[NB_RB_MAX];
  unsigned char *tx_sdu;

  start_meas(&UE_mac_inst[module_idP].rx_dlsch_sdu);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SEND_SDU, VCD_FUNCTION_IN);

  LOG_T(MAC,"sdu: %x.%x.%x\n",sdu[0],sdu[1],sdu[2]);

  if (opt_enabled) {
    trace_pdu(1, sdu, sdu_len, module_idP, 3, UE_mac_inst[module_idP].crnti,
              UE_mac_inst[module_idP].subframe, 0, 0); 
    LOG_D(OPT,"[UE %d][DLSCH] Frame %d trace pdu for rnti %x  with size %d\n",
          module_idP, frameP, UE_mac_inst[module_idP].crnti, sdu_len);
  }

  payload_ptr = parse_header(sdu,&num_ce,&num_sdu,rx_ces,rx_lcids,rx_lengths,sdu_len);

#ifdef DEBUG_HEADER_PARSING
  LOG_D(MAC,"[UE %d] ue_send_sdu : Frame %d eNB_index %d : num_ce %d num_sdu %d\n",module_idP,
        frameP,eNB_index,num_ce,num_sdu);
#endif

#if defined(ENABLE_MAC_PAYLOAD_DEBUG)
  LOG_T(MAC,"[eNB %d] First 32 bytes of DLSCH : \n");

  for (i=0; i<32; i++) {
    LOG_T(MAC,"%x.",sdu[i]);
  }

  LOG_T(MAC,"\n");
#endif

  for (i=0; i<num_ce; i++) {
    //    printf("ce %d : %d\n",i,rx_ces[i]);
    switch (rx_ces[i]) {
    case UE_CONT_RES:

      LOG_I(MAC,"[UE %d][RAPROC] Frame %d : received contention resolution msg: %x.%x.%x.%x.%x.%x, Terminating RA procedure\n",
            module_idP,frameP,payload_ptr[0],payload_ptr[1],payload_ptr[2],payload_ptr[3],payload_ptr[4],payload_ptr[5]);

      if (UE_mac_inst[module_idP].RA_active == 1) {
        LOG_I(MAC,"[UE %d][RAPROC] Frame %d : Clearing RA_active flag\n");
        UE_mac_inst[module_idP].RA_active=0;
        // check if RA procedure has finished completely (no contention)
        tx_sdu = &UE_mac_inst[module_idP].CCCH_pdu.payload[3];

        //Note: 3 assumes sizeof(SCH_SUBHEADER_SHORT) + PADDING CE, which is when UL-Grant has TBS >= 9 (64 bits)
        // (other possibility is 1 for TBS=7 (SCH_SUBHEADER_FIXED), or 2 for TBS=8 (SCH_SUBHEADER_FIXED+PADDING or SCH_SUBHEADER_SHORT)
        for (i=0; i<6; i++)
          if (tx_sdu[i] != payload_ptr[i]) {
            LOG_E(MAC,"[UE %d][RAPROC] Contention detected, RA failed\n",module_idP);
            mac_xface->ra_failed(module_idP,CC_id,eNB_index);
            UE_mac_inst[module_idP].RA_contention_resolution_timer_active = 0;
            VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SEND_SDU, VCD_FUNCTION_OUT);
            return;
          }

        LOG_I(MAC,"[UE %d][RAPROC] Frame %d : Clearing contention resolution timer\n");
        UE_mac_inst[module_idP].RA_contention_resolution_timer_active = 0;
        mac_xface->ra_succeeded(module_idP,CC_id,eNB_index);
      }

      payload_ptr+=6;
      break;

    case TIMING_ADV_CMD:
#ifdef DEBUG_HEADER_PARSING
      LOG_D(MAC,"[UE] CE %d : UE Timing Advance : %d\n",i,payload_ptr[0]);
#endif
      mac_xface->process_timing_advance(module_idP,CC_id,payload_ptr[0]);
      payload_ptr++;
      break;

    case DRX_CMD:
#ifdef DEBUG_HEADER_PARSING
      LOG_D(MAC,"[UE] CE %d : UE DRX :",i);
#endif
      payload_ptr++;
      break;
    }
  }

  for (i=0; i<num_sdu; i++) {
#ifdef DEBUG_HEADER_PARSING
    LOG_D(MAC,"[UE] SDU %d : LCID %d, length %d\n",i,rx_lcids[i],rx_lengths[i]);
#endif
    
    if (rx_lcids[i] == CCCH) {

      LOG_D(MAC,"[UE %d] rnti %x Frame %d : DLSCH -> DL-CCCH, RRC message (eNB %d, %d bytes)\n",
            module_idP,
            UE_mac_inst[module_idP].crnti,
            frameP,
            eNB_index,
            rx_lengths[i]);

#if defined(ENABLE_MAC_PAYLOAD_DEBUG)
      int j;

      for (j=0; j<rx_lengths[i]; j++) {
        LOG_T(MAC,"%x.",(uint8_t)payload_ptr[j]);
      }

      LOG_T(MAC,"\n");
#endif
      mac_rrc_data_ind(module_idP,
                       CC_id,
                       frameP,0, // unknown subframe
                       UE_mac_inst[module_idP].crnti,
                       CCCH,
                       (uint8_t*)payload_ptr,
                       rx_lengths[i],
                       ENB_FLAG_NO,
                       eNB_index,
                       0);

    } else if ((rx_lcids[i] == DCCH) || (rx_lcids[i] == DCCH1)) {
      LOG_D(MAC,"[UE %d] Frame %d : DLSCH -> DL-DCCH%d, RRC message (eNB %d, %d bytes)\n", module_idP, frameP, rx_lcids[i],eNB_index,rx_lengths[i]);
      mac_rlc_data_ind(module_idP,
                       UE_mac_inst[module_idP].crnti,
		       eNB_index,
                       frameP,
                       ENB_FLAG_NO,
                       MBMS_FLAG_NO,
                       rx_lcids[i],
                       (char *)payload_ptr,
                       rx_lengths[i],
                       1,
                       NULL);
 
    } else if ((rx_lcids[i]  < NB_RB_MAX) && (rx_lcids[i] > DCCH1 )) {
      
      LOG_D(MAC,"[UE %d] Frame %d : DLSCH -> DL-DTCH%d (eNB %d, %d bytes)\n", module_idP, frameP,rx_lcids[i], eNB_index,rx_lengths[i]);

#if defined(ENABLE_MAC_PAYLOAD_DEBUG)
      int j;
      for (j=0;j<rx_lengths[i];j++)
	LOG_T(MAC,"%x.",(unsigned char)payload_ptr[j]);
      LOG_T(MAC,"\n");
#endif
      mac_rlc_data_ind(module_idP,
		       UE_mac_inst[module_idP].crnti,
		       eNB_index,
		       frameP,
		       ENB_FLAG_NO,
		       MBMS_FLAG_NO,
		       rx_lcids[i],
		       (char *)payload_ptr,
		       rx_lengths[i],
		       1,
		       NULL);
    } else {
      LOG_E(MAC,"[UE %d] Frame %d : unknown LCID %d (eNB %d)\n", module_idP, frameP,rx_lcids[i], eNB_index);
    }
    payload_ptr+= rx_lengths[i];
  }
  
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SEND_SDU, VCD_FUNCTION_OUT);
  stop_meas(&UE_mac_inst[module_idP].rx_dlsch_sdu);
}

void ue_decode_si(module_id_t module_idP,int CC_id,frame_t frameP, uint8_t eNB_index, void *pdu,uint16_t len)
{

  start_meas(&UE_mac_inst[module_idP].rx_si);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_SI, VCD_FUNCTION_IN);

  LOG_D(MAC,"[UE %d] Frame %d Sending SI to RRC (LCID Id %d,len %d)\n",module_idP,frameP,BCCH,len);

  mac_rrc_data_ind(module_idP,
                   CC_id,
                   frameP,0, // unknown subframe
                   SI_RNTI,
                   BCCH,
                   (uint8_t *)pdu,
                   len,
                   ENB_FLAG_NO,
                   eNB_index,
                   0);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_DECODE_SI, VCD_FUNCTION_OUT);
  stop_meas(&UE_mac_inst[module_idP].rx_si);
  if (opt_enabled == 1) {
    trace_pdu(0,
	      (uint8_t *)pdu,
	      len,
	      module_idP,
	      4,
	      0xffff,
	      UE_mac_inst[module_idP].subframe,
	      0,
	      0);
    LOG_D(OPT,"[UE %d][BCH] Frame %d trace pdu for CC_id %d rnti %x with size %d\n",
	    module_idP, frameP, CC_id, 0xffff, len);
  }
}

#ifdef Rel10
unsigned char *parse_mch_header(unsigned char *mac_header,
                                unsigned char *num_sdu,
                                unsigned char *rx_lcids,
                                unsigned short *rx_lengths,
                                unsigned short tb_length)
{
  unsigned char not_done=1, num_sdus=0, lcid, i;
  unsigned char *mac_header_ptr = mac_header;
  unsigned short length;

  while (not_done == 1) {
    if (((SCH_SUBHEADER_FIXED *)mac_header_ptr)->E == 0) {
      not_done = 0;
    }

    lcid = ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->LCID;

    if (lcid < SHORT_PADDING) {// subheader for MSI, MCCH or MTCH
      if (not_done == 0) { // last MAC SDU, length is implicit
        mac_header_ptr++;
        length = tb_length- (mac_header_ptr - mac_header);

        for (i=0; i<num_sdus; i++) {
          length -= rx_lengths[i];
        }
      } else { // not the last MAC SDU
        if ( ((SCH_SUBHEADER_LONG *)mac_header_ptr)->F == 1) {// subheader has length of 3octets
          //    length = ((SCH_SUBHEADER_LONG *)mac_header_ptr)->L;
          length = ((((SCH_SUBHEADER_LONG *)mac_header_ptr)->L_MSB & 0x7f ) << 8 ) | (((SCH_SUBHEADER_LONG *)mac_header_ptr)->L_LSB & 0xff);
          mac_header_ptr += 3;
        } else { // subheader has length of 2octets
          length = ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->L;
          mac_header_ptr += 2;
        }
      }

      rx_lcids[num_sdus] = lcid;
      rx_lengths[num_sdus] = length;
      num_sdus++;
    } else { // subheader for padding
      //     if (lcid == SHORT_PADDING)
      mac_header_ptr++;
    }
  }

  *num_sdu = num_sdus;
  return(mac_header_ptr);
}

// this function is for sending mch_sdu from phy to mac
void ue_send_mch_sdu(module_id_t module_idP, uint8_t CC_id, frame_t frameP, uint8_t *sdu, uint16_t sdu_len, uint8_t eNB_index, uint8_t sync_area)
{

  unsigned char num_sdu, i, *payload_ptr;
  unsigned char rx_lcids[NB_RB_MAX];
  unsigned short rx_lengths[NB_RB_MAX];

  start_meas(&UE_mac_inst[module_idP].rx_mch_sdu);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SEND_MCH_SDU, VCD_FUNCTION_IN);

  LOG_D(MAC,"[UE %d] Frame %d : process the mch PDU for sync area %d \n",module_idP,frameP, sync_area);
  LOG_D(MAC,"[UE %d] sdu: %x.%x\n", module_idP,sdu[0], sdu[1]);
  LOG_D(MAC,"[UE %d] parse_mch_header, demultiplex\n",module_idP);

  payload_ptr = parse_mch_header(sdu, &num_sdu, rx_lcids, rx_lengths, sdu_len);
  LOG_D(MAC,"[UE %d] parse_mch_header, found %d sdus\n",module_idP,num_sdu);

  for (i=0; i<num_sdu; i++) {
    if (rx_lcids[i] == MCH_SCHDL_INFO) {
      if (UE_mac_inst[module_idP].mcch_status==1) {
        LOG_I(MAC,"[UE %d] Frame %d : MCH->MSI for sync area %d (eNB %d, %d bytes)\n",module_idP,frameP, sync_area, eNB_index, rx_lengths[i]);
        // ??store necessary scheduling info to ue_mac_inst in order to
        // calculate exact position of interested service (for the complex case has >1 mtch)
        // set msi_status to 1
        UE_mac_inst[module_idP].msi_status = 1;
      }
    } else if (rx_lcids[i] == MCCH_LCHANID) {
      LOG_I(MAC,"[UE %d] Frame %d : SDU %d MCH->MCCH for sync area %d (eNB %d, %d bytes)\n",module_idP,frameP, i, sync_area, eNB_index, rx_lengths[i]);
      mac_rrc_data_ind(module_idP,
                       CC_id,
                       frameP,0, // unknown subframe
                       M_RNTI,
                       MCCH,
                       payload_ptr, rx_lengths[i], 0, eNB_index, sync_area);
    } else if (rx_lcids[i] == MTCH) {
      if (UE_mac_inst[module_idP].msi_status==1) {
        LOG_I(MAC,"[UE %d] Frame %d : MCH->MTCH for sync area %d (eNB %d, %d bytes)\n",module_idP,frameP, sync_area, eNB_index, rx_lengths[i]);

        mac_rlc_data_ind(
          module_idP,
          UE_mac_inst[module_idP].crnti,
          eNB_index,
	  frameP,
          ENB_FLAG_NO,
          MBMS_FLAG_YES,
          MTCH, /*+ (maxDRB + 3),*/
          (char *)payload_ptr,
          rx_lengths[i],
          1,
          NULL);

      }
    } else {
      LOG_W(MAC,"[UE %d] Frame %d : unknown sdu %d rx_lcids[%d]=%d mcch status %d eNB %d \n",
            module_idP,
            frameP,
            rx_lengths[i],
            i,
            rx_lcids[i],
            UE_mac_inst[module_idP].mcch_status, eNB_index);
    }

    payload_ptr += rx_lengths[i];
  }

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SEND_MCH_SDU, VCD_FUNCTION_OUT);
  stop_meas(&UE_mac_inst[module_idP].rx_mch_sdu);
}

int8_t ue_get_mbsfn_sf_alloction (module_id_t module_idP, uint8_t mbsfn_sync_area, unsigned char eNB_index)
{
  // currently there is one-to-one mapping between sf allocation pattern and sync area
  if (mbsfn_sync_area >= MAX_MBSFN_AREA) {
    LOG_W( MAC, "[UE %"PRIu8"] MBSFN synchronization area %"PRIu8" out of range for eNB %"PRIu8"\n", module_idP, mbsfn_sync_area, eNB_index );
    return -1;
  } else if (UE_mac_inst[module_idP].mbsfn_SubframeConfig[mbsfn_sync_area] != NULL) {
    return mbsfn_sync_area;
  } else {
    LOG_W( MAC, "[UE %"PRIu8"] MBSFN Subframe Config pattern %"PRIu8" not found \n", module_idP, mbsfn_sync_area );
    return -1;
  }
}

int ue_query_mch(module_id_t module_idP, uint8_t CC_id, uint32_t frameP, uint32_t subframe, uint8_t eNB_index,uint8_t *sync_area, uint8_t *mcch_active)
{

  int i=0, j=0, ii=0, msi_pos=0, mcch_mcs = - 1;
  int mcch_flag=0, mtch_flag=0, msi_flag=0;
  int mbsfn_period = 0;// 1<<(UE_mac_inst[module_idP].mbsfn_SubframeConfig[0]->radioframeAllocationPeriod);
  int mcch_period = 0;// 32<<(UE_mac_inst[module_idP].mbsfn_AreaInfo[0]->mcch_Config_r9.mcch_RepetitionPeriod_r9);
  int mch_scheduling_period = -1;

  start_meas(&UE_mac_inst[module_idP].ue_query_mch);

  if (UE_mac_inst[module_idP].pmch_Config[0]) {
    mch_scheduling_period = 8<<(UE_mac_inst[module_idP].pmch_Config[0]->mch_SchedulingPeriod_r9);
  }

  for (i=0;
       i< UE_mac_inst[module_idP].num_active_mbsfn_area;
       i++ ) {
    // assume, that there is always a mapping
    if ((j=ue_get_mbsfn_sf_alloction(module_idP,i,eNB_index)) == -1) {
      return -1; // continue;
    }

    ii=0;
    msi_pos=0;
    mbsfn_period = 1<<(UE_mac_inst[module_idP].mbsfn_SubframeConfig[0]->radioframeAllocationPeriod);
    mcch_period = 32<<(UE_mac_inst[module_idP].mbsfn_AreaInfo[0]->mcch_Config_r9.mcch_RepetitionPeriod_r9);

    LOG_D(MAC,
          "[UE %d] Frame %d subframe %d: Checking MBSFN Sync Area %d/%d with SF allocation %d/%d for MCCH and MTCH (mbsfn period %d, mcch period %d,mac sched period (%d,%d))\n",
          module_idP,frameP, subframe,i,UE_mac_inst[module_idP].num_active_mbsfn_area,
          j,UE_mac_inst[module_idP].num_sf_allocation_pattern,mbsfn_period,mcch_period,
          mch_scheduling_period,UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->radioframeAllocationOffset);

    // get the real MCS value
    switch (UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.signallingMCS_r9) {
    case 0:
      mcch_mcs = 2;
      break;

    case 1:
      mcch_mcs = 7;
      break;

    case 2:
      mcch_mcs = 13;
      break;

    case 3:
      mcch_mcs = 19;
      break;
    }

    if (frameP % mbsfn_period == UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->radioframeAllocationOffset) { // MBSFN frameP
      if (UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.present == MBSFN_SubframeConfig__subframeAllocation_PR_oneFrame) { // one-frameP format

        if (UE_mac_inst[module_idP].pmch_Config[0]) {
          //  Find the first subframe in this MCH to transmit MSI
          if (frameP % mch_scheduling_period == UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->radioframeAllocationOffset ) {
            while (ii == 0) {
              ii = UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & (0x80>>msi_pos);
              msi_pos++;
            }
          }
        }

        // Check if the subframe is for MSI, MCCH or MTCHs and Set the correspoding flag to 1
        switch (subframe) {
        case 1:
          if (mac_xface->lte_frame_parms->frame_type == FDD) {
            if ((UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_FDD_SF1) == MBSFN_FDD_SF1) {
              if (msi_pos == 1) {
                msi_flag = 1;
              }

              if ( (frameP % mcch_period == UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9) &&
                   ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF1) == MBSFN_FDD_SF1) ) {
                mcch_flag = 1;
              }

              mtch_flag = 1;
            }
          }

          break;

        case 2:
          if (mac_xface->lte_frame_parms->frame_type == FDD) {
            if ((UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_FDD_SF2) == MBSFN_FDD_SF2) {
              if (msi_pos == 2) {
                msi_flag = 1;
              }

              if ( (frameP % mcch_period == UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9) &&
                   ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF2) == MBSFN_FDD_SF2) ) {
                mcch_flag = 1;
              }

              mtch_flag = 1;
            }
          }

          break;

        case 3:
          if (mac_xface->lte_frame_parms->frame_type == TDD) { // TDD
            if ((UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_TDD_SF3) == MBSFN_TDD_SF3) {
              if (msi_pos == 1) {
                msi_flag = 1;
              }

              if ( (frameP % mcch_period == UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9) &&
                   ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_TDD_SF3) == MBSFN_TDD_SF3) ) {
                mcch_flag = 1;
              }

              mtch_flag = 1;
            }
          } else { // FDD
            if ((UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_FDD_SF3) == MBSFN_FDD_SF3) {
              if (msi_pos == 3) {
                msi_flag = 1;
              }

              if ((frameP % mcch_period == UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9) &&
                  ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF3) == MBSFN_FDD_SF3) ) {
                mcch_flag = 1;
              }

              mtch_flag = 1;
            }
          }

          break;

        case 4:
          if (mac_xface->lte_frame_parms->frame_type == TDD) {
            if ((UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_TDD_SF4) == MBSFN_TDD_SF4) {
              if (msi_pos == 2) {
                msi_flag = 1;
              }

              if ((frameP % mcch_period == UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9) &&
                  ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_TDD_SF4) == MBSFN_TDD_SF4) ) {
                mcch_flag = 1;
              }

              mtch_flag = 1;
            }
          }

          break;

        case 6:
          if (mac_xface->lte_frame_parms->frame_type == FDD) {
            if ((UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_FDD_SF6) == MBSFN_FDD_SF6) {
              if (msi_pos == 4) {
                msi_flag = 1;
              }

              if ((frameP % mcch_period == UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9) &&
                  ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF6) == MBSFN_FDD_SF6) ) {
                mcch_flag = 1;
              }

              mtch_flag = 1;
            }
          }

          break;

        case 7:
          if (mac_xface->lte_frame_parms->frame_type == TDD) { // TDD
            if ((UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_TDD_SF7) == MBSFN_TDD_SF7) {
              if (msi_pos == 3) {
                msi_flag = 1;
              }

              if ((frameP % mcch_period == UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9) &&
                  ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_TDD_SF7) == MBSFN_TDD_SF7) ) {
                mcch_flag = 1;
              }

              mtch_flag = 1;
            }
          } else { // FDD
            if ((UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_FDD_SF7) == MBSFN_FDD_SF7) {
              if (msi_pos == 5) {
                msi_flag = 1;
              }

              if ((frameP % mcch_period == UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9) &&
                  ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF7) == MBSFN_FDD_SF7) ) {
                mcch_flag = 1;
              }

              mtch_flag = 1;
            }
          }

          break;

        case 8:
          if (mac_xface->lte_frame_parms->frame_type == TDD) { //TDD
            if ((UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_TDD_SF8) == MBSFN_TDD_SF8) {
              if (msi_pos == 4) {
                msi_flag = 1;
              }

              if ( (frameP % mcch_period == UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9) &&
                   ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_TDD_SF8) == MBSFN_TDD_SF8) ) {
                mcch_flag = 1;
              }

              mtch_flag = 1;
            }
          } else { // FDD
            if ((UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_FDD_SF8) == MBSFN_FDD_SF8) {
              if (msi_pos == 6) {
                msi_flag = 1;
              }

              if ((frameP % mcch_period == UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9) &&
                  ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_FDD_SF8) == MBSFN_FDD_SF8) ) {
                mcch_flag = 1;
              }

              mtch_flag = 1;
            }
          }

          break;

        case 9:
          if (mac_xface->lte_frame_parms->frame_type == TDD) {
            if ((UE_mac_inst[module_idP].mbsfn_SubframeConfig[j]->subframeAllocation.choice.oneFrame.buf[0] & MBSFN_TDD_SF9) == MBSFN_TDD_SF9) {
              if (msi_pos == 5) {
                msi_flag = 1;
              }

              if ((frameP % mcch_period == UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.mcch_Offset_r9) &&
                  ((UE_mac_inst[module_idP].mbsfn_AreaInfo[i]->mcch_Config_r9.sf_AllocInfo_r9.buf[0] & MBSFN_TDD_SF9) == MBSFN_TDD_SF9) ) {
                mcch_flag = 1;
              }

              mtch_flag = 1;
            }
          }

          break;
        }// end switch

        // sf allocation is non-overlapping
        if ((msi_flag==1) || (mcch_flag==1) || (mtch_flag==1)) {
          LOG_D(MAC,"[UE %d] Frame %d Subframe %d: sync area %d SF alloc %d: msi flag %d, mcch flag %d, mtch flag %d\n",
                module_idP, frameP, subframe,i,j,msi_flag,mcch_flag,mtch_flag);

          *sync_area=i;
          break;
        }
      } else { // four-frameP format
      }
    }
  } // end of for

  stop_meas(&UE_mac_inst[module_idP].ue_query_mch);

  if ( (mcch_flag==1)) { // || (msi_flag==1))
    *mcch_active=1;
  }

  if ( (mcch_flag==1) || ((msi_flag==1) && (UE_mac_inst[module_idP].mcch_status==1)) ) {
    return mcch_mcs;
  } else if ((mtch_flag==1) && (UE_mac_inst[module_idP].msi_status==1)) {
    return UE_mac_inst[module_idP].pmch_Config[0]->dataMCS_r9;
  } else {
    return -1;
  }
}

#endif

unsigned char generate_ulsch_header(uint8_t *mac_header,
                                    uint8_t num_sdus,
                                    uint8_t short_padding,
                                    uint16_t *sdu_lengths,
                                    uint8_t *sdu_lcids,
                                    POWER_HEADROOM_CMD *power_headroom,
                                    uint16_t *crnti,
                                    BSR_SHORT *truncated_bsr,
                                    BSR_SHORT *short_bsr,
                                    BSR_LONG *long_bsr,
                                    unsigned short post_padding)
{

  SCH_SUBHEADER_FIXED *mac_header_ptr = (SCH_SUBHEADER_FIXED *)mac_header;
  unsigned char first_element=0,last_size=0,i;
  unsigned char mac_header_control_elements[16],*ce_ptr;

  LOG_D(MAC,"[UE] Generate ULSCH : num_sdus %d\n",num_sdus);
#ifdef DEBUG_HEADER_PARSING

  for (i=0; i<num_sdus; i++) {
    LOG_T(MAC,"[UE] sdu %d : lcid %d length %d",i,sdu_lcids[i],sdu_lengths[i]);
  }

  LOG_T(MAC,"\n");
#endif
  ce_ptr = &mac_header_control_elements[0];

  if ((short_padding == 1) || (short_padding == 2)) {
    mac_header_ptr->R    = 0;
    mac_header_ptr->E    = 0;
    mac_header_ptr->LCID = SHORT_PADDING;
    first_element=1;
    last_size=1;
  }

  if (short_padding == 2) {
    mac_header_ptr->E = 1;
    mac_header_ptr++;
    mac_header_ptr->R = 0;
    mac_header_ptr->E    = 0;
    mac_header_ptr->LCID = SHORT_PADDING;
    last_size=1;
  }

  if (power_headroom) {
    if (first_element>0) {
      mac_header_ptr->E = 1;
      mac_header_ptr++;
    } else {
      first_element=1;
    }

    mac_header_ptr->R = 0;
    mac_header_ptr->E    = 0;
    mac_header_ptr->LCID = POWER_HEADROOM;
    last_size=1;
    *((POWER_HEADROOM_CMD *)ce_ptr)=(*power_headroom);
    ce_ptr+=sizeof(POWER_HEADROOM_CMD);
    LOG_D(MAC, "phr header size %d\n",sizeof(POWER_HEADROOM_CMD));
  }

  if (crnti) {
#ifdef DEBUG_HEADER_PARSING
    LOG_D(MAC,"[UE] CRNTI : %x (first_element %d)\n",*crnti,first_element);
#endif

    if (first_element>0) {
      mac_header_ptr->E = 1;
      mac_header_ptr++;
    } else {
      first_element=1;
    }

    mac_header_ptr->R    = 0;
    mac_header_ptr->E    = 0;
    mac_header_ptr->LCID = CRNTI;
    last_size=1;
    *((uint16_t *)ce_ptr)=(*crnti);
    ce_ptr+=sizeof(uint16_t);
    //    printf("offset %d\n",ce_ptr-mac_header_control_elements);
  }

  if (truncated_bsr) {
    if (first_element>0) {
      mac_header_ptr->E = 1;
      /*
      printf("last subheader : %x (R%d,E%d,LCID%d)\n",*(unsigned char*)mac_header_ptr,
      ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->R,
      ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->E,
      ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->LCID);
       */
      mac_header_ptr++;
    } else {
      first_element=1;
    }

#ifdef DEBUG_HEADER_PARSING
    LOG_D(MAC,"[UE] Scheduler Truncated BSR Header\n");
#endif
    mac_header_ptr->R = 0;
    mac_header_ptr->E    = 0;
    mac_header_ptr->LCID = TRUNCATED_BSR;
    last_size=1;
    *((BSR_TRUNCATED *)ce_ptr)=(*truncated_bsr);
    ce_ptr+=sizeof(BSR_TRUNCATED);
    //    printf("(cont_res) : offset %d\n",ce_ptr-mac_header_control_elements);

  } else if (short_bsr) {
    if (first_element>0) {
      mac_header_ptr->E = 1;
      /*
      printf("last subheader : %x (R%d,E%d,LCID%d)\n",*(unsigned char*)mac_header_ptr,
      ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->R,
      ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->E,
      ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->LCID);
       */
      mac_header_ptr++;
    } else {
      first_element=1;
    }

#ifdef DEBUG_HEADER_PARSING
    LOG_D(MAC,"[UE] Scheduler SHORT BSR Header\n");
#endif
    mac_header_ptr->R = 0;
    mac_header_ptr->E    = 0;
    mac_header_ptr->LCID = SHORT_BSR;
    last_size=1;
    *((BSR_SHORT *)ce_ptr)=(*short_bsr);
    ce_ptr+=sizeof(BSR_SHORT);

    //    printf("(cont_res) : offset %d\n",ce_ptr-mac_header_control_elements);
  } else if (long_bsr) {
    if (first_element>0) {
      mac_header_ptr->E = 1;
      /*
      printf("last subheader : %x (R%d,E%d,LCID%d)\n",*(unsigned char*)mac_header_ptr,
      ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->R,
      ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->E,
      ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->LCID);
       */
      mac_header_ptr++;
    } else {
      first_element=1;
    }

#ifdef DEBUG_HEADER_PARSING
    LOG_D(MAC,"[UE] Scheduler Long BSR Header\n");
#endif
    mac_header_ptr->R = 0;
    mac_header_ptr->E    = 0;
    mac_header_ptr->LCID = LONG_BSR;
    last_size=1;

    *(ce_ptr)     = (long_bsr->Buffer_size0 << 2) | ((long_bsr->Buffer_size1 & 0x30) >> 4);
    *(ce_ptr + 1) = ((long_bsr->Buffer_size1 & 0x0F) << 4) | ((long_bsr->Buffer_size2 & 0x3C) >> 2);
    *(ce_ptr + 2) = ((long_bsr->Buffer_size2 & 0x03) << 2) | (long_bsr->Buffer_size3 & 0x3F);
    /* Padding */
    *(ce_ptr + 3) = 0;
    ce_ptr += BSR_LONG_SIZE;

    //    printf("(cont_res) : offset %d\n",ce_ptr-mac_header_control_elements);
  }

  //  printf("last_size %d,mac_header_ptr %p\n",last_size,mac_header_ptr);

  for (i=0; i<num_sdus; i++) {
#ifdef DEBUG_HEADER_PARSING
    LOG_T(MAC,"[UE] sdu subheader %d (lcid %d, %d bytes)\n",i,sdu_lcids[i],sdu_lengths[i]);
#endif

    if ((first_element>0)) {
      mac_header_ptr->E = 1;
#ifdef DEBUG_HEADER_PARSING
      LOG_D(MAC,"[UE] last subheader : %x (R%d,E%d,LCID%d)\n",*(unsigned char*)mac_header_ptr,
            ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->R,
            ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->E,
            ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->LCID);
#endif
      mac_header_ptr+=last_size;
      //      printf("last_size %d,mac_header_ptr %p\n",last_size,mac_header_ptr);
    } else {
      first_element=1;

    }

    if (sdu_lengths[i] < 128) {
      ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->R    = 0; // 3
      ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->E    = 0;
      ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->F    = 0;
      ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->LCID = sdu_lcids[i];
      ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->L    = (unsigned char)sdu_lengths[i];
      last_size=2;
#ifdef DEBUG_HEADER_PARSING
      LOG_D(MAC,"[UE] short sdu\n");
      LOG_T(MAC,"[UE] last subheader : %x (R%d,E%d,LCID%d,F%d,L%d)\n",
            ((uint16_t*)mac_header_ptr)[0],
            ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->R,
            ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->E,
            ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->LCID,
            ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->F,
            ((SCH_SUBHEADER_SHORT *)mac_header_ptr)->L);
#endif
    } else {
      ((SCH_SUBHEADER_LONG *)mac_header_ptr)->R    = 0;
      ((SCH_SUBHEADER_LONG *)mac_header_ptr)->E    = 0;
      ((SCH_SUBHEADER_LONG *)mac_header_ptr)->F    = 1;
      ((SCH_SUBHEADER_LONG *)mac_header_ptr)->LCID = sdu_lcids[i];
      ((SCH_SUBHEADER_LONG *)mac_header_ptr)->L_MSB    = ((unsigned short) sdu_lengths[i]>>8)&0x7f;
      ((SCH_SUBHEADER_LONG *)mac_header_ptr)->L_LSB    = (unsigned short) sdu_lengths[i]&0xff;
      ((SCH_SUBHEADER_LONG *)mac_header_ptr)->padding  = 0x00;
      last_size=3;
#ifdef DEBUG_HEADER_PARSING
      LOG_D(MAC,"[UE] long sdu\n");
#endif
    }
  }

  if (post_padding>0) {// we have lots of padding at the end of the packet
    mac_header_ptr->E = 1;
    mac_header_ptr+=last_size;
    // add a padding element
    mac_header_ptr->R    = 0;
    mac_header_ptr->E    = 0;
    mac_header_ptr->LCID = SHORT_PADDING;
    mac_header_ptr++;
  } else { // no end of packet padding
    // last SDU subhead is of fixed type (sdu length implicitly to be computed at UE)
    mac_header_ptr++;
    //mac_header_ptr=last_size; // FIXME: should be ++
  }

  if ((ce_ptr-mac_header_control_elements) > 0) {
    memcpy((void*)mac_header_ptr,mac_header_control_elements,ce_ptr-mac_header_control_elements);
    mac_header_ptr+=(unsigned char)(ce_ptr-mac_header_control_elements);
  }

#ifdef DEBUG_HEADER_PARSING
  LOG_T(MAC," [UE %d] header : ", crnti);

  for (i=0; i<((unsigned char*)mac_header_ptr - mac_header); i++) {
    LOG_T(MAC,"%2x.",mac_header[i]);
  }

  LOG_T(MAC,"\n");
#endif
  return((unsigned char*)mac_header_ptr - mac_header);

}

void ue_get_sdu(module_id_t module_idP,int CC_id,frame_t frameP,sub_frame_t subframe, uint8_t eNB_index,uint8_t *ulsch_buffer,uint16_t buflen, uint8_t *access_mode)
{

  mac_rlc_status_resp_t rlc_status;
  uint8_t dcch_header_len=0,dcch1_header_len=0,dtch_header_len=0,dtch_header_len_last=0;
  uint8_t dcch_header_len_tmp=0, dtch_header_len_tmp=0;
  uint8_t bsr_header_len=0, bsr_ce_len=0, bsr_len=0;
  uint8_t phr_header_len=0, phr_ce_len=0,phr_len=0;
  uint8_t lcid=0;
  uint16_t sdu_lengths[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };
  uint8_t sdu_lcids[8]    = { 0, 0, 0, 0, 0, 0, 0, 0 };
  uint8_t payload_offset=0,num_sdus=0;
  uint8_t ulsch_buff[MAX_ULSCH_PAYLOAD_BYTES];
  uint16_t sdu_length_total=0;
  BSR_SHORT bsr_short;
  BSR_LONG bsr_long;
  BSR_SHORT *bsr_s=&bsr_short;
  BSR_LONG  *bsr_l=&bsr_long;
  POWER_HEADROOM_CMD phr;
  POWER_HEADROOM_CMD *phr_p=&phr;
  unsigned short short_padding=0, post_padding=0;
  int lcgid;
  int j; // used for padding
  // Compute header length

  if (CC_id>0) {
    LOG_E(MAC,"Transmission on secondary CCs is not supported yet\n");
    mac_xface->macphy_exit("MAC FATAL  CC_id>0");
    return;
  }

  start_meas(&UE_mac_inst[module_idP].tx_ulsch_sdu);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_GET_SDU, VCD_FUNCTION_IN);

#ifdef CBA

  if (*access_mode==CBA_ACCESS) {
    LOG_D(MAC,"[UE %d] frameP %d subframe %d try CBA transmission\n",
          module_idP, frameP, subframe);

    //if (UE_mac_inst[module_idP].scheduling_info.LCID_status[DTCH] == LCID_EMPTY)
    if (cba_access(module_idP,frameP,subframe,eNB_index,buflen)==0) {
      *access_mode=POSTPONED_ACCESS;
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_GET_SDU, VCD_FUNCTION_OUT);
      return;
    }

    LOG_I(MAC,"[UE %d] frameP %d subframe %d CBA transmission oppurtunity, tbs %d\n",
          module_idP, frameP, subframe,buflen);
  }

#endif
  dcch_header_len=2;//sizeof(SCH_SUBHEADER_SHORT);
  dcch1_header_len=2;//sizeof(SCH_SUBHEADER_SHORT);
  // hypo length,in case of long header skip the padding byte
  dtch_header_len=(buflen > 128 ) ? 3 : 2 ; //sizeof(SCH_SUBHEADER_LONG)-1 : sizeof(SCH_SUBHEADER_SHORT);
  bsr_header_len = 1;//sizeof(SCH_SUBHEADER_FIXED);
  phr_header_len = 1;//sizeof(SCH_SUBHEADER_FIXED);
  phr_ce_len = (UE_mac_inst[module_idP].PHR_reporting_active == 1) ? 1 /* sizeof(POWER_HEADROOM_CMD)*/: 0;

  if (phr_ce_len > 0) {
    phr_len = phr_ce_len + phr_header_len;
    LOG_D(MAC,"[UE %d] header size info: PHR len %d (ce%d,hdr%d) buff_len %d\n",
          module_idP, phr_len, phr_ce_len, phr_header_len, buflen);
  } else {
    phr_len=0;
  }

  bsr_ce_len = get_bsr_len (module_idP, buflen-phr_len);

  if (bsr_ce_len > 0 ) {
    bsr_len = bsr_ce_len + bsr_header_len;
    LOG_D(MAC,"[UE %d] header size info: dcch %d, dcch1 %d, dtch %d, bsr (ce%d,hdr%d) buff_len %d\n",
          module_idP, dcch_header_len,dcch1_header_len,dtch_header_len, bsr_ce_len, bsr_header_len, buflen);
  } else {
    bsr_len=0;
    //LOG_D(MAC,"[UE %d] Empty buffers, send a long BSR to reset the bsr at eNB \n ",Mod_id);
    //    bsr_ce_len = sizeof(BSR_LONG);
    //bsr_len = bsr_ce_len + bsr_header_len;
  }

  // check for UL bandwidth requests and add SR control element

  // check for UL bandwidth requests and add SR control element

  // Check for DCCH first

  if (UE_mac_inst[module_idP].scheduling_info.LCID_status[DCCH] == LCID_NOT_EMPTY) {

    rlc_status = mac_rlc_status_ind(module_idP, 
				    UE_mac_inst[module_idP].crnti, 
				    eNB_index, 
				    frameP,
				    ENB_FLAG_NO,
				    MBMS_FLAG_NO, // eNB_index
                                    DCCH,
                                    (buflen-dcch_header_len-bsr_len-phr_len));
    LOG_D(MAC, "[UE %d] Frame %d : UL-DCCH -> ULSCH, RRC message has %d bytes to "
          "send (Transport Block size %d, mac header len %d)\n",
          module_idP,frameP, rlc_status.bytes_in_buffer,buflen,dcch_header_len);

    sdu_lengths[0] += mac_rlc_data_req(module_idP, 
				       UE_mac_inst[module_idP].crnti,
				       eNB_index,
				       frameP,
				       ENB_FLAG_NO, 
				       MBMS_FLAG_NO,
                                       DCCH,
                                       (char *)&ulsch_buff[sdu_lengths[0]]);

    sdu_length_total += sdu_lengths[0];
    sdu_lcids[0] = DCCH;
    LOG_D(MAC,"[UE %d] TX Got %d bytes for DCCH\n",module_idP,sdu_lengths[0]);
    num_sdus = 1;
    update_bsr(module_idP, frameP, eNB_index, DCCH, UE_mac_inst[module_idP].scheduling_info.LCGID[DCCH]);
    //header_len +=2;
  } else {
    dcch_header_len=0;
    num_sdus = 0;
  }

  // if the RLC AM is used, then RLC will only provide 2 bytes for ACK
  // in this case, we sould add bsr

  // DCCH1
  if (UE_mac_inst[module_idP].scheduling_info.LCID_status[DCCH1] == LCID_NOT_EMPTY) {

    rlc_status = mac_rlc_status_ind(module_idP, 
				    UE_mac_inst[module_idP].crnti, 
				    eNB_index,
				    frameP,
				    ENB_FLAG_NO,
				    MBMS_FLAG_NO, // eNB_index
                                    DCCH1,
                                    (buflen-bsr_len-phr_len-dcch_header_len-dcch1_header_len-sdu_length_total));

    LOG_D(MAC,"[UE %d] Frame %d : UL-DCCH1 -> ULSCH, RRC message has %d bytes to"
          " send (Transport Block size %d, mac header len %d)\n",
          module_idP,frameP, rlc_status.bytes_in_buffer,buflen,dcch1_header_len);

    sdu_lengths[num_sdus] = mac_rlc_data_req(module_idP, 
					     UE_mac_inst[module_idP].crnti,
					     eNB_index,
					     frameP,
					     ENB_FLAG_NO,
					     MBMS_FLAG_NO,
					     DCCH1,
					     (char *)&ulsch_buff[sdu_lengths[num_sdus]]);
    sdu_length_total += sdu_lengths[num_sdus];
    sdu_lcids[num_sdus] = DCCH1;
    LOG_D(MAC,"[UE %d] TX Got %d bytes for DCCH1\n",module_idP,sdu_lengths[num_sdus]);
    num_sdus++;
    //update_bsr(module_idP, frameP, DCCH1);
    //dcch_header_len +=2; // include dcch1
  } else {
    dcch1_header_len =0;
  }

  dtch_header_len=0;
  dtch_header_len_last=0;
  for (lcid=NB_RB_MAX-1; lcid>=DTCH ; lcid--){
    dtch_header_len+=3; 
    dtch_header_len_last=3;
    
    if ((UE_mac_inst[module_idP].scheduling_info.LCID_status[lcid] == LCID_NOT_EMPTY) &&
	((bsr_len+phr_len+dcch_header_len+dcch1_header_len+dtch_header_len+sdu_length_total) <= buflen)) {

    // optimize the dtch header lenght
    //if ((UE_mac_inst[module_idP].scheduling_info.BSR_bytes[DTCH] > 128) &&
    /*   if (((UE_mac_inst[module_idP].scheduling_info.BSR_bytes[DTCH] >= 128) &&
    ((UE_mac_inst[module_idP].scheduling_info.BSR_bytes[DTCH]+bsr_len+phr_len+dcch_header_len+dcch1_header_len+dtch_header_len) > buflen)&&
    buflen >=128 ))
    dtch_header_len = 3;//sizeof(SCH_SUBHEADER_LONG);
    else
    dtch_header_len = 2;//sizeof(SCH_SUBHEADER_SHORT);
     */
      rlc_status = mac_rlc_status_ind(module_idP, 
				      UE_mac_inst[module_idP].crnti, 
				      eNB_index,
				      frameP,
				      ENB_FLAG_NO,
				      MBMS_FLAG_NO, // eNB_index
				      lcid,
				      buflen-bsr_len-phr_len-dcch_header_len-dcch1_header_len-dtch_header_len-sdu_length_total);

      LOG_D(MAC,"[UE %d] Frame %d : UL-DTCH -> ULSCH%d, %d bytes to send (Transport Block size %d, mac header len %d, BSR byte[%d] %d)\n",
	    module_idP,frameP, lcid, rlc_status.bytes_in_buffer,buflen,dtch_header_len,
	    lcid, UE_mac_inst[module_idP].scheduling_info.BSR_bytes[lcid]);
      
       if (rlc_status.bytes_in_buffer > 0) {
	 sdu_lengths[num_sdus] = mac_rlc_data_req(module_idP,
						  UE_mac_inst[module_idP].crnti,
						  eNB_index,
						  frameP, 
						  ENB_FLAG_NO, 
						  MBMS_FLAG_NO, // eNB_index
						  lcid,
						  (char *)&ulsch_buff[sdu_length_total]);
	 
	 //adjust dtch header
	 LOG_D(MAC,"[UE %d] TX Got %d bytes for DTCH\n",module_idP,sdu_lengths[num_sdus]);
	 sdu_lcids[num_sdus] = lcid;
	 sdu_length_total += sdu_lengths[num_sdus];
	 if (sdu_lengths[num_sdus] < 128) {
	   dtch_header_len --;
	   dtch_header_len_last --;
	 }
	 num_sdus++;
	 UE_mac_inst[module_idP].ul_active = update_bsr(module_idP, frameP, eNB_index,lcid, UE_mac_inst[module_idP].scheduling_info.LCGID[lcid]);
       } else {
	 dtch_header_len -= 3;
       }
    } else { // no rlc pdu : generate the dummy header
      dtch_header_len -= 3;
    }
  }
  
  lcgid= get_bsr_lcgid(module_idP);

  if (lcgid < 0 ) {
    bsr_s = NULL;
    bsr_l = NULL ;
  } else if ((lcgid ==MAX_NUM_LCGID) && (bsr_ce_len == sizeof(BSR_LONG))) {
    bsr_s = NULL;
    bsr_l->Buffer_size0 = UE_mac_inst[module_idP].scheduling_info.BSR[LCGID0];
    bsr_l->Buffer_size1 = UE_mac_inst[module_idP].scheduling_info.BSR[LCGID1];
    bsr_l->Buffer_size2 = UE_mac_inst[module_idP].scheduling_info.BSR[LCGID2];
    bsr_l->Buffer_size3 = UE_mac_inst[module_idP].scheduling_info.BSR[LCGID3];
    LOG_D(MAC, "[UE %d] Frame %d report long BSR (level LCGID0 %d,level LCGID1 %d,level LCGID2 %d,level LCGID3 %d)\n", module_idP,frameP,
          UE_mac_inst[module_idP].scheduling_info.BSR[LCGID0],
          UE_mac_inst[module_idP].scheduling_info.BSR[LCGID1],
          UE_mac_inst[module_idP].scheduling_info.BSR[LCGID2],
          UE_mac_inst[module_idP].scheduling_info.BSR[LCGID3]);
  } else if (bsr_ce_len == sizeof(BSR_SHORT)) {
    bsr_l = NULL;
    bsr_s->LCGID = lcgid;
    bsr_s->Buffer_size = UE_mac_inst[module_idP].scheduling_info.BSR[lcgid];
    LOG_D(MAC,"[UE %d] Frame %d report SHORT BSR with level %d for LCGID %d\n",
          module_idP, frameP, UE_mac_inst[module_idP].scheduling_info.BSR[lcgid],lcgid);
  } else {
    bsr_s = NULL;
    bsr_l = NULL;
  }

  // build PHR and update the timers
  if (phr_ce_len == sizeof(POWER_HEADROOM_CMD)) {
    phr_p->PH = get_phr_mapping(module_idP,CC_id,eNB_index);
    phr_p->R  = 0;
    LOG_D(MAC,"[UE %d] Frame %d report PHR with mapping (%d->%d) for LCID %d\n",
          module_idP,frameP, mac_xface->get_PHR(module_idP,CC_id,eNB_index), phr_p->PH,POWER_HEADROOM);
    update_phr(module_idP,CC_id);
  } else {
    phr_p=NULL;
  }

  LOG_T(MAC,"[UE %d] Frame %d: bsr s %p bsr_l %p, phr_p %p\n",  module_idP,frameP,bsr_s, bsr_l, phr_p);

  if (dtch_header_len == 0 )
    dtch_header_len_last =0;

  // adjust the header length
  dcch_header_len_tmp = dcch_header_len;
  dtch_header_len_tmp = dtch_header_len;

  if (dtch_header_len==0) {
    dcch_header_len = (dcch_header_len>0)? 1: dcch_header_len;
  } else {
    dtch_header_len_last-=1; 
    dtch_header_len= (dtch_header_len >0)? dtch_header_len - dtch_header_len_last : dtch_header_len;   
  }

  if ((buflen-bsr_len-phr_len-dcch_header_len-dcch1_header_len-dtch_header_len-sdu_length_total) <= 2) {
    short_padding = buflen-bsr_len-phr_len-dcch_header_len-dcch1_header_len-dtch_header_len-sdu_length_total;
    post_padding = 0;
  } else {
    if ((buflen-bsr_len-phr_len-dcch_header_len-dcch1_header_len-dtch_header_len-sdu_length_total) == buflen) {
      *access_mode=CANCELED_ACCESS;
    }

    short_padding = 0;

    if (dtch_header_len==0) {
      dcch_header_len = dcch_header_len_tmp;
    } else {
      dtch_header_len= dtch_header_len_tmp;
    }

    post_padding = buflen-bsr_len-phr_len-dcch_header_len-dcch1_header_len-dtch_header_len-sdu_length_total -1 ;
  }

  // Generate header
  // if (num_sdus>0) {

  payload_offset = generate_ulsch_header(ulsch_buffer,  // mac header
                                         num_sdus,      // num sdus
                                         short_padding,            // short pading
                                         sdu_lengths,  // sdu length
                                         sdu_lcids,    // sdu lcid
                                         phr_p,  // power headroom
                                         NULL,  // crnti
                                         NULL,  // truncated bsr
                                         bsr_s, // short bsr
                                         bsr_l,
                                         post_padding); // long_bsr

  LOG_D(MAC,
        "[UE %d] Generate header :bufflen %d  sdu_length_total %d, num_sdus %d, sdu_lengths[0] %d, sdu_lcids[0] %d => payload offset %d,  dcch_header_len %d, dtch_header_len %d, padding %d,post_padding %d, bsr len %d, phr len %d, reminder %d \n",
        module_idP,buflen, sdu_length_total,num_sdus,sdu_lengths[0],sdu_lcids[0],payload_offset, dcch_header_len,  dtch_header_len,
        short_padding,post_padding, bsr_len, phr_len,buflen-sdu_length_total-payload_offset);
  // cycle through SDUs and place in ulsch_buffer
  memcpy(&ulsch_buffer[payload_offset],ulsch_buff,sdu_length_total);

  // fill remainder of DLSCH with random data
  for (j=0; j<(buflen-sdu_length_total-payload_offset); j++) {
    ulsch_buffer[payload_offset+sdu_length_total+j] = (char)(taus()&0xff);
  }
  LOG_D(MAC,"[UE %d][SR] Gave SDU to PHY, clearing any scheduling request\n",
        module_idP,payload_offset, sdu_length_total);
  UE_mac_inst[module_idP].scheduling_info.SR_pending=0;
  UE_mac_inst[module_idP].scheduling_info.SR_COUNTER=0;
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_GET_SDU, VCD_FUNCTION_OUT);
  stop_meas(&UE_mac_inst[module_idP].tx_ulsch_sdu);
  
  if (opt_enabled) {
    trace_pdu(0, ulsch_buffer, buflen, module_idP, 3, UE_mac_inst[module_idP].crnti, UE_mac_inst[module_idP].subframe, 0, 0);
    LOG_D(OPT,"[UE %d][ULSCH] Frame %d trace pdu for rnti %x  with size %d\n",
          module_idP, UE_mac_inst[module_idP].subframe, UE_mac_inst[module_idP].crnti, buflen);
  }
}

//------------------------------------------------------------------------------
// called at each subframe
// Performs :
// 1. Trigger PDCP every 5ms
// 2. Call RRC for link status return to PHY
// 3. Perform SR/BSR procedures for scheduling feedback
// 4. Perform PHR procedures

UE_L2_STATE_t
ue_scheduler(
  const module_id_t    module_idP,
  const frame_t        frameP,
  const sub_frame_t    subframeP,
  const lte_subframe_t directionP,
  const uint8_t        eNB_indexP,
  const int            CC_id)
//------------------------------------------------------------------------------
{

  int lcid; // lcid index
  int TTI= 1;
  int bucketsizeduration = -1;
  int bucketsizeduration_max = -1;
  // mac_rlc_status_resp_t rlc_status[MAX_NUM_LCGID]; // 4
  // int8_t lcg_id;
  struct RACH_ConfigCommon *rach_ConfigCommon = (struct RACH_ConfigCommon *)NULL;
  protocol_ctxt_t   ctxt;
#if defined(ENABLE_ITTI)
  MessageDef   *msg_p;
  const char   *msg_name;
  instance_t    instance;
  int           result;
#endif
  start_meas(&UE_mac_inst[module_idP].ue_scheduler);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SCHEDULER, VCD_FUNCTION_IN);

  PROTOCOL_CTXT_SET_BY_MODULE_ID(&ctxt, module_idP, ENB_FLAG_NO, UE_mac_inst[module_idP].crnti, frameP, subframeP,eNB_indexP);
#if defined(ENABLE_ITTI)

  do {
    // Checks if a message has been sent to MAC sub-task
    itti_poll_msg (TASK_MAC_UE, &msg_p);

    if (msg_p != NULL) {
      msg_name = ITTI_MSG_NAME (msg_p);
      instance = ITTI_MSG_INSTANCE (msg_p);

      switch (ITTI_MSG_ID(msg_p)) {
      case RRC_MAC_CCCH_DATA_REQ:
        LOG_I(MAC, "Received %s from %s: instance %d, frameP %d, eNB_index %d\n",
              msg_name, ITTI_MSG_ORIGIN_NAME(msg_p), instance,
              RRC_MAC_CCCH_DATA_REQ (msg_p).frame, RRC_MAC_CCCH_DATA_REQ (msg_p).enb_index);

        // TODO process CCCH data req.
        break;


      default:
        LOG_E(MAC, "Received unexpected message %s\n", msg_name);
        break;
      }

      result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
      AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
    }
  } while(msg_p != NULL);

#endif

  //Mac_rlc_xface->frameP=frameP;
  //Rrc_xface->Frame_index=Mac_rlc_xface->frameP;
  //if (subframe%5 == 0)
  //LG#ifdef EXMIMO
  pdcp_run(&ctxt);
  //#endif
  UE_mac_inst[module_idP].frame = frameP;
  UE_mac_inst[module_idP].subframe = subframeP;

#ifdef CELLULAR
  rrc_rx_tx(module_idP, frameP, 0, eNB_indexP);
#else

  switch (rrc_rx_tx(&ctxt,
                    eNB_indexP,
                    CC_id)) {
  case RRC_OK:
    break;

  case RRC_ConnSetup_failed:
    LOG_E(MAC,"RRCConnectionSetup failed, returning to IDLE state\n");
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SCHEDULER, VCD_FUNCTION_OUT);
    stop_meas(&UE_mac_inst[module_idP].ue_scheduler);
    return(CONNECTION_LOST);
    break;

  case RRC_PHY_RESYNCH:
    LOG_E(MAC,"RRC Loss of synch, returning PHY_RESYNCH\n");
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SCHEDULER, VCD_FUNCTION_OUT);
    stop_meas(&UE_mac_inst[module_idP].ue_scheduler);
    return(PHY_RESYNCH);

  case RRC_Handover_failed:
    LOG_N(MAC,"Handover failure for UE %d eNB_index %d\n",module_idP,eNB_indexP);
    //Invalid...need to add another MAC UE state for re-connection procedure
    mac_xface->phy_config_afterHO_ue(module_idP,0,eNB_indexP,(MobilityControlInfo_t *)NULL,1);
    //return(3);
    break;

  case RRC_HO_STARTED:
    LOG_I(MAC,"RRC handover, Instruct PHY to start the contention-free PRACH and synchronization\n");
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SCHEDULER, VCD_FUNCTION_OUT);
    stop_meas(&UE_mac_inst[module_idP].ue_scheduler);
    return(PHY_HO_PRACH);

  default:
    break;
  }

#endif

  // Check Contention resolution timer (put in a function later)
  if (UE_mac_inst[module_idP].RA_contention_resolution_timer_active == 1) {

    if (UE_mac_inst[module_idP].radioResourceConfigCommon) {
      rach_ConfigCommon = &UE_mac_inst[module_idP].radioResourceConfigCommon->rach_ConfigCommon;
    } else {
      LOG_E(MAC,"FATAL: radioResourceConfigCommon is NULL!!!\n");
      VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SCHEDULER, VCD_FUNCTION_OUT);
      stop_meas(&UE_mac_inst[module_idP].ue_scheduler);
      mac_xface->macphy_exit("");
      //return(RRC_OK);
    }

    LOG_I(MAC,"Frame %d: Contention resolution timer %d/%d\n",frameP,UE_mac_inst[module_idP].RA_contention_resolution_cnt,
          ((1+rach_ConfigCommon->ra_SupervisionInfo.mac_ContentionResolutionTimer)<<3));

    UE_mac_inst[module_idP].RA_contention_resolution_cnt++;

    if (UE_mac_inst[module_idP].RA_contention_resolution_cnt ==
        ((1+rach_ConfigCommon->ra_SupervisionInfo.mac_ContentionResolutionTimer)<<3)) {
      UE_mac_inst[module_idP].RA_active = 0;
      UE_mac_inst[module_idP].RA_contention_resolution_timer_active = 0;
      // Signal PHY to quit RA procedure
      LOG_E(MAC,"Module id %u Contention resolution timer expired, RA failed\n", module_idP);
      mac_xface->ra_failed(module_idP,0,eNB_indexP);
    }
  }


  // call SR procedure to generate pending SR and BSR for next PUCCH/PUSCH TxOp.  This should implement the procedures
  // outlined in Sections 5.4.4 an 5.4.5 of 36.321
  // Put this in another function

  // Get RLC status info and update Bj for all lcids that are active
  for (lcid=DCCH; lcid <= DTCH; lcid++ ) {
    if ((lcid == 0) ||(UE_mac_inst[module_idP].logicalChannelConfig[lcid])) {
      // meausre the Bj
      if ((directionP == SF_UL)&& (UE_mac_inst[module_idP].scheduling_info.Bj[lcid] >= 0)) {
        if (UE_mac_inst[module_idP].logicalChannelConfig[lcid]->ul_SpecificParameters) {
          bucketsizeduration = UE_mac_inst[module_idP].logicalChannelConfig[lcid]->ul_SpecificParameters->prioritisedBitRate * TTI;
          bucketsizeduration_max = get_ms_bucketsizeduration(UE_mac_inst[module_idP].logicalChannelConfig[lcid]->ul_SpecificParameters->bucketSizeDuration);
        } else {
          LOG_E(MAC,"[UE %d] lcid %d, NULL ul_SpecificParameters\n",module_idP,lcid);
          mac_xface->macphy_exit("");
        }

        if ( UE_mac_inst[module_idP].scheduling_info.Bj[lcid] > bucketsizeduration_max ) {
          UE_mac_inst[module_idP].scheduling_info.Bj[lcid] = bucketsizeduration_max;
        } else {
          UE_mac_inst[module_idP].scheduling_info.Bj[lcid] = bucketsizeduration;
        }
      }

      if (update_bsr(module_idP,frameP, eNB_indexP, lcid, UE_mac_inst[module_idP].scheduling_info.LCGID[lcid])) {
        UE_mac_inst[module_idP].scheduling_info.SR_pending= 1;
        LOG_D(MAC,"[UE %d][SR] Frame %d subframe %d SR for PUSCH is pending for LCGID %d with BSR level %d (%d bytes in RLC)\n",
              module_idP, frameP,subframeP,UE_mac_inst[module_idP].scheduling_info.LCGID[lcid],
              UE_mac_inst[module_idP].scheduling_info.BSR[UE_mac_inst[module_idP].scheduling_info.LCGID[lcid]],
              UE_mac_inst[module_idP].scheduling_info.BSR_bytes[UE_mac_inst[module_idP].scheduling_info.LCGID[lcid]]);
      }
    }
  }

  // UE has no valid phy config dedicated ||  no valid/released  SR
  if ((UE_mac_inst[module_idP].physicalConfigDedicated == NULL)) {
    // cancel all pending SRs
    UE_mac_inst[module_idP].scheduling_info.SR_pending=0;
    UE_mac_inst[module_idP].ul_active=0;
    LOG_T(MAC,"[UE %d] Release all SRs \n", module_idP);
    VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SCHEDULER, VCD_FUNCTION_OUT);
    stop_meas(&UE_mac_inst[module_idP].ue_scheduler);
    return(CONNECTION_OK);
  }

  if ((UE_mac_inst[module_idP].physicalConfigDedicated->schedulingRequestConfig == NULL) ||
      (UE_mac_inst[module_idP].physicalConfigDedicated->schedulingRequestConfig->present == SchedulingRequestConfig_PR_release)) {

    // initiate RA with CRNTI included in msg3 (no contention) as descibed in 36.321 sec 5.1.5

    // cancel all pending SRs
    UE_mac_inst[module_idP].scheduling_info.SR_pending=0;
    UE_mac_inst[module_idP].ul_active=0;
    LOG_T(MAC,"[UE %d] Release all SRs \n", module_idP);
  }

  // Put this in a function
  // Call PHR procedure as described in Section 5.4.6 in 36.321
  if (UE_mac_inst[module_idP].PHR_state == MAC_MainConfig__phr_Config_PR_setup) { // normal operation

    if (UE_mac_inst[module_idP].PHR_reconfigured == 1) { // upon (re)configuration of the power headroom reporting functionality by upper layers
      UE_mac_inst[module_idP].PHR_reporting_active = 1;
      UE_mac_inst[module_idP].PHR_reconfigured = 0;
    } else {
      //LOG_D(MAC,"PHR normal operation %d active %d \n", UE_mac_inst[module_idP].scheduling_info.periodicPHR_SF, UE_mac_inst[module_idP].PHR_reporting_active);
      if ((UE_mac_inst[module_idP].scheduling_info.prohibitPHR_SF <= 0) &&
          ((mac_xface->get_PL(module_idP,0,eNB_indexP) <  UE_mac_inst[module_idP].scheduling_info.PathlossChange_db) ||
           (UE_mac_inst[module_idP].power_backoff_db[eNB_indexP] > UE_mac_inst[module_idP].scheduling_info.PathlossChange_db)))
        // trigger PHR and reset the timer later when the PHR report is sent
      {
        UE_mac_inst[module_idP].PHR_reporting_active = 1;
      } else if (UE_mac_inst[module_idP].PHR_reporting_active ==0 ) {
        UE_mac_inst[module_idP].scheduling_info.prohibitPHR_SF--;
      }

      if (UE_mac_inst[module_idP].scheduling_info.periodicPHR_SF <= 0 )
        // trigger PHR and reset the timer later when the PHR report is sent
      {
        UE_mac_inst[module_idP].PHR_reporting_active = 1;
      } else if (UE_mac_inst[module_idP].PHR_reporting_active == 0 ) {
        UE_mac_inst[module_idP].scheduling_info.periodicPHR_SF--;
      }
    }
  } else {    // release / nothing
    UE_mac_inst[module_idP].PHR_reporting_active = 0; // release PHR
  }

  //If the UE has UL resources allocated for new transmission for this TTI here:
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_UE_SCHEDULER, VCD_FUNCTION_OUT);
  stop_meas(&UE_mac_inst[module_idP].ue_scheduler);
  return(CONNECTION_OK);
}

// to be improved
#ifdef CBA
extern int cba_backoff;
double uniform_rngen(int min, int max)
{
  double random = (double)taus()/((double)0xffffffff);
  return (max - min) * random + min;
}

int cba_access(module_id_t module_idP,frame_t frameP,sub_frame_t subframe, uint8_t eNB_index,uint16_t buflen)
{

  mac_rlc_status_resp_t rlc_status;
  int header_offset=4;
  int rv =0;

  /*
  if (( ((UE_mac_inst[module_idP].scheduling_info.BSR[LCGID1]>0)&&(UE_mac_inst[module_idP].scheduling_info.BSR[LCGID1]<64))   ||
      ((UE_mac_inst[module_idP].scheduling_info.BSR[LCGID2]>0)&&(UE_mac_inst[module_idP].scheduling_info.BSR[LCGID2]<64))   ||
      ((UE_mac_inst[module_idP].scheduling_info.BSR[LCGID3]>0)&&(UE_mac_inst[module_idP].scheduling_info.BSR[LCGID3]<64)) )
      //  && (UE_mac_inst[module_idP].ul_active == 0) // check if the ul is acrtive
      && (UE_mac_inst[module_idP].cba_last_access[0] <= 0) ) { // backoff
      //  LOG_D(MAC,"[UE %d] Frame %d Subframe %d: the current CBA backoff is %d \n", module_idP, frameP, subframe,
      //  UE_mac_inst[module_idP].cba_last_access[0] );

      UE_mac_inst[module_idP].cba_last_access[0]= round(uniform_rngen(1,40));
      LOG_D(MAC,"[UE %d] Frame %d Subframe %d: start a new CBA backoff  %d UL active state %d \n", module_idP, frameP, subframe,
          UE_mac_inst[module_idP].cba_last_access[0], UE_mac_inst[module_idP].ul_active);

      rv=1;
  } else if (( ((UE_mac_inst[module_idP].scheduling_info.BSR[LCGID1]> 0 ))   ||
      ((UE_mac_inst[module_idP].scheduling_info.BSR[LCGID2]> 0 ))   ||
      ((UE_mac_inst[module_idP].scheduling_info.BSR[LCGID3]> 0 )) )
      // && (UE_mac_inst[module_idP].ul_active == 0) // check if the ul is acrtive
      && (UE_mac_inst[module_idP].cba_last_access[0]> 0) ){

      UE_mac_inst[module_idP].cba_last_access[0]-=1;
      LOG_D(MAC,"[UE %d] Frame %d Subframe %d: CBA backoff is decreased by one to %d UL active state %d \n",
          module_idP, frameP, subframe,
          UE_mac_inst[module_idP].cba_last_access[0], UE_mac_inst[module_idP].ul_active);

  } else if (( ((UE_mac_inst[module_idP].scheduling_info.BSR[LCGID1] == 0 ))   &&
               ((UE_mac_inst[module_idP].scheduling_info.BSR[LCGID2] == 0 ))   &&
               ((UE_mac_inst[module_idP].scheduling_info.BSR[LCGID3] ==  0 )) )
             && (UE_mac_inst[module_idP].cba_last_access[0]> 0) ){
    UE_mac_inst[module_idP].cba_last_access[0]-=1;
    }*/

  if ((UE_mac_inst[module_idP].scheduling_info.BSR[LCGID0]>0)&&(UE_mac_inst[module_idP].scheduling_info.BSR[LCGID0]<64) ) {
    return 0;
  }

  if ((UE_mac_inst[module_idP].scheduling_info.BSR[LCGID1] <= 0 ) &&
      (UE_mac_inst[module_idP].scheduling_info.BSR[LCGID2] <= 0 ) &&
      (UE_mac_inst[module_idP].scheduling_info.BSR[LCGID3] <= 0 ) ) {
    return 0;
  }

  if (cba_backoff == 0 ) { // apply probablisitc method
    UE_mac_inst[module_idP].cba_last_access[0]= uniform_rngen(0,1);

    if (uniform_rngen(0,1) > 0.6 ) {
      LOG_I(MAC,"[UE %d] Frame %d Subframe %d: CBA probability-based backoff (%d), UL active state %d \n", module_idP, frameP, subframe,
            cba_backoff,UE_mac_inst[module_idP].ul_active);

      rv=1;
    }
  } else {

    if (UE_mac_inst[module_idP].cba_last_access[0] <= 0) {
      UE_mac_inst[module_idP].cba_last_access[0]= round(uniform_rngen(1,cba_backoff));

      LOG_I(MAC,"[UE %d] Frame %d Subframe %d: start a new CBA backoff  %d/%d UL active state %d \n", module_idP, frameP, subframe,
            UE_mac_inst[module_idP].cba_last_access[0], cba_backoff,UE_mac_inst[module_idP].ul_active);

      rv = 1;
      /*
      rlc_status = mac_rlc_status_ind(module_idP, UE_mac_inst[module_idP].crnti,frameP,ENB_FLAG_NO,MBMS_FLAG_NO, // eNB_index
      DTCH,
      0);

      if ((
      //  (rlc_status.pdus_in_buffer > 0 )           &&
      // (UE_mac_inst[module_idP].ul_active == 0)  && // check if the ul is acrtive
      (rlc_status.head_sdu_is_segmented  == 0 )          &&
      ((rlc_status.head_sdu_remaining_size_to_send + header_offset ) <= buflen )
      )){
      rv = 1;

      UE_mac_inst[module_idP].cba_last_access[0]= round(uniform_rngen(1,30));
      LOG_D(MAC,"[UE %d] Frame %d Subframe %d: start a new CBA backoff  %d UL active state %d \n", module_idP, frameP, subframe,
      UE_mac_inst[module_idP].cba_last_access[0], UE_mac_inst[module_idP].ul_active);
      */
    } else {
      UE_mac_inst[module_idP].cba_last_access[0]-=1;
      LOG_D(MAC,"[UE %d] Frame %d Subframe %d: wait for backoff to expire (%d) CBA UL active state %d \n",
            module_idP, frameP, subframe,
            UE_mac_inst[module_idP].cba_last_access[0], UE_mac_inst[module_idP].ul_active);
    }
  }

  return rv;
  /*
    if (( ((UE_mac_inst[module_idP].scheduling_info.BSR[LCGID1]>0)&&(UE_mac_inst[module_idP].scheduling_info.BSR[LCGID1]<64))   ||
    ((UE_mac_inst[module_idP].scheduling_info.BSR[LCGID2]>0)&&(UE_mac_inst[module_idP].scheduling_info.BSR[LCGID2]<64))   ||
    ((UE_mac_inst[module_idP].scheduling_info.BSR[LCGID3]>0)&&(UE_mac_inst[module_idP].scheduling_info.BSR[LCGID3]<64)) )
  //  && (UE_mac_inst[module_idP].ul_active == 0) // check if the ul is acrtive
  && (UE_mac_inst[module_idP].cba_last_access[0] <= 0) ) {

      UE_mac_inst[module_idP].cba_last_access[0]= round(uniform_rngen(1,cba_backoff));

      LOG_I(MAC,"[UE %d] Frame %d Subframe %d: start a new CBA backoff  %d/%d UL active state %d \n", module_idP, frameP, subframe,
      UE_mac_inst[module_idP].cba_last_access[0], cba_backoff,UE_mac_inst[module_idP].ul_active);

      rv = 1;

    rlc_status = mac_rlc_status_ind(module_idP, UE_mac_inst[module_idP].crnti,frameP,ENB_FLAG_NO,MBMS_FLAG_NO, // eNB_index
            DTCH,
            0);

    if ((
   // (rlc_status.pdus_in_buffer > 0 )           &&
  // (UE_mac_inst[module_idP].ul_active == 0)  && // check if the ul is acrtive
   (rlc_status.head_sdu_is_segmented  == 0 )          &&
   ((rlc_status.head_sdu_remaining_size_to_send + header_offset ) <= buflen )
   )){
      rv = 1;

      UE_mac_inst[module_idP].cba_last_access[0]= round(uniform_rngen(1,30));
      LOG_D(MAC,"[UE %d] Frame %d Subframe %d: start a new CBA backoff  %d UL active state %d \n", module_idP, frameP, subframe,
      UE_mac_inst[module_idP].cba_last_access[0], UE_mac_inst[module_idP].ul_active);
    } else
      UE_mac_inst[module_idP].cba_last_access[0]= round(uniform_rngen(1,5));


    } else if (( ((UE_mac_inst[module_idP].scheduling_info.BSR[LCGID1]> 0 ))   ||
     ((UE_mac_inst[module_idP].scheduling_info.BSR[LCGID2]> 0 ))   ||
     ((UE_mac_inst[module_idP].scheduling_info.BSR[LCGID3]> 0 )) )
         // && (UE_mac_inst[module_idP].ul_active == 0) // check if the ul is acrtive
         && (UE_mac_inst[module_idP].cba_last_access[0]> 0) )
      {

  UE_mac_inst[module_idP].cba_last_access[0]-=1;
  LOG_D(MAC,"[UE %d] Frame %d Subframe %d: wait for backoff to expire (%d) CBA UL active state %d \n",
        module_idP, frameP, subframe,
        UE_mac_inst[module_idP].cba_last_access[0], UE_mac_inst[module_idP].ul_active);
      }
  }
  */

}
#endif

int get_bsr_lcgid (module_id_t module_idP)
{
  int lcgid, lcgid_tmp=-1;
  int num_active_lcgid = 0;

  for (lcgid = 0 ; lcgid < MAX_NUM_LCGID; lcgid++) {
    if (UE_mac_inst[module_idP].scheduling_info.BSR[lcgid] > 0 ) {
      lcgid_tmp = lcgid;
      num_active_lcgid+=1;
    }
  }

  if (num_active_lcgid == 0) {
    return -1;
  } else if (num_active_lcgid == 1) {
    return lcgid_tmp;
  } else {
    return MAX_NUM_LCGID;
  }
}

uint8_t get_bsr_len (module_id_t module_idP, uint16_t buflen)
{

  int lcgid=0;
  uint8_t bsr_len=0,  num_lcgid=0;
  int pdu = 0;


  for (lcgid=0; lcgid < MAX_NUM_LCGID; lcgid++ ) {
    if (UE_mac_inst[module_idP].scheduling_info.BSR_bytes[lcgid] > 0 ) {
      pdu += (UE_mac_inst[module_idP].scheduling_info.BSR_bytes[lcgid] +  bsr_len + 2); //2 = sizeof(SCH_SUBHEADER_SHORT)
    }

    if (UE_mac_inst[module_idP].scheduling_info.BSR_bytes[lcgid] > 128 ) { // long header size: adjust the header size
      pdu += 1;
    }

    // current phy buff can not transport all sdu for this lcgid -> transmit a bsr for this lcgid

    if ( (pdu > buflen) &&  (UE_mac_inst[module_idP].scheduling_info.BSR_bytes[lcgid] > 0 ) ) {
      num_lcgid +=1;
      bsr_len = (num_lcgid >= 2 ) ? sizeof(BSR_LONG) :  sizeof(BSR_SHORT) ;
    }

    LOG_D(MAC,"BSR Bytes %d for lcgid %d bsr len %d num lcgid %d\n", UE_mac_inst[module_idP].scheduling_info.BSR_bytes[lcgid], lcgid, bsr_len, num_lcgid);
  }

  if ( bsr_len > 0 )
    LOG_D(MAC,"[UE %d] Prepare a %s (Transport Block Size %d, MAC pdu Size %d) \n",
          module_idP, map_int_to_str(BSR_names, bsr_len), buflen, pdu);

  return bsr_len;
}


boolean_t  update_bsr(module_id_t module_idP, frame_t frameP, eNB_index_t eNB_index, uint8_t lcid, uint8_t lcg_id)
{

  mac_rlc_status_resp_t rlc_status;
  boolean_t sr_pending = FALSE;


  if ((lcg_id < 0) || (lcg_id >= MAX_NUM_LCGID) ) {
    return sr_pending;
  }

  // fixme: need a better way to reset
  if ((lcid == DCCH) || (lcid == DTCH)) {
    UE_mac_inst[module_idP].scheduling_info.BSR[lcg_id]=0;
    UE_mac_inst[module_idP].scheduling_info.BSR_bytes[lcg_id]=0;
  }

  //  for (lcid =0 ; lcid < MAX_NUM_LCID; lcid++) {
  if (UE_mac_inst[module_idP].scheduling_info.LCGID[lcid] == lcg_id) {
    rlc_status = mac_rlc_status_ind(module_idP, UE_mac_inst[module_idP].crnti,eNB_index,frameP,ENB_FLAG_NO,MBMS_FLAG_NO,
                                    lcid,
                                    0);

    if (rlc_status.bytes_in_buffer > 0 ) {
      sr_pending = TRUE;
      UE_mac_inst[module_idP].scheduling_info.LCID_status[lcid] = LCID_NOT_EMPTY;
      UE_mac_inst[module_idP].scheduling_info.BSR[lcg_id] += locate (BSR_TABLE,BSR_TABLE_SIZE, rlc_status.bytes_in_buffer);
      UE_mac_inst[module_idP].scheduling_info.BSR_bytes[lcg_id] += rlc_status.bytes_in_buffer;
      // UE_mac_inst[module_idP].scheduling_info.BSR_short_lcid = lcid; // only applicable to short bsr
      LOG_D(MAC,"[UE %d] BSR level %d (LCGID %d, rlc buffer %d byte)\n",
            module_idP, UE_mac_inst[module_idP].scheduling_info.BSR[lcg_id],lcg_id,  UE_mac_inst[module_idP].scheduling_info.BSR_bytes[lcg_id]);
    } else {
      UE_mac_inst[module_idP].scheduling_info.LCID_status[lcid]=LCID_EMPTY;
    }
  }

  //}
  return sr_pending;
}

uint8_t locate (const uint32_t *table, int size, int value)
{

  uint8_t ju, jm, jl;
  int ascend;

  DevAssert( size > 0 );
  DevAssert( size <= 256 );

  if (value == 0) {
    return 0;  //elseif (value > 150000) return 63;
  }

  jl = 0;        // lower bound
  ju = size - 1; // upper bound
  ascend = (table[ju] >= table[jl]) ? 1 : 0; // determine the order of the the table:  1 if ascending order of table, 0 otherwise

  while (ju-jl > 1) { //If we are not yet done,
    jm = (ju+jl) >> 1; //compute a midpoint,

    if ((value >= table[jm]) == ascend) {
      jl=jm; // replace the lower limit
    } else {
      ju=jm; //replace the upper limit
    }

    LOG_T(MAC,"[UE] searching BSR index %d for (BSR TABLE %d < value %d)\n", jm, table[jm], value);
  }

  if (value == table[jl]) {
    return jl;
  } else                    {
    return jl+1;  //equally  ju
  }

}

int get_sf_periodicBSRTimer(uint8_t sf_offset)
{

  switch (sf_offset) {
  case MAC_MainConfig__ul_SCH_Config__periodicBSR_Timer_sf5:
    return 5;
    break;

  case MAC_MainConfig__ul_SCH_Config__periodicBSR_Timer_sf10:
    return 10;
    break;

  case MAC_MainConfig__ul_SCH_Config__periodicBSR_Timer_sf16:
    return 16;
    break;

  case MAC_MainConfig__ul_SCH_Config__periodicBSR_Timer_sf20:
    return 20;
    break;

  case MAC_MainConfig__ul_SCH_Config__periodicBSR_Timer_sf32:
    return 32;
    break;

  case MAC_MainConfig__ul_SCH_Config__periodicBSR_Timer_sf40:
    return 40;
    break;

  case MAC_MainConfig__ul_SCH_Config__periodicBSR_Timer_sf64:
    return 64;
    break;

  case MAC_MainConfig__ul_SCH_Config__periodicBSR_Timer_sf80:
    return 80;
    break;

  case MAC_MainConfig__ul_SCH_Config__periodicBSR_Timer_sf128:
    return 128;
    break;

  case MAC_MainConfig__ul_SCH_Config__periodicBSR_Timer_sf160:
    return 160;
    break;

  case MAC_MainConfig__ul_SCH_Config__periodicBSR_Timer_sf320:
    return 320;
    break;

  case MAC_MainConfig__ul_SCH_Config__periodicBSR_Timer_sf640:
    return 640;
    break;

  case MAC_MainConfig__ul_SCH_Config__periodicBSR_Timer_sf1280:
    return 1280;
    break;

  case MAC_MainConfig__ul_SCH_Config__periodicBSR_Timer_sf2560:
    return 2560;
    break;

  case MAC_MainConfig__ul_SCH_Config__periodicBSR_Timer_infinity:
  default:
    return -1;
    break;
  }
}

int get_sf_retxBSRTimer(uint8_t sf_offset)
{

  switch (sf_offset) {
  case MAC_MainConfig__ul_SCH_Config__retxBSR_Timer_sf320:
    return 320;
    break;

  case MAC_MainConfig__ul_SCH_Config__retxBSR_Timer_sf640:
    return 640;
    break;

  case MAC_MainConfig__ul_SCH_Config__retxBSR_Timer_sf1280:
    return 1280;
    break;

  case MAC_MainConfig__ul_SCH_Config__retxBSR_Timer_sf2560:
    return 2560;
    break;

  case MAC_MainConfig__ul_SCH_Config__retxBSR_Timer_sf5120:
    return 5120;
    break;

  case MAC_MainConfig__ul_SCH_Config__retxBSR_Timer_sf10240:
    return 10240;
    break;

  default:
    return -1;
    break;
  }
}
int get_ms_bucketsizeduration(uint8_t bucketsizeduration)
{

  switch (bucketsizeduration) {
  case LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms50:
    return 50;
    break;

  case LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms100:
    return 100;
    break;

  case LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms150:
    return 150;
    break;

  case LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms300:
    return 300;
    break;

  case LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms500:
    return 500;
    break;

  case LogicalChannelConfig__ul_SpecificParameters__bucketSizeDuration_ms1000:
    return 1000;
    break;

  default:
    return 0;
    break;
  }
}

void update_phr(module_id_t module_idP,int CC_id)
{

  if (CC_id>0) {
    LOG_E(MAC,"Transmission on secondary CCs is not supported yet\n");
    mac_xface->macphy_exit("MAC FATAL  CC_id>0");
    return;
  }

  UE_mac_inst[module_idP].PHR_reporting_active =0;
  UE_mac_inst[module_idP].scheduling_info.periodicPHR_SF =  get_sf_perioidicPHR_Timer(UE_mac_inst[module_idP].scheduling_info.periodicPHR_Timer);
  UE_mac_inst[module_idP].scheduling_info.prohibitPHR_SF =  get_sf_prohibitPHR_Timer(UE_mac_inst[module_idP].scheduling_info.prohibitPHR_Timer);
  // LOG_D(MAC,"phr %d %d\n ",UE_mac_inst[module_idP].scheduling_info.periodicPHR_SF, UE_mac_inst[module_idP].scheduling_info.prohibitPHR_SF);
}
uint8_t get_phr_mapping (module_id_t module_idP, int CC_id, uint8_t eNB_index)
{

  if (CC_id>0) {
    LOG_E(MAC,"Transmission on secondary CCs is not supported yet\n");
    mac_xface->macphy_exit("MAC FATAL  CC_id>0");
    return 0; // not reached
  }

  //power headroom reporting range is from -23 ...+40 dB, as described in 36313
  //note: mac_xface->get_Po_NOMINAL_PUSCH(module_idP) is float
  if (mac_xface->get_PHR(module_idP,CC_id,eNB_index) < -23) {
    return 0;
  } else if (mac_xface->get_PHR(module_idP,CC_id,eNB_index) >= 40) {
    return 63;
  } else { // -23 to 40
    return  (uint8_t) mac_xface->get_PHR(module_idP,CC_id,eNB_index) + PHR_MAPPING_OFFSET;

  }
}
int get_sf_perioidicPHR_Timer(uint8_t perioidicPHR_Timer)
{
  return (perioidicPHR_Timer+1)*10;
}


int get_sf_prohibitPHR_Timer(uint8_t prohibitPHR_Timer)
{
  return (prohibitPHR_Timer)*10;
}

int get_db_dl_PathlossChange(uint8_t dl_PathlossChange)
{
  switch (dl_PathlossChange) {
  case MAC_MainConfig__phr_Config__setup__dl_PathlossChange_dB1:
    return 1;
    break;

  case MAC_MainConfig__phr_Config__setup__dl_PathlossChange_dB3:
    return 3;
    break;

  case MAC_MainConfig__phr_Config__setup__dl_PathlossChange_dB6:
    return 6;
    break;

  case MAC_MainConfig__phr_Config__setup__dl_PathlossChange_infinity:
  default:
    return -1;
    break;
  }
}
