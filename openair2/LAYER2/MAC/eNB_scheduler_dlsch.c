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

/*! \file eNB_scheduler_dlsch.c
 * \brief procedures related to eNB for the DLSCH transport channel
 * \author  Navid Nikaein and Raymond Knopp
 * \date 2010 - 2014
 * \email: navid.nikaein@eurecom.fr
 * \version 1.0
 * @ingroup _mac

 */

#include "assertions.h"
#include "PHY/defs.h"
#include "PHY/extern.h"

#include "SCHED/defs.h"
#include "SCHED/extern.h"

#include "LAYER2/MAC/defs.h"
#include "LAYER2/MAC/proto.h"
#include "LAYER2/MAC/extern.h"
#include "UTIL/LOG/log.h"
#include "UTIL/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"

#include "RRC/LITE/extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"

//#include "LAYER2/MAC/pre_processor.c"
#include "pdcp.h"

#include "SIMULATION/TOOLS/defs.h"	// for taus

#if defined(ENABLE_ITTI)
#include "intertask_interface.h"
#endif

#include "ENB_APP/flexran_agent_defs.h"
#include "flexran_agent_ran_api.h"
#include "header.pb-c.h"
#include "flexran.pb-c.h"
#include <dlfcn.h>

#include "T.h"

#define ENABLE_MAC_PAYLOAD_DEBUG
//#define DEBUG_eNB_SCHEDULER 1

extern RAN_CONTEXT_t RC;
extern uint8_t nfapi_mode;


// number of active slices for  past and current time
int n_active_slices = 1;
int n_active_slices_current = 1;

// RB share for each slice for past and current time
float avg_slice_percentage=0.25;
float slice_percentage[MAX_NUM_SLICES] = {1.0, 0.0, 0.0, 0.0};
float slice_percentage_current[MAX_NUM_SLICES] = {1.0, 0.0, 0.0, 0.0};
float total_slice_percentage = 0;
float total_slice_percentage_current = 0;

// MAX MCS for each slice for past and current time
int slice_maxmcs[MAX_NUM_SLICES] = { 28, 28, 28, 28 };
int slice_maxmcs_current[MAX_NUM_SLICES] = { 28, 28, 28, 28 };

int update_dl_scheduler[MAX_NUM_SLICES] = { 1, 1, 1, 1 };
int update_dl_scheduler_current[MAX_NUM_SLICES] = { 1, 1, 1, 1 };

// name of available scheduler
char *dl_scheduler_type[MAX_NUM_SLICES] =
  { "schedule_ue_spec",
    "schedule_ue_spec",
    "schedule_ue_spec",
    "schedule_ue_spec"
  };

// The lists of criteria that enforce the sorting policies of the slices
uint32_t sorting_policy[MAX_NUM_SLICES] = {0x01234, 0x01234, 0x01234, 0x01234};
uint32_t sorting_policy_current[MAX_NUM_SLICES] = {0x01234, 0x01234, 0x01234, 0x01234};

// pointer to the slice specific scheduler
slice_scheduler_dl slice_sched_dl[MAX_NUM_SLICES] = {0};

//------------------------------------------------------------------------------
void
add_ue_dlsch_info(module_id_t module_idP,
		  int CC_id,
		  int UE_id, sub_frame_t subframeP, UE_DLSCH_STATUS status)
//------------------------------------------------------------------------------
{
  //LOG_D(MAC, "%s(module_idP:%d, CC_id:%d, UE_id:%d, subframeP:%d, status:%d) serving_num:%d rnti:%x\n", __FUNCTION__, module_idP, CC_id, UE_id, subframeP, status, eNB_dlsch_info[module_idP][CC_id][UE_id].serving_num, UE_RNTI(module_idP,UE_id));

  eNB_dlsch_info[module_idP][CC_id][UE_id].rnti =
    UE_RNTI(module_idP, UE_id);
  //  eNB_dlsch_info[module_idP][CC_id][ue_mod_idP].weight           = weight;
  eNB_dlsch_info[module_idP][CC_id][UE_id].subframe = subframeP;
  eNB_dlsch_info[module_idP][CC_id][UE_id].status = status;

  eNB_dlsch_info[module_idP][CC_id][UE_id].serving_num++;

}

//------------------------------------------------------------------------------
int
schedule_next_dlue(module_id_t module_idP, int CC_id,
		   sub_frame_t subframeP)
//------------------------------------------------------------------------------
{

  int next_ue;
  UE_list_t *UE_list = &RC.mac[module_idP]->UE_list;

  for (next_ue = UE_list->head; next_ue >= 0;
       next_ue = UE_list->next[next_ue]) {
    if (eNB_dlsch_info[module_idP][CC_id][next_ue].status ==
	S_DL_WAITING) {
      return next_ue;
    }
  }

  for (next_ue = UE_list->head; next_ue >= 0;
       next_ue = UE_list->next[next_ue]) {
    if (eNB_dlsch_info[module_idP][CC_id][next_ue].status == S_DL_BUFFERED) {
      eNB_dlsch_info[module_idP][CC_id][next_ue].status = S_DL_WAITING;
    }
  }

  return (-1);		//next_ue;

}

//------------------------------------------------------------------------------
int
generate_dlsch_header(unsigned char *mac_header,
		      unsigned char num_sdus,
		      unsigned short *sdu_lengths,
		      unsigned char *sdu_lcids,
		      unsigned char drx_cmd,
		      unsigned short timing_advance_cmd,
		      unsigned char *ue_cont_res_id,
		      unsigned char short_padding,
		      unsigned short post_padding)
//------------------------------------------------------------------------------
{

  SCH_SUBHEADER_FIXED *mac_header_ptr = (SCH_SUBHEADER_FIXED *) mac_header;
  uint8_t first_element = 0, last_size = 0, i;
  uint8_t mac_header_control_elements[16], *ce_ptr;

  ce_ptr = &mac_header_control_elements[0];

  // compute header components

  if ((short_padding == 1) || (short_padding == 2)) {
    mac_header_ptr->R = 0;
    mac_header_ptr->E = 0;
    mac_header_ptr->LCID = SHORT_PADDING;
    first_element = 1;
    last_size = 1;
  }

  if (short_padding == 2) {
    mac_header_ptr->E = 1;
    mac_header_ptr++;
    mac_header_ptr->R = 0;
    mac_header_ptr->E = 0;
    mac_header_ptr->LCID = SHORT_PADDING;
    last_size = 1;
  }

  if (drx_cmd != 255) {
    if (first_element > 0) {
      mac_header_ptr->E = 1;
      mac_header_ptr++;
    } else {
      first_element = 1;
    }

    mac_header_ptr->R = 0;
    mac_header_ptr->E = 0;
    mac_header_ptr->LCID = DRX_CMD;
    last_size = 1;
  }

  if (timing_advance_cmd != 31) {
    if (first_element > 0) {
      mac_header_ptr->E = 1;
      mac_header_ptr++;
    } else {
      first_element = 1;
    }

    mac_header_ptr->R = 0;
    mac_header_ptr->E = 0;
    mac_header_ptr->LCID = TIMING_ADV_CMD;
    last_size = 1;
    //    msg("last_size %d,mac_header_ptr %p\n",last_size,mac_header_ptr);
    ((TIMING_ADVANCE_CMD *) ce_ptr)->R = 0;
    AssertFatal(timing_advance_cmd < 64,
		"timing_advance_cmd %d > 63\n", timing_advance_cmd);
    ((TIMING_ADVANCE_CMD *) ce_ptr)->TA = timing_advance_cmd;	//(timing_advance_cmd+31)&0x3f;
    LOG_D(MAC, "timing advance =%d (%d)\n", timing_advance_cmd,
	  ((TIMING_ADVANCE_CMD *) ce_ptr)->TA);
    ce_ptr += sizeof(TIMING_ADVANCE_CMD);
    //msg("offset %d\n",ce_ptr-mac_header_control_elements);
  }

  if (ue_cont_res_id) {
    if (first_element > 0) {
      mac_header_ptr->E = 1;
      /*
	printf("[eNB][MAC] last subheader : %x (R%d,E%d,LCID%d)\n",*(unsigned char*)mac_header_ptr,
	((SCH_SUBHEADER_FIXED *)mac_header_ptr)->R,
	((SCH_SUBHEADER_FIXED *)mac_header_ptr)->E,
	((SCH_SUBHEADER_FIXED *)mac_header_ptr)->LCID);
      */
      mac_header_ptr++;
    } else {
      first_element = 1;
    }

    mac_header_ptr->R = 0;
    mac_header_ptr->E = 0;
    mac_header_ptr->LCID = UE_CONT_RES;
    last_size = 1;

    LOG_T(MAC,
	  "[eNB ][RAPROC] Generate contention resolution msg: %x.%x.%x.%x.%x.%x\n",
	  ue_cont_res_id[0], ue_cont_res_id[1], ue_cont_res_id[2],
	  ue_cont_res_id[3], ue_cont_res_id[4], ue_cont_res_id[5]);

    memcpy(ce_ptr, ue_cont_res_id, 6);
    ce_ptr += 6;
    // msg("(cont_res) : offset %d\n",ce_ptr-mac_header_control_elements);
  }
  //msg("last_size %d,mac_header_ptr %p\n",last_size,mac_header_ptr);

  for (i = 0; i < num_sdus; i++) {
    LOG_T(MAC, "[eNB] Generate DLSCH header num sdu %d len sdu %d\n",
	  num_sdus, sdu_lengths[i]);

    if (first_element > 0) {
      mac_header_ptr->E = 1;
      /*msg("last subheader : %x (R%d,E%d,LCID%d)\n",*(unsigned char*)mac_header_ptr,
	((SCH_SUBHEADER_FIXED *)mac_header_ptr)->R,
	((SCH_SUBHEADER_FIXED *)mac_header_ptr)->E,
	((SCH_SUBHEADER_FIXED *)mac_header_ptr)->LCID);
      */
      mac_header_ptr += last_size;
      //msg("last_size %d,mac_header_ptr %p\n",last_size,mac_header_ptr);
    } else {
      first_element = 1;
    }

    if (sdu_lengths[i] < 128) {
      ((SCH_SUBHEADER_SHORT *) mac_header_ptr)->R = 0;
      ((SCH_SUBHEADER_SHORT *) mac_header_ptr)->E = 0;
      ((SCH_SUBHEADER_SHORT *) mac_header_ptr)->F = 0;
      ((SCH_SUBHEADER_SHORT *) mac_header_ptr)->LCID = sdu_lcids[i];
      ((SCH_SUBHEADER_SHORT *) mac_header_ptr)->L = (unsigned char) sdu_lengths[i];
      last_size = 2;
    } else {
      ((SCH_SUBHEADER_LONG *) mac_header_ptr)->R = 0;
      ((SCH_SUBHEADER_LONG *) mac_header_ptr)->E = 0;
      ((SCH_SUBHEADER_LONG *) mac_header_ptr)->F = 1;
      ((SCH_SUBHEADER_LONG *) mac_header_ptr)->LCID = sdu_lcids[i];
      ((SCH_SUBHEADER_LONG *) mac_header_ptr)->L_MSB = ((unsigned short) sdu_lengths[i] >> 8) & 0x7f;
      ((SCH_SUBHEADER_LONG *) mac_header_ptr)->L_LSB = (unsigned short) sdu_lengths[i] & 0xff;
      ((SCH_SUBHEADER_LONG *) mac_header_ptr)->padding = 0x00;
      last_size = 3;
#ifdef DEBUG_HEADER_PARSING
      LOG_D(MAC,
	    "[eNB] generate long sdu, size %x (MSB %x, LSB %x)\n",
	    sdu_lengths[i],
	    ((SCH_SUBHEADER_LONG *) mac_header_ptr)->L_MSB,
	    ((SCH_SUBHEADER_LONG *) mac_header_ptr)->L_LSB);
#endif
    }
  }

  /*

    printf("last_size %d,mac_header_ptr %p\n",last_size,mac_header_ptr);

    printf("last subheader : %x (R%d,E%d,LCID%d)\n",*(unsigned char*)mac_header_ptr,
    ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->R,
    ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->E,
    ((SCH_SUBHEADER_FIXED *)mac_header_ptr)->LCID);


    if (((SCH_SUBHEADER_FIXED*)mac_header_ptr)->LCID < UE_CONT_RES) {
    if (((SCH_SUBHEADER_SHORT*)mac_header_ptr)->F == 0)
    printf("F = 0, sdu len (L field) %d\n",(((SCH_SUBHEADER_SHORT*)mac_header_ptr)->L));
    else
    printf("F = 1, sdu len (L field) %d\n",(((SCH_SUBHEADER_LONG*)mac_header_ptr)->L));
    }
  */
  if (post_padding > 0) {	// we have lots of padding at the end of the packet
    mac_header_ptr->E = 1;
    mac_header_ptr += last_size;
    // add a padding element
    mac_header_ptr->R = 0;
    mac_header_ptr->E = 0;
    mac_header_ptr->LCID = SHORT_PADDING;
    mac_header_ptr++;
  } else {			// no end of packet padding
    // last SDU subhead is of fixed type (sdu length implicitly to be computed at UE)
    mac_header_ptr++;
  }

  //msg("After subheaders %d\n",(uint8_t*)mac_header_ptr - mac_header);

  if ((ce_ptr - mac_header_control_elements) > 0) {
    // printf("Copying %d bytes for control elements\n",ce_ptr-mac_header_control_elements);
    memcpy((void *) mac_header_ptr, mac_header_control_elements,
	   ce_ptr - mac_header_control_elements);
    mac_header_ptr +=
      (unsigned char) (ce_ptr - mac_header_control_elements);
  }
  //msg("After CEs %d\n",(uint8_t*)mac_header_ptr - mac_header);

  return ((unsigned char *) mac_header_ptr - mac_header);
}

