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
#include "ran_e2sm_ue_id.h"
#include "../../flexric/src/sm/rc_sm/ie/ir/lst_ran_param.h"
#include "../../flexric/src/sm/rc_sm/ie/ir/ran_param_list.h"
#include "../../flexric/src/agent/e2_agent_api.h"

#include <stdio.h>
#include <unistd.h>
#include "common/ran_context.h"

static pthread_once_t once_rc_mutex = PTHREAD_ONCE_INIT;
static rc_subs_data_t rc_subs_data = {0};
static pthread_mutex_t rc_mutex = PTHREAD_MUTEX_INITIALIZER;

static ngran_node_t get_e2_node_type(void)
{
  ngran_node_t node_type = 0;

#if defined(NGRAN_GNB_DU) && defined(NGRAN_GNB_CUUP) && defined(NGRAN_GNB_CUCP)
  node_type = RC.nrrrc[0]->node_type;
#elif defined (NGRAN_GNB_CUUP)
  node_type =  ngran_gNB_CUUP;
#endif

  return node_type;
}

static void init_once_rc(void)
{
  init_rc_subs_data(&rc_subs_data);
}

static void fill_rc_ev_trig(ran_func_def_ev_trig_t* ev_trig)
{
  // Sequence of EVENT TRIGGER styles
  // [1 - 63]
  ev_trig->sz_seq_ev_trg_style = 1;
  ev_trig->seq_ev_trg_style = calloc(ev_trig->sz_seq_ev_trg_style, sizeof(seq_ev_trg_style_t));
  assert(ev_trig->seq_ev_trg_style != NULL && "Memory exhausted");

  seq_ev_trg_style_t* ev_trig_style = &ev_trig->seq_ev_trg_style[0];

  // RIC Event Trigger Style Type
  // Mandatory
  // 9.3.3
  // 6.2.2.2.
  //  INTEGER
  ev_trig_style->style = 4;

  // RIC Event Trigger Style Name
  // Mandatory
  // 9.3.4
  // 6.2.2.3
  //PrintableString(SIZE(1..150,...))
  const char ev_style_name[] = "UE Information Change";
  ev_trig_style->name = cp_str_to_ba(ev_style_name);

  // RIC Event Trigger Format Type
  // Mandatory
  // 9.3.5
  // 6.2.2.4.
  // INTEGER
  ev_trig_style->format = FORMAT_4_E2SM_RC_EV_TRIGGER_FORMAT;

  // Sequence of RAN Parameters for L2 Variables
  // [0 - 65535]
  ev_trig->sz_seq_ran_param_l2_var = 0;
  ev_trig->seq_ran_param_l2_var = NULL;

  //Sequence of Call Process Types
  // [0-65535]
  ev_trig->sz_seq_call_proc_type = 0;
  ev_trig->seq_call_proc_type = NULL;

  // Sequence of RAN Parameters for Identifying UEs
  // 0-65535
  ev_trig->sz_seq_ran_param_id_ue = 0;
  ev_trig->seq_ran_param_id_ue = NULL;

  // Sequence of RAN Parameters for Identifying Cells
  // 0-65535
  ev_trig->sz_seq_ran_param_id_cell = 0;
  ev_trig->seq_ran_param_id_cell = NULL;
}

