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

#include "ran_func_rc.h"
#include "ran_func_rc_subs.h"
#include "ran_func_rc_extern.h"
#include "../../flexric/test/rnd/fill_rnd_data_rc.h"  // this dependancy will be taken out once RAN Function Definition is implemented
#include "../../flexric/src/sm/rc_sm/ie/ir/lst_ran_param.h"
#include "../../flexric/src/sm/rc_sm/ie/ir/ran_param_list.h"
#include "../../flexric/src/agent/e2_agent_api.h"

#include <stdio.h>
#include <unistd.h>
#include "common/ran_context.h"

static pthread_once_t once_rc_mutex = PTHREAD_ONCE_INIT;
static rc_subs_data_t rc_subs_data = {0};
static pthread_mutex_t rc_mutex = PTHREAD_MUTEX_INITIALIZER;

static void init_once_rc(void)
{
  init_rc_subs_data(&rc_subs_data);
}

void read_rc_setup_sm(void* data)
{
  assert(data != NULL);
//  assert(data->type == RAN_CTRL_V1_3_AGENT_IF_E2_SETUP_ANS_V0);
  rc_e2_setup_t* rc = (rc_e2_setup_t*)data;
  rc->ran_func_def = fill_rc_ran_func_def();

  // E2 Setup Request is sent periodically until the connection is established
  // RC subscritpion data should be initialized only once
  const int ret = pthread_once(&once_rc_mutex, init_once_rc);
  DevAssert(ret == 0);
}


RB_PROTOTYPE(ric_id_2_param_id_trees, ric_req_id_s, entries, cmp_ric_req_id);

static ue_id_e2sm_t fill_ue_id_data(const gNB_RRC_UE_t *rrc_ue_context)
{
  ue_id_e2sm_t ue_id = {0};

  // Check the E2 node type
  #if defined (NGRAN_GNB_CUUP) && defined (NGRAN_GNB_CUCP)
  ue_id.type = GNB_UE_ID_E2SM;
  if (RC.nrrrc[0]->node_type == ngran_gNB_CU) {
    // gNB-CU UE F1AP ID List
    // C-ifCUDUseparated
    ue_id.gnb.gnb_cu_ue_f1ap_lst_len = 1;
    ue_id.gnb.gnb_cu_ue_f1ap_lst = calloc(ue_id.gnb.gnb_cu_ue_f1ap_lst_len, sizeof(uint32_t));
    assert(ue_id.gnb.gnb_cu_ue_f1ap_lst != NULL && "Memory exhausted");

    for(size_t i = 0; i < ue_id.gnb.gnb_cu_ue_f1ap_lst_len; i++) {
      ue_id.gnb.gnb_cu_ue_f1ap_lst[i] = rrc_ue_context->rrc_ue_id;
    }
  } else if (RC.nrrrc[0]->node_type == ngran_gNB_CUCP) {
    //gNB-CU-CP UE E1AP ID List
    //C-ifCPUPseparated 
    ue_id.gnb.gnb_cu_cp_ue_e1ap_lst_len = 1;
    ue_id.gnb.gnb_cu_cp_ue_e1ap_lst = calloc(ue_id.gnb.gnb_cu_cp_ue_e1ap_lst_len, sizeof(uint32_t));
    assert(ue_id.gnb.gnb_cu_cp_ue_e1ap_lst != NULL && "Memory exhausted");

    for(size_t i = 0; i < ue_id.gnb.gnb_cu_cp_ue_e1ap_lst_len; i++) {
      ue_id.gnb.gnb_cu_cp_ue_e1ap_lst[i] = rrc_ue_context->rrc_ue_id;
    }
  } else if (RC.nrrrc[0]->node_type == ngran_gNB_DU) {
    assert(false && "RRC state change cannot be triggered from DU\n");
  }

  #elif defined (NGRAN_GNB_CUUP)
  assert(false && "RRC state change cannot be triggered from CU-UP\n");

  #endif

  ue_id.gnb.amf_ue_ngap_id = rrc_ue_context->rrc_ue_id;
  ue_id.gnb.guami.amf_ptr = rrc_ue_context->ue_guami.amf_pointer;
  ue_id.gnb.guami.amf_region_id = rrc_ue_context->ue_guami.amf_region_id;
  ue_id.gnb.guami.amf_set_id = rrc_ue_context->ue_guami.amf_set_id;
  ue_id.gnb.guami.plmn_id.mcc = rrc_ue_context->ue_guami.mcc;
  ue_id.gnb.guami.plmn_id.mnc = rrc_ue_context->ue_guami.mnc;
  ue_id.gnb.guami.plmn_id.mnc_digit_len = rrc_ue_context->ue_guami.mnc_len;

  return ue_id;
}