//------------------------------------------------------------------------------
void
set_ul_DAI(int module_idP, int UE_idP, int CC_idP, int frameP,
	   int subframeP)
//------------------------------------------------------------------------------
{

  eNB_MAC_INST *eNB = RC.mac[module_idP];
  UE_list_t *UE_list = &eNB->UE_list;
  unsigned char DAI;
  COMMON_channels_t *cc = &eNB->common_channels[CC_idP];
  if (cc->tdd_Config != NULL) {	//TDD
    DAI = (UE_list->UE_template[CC_idP][UE_idP].DAI - 1) & 3;
    LOG_D(MAC,
	  "[eNB %d] CC_id %d Frame %d, subframe %d: DAI %d for UE %d\n",
	  module_idP, CC_idP, frameP, subframeP, DAI, UE_idP);
    // Save DAI for Format 0 DCI

    switch (cc->tdd_Config->subframeAssignment) {
    case 0:
      //      if ((subframeP==0)||(subframeP==1)||(subframeP==5)||(subframeP==6))
      break;

    case 1:
      switch (subframeP) {
      case 1:
	UE_list->UE_template[CC_idP][UE_idP].DAI_ul[7] = DAI;
	break;

      case 4:
	UE_list->UE_template[CC_idP][UE_idP].DAI_ul[8] = DAI;
	break;

      case 6:
	UE_list->UE_template[CC_idP][UE_idP].DAI_ul[2] = DAI;
	break;

      case 9:
	UE_list->UE_template[CC_idP][UE_idP].DAI_ul[3] = DAI;
	break;
      }

    case 2:
      //      if ((subframeP==3)||(subframeP==8))
      //  UE_list->UE_template[CC_idP][UE_idP].DAI_ul = DAI;
      break;

    case 3:

      //if ((subframeP==6)||(subframeP==8)||(subframeP==0)) {
      //  LOG_D(MAC,"schedule_ue_spec: setting UL DAI to %d for subframeP %d => %d\n",DAI,subframeP, ((subframeP+8)%10)>>1);
      //  UE_list->UE_template[CC_idP][UE_idP].DAI_ul[((subframeP+8)%10)>>1] = DAI;
      //}
      switch (subframeP) {
      case 5:
      case 6:
      case 1:
	UE_list->UE_template[CC_idP][UE_idP].DAI_ul[2] = DAI;
	break;

      case 7:
      case 8:
	UE_list->UE_template[CC_idP][UE_idP].DAI_ul[3] = DAI;
	break;

      case 9:
      case 0:
	UE_list->UE_template[CC_idP][UE_idP].DAI_ul[4] = DAI;
	break;

      default:
	break;
      }

      break;

    case 4:
      //      if ((subframeP==8)||(subframeP==9))
      //  UE_list->UE_template[CC_idP][UE_idP].DAI_ul = DAI;
      break;

    case 5:
      //      if (subframeP==8)
      //  UE_list->UE_template[CC_idP][UE_idP].DAI_ul = DAI;
      break;

    case 6:
      //      if ((subframeP==1)||(subframeP==4)||(subframeP==6)||(subframeP==9))
      //  UE_list->UE_template[CC_idP][UE_idP].DAI_ul = DAI;
      break;

    default:
      break;
    }
  }
}

//------------------------------------------------------------------------------
void
schedule_dlsch(module_id_t module_idP,
	        frame_t frameP, sub_frame_t subframeP, int *mbsfn_flag)
//------------------------------------------------------------------------------{
{

  int i = 0;

  total_slice_percentage=0;
  avg_slice_percentage=1.0/n_active_slices;

  // reset the slice percentage for inactive slices
  for (i = n_active_slices; i< MAX_NUM_SLICES; i++) {
    slice_percentage[i]=0;
  }
  for (i = 0; i < n_active_slices; i++) {
    if (slice_percentage[i] < 0 ){
      LOG_W(MAC, "[eNB %d] frame %d subframe %d:invalid slice %d percentage %f. resetting to zero",
	    module_idP, frameP, subframeP, i, slice_percentage[i]);
      slice_percentage[i]=0;
    }
    total_slice_percentage+=slice_percentage[i];
  }

  for (i = 0; i < n_active_slices; i++) {

    // Load any updated functions
    if (update_dl_scheduler[i] > 0 ) {
      slice_sched_dl[i] = dlsym(NULL, dl_scheduler_type[i]);
      update_dl_scheduler[i] = 0 ;
      update_dl_scheduler_current[i] = 0;
      LOG_N(MAC,"update dl scheduler slice %d\n", i);
    }

    if (total_slice_percentage <= 1.0){ // the new total RB share is within the range

      // check if the number of slices has changed, and log
      if (n_active_slices_current != n_active_slices ){
	if ((n_active_slices > 0) && (n_active_slices <= MAX_NUM_SLICES)) {
	  LOG_N(MAC,"[eNB %d]frame %d subframe %d: number of active DL slices has changed: %d-->%d\n",
		module_idP, frameP, subframeP, n_active_slices_current, n_active_slices);

	  n_active_slices_current = n_active_slices;

	} else {
	  LOG_W(MAC,"invalid number of DL slices %d, revert to the previous value %d\n",n_active_slices, n_active_slices_current);
	  n_active_slices = n_active_slices_current;
	}
      }

      // check if the slice rb share has changed, and log the console
      if (slice_percentage_current[i] != slice_percentage[i]){ // new slice percentage
	LOG_N(MAC,"[eNB %d][SLICE %d][DL] frame %d subframe %d: total percentage %f-->%f, slice RB percentage has changed: %f-->%f\n",
	      module_idP, i, frameP, subframeP, total_slice_percentage_current, total_slice_percentage, slice_percentage_current[i], slice_percentage[i]);
	total_slice_percentage_current= total_slice_percentage;
	slice_percentage_current[i] = slice_percentage[i];

      }

      // check if the slice max MCS, and log the console
      if (slice_maxmcs_current[i] != slice_maxmcs[i]){
	if ((slice_maxmcs[i] >= 0) && (slice_maxmcs[i] < 29)){
	  LOG_N(MAC,"[eNB %d][SLICE %d][DL] frame %d subframe %d: slice MAX MCS has changed: %d-->%d\n",
		module_idP, i, frameP, subframeP, slice_maxmcs_current[i], slice_maxmcs[i]);
	  slice_maxmcs_current[i] = slice_maxmcs[i];
	} else {
	  LOG_W(MAC,"[eNB %d][SLICE %d][DL] invalid slice max mcs %d, revert the previous value %d\n",module_idP, i, slice_maxmcs[i],slice_maxmcs_current[i]);
	  slice_maxmcs[i]= slice_maxmcs_current[i];
	}
      }

      // check if a new scheduler, and log the console
      if (update_dl_scheduler_current[i] != update_dl_scheduler[i]){
	LOG_N(MAC,"[eNB %d][SLICE %d][DL] frame %d subframe %d: DL scheduler for this slice is updated: %s \n",
	      module_idP, i, frameP, subframeP, dl_scheduler_type[i]);
	update_dl_scheduler_current[i] = update_dl_scheduler[i];
      }

    } else {
      // here we can correct the values, e.g. reduce proportionally

      if (n_active_slices == n_active_slices_current){
	LOG_W(MAC,"[eNB %d][SLICE %d][DL] invalid total RB share (%f->%f), reduce proportionally the RB share by 0.1\n",
	      module_idP, i, total_slice_percentage_current, total_slice_percentage);
	if (slice_percentage[i] >= avg_slice_percentage){
	  slice_percentage[i]-=0.1;
	  total_slice_percentage-=0.1;
	}
      } else {
	LOG_W(MAC,"[eNB %d][SLICE %d][DL] invalid total RB share (%f->%f), revert the number of slice to its previous value (%d->%d)\n",
	      module_idP, i, total_slice_percentage_current, total_slice_percentage,
	      n_active_slices, n_active_slices_current );
	n_active_slices = n_active_slices_current;
	slice_percentage[i] = slice_percentage_current[i];
      }
    }

    // Check for new sorting policy
    if (sorting_policy_current[i] != sorting_policy[i]) {
      LOG_N(MAC,"[eNB %d][SLICE %d][DL] frame %d subframe %d: UE sorting policy has changed (%x-->%x)\n",
            module_idP, i, frameP, subframeP, sorting_policy_current[i], sorting_policy[i]);
      sorting_policy_current[i] = sorting_policy[i];
    }

    // Run each enabled slice-specific schedulers one by one
    slice_sched_dl[i](module_idP, i, frameP, subframeP, mbsfn_flag/*, dl_info*/);
  }

}

// changes to pre-processor for eMTC

//------------------------------------------------------------------------------
void
schedule_ue_spec(module_id_t module_idP,slice_id_t slice_idP,
		 frame_t frameP, sub_frame_t subframeP, int *mbsfn_flag)
