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

#define _GNU_SOURCE

#include "LAYER2/MAC/mac.h"
#include "LAYER2/MAC/mac_proto.h"
#include "LAYER2/MAC/mac_extern.h"
#include "common/utils/LOG/log.h"
#include "nfapi/oai_integration/vendor_ext.h"
#include "common/utils/LOG/vcd_signal_dumper.h"
#include "UTIL/OPT/opt.h"
#include "OCG.h"
#include "OCG_extern.h"
#include "PHY/LTE_TRANSPORT/transport_common_proto.h"

#include "RRC/LTE/rrc_extern.h"
#include "RRC/L2_INTERFACE/openair_rrc_L2_interface.h"
#include "openair2/RRC/LTE/rrc_eNB_UE_context.h"

//#include "LAYER2/MAC/pre_processor.c"
#include "pdcp.h"

#include "SIMULATION/TOOLS/sim.h" // for taus

#include "assertions.h"

#include "intertask_interface.h"

#include <dlfcn.h>

#include "T.h"

#define ENABLE_MAC_PAYLOAD_DEBUG
//#define DEBUG_eNB_SCHEDULER 1

#include "common/ran_context.h"
extern RAN_CONTEXT_t RC;


//------------------------------------------------------------------------------
void
add_ue_dlsch_info(module_id_t module_idP,
                  int CC_id,
                  int UE_id,
                  sub_frame_t subframeP,
                  UE_DLSCH_STATUS status,
                  rnti_t rnti)