static void fill_rc_report(ran_func_def_report_t* report)
{
  // Sequence of REPORT styles
  // [1 - 63]
  report->sz_seq_report_sty = 1;
  report->seq_report_sty = calloc(report->sz_seq_report_sty, sizeof(seq_report_sty_t));
  assert(report->seq_report_sty != NULL && "Memory exhausted");

  seq_report_sty_t* report_style = &report->seq_report_sty[0];

    // RIC Report Style Type
  // Mandatory
  // 9.3.3
  // 6.2.2.2.
  // INTEGER
  report_style->report_type = 4;

  // RIC Report Style Name
  // Mandatory
  // 9.3.4
  // 6.2.2.3.
  // PrintableString(SIZE(1..150,...)) 
  const char report_name[] = "UE Information";
  report_style->name = cp_str_to_ba(report_name);

  // Supported RIC Event Trigger Style Type 
  // Mandatory
  // 9.3.3
  // 6.2.2.2.
  // INTEGER
  report_style->ev_trig_type = FORMAT_4_E2SM_RC_EV_TRIGGER_FORMAT;

  // RIC Report Action Format Type
  // Mandatory
  // 9.3.5
  // 6.2.2.4.
  // INTEGER
  report_style->act_frmt_type = FORMAT_1_E2SM_RC_ACT_DEF;

  // RIC Indication Header Format Type
  // Mandatory
  // 9.3.5
  // 6.2.2.4.
  // INTEGER
  report_style->ind_hdr_type = FORMAT_1_E2SM_RC_IND_HDR;

  // RIC Indication Message Format Type
  // Mandatory
  // 9.3.5
  // 6.2.2.4.
  // INTEGER
  report_style->ind_msg_type = FORMAT_2_E2SM_RC_IND_MSG;

  // Sequence of RAN Parameters Supported
  // [0 - 65535]
  report_style->sz_seq_ran_param = 1;
  report_style->ran_param = calloc(report_style->sz_seq_ran_param, sizeof(seq_ran_param_3_t));
  assert(report_style->ran_param != NULL && "Memory exhausted");

  // RAN Parameter ID
  // Mandatory
  // 9.3.8
  // [1- 4294967295]
  report_style->ran_param[0].id = RRC_STATE_CHANGED_TO_E2SM_RC_RAN_PARAM_ID;

  // RAN Parameter Name
  // Mandatory
  // 9.3.9
  // [1-150] 
  const char ran_param_name[] = "RRC State";
  report_style->ran_param[0].name = cp_str_to_ba(ran_param_name);

  // RAN Parameter Definition
  // Optional
  // 9.3.51
  report_style->ran_param[0].def = NULL;
}