//------------------------------------------------------------------------------
{
  int CC_id;
  int UE_id;
  int aggregation;
  mac_rlc_status_resp_t rlc_status;
  int ta_len = 0;
  unsigned char sdu_lcids[NB_RB_MAX];
  int lcid, offset, num_sdus = 0;
  int nb_rb, nb_rb_temp, nb_available_rb;
  uint16_t sdu_lengths[NB_RB_MAX];
  int TBS, j, rnti, padding = 0, post_padding = 0;
  unsigned char dlsch_buffer[MAX_DLSCH_PAYLOAD_BYTES];
  int round = 0;
  int harq_pid = 0;
  eNB_UE_STATS *eNB_UE_stats = NULL;
  int sdu_length_total = 0;

  eNB_MAC_INST *eNB = RC.mac[module_idP];
  COMMON_channels_t *cc = eNB->common_channels;
  UE_list_t *UE_list = &eNB->UE_list;
  int continue_flag = 0;
  int32_t normalized_rx_power, target_rx_power;
  int tpc = 1;
  UE_sched_ctrl *ue_sched_ctl;
  int mcs;
  int i;
  int min_rb_unit[MAX_NUM_CCs];
  int N_RB_DL[MAX_NUM_CCs];
  int total_nb_available_rb[MAX_NUM_CCs];
  int N_RBG[MAX_NUM_CCs];
  nfapi_dl_config_request_body_t *dl_req;
  nfapi_dl_config_request_pdu_t *dl_config_pdu;
  int tdd_sfa;
  int ta_update;
  int header_length_last;
  int header_length_total;

#if 0
  if (UE_list->head == -1) {
    return;
  }
#endif

  start_meas(&eNB->schedule_dlsch);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
    (VCD_SIGNAL_DUMPER_FUNCTIONS_SCHEDULE_DLSCH, VCD_FUNCTION_IN);


  // for TDD: check that we have to act here, otherwise return
  if (cc[0].tdd_Config) {
    tdd_sfa = cc[0].tdd_Config->subframeAssignment;
    switch (subframeP) {
    case 0:
      // always continue
      break;
    case 1:
      return;
      break;
    case 2:
      return;
      break;
    case 3:
      if ((tdd_sfa != 2) && (tdd_sfa != 5))
	return;
      break;
    case 4:
      if ((tdd_sfa != 1) && (tdd_sfa != 2) && (tdd_sfa != 4)
	  && (tdd_sfa != 5))
	return;
      break;
    case 5:
      break;
    case 6:
    case 7:
      if ((tdd_sfa != 1) && (tdd_sfa != 2) && (tdd_sfa != 4)
	  && (tdd_sfa != 5))
	return;
      break;
    case 8:
      if ((tdd_sfa != 2) && (tdd_sfa != 3) && (tdd_sfa != 4)
	  && (tdd_sfa != 5))
	return;
      break;
    case 9:
      if ((tdd_sfa != 1) && (tdd_sfa != 3) && (tdd_sfa != 4)
	  && (tdd_sfa != 6))
	return;
      break;

    }
  }
  //weight = get_ue_weight(module_idP,UE_id);
  aggregation = 2;
  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    N_RB_DL[CC_id] = to_prb(cc[CC_id].mib->message.dl_Bandwidth);
    min_rb_unit[CC_id] = get_min_rb_unit(module_idP, CC_id);
    // get number of PRBs less those used by common channels
    total_nb_available_rb[CC_id] = N_RB_DL[CC_id];
    for (i = 0; i < N_RB_DL[CC_id]; i++)
      if (cc[CC_id].vrb_map[i] != 0)
	total_nb_available_rb[CC_id]--;

    N_RBG[CC_id] = to_rbg(cc[CC_id].mib->message.dl_Bandwidth);

    // store the global enb stats:
    eNB->eNB_stats[CC_id].num_dlactive_UEs = UE_list->num_UEs;
    eNB->eNB_stats[CC_id].available_prbs = total_nb_available_rb[CC_id];
    eNB->eNB_stats[CC_id].total_available_prbs += total_nb_available_rb[CC_id];
    eNB->eNB_stats[CC_id].dlsch_bytes_tx = 0;
    eNB->eNB_stats[CC_id].dlsch_pdus_tx = 0;
  }

  /// CALLING Pre_Processor for downlink scheduling (Returns estimation of RBs required by each UE and the allocation on sub-band)

  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
      (VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_PREPROCESSOR, VCD_FUNCTION_IN);
  start_meas(&eNB->schedule_dlsch_preprocessor);
  dlsch_scheduler_pre_processor(module_idP,
                                slice_idP,
                                frameP,
                                subframeP,
                                N_RBG,
                                mbsfn_flag);
  stop_meas(&eNB->schedule_dlsch_preprocessor);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
      (VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_PREPROCESSOR, VCD_FUNCTION_OUT);

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    LOG_D(MAC, "doing schedule_ue_spec for CC_id %d\n", CC_id);

    dl_req = &eNB->DL_req[CC_id].dl_config_request_body;

    if (mbsfn_flag[CC_id] > 0)
      continue;

    for (UE_id = UE_list->head; UE_id >= 0;
	 UE_id = UE_list->next[UE_id]) {
      continue_flag = 0;	// reset the flag to allow allocation for the remaining UEs
      rnti = UE_RNTI(module_idP, UE_id);
      eNB_UE_stats = &UE_list->eNB_UE_stats[CC_id][UE_id];
      ue_sched_ctl = &UE_list->UE_sched_ctrl[UE_id];


      if (rnti == NOT_A_RNTI) {
	LOG_D(MAC, "Cannot find rnti for UE_id %d (num_UEs %d)\n",
	      UE_id, UE_list->num_UEs);
	continue_flag = 1;
      }

      if (eNB_UE_stats == NULL) {
          LOG_D(MAC, "[eNB] Cannot find eNB_UE_stats\n");
          continue_flag = 1;
      }
      if (!ue_slice_membership(UE_id, slice_idP))
                  continue;

      if (continue_flag != 1) {
	switch (get_tmode(module_idP, CC_id, UE_id)) {
	case 1:
	case 2:
	case 7:
	  aggregation =
	    get_aggregation(get_bw_index(module_idP, CC_id),
			    ue_sched_ctl->dl_cqi[CC_id],
			    format1);
	  break;
	case 3:
	  aggregation =
	    get_aggregation(get_bw_index(module_idP, CC_id),
			    ue_sched_ctl->dl_cqi[CC_id],
			    format2A);
	  break;
	default:
	  LOG_W(MAC, "Unsupported transmission mode %d\n",
		get_tmode(module_idP, CC_id, UE_id));
	  aggregation = 2;
	}
      }
      /* if (continue_flag != 1 */
      if ((ue_sched_ctl->pre_nb_available_rbs[CC_id] == 0) ||	// no RBs allocated
	  CCE_allocation_infeasible(module_idP, CC_id, 1, subframeP,
				    aggregation, rnti)) {
	LOG_D(MAC,
	      "[eNB %d] Frame %d : no RB allocated for UE %d on CC_id %d: continue \n",
	      module_idP, frameP, UE_id, CC_id);
	continue_flag = 1;	//to next user (there might be rbs availiable for other UEs in TM5
      }

      if (cc[CC_id].tdd_Config != NULL) {	//TDD
	set_ue_dai(subframeP,
		   UE_id,
		   CC_id,
		   cc[CC_id].tdd_Config->subframeAssignment,
		   UE_list);
	// update UL DAI after DLSCH scheduling
	set_ul_DAI(module_idP, UE_id, CC_id, frameP, subframeP);
      }

      if (continue_flag == 1) {
	add_ue_dlsch_info(module_idP,
			  CC_id, UE_id, subframeP, S_DL_NONE);
	continue;
      }
#warning RK->CR This old API call has to be revisited for FAPI, or logic must be changed
#if 0
      /* add "fake" DCI to have CCE_allocation_infeasible work properly for next allocations */
      /* if we don't add it, next allocations may succeed but overall allocations may fail */
      /* will be removed at the end of this function */
      add_ue_spec_dci(&eNB->common_channels[CC_id].DCI_pdu, &(char[]) {
	  0}, rnti, 1, aggregation, 1, format1, 0);
#endif

      nb_available_rb = ue_sched_ctl->pre_nb_available_rbs[CC_id];

      if (cc->tdd_Config) harq_pid = ((frameP * 10) + subframeP) % 10;
      else            	  harq_pid = ((frameP * 10) + subframeP) & 7;

      round = ue_sched_ctl->round[CC_id][harq_pid];

      UE_list->eNB_UE_stats[CC_id][UE_id].crnti      = rnti;
      UE_list->eNB_UE_stats[CC_id][UE_id].rrc_status = mac_eNB_get_rrc_status(module_idP, rnti);
      UE_list->eNB_UE_stats[CC_id][UE_id].harq_pid   = harq_pid;
      UE_list->eNB_UE_stats[CC_id][UE_id].harq_round = round;


      if (UE_list->eNB_UE_stats[CC_id][UE_id].rrc_status < RRC_CONNECTED) continue;

      header_length_total = 0;
      sdu_length_total = 0;
      num_sdus = 0;

      /*
	DevCheck(((eNB_UE_stats->dl_cqi < MIN_CQI_VALUE) || (eNB_UE_stats->dl_cqi > MAX_CQI_VALUE)),
	eNB_UE_stats->dl_cqi, MIN_CQI_VALUE, MAX_CQI_VALUE);
      */
      if (nfapi_mode) {
		  eNB_UE_stats->dlsch_mcs1 = 10;//cqi_to_mcs[ue_sched_ctl->dl_cqi[CC_id]];
      }
      else { // this operation is also done in the preprocessor
		  eNB_UE_stats->dlsch_mcs1 = cmin(eNB_UE_stats->dlsch_mcs1, slice_maxmcs[slice_idP]);  //cmin(eNB_UE_stats->dlsch_mcs1, openair_daq_vars.target_ue_dl_mcs);
      }



      // store stats
      //UE_list->eNB_UE_stats[CC_id][UE_id].dl_cqi= eNB_UE_stats->dl_cqi;

      // initializing the rb allocation indicator for each UE
      for (j = 0; j < N_RBG[CC_id]; j++) {
	UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j] = 0;
      }

      LOG_D(MAC,
	    "[eNB %d] Frame %d: Scheduling UE %d on CC_id %d (rnti %x, harq_pid %d, round %d, rb %d, cqi %d, mcs %d, rrc %d)\n",
	    module_idP, frameP, UE_id, CC_id, rnti, harq_pid, round,
	    nb_available_rb, ue_sched_ctl->dl_cqi[CC_id],
	    eNB_UE_stats->dlsch_mcs1,
	    UE_list->eNB_UE_stats[CC_id][UE_id].rrc_status);



      /* process retransmission  */

      if (round != 8) {

	// get freq_allocation
	nb_rb = UE_list->UE_template[CC_id][UE_id].nb_rb[harq_pid];
	TBS = get_TBS_DL(UE_list->UE_template[CC_id][UE_id].oldmcs1[harq_pid],
			 nb_rb);

	if (nb_rb <= nb_available_rb) {
	  if (cc[CC_id].tdd_Config != NULL) {
	    UE_list->UE_template[CC_id][UE_id].DAI++;
	    update_ul_dci(module_idP, CC_id, rnti,
			  UE_list->UE_template[CC_id][UE_id].DAI);
	    LOG_D(MAC,
		  "DAI update: CC_id %d subframeP %d: UE %d, DAI %d\n",
		  CC_id, subframeP, UE_id,
		  UE_list->UE_template[CC_id][UE_id].DAI);
	  }

	  if (nb_rb == ue_sched_ctl->pre_nb_available_rbs[CC_id]) {
	    for (j = 0; j < N_RBG[CC_id]; j++) {	// for indicating the rballoc for each sub-band
	      UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j] = ue_sched_ctl->rballoc_sub_UE[CC_id][j];
	    }
	  } else {
	    nb_rb_temp = nb_rb;
	    j = 0;

	    while ((nb_rb_temp > 0) && (j < N_RBG[CC_id])) {
	      if (ue_sched_ctl->rballoc_sub_UE[CC_id][j] == 1) {
		if (UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j])
		  printf("WARN: rballoc_subband not free for retrans?\n");
		UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j] = ue_sched_ctl->rballoc_sub_UE[CC_id][j];

		if ((j == N_RBG[CC_id] - 1) &&
		    ((N_RB_DL[CC_id] == 25) ||
		     (N_RB_DL[CC_id] == 50))) {
		  nb_rb_temp = nb_rb_temp - min_rb_unit[CC_id] + 1;
		} else {
		  nb_rb_temp = nb_rb_temp - min_rb_unit[CC_id];
		}
	      }

	      j = j + 1;
	    }
	  }

	  nb_available_rb -= nb_rb;
	  /*
	    eNB->mu_mimo_mode[UE_id].pre_nb_available_rbs = nb_rb;
	    eNB->mu_mimo_mode[UE_id].dl_pow_off = ue_sched_ctl->dl_pow_off[CC_id];

	    for(j=0; j<N_RBG[CC_id]; j++) {
	    eNB->mu_mimo_mode[UE_id].rballoc_sub[j] = UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j];
	    }
	  */

	  switch (get_tmode(module_idP, CC_id, UE_id)) {
	  case 1:
	  case 2:
	  case 7:
	  default:
	    LOG_D(MAC,"retransmission DL_REQ: rnti:%x\n",rnti);

	    dl_config_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu];
	    memset((void *) dl_config_pdu, 0,
		   sizeof(nfapi_dl_config_request_pdu_t));
	    dl_config_pdu->pdu_type = NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE;
	    dl_config_pdu->pdu_size = (uint8_t) (2 + sizeof(nfapi_dl_config_dci_dl_pdu));
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tl.tag = NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL8_TAG;
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format = NFAPI_DL_DCI_FORMAT_1;
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level = get_aggregation(get_bw_index(module_idP, CC_id),
											  ue_sched_ctl->dl_cqi[CC_id],
											  format1);
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti      = rnti;
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type = 1;	// CRNTI : see Table 4-10 from SCF082 - nFAPI specifications
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.transmission_power = 6000;	// equal to RS power

	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.harq_process = harq_pid;
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tpc = 1;	// dont adjust power when retransmitting
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.new_data_indicator_1 = UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1 = UE_list->UE_template[CC_id][UE_id].oldmcs1[harq_pid];
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_1 = round & 3;

	    if (cc[CC_id].tdd_Config != NULL) {	//TDD
	      dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.downlink_assignment_index = (UE_list->UE_template[CC_id][UE_id].DAI - 1) & 3;
	      LOG_D(MAC,
		    "[eNB %d] Retransmission CC_id %d : harq_pid %d, round %d, dai %d, mcs %d\n",
		    module_idP, CC_id, harq_pid, round,
		    (UE_list->UE_template[CC_id][UE_id].DAI - 1),
		    UE_list->UE_template[CC_id][UE_id].oldmcs1[harq_pid]);
	    } else {
	      LOG_D(MAC,
		    "[eNB %d] Retransmission CC_id %d : harq_pid %d, round %d, mcs %d\n",
		    module_idP, CC_id, harq_pid, round,
		    UE_list->UE_template[CC_id][UE_id].oldmcs1[harq_pid]);

	    }
	    if (!CCE_allocation_infeasible(module_idP, CC_id, 1, subframeP,
					   dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level, rnti)) {
	      dl_req->number_dci++;
	      dl_req->number_pdu++;
	      dl_req->tl.tag = NFAPI_DL_CONFIG_REQUEST_BODY_TAG;

	      eNB->DL_req[CC_id].sfn_sf = frameP<<4 | subframeP;
	      eNB->DL_req[CC_id].header.message_id = NFAPI_DL_CONFIG_REQUEST;

	      fill_nfapi_dlsch_config(eNB, dl_req, TBS, -1
				      /* retransmission, no pdu_index */
				      , rnti, 0,	// type 0 allocation from 7.1.6 in 36.213
				      0,	// virtual_resource_block_assignment_flag, unused here
				      0,	// resource_block_coding, to be filled in later
				      getQm(UE_list->UE_template[CC_id][UE_id].oldmcs1[harq_pid]), round & 3,	// redundancy version
				      1,	// transport blocks
				      0,	// transport block to codeword swap flag
				      cc[CC_id].p_eNB == 1 ? 0 : 1,	// transmission_scheme
				      1,	// number of layers
				      1,	// number of subbands
				      //                      uint8_t codebook_index,
				      4,	// UE category capacity
				      UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->pdsch_ConfigDedicated->p_a, 0,	// delta_power_offset for TM5
				      0,	// ngap
				      0,	// nprb
				      cc[CC_id].p_eNB == 1 ? 1 : 2,	// transmission mode
				      0,	//number of PRBs treated as one subband, not used here
				      0	// number of beamforming vectors, not used here
				      );

	      LOG_D(MAC,
		    "Filled NFAPI configuration for DCI/DLSCH %d, retransmission round %d\n",
		    eNB->pdu_index[CC_id], round);

	      program_dlsch_acknak(module_idP, CC_id, UE_id,
				   frameP, subframeP,
				   dl_config_pdu->
				   dci_dl_pdu.dci_dl_pdu_rel8.
				   cce_idx);
	      // No TX request for retransmission (check if null request for FAPI)
	    } else {
	      LOG_W(MAC,
		    "Frame %d, Subframe %d: Dropping DLSCH allocation for UE %d\%x, infeasible CCE allocation\n",
		    frameP, subframeP, UE_id, rnti);
	    }
	  }


	  add_ue_dlsch_info(module_idP,
			    CC_id, UE_id, subframeP,
			    S_DL_SCHEDULED);

	  //eNB_UE_stats->dlsch_trials[round]++;
	  UE_list->eNB_UE_stats[CC_id][UE_id].num_retransmission += 1;
	  UE_list->eNB_UE_stats[CC_id][UE_id].rbs_used_retx = nb_rb;
	  UE_list->eNB_UE_stats[CC_id][UE_id].total_rbs_used_retx += nb_rb;
	  UE_list->eNB_UE_stats[CC_id][UE_id].dlsch_mcs1 = eNB_UE_stats->dlsch_mcs1;
	  UE_list->eNB_UE_stats[CC_id][UE_id].dlsch_mcs2 = eNB_UE_stats->dlsch_mcs1;
	} else {
	  LOG_D(MAC,
		"[eNB %d] Frame %d CC_id %d : don't schedule UE %d, its retransmission takes more resources than we have\n",
		module_idP, frameP, CC_id, UE_id);
	}
      } else {		/* This is a potentially new SDU opportunity */
	rlc_status.bytes_in_buffer = 0;

	// Now check RLC information to compute number of required RBs
	// get maximum TBS size for RLC request
	TBS = get_TBS_DL(eNB_UE_stats->dlsch_mcs1, nb_available_rb);

	// add the length for  all the control elements (timing adv, drx, etc) : header + payload

	if (ue_sched_ctl->ta_timer == 0) {
	  ta_update = ue_sched_ctl->ta_update;
	  /* if we send TA then set timer to not send it for a while */
	  if (ta_update != 31) ue_sched_ctl->ta_timer = 20;
	  /* reset ta_update */
	  ue_sched_ctl->ta_update = 31;
	} else {
	  ta_update = 31;
	}

	ta_len = (ta_update != 31) ? 2 : 0;

	// RLC data on DCCH
	if (TBS - ta_len - header_length_total - sdu_length_total - 3 > 0) {
	  rlc_status = mac_rlc_status_ind(module_idP, rnti, module_idP, frameP, subframeP, ENB_FLAG_YES, MBMS_FLAG_NO, DCCH,
                                          TBS - ta_len - header_length_total - sdu_length_total - 3);

	  sdu_lengths[0] = 0;

	  if (rlc_status.bytes_in_buffer > 0) {
	    LOG_D(MAC, "[eNB %d] SFN/SF %d.%d, DL-DCCH->DLSCH CC_id %d, Requesting %d bytes from RLC (RRC message)\n",
		  module_idP, frameP, subframeP, CC_id,
		  TBS - ta_len - header_length_total - sdu_length_total - 3);

	    sdu_lengths[0] = mac_rlc_data_req(module_idP, rnti, module_idP, frameP, ENB_FLAG_YES, MBMS_FLAG_NO, DCCH,
                                              TBS, //not used
					      (char *)&dlsch_buffer[0]);

	    T(T_ENB_MAC_UE_DL_SDU, T_INT(module_idP),
	      T_INT(CC_id), T_INT(rnti), T_INT(frameP),
	      T_INT(subframeP), T_INT(harq_pid), T_INT(DCCH),
	      T_INT(sdu_lengths[0]));

	    LOG_D(MAC, "[eNB %d][DCCH] CC_id %d Got %d bytes from RLC\n",
		  module_idP, CC_id, sdu_lengths[0]);

	    sdu_length_total = sdu_lengths[0];
	    sdu_lcids[0] = DCCH;
            UE_list->eNB_UE_stats[CC_id][UE_id].lcid_sdu[0] = DCCH;
            UE_list->eNB_UE_stats[CC_id][UE_id].sdu_length_tx[DCCH] = sdu_lengths[0];
	    UE_list->eNB_UE_stats[CC_id][UE_id].num_pdu_tx[DCCH] += 1;
	    UE_list->eNB_UE_stats[CC_id][UE_id].num_bytes_tx[DCCH] += sdu_lengths[0];

            header_length_last = 1 + 1 + (sdu_lengths[0] >= 128);
            header_length_total += header_length_last;

	    num_sdus = 1;

#ifdef DEBUG_eNB_SCHEDULER
	    LOG_T(MAC,
		  "[eNB %d][DCCH] CC_id %d Got %d bytes :",
		  module_idP, CC_id, sdu_lengths[0]);

	    for (j = 0; j < sdu_lengths[0]; j++) {
	      LOG_T(MAC, "%x ", dlsch_buffer[j]);
	    }

	    LOG_T(MAC, "\n");
#endif
	  }
	}

	// RLC data on DCCH1
	if (TBS - ta_len - header_length_total - sdu_length_total - 3 > 0) {
	  rlc_status = mac_rlc_status_ind(module_idP, rnti, module_idP, frameP, subframeP, ENB_FLAG_YES, MBMS_FLAG_NO, DCCH + 1,
                                          TBS - ta_len - header_length_total - sdu_length_total - 3);

	  // DCCH SDU
	  sdu_lengths[num_sdus] = 0;

	  if (rlc_status.bytes_in_buffer > 0) {
	    LOG_D(MAC, "[eNB %d], Frame %d, DCCH1->DLSCH, CC_id %d, Requesting %d bytes from RLC (RRC message)\n",
		  module_idP, frameP, CC_id,
		  TBS - ta_len - header_length_total - sdu_length_total - 3);

	    sdu_lengths[num_sdus] += mac_rlc_data_req(module_idP, rnti, module_idP, frameP, ENB_FLAG_YES, MBMS_FLAG_NO, DCCH + 1,
                                                      TBS, //not used
						      (char *)&dlsch_buffer[sdu_length_total]);

	    T(T_ENB_MAC_UE_DL_SDU, T_INT(module_idP),
	      T_INT(CC_id), T_INT(rnti), T_INT(frameP),
	      T_INT(subframeP), T_INT(harq_pid),
	      T_INT(DCCH + 1), T_INT(sdu_lengths[num_sdus]));

	    sdu_lcids[num_sdus] = DCCH1;
	    sdu_length_total += sdu_lengths[num_sdus];
            UE_list->eNB_UE_stats[CC_id][UE_id].lcid_sdu[num_sdus] = DCCH1;
            UE_list->eNB_UE_stats[CC_id][UE_id].sdu_length_tx[DCCH1] = sdu_lengths[num_sdus];
	    UE_list->eNB_UE_stats[CC_id][UE_id].num_pdu_tx[DCCH1] += 1;
	    UE_list->eNB_UE_stats[CC_id][UE_id].num_bytes_tx[DCCH1] += sdu_lengths[num_sdus];

            header_length_last = 1 + 1 + (sdu_lengths[num_sdus] >= 128);
            header_length_total += header_length_last;

	    num_sdus++;

#ifdef DEBUG_eNB_SCHEDULER
	    LOG_T(MAC,
		  "[eNB %d][DCCH1] CC_id %d Got %d bytes :",
		  module_idP, CC_id, sdu_lengths[num_sdus]);

	    for (j = 0; j < sdu_lengths[num_sdus]; j++) {
	      LOG_T(MAC, "%x ", dlsch_buffer[j]);
	    }

	    LOG_T(MAC, "\n");
#endif
	  }
	}

	// TODO: lcid has to be sorted before the actual allocation (similar struct as ue_list).
	for (lcid = NB_RB_MAX - 1; lcid >= DTCH; lcid--) {
	  // TODO: check if the lcid is active

	  LOG_D(MAC, "[eNB %d], Frame %d, DTCH%d->DLSCH, Checking RLC status (tbs %d, len %d)\n",
		module_idP, frameP, lcid, TBS,
                TBS - ta_len - header_length_total - sdu_length_total - 3);

	  if (TBS - ta_len - header_length_total - sdu_length_total - 3 > 0) {
	    rlc_status = mac_rlc_status_ind(module_idP,
					    rnti,
					    module_idP,
					    frameP,
					    subframeP,
					    ENB_FLAG_YES,
					    MBMS_FLAG_NO,
					    lcid,
					    TBS - ta_len - header_length_total - sdu_length_total - 3);



	    if (rlc_status.bytes_in_buffer > 0) {
	      LOG_D(MAC,
		    "[eNB %d][USER-PLANE DEFAULT DRB] Frame %d : DTCH->DLSCH, Requesting %d bytes from RLC (lcid %d total hdr len %d)\n",
		    module_idP, frameP,
                    TBS - ta_len - header_length_total - sdu_length_total - 3,
		    lcid,
		    header_length_total);

	      sdu_lengths[num_sdus] = mac_rlc_data_req(module_idP, rnti, module_idP, frameP, ENB_FLAG_YES, MBMS_FLAG_NO, lcid,
                                                       TBS, //not used
						       (char *)&dlsch_buffer[sdu_length_total]);

	      T(T_ENB_MAC_UE_DL_SDU, T_INT(module_idP),
		T_INT(CC_id), T_INT(rnti), T_INT(frameP),
		T_INT(subframeP), T_INT(harq_pid),
		T_INT(lcid), T_INT(sdu_lengths[num_sdus]));

	      LOG_D(MAC,
		    "[eNB %d][USER-PLANE DEFAULT DRB] Got %d bytes for DTCH %d \n",
		    module_idP, sdu_lengths[num_sdus], lcid);

	      sdu_lcids[num_sdus] = lcid;
	      sdu_length_total += sdu_lengths[num_sdus];
	      UE_list->eNB_UE_stats[CC_id][UE_id].num_pdu_tx[lcid]++;
              UE_list->eNB_UE_stats[CC_id][UE_id].lcid_sdu[num_sdus] = lcid;
              UE_list->eNB_UE_stats[CC_id][UE_id].sdu_length_tx[lcid] = sdu_lengths[num_sdus];
	      UE_list->eNB_UE_stats[CC_id][UE_id].num_bytes_tx[lcid] += sdu_lengths[num_sdus];

              header_length_last = 1 + 1 + (sdu_lengths[num_sdus] >= 128);
              header_length_total += header_length_last;

	      num_sdus++;

	      UE_list->UE_sched_ctrl[UE_id].uplane_inactivity_timer = 0;
	    }
	  } else {
            // no TBS left
	    break;
	  }
	}

        /* last header does not have length field */
        if (header_length_total) {
          header_length_total -= header_length_last;
          header_length_total++;
        }

	// there is at least one SDU or TA command
	// if (num_sdus > 0 ){
	if (ta_len + sdu_length_total + header_length_total > 0) {

	  // Now compute number of required RBs for total sdu length
	  // Assume RAH format 2

	  mcs = eNB_UE_stats->dlsch_mcs1;

	  if (mcs == 0) {
	    nb_rb = 4;	// don't let the TBS get too small
	  } else {
	    nb_rb = min_rb_unit[CC_id];
	  }

	  TBS = get_TBS_DL(mcs, nb_rb);

	  while (TBS < sdu_length_total + header_length_total + ta_len) {
	    nb_rb += min_rb_unit[CC_id];	//

	    if (nb_rb > nb_available_rb) {	// if we've gone beyond the maximum number of RBs
	      // (can happen if N_RB_DL is odd)
	      TBS = get_TBS_DL(eNB_UE_stats->dlsch_mcs1, nb_available_rb);
	      nb_rb = nb_available_rb;
	      break;
	    }

	    TBS = get_TBS_DL(eNB_UE_stats->dlsch_mcs1, nb_rb);
	  }

	  if (nb_rb == ue_sched_ctl->pre_nb_available_rbs[CC_id]) {
	    for (j = 0; j < N_RBG[CC_id]; j++) {	// for indicating the rballoc for each sub-band
	      UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j] =	ue_sched_ctl->rballoc_sub_UE[CC_id][j];
	    }
	  } else {
	    nb_rb_temp = nb_rb;
	    j = 0;

	    while ((nb_rb_temp > 0) && (j < N_RBG[CC_id])) {
	      if (ue_sched_ctl->rballoc_sub_UE[CC_id][j] == 1) {
		UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][j] = ue_sched_ctl->rballoc_sub_UE[CC_id][j];

		if ((j == N_RBG[CC_id] - 1) &&
		    ((N_RB_DL[CC_id] == 25) ||
		     (N_RB_DL[CC_id] == 50))) {
		  nb_rb_temp = nb_rb_temp - min_rb_unit[CC_id] + 1;
		} else {
		  nb_rb_temp = nb_rb_temp - min_rb_unit[CC_id];
		}
	      }

	      j = j + 1;
	    }
	  }

	  // decrease mcs until TBS falls below required length
	  while ((TBS > sdu_length_total + header_length_total + ta_len) && (mcs > 0)) {
	    mcs--;
	    TBS = get_TBS_DL(mcs, nb_rb);
	  }

	  // if we have decreased too much or we don't have enough RBs, increase MCS
	  while ((TBS < sdu_length_total + header_length_total + ta_len)
		 && (((ue_sched_ctl->dl_pow_off[CC_id] > 0)
		      && (mcs < 28))
		     || ((ue_sched_ctl->dl_pow_off[CC_id] == 0)
			 && (mcs <= 15)))) {
	    mcs++;
	    TBS = get_TBS_DL(mcs, nb_rb);
	  }

	  LOG_D(MAC,
		"dlsch_mcs before and after the rate matching = (%d, %d)\n",
		eNB_UE_stats->dlsch_mcs1, mcs);