//------------------------------------------------------------------------------
{
  eNB_DLSCH_INFO *info = &eNB_dlsch_info[module_idP][CC_id][UE_id];
  // LOG_D(MAC, "%s(module_idP:%d, CC_id:%d, UE_id:%d, subframeP:%d, status:%d) serving_num:%d rnti:%x\n", __FUNCTION__, module_idP, CC_id, UE_id, subframeP, status, eNB_dlsch_info[module_idP][CC_id][UE_id].serving_num, UE_RNTI(module_idP,UE_id));
  info->rnti = rnti;
  //  info->weight = weight;
  info->subframe = subframeP;
  info->status = status;
  info->serving_num++;
  return;
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

  if (short_padding == 1 || short_padding == 2) {
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
    AssertFatal(timing_advance_cmd < 64, "timing_advance_cmd %d > 63\n",
                timing_advance_cmd);
    ((TIMING_ADVANCE_CMD *) ce_ptr)->TA = timing_advance_cmd;    //(timing_advance_cmd+31)&0x3f;
    LOG_D(MAC, "timing advance =%d (%d)\n",
          timing_advance_cmd,
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
    LOG_T(MAC, "[eNB ][RAPROC] Generate contention resolution msg: %x.%x.%x.%x.%x.%x\n",
          ue_cont_res_id[0],
          ue_cont_res_id[1],
          ue_cont_res_id[2],
          ue_cont_res_id[3],
          ue_cont_res_id[4],
          ue_cont_res_id[5]);
    memcpy(ce_ptr,
           ue_cont_res_id,
           6);
    ce_ptr += 6;
    // msg("(cont_res) : offset %d\n",ce_ptr-mac_header_control_elements);
  }

  //msg("last_size %d,mac_header_ptr %p\n",last_size,mac_header_ptr);

  for (i = 0; i < num_sdus; i++) {
    LOG_T(MAC, "[eNB] Generate DLSCH header num sdu %d len sdu %d\n",
          num_sdus,
          sdu_lengths[i]);

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
      LOG_D(MAC, "[eNB] generate long sdu, size %x (MSB %x, LSB %x)\n",
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
  if (post_padding > 0) {    // we have lots of padding at the end of the packet
    mac_header_ptr->E = 1;
    mac_header_ptr += last_size;
    // add a padding element
    mac_header_ptr->R = 0;
    mac_header_ptr->E = 0;
    mac_header_ptr->LCID = SHORT_PADDING;
    mac_header_ptr++;
  } else {            // no end of packet padding
    // last SDU subhead is of fixed type (sdu length implicitly to be computed at UE)
    mac_header_ptr++;
  }

  //msg("After subheaders %d\n",(uint8_t*)mac_header_ptr - mac_header);

  if ((ce_ptr - mac_header_control_elements) > 0) {
    // printf("Copying %d bytes for control elements\n",ce_ptr-mac_header_control_elements);
    memcpy((void *) mac_header_ptr,
           mac_header_control_elements,
           ce_ptr - mac_header_control_elements);
    mac_header_ptr += (unsigned char) (ce_ptr - mac_header_control_elements);
  }

  //msg("After CEs %d\n",(uint8_t*)mac_header_ptr - mac_header);
  return ((unsigned char *) mac_header_ptr - mac_header);
}

//------------------------------------------------------------------------------
void
set_ul_DAI(int module_idP,
           int UE_idP,
           int CC_idP,
           int frameP,
           int subframeP)
//------------------------------------------------------------------------------
{
  eNB_MAC_INST *eNB = RC.mac[module_idP];
  UE_info_t *UE_info = &eNB->UE_info;
  unsigned char DAI;
  COMMON_channels_t *cc = &eNB->common_channels[CC_idP];

  if (cc->tdd_Config != NULL) {    //TDD
    DAI = (UE_info->UE_template[CC_idP][UE_idP].DAI - 1) & 3;
    LOG_D(MAC, "[eNB %d] CC_id %d Frame %d, subframe %d: DAI %d for UE %d\n",
          module_idP,
          CC_idP,
          frameP,
          subframeP,
          DAI,
          UE_idP);
    // Save DAI for Format 0 DCI

    switch (cc->tdd_Config->subframeAssignment) {
      case 0:
        //      if ((subframeP==0)||(subframeP==1)||(subframeP==5)||(subframeP==6))
        break;

      case 1:
        switch (subframeP) {
          case 0:
          case 1:
            UE_info->UE_template[CC_idP][UE_idP].DAI_ul[7] = DAI;
            break;

          case 4:
            UE_info->UE_template[CC_idP][UE_idP].DAI_ul[8] = DAI;
            break;

          case 5:
          case 6:
            UE_info->UE_template[CC_idP][UE_idP].DAI_ul[2] = DAI;
            break;

          case 9:
            UE_info->UE_template[CC_idP][UE_idP].DAI_ul[3] = DAI;
            break;
        }

        break;

      case 2:
        //      if ((subframeP==3)||(subframeP==8))
        //  UE_info->UE_template[CC_idP][UE_idP].DAI_ul = DAI;
        break;

      case 3:

        //if ((subframeP==6)||(subframeP==8)||(subframeP==0)) {
        //  LOG_D(MAC,"schedule_ue_spec: setting UL DAI to %d for subframeP %d => %d\n",DAI,subframeP, ((subframeP+8)%10)>>1);
        //  UE_info->UE_template[CC_idP][UE_idP].DAI_ul[((subframeP+8)%10)>>1] = DAI;
        //}
        switch (subframeP) {
          case 5:
          case 6:
          case 1:
            UE_info->UE_template[CC_idP][UE_idP].DAI_ul[2] = DAI;
            break;

          case 7:
          case 8:
            UE_info->UE_template[CC_idP][UE_idP].DAI_ul[3] = DAI;
            break;

          case 9:
          case 0:
            UE_info->UE_template[CC_idP][UE_idP].DAI_ul[4] = DAI;
            break;

          default:
            break;
        }

        break;

      case 4:
        //      if ((subframeP==8)||(subframeP==9))
        //  UE_info->UE_template[CC_idP][UE_idP].DAI_ul = DAI;
        break;

      case 5:
        //      if (subframeP==8)
        //  UE_info->UE_template[CC_idP][UE_idP].DAI_ul = DAI;
        break;

      case 6:
        //      if ((subframeP==1)||(subframeP==4)||(subframeP==6)||(subframeP==9))
        //  UE_info->UE_template[CC_idP][UE_idP].DAI_ul = DAI;
        break;

      default:
        break;
    }
  }

  return;
}

void ue_rrc_release(rnti_t rnti) {
  if(rrc_release_info.num_UEs == 0 || rlc_am_mui.rrc_mui_num == 0) {
    return;
  }

  while(pthread_mutex_trylock(&rrc_release_freelist)) {
    /* spin... */
  }

  uint16_t release_total = 0;

  for (uint16_t n = 0; n < NUMBER_OF_UE_MAX; n++) {
    RRC_release_ctrl_t *release_ctrl = &rrc_release_info.RRC_release_ctrl[n];
    if(release_ctrl->flag == 0)
      continue;

    release_total++;
    if(release_ctrl->flag == 1 && release_ctrl->rnti == rnti) {
      for(uint16_t mui_num = 0; mui_num < rlc_am_mui.rrc_mui_num; mui_num++) {
        if(release_ctrl->rrc_eNB_mui == rlc_am_mui.rrc_mui[mui_num]) {
          release_ctrl->flag = 3;
          LOG_D(MAC,
                "DLSCH Release send:index %d rnti %x mui %d mui_num %d flag 1->3\n",
                n, rnti, rlc_am_mui.rrc_mui[mui_num], mui_num);
          break;
        }
      }
    }

    if(release_ctrl->flag == 2 && release_ctrl->rnti == rnti) {
      for (uint16_t mui_num = 0; mui_num < rlc_am_mui.rrc_mui_num; mui_num++) {
        if(release_ctrl->rrc_eNB_mui == rlc_am_mui.rrc_mui[mui_num]) {
          release_ctrl->flag = 4;
          LOG_D(MAC, "DLSCH Release send:index %d rnti %x mui %d mui_num %d flag 2->4\n",
                n,
                rnti,
                rlc_am_mui.rrc_mui[mui_num],
                mui_num);
          break;
        }
      }
    }

    if(release_total >= rrc_release_info.num_UEs)
      break;
  }

  pthread_mutex_unlock(&rrc_release_freelist);
}

void check_ra_rnti_mui(module_id_t mod_id, int CC_id, frame_t f, sub_frame_t sf, rnti_t rnti) {
  LTE_TDD_Config_t *tdd_config = RC.mac[mod_id]->common_channels[CC_id].tdd_Config;
  int harq_pid = frame_subframe2_dl_harq_pid(tdd_config, f, sf);
  RA_t *ra = &RC.mac[mod_id]->common_channels[CC_id].ra[0];
  for (uint8_t ra_ii = 0; ra_ii < NB_RA_PROC_MAX; ra_ii++, ra++) {
    if ((ra->rnti == rnti) && (ra->state == MSGCRNTI)) {
      for (uint16_t mui_num = 0; mui_num < rlc_am_mui.rrc_mui_num; mui_num++) {
        if (ra->crnti_rrc_mui == rlc_am_mui.rrc_mui[mui_num]) {
          ra->crnti_harq_pid = harq_pid;
          ra->state = MSGCRNTI_ACK;
          break;
        }
      }
    }
  }
}

//------------------------------------------------------------------------------
void
schedule_dlsch(module_id_t module_idP, frame_t frameP, sub_frame_t subframeP, int *mbsfn_flag) {
  if (is_pmch_subframe(frameP, subframeP, &RC.eNB[module_idP][0]->frame_parms))
    return;

  for (int CC_id = 0; CC_id < RC.nb_mac_CC[module_idP]; CC_id++) {
    // for TDD: check that we have to act here, otherwise skip
    COMMON_channels_t *cc = &RC.mac[module_idP]->common_channels[CC_id];
    if (cc->tdd_Config) {
      int tdd_sfa = cc->tdd_Config->subframeAssignment;

      switch (subframeP) {
        case 0:
          // always continue
          break;

        case 1:
        case 2:
          continue;

        case 3:
          if (tdd_sfa != 2 && tdd_sfa != 5)
            continue;
          break;

        case 4:
          if (tdd_sfa != 1 && tdd_sfa != 2 && tdd_sfa != 4 && tdd_sfa != 5)
            continue;
          break;

        case 5:
          break;

        case 6:
        case 7:
          if (tdd_sfa != 3 && tdd_sfa != 4 && tdd_sfa != 5)
            continue;
          break;

        case 8:
          if (tdd_sfa != 2 && tdd_sfa != 3 && tdd_sfa != 4 && tdd_sfa != 5)
            continue;
          break;

        case 9:
          if (tdd_sfa == 0)
            continue;
          break;
      }
    }

    if (mbsfn_flag[CC_id] != 0)
      continue;

    schedule_ue_spec(module_idP, CC_id, frameP, subframeP);
  }
}

// changes to pre-processor for eMTC
//------------------------------------------------------------------------------

void  getRepetition(UE_TEMPLATE *pue_template,unsigned int *maxRep, unsigned int *narrowBandindex) {
  LTE_EPDCCH_SetConfig_r11_t *epdcch_setconfig_r11;
  AssertFatal(pue_template->physicalConfigDedicated !=NULL, "no RRC physical configuration for this UE ") ;
  AssertFatal(pue_template->physicalConfigDedicated->ext4 !=NULL, "no RRC physical configuration for this UE ") ;
  AssertFatal(pue_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.choice.setup.setConfigToAddModList_r11->list.count > 0,"epdcch config list is empty") ;
  epdcch_setconfig_r11 = pue_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.choice.setup.setConfigToAddModList_r11->list.array[0] ;
  AssertFatal(epdcch_setconfig_r11->ext2 !=NULL && epdcch_setconfig_r11->ext2->mpdcch_config_r13 !=NULL," mpdcch config not found")  ;
  *maxRep = epdcch_setconfig_r11->ext2->mpdcch_config_r13->choice.setup.mpdcch_NumRepetition_r13  ;
  *narrowBandindex = epdcch_setconfig_r11->ext2->mpdcch_config_r13->choice.setup.mpdcch_Narrowband_r13  ;
}

//------------------------------------------------------------------------------
/*
* Schedule the DLSCH
*/
void
schedule_ue_spec(module_id_t module_idP,
                 int CC_id,
                 frame_t frameP,
                 sub_frame_t subframeP)
//------------------------------------------------------------------------------
{
  unsigned char dlsch_buffer[MAX_DLSCH_PAYLOAD_BYTES];
  eNB_MAC_INST *eNB = RC.mac[module_idP];
  COMMON_channels_t *cc = eNB->common_channels;
  UE_info_t *UE_info = &eNB->UE_info;
  const int dl_Bandwidth = cc[CC_id].mib->message.dl_Bandwidth;
  const int N_RB_DL = to_prb(dl_Bandwidth);
  const int N_RBG = to_rbg(eNB->common_channels[CC_id].mib->message.dl_Bandwidth);
  int total_nb_available_rb = N_RB_DL;
  nfapi_dl_config_request_body_t *dl_req = &eNB->DL_req[CC_id].dl_config_request_body;

  start_meas(&eNB->schedule_dlsch);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_SCHEDULE_DLSCH,
                                          VCD_FUNCTION_IN);

  for (int i = 0; i < N_RB_DL; i++)
    if (cc[CC_id].vrb_map[i] != 0)
      total_nb_available_rb--;

  // store the global enb stats:
  eNB->eNB_stats[CC_id].num_dlactive_UEs = UE_info->num_UEs;
  eNB->eNB_stats[CC_id].available_prbs = total_nb_available_rb;
  eNB->eNB_stats[CC_id].total_available_prbs += total_nb_available_rb;
  eNB->eNB_stats[CC_id].dlsch_bytes_tx = 0;
  eNB->eNB_stats[CC_id].dlsch_pdus_tx = 0;

  // CALLING Pre_Processor for downlink scheduling
  // (Returns estimation of RBs required by each UE and the allocation on sub-band)
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_PREPROCESSOR,
                                          VCD_FUNCTION_IN);
  start_meas(&eNB->schedule_dlsch_preprocessor);
  eNB->pre_processor_dl.dl(module_idP, CC_id, frameP, subframeP);
  stop_meas(&eNB->schedule_dlsch_preprocessor);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_DLSCH_PREPROCESSOR,
                                          VCD_FUNCTION_OUT);

  for (int UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id]) {
    LOG_D(MAC, "doing schedule_ue_spec for CC_id %d UE %d\n",
          CC_id,
          UE_id);
    UE_sched_ctrl_t *ue_sched_ctrl = &UE_info->UE_sched_ctrl[UE_id];
    UE_TEMPLATE *ue_template = &UE_info->UE_template[CC_id][UE_id];
    eNB_UE_STATS *eNB_UE_stats = &UE_info->eNB_UE_stats[CC_id][UE_id];
    eNB_UE_stats->TBS = 0;
    const rnti_t rnti = ue_template->rnti;

    // If TDD
    if (cc[CC_id].tdd_Config != NULL) {    //TDD
      set_ue_dai(subframeP,
                 UE_id,
                 CC_id,
                 cc[CC_id].tdd_Config->subframeAssignment,
                 UE_info);
      // update UL DAI after DLSCH scheduling
      set_ul_DAI(module_idP,
                 UE_id,
                 CC_id,
                 frameP,
                 subframeP);
    }

    ue_rrc_release(rnti);
    check_ra_rnti_mui(module_idP, CC_id, frameP, subframeP, rnti);

    if (ue_sched_ctrl->pre_nb_available_rbs[CC_id] == 0) { // no RBs allocated
      LOG_D(MAC, "%d.%d: no RBs allocated for UE %d on CC_id %d\n",
            frameP,
            subframeP,
            UE_id,
            CC_id);
      continue;
    }

    int dci_dl_pdu_idx = ue_sched_ctrl->pre_dci_dl_pdu_idx;
    if (dci_dl_pdu_idx < 0) {
      dci_dl_pdu_idx = CCE_try_allocate_dlsch(
          module_idP, CC_id, subframeP, UE_id, ue_sched_ctrl->dl_cqi[CC_id]);
      if (dci_dl_pdu_idx < 0) {
        LOG_W(MAC, "%d.%d: Dropping Allocation for RNTI 0x%04x/UE %d\n",
              frameP,
              subframeP,
              rnti,
              UE_id);
        continue;
      }
    }

    /* verify it is the right UE */
    nfapi_dl_config_request_pdu_t *dci = &dl_req->dl_config_pdu_list[dci_dl_pdu_idx];
    nfapi_dl_config_request_pdu_t *dlsch = &dl_req->dl_config_pdu_list[dci_dl_pdu_idx + 1];
    if (dci->pdu_type != NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE
        || dci->dci_dl_pdu.dci_dl_pdu_rel8.rnti != rnti
        || dlsch->pdu_type != NFAPI_DL_CONFIG_DLSCH_PDU_TYPE
        || dlsch->dlsch_pdu.dlsch_pdu_rel8.rnti != rnti) {
      LOG_E(MAC, "illegal dl_config_pdu_list index %d for UE %d/RNTI %04x\n",
            dci_dl_pdu_idx,
            UE_id,
            rnti);
      continue;
    }

    const int harq_pid = frame_subframe2_dl_harq_pid(cc->tdd_Config,
                                                     frameP,
                                                     subframeP);
    const int round_DL = ue_sched_ctrl->round[CC_id][harq_pid];
    eNB_UE_stats->crnti = rnti;
    eNB_UE_stats->rrc_status = mac_eNB_get_rrc_status(module_idP, rnti);
    eNB_UE_stats->harq_pid = harq_pid;
    eNB_UE_stats->harq_round = round_DL;
    eNB_UE_stats->dlsch_rounds[round_DL&7]++;
    if (eNB_UE_stats->rrc_status < RRC_RECONFIGURED) {
      ue_sched_ctrl->uplane_inactivity_timer = 0;
    }

    LOG_D(MAC, "[eNB %d] Frame %d: Scheduling UE %d on CC_id %d (rnti %x, harq_pid %d, round %d, rb %d, cqi %d, mcs %d, rrc %d)\n",
          module_idP,
          frameP,
          UE_id,
          CC_id,
          rnti,
          harq_pid,
          round_DL,
          ue_sched_ctrl->pre_nb_available_rbs[CC_id],
          ue_sched_ctrl->dl_cqi[CC_id],
          eNB_UE_stats->dlsch_mcs1,
          eNB_UE_stats->rrc_status);

    /* Process retransmission  */
    if (round_DL != 8) {
      // get freq_allocation
      const int nb_rb = ue_template->nb_rb[harq_pid];
      int TBS = get_TBS_DL(ue_template->oldmcs1[harq_pid], nb_rb);
      const uint32_t rbc = allocate_prbs_sub(
          nb_rb, N_RB_DL, N_RBG, ue_sched_ctrl->rballoc_sub_UE[CC_id]);

      if (nb_rb > ue_sched_ctrl->pre_nb_available_rbs[CC_id])
        LOG_W(MAC,
              "[eNB %d] %d.%d CC_id %d : should not schedule UE %d, its "
              "retransmission takes more resources than we have\n",
              module_idP,
              frameP,
              subframeP,
              CC_id,
              UE_id);

      /* CDRX */
      ue_sched_ctrl->harq_rtt_timer[CC_id][harq_pid] = 1; // restart HARQ RTT timer

      if (ue_sched_ctrl->cdrx_configured) {
        ue_sched_ctrl->drx_retransmission_timer[harq_pid] = 0; // stop drx retransmission

        /*
         * Note: contrary to the spec drx_retransmission_timer[harq_pid] is reset not stop.
         */
        if (harq_pid == 0) {
          VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DRX_RETRANSMISSION_HARQ0, (unsigned long) ue_sched_ctrl->drx_retransmission_timer[0]);
        }
      }

      if (cc[CC_id].tdd_Config != NULL) {
        ue_template->DAI++;
        update_ul_dci(module_idP,
                      CC_id,
                      rnti,
                      ue_template->DAI,
                      subframeP);
        LOG_D(MAC, "DAI update: CC_id %d subframeP %d: UE %d, DAI %d\n",
              CC_id,
              subframeP,
              UE_id,
              ue_template->DAI);
      }

      nfapi_dl_config_request_pdu_t *dl_config_pdu = &dl_req->dl_config_pdu_list[dci_dl_pdu_idx];
      switch (get_tmode(module_idP, CC_id, UE_id)) {
        case 1:
        case 2:
        case 7:
        default:
          LOG_D(MAC, "retransmission DL_REQ: rnti:%x\n",
                rnti);
          dl_config_pdu->pdu_size = (uint8_t) (2 + sizeof(nfapi_dl_config_dci_dl_pdu));
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format = NFAPI_DL_DCI_FORMAT_1;
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type = 1; // CRNTI: see Table 4-10 from SCF082 - nFAPI specifications
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.transmission_power = 6000; // equal to RS power
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.harq_process = harq_pid;
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tpc = 1; // Don't adjust power when retransmitting
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.new_data_indicator_1 = ue_template->oldNDI[harq_pid];
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1 = ue_template->oldmcs1[harq_pid];
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_1 = round_DL & 3;
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding = rbc;
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_allocation_type = 0;

          // TDD
          if (cc[CC_id].tdd_Config != NULL) {
            dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.downlink_assignment_index = (ue_template->DAI - 1) & 3;
            LOG_D(MAC, "[eNB %d] Retransmission CC_id %d : harq_pid %d, round %d, dai %d, mcs %d\n",
                  module_idP,
                  CC_id,
                  harq_pid,
                  round_DL,
                  ue_template->DAI - 1,
                  ue_template->oldmcs1[harq_pid]);
          } else {
            LOG_D(MAC, "[eNB %d] Retransmission CC_id %d : harq_pid %d, round %d, mcs %d\n",
                  module_idP,
                  CC_id,
                  harq_pid,
                  round_DL,
                  ue_template->oldmcs1[harq_pid]);
          }

          dl_req->tl.tag = NFAPI_DL_CONFIG_REQUEST_BODY_TAG;
          eNB->DL_req[CC_id].sfn_sf = frameP<<4 | subframeP;
          eNB->DL_req[CC_id].header.message_id = NFAPI_DL_CONFIG_REQUEST;
          fill_nfapi_dlsch_config(&dl_req->dl_config_pdu_list[dci_dl_pdu_idx + 1],
                                  TBS,
                                  -1,   // retransmission, no pdu_index
                                  rnti,
                                  0,    // type 0 allocation from 7.1.6 in 36.213
                                  0,    // virtual_resource_block_assignment_flag, unused here
                                  rbc,  // resource_block_coding
                                  getQm(ue_template->oldmcs1[harq_pid]),
                                  round_DL & 3, // redundancy version
                                  1,    // transport blocks
                                  0,    // transport block to codeword swap flag
                                  cc[CC_id].p_eNB == 1 ? 0 : 1,    // transmission_scheme
                                  1,    // number of layers
                                  1,    // number of subbands
                                  //                      uint8_t codebook_index,
                                  4,    // UE category capacity
                                  ue_template->physicalConfigDedicated->pdsch_ConfigDedicated->p_a,
                                  0,    // delta_power_offset for TM5
                                  0,    // ngap
                                  0,    // nprb
                                  cc[CC_id].p_eNB == 1 ? 1 : 2,    // transmission mode
                                  0,    //number of PRBs treated as one subband, not used here
                                  0);   // number of beamforming vectors, not used here
          LOG_D(MAC, "Filled NFAPI configuration for DCI/DLSCH %d, retransmission round %d\n",
                eNB->pdu_index[CC_id],
                round_DL);
          program_dlsch_acknak(module_idP,
                               CC_id,
                               UE_id,
                               frameP,
                               subframeP,
                               dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.cce_idx);
          // No TX request for retransmission (check if null request for FAPI)
      }

      eNB_UE_stats->num_retransmission += 1;
      eNB_UE_stats->rbs_used_retx = nb_rb;
      eNB_UE_stats->total_rbs_used_retx += nb_rb;
      eNB_UE_stats->dlsch_mcs2 = eNB_UE_stats->dlsch_mcs1;
      eNB_UE_stats->TBS = TBS;
    } else {
      // Now check RLC information to compute number of required RBs
      // get maximum TBS size for RLC request
      const int nb_rb = ue_sched_ctrl->pre_nb_available_rbs[CC_id];
      int TBS = get_TBS_DL(eNB_UE_stats->dlsch_mcs1, nb_rb);
      const uint32_t rbc = allocate_prbs_sub(nb_rb, N_RB_DL, N_RBG, ue_sched_ctrl->rballoc_sub_UE[CC_id]);

      // add the length for  all the control elements (timing adv, drx, etc) : header + payload

      if (ue_sched_ctrl->ta_timer)
        ue_sched_ctrl->ta_timer--;

      int ta_update = 31;
      if (ue_sched_ctrl->ta_timer == 0) {
        ta_update = ue_sched_ctrl->ta_update;

        /* if we send TA then set timer to not send it for a while */
        if (ta_update != 31) {
          ue_sched_ctrl->ta_timer = 20;
        }

        /* reset ta_update */
        ue_sched_ctrl->ta_update = 31;
        ue_sched_ctrl->ta_update_f = 31.0;
      }

      int ta_len = (ta_update != 31) ? 2 : 0;


      int num_sdus = 0;
      uint16_t sdu_lengths[NB_RB_MAX];
      int sdu_length_total = 0;
      int header_length_total = 0;
      int header_length_last = 0;
      unsigned char sdu_lcids[NB_RB_MAX];
      for (int i = 0; i < ue_sched_ctrl->dl_lc_num; ++i) {
        const int lcid = ue_sched_ctrl->dl_lc_ids[i]; // will match DCCH, DCCH1, DTCH

        if (TBS - ta_len - header_length_total - sdu_length_total - 3 <= 0)
          break;

        if (ue_sched_ctrl->dl_lc_bytes[i] == 0) // no data in this LC!
          continue;

        const uint32_t data =
            min(ue_sched_ctrl->dl_lc_bytes[i],
                TBS - ta_len - header_length_total - sdu_length_total - 3);
        LOG_D(MAC,
              "[eNB %d] SFN/SF %d.%d, LC%d->DLSCH CC_id %d, Requesting %d "
              "bytes from RLC (RRC message)\n",
              module_idP,
              frameP,
              subframeP,
              lcid,
              CC_id,
              data);

        sdu_lengths[num_sdus] = mac_rlc_data_req(module_idP,
                                                 rnti,
                                                 module_idP,
                                                 frameP,
                                                 ENB_FLAG_YES,
                                                 MBMS_FLAG_NO,
                                                 lcid,
                                                 data,
                                                 (char *)&dlsch_buffer[sdu_length_total],
                                                 0,
                                                 0
                                               );

#ifdef DEBUG_eNB_SCHEDULER
          LOG_T(MAC, "[eNB %d][%d] CC_id %d Got %d bytes :",
                module_idP,
                lcid,
                CC_id,
                sdu_lengths[num_sdus]);
          for (int j = 0; j < sdu_lengths[num_sdus]; ++j)
            LOG_T(MAC, "%x ", dlsch_buffer[sdu_length_total + j]);
          LOG_T(MAC, "\n");
#endif

        T(T_ENB_MAC_UE_DL_SDU,
          T_INT(module_idP),
          T_INT(CC_id),
          T_INT(rnti),
          T_INT(frameP),
          T_INT(subframeP),
          T_INT(harq_pid),
          T_INT(lcid),
          T_INT(sdu_lengths[num_sdus]));
        LOG_D(MAC, "[eNB %d][LC %d] CC_id %d Got %d bytes from RLC\n",
              module_idP,
              lcid,
              CC_id,
              sdu_lengths[num_sdus]);
        sdu_lcids[num_sdus]               = lcid;
        sdu_length_total                 += sdu_lengths[num_sdus];
        eNB_UE_stats->num_pdu_tx[lcid]   += 1;
        eNB_UE_stats->lcid_sdu[num_sdus]  = lcid;
        eNB_UE_stats->sdu_length_tx[lcid] = sdu_lengths[num_sdus];
        eNB_UE_stats->num_bytes_tx[lcid] += sdu_lengths[num_sdus];
        header_length_last                = 1 + 1 + (sdu_lengths[num_sdus] >= 128);
        header_length_total              += header_length_last;
        num_sdus                         += 1;

        if (lcid >= 3) { // if this is DTCH
            ue_sched_ctrl->uplane_inactivity_timer = 0;

            // reset RRC inactivity timer after uplane activity
            rrc_eNB_ue_context_t *ue_contextP = rrc_eNB_get_ue_context(RC.rrc[module_idP], rnti);
            if (ue_contextP == NULL)
              LOG_E(MAC, "[eNB %d] CC_id %d Couldn't find the context associated to UE (RNTI %d) and reset RRC inactivity timer\n",
                    module_idP,
                    CC_id,
                    rnti);
            ue_contextP->ue_context.ue_rrc_inactivity_timer = 1;
        }
      }

      /* Last header does not have length field */
      if (header_length_total) {
        header_length_total -= header_length_last;
        header_length_total++;
      }

      // there is at least one SDU or TA command
      if (ta_len + sdu_length_total + header_length_total > 0) {
        // Now compute number of required RBs for total sdu length
        // Assume RAH format 2
        int mcs = eNB_UE_stats->dlsch_mcs1;

        /* TODO make special function */
        // decrease mcs until TBS falls below required length
        while ((TBS > sdu_length_total + header_length_total + ta_len) && (mcs > 0)) {
          mcs--;
          TBS = get_TBS_DL(mcs, nb_rb);
        }

        // if we have decreased too much or we don't have enough RBs, increase MCS
        while (TBS < sdu_length_total + header_length_total + ta_len && mcs < 28) {
            // the second part of this condition is useless since dl_pow_off is
            // always 2?
            //&& ((ue_sched_ctrl->dl_pow_off[CC_id] > 0 && mcs < 28) || (ue_sched_ctrl->dl_pow_off[CC_id] == 0 && mcs <= 15))) {
          mcs++;
          TBS = get_TBS_DL(mcs,
                           nb_rb);
        }

        LOG_D(MAC, "dlsch_mcs before and after the rate matching = (%d, %d), TBS %d, nb_rb %d\n",
              eNB_UE_stats->dlsch_mcs1, mcs, TBS, nb_rb);

        int post_padding = TBS - header_length_total - sdu_length_total - ta_len > 2;
        int padding = post_padding ? 0 : TBS - header_length_total - sdu_length_total - ta_len;

        const int offset = generate_dlsch_header((unsigned char *) UE_info->DLSCH_pdu[CC_id][0][UE_id].payload[0],
                                                 num_sdus,    //num_sdus
                                                 sdu_lengths,    //
                                                 sdu_lcids,
                                                 255,    // no drx
                                                 ta_update,    // timing advance
                                                 NULL,    // contention res id
                                                 padding,
                                                 post_padding);

        //#ifdef DEBUG_eNB_SCHEDULER
        if (ta_update != 31) {
          LOG_D(MAC,
                "[eNB %d][DLSCH] Frame %d Generate header for UE_id %d on CC_id %d: sdu_length_total %d, num_sdus %d, sdu_lengths[0] %d, sdu_lcids[0] %d => payload offset %d,timing advance value : %d, padding %d,post_padding %d,(mcs %d, TBS %d, nb_rb %d),header_length %d\n",
                module_idP,
                frameP,
                UE_id,
                CC_id,
                sdu_length_total,
                num_sdus,
                sdu_lengths[0],
                sdu_lcids[0],
                offset,
                ta_update,
                padding,
                post_padding,
                mcs,
                TBS,
                nb_rb,
                header_length_total);
        }

        //#endif
#ifdef DEBUG_eNB_SCHEDULER
        LOG_T(MAC, "[eNB %d] First 16 bytes of DLSCH : \n");

        for (int i = 0; i < 16; i++) {
          LOG_T(MAC, "%x.",
                dlsch_buffer[i]);
        }

        LOG_T(MAC, "\n");
#endif
        // cycle through SDUs and place in dlsch_buffer
        DLSCH_PDU *dlsch_pdu = &UE_info->DLSCH_pdu[CC_id][0][UE_id];
        memcpy(&dlsch_pdu->payload[0][offset], dlsch_buffer, sdu_length_total);

        // fill remainder of DLSCH with 0
        for (int j = 0; j < TBS - sdu_length_total - offset; j++) {
          dlsch_pdu->payload[0][offset + sdu_length_total + j] = 0;
        }

        trace_pdu(DIRECTION_DOWNLINK,
                  (uint8_t *) dlsch_pdu->payload[0],
                  TBS,
                  module_idP,
                  WS_C_RNTI,
                  UE_RNTI(module_idP,
                          UE_id),
                  eNB->frame,
                  eNB->subframe,
                  0,
                  0);
        T(T_ENB_MAC_UE_DL_PDU_WITH_DATA,
          T_INT(module_idP),
          T_INT(CC_id),
          T_INT(rnti),
          T_INT(frameP),
          T_INT(subframeP),
          T_INT(harq_pid),
          T_BUFFER(dlsch_pdu->payload[0],
                   TBS));
        ue_template->nb_rb[harq_pid] = nb_rb;
        // store stats
        eNB->eNB_stats[CC_id].dlsch_bytes_tx += sdu_length_total;
        eNB->eNB_stats[CC_id].dlsch_pdus_tx += 1;
        eNB_UE_stats->rbs_used = nb_rb;
        eNB_UE_stats->num_mac_sdu_tx = num_sdus;
        eNB_UE_stats->total_rbs_used += nb_rb;
        eNB_UE_stats->dlsch_mcs2 = mcs;
        eNB_UE_stats->TBS = TBS;
        eNB_UE_stats->overhead_bytes = TBS - sdu_length_total;
        eNB_UE_stats->total_sdu_bytes += sdu_length_total;
        eNB_UE_stats->total_pdu_bytes += TBS;
        eNB_UE_stats->total_num_pdus += 1;

        if (cc[CC_id].tdd_Config != NULL) { // TDD
          ue_template->DAI++;
          update_ul_dci(module_idP,
                        CC_id,
                        rnti,
                        ue_template->DAI,
                        subframeP);
        }

        // do PUCCH power control
        // this is the snr
        // unit is not dBm, it's special from nfapi
        // converting to dBm
        int snr = ue_sched_ctrl->pucch1_snr[CC_id];
        int target_snr = eNB->puCch10xSnr / 10;
        // this assumes accumulated tpc
        // make sure that we are only sending a tpc update once a frame, otherwise the control loop will freak out
        int32_t framex10psubframe = ue_template->pucch_tpc_tx_frame * 10 + ue_template->pucch_tpc_tx_subframe;

        int tpc = 1;
        if (framex10psubframe + 10 <= (frameP * 10) + subframeP ||  //normal case
            (framex10psubframe > (frameP * 10) + subframeP && 10240 - framex10psubframe + (frameP * 10) + subframeP >= 10)) { //frame wrap-around
          if (ue_sched_ctrl->pucch1_cqi_update[CC_id] == 1) {
            ue_sched_ctrl->pucch1_cqi_update[CC_id] = 0;
            ue_template->pucch_tpc_tx_frame = frameP;
            ue_template->pucch_tpc_tx_subframe = subframeP;

            if (snr > target_snr + PUCCH_PCHYST) {
              tpc = 0;  //-1
	      ue_sched_ctrl->pucch_tpc_accumulated[CC_id]--;
            } else if (snr < target_snr - PUCCH_PCHYST) {
              tpc = 2;  //+1
              ue_sched_ctrl->pucch_tpc_accumulated[CC_id]--;
            } else {
              tpc = 1;  //0
            }

            LOG_D(MAC, "[eNB %d] DLSCH scheduler: frame %d, subframe %d, harq_pid %d, tpc %d (accumulated %d), snr/target snr %d/%d (normal case)\n",
                  module_idP,
                  frameP,
                  subframeP,
                  harq_pid,
                  tpc,
		  ue_sched_ctrl->pucch_tpc_accumulated[CC_id],
                  snr,
                  target_snr);
          } // Po_PUCCH has been updated
        }

        nfapi_dl_config_request_pdu_t *dl_config_pdu = &dl_req->dl_config_pdu_list[dci_dl_pdu_idx];
        dl_config_pdu->pdu_size = (uint8_t) (2 + sizeof(nfapi_dl_config_dci_dl_pdu));
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format = NFAPI_DL_DCI_FORMAT_1;
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type = 1;    // CRNTI : see Table 4-10 from SCF082 - nFAPI specifications
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.transmission_power = 6000;    // equal to RS power
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.harq_process = harq_pid;
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tpc = tpc;    // dont adjust power when retransmitting
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.new_data_indicator_1 = 1 - ue_template->oldNDI[harq_pid];
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1 = mcs;
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_1 = 0;
        //deactivate second codeword
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_2 = 0;
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_2 = 1;
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding = rbc;
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_allocation_type = 0;

        if (cc[CC_id].tdd_Config != NULL) {    //TDD
          dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.downlink_assignment_index = (ue_template->DAI - 1) & 3;
          LOG_D(MAC, "[eNB %d] Initial transmission CC_id %d : harq_pid %d, dai %d, mcs %d\n",
                module_idP,
                CC_id,
                harq_pid,
                (ue_template->DAI - 1),
                mcs);
        } else {
          LOG_D(MAC, "[eNB %d] Initial transmission CC_id %d : harq_pid %d, mcs %d\n",
                module_idP,
                CC_id,
                harq_pid,
                mcs);
        }

        LOG_D(MAC, "Checking feasibility pdu %d (new sdu)\n",
              dl_req->number_pdu);

        ue_sched_ctrl->round[CC_id][harq_pid] = 0;
        dl_req->tl.tag = NFAPI_DL_CONFIG_REQUEST_BODY_TAG;
        eNB->DL_req[CC_id].sfn_sf = frameP << 4 | subframeP;
        eNB->DL_req[CC_id].header.message_id = NFAPI_DL_CONFIG_REQUEST;
        /* CDRX */
        ue_sched_ctrl->harq_rtt_timer[CC_id][harq_pid] = 1; // restart HARQ RTT timer

        if (ue_sched_ctrl->cdrx_configured) {
          ue_sched_ctrl->drx_inactivity_timer = 1; // restart drx inactivity timer when new transmission
          ue_sched_ctrl->drx_retransmission_timer[harq_pid] = 0; // stop drx retransmission
          /*
           * Note: contrary to the spec drx_retransmission_timer[harq_pid] is reset not stop.
           */
          VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DRX_INACTIVITY, (unsigned long) ue_sched_ctrl->drx_inactivity_timer);

          if (harq_pid == 0) {
            VCD_SIGNAL_DUMPER_DUMP_VARIABLE_BY_NAME(VCD_SIGNAL_DUMPER_VARIABLES_DRX_RETRANSMISSION_HARQ0, (unsigned long) ue_sched_ctrl->drx_retransmission_timer[0]);
          }
        }

        // Toggle NDI for next time
        LOG_D(MAC, "CC_id %d Frame %d, subframeP %d: Toggling Format1 NDI for UE %d (rnti %x/%d) oldNDI %d\n",
              CC_id,
              frameP,
              subframeP,
              UE_id,
              rnti,
              harq_pid,
              ue_template->oldNDI[harq_pid]);
        ue_template->oldNDI[harq_pid] = 1 - ue_template->oldNDI[harq_pid];
        ue_template->oldmcs1[harq_pid] = mcs;
        ue_template->oldmcs2[harq_pid] = 0;
        AssertFatal(ue_template->physicalConfigDedicated != NULL, "physicalConfigDedicated is NULL\n");
        AssertFatal(ue_template->physicalConfigDedicated->pdsch_ConfigDedicated != NULL,
                    "physicalConfigDedicated->pdsch_ConfigDedicated is NULL\n");
        fill_nfapi_dlsch_config(&dl_req->dl_config_pdu_list[dci_dl_pdu_idx + 1],
                                TBS,
                                eNB->pdu_index[CC_id],
                                rnti,
                                0, // type 0 allocation from 7.1.6 in 36.213
                                0,  // virtual_resource_block_assignment_flag, unused here
                                rbc,// resource_block_coding
                                getQm(mcs),
                                0,  // redundancy version
                                1,  // transport blocks
                                0,  // transport block to codeword swap flag
                                cc[CC_id].p_eNB == 1 ? 0 : 1, // transmission_scheme
                                1,  // number of layers
                                1,  // number of subbands
                                //                       uint8_t codebook_index,
                                4,  // UE category capacity
                                ue_template->physicalConfigDedicated->pdsch_ConfigDedicated->p_a,
                                0,  // delta_power_offset for TM5
                                0,  // ngap
                                0,  // nprb
                                cc[CC_id].p_eNB == 1 ? 1 : 2, // transmission mode
                                0,  //number of PRBs treated as one subband, not used here
                                0); // number of beamforming vectors, not used here
        eNB->TX_req[CC_id].sfn_sf = fill_nfapi_tx_req(&eNB->TX_req[CC_id].tx_request_body,
                                    (frameP * 10) + subframeP,
                                    TBS,
                                    eNB->pdu_index[CC_id],
                                    dlsch_pdu->payload[0]);
        LOG_D(MAC, "Filled NFAPI configuration for DCI/DLSCH/TXREQ %d, new SDU\n",
              eNB->pdu_index[CC_id]);
        eNB->pdu_index[CC_id]++;
        program_dlsch_acknak(module_idP,
                             CC_id,
                             UE_id,
                             frameP,
                             subframeP,
                             dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.cce_idx);
      }
    }
  }     // UE_id loop
  stop_meas(&eNB->schedule_dlsch);
  VCD_SIGNAL_DUMPER_DUMP_FUNCTION_BY_NAME(VCD_SIGNAL_DUMPER_FUNCTIONS_SCHEDULE_DLSCH,
                                          VCD_FUNCTION_OUT);
}