static seq_ran_param_t fill_rrc_state_change_seq_ran(const rc_sm_rrc_state_e rrc_state)
{
  seq_ran_param_t seq_ran_param = {0};

  seq_ran_param.ran_param_id = RRC_STATE_CHANGED_TO_E2SM_RC_RAN_PARAM_ID;
  seq_ran_param.ran_param_val.type = ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE;
  seq_ran_param.ran_param_val.flag_false = calloc(1, sizeof(ran_parameter_value_t));
  assert(seq_ran_param.ran_param_val.flag_false != NULL && "Memory exhausted");
  seq_ran_param.ran_param_val.flag_false->type = INTEGER_RAN_PARAMETER_VALUE;
  seq_ran_param.ran_param_val.flag_false->int_ran = rrc_state;  

  return seq_ran_param;
}

static rc_ind_data_t* fill_ue_rrc_state_change(const gNB_RRC_UE_t *rrc_ue_context, const rc_sm_rrc_state_e rrc_state)
{
  rc_ind_data_t* rc_ind = calloc(1, sizeof(rc_ind_data_t));
  assert(rc_ind != NULL && "Memory exhausted");

  // Generate Indication Header
  rc_ind->hdr.format = FORMAT_1_E2SM_RC_IND_HDR;
  rc_ind->hdr.frmt_1.ev_trigger_id = NULL;

  // Generate Indication Message
  rc_ind->msg.format = FORMAT_2_E2SM_RC_IND_MSG;

  //Sequence of UE Identifier
  //[1-65535]
  rc_ind->msg.frmt_2.sz_seq_ue_id = 1;
  rc_ind->msg.frmt_2.seq_ue_id = calloc(rc_ind->msg.frmt_2.sz_seq_ue_id, sizeof(seq_ue_id_t));
  assert(rc_ind->msg.frmt_2.seq_ue_id != NULL && "Memory exhausted");

  // UE ID
  // Mandatory
  // 9.3.10
  rc_ind->msg.frmt_2.seq_ue_id[0].ue_id = fill_ue_id_data(rrc_ue_context);

  // Sequence of
  // RAN Parameter
  // [1- 65535]
  rc_ind->msg.frmt_2.seq_ue_id[0].sz_seq_ran_param = 1;
  rc_ind->msg.frmt_2.seq_ue_id[0].seq_ran_param = calloc(rc_ind->msg.frmt_2.seq_ue_id[0].sz_seq_ran_param, sizeof(seq_ran_param_t));
  assert(rc_ind->msg.frmt_2.seq_ue_id[0].seq_ran_param != NULL && "Memory exhausted");
  rc_ind->msg.frmt_2.seq_ue_id[0].seq_ran_param[0] = fill_rrc_state_change_seq_ran(rrc_state);

  return rc_ind;
}

void signal_rrc_state_changed_to(const gNB_RRC_UE_t *rrc_ue_context, const rc_sm_rrc_state_e rrc_state)
{ 
  pthread_mutex_lock(&rc_mutex);
  if (rc_subs_data.rb[RRC_STATE_CHANGED_TO_E2SM_RC_RAN_PARAM_ID].rbh_root == NULL) {
    pthread_mutex_unlock(&rc_mutex);
    return;
  }
  
  struct ric_req_id_s *node;
  RB_FOREACH(node, ric_id_2_param_id_trees, &rc_subs_data.rb[RRC_STATE_CHANGED_TO_E2SM_RC_RAN_PARAM_ID]) {
    rc_ind_data_t* rc_ind_data = fill_ue_rrc_state_change(rrc_ue_context, rrc_state);

    // Needs review: memory ownership of the type rc_ind_data_t is transferred to the E2 Agent. Bad
    async_event_agent_api(node->ric_req_id, rc_ind_data);
    printf( "Event for RIC Req ID %u generated\n", node->ric_req_id);
  }
  
  pthread_mutex_unlock(&rc_mutex);
}

static void free_aperiodic_subscription(uint32_t ric_req_id)
{
  remove_rc_subs_data(&rc_subs_data, ric_req_id);
}