#ifdef DEBUG_eNB_SCHEDULER
	  LOG_D(MAC,
		"[eNB %d] CC_id %d Generated DLSCH header (mcs %d, TBS %d, nb_rb %d)\n",
		module_idP, CC_id, mcs, TBS, nb_rb);
	  // msg("[MAC][eNB ] Reminder of DLSCH with random data %d %d %d %d \n",
	  //  TBS, sdu_length_total, offset, TBS-sdu_length_total-offset);
#endif

	  if (TBS - header_length_total - sdu_length_total - ta_len <= 2) {
	    padding = TBS - header_length_total - sdu_length_total - ta_len;
	    post_padding = 0;
	  } else {
	    padding = 0;
	    post_padding = 1;
	  }

	  offset = generate_dlsch_header((unsigned char *) UE_list->DLSCH_pdu[CC_id][0][UE_id].payload[0], num_sdus,	//num_sdus
					 sdu_lengths,	//
					 sdu_lcids, 255,	// no drx
					 ta_update,	// timing advance
					 NULL,	// contention res id
					 padding, post_padding);

	  //#ifdef DEBUG_eNB_SCHEDULER
	  if (ta_update != 31) {
	    LOG_D(MAC,
		  "[eNB %d][DLSCH] Frame %d Generate header for UE_id %d on CC_id %d: sdu_length_total %d, num_sdus %d, sdu_lengths[0] %d, sdu_lcids[0] %d => payload offset %d,timing advance value : %d, padding %d,post_padding %d,(mcs %d, TBS %d, nb_rb %d),header_length %d\n",
		  module_idP, frameP, UE_id, CC_id,
		  sdu_length_total, num_sdus, sdu_lengths[0],
		  sdu_lcids[0], offset, ta_update, padding,
		  post_padding, mcs, TBS, nb_rb,
		  header_length_total);
	  }
	  //#endif