//------------------------------------------------------------------------------
/*
 * Default DLSCH scheduler for LTE-M
 */
void
schedule_ue_spec_br(module_id_t module_idP,
                    frame_t       frameP,
                    sub_frame_t subframeP)
//------------------------------------------------------------------------------
{
  int CC_id = 0;
  int UE_id = -1;
  int rvseq[4] = {0,2,3,1};
  int mcs = 0;
  int round_DL = 0;
  int ta_update = 0;
  int32_t tpc = 1;
  int32_t snr = 0;
  int32_t target_snr = 0;
  uint16_t TBS = 0;
  uint16_t j = 0;
  uint16_t sdu_lengths[NB_RB_MAX];
  uint16_t rnti = 0;
  uint16_t padding = 0;
  uint16_t post_padding = 0;
  uint16_t                       sdu_length_total = 0;
  mac_rlc_status_resp_t rlc_status;
  rrc_eNB_ue_context_t *ue_contextP = NULL;
  unsigned char header_len_dcch = 0;
  unsigned char header_len_dcch_tmp = 0;
  unsigned char header_len_dtch = 0;
  unsigned char header_len_dtch_tmp = 0;
  unsigned char header_len_dtch_last = 0;
  unsigned char ta_len = 0;
  unsigned char sdu_lcids[NB_RB_MAX];
  unsigned char lcid = 0;
  unsigned char  offset,num_sdus=0;
  unsigned char dlsch_buffer[MAX_DLSCH_PAYLOAD_BYTES];
  eNB_MAC_INST *mac = RC.mac[module_idP];
  COMMON_channels_t *cc = mac->common_channels;
  UE_info_t *UE_info = &mac->UE_info;
  UE_TEMPLATE *UE_template = NULL;
  UE_sched_ctrl_t *ue_sched_ctl = NULL;
  nfapi_dl_config_request_pdu_t *dl_config_pdu = NULL;
  nfapi_ul_config_request_pdu_t *ul_config_pdu = NULL;
  nfapi_tx_request_pdu_t *TX_req = NULL;
  nfapi_dl_config_request_body_t *dl_req = NULL;
  nfapi_ul_config_request_body_t *ul_req = NULL;
  struct LTE_PRACH_ConfigSIB_v1310 *ext4_prach = NULL;
  struct LTE_PUCCH_ConfigCommon_v1310 *ext4_pucch = NULL;
  LTE_PRACH_ParametersListCE_r13_t *prach_ParametersListCE_r13 = NULL;
  struct LTE_N1PUCCH_AN_InfoList_r13 *pucch_N1PUCCH_AN_InfoList_r13 = NULL;
  int             pucchreps[4] = { 1, 1, 1, 1 };
  int             n1pucchan[4] = { 0, 0, 0, 0 };
  uint32_t        ackNAK_absSF;
  int             first_rb;
  dl_req = &(mac->DL_req[CC_id].dl_config_request_body);
  dl_config_pdu = &(dl_req->dl_config_pdu_list[dl_req->number_pdu]);

  /* Return if frame is even */
  if ((frameP & 1) == 0) {
    return;
  }

  if (cc[CC_id].mib->message.schedulingInfoSIB1_BR_r13 == 0) {
    return;
  }

  if (cc[CC_id].radioResourceConfigCommon_BR) {
    ext4_prach = cc[CC_id].radioResourceConfigCommon_BR->ext4->prach_ConfigCommon_v1310;
    ext4_pucch = cc[CC_id].radioResourceConfigCommon_BR->ext4->pucch_ConfigCommon_v1310;
    prach_ParametersListCE_r13 = &ext4_prach->prach_ParametersListCE_r13;
    pucch_N1PUCCH_AN_InfoList_r13 = ext4_pucch->n1PUCCH_AN_InfoList_r13;
    AssertFatal (prach_ParametersListCE_r13 != NULL, "prach_ParametersListCE_r13 is null\n");
    AssertFatal (pucch_N1PUCCH_AN_InfoList_r13 != NULL, "pucch_N1PUCCH_AN_InfoList_r13 is null\n");
    /* Check to verify CE-Level compatibility in SIB2_BR */
    AssertFatal (prach_ParametersListCE_r13->list.count == pucch_N1PUCCH_AN_InfoList_r13->list.count, "prach_ParametersListCE_r13->list.count!= pucch_N1PUCCH_AN_InfoList_r13->list.count\n");

    switch (prach_ParametersListCE_r13->list.count) {
      case 4:
        n1pucchan[3] = *pucch_N1PUCCH_AN_InfoList_r13->list.array[3];
        AssertFatal (ext4_pucch->pucch_NumRepetitionCE_Msg4_Level3_r13 != NULL, "pucch_NumRepetitionCE_Msg4_Level3 shouldn't be NULL\n");
        pucchreps[3] = (int) (4 << *ext4_pucch->pucch_NumRepetitionCE_Msg4_Level3_r13);

      case 3:
        n1pucchan[2] = *pucch_N1PUCCH_AN_InfoList_r13->list.array[2];
        AssertFatal (ext4_pucch->pucch_NumRepetitionCE_Msg4_Level2_r13 != NULL, "pucch_NumRepetitionCE_Msg4_Level2 shouldn't be NULL\n");
        pucchreps[2] = (int) (4 << *ext4_pucch->pucch_NumRepetitionCE_Msg4_Level2_r13);

      case 2:
        n1pucchan[1] = *pucch_N1PUCCH_AN_InfoList_r13->list.array[1];
        AssertFatal (ext4_pucch->pucch_NumRepetitionCE_Msg4_Level1_r13 != NULL, "pucch_NumRepetitionCE_Msg4_Level1 shouldn't be NULL\n");
        pucchreps[1] = (int) (1 << *ext4_pucch->pucch_NumRepetitionCE_Msg4_Level1_r13);

      case 1:
        n1pucchan[0] = *pucch_N1PUCCH_AN_InfoList_r13->list.array[0];
        AssertFatal (ext4_pucch->pucch_NumRepetitionCE_Msg4_Level0_r13 != NULL, "pucch_NumRepetitionCE_Msg4_Level0 shouldn't be NULL\n");
        pucchreps[0] = (int) (1 << *ext4_pucch->pucch_NumRepetitionCE_Msg4_Level0_r13);
        break;

      default:
        AssertFatal (1 == 0, "Illegal count for prach_ParametersListCE_r13 %d\n", prach_ParametersListCE_r13->list.count);
    }
  }

  for (UE_id = UE_info->list.head; UE_id >= 0; UE_id = UE_info->list.next[UE_id]) {
    int harq_pid = 0;
    rnti = UE_RNTI(module_idP, UE_id);

    if (rnti==NOT_A_RNTI) {
      continue;
    }

    ue_sched_ctl = &UE_info->UE_sched_ctrl[UE_id];
    UE_template  = &UE_info->UE_template[CC_id][UE_id];

    if (UE_template->rach_resource_type == 0) {
      continue;
    }

    uint8_t rrc_status = mac_eNB_get_rrc_status(module_idP, rnti);

    if (rrc_status < RRC_CONNECTED) {
      continue;
    }

    round_DL = ue_sched_ctl->round[CC_id][harq_pid];
    AssertFatal (UE_template->physicalConfigDedicated != NULL, "UE_template->physicalConfigDedicated is null\n");
    AssertFatal (UE_template->physicalConfigDedicated->ext4 != NULL, "UE_template->physicalConfigDedicated->ext4 is null\n");
    AssertFatal (UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11 != NULL, "UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11 is null\n");
    AssertFatal (UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.present == LTE_EPDCCH_Config_r11__config_r11_PR_setup,
                 "UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.present != setup\n");
    AssertFatal (UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.choice.setup.setConfigToAddModList_r11 != NULL,
                 "UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.choice.setup.setConfigToAddModList_r11 = NULL\n");
    LTE_EPDCCH_SetConfig_r11_t *epdcch_setconfig_r11 = UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.choice.setup.setConfigToAddModList_r11->list.array[0];
    AssertFatal(epdcch_setconfig_r11 != NULL, "epdcch_setconfig_r11 is null\n");
    AssertFatal(epdcch_setconfig_r11->ext2 != NULL, "epdcch_setconfig_r11->ext2 is null\n");
    AssertFatal(epdcch_setconfig_r11->ext2->mpdcch_config_r13 != NULL, "epdcch_setconfig_r11->ext2->mpdcch_config_r13 is null");
    AssertFatal(epdcch_setconfig_r11->ext2->mpdcch_config_r13 != NULL, "epdcch_setconfig_r11->ext2->mpdcch_config_r13 is null");
    AssertFatal(epdcch_setconfig_r11->ext2->mpdcch_config_r13->present == LTE_EPDCCH_SetConfig_r11__ext2__mpdcch_config_r13_PR_setup,
                "epdcch_setconfig_r11->ext2->mpdcch_config_r13->present is not setup\n");
    AssertFatal(epdcch_setconfig_r11->ext2->numberPRB_Pairs_v1310 != NULL, "epdcch_setconfig_r11->ext2->numberPRB_Pairs_v1310 is null");
    AssertFatal(epdcch_setconfig_r11->ext2->numberPRB_Pairs_v1310->present == LTE_EPDCCH_SetConfig_r11__ext2__numberPRB_Pairs_v1310_PR_setup,
                "epdcch_setconfig_r11->ext2->numberPRB_Pairs_v1310->present is not setup\n");

    /* Simple scheduler for 1 repetition, 1 HARQ */
    if (subframeP == 5) { // MPDCCH
      if (round_DL < 8) LOG_D(MAC, "MPDCCH round_DL = %d in frame %d subframe %d\n", round_DL, frameP, subframeP);

      if (round_DL == 8) {
        rlc_status.bytes_in_buffer = 0;
        /* Now check RLC information to compute number of required RBs */
        /* Get maximum TBS size for RLC request */
        TBS = get_TBS_DL(9,6);

        /* Check first for RLC data on DCCH */

        /* Add the length for all the control elements (timing adv, drx, etc) : header + payload */

        if (ue_sched_ctl->ta_timer == 0) {
          ta_update = ue_sched_ctl->ta_update;

          /* If we send TA then set timer to not send it for a while */
          if (ta_update != 31)
            ue_sched_ctl->ta_timer = 20;

          /* Reset ta_update */
          ue_sched_ctl->ta_update = 31;
          ue_sched_ctl->ta_update_f = 31.0;
        } else {
          ta_update = 31;
        }

        ta_len = (ta_update != 31) ? 2 : 0;
        header_len_dcch = 2; // 2 bytes DCCH SDU subheader

        if (TBS - ta_len-header_len_dcch > 0 ) {
          LOG_D(MAC, "Calling mac_rlc_status_ind for DCCH\n");
          rlc_status = mac_rlc_status_ind(module_idP,
                                          rnti,
                                          module_idP,
                                          frameP,
                                          subframeP,
                                          ENB_FLAG_YES,
                                          MBMS_FLAG_NO,
                                          DCCH,
                                          0,
                                          0); // transport block set size
          sdu_lengths[0] = 0;

          if (rlc_status.bytes_in_buffer > 0) {  // There is DCCH to transmit
            LOG_D(MAC, "[eNB %d] Frame %d, DL-DCCH->DLSCH CC_id %d, Requesting %d bytes from RLC (RRC message)\n",
                  module_idP,
                  frameP,
                  CC_id,
                  TBS-header_len_dcch);
            sdu_lengths[0] = mac_rlc_data_req(module_idP,
                                              rnti,
                                              module_idP,
                                              frameP,
                                              ENB_FLAG_YES,
                                              MBMS_FLAG_NO,
                                              DCCH,
                                              TBS-ta_len-header_len_dcch,
                                              (char *)&dlsch_buffer[0],
                                              0,
                                              0);
            T(T_ENB_MAC_UE_DL_SDU,
              T_INT(module_idP),
              T_INT(CC_id),
              T_INT(rnti),
              T_INT(frameP),
              T_INT(subframeP),
              T_INT(harq_pid),
              T_INT(DCCH),
              T_INT(sdu_lengths[0]));
            LOG_D(MAC,"[eNB %d][DCCH] CC_id %d Got %d bytes from RLC\n",
                  module_idP,
                  CC_id,
                  sdu_lengths[0]);
            sdu_length_total = sdu_lengths[0];
            sdu_lcids[0] = DCCH;
            UE_info->eNB_UE_stats[CC_id][UE_id].num_pdu_tx[DCCH]+=1;
            UE_info->eNB_UE_stats[CC_id][UE_id].num_bytes_tx[DCCH]+=sdu_lengths[0];
            num_sdus = 1;
          } else {
            header_len_dcch = 0;
            sdu_length_total = 0;
          }
        }

        /* Check for DCCH1 and update header information (assume 2 byte sub-header) */
        if (TBS - ta_len-header_len_dcch - sdu_length_total > 0) {
          rlc_status = mac_rlc_status_ind(module_idP,
                                          rnti,
                                          module_idP,
                                          frameP,
                                          subframeP,
                                          ENB_FLAG_YES,
                                          MBMS_FLAG_NO,
                                          DCCH + 1,
                                          0,
                                          0); // transport block set size less allocations for timing advance and DCCH SDU
          sdu_lengths[num_sdus] = 0;

          if (rlc_status.bytes_in_buffer > 0) {
            LOG_D(MAC,"[eNB %d], Frame %d, DCCH1->DLSCH, CC_id %d, Requesting %d bytes from RLC (RRC message)\n",
                  module_idP,
                  frameP,
                  CC_id,
                  TBS-header_len_dcch - sdu_length_total);
            sdu_lengths[num_sdus] += mac_rlc_data_req(module_idP,
                                     rnti,
                                     module_idP,
                                     frameP,
                                     ENB_FLAG_YES,
                                     MBMS_FLAG_NO,
                                     DCCH+1,
                                     TBS-ta_len-header_len_dcch-sdu_length_total,
                                     (char *)&dlsch_buffer[sdu_length_total],
                                     0,
                                     0);
            T(T_ENB_MAC_UE_DL_SDU,
              T_INT(module_idP),
              T_INT(CC_id),
              T_INT(rnti),
              T_INT(frameP),
              T_INT(subframeP),
              T_INT(harq_pid),
              T_INT(DCCH+1),
              T_INT(sdu_lengths[num_sdus]));
            sdu_lcids[num_sdus] = DCCH1;
            sdu_length_total += sdu_lengths[num_sdus];
            header_len_dcch += 2;
            UE_info->eNB_UE_stats[CC_id][UE_id].num_pdu_tx[DCCH1] += 1;
            UE_info->eNB_UE_stats[CC_id][UE_id].num_bytes_tx[DCCH1] += sdu_lengths[num_sdus];
            num_sdus++;
          }
        }

        /* Assume the max dtch header size, and adjust it later */
        header_len_dtch = 0;
        header_len_dtch_last = 0; // the header length of the last mac sdu

        /* lcid has to be sorted before the actual allocation (similar struct as ue_list) */
        for (lcid = NB_RB_MAX-1; lcid >= DTCH ; lcid--) {
          /* TBD: check if the lcid is active */
          header_len_dtch += 3;
          header_len_dtch_last = 3;
          LOG_D(MAC,"[eNB %d], Frame %d, DTCH%d->DLSCH, Checking RLC status (tbs %d, len %d)\n",
                module_idP,
                frameP,
                lcid,
                TBS,
                TBS - ta_len-header_len_dcch - sdu_length_total - header_len_dtch);

          if (TBS - ta_len - header_len_dcch - sdu_length_total - header_len_dtch > 0) { // NN: > 2 ?
            rlc_status = mac_rlc_status_ind(module_idP,
                                            rnti,
                                            module_idP,
                                            frameP,
                                            subframeP,
                                            ENB_FLAG_YES,
                                            MBMS_FLAG_NO,
                                            lcid,
                                            0,
                                            0);

            if (rlc_status.bytes_in_buffer > 0) {
              /* RRC inactivity LTE-M */
              /* Reset RRC inactivity timer after uplane activity */
              ue_contextP = rrc_eNB_get_ue_context(RC.rrc[module_idP], rnti);

              if (ue_contextP != NULL) {
                ue_contextP->ue_context.ue_rrc_inactivity_timer = 1;
              } else {
                LOG_E(MAC, "[eNB %d] CC_id %d Couldn't find the context associated to UE (RNTI %d) and reset RRC inactivity timer\n",
                      module_idP,
                      CC_id,
                      rnti);
              }

              LOG_D(MAC,"[eNB %d][USER-PLANE DEFAULT DRB] Frame %d : DTCH->DLSCH, Requesting %d bytes from RLC (lcid %d total hdr len %d)\n",
                    module_idP,
                    frameP,
                    TBS - header_len_dcch - sdu_length_total - header_len_dtch,
                    lcid,
                    header_len_dtch);
              sdu_lengths[num_sdus] = mac_rlc_data_req(module_idP,
                                      rnti,
                                      module_idP,
                                      frameP,
                                      ENB_FLAG_YES,
                                      MBMS_FLAG_NO,
                                      lcid,
                                      TBS - ta_len - header_len_dcch - sdu_length_total - header_len_dtch,
                                      (char *) &dlsch_buffer[sdu_length_total],
                                      0,
                                      0);
              T(T_ENB_MAC_UE_DL_SDU,
                T_INT(module_idP),
                T_INT(CC_id),
                T_INT(rnti),
                T_INT(frameP),
                T_INT(subframeP),
                T_INT(harq_pid),
                T_INT(lcid),
                T_INT(sdu_lengths[num_sdus]));
              LOG_D(MAC,"[eNB %d][USER-PLANE DEFAULT DRB] Got %d bytes for DTCH %d \n",
                    module_idP,
                    sdu_lengths[num_sdus],
                    lcid);
              sdu_lcids[num_sdus] = lcid;
              sdu_length_total += sdu_lengths[num_sdus];

              if (sdu_lengths[num_sdus] < 128) {
                header_len_dtch--;
                header_len_dtch_last--;
              }

              num_sdus++;
            } else { // no data for this LCID
              header_len_dtch -= 3;
            }
          } else { // no TBS left
            header_len_dtch -= 3;
            break;
          }
        } // for loop LCID

        if (header_len_dtch == 0) {
          header_len_dtch_last = 0;
        }

        /* There is at least one SDU  */
        if ((sdu_length_total + header_len_dcch + header_len_dtch) > 0) {
          /* Now compute number of required RBs for total sdu length */
          /* Assume RAH format 2 */
          /* Adjust  header lengths */
          header_len_dcch_tmp = header_len_dcch;
          header_len_dtch_tmp = header_len_dtch;

          if (header_len_dtch == 0) {
            header_len_dcch = (header_len_dcch > 0) ? 1 : 0; // remove length field
          } else {
            header_len_dtch_last -= 1; // now use it to find how many bytes has to be removed for the last MAC SDU
            header_len_dtch = (header_len_dtch > 0) ? header_len_dtch - header_len_dtch_last : header_len_dtch; // remove length field for the last SDU
          }

          mcs = 9;

          /* Decrease mcs until TBS falls below required length */
          while ((TBS > (sdu_length_total + header_len_dcch + header_len_dtch + ta_len)) && (mcs>0)) {
            mcs--;
            TBS = get_TBS_DL(mcs,6);
          }

          /* If we have decreased too much or we don't have enough RBs, increase MCS */
          while (TBS < (sdu_length_total + header_len_dcch + header_len_dtch + ta_len)) {
            mcs++;
            TBS = get_TBS_DL(mcs,6);
          }

          LOG_D(MAC, "[eNB %d] CC_id %d Generated DLSCH header (mcs %d, TBS %d, nb_rb %d)\n",
                module_idP,
                CC_id,
                mcs,
                TBS,
                6);

          if ((TBS - header_len_dcch - header_len_dtch - sdu_length_total - ta_len) <= 2) {
            padding = (TBS - header_len_dcch - header_len_dtch - sdu_length_total - ta_len);
            post_padding = 0;
          } else {
            padding = 0;

            /* Adjust the header len */
            if (header_len_dtch == 0) {
              header_len_dcch = header_len_dcch_tmp;
            } else { // if ((header_len_dcch==0)&&((header_len_dtch==1)||(header_len_dtch==2)))
              header_len_dtch = header_len_dtch_tmp;
            }

            post_padding = TBS - sdu_length_total - header_len_dcch - header_len_dtch - ta_len; // 1 is for the postpadding header
          }

          offset = generate_dlsch_header((unsigned char *)UE_info->DLSCH_pdu[CC_id][0][UE_id].payload[0],
                                         num_sdus,              //num_sdus
                                         sdu_lengths,  //
                                         sdu_lcids,
                                         255,                                   // no drx
                                         ta_update, // timing advance
                                         NULL,                                  // contention res id
                                         padding,
                                         post_padding);

          if (ta_update != 31) {
            LOG_D(MAC,
                  "[eNB %d][DLSCH] Frame %d Generate header for UE_id %d on CC_id %d: sdu_length_total %d, num_sdus %d, sdu_lengths[0] %d, sdu_lcids[0] %d => payload offset %d,timing advance value : %d, padding %d,post_padding %d,(mcs %d, TBS %d, nb_rb %d),header_dcch %d, header_dtch %d\n",
                  module_idP,
                  frameP,
                  UE_id,
                  CC_id,
                  sdu_length_total,
                  num_sdus,
                  sdu_lengths[0],
                  sdu_lcids[0],
                  offset,
                  ta_update,
                  padding,
                  post_padding,
                  mcs,
                  TBS,
                  6,
                  header_len_dcch,
                  header_len_dtch);
          }

          /* Cycle through SDUs and place in dlsch_buffer */
          memcpy(&UE_info->DLSCH_pdu[CC_id][0][UE_id].payload[0][offset], dlsch_buffer, sdu_length_total);

          /* Fill remainder of DLSCH with random data */
          for (j = 0; j < (TBS - sdu_length_total - offset); j++) {
            UE_info->DLSCH_pdu[CC_id][0][UE_id].payload[0][offset + sdu_length_total + j] = (char)(taus()&0xff);
          }

          trace_pdu(DIRECTION_DOWNLINK,
                    (uint8_t *)UE_info->DLSCH_pdu[CC_id][0][UE_id].payload[0],
                    TBS,
                    module_idP,
                    3,
                    UE_RNTI(module_idP,UE_id),
                    mac->frame,
                    mac->subframe,
                    0,
                    0);
          T(T_ENB_MAC_UE_DL_PDU_WITH_DATA,
            T_INT(module_idP),
            T_INT(CC_id),
            T_INT(rnti),
            T_INT(frameP),
            T_INT(subframeP),
            T_INT(harq_pid),
            T_BUFFER(UE_info->DLSCH_pdu[CC_id][0][UE_id].payload[0], TBS));
          /* Do PUCCH power control */
          /* This is the snr */
          /* unit is not dBm, it's special from nfapi, convert to dBm */
          snr = ue_sched_ctl->pucch1_snr[CC_id];
          target_snr = mac->puCch10xSnr / 10;
          /* This assumes accumulated tpc */
          /* Make sure that we are only sending a tpc update once a frame, otherwise the control loop will freak out */
          int32_t framex10psubframe = UE_info->UE_template[CC_id][UE_id].pucch_tpc_tx_frame * 10 + UE_info->UE_template[CC_id][UE_id].pucch_tpc_tx_subframe;

          if (((framex10psubframe + 10) <= (frameP * 10 + subframeP)) || // normal case
              ((framex10psubframe > (frameP * 10 + subframeP)) &&
               (((10240 - framex10psubframe +frameP * 10 + subframeP) >= 10)))) { // frame wrap-around
            if (ue_sched_ctl->pucch1_cqi_update[CC_id] == 1) {
              ue_sched_ctl->pucch1_cqi_update[CC_id] = 0;
              UE_info->UE_template[CC_id][UE_id].pucch_tpc_tx_frame = frameP;
              UE_info->UE_template[CC_id][UE_id].pucch_tpc_tx_subframe = subframeP;

              if (snr > target_snr + PUCCH_PCHYST) {
                tpc = 0; //-1
              } else if (snr < target_snr - PUCCH_PCHYST) {
                tpc = 2; //+1
              } else {
                tpc = 1; //0
              }

              LOG_D(MAC,"[eNB %d] DLSCH scheduler: frame %d, subframe %d, harq_pid %d, tpc %d, snr/target snr %d/%d\n",
                    module_idP,
                    frameP,
                    subframeP,
                    harq_pid,
                    tpc,
                    snr,
                    target_snr);
            } else { // Po_PUCCH has been updated
              tpc = 1; // 0
            }
          } else { // time to do TPC update
            tpc = 1; //0
          }

          // Toggle NDI in first round
          UE_template->oldNDI[harq_pid] = 1 - UE_template->oldNDI[harq_pid];
          ue_sched_ctl->round[CC_id][harq_pid] = 0;
          round_DL = 0;
        } // if ((sdu_length_total + header_len_dcch + header_len_dtch) > 0)
      }

      if (round_DL < 8) {
        /* Fill in MDPDCCH */
        memset ((void *) dl_config_pdu, 0, sizeof (nfapi_dl_config_request_pdu_t));
        dl_config_pdu->pdu_type = NFAPI_DL_CONFIG_MPDCCH_PDU_TYPE;
        dl_config_pdu->pdu_size = (uint8_t) (2 + sizeof (nfapi_dl_config_mpdcch_pdu));
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.dci_format = (UE_template->rach_resource_type > 1) ? 11 : 10;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.mpdcch_narrow_band = epdcch_setconfig_r11->ext2->mpdcch_config_r13->choice.setup.mpdcch_Narrowband_r13-1;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.number_of_prb_pairs = 6;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.resource_block_assignment = 0; // Note: this can be dynamic
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.mpdcch_tansmission_type = epdcch_setconfig_r11->transmissionType_r11;
        AssertFatal(UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.choice.setup.startSymbol_r11 != NULL,
                    "UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.choice.setup.startSymbol_r11 is null\n");
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.start_symbol = *UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.choice.setup.startSymbol_r11;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.ecce_index = 0;        // Note: this should be dynamic
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.aggregation_level = 24;        // OK for CEModeA r1-3 (9.1.5-1b) or CEModeB r1-4
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.rnti_type = 4; // t-CRNTI
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.rnti = rnti;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.ce_mode = (UE_template->rach_resource_type < 3) ? 1 : 2;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.drms_scrambling_init = epdcch_setconfig_r11->dmrs_ScramblingSequenceInt_r11;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.initial_transmission_sf_io = (frameP * 10) + subframeP;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.transmission_power = 6000;     // 0dB
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.resource_block_coding = getRIV (6, 0, 6) | ((epdcch_setconfig_r11->ext2->mpdcch_config_r13->choice.setup.mpdcch_Narrowband_r13-1)<<5);
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.mcs = mcs;       // adjust according to size of RAR, 208 bits with N1A_PRB=3
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.pdsch_reptition_levels = 0;    // fix to 4 for now
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.redundancy_version = rvseq[round_DL&3];
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.new_data_indicator = UE_template->oldNDI[harq_pid];
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.harq_process = 0;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.tpmi_length = 0;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.tpmi = 0;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.pmi_flag = 0;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.pmi = 0;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.harq_resource_offset = 0;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.dci_subframe_repetition_number = 0;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.tpc = 3;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.downlink_assignment_index_length = 0;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.downlink_assignment_index = 0;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.allocate_prach_flag = 0;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.preamble_index = 0;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.prach_mask_index = 0;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.starting_ce_level = 0;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.srs_request = 0;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.antenna_ports_and_scrambling_identity_flag = 0;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.antenna_ports_and_scrambling_identity = 0;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.frequency_hopping_enabled_flag = 0;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.paging_direct_indication_differentiation_flag = 0;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.direct_indication = 0;
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.total_dci_length_including_padding = 0;        // this is not needed by OAI L1, but should be filled in
        dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.number_of_tx_antenna_ports = 1;
        dl_req->number_pdu++;
        UE_template->mcs[harq_pid] = dl_config_pdu->mpdcch_pdu.mpdcch_pdu_rel13.mcs;
      }
    } else if ((subframeP == 7) && (round_DL < 8)) { // DLSCH
      LOG_D(MAC, "DLSCH round_DL = %d in frame %d subframe %d\n", round_DL, frameP, subframeP);
      int             absSF = (frameP * 10) + subframeP;
      /* Have to check that MPDCCH was generated */
      LOG_D(MAC, "[eNB %d][RAPROC] CC_id %d Frame %d, subframeP %d: Generating DLSCH (ce_level %d RNTI %x)\n",
            module_idP,
            CC_id,
            frameP,
            subframeP,
            UE_template->rach_resource_type - 1,
            rnti);
      first_rb = narrowband_to_first_rb(&cc[CC_id], epdcch_setconfig_r11->ext2->mpdcch_config_r13->choice.setup.mpdcch_Narrowband_r13 - 1);
      dl_config_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu];
      memset ((void *) dl_config_pdu, 0, sizeof (nfapi_dl_config_request_pdu_t));
      dl_config_pdu->pdu_type = NFAPI_DL_CONFIG_DLSCH_PDU_TYPE;
      dl_config_pdu->pdu_size = (uint8_t) (2 + sizeof (nfapi_dl_config_dlsch_pdu));
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index = mac->pdu_index[CC_id];
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.rnti = rnti;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_allocation_type = 2;   // format 1A/1B/1D
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.virtual_resource_block_assignment_flag = 0;     // localized
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_block_coding = getRIV (to_prb (cc[CC_id].mib->message.dl_Bandwidth), first_rb, 6);  // check that this isn't getRIV(6,0,6)
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.modulation = 2; //QPSK
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.redundancy_version = 0;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_blocks = 1;   // first block
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_block_to_codeword_swap_flag = 0;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transmission_scheme = (cc[CC_id].p_eNB == 1) ? 0 : 1;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.number_of_layers = 1;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.number_of_subbands = 1;
      //      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.codebook_index                         = ;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.ue_category_capacity = 1;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pa = 4; // 0 dB
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.delta_power_offset_index = 0;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.ngap = 0;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.nprb = get_subbandsize (cc[CC_id].mib->message.dl_Bandwidth); // ignored
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transmission_mode = (cc[CC_id].p_eNB == 1) ? 1 : 2;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.num_bf_prb_per_subband = 1;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.num_bf_vector = 1;
      //      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.bf_vector                    = ;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel10.pdsch_start = *UE_template->physicalConfigDedicated->ext4->epdcch_Config_r11->config_r11.choice.setup.startSymbol_r11;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.ue_type = (UE_template->rach_resource_type < 3) ? 1 : 2;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.pdsch_payload_type = 2;        // not SI message
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.initial_transmission_sf_io = (10 * frameP) + subframeP;
      dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.drms_table_flag = 0;
      dl_req->number_pdu++;
      // DL request
      mac->TX_req[CC_id].sfn_sf = (frameP << 4) + subframeP;
      TX_req = &mac->TX_req[CC_id].tx_request_body.tx_pdu_list[mac->TX_req[CC_id].tx_request_body.number_of_pdus];
      TX_req->pdu_length = get_TBS_DL(UE_template->mcs[harq_pid], 6);
      TX_req->pdu_index = mac->pdu_index[CC_id]++;
      TX_req->num_segments = 1;
      TX_req->segments[0].segment_length = TX_req->pdu_length;
      TX_req->segments[0].segment_data = mac->UE_info.DLSCH_pdu[CC_id][0][(unsigned char) UE_id].payload[0];
      mac->TX_req[CC_id].tx_request_body.number_of_pdus++;
      ackNAK_absSF = absSF + 4;
      ul_req = &mac->UL_req_tmp[CC_id][ackNAK_absSF % 10].ul_config_request_body;
      ul_config_pdu = &ul_req->ul_config_pdu_list[ul_req->number_of_pdus];
      ul_config_pdu->pdu_type = NFAPI_UL_CONFIG_UCI_HARQ_PDU_TYPE;
      ul_config_pdu->pdu_size = (uint8_t) (2 + sizeof (nfapi_ul_config_uci_harq_pdu));
      ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel8.handle = 0;      // don't know how to use this
      ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel8.rnti = rnti;
      ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel13.ue_type = (UE_template->rach_resource_type < 3) ? 1 : 2;
      ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel13.empty_symbols = 0;
      ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel13.total_number_of_repetitions = pucchreps[UE_template->rach_resource_type - 1];
      ul_config_pdu->uci_harq_pdu.ue_information.ue_information_rel13.repetition_number = 0;

      if (cc[CC_id].tdd_Config == NULL) {    // FDD case
        ul_config_pdu->uci_harq_pdu.harq_information.harq_information_rel9_fdd.n_pucch_1_0 = n1pucchan[UE_template->rach_resource_type - 1];
        // NOTE: How to fill in the rest of the n_pucch_1_0 information 213 Section 10.1.2.1 in the general case
        // = N_ECCE_q + Delta_ARO + n1pucchan[ce_level]
        // higher in the MPDCCH configuration, N_ECCE_q is hard-coded to 0, and harq resource offset to 0 =>
        // Delta_ARO = 0 from Table 10.1.2.1-1
        ul_config_pdu->uci_harq_pdu.harq_information.harq_information_rel9_fdd.harq_size = 1; // 1-bit ACK/NAK
        ul_config_pdu->uci_harq_pdu.harq_information.harq_information_rel9_fdd.number_of_pucch_resources = 1;
      } else {
        AssertFatal (1 == 0, "PUCCH configuration for ACK/NAK not handled yet for TDD BL/CE case\n");
      }

      ul_req->number_of_pdus++;
      T(T_ENB_MAC_UE_DL_PDU_WITH_DATA,
        T_INT (module_idP),
        T_INT (CC_id),
        T_INT (rnti),
        T_INT (frameP),
        T_INT (subframeP),
        T_INT (0 /* harq_pid always 0? */ ),
        T_BUFFER (&mac->UE_info.DLSCH_pdu[CC_id][0][UE_id].payload[0], TX_req->pdu_length));
      trace_pdu(1,
                (uint8_t *) mac->UE_info.DLSCH_pdu[CC_id][0][(unsigned char) UE_id].payload[0],
                TX_req->pdu_length,
                UE_id,
                3,
                rnti,
                frameP,
                subframeP,
                0,
                0);
    } // end else if ((subframeP == 7) && (round_DL < 8))
  } // end loop on UE_id
}