static void fill_rc_control(ran_func_def_ctrl_t* ctrl)
{
  // Sequence of CONTROL styles
  // [1 - 63]
  ctrl->sz_seq_ctrl_style = 1;
  ctrl->seq_ctrl_style = calloc(ctrl->sz_seq_ctrl_style, sizeof(seq_ctrl_style_t));
  assert(ctrl->seq_ctrl_style != NULL && "Memory exhausted");

  seq_ctrl_style_t* ctrl_style = &ctrl->seq_ctrl_style[0];

  // RIC Control Style Type
  // Mandatory
  // 9.3.3
  // 6.2.2.2
  ctrl_style->style_type = 1;

  //RIC Control Style Name
  //Mandatory
  //9.3.4
  // [1 -150]
  const char control_name[] = "Radio Bearer Control";
  ctrl_style->name = cp_str_to_ba(control_name);

  // RIC Control Header Format Type
  // Mandatory
  // 9.3.5
  ctrl_style->hdr = FORMAT_1_E2SM_RC_CTRL_HDR;

  // RIC Control Message Format Type
  // Mandatory
  // 9.3.5
  ctrl_style->msg = FORMAT_1_E2SM_RC_CTRL_MSG;

  // RIC Call Process ID Format Type
  // Optional
  ctrl_style->call_proc_id_type = NULL;

  // RIC Control Outcome Format Type
  // Mandatory
  // 9.3.5
  ctrl_style->out_frmt = FORMAT_1_E2SM_RC_CTRL_OUT;

  // Sequence of Control Actions
  // [0-65535]
  ctrl_style->sz_seq_ctrl_act = 1;
  ctrl_style->seq_ctrl_act = calloc(ctrl_style->sz_seq_ctrl_act, sizeof(seq_ctrl_act_2_t));
  assert(ctrl_style->seq_ctrl_act != NULL && "Memory exhausted");

  seq_ctrl_act_2_t* ctrl_act = &ctrl_style->seq_ctrl_act[0];

  // Control Action ID
  // Mandatory
  // 9.3.6
  // [1-65535]
  ctrl_act->id = 2;

  // Control Action Name
  // Mandatory
  // 9.3.7
  // [1-150]
  const char control_act_name[] = "QoS flow mapping configuration";
  ctrl_act->name = cp_str_to_ba(control_act_name);

  // Sequence of Associated RAN Parameters
  // [0-65535]
  ctrl_act->sz_seq_assoc_ran_param = 2;
  ctrl_act->assoc_ran_param = calloc(ctrl_act->sz_seq_assoc_ran_param, sizeof(seq_ran_param_3_t));
  assert(ctrl_act->assoc_ran_param != NULL && "Memory exhausted");

  seq_ran_param_3_t* assoc_ran_param = ctrl_act->assoc_ran_param;

  // DRB ID
  assoc_ran_param[0].id = 1;
  const char ran_param_drb_id[] = "DRB ID";
  assoc_ran_param[0].name = cp_str_to_ba(ran_param_drb_id);
  assoc_ran_param[0].def = NULL;

  // List of QoS Flows to be modified in DRB
  assoc_ran_param[1].id = 2;
  const char ran_param_list_qos_flow[] = "List of QoS Flows to be modified in DRB";
  assoc_ran_param[1].name = cp_str_to_ba(ran_param_list_qos_flow);
  assoc_ran_param[1].def = calloc(1, sizeof(ran_param_def_t));
  assert(assoc_ran_param[1].def != NULL && "Memory exhausted");

  assoc_ran_param[1].def->type = LIST_RAN_PARAMETER_DEF_TYPE;
  assoc_ran_param[1].def->lst = calloc(1, sizeof(ran_param_type_t));
  assert(assoc_ran_param[1].def->lst != NULL && "Memory exhausted");

  ran_param_type_t* lst = assoc_ran_param[1].def->lst;
  lst->sz_ran_param = 2;
  lst->ran_param = calloc(lst->sz_ran_param, sizeof(ran_param_lst_struct_t));
  assert(lst->ran_param != NULL && "Memory exhausted");

  // QoS Flow Identifier
  lst->ran_param[0].ran_param_id = 4;
  const char ran_param_qos_flow_id[] = "QoS Flow Identifier";
  lst->ran_param[0].ran_param_name = cp_str_to_ba(ran_param_qos_flow_id);
  lst->ran_param[0].ran_param_def = NULL;

  // QoS Flow Mapping Indication
  lst->ran_param[1].ran_param_id = 5;
  const char ran_param_qos_flow_mapping_ind[] = "QoS Flow Mapping Indication";
  lst->ran_param[1].ran_param_name = cp_str_to_ba(ran_param_qos_flow_mapping_ind);
  lst->ran_param[1].ran_param_def = NULL;

  // Sequence of Associated RAN 
  // Parameters for Control Outcome
  // [0- 255]
  ctrl_style->sz_ran_param_ctrl_out = 0;
  ctrl_style->ran_param_ctrl_out = NULL;
}

e2sm_rc_func_def_t fill_rc_ran_def_gnb(void)
{
  e2sm_rc_func_def_t def = {0};

  // RAN Function Definition for EVENT TRIGGER
  // Optional
  // 9.2.2.2
  def.ev_trig = calloc(1, sizeof(ran_func_def_ev_trig_t));
  assert(def.ev_trig != NULL && "Memory exhausted");
  fill_rc_ev_trig(def.ev_trig);

  // RAN Function Definition for REPORT
  // Optional
  // 9.2.2.3
  def.report = calloc(1, sizeof(ran_func_def_report_t));
  assert(def.report != NULL && "Memory exhausted");
  fill_rc_report(def.report);

  // RAN Function Definition for INSERT
  // Optional
  // 9.2.2.4
  def.insert = NULL;

  // RAN Function Definition for CONTROL
  // Optional
  // 9.2.2.5
  def.ctrl = calloc(1, sizeof(ran_func_def_ctrl_t));
  assert(def.ctrl != NULL && "Memory exhausted");
  fill_rc_control(def.ctrl);

  // RAN Function Definition for POLICY
  // Optional
  // 9.2.2.6
  def.policy = NULL;

  return def;
}