#ifdef DEBUG_eNB_SCHEDULER
	  LOG_T(MAC, "[eNB %d] First 16 bytes of DLSCH : \n");

	  for (i = 0; i < 16; i++) {
	    LOG_T(MAC, "%x.", dlsch_buffer[i]);
	  }

	  LOG_T(MAC, "\n");
#endif

	  // cycle through SDUs and place in dlsch_buffer
	  memcpy(&UE_list->DLSCH_pdu[CC_id][0][UE_id].payload[0][offset],
		 dlsch_buffer, sdu_length_total);
	  // memcpy(RC.mac[0].DLSCH_pdu[0][0].payload[0][offset],dcch_buffer,sdu_lengths[0]);

#if 0
	  // fill remainder of DLSCH with random data
	  for (j = 0; j < (TBS - sdu_length_total - offset); j++) {
	    UE_list->DLSCH_pdu[CC_id][0][UE_id].payload[0][offset + sdu_length_total + j] = (char) (taus() & 0xff);
	  }
#endif
	  // fill remainder of DLSCH with 0
	  for (j = 0; j < (TBS - sdu_length_total - offset); j++) {
	    UE_list->DLSCH_pdu[CC_id][0][UE_id].payload[0][offset + sdu_length_total + j] = 0;
	  }

	  if (opt_enabled == 1) {
	    trace_pdu(1, (uint8_t *)
		      UE_list->DLSCH_pdu[CC_id][0][UE_id].
		      payload[0], TBS, module_idP, 3,
		      UE_RNTI(module_idP, UE_id), eNB->frame,
		      eNB->subframe, 0, 0);
	    LOG_D(OPT,
		  "[eNB %d][DLSCH] CC_id %d Frame %d  rnti %x  with size %d\n",
		  module_idP, CC_id, frameP,
		  UE_RNTI(module_idP, UE_id), TBS);
	  }

	  T(T_ENB_MAC_UE_DL_PDU_WITH_DATA, T_INT(module_idP),
	    T_INT(CC_id), T_INT(rnti), T_INT(frameP),
	    T_INT(subframeP), T_INT(harq_pid),
	    T_BUFFER(UE_list->DLSCH_pdu[CC_id][0][UE_id].payload[0], TBS));

	  UE_list->UE_template[CC_id][UE_id].nb_rb[harq_pid] = nb_rb;

	  add_ue_dlsch_info(module_idP,
			    CC_id, UE_id, subframeP,
			    S_DL_SCHEDULED);
	  // store stats
	  eNB->eNB_stats[CC_id].dlsch_bytes_tx += sdu_length_total;
	  eNB->eNB_stats[CC_id].dlsch_pdus_tx += 1;

	  UE_list->eNB_UE_stats[CC_id][UE_id].rbs_used = nb_rb;
          UE_list->eNB_UE_stats[CC_id][UE_id].num_mac_sdu_tx = num_sdus;
	  UE_list->eNB_UE_stats[CC_id][UE_id].total_rbs_used += nb_rb;
	  UE_list->eNB_UE_stats[CC_id][UE_id].dlsch_mcs1 = eNB_UE_stats->dlsch_mcs1;
	  UE_list->eNB_UE_stats[CC_id][UE_id].dlsch_mcs2 = mcs;
	  UE_list->eNB_UE_stats[CC_id][UE_id].TBS = TBS;

	  UE_list->eNB_UE_stats[CC_id][UE_id].overhead_bytes = TBS - sdu_length_total;
	  UE_list->eNB_UE_stats[CC_id][UE_id].total_sdu_bytes += sdu_length_total;
	  UE_list->eNB_UE_stats[CC_id][UE_id].total_pdu_bytes += TBS;
	  UE_list->eNB_UE_stats[CC_id][UE_id].total_num_pdus += 1;

	  if (cc[CC_id].tdd_Config != NULL) {	// TDD
	    UE_list->UE_template[CC_id][UE_id].DAI++;
	    update_ul_dci(module_idP, CC_id, rnti,
			  UE_list->UE_template[CC_id][UE_id].
			  DAI);
	  }

	  // do PUCCH power control
	  // this is the normalized RX power
	  eNB_UE_stats = &UE_list->eNB_UE_stats[CC_id][UE_id];

	  /* TODO: fix how we deal with power, unit is not dBm, it's special from nfapi */
	  normalized_rx_power = ue_sched_ctl->pucch1_snr[CC_id];
	  target_rx_power = 208;

	  // this assumes accumulated tpc
	  // make sure that we are only sending a tpc update once a frame, otherwise the control loop will freak out
	  int32_t framex10psubframe = UE_list->UE_template[CC_id][UE_id].pucch_tpc_tx_frame * 10 + UE_list->UE_template[CC_id][UE_id].pucch_tpc_tx_subframe;
	  if (((framex10psubframe + 10) <= (frameP * 10 + subframeP)) ||	//normal case
	      ((framex10psubframe > (frameP * 10 + subframeP)) && (((10240 - framex10psubframe + frameP * 10 + subframeP) >= 10))))	//frame wrap-around
	    if (ue_sched_ctl->pucch1_cqi_update[CC_id] == 1) {
	      ue_sched_ctl->pucch1_cqi_update[CC_id] = 0;

	      UE_list->UE_template[CC_id][UE_id].pucch_tpc_tx_frame = frameP;
	      UE_list->UE_template[CC_id][UE_id].pucch_tpc_tx_subframe = subframeP;

	      if (normalized_rx_power > (target_rx_power + 4)) {
		tpc = 0;	//-1
	      } else if (normalized_rx_power < (target_rx_power - 4)) {
		tpc = 2;	//+1
	      } else {
		tpc = 1;	//0
	      }

	      LOG_D(MAC,
		    "[eNB %d] DLSCH scheduler: frame %d, subframe %d, harq_pid %d, tpc %d, normalized/target rx power %d/%d\n",
		    module_idP, frameP, subframeP, harq_pid, tpc,
		    normalized_rx_power, target_rx_power);

	    }	// Po_PUCCH has been updated
	    else {
	      tpc = 1;	//0
	    }	// time to do TPC update
	  else {
	    tpc = 1;	//0
	  }

	  dl_config_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu];
	  memset((void *) dl_config_pdu, 0,
		 sizeof(nfapi_dl_config_request_pdu_t));
	  dl_config_pdu->pdu_type = NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE;
	  dl_config_pdu->pdu_size = (uint8_t) (2 + sizeof(nfapi_dl_config_dci_dl_pdu));
	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format = NFAPI_DL_DCI_FORMAT_1;
	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level = get_aggregation(get_bw_index(module_idP, CC_id),
											ue_sched_ctl->dl_cqi[CC_id],
											format1);
	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tl.tag = NFAPI_DL_CONFIG_REQUEST_DCI_DL_PDU_REL8_TAG;
	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti = rnti;
	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type = 1;	// CRNTI : see Table 4-10 from SCF082 - nFAPI specifications
	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.transmission_power = 6000;	// equal to RS power

	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.harq_process = harq_pid;
	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tpc = tpc;	// dont adjust power when retransmitting
	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.new_data_indicator_1 = 1 - UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1 = mcs;
	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_1 = 0;
	  //deactivate second codeword
	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_2 = 0;
	  dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_2 = 1;
	  if (cc[CC_id].tdd_Config != NULL) {	//TDD
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.downlink_assignment_index = (UE_list->UE_template[CC_id][UE_id].DAI - 1) & 3;
	    LOG_D(MAC,
		  "[eNB %d] Initial transmission CC_id %d : harq_pid %d, dai %d, mcs %d\n",
		  module_idP, CC_id, harq_pid,
		  (UE_list->UE_template[CC_id][UE_id].DAI - 1),
		  mcs);
	  } else {
	    LOG_D(MAC,
		  "[eNB %d] Initial transmission CC_id %d : harq_pid %d, mcs %d\n",
		  module_idP, CC_id, harq_pid, mcs);

	  }

	  LOG_D(MAC, "Checking feasibility pdu %d (new sdu)\n",
		dl_req->number_pdu);

	  if (!CCE_allocation_infeasible(module_idP, CC_id, 1, subframeP,
					 dl_config_pdu->dci_dl_pdu.
					 dci_dl_pdu_rel8.aggregation_level, rnti)) {
	    ue_sched_ctl->round[CC_id][harq_pid] = 0;
	    dl_req->number_dci++;
	    dl_req->number_pdu++;
	    dl_req->tl.tag = NFAPI_DL_CONFIG_REQUEST_BODY_TAG;

	    eNB->DL_req[CC_id].sfn_sf = frameP<<4 | subframeP;
	    eNB->DL_req[CC_id].header.message_id = NFAPI_DL_CONFIG_REQUEST;

	    // Toggle NDI for next time
	    LOG_D(MAC,
		  "CC_id %d Frame %d, subframeP %d: Toggling Format1 NDI for UE %d (rnti %x/%d) oldNDI %d\n",
		  CC_id, frameP, subframeP, UE_id, rnti,
		  harq_pid,
		  UE_list->
		  UE_template[CC_id][UE_id].oldNDI[harq_pid]);

	    UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid] = 1 - UE_list->UE_template[CC_id][UE_id].oldNDI[harq_pid];
	    UE_list->UE_template[CC_id][UE_id].oldmcs1[harq_pid] = mcs;
	    UE_list->UE_template[CC_id][UE_id].oldmcs2[harq_pid] = 0;
	    AssertFatal(UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated != NULL,
			"physicalConfigDedicated is NULL\n");
	    AssertFatal(UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->pdsch_ConfigDedicated != NULL,
			"physicalConfigDedicated->pdsch_ConfigDedicated is NULL\n");

	    fill_nfapi_dlsch_config(eNB, dl_req, TBS, eNB->pdu_index[CC_id], rnti, 0,	// type 0 allocation from 7.1.6 in 36.213
				    0,	// virtual_resource_block_assignment_flag, unused here
				    0,	// resource_block_coding, to be filled in later
				    getQm(mcs), 0,	// redundancy version
				    1,	// transport blocks
				    0,	// transport block to codeword swap flag
				    cc[CC_id].p_eNB == 1 ? 0 : 1,	// transmission_scheme
				    1,	// number of layers
				    1,	// number of subbands
				    //                       uint8_t codebook_index,
				    4,	// UE category capacity
				    UE_list->UE_template[CC_id][UE_id].physicalConfigDedicated->pdsch_ConfigDedicated->p_a, 0,	// delta_power_offset for TM5
				    0,	// ngap
				    0,	// nprb
				    cc[CC_id].p_eNB == 1 ? 1 : 2,	// transmission mode
				    0,	//number of PRBs treated as one subband, not used here
				    0	// number of beamforming vectors, not used here
				    );
	    eNB->TX_req[CC_id].sfn_sf = fill_nfapi_tx_req(&eNB->TX_req[CC_id].tx_request_body,
							  (frameP * 10) + subframeP,
							  TBS, eNB->pdu_index[CC_id],
							  eNB->UE_list.DLSCH_pdu[CC_id][0][UE_id].payload[0]);

	    LOG_D(MAC,
		  "Filled NFAPI configuration for DCI/DLSCH/TXREQ %d, new SDU\n",
		  eNB->pdu_index[CC_id]);

	    eNB->pdu_index[CC_id]++;
	    program_dlsch_acknak(module_idP, CC_id, UE_id,
				 frameP, subframeP,
				 dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.cce_idx);
	  } else {
	    LOG_W(MAC,
		  "Frame %d, Subframe %d: Dropping DLSCH allocation for UE %d/%x, infeasible CCE allocations\n",
		  frameP, subframeP, UE_id, rnti);
	  }
	} else {	// There is no data from RLC or MAC header, so don't schedule

	}
      }

      if (cc[CC_id].tdd_Config != NULL) {	// TDD
	set_ul_DAI(module_idP, UE_id, CC_id, frameP, subframeP);
      }

    }			// UE_id loop
  }				// CC_id loop



  fill_DLSCH_dci(module_idP, frameP, subframeP, mbsfn_flag);

  stop_meas(&eNB->schedule_dlsch);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_SCHEDULE_DLSCH, VCD_FUNCTION_OUT);
}