//------------------------------------------------------------------------------
unsigned char *get_dlsch_sdu(module_id_t module_idP,
                             int CC_id,
                             frame_t frameP,
                             rnti_t rntiP,
                             uint8_t TBindex)
//------------------------------------------------------------------------------
{
  int UE_id;
  eNB_MAC_INST *eNB = RC.mac[module_idP];

  if (rntiP == SI_RNTI) {
    LOG_D(MAC, "[eNB %d] CC_id %d Frame %d Get DLSCH sdu for BCCH \n",
          module_idP,
          CC_id,
          frameP);
    return ((unsigned char *) &eNB->common_channels[CC_id].BCCH_pdu.payload[0]);
  }

  if (rntiP == P_RNTI) {
    LOG_D(MAC, "[eNB %d] CC_id %d Frame %d Get PCH sdu for PCCH \n",
          module_idP,
          CC_id,
          frameP);
    return ((unsigned char *) &eNB->common_channels[CC_id].PCCH_pdu.payload[0]);
  }

  UE_id = find_UE_id(module_idP, rntiP);

  if (UE_id != -1) {
    LOG_D(MAC, "[eNB %d] Frame %d:  CC_id %d Get DLSCH sdu for rnti %x => UE_id %d\n",
          module_idP,
          frameP,
          CC_id,
          rntiP,
          UE_id);
    return ((unsigned char *) &eNB->UE_info.DLSCH_pdu[CC_id][TBindex][UE_id].payload[0]);
  }

  LOG_E(MAC, "[eNB %d] Frame %d: CC_id %d UE with RNTI %x does not exist\n",
        module_idP,
        frameP,
        CC_id,
        rntiP);
  return NULL;
}