static e2sm_rc_func_def_t fill_rc_ran_def_cu(void)
{
  e2sm_rc_func_def_t def = {0};

  // RAN Function Definition for EVENT TRIGGER
  // Optional
  // 9.2.2.2
  def.ev_trig = calloc(1, sizeof(ran_func_def_ev_trig_t));
  assert(def.ev_trig != NULL && "Memory exhausted");
  fill_rc_ev_trig(def.ev_trig);

  // RAN Function Definition for REPORT
  // Optional
  // 9.2.2.3
  def.report = calloc(1, sizeof(ran_func_def_report_t));
  assert(def.report != NULL && "Memory exhausted");
  fill_rc_report(def.report);

  // RAN Function Definition for INSERT
  // Optional
  // 9.2.2.4
  def.insert = NULL;

  // RAN Function Definition for CONTROL
  // Optional
  // 9.2.2.5
  def.ctrl = calloc(1, sizeof(ran_func_def_ctrl_t));
  assert(def.ctrl != NULL && "Memory exhausted");
  fill_rc_control(def.ctrl);

  // RAN Function Definition for POLICY
  // Optional
  // 9.2.2.6
  def.policy = NULL;

  return def;
}

static e2sm_rc_func_def_t fill_rc_ran_def_null(void)
{
  e2sm_rc_func_def_t def = {0};

  // RAN Function Definition for EVENT TRIGGER
  // Optional
  // 9.2.2.2
  def.ev_trig = NULL;

  // RAN Function Definition for REPORT
  // Optional
  // 9.2.2.3
  def.report = NULL;

  // RAN Function Definition for INSERT
  // Optional
  // 9.2.2.4
  def.insert = NULL;

  // RAN Function Definition for CONTROL
  // Optional
  // 9.2.2.5
  def.ctrl = NULL;

  // RAN Function Definition for POLICY
  // Optional
  // 9.2.2.6
  def.policy = NULL;

  return def;
}

static e2sm_rc_func_def_t fill_rc_ran_def_cucp(void)
{
  e2sm_rc_func_def_t def = {0};

  // RAN Function Definition for EVENT TRIGGER
  // Optional
  // 9.2.2.2
  def.ev_trig = calloc(1, sizeof(ran_func_def_ev_trig_t));
  assert(def.ev_trig != NULL && "Memory exhausted");
  fill_rc_ev_trig(def.ev_trig);

  // RAN Function Definition for REPORT
  // Optional
  // 9.2.2.3
  def.report = calloc(1, sizeof(ran_func_def_report_t));
  assert(def.report != NULL && "Memory exhausted");
  fill_rc_report(def.report);

  // RAN Function Definition for INSERT
  // Optional
  // 9.2.2.4
  def.insert = NULL;

  // RAN Function Definition for CONTROL
  // Optional
  // 9.2.2.5
  def.ctrl = NULL;

  // RAN Function Definition for POLICY
  // Optional
  // 9.2.2.6
  def.policy = NULL;

  return def;
}

typedef e2sm_rc_func_def_t (*fp_rc_func_def)(void);

static const fp_rc_func_def ran_def_rc[END_NGRAN_NODE_TYPE] =
{
  NULL,
  NULL,
  fill_rc_ran_def_gnb,
  NULL,
  NULL,
  fill_rc_ran_def_cu,
  NULL,
  fill_rc_ran_def_null, // DU - at the moment, no Service is supported
  NULL,
  fill_rc_ran_def_cucp,
  fill_rc_ran_def_null, // CU-UP - at the moment, no Service is supported
};