//------------------------------------------------------------------------------
void
fill_DLSCH_dci(module_id_t module_idP,
	       frame_t frameP, sub_frame_t subframeP, int *mbsfn_flagP)
//------------------------------------------------------------------------------
{

  // loop over all allocated UEs and compute frequency allocations for PDSCH
  int UE_id = -1;
  uint8_t /* first_rb, */ nb_rb = 3;
  rnti_t rnti;
  //unsigned char *vrb_map;
  uint8_t rballoc_sub[25];
  //uint8_t number_of_subbands=13;

  //unsigned char round;
  unsigned char harq_pid;
  int i;
  int CC_id;
  eNB_MAC_INST *eNB = RC.mac[module_idP];
  UE_list_t *UE_list = &eNB->UE_list;
  int N_RBG;
  int N_RB_DL;
  COMMON_channels_t *cc;

  start_meas(&eNB->fill_DLSCH_dci);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME
    (VCD_SIGNAL_DUMPER_FUNCTIONS_FILL_DLSCH_DCI, VCD_FUNCTION_IN);

  for (CC_id = 0; CC_id < MAX_NUM_CCs; CC_id++) {
    LOG_D(MAC, "Doing fill DCI for CC_id %d\n", CC_id);

    if (mbsfn_flagP[CC_id] > 0)
      continue;

    cc = &eNB->common_channels[CC_id];
    N_RBG = to_rbg(cc->mib->message.dl_Bandwidth);
    N_RB_DL = to_prb(cc->mib->message.dl_Bandwidth);

    // UE specific DCIs
    for (UE_id = UE_list->head; UE_id >= 0;
	 UE_id = UE_list->next[UE_id]) {
      LOG_T(MAC, "CC_id %d, UE_id: %d => status %d\n", CC_id, UE_id,
	    eNB_dlsch_info[module_idP][CC_id][UE_id].status);

      if (eNB_dlsch_info[module_idP][CC_id][UE_id].status == S_DL_SCHEDULED) {

	// clear scheduling flag
	eNB_dlsch_info[module_idP][CC_id][UE_id].status = S_DL_WAITING;
	rnti = UE_RNTI(module_idP, UE_id);
	if (cc->tdd_Config) harq_pid = ((frameP * 10) + subframeP) % 10;
	else          	    harq_pid = ((frameP * 10) + subframeP) & 7;
	nb_rb = UE_list->UE_template[CC_id][UE_id].nb_rb[harq_pid];

	/// Synchronizing rballoc with rballoc_sub
	for (i = 0; i < N_RBG; i++) {
	  rballoc_sub[i] = UE_list->UE_template[CC_id][UE_id].rballoc_subband[harq_pid][i];
	}

	nfapi_dl_config_request_t *DL_req = &RC.mac[module_idP]->DL_req[0];
	nfapi_dl_config_request_pdu_t *dl_config_pdu;

	for (i = 0;
	     i < DL_req[CC_id].dl_config_request_body.number_pdu;
	     i++) {
	  dl_config_pdu = &DL_req[CC_id].dl_config_request_body.dl_config_pdu_list[i];
	  if ((dl_config_pdu->pdu_type == NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE)
	      && (dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti == rnti)
	      && (dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format != 1)) {
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding = allocate_prbs_sub(nb_rb, N_RB_DL, N_RBG,rballoc_sub);
	    dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_allocation_type = 0;
	  } else
	    if ((dl_config_pdu->pdu_type == NFAPI_DL_CONFIG_DLSCH_PDU_TYPE)
		&& (dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.rnti == rnti)
		&& (dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_allocation_type == 0)) {
	      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_block_coding =allocate_prbs_sub(nb_rb, N_RB_DL, N_RBG,rballoc_sub);
	    }
	}
      }
    }

  }

  stop_meas(&eNB->fill_DLSCH_dci);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_FILL_DLSCH_DCI, VCD_FUNCTION_OUT);
}