//------------------------------------------------------------------------------
void
update_ul_dci(module_id_t module_idP,
              uint8_t CC_idP,
              rnti_t rntiP,
              uint8_t daiP,
              sub_frame_t subframe)
//------------------------------------------------------------------------------
{
  eNB_MAC_INST *eNB = RC.mac[module_idP];
  COMMON_channels_t *cc = &eNB->common_channels[CC_idP];

  if (cc->tdd_Config != NULL) { // TDD
    nfapi_hi_dci0_request_t *HI_DCI0_req = &eNB->HI_DCI0_req[CC_idP][subframe];
    nfapi_hi_dci0_request_pdu_t *hi_dci0_pdu = &HI_DCI0_req->hi_dci0_request_body.hi_dci0_pdu_list[0];
    int limit = HI_DCI0_req->hi_dci0_request_body.number_of_dci + HI_DCI0_req->hi_dci0_request_body.number_of_hi;

    for (int i = 0; i < limit; i++, hi_dci0_pdu++) {
      if (hi_dci0_pdu->pdu_type == NFAPI_HI_DCI0_DCI_PDU_TYPE && hi_dci0_pdu->dci_pdu.dci_pdu_rel8.rnti == rntiP)
        hi_dci0_pdu->dci_pdu.dci_pdu_rel8.dl_assignment_index = (daiP - 1) & 3;
    }
  }

  return;
}