sm_ag_if_ans_t write_subs_rc_sm(void const* src)
{
  assert(src != NULL); // && src->type == RAN_CTRL_SUBS_V1_03);

  wr_rc_sub_data_t* wr_rc = (wr_rc_sub_data_t*)src;

  assert(wr_rc->rc.ad != NULL && "Cannot be NULL");

  // 9.2.1.2  RIC ACTION DEFINITION IE
  switch (wr_rc->rc.ad->format) {
    case FORMAT_1_E2SM_RC_ACT_DEF: {
      // Parameters to be Reported List
      // [1-65535]
      const uint32_t ric_req_id = wr_rc->ric_req_id;
      arr_ran_param_id_t* arr_ran_param_id = calloc(1, sizeof(arr_ran_param_id_t));
      assert(arr_ran_param_id != NULL && "Memory exhausted");
      arr_ran_param_id->len = wr_rc->rc.ad->frmt_1.sz_param_report_def;
      arr_ran_param_id->ran_param_id = calloc(arr_ran_param_id->len, sizeof(ran_param_id_e));

      const size_t sz = arr_ran_param_id->len;
      for(size_t i = 0; i < sz; i++) {    
        arr_ran_param_id->ran_param_id[i] = wr_rc->rc.ad->frmt_1.param_report_def[i].ran_param_id;
      }
      insert_rc_subs_data(&rc_subs_data, ric_req_id, arr_ran_param_id);
      break;
    }
  
    default:
      AssertFatal(wr_rc->rc.ad->format == FORMAT_1_E2SM_RC_ACT_DEF, "Action Definition Format %d not yet implemented", wr_rc->rc.ad->format);
  }

  sm_ag_if_ans_t ans = {.type = SUBS_OUTCOME_SM_AG_IF_ANS_V0};
  ans.subs_out.type = APERIODIC_SUBSCRIPTION_FLRC;
  ans.subs_out.aper.free_aper_subs = free_aperiodic_subscription;

  return ans;
}


sm_ag_if_ans_t write_ctrl_rc_sm(void const* data)
{
  assert(data != NULL);
//  assert(data->type == RAN_CONTROL_CTRL_V1_03 );

  rc_ctrl_req_data_t const* ctrl = (rc_ctrl_req_data_t const*)data;
  if(ctrl->hdr.format == FORMAT_1_E2SM_RC_CTRL_HDR){
    if(ctrl->hdr.frmt_1.ric_style_type == 1 && ctrl->hdr.frmt_1.ctrl_act_id == 2){
      printf("QoS flow mapping configuration \n");
      e2sm_rc_ctrl_msg_frmt_1_t const* frmt_1 = &ctrl->msg.frmt_1;
      for(size_t i = 0; i < frmt_1->sz_ran_param; ++i){
        seq_ran_param_t const* rp = frmt_1->ran_param;
        if(rp[i].ran_param_id == 1){
          assert(rp[i].ran_param_val.type == ELEMENT_KEY_FLAG_TRUE_RAN_PARAMETER_VAL_TYPE );
          printf("DRB ID %ld \n", rp[i].ran_param_val.flag_true->int_ran);
        } else if(rp[i].ran_param_id == 2){
          assert(rp[i].ran_param_val.type == LIST_RAN_PARAMETER_VAL_TYPE);
          printf("List of QoS Flows to be modified \n");
          for(size_t j = 0; j < ctrl->msg.frmt_1.ran_param[i].ran_param_val.lst->sz_lst_ran_param; ++j){ 
            lst_ran_param_t const* lrp = rp[i].ran_param_val.lst->lst_ran_param;
            // The following assertion should be true, but there is a bug in the std
            // check src/sm/rc_sm/enc/rc_enc_asn.c:1085 and src/sm/rc_sm/enc/rc_enc_asn.c:984 
            // assert(lrp[j].ran_param_id == 3); 
            assert(lrp[j].ran_param_struct.ran_param_struct[0].ran_param_id == 4) ;
            assert(lrp[j].ran_param_struct.ran_param_struct[0].ran_param_val.type == ELEMENT_KEY_FLAG_TRUE_RAN_PARAMETER_VAL_TYPE);

            int64_t qfi = lrp[j].ran_param_struct.ran_param_struct[0].ran_param_val.flag_true->int_ran;
            assert(qfi > -1 && qfi < 65);

            assert(lrp[j].ran_param_struct.ran_param_struct[1].ran_param_id == 5);
            assert(lrp[j].ran_param_struct.ran_param_struct[1].ran_param_val.type == ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE);
            int64_t dir = lrp[j].ran_param_struct.ran_param_struct[1].ran_param_val.flag_false->int_ran;
            assert(dir == 0 || dir == 1);
            printf("qfi = %ld dir %ld \n", qfi, dir);
          }
        } 
      }
    }
  }

  sm_ag_if_ans_t ans = {.type = CTRL_OUTCOME_SM_AG_IF_ANS_V0};
  ans.ctrl_out.type = RAN_CTRL_V1_3_AGENT_IF_CTRL_ANS_V0;
  return ans;
}


bool read_rc_sm(void* data)
{
  assert(data != NULL);
//  assert(data->type == RAN_CTRL_STATS_V1_03);
  assert(0!=0 && "Not implemented");

  return true;
}