//------------------------------------------------------------------------------
unsigned char *get_dlsch_sdu(module_id_t module_idP,
			     int CC_id, frame_t frameP, rnti_t rntiP,
			     uint8_t TBindex)
//------------------------------------------------------------------------------
{

  int UE_id;
  eNB_MAC_INST *eNB = RC.mac[module_idP];

  if (rntiP == SI_RNTI) {
    LOG_D(MAC, "[eNB %d] CC_id %d Frame %d Get DLSCH sdu for BCCH \n",
	  module_idP, CC_id, frameP);

    return ((unsigned char *) &eNB->common_channels[CC_id].BCCH_pdu.payload[0]);
  }

  if (rntiP==P_RNTI) {
    LOG_D(MAC,"[eNB %d] CC_id %d Frame %d Get PCH sdu for PCCH \n", module_idP, CC_id, frameP);

    return((unsigned char *)&eNB->common_channels[CC_id].PCCH_pdu.payload[0]);
  }

  UE_id = find_UE_id(module_idP,rntiP);

  if (UE_id != -1) {
    LOG_D(MAC,
	  "[eNB %d] Frame %d:  CC_id %d Get DLSCH sdu for rnti %x => UE_id %d\n",
	  module_idP, frameP, CC_id, rntiP, UE_id);
    return ((unsigned char *) &eNB->UE_list.DLSCH_pdu[CC_id][TBindex][UE_id].payload[0]);
  } else {
    LOG_E(MAC,
	  "[eNB %d] Frame %d: CC_id %d UE with RNTI %x does not exist\n",
	  module_idP, frameP, CC_id, rntiP);
    return NULL;
  }

}


//------------------------------------------------------------------------------
void
update_ul_dci(module_id_t module_idP,
	      uint8_t CC_idP, rnti_t rntiP, uint8_t daiP)
//------------------------------------------------------------------------------
{

  nfapi_hi_dci0_request_t *HI_DCI0_req =
    &RC.mac[module_idP]->HI_DCI0_req[CC_idP];
  nfapi_hi_dci0_request_pdu_t *hi_dci0_pdu =
    &HI_DCI0_req->hi_dci0_request_body.hi_dci0_pdu_list[0];
  COMMON_channels_t *cc = &RC.mac[module_idP]->common_channels[CC_idP];
  int i;


  if (cc->tdd_Config != NULL) {	// TDD
    for (i = 0;
	 i <HI_DCI0_req->hi_dci0_request_body.number_of_dci + HI_DCI0_req->hi_dci0_request_body.number_of_dci;
	 i++) {

      if ((hi_dci0_pdu[i].pdu_type == NFAPI_HI_DCI0_DCI_PDU_TYPE) &&
	  (hi_dci0_pdu[i].dci_pdu.dci_pdu_rel8.rnti == rntiP))
	hi_dci0_pdu[i].dci_pdu.dci_pdu_rel8.dl_assignment_index = (daiP - 1) & 3;

    }
  }
}


//------------------------------------------------------------------------------
void
set_ue_dai(sub_frame_t subframeP,
	   int UE_id, uint8_t CC_id, uint8_t tdd_config,
	   UE_list_t * UE_list)
//------------------------------------------------------------------------------
{
  switch (tdd_config) {
  case 0:
    if ((subframeP == 0) || (subframeP == 1) || (subframeP == 3)
	|| (subframeP == 5) || (subframeP == 6) || (subframeP == 8)) {
      UE_list->UE_template[CC_id][UE_id].DAI = 0;
    }

    break;

  case 1:
    if ((subframeP == 0) || (subframeP == 4) || (subframeP == 5)
	|| (subframeP == 9)) {
      UE_list->UE_template[CC_id][UE_id].DAI = 0;
    }

    break;

  case 2:
    if ((subframeP == 4) || (subframeP == 5)) {
      UE_list->UE_template[CC_id][UE_id].DAI = 0;
    }

    break;

  case 3:
    if ((subframeP == 5) || (subframeP == 7) || (subframeP == 9)) {
      UE_list->UE_template[CC_id][UE_id].DAI = 0;
    }

    break;

  case 4:
    if ((subframeP == 0) || (subframeP == 6)) {
      UE_list->UE_template[CC_id][UE_id].DAI = 0;
    }

    break;

  case 5:
    if (subframeP == 9) {
      UE_list->UE_template[CC_id][UE_id].DAI = 0;
    }

    break;

  case 6:
    if ((subframeP == 0) || (subframeP == 1) || (subframeP == 5)
	|| (subframeP == 6) || (subframeP == 9)) {
      UE_list->UE_template[CC_id][UE_id].DAI = 0;
    }

    break;

  default:
    UE_list->UE_template[CC_id][UE_id].DAI = 0;
    LOG_N(MAC, "unknow TDD config %d\n", tdd_config);
    break;
  }
}