//------------------------------------------------------------------------------
void
set_ue_dai(sub_frame_t subframeP,
           int UE_id,
           uint8_t CC_id,
           uint8_t tdd_config,
           UE_info_t *UE_info)
//------------------------------------------------------------------------------
{
  switch (tdd_config) {
    case 0:
      if (subframeP == 0 || subframeP == 1 || subframeP == 3 || subframeP == 5 || subframeP == 6 || subframeP == 8) {
        UE_info->UE_template[CC_id][UE_id].DAI = 0;
      }

      break;

    case 1:
      if (subframeP == 0 || subframeP == 4 || subframeP == 5 || subframeP == 9) {
        UE_info->UE_template[CC_id][UE_id].DAI = 0;
      }

      break;

    case 2:
      if (subframeP == 4 || subframeP == 5) {
        UE_info->UE_template[CC_id][UE_id].DAI = 0;
      }

      break;

    case 3:
      if (subframeP == 5 || subframeP == 7 || subframeP == 9) {
        UE_info->UE_template[CC_id][UE_id].DAI = 0;
      }

      break;

    case 4:
      if (subframeP == 0 || subframeP == 6) {
        UE_info->UE_template[CC_id][UE_id].DAI = 0;
      }

      break;

    case 5:
      if (subframeP == 9) {
        UE_info->UE_template[CC_id][UE_id].DAI = 0;
      }

      break;

    case 6:
      if (subframeP == 0 || subframeP == 1 || subframeP == 5 || subframeP == 6 || subframeP == 9) {
        UE_info->UE_template[CC_id][UE_id].DAI = 0;
      }

      break;

    default:
      UE_info->UE_template[CC_id][UE_id].DAI = 0;
      LOG_I(MAC, "unknown TDD config %d\n",
            tdd_config);
      break;
  }

  return;
}