void read_rc_setup_sm(void* data)
{
  assert(data != NULL);
//  assert(data->type == RAN_CTRL_V1_3_AGENT_IF_E2_SETUP_ANS_V0);
  rc_e2_setup_t* rc = (rc_e2_setup_t*)data;

  /* Fill the RAN Function Definition with currently supported measurements */
  
  // RAN Function Name is already filled in fill_ran_function_name() in rc_sm_agent.c

  const ngran_node_t node_type = get_e2_node_type();
  rc->ran_func_def = ran_def_rc[node_type]();

  // E2 Setup Request is sent periodically until the connection is established
  // RC subscritpion data should be initialized only once
  const int ret = pthread_once(&once_rc_mutex, init_once_rc);
  DevAssert(ret == 0);
}


RB_PROTOTYPE(ric_id_2_param_id_trees, ric_req_id_s, entries, cmp_ric_req_id);

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
  const ngran_node_t node_type = get_e2_node_type();
  rc_ind->msg.frmt_2.seq_ue_id[0].ue_id = fill_ue_id_data[node_type](rrc_ue_context, 0, 0);

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

  assert(ctrl->hdr.format == FORMAT_1_E2SM_RC_CTRL_HDR && "Indication Header Format received not valid");
  assert(ctrl->msg.format == FORMAT_1_E2SM_RC_CTRL_MSG && "Indication Message Format received not valid");
  assert(ctrl->hdr.frmt_1.ctrl_act_id == 2 && "Currently only QoS flow mapping configuration supported");

  printf("QoS flow mapping configuration\n");

  const seq_ran_param_t* ran_param = ctrl->msg.frmt_1.ran_param;

  // DRB ID
  assert(ran_param[0].ran_param_id == 1 && "First RAN Parameter ID has to be DRB ID");
  assert(ran_param[0].ran_param_val.type == ELEMENT_KEY_FLAG_TRUE_RAN_PARAMETER_VAL_TYPE);
  printf("DRB ID %ld \n", ran_param[0].ran_param_val.flag_true->int_ran);


  // List of QoS Flows to be modified in DRB
  assert(ran_param[1].ran_param_id == 2 && "Second RAN Parameter ID has to be List of QoS Flows");
  assert(ran_param[1].ran_param_val.type == LIST_RAN_PARAMETER_VAL_TYPE);
  printf("List of QoS Flows to be modified in DRB\n");
  const lst_ran_param_t* lrp = ran_param[1].ran_param_val.lst->lst_ran_param;

  // The following assertion should be true, but there is a bug in the std
  // check src/sm/rc_sm/enc/rc_enc_asn.c:1085 and src/sm/rc_sm/enc/rc_enc_asn.c:984 
  // assert(lrp->ran_param_struct.ran_param_struct[0].ran_param_id == 3);

  // QoS Flow Identifier
  assert(lrp->ran_param_struct.ran_param_struct[0].ran_param_id == 4);
  assert(lrp->ran_param_struct.ran_param_struct[0].ran_param_val.type == ELEMENT_KEY_FLAG_TRUE_RAN_PARAMETER_VAL_TYPE);
  int64_t qfi = lrp->ran_param_struct.ran_param_struct[0].ran_param_val.flag_true->int_ran;
  assert(qfi > -1 && qfi < 65);

  // QoS Flow Mapping Indication
  assert(lrp->ran_param_struct.ran_param_struct[1].ran_param_id == 5);
  assert(lrp->ran_param_struct.ran_param_struct[1].ran_param_val.type == ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE);
  int64_t dir = lrp->ran_param_struct.ran_param_struct[1].ran_param_val.flag_false->int_ran;
  assert(dir == 0 || dir == 1);

  printf("qfi = %ld, dir %ld \n", qfi, dir);


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