void schedule_PCH(module_id_t module_idP,frame_t frameP,sub_frame_t subframeP)
{
  /* DCI:format 1A/1C P-RNTI:0xFFFE */
  /* PDU:eNB_rrc_inst[Mod_idP].common_channels[CC_id].PCCH_pdu.payload */
  uint16_t                       pcch_sdu_length;
  int                            mcs = -1;
  int                            CC_id;
  eNB_MAC_INST                   *eNB      = RC.mac[module_idP];
  COMMON_channels_t              *cc;
  uint8_t                        *vrb_map;
  int                            n_rb_dl;
  int                            first_rb = -1;
  nfapi_dl_config_request_pdu_t  *dl_config_pdu;
  nfapi_tx_request_pdu_t         *TX_req;
  nfapi_dl_config_request_body_t *dl_req;
#ifdef FORMAT1C
  int                            gap_index = 0;      /* indicate which gap(1st or 2nd) is used (0:1st) */
  const int                      GAP_MAP [9][2] = {
    {-1, 0},        /* N_RB_DL [6-10] -1: |N_RB/2| 0: N/A*/
    {4, 0},         /* N_RB_DL [11] */
    {8, 0},         /* N_RB_DL [12-19] */
    {12, 0},        /* N_RB_DL [20-26] */
    {18, 0},        /* N_RB_DL [27-44] */
    {27, 0},        /* N_RB_DL [45-49] */
    {27, 9},        /* N_RB_DL [50-63] */
    {32, 16},       /* N_RB_DL [64-79] */
    {48, 16}        /* N_RB_DL [80-110] */
  };
  uint8_t                        n_rb_step = 0;
  uint8_t                        n_gap = 0;
  uint8_t                        n_vrb_dl = 0;
  uint8_t                        Lcrbs = 0;
  uint16_t                       rb_bit    = 168;    /* RB bit number value is unsure */
#endif

  start_meas(&eNB->schedule_pch);

  for (CC_id=0; CC_id<MAX_NUM_CCs; CC_id++) {
    cc              = &eNB->common_channels[CC_id];
    vrb_map         = (void*)&cc->vrb_map;
    n_rb_dl         = to_prb(cc->mib->message.dl_Bandwidth);
    dl_req          = &eNB->DL_req[CC_id].dl_config_request_body;
    for (uint16_t i = 0; i < NUMBER_OF_UE_MAX; i++) {
      if (UE_PF_PO[CC_id][i].enable_flag != TRUE) {
        continue;
      }
      if (frameP % UE_PF_PO[CC_id][i].T == UE_PF_PO[CC_id][i].PF_min && subframeP == UE_PF_PO[CC_id][i].PO) {
	pcch_sdu_length = mac_rrc_data_req(module_idP,
                                           CC_id,
                                           frameP,
                                           PCCH,1,
                                           &cc->PCCH_pdu.payload[0],
                                           i); // used for ue index
	if (pcch_sdu_length == 0) {
	  LOG_D(MAC,"[eNB %d] Frame %d subframe %d: PCCH not active(size = 0 byte)\n", module_idP,frameP, subframeP);
	  continue;
	}
	LOG_D(MAC,"[eNB %d] Frame %d subframe %d: PCCH->PCH CC_id %d UE_id %d, Received %d bytes \n", module_idP, frameP, subframeP, CC_id,i, pcch_sdu_length);
#ifdef FORMAT1C
	//NO SIB
	if ((subframeP == 1 || subframeP == 2 || subframeP == 4 || subframeP == 6 || subframeP == 9) ||
	    (subframeP == 5 && ((frameP % 2) != 0 && (frameP % 8) != 1))) {
	  switch (n_rb_dl) {
#if 0
	  case 6:
	    n_gap = n_rb_dl/2;  /* expect: 3 */
	    n_vrb_dl = 2*((n_gap < (n_rb_dl - n_gap)) ? n_gap : (n_rb_dl - n_gap));;  /* expect: 6 */
	    first_rb = 0;
	    break;
	  case 15:
	    n_gap = GAP_MAP[2][0];  /* expect: 8 */
	    n_vrb_dl = 2*((n_gap < (n_rb_dl - n_gap)) ? n_gap : (n_rb_dl - n_gap));  /* expect: 14 */
	    first_rb = 6;
	    break;
#endif
	  case 25:
	    n_gap = GAP_MAP[3][0];  /* expect: 12 */
	    n_vrb_dl = 2*((n_gap < (n_rb_dl - n_gap)) ? n_gap : (n_rb_dl - n_gap));  /* expect: 24 */
	    first_rb = 10;
	    break;
	  case 50:
	    n_gap = GAP_MAP[6][gap_index];  /* expect: 27 or 9 */
	    if (gap_index > 0) {
	      n_vrb_dl = (n_rb_dl / (2*n_gap)) * (2*n_gap);  /* 36 */
	    } else {
	      n_vrb_dl = 2*((n_gap < (n_rb_dl - n_gap)) ? n_gap : (n_rb_dl - n_gap));  /* expect: 46 */
	    }
	    first_rb = 24;
	    break;
	  case 100:
	    n_gap = GAP_MAP[8][gap_index];  /* expect: 48 or 16 */
	    if (gap_index > 0) {
	      n_vrb_dl = (n_rb_dl / (2*n_gap)) * (2*n_gap);  /* expect: 96 */
	    } else {
	      n_vrb_dl = 2*((n_gap < (n_rb_dl - n_gap)) ? n_gap : (n_rb_dl - n_gap));  /* expect: 96 */
	    }
	    first_rb = 48;
	    break;
	  }
	} else if (subframeP == 5 && ((frameP % 2) == 0 || (frameP % 8) == 1)) {  // SIB + paging
	  switch (n_rb_dl) {
#if 0
	  case 6:
	    n_gap = n_rb_dl/2;  /* expect: 3 */
	    n_vrb_dl = 2*((n_gap < (n_rb_dl - n_gap)) ? n_gap : (n_rb_dl - n_gap));;  /* expect: 6 */
	    first_rb = 0;
	    break;
	  case 15:
	    n_gap = GAP_MAP[2][0];  /* expect: 8 */
	    n_vrb_dl = 2*((n_gap < (n_rb_dl - n_gap)) ? n_gap : (n_rb_dl - n_gap));  /* expect: 14 */
	    first_rb = 10;
	    break;
#endif
	  case 25:
	    n_gap = GAP_MAP[3][0];  /* expect: 12 */
	    n_vrb_dl = 2*((n_gap < (n_rb_dl - n_gap)) ? n_gap : (n_rb_dl - n_gap));  /* expect: 24 */
	    first_rb = 14;
	    break;
	  case 50:
	    n_gap = GAP_MAP[6][gap_index];  /* expect: 27 or 9 */
	    if (gap_index > 0) {
	      n_vrb_dl = (n_rb_dl / (2*n_gap)) * (2*n_gap);  /* 36 */
	    } else {
	      n_vrb_dl = 2*((n_gap < (n_rb_dl - n_gap)) ? n_gap : (n_rb_dl - n_gap));  /* expect: 46 */
	    }
	    first_rb = 28;
	    break;
	  case 100:
	    n_gap = GAP_MAP[8][gap_index];  /* expect: 48 or 16 */
	    if (gap_index > 0) {
	      n_vrb_dl = (n_rb_dl / (2*n_gap)) * (2*n_gap);  /* expect: 96 */
	    } else {
	      n_vrb_dl = 2*((n_gap < (n_rb_dl - n_gap)) ? n_gap : (n_rb_dl - n_gap));  /* expect: 96 */
	    }
	    first_rb = 52;
	    break;
	  }
	}
	/* Get MCS for length of PCH */
	if (pcch_sdu_length <= TBStable1C[0]) {
	  mcs=0;
	} else if (pcch_sdu_length <= TBStable1C[1]) {
	  mcs=1;
	} else if (pcch_sdu_length <= TBStable1C[2]) {
	  mcs=2;
	} else if (pcch_sdu_length <= TBStable1C[3]) {
	  mcs=3;
	} else if (pcch_sdu_length <= TBStable1C[4]) {
	  mcs=4;
	} else if (pcch_sdu_length <= TBStable1C[5]) {
	  mcs=5;
	} else if (pcch_sdu_length <= TBStable1C[6]) {
	  mcs=6;
	} else if (pcch_sdu_length <= TBStable1C[7]) {
	  mcs=7;
	} else if (pcch_sdu_length <= TBStable1C[8]) {
	  mcs=8;
	} else if (pcch_sdu_length <= TBStable1C[9]) {
	  mcs=9;
	} else {
	  /* unexpected: pcch sdb size is over max value*/
	  LOG_E(MAC,"[eNB %d] Frame %d : PCCH->PCH CC_id %d, Received %d bytes is over max length(256) \n",
		module_idP, frameP,CC_id, pcch_sdu_length);
	  return;
	}
	rb_num = TBStable1C[mcs] / rb_bit + ( (TBStable1C[mcs] % rb_bit == 0)? 0: 1) + 1;
	/* calculate N_RB_STEP and Lcrbs */
	if (n_rb_dl < 50) {
	  n_rb_step = 2;
	  Lcrbs = rb_num / 2 + ((rb_num % 2 == 0) ? 0:2);
	} else {
	  n_rb_step = 4;
	  Lcrbs = rb_num / 4 + ((rb_num % 4 == 0) ? 0:4);
	}
	for(i = 0;i < Lcrbs ;i++){
	  vrb_map[first_rb+i] = 1;
	}
#else
	//NO SIB
	if ((subframeP == 1 || subframeP == 2 || subframeP == 4 || subframeP == 6 || subframeP == 9) ||
	    (subframeP == 5 && ((frameP % 2) != 0 && (frameP % 8) != 1))) {
	  switch (n_rb_dl) {
	  case 25:
	    first_rb = 10;
	    break;
	  case 50:
	    first_rb = 24;
	    break;
	  case 100:
	    first_rb = 48;
	    break;
	  }
	} else if (subframeP == 5 && ((frameP % 2) == 0 || (frameP % 8) == 1)) {  // SIB + paging
	  switch (n_rb_dl) {
	  case 25:
	    first_rb = 14;
	    break;
	  case 50:
	    first_rb = 28;
	    break;
	  case 100:
	    first_rb = 52;
	    break;
	  }
	}

	vrb_map[first_rb] = 1;
	vrb_map[first_rb+1] = 1;
	vrb_map[first_rb+2] = 1;
	vrb_map[first_rb+3] = 1;
	/* Get MCS for length of PCH */
	if (pcch_sdu_length <= get_TBS_DL(0,3)) {
	  mcs=0;
	} else if (pcch_sdu_length <= get_TBS_DL(1,3)) {
	  mcs=1;
	} else if (pcch_sdu_length <= get_TBS_DL(2,3)) {
	  mcs=2;
	} else if (pcch_sdu_length <= get_TBS_DL(3,3)) {
	  mcs=3;
	} else if (pcch_sdu_length <= get_TBS_DL(4,3)) {
	  mcs=4;
	} else if (pcch_sdu_length <= get_TBS_DL(5,3)) {
	  mcs=5;
	} else if (pcch_sdu_length <= get_TBS_DL(6,3)) {
	  mcs=6;
	} else if (pcch_sdu_length <= get_TBS_DL(7,3)) {
	  mcs=7;
	} else if (pcch_sdu_length <= get_TBS_DL(8,3)) {
	  mcs=8;
	} else if (pcch_sdu_length <= get_TBS_DL(9,3)) {
	  mcs=9;
	}
#endif
	dl_config_pdu                                                         = &dl_req->dl_config_pdu_list[dl_req->number_pdu];
	memset((void*)dl_config_pdu,0,sizeof(nfapi_dl_config_request_pdu_t));
	dl_config_pdu->pdu_type                                               = NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE;
	dl_config_pdu->pdu_size                                               = (uint8_t)(2+sizeof(nfapi_dl_config_dci_dl_pdu));
#ifdef FORMAT1C
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format                  = NFAPI_DL_DCI_FORMAT_1C;
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding       = getRIV(n_vrb_dl/n_rb_step, first_rb/n_rb_step, Lcrbs/n_rb_step);
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.ngap                        = n_gap;
#else
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format                  = NFAPI_DL_DCI_FORMAT_1A;
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.harq_process                = 0;
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tpc                         = 1; // no TPC
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.new_data_indicator_1        = 1;
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_1        = 1;
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding       = getRIV(n_rb_dl,first_rb,4);
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.virtual_resource_block_assignment_flag = 0;
#endif
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level           = 4;
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti                        = 0xFFFE; // P-RNTI
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type                   = 2;    // P-RNTI : see Table 4-10 from SCF082 - nFAPI specifications
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.transmission_power          = 6000; // equal to RS power
	dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1                       = mcs;

	// Rel10 fields
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel10.pdsch_start                           = 3;
	// Rel13 fields
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.ue_type                               = 0; // regular UE
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.pdsch_payload_type                    = 2; // not BR
	dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.initial_transmission_sf_io            = 0xFFFF;

	if (!CCE_allocation_infeasible(module_idP, CC_id, 0, subframeP, dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level, P_RNTI)) {
	  LOG_D(MAC,"Frame %d: Subframe %d : Adding common DCI for P_RNTI\n", frameP,subframeP);
	  dl_req->number_dci++;
	  dl_req->number_pdu++;
	  dl_config_pdu                                                                  = &dl_req->dl_config_pdu_list[dl_req->number_pdu];
	  memset((void*)dl_config_pdu,0,sizeof(nfapi_dl_config_request_pdu_t));
	  dl_config_pdu->pdu_type                                                        = NFAPI_DL_CONFIG_DLSCH_PDU_TYPE;
	  dl_config_pdu->pdu_size                                                        = (uint8_t)(2+sizeof(nfapi_dl_config_dlsch_pdu));
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index                              = eNB->pdu_index[CC_id];
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.rnti                                   = 0xFFFE;
#ifdef FORMAT1C
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_allocation_type               = 3;   // format 1C
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_block_coding                  = getRIV(n_vrb_dl/n_rb_step, first_rb/n_rb_step, Lcrbs/n_rb_step);
#else
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_allocation_type               = 2;   // format 1A/1B/1D
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_block_coding                  = getRIV(n_rb_dl,first_rb,4);
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.virtual_resource_block_assignment_flag = 0;   // localized
#endif
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.modulation                             = 2; //QPSK
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.redundancy_version                     = 1;
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_blocks                       = 1;// first block
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_block_to_codeword_swap_flag  = 0;
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transmission_scheme                    = (cc->p_eNB==1 ) ? 0 : 1;
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.number_of_layers                       = 1;
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.number_of_subbands                     = 1;
	  // dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.codebook_index                         = ;
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.ue_category_capacity                   = 1;
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pa                                     = 4; // 0 dB
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.delta_power_offset_index               = 0;
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.ngap                                   = 0;
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.nprb                                   = get_subbandsize(cc->mib->message.dl_Bandwidth); // ignored
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transmission_mode                      = (cc->p_eNB==1 ) ? 1 : 2;
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.num_bf_prb_per_subband                 = 1;
	  dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.num_bf_vector                          = 1;
	  // dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.bf_vector                    = ;
	  dl_req->number_pdu++;

	  eNB->TX_req[CC_id].sfn_sf                                                     = (frameP<<4)+subframeP;
	  TX_req                                                                         = &eNB->TX_req[CC_id].tx_request_body.tx_pdu_list[eNB->TX_req[CC_id].tx_request_body.number_of_pdus];
	  TX_req->pdu_length                                                             = pcch_sdu_length;
	  TX_req->pdu_index                                                              = eNB->pdu_index[CC_id]++;
	  TX_req->num_segments                                                           = 1;
	  TX_req->segments[0].segment_length                                             = pcch_sdu_length;
	  TX_req->segments[0].segment_data                                               = cc[CC_id].PCCH_pdu.payload;
	  eNB->TX_req[CC_id].tx_request_body.number_of_pdus++;
	} else {
	  LOG_E(MAC,"[eNB %d] CCid %d Frame %d, subframe %d : Cannot add DCI 1A/1C for Paging\n",module_idP, CC_id, frameP, subframeP);
	  continue;
	}

	if (opt_enabled == 1) {
	  trace_pdu(1,
		    &eNB->common_channels[CC_id].PCCH_pdu.payload[0],
		    pcch_sdu_length,
		    0xffff,
		    PCCH,
		    P_RNTI,
		    eNB->frame,
		    eNB->subframe,
		    0,
		    0);
	  LOG_D(OPT,"[eNB %d][PCH] Frame %d trace pdu for CC_id %d rnti %x with size %d\n",
		module_idP, frameP, CC_id, 0xffff, pcch_sdu_length);
	}
	eNB->eNB_stats[CC_id].total_num_pcch_pdu+=1;
	eNB->eNB_stats[CC_id].pcch_buffer=pcch_sdu_length;
	eNB->eNB_stats[CC_id].total_pcch_buffer+=pcch_sdu_length;
	eNB->eNB_stats[CC_id].pcch_mcs=mcs;
	//paging first_rb log
	LOG_D(MAC,"[eNB %d] Frame %d subframe %d PCH: paging_ue_index %d pcch_sdu_length %d mcs %d first_rb %d\n",
	      module_idP, frameP, subframeP, UE_PF_PO[CC_id][i].ue_index_value, pcch_sdu_length, mcs, first_rb);

	pthread_mutex_lock(&ue_pf_po_mutex);
	memset(&UE_PF_PO[CC_id][i], 0, sizeof(UE_PF_PO_t));
	pthread_mutex_unlock(&ue_pf_po_mutex);
      }
    }
  }
  /* this might be misleading when pcch is inactive */
  stop_meas(&eNB->schedule_pch);
  return;
}