void
schedule_PCH(module_id_t module_idP,
             frame_t frameP,
             sub_frame_t subframeP) {
  /* DCI:format 1A/1C P-RNTI:0xFFFE */
  /* PDU:eNB_rrc_inst[Mod_idP].common_channels[CC_id].PCCH_pdu.payload */
  uint16_t pcch_sdu_length;
  int mcs = -1;
  int CC_id;
  eNB_MAC_INST *eNB = RC.mac[module_idP];
  COMMON_channels_t *cc;
  uint8_t *vrb_map;
  int n_rb_dl;
  int first_rb = -1;
  nfapi_dl_config_request_pdu_t *dl_config_pdu;
  nfapi_tx_request_pdu_t *TX_req;
  nfapi_dl_config_request_body_t *dl_req;
  UE_PF_PO_t *ue_pf_po;
#ifdef FORMAT1C
  int gap_index = 0;      /* indicate which gap(1st or 2nd) is used (0:1st) */
  const int GAP_MAP [9][2] = {
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
  uint8_t n_rb_step = 0;
  uint8_t n_gap = 0;
  uint8_t n_vrb_dl = 0;
  uint8_t Lcrbs = 0;
  uint16_t rb_bit = 168;    /* RB bit number value is unsure */
#endif
  start_meas(&eNB->schedule_pch);

  for (CC_id = 0; CC_id < RC.nb_mac_CC[module_idP]; CC_id++) {
    cc              = &eNB->common_channels[CC_id];
    vrb_map         = (void *) &cc->vrb_map;
    n_rb_dl         = to_prb(cc->mib->message.dl_Bandwidth);
    dl_req          = &eNB->DL_req[CC_id].dl_config_request_body;

    for (uint16_t i = 0; i < MAX_MOBILES_PER_ENB; i++) {
      ue_pf_po = &UE_PF_PO[CC_id][i];

      if (ue_pf_po->enable_flag != true) {
        continue;
      }

      if (frameP % ue_pf_po->T == ue_pf_po->PF_min && subframeP == ue_pf_po->PO) {
        pcch_sdu_length = mac_rrc_data_req(module_idP,
                                           CC_id,
                                           frameP,
                                           PCCH,
                                           0xFFFE,
                                           1,
                                           &cc->PCCH_pdu.payload[0],
                                           i); // used for ue index

        if (pcch_sdu_length == 0) {
          LOG_D(MAC, "[eNB %d] Frame %d subframe %d: PCCH not active(size = 0 byte)\n",
                module_idP,
                frameP,
                subframeP);
          continue;
        }

        LOG_D(MAC, "[eNB %d] Frame %d subframe %d: PCCH->PCH CC_id %d UE_id %d, Received %d bytes \n",
              module_idP,
              frameP,
              subframeP,
              CC_id,
              i,
              pcch_sdu_length);
#ifdef FORMAT1C

        //NO SIB
        if ((subframeP == 0 || subframeP == 1 || subframeP == 2 || subframeP == 4 || subframeP == 6 || subframeP == 9) ||
            (subframeP == 5 && ((frameP % 2) != 0 && (frameP % 8) != 1))) {
          switch (n_rb_dl) {
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
                module_idP,
                frameP,
                CC_id,
                pcch_sdu_length);
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

        for(i = 0; i < Lcrbs ; i++) {
          vrb_map[first_rb+i] = 1;
        }

#else

        //NO SIB
        if ((subframeP == 0 || subframeP == 1 || subframeP == 2 || subframeP == 4 || subframeP == 6 || subframeP == 9) ||
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
        vrb_map[first_rb + 1] = 1;
        vrb_map[first_rb + 2] = 1;
        vrb_map[first_rb + 3] = 1;

        /* Get MCS for length of PCH */
        if (pcch_sdu_length <= get_TBS_DL(0, 3)) {
          mcs = 0;
        } else if (pcch_sdu_length <= get_TBS_DL(1, 3)) {
          mcs = 1;
        } else if (pcch_sdu_length <= get_TBS_DL(2, 3)) {
          mcs = 2;
        } else if (pcch_sdu_length <= get_TBS_DL(3, 3)) {
          mcs = 3;
        } else if (pcch_sdu_length <= get_TBS_DL(4, 3)) {
          mcs = 4;
        } else if (pcch_sdu_length <= get_TBS_DL(5, 3)) {
          mcs = 5;
        } else if (pcch_sdu_length <= get_TBS_DL(6, 3)) {
          mcs = 6;
        } else if (pcch_sdu_length <= get_TBS_DL(7, 3)) {
          mcs = 7;
        } else if (pcch_sdu_length <= get_TBS_DL(8, 3)) {
          mcs = 8;
        } else if (pcch_sdu_length <= get_TBS_DL(9, 3)) {
          mcs = 9;
        }

#endif
        dl_config_pdu = &dl_req->dl_config_pdu_list[dl_req->number_pdu];
        memset((void *) dl_config_pdu,
               0,
               sizeof(nfapi_dl_config_request_pdu_t));
        dl_config_pdu->pdu_type = NFAPI_DL_CONFIG_DCI_DL_PDU_TYPE;
        dl_config_pdu->pdu_size = (uint8_t) (2 + sizeof(nfapi_dl_config_dci_dl_pdu));
#ifdef FORMAT1C
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format                  = NFAPI_DL_DCI_FORMAT_1C;
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding       = getRIV(n_vrb_dl/n_rb_step, first_rb/n_rb_step, Lcrbs/n_rb_step);
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.ngap                        = n_gap;
#else
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.dci_format                  = NFAPI_DL_DCI_FORMAT_1A;
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.harq_process                = 0;
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.tpc                         = 1; // no TPC
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.new_data_indicator_1        = 0;
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.redundancy_version_1        = 0;
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.resource_block_coding = getRIV(n_rb_dl, first_rb, 4);
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.virtual_resource_block_assignment_flag = 0;
#endif
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level           = 4;
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti                        = 0xFFFE; // P-RNTI
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.rnti_type                   = 2;    // P-RNTI : see Table 4-10 from SCF082 - nFAPI specifications
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.transmission_power          = 6000; // equal to RS power
        dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.mcs_1                       = mcs;

        if (!CCE_allocation_infeasible(module_idP,
                                       CC_id,
                                       0,
                                       subframeP,
                                       dl_config_pdu->dci_dl_pdu.dci_dl_pdu_rel8.aggregation_level,
                                       P_RNTI)) {
          LOG_D(MAC,"Frame %d: Subframe %d : Adding common DCI for P_RNTI\n",
                frameP,
                subframeP);
          dl_req->number_dci++;
          dl_req->number_pdu++;
          dl_req->tl.tag = NFAPI_DL_CONFIG_REQUEST_BODY_TAG;
          eNB->DL_req[CC_id].sfn_sf = frameP<<4 | subframeP;
          eNB->DL_req[CC_id].header.message_id = NFAPI_DL_CONFIG_REQUEST;
          dl_config_pdu                                                                  = &dl_req->dl_config_pdu_list[dl_req->number_pdu];
          memset((void *)dl_config_pdu,
                 0,
                 sizeof(nfapi_dl_config_request_pdu_t));
          dl_config_pdu->pdu_type                                                        = NFAPI_DL_CONFIG_DLSCH_PDU_TYPE;
          dl_config_pdu->pdu_size                                                        = (uint8_t)(2 + sizeof(nfapi_dl_config_dlsch_pdu));
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pdu_index                              = eNB->pdu_index[CC_id];
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.rnti                                   = 0xFFFE;
#ifdef FORMAT1C
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_allocation_type               = 3;   // format 1C
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_block_coding                  = getRIV(n_vrb_dl / n_rb_step,
              first_rb / n_rb_step,
              Lcrbs / n_rb_step);
#else
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_allocation_type               = 2;   // format 1A/1B/1D
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.resource_block_coding                  = getRIV(n_rb_dl,
              first_rb,
              4);
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.virtual_resource_block_assignment_flag = 0;   // localized
#endif
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.modulation                             = 2; //QPSK
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.redundancy_version                     = 1;
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_blocks                       = 1;// first block
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transport_block_to_codeword_swap_flag  = 0;
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transmission_scheme                    = (cc->p_eNB==1 ) ? 0 : 1;
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.number_of_layers                       = 1;
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.number_of_subbands                     = 1;
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.ue_category_capacity                   = 1;
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.pa                                     = 4; // 0 dB
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.delta_power_offset_index               = 0;
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.ngap                                   = 0;
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.nprb                                   = get_subbandsize(cc->mib->message.dl_Bandwidth); // ignored
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.transmission_mode                      = (cc->p_eNB==1 ) ? 1 : 2;
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.num_bf_prb_per_subband                 = 1;
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel8.num_bf_vector                          = 1;
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel10.pdsch_start                           = 3;
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.ue_type                               = 0; // regular UE
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.pdsch_payload_type                    = 2; // not BR
          dl_config_pdu->dlsch_pdu.dlsch_pdu_rel13.initial_transmission_sf_io            = 0xFFFF;
          dl_req->number_pdu++;
          eNB->TX_req[CC_id].sfn_sf                                                      = (frameP<<4)+subframeP;
          TX_req = &eNB->TX_req[CC_id].tx_request_body.tx_pdu_list[eNB->TX_req[CC_id].tx_request_body.number_of_pdus];
          TX_req->pdu_length                                                             = pcch_sdu_length;
          TX_req->pdu_index                                                              = eNB->pdu_index[CC_id]++;
          TX_req->num_segments                                                           = 1;
          TX_req->segments[0].segment_length                                             = pcch_sdu_length;
          TX_req->segments[0].segment_data                                               = cc[CC_id].PCCH_pdu.payload;
          eNB->TX_req[CC_id].tx_request_body.tl.tag                                      = NFAPI_TX_REQUEST_BODY_TAG;
          eNB->TX_req[CC_id].tx_request_body.number_of_pdus++;
        } else {
          LOG_E(MAC,"[eNB %d] CCid %d Frame %d, subframe %d : Cannot add DCI 1A/1C for Paging\n",
                module_idP,
                CC_id,
                frameP,
                subframeP);
          continue;
        }

        trace_pdu(DIRECTION_DOWNLINK,
                  &eNB->common_channels[CC_id].PCCH_pdu.payload[0],
                  pcch_sdu_length,
                  0xffff,
                  PCCH,
                  P_RNTI,
                  eNB->frame,
                  eNB->subframe,
                  0,
                  0);
        eNB->eNB_stats[CC_id].total_num_pcch_pdu++;
        eNB->eNB_stats[CC_id].pcch_buffer = pcch_sdu_length;
        eNB->eNB_stats[CC_id].total_pcch_buffer += pcch_sdu_length;
        eNB->eNB_stats[CC_id].pcch_mcs = mcs;
        //paging first_rb log
        LOG_D(MAC,"[eNB %d] Frame %d subframe %d PCH: paging_ue_index %d pcch_sdu_length %d mcs %d first_rb %d\n",
              module_idP,
              frameP,
              subframeP,
              ue_pf_po->ue_index_value,
              pcch_sdu_length,
              mcs,
              first_rb);
        pthread_mutex_lock(&ue_pf_po_mutex);
        memset(ue_pf_po,
               0,
               sizeof(UE_PF_PO_t));
        pthread_mutex_unlock(&ue_pf_po_mutex);
      }
    }
  }

  /* this might be misleading when pcch is inactive */
  stop_meas(&eNB->schedule_pch);
  return;
}
