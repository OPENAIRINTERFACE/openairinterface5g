/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "ran_func_rc.h"
#include "ran_func_rc_subs.h"
#include "ran_func_rc_extern.h"
#include "ran_e2sm_ue_id.h"
#include "../../flexric/src/sm/rc_sm/ie/ir/lst_ran_param.h"
#include "../../flexric/src/sm/rc_sm/ie/ir/ran_param_list.h"
#include "../../flexric/src/agent/e2_agent_api.h"
#include "openair2/E2AP/flexric/src/lib/sm/enc/enc_ue_id.h"
#include "openair2/E2AP/flexric/src/sm/rc_sm/rc_sm_id.h"
#include "openair2/RRC/NR/rrc_cell_management.h"
#include "openair2/RRC/NR/rrc_gNB_du.h"
#include "openair2/RRC/NR/rrc_gNB_mobility.h"
#include "openair2/RRC/NR/nr_rrc_proto.h"

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

static seq_ev_trg_style_t fill_ev_tr_format_4(const char *ev_style_name)
{
  seq_ev_trg_style_t ev_trig_style = {0};

  // RIC Event Trigger Style Type
  // Mandatory
  // 9.3.3
  ev_trig_style.style = 4;

  // RIC Event Trigger Style Name
  // Mandatory
  // 9.3.4
  ev_trig_style.name = cp_str_to_ba(ev_style_name);

  // RIC Event Trigger Format Type
  // Mandatory
  // 9.3.5
  ev_trig_style.format = FORMAT_4_E2SM_RC_EV_TRIGGER_FORMAT;

  return ev_trig_style;
}

static seq_ev_trg_style_t fill_ev_tr_format_1(const char *ev_style_name)
{
  seq_ev_trg_style_t ev_trig_style = {0};

  // RIC Event Trigger Style Type
  // Mandatory
  // 9.3.3
  ev_trig_style.style = 1;

  // RIC Event Trigger Style Name
  // Mandatory
  // 9.3.4
  ev_trig_style.name = cp_str_to_ba(ev_style_name);

  // RIC Event Trigger Format Type
  // Mandatory
  // 9.3.5
  ev_trig_style.format = FORMAT_1_E2SM_RC_EV_TRIGGER_FORMAT;

  return ev_trig_style;
}

static seq_ev_trg_style_t fill_ev_tr_format_5(const char *ev_style_name)
{
  seq_ev_trg_style_t ev_trig_style = {0};

  // RIC Event Trigger Style Type
  // Mandatory
  // 9.3.3
  ev_trig_style.style = 5;

  // RIC Event Trigger Style Name
  // Mandatory
  // 9.3.4
  ev_trig_style.name = cp_str_to_ba(ev_style_name);

  // RIC Event Trigger Format Type
  // Mandatory
  // 9.3.5
  ev_trig_style.format = FORMAT_5_E2SM_RC_EV_TRIGGER_FORMAT;

  return ev_trig_style;
}

static void fill_rc_ev_trig(ran_func_def_ev_trig_t* ev_trig)
{
  // Sequence of EVENT TRIGGER styles
  // [1 - 63]
  ev_trig->sz_seq_ev_trg_style = 3;
  ev_trig->seq_ev_trg_style = calloc(ev_trig->sz_seq_ev_trg_style, sizeof(seq_ev_trg_style_t));
  assert(ev_trig->seq_ev_trg_style != NULL && "Memory exhausted");

  ev_trig->seq_ev_trg_style[0] = fill_ev_tr_format_1("Message Event");
  ev_trig->seq_ev_trg_style[1] = fill_ev_tr_format_4("UE Information Change");
  ev_trig->seq_ev_trg_style[2] = fill_ev_tr_format_5("On Demand");

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

static seq_report_sty_t fill_report_style_4(const char *report_name)
{
  seq_report_sty_t report_style = {0};

  // RIC Report Style Type
  // Mandatory
  // 9.3.3
  report_style.report_type = 4;

  // RIC Report Style Name
  // Mandatory
  // 9.3.4
  report_style.name = cp_str_to_ba(report_name);

  // Supported RIC Event Trigger Style Type 
  // Mandatory
  // 9.3.3
  report_style.ev_trig_type = FORMAT_4_E2SM_RC_EV_TRIGGER_FORMAT;

  // RIC Report Action Format Type
  // Mandatory
  // 9.3.5
  report_style.act_frmt_type = FORMAT_1_E2SM_RC_ACT_DEF;

  // RIC Indication Header Format Type
  // Mandatory
  // 9.3.5
  report_style.ind_hdr_type = FORMAT_1_E2SM_RC_IND_HDR;

  // RIC Indication Message Format Type
  // Mandatory
  // 9.3.5
  report_style.ind_msg_type = FORMAT_2_E2SM_RC_IND_MSG;

  // Sequence of RAN Parameters Supported
  // [0 - 65535]
  report_style.sz_seq_ran_param = 1;
  report_style.ran_param = calloc(report_style.sz_seq_ran_param, sizeof(seq_ran_param_3_t));
  assert(report_style.ran_param != NULL && "Memory exhausted");

  // RAN Parameter ID
  // Mandatory
  // 9.3.8
  // [1- 4294967295]
  report_style.ran_param[0].id = E2SM_RC_RS4_RRC_STATE_CHANGED_TO;

  // RAN Parameter Name
  // Mandatory
  // 9.3.9
  // [1-150] 
  const char ran_param_name[] = "RRC State Changed To";
  report_style.ran_param[0].name = cp_str_to_ba(ran_param_name);

  // RAN Parameter Definition
  // Optional
  // 9.3.51
  report_style.ran_param[0].def = NULL;

  return report_style;
}

static seq_report_sty_t fill_report_style_1(const char *report_name)
{
  seq_report_sty_t report_style = {0};

  // RIC Report Style Type
  // Mandatory
  // 9.3.3
  report_style.report_type = 1;

  // RIC Report Style Name
  // Mandatory
  // 9.3.4
  report_style.name = cp_str_to_ba(report_name);

  // Supported RIC Event Trigger Style Type 
  // Mandatory
  // 9.3.3
  report_style.ev_trig_type = FORMAT_1_E2SM_RC_EV_TRIGGER_FORMAT;

  // RIC Report Action Format Type
  // Mandatory
  // 9.3.5
  report_style.act_frmt_type = FORMAT_1_E2SM_RC_ACT_DEF;

  // RIC Indication Header Format Type
  // Mandatory
  // 9.3.5
  report_style.ind_hdr_type = FORMAT_1_E2SM_RC_IND_HDR;

  // RIC Indication Message Format Type
  // Mandatory
  // 9.3.5
  report_style.ind_msg_type = FORMAT_1_E2SM_RC_IND_MSG;

  // Sequence of RAN Parameters Supported
  // [0 - 65535]
  report_style.sz_seq_ran_param = 2;
  report_style.ran_param = calloc_or_fail(report_style.sz_seq_ran_param, sizeof(seq_ran_param_3_t));

  // RAN Parameter ID
  // Mandatory
  // 9.3.8
  // [1- 4294967295]
  report_style.ran_param[0].id = E2SM_RC_RS1_RRC_MESSAGE;
  report_style.ran_param[1].id = E2SM_RC_RS1_UE_ID;

  // RAN Parameter Name
  // Mandatory
  // 9.3.9
  // [1-150] 
  const char ran_param_name_0[] = "RRC Message";
  report_style.ran_param[0].name = cp_str_to_ba(ran_param_name_0);
  const char ran_param_name_1[] = "UE ID";
  report_style.ran_param[1].name = cp_str_to_ba(ran_param_name_1);

  // RAN Parameter Definition
  // Optional
  // 9.3.51
  report_style.ran_param[0].def = NULL;
  report_style.ran_param[1].def = NULL;

  return report_style;
}

static seq_report_sty_t fill_report_style_5(const char *report_name)
{
  seq_report_sty_t report_style = {0};

  // RIC Report Style Type
  // Mandatory
  // 9.3.3
  report_style.report_type = 5;

  // RIC Report Style Name
  // Mandatory
  // 9.3.4
  report_style.name = cp_str_to_ba(report_name);

  // Supported RIC Event Trigger Style Type
  // Mandatory
  // 9.3.3
  report_style.ev_trig_type = FORMAT_5_E2SM_RC_EV_TRIGGER_FORMAT;

  // RIC Report Action Format Type
  // Mandatory
  // 9.3.5
  report_style.act_frmt_type = FORMAT_1_E2SM_RC_ACT_DEF;

  // RIC Indication Header Format Type
  // Mandatory
  // 9.3.5
  report_style.ind_hdr_type = FORMAT_1_E2SM_RC_IND_HDR;

  // RIC Indication Message Format Type
  // Mandatory
  // 9.3.5
  report_style.ind_msg_type = FORMAT_4_E2SM_RC_IND_MSG;

  // Sequence of RAN Parameters Supported
  // [0 - 65535]
  report_style.sz_seq_ran_param = 2;
  report_style.ran_param = calloc(report_style.sz_seq_ran_param, sizeof(seq_ran_param_3_t));
  assert(report_style.ran_param != NULL && "Memory exhausted");

  // RAN Parameter ID
  // Mandatory
  // 9.3.8
  // [1- 4294967295]
  report_style.ran_param[0].id = E2SM_RC_RS5_UE_CONTEXT_INFORMATION;
  report_style.ran_param[1].id = E2SM_RC_RS5_NEIGHBOUR_RELATION_TABLE;

  // RAN Parameter Name
  // Mandatory
  // 9.3.9
  // [1-150]
  const char ran_param_name[] = "UE Context Information";
  report_style.ran_param[0].name = cp_str_to_ba(ran_param_name);
  const char ran_param_name_1[] = "Neighbour Relation Table";
  report_style.ran_param[1].name = cp_str_to_ba(ran_param_name_1);

  // RAN Parameter Definition
  // Optional
  // 9.3.51
  report_style.ran_param[0].def = NULL;
  report_style.ran_param[1].def = NULL;

  return report_style;
}

static void fill_rc_report(ran_func_def_report_t* report)
{
  // Sequence of REPORT styles
  // [1 - 63]
  report->sz_seq_report_sty = 3;
  report->seq_report_sty = calloc(report->sz_seq_report_sty, sizeof(seq_report_sty_t));
  assert(report->seq_report_sty != NULL && "Memory exhausted");

  report->seq_report_sty[0] = fill_report_style_1("Message Copy");
  report->seq_report_sty[1] = fill_report_style_4("UE Information");
  report->seq_report_sty[2] = fill_report_style_5("On Demand Report");
}

static void fill_rc_radio_bearer_ctrl(seq_ctrl_style_t* ctrl_style)
{
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

/* RIC Control Style Types, 7.6. Only the two below are implemented. */
typedef enum {
  RC_CTRL_STYLE_RADIO_BEARER = 1,
  RC_CTRL_STYLE_CONN_MODE_MOBILITY = 3,
} rc_ctrl_style_type_e;

/** @brief Add a leaf RAN parameter (no nested definition) to a LIST/STRUCTURE item */
static void fill_ran_param_leaf(ran_param_lst_struct_t* dst, uint32_t id, const char* name)
{
  dst->ran_param_id = id;
  dst->ran_param_name = cp_str_to_ba(name);
  dst->ran_param_def = NULL;
}

/** @brief Give a LIST/STRUCTURE item a nested STRUCTURE definition of sz members
 * @return the member array, for the caller to fill */
static ran_param_lst_struct_t* fill_ran_param_strct(ran_param_lst_struct_t* dst, uint32_t id, const char* name, size_t sz)
{
  dst->ran_param_id = id;
  dst->ran_param_name = cp_str_to_ba(name);

  dst->ran_param_def = calloc(1, sizeof(ran_param_def_t));
  AssertFatal(dst->ran_param_def != NULL, "Memory exhausted");
  dst->ran_param_def->type = STRUCTURE_RAN_PARAMETER_DEF_TYPE;
  dst->ran_param_def->strct = calloc(1, sizeof(ran_param_type_t));
  AssertFatal(dst->ran_param_def->strct != NULL, "Memory exhausted");

  ran_param_type_t* strct = dst->ran_param_def->strct;
  strct->sz_ran_param = sz;
  strct->ran_param = calloc(sz, sizeof(ran_param_lst_struct_t));
  AssertFatal(strct->ran_param != NULL, "Memory exhausted");

  return strct->ran_param;
}

/** @brief Advertise CONTROL Style 3 "Connected mode mobility control", 7.6.4
 *
 * Only the Handover Control action is offered, and of its RAN parameters only
 * Target Primary Cell ID: the handover this triggers moves the UE with its
 * existing bearers, so there is nothing to say about the PDU session, DRB and
 * secondary cell lists 8.4.4.1 also defines. Advertising them would invite
 * CONTROL messages write_ctrl_rc_sm() would have to reject. */
static void fill_rc_conn_mode_mobility_ctrl(seq_ctrl_style_t* ctrl_style)
{
  // RIC Control Style Type. Mandatory. 9.3.3, 6.2.2.2
  ctrl_style->style_type = RC_CTRL_STYLE_CONN_MODE_MOBILITY;

  // RIC Control Style Name. Mandatory. 9.3.4. [1-150]
  ctrl_style->name = cp_str_to_ba("Connected mode mobility control");

  // RIC Control Header/Message/Outcome Format Type. Mandatory. 9.3.5
  ctrl_style->hdr = FORMAT_1_E2SM_RC_CTRL_HDR;
  ctrl_style->msg = FORMAT_1_E2SM_RC_CTRL_MSG;
  ctrl_style->out_frmt = FORMAT_1_E2SM_RC_CTRL_OUT;

  // RIC Call Process ID Format Type. Optional
  ctrl_style->call_proc_id_type = NULL;

  // Sequence of Control Actions. [0-65535]
  ctrl_style->sz_seq_ctrl_act = 1;
  ctrl_style->seq_ctrl_act = calloc(ctrl_style->sz_seq_ctrl_act, sizeof(seq_ctrl_act_2_t));
  AssertFatal(ctrl_style->seq_ctrl_act != NULL, "Memory exhausted");

  seq_ctrl_act_2_t* ctrl_act = &ctrl_style->seq_ctrl_act[0];

  // Control Action ID. Mandatory. 9.3.6. [1-65535]
  ctrl_act->id = HANDOVER_CONTROL_7_6_4_1;

  // Control Action Name. Mandatory. 9.3.7. [1-150]
  ctrl_act->name = cp_str_to_ba("Handover Control");

  // Sequence of Associated RAN Parameters. [0-65535]
  ctrl_act->sz_seq_assoc_ran_param = 1;
  ctrl_act->assoc_ran_param = calloc(ctrl_act->sz_seq_assoc_ran_param, sizeof(seq_ran_param_3_t));
  AssertFatal(ctrl_act->assoc_ran_param != NULL, "Memory exhausted");

  /* Target Primary Cell ID
   *   > CHOICE Target Cell
   *       > NR Cell ID
   *           > NR CGI
   * The E-UTRA branch of the CHOICE is not offered: this is an NR-only CU. */
  seq_ran_param_3_t* target_cell = &ctrl_act->assoc_ran_param[0];
  target_cell->id = TARGET_PRIMARY_CELL_ID_8_4_4_1;
  target_cell->name = cp_str_to_ba("Target Primary Cell ID");

  target_cell->def = calloc(1, sizeof(ran_param_def_t));
  AssertFatal(target_cell->def != NULL, "Memory exhausted");
  target_cell->def->type = STRUCTURE_RAN_PARAMETER_DEF_TYPE;
  target_cell->def->strct = calloc(1, sizeof(ran_param_type_t));
  AssertFatal(target_cell->def->strct != NULL, "Memory exhausted");

  ran_param_type_t* strct = target_cell->def->strct;
  strct->sz_ran_param = 1;
  strct->ran_param = calloc(strct->sz_ran_param, sizeof(ran_param_lst_struct_t));
  AssertFatal(strct->ran_param != NULL, "Memory exhausted");

  ran_param_lst_struct_t* choice = fill_ran_param_strct(&strct->ran_param[0], CHOICE_TARGET_CELL_8_4_4_1, "CHOICE Target Cell", 1);
  ran_param_lst_struct_t* nr_cell = fill_ran_param_strct(&choice[0], NR_CELL_8_4_4_1, "NR Cell ID", 1);
  fill_ran_param_leaf(&nr_cell[0], NR_CGI_8_4_4_1, "NR CGI");

  // Sequence of Associated RAN Parameters for Control Outcome. [0-255]
  ctrl_style->sz_ran_param_ctrl_out = 0;
  ctrl_style->ran_param_ctrl_out = NULL;
}

static void fill_rc_control(ran_func_def_ctrl_t* ctrl)
{
  // Sequence of CONTROL styles
  // [1 - 63]
  ctrl->sz_seq_ctrl_style = 2;
  ctrl->seq_ctrl_style = calloc(ctrl->sz_seq_ctrl_style, sizeof(seq_ctrl_style_t));
  assert(ctrl->seq_ctrl_style != NULL && "Memory exhausted");

  fill_rc_radio_bearer_ctrl(&ctrl->seq_ctrl_style[0]);
  fill_rc_conn_mode_mobility_ctrl(&ctrl->seq_ctrl_style[1]);
}

static ran_function_name_t fill_rc_ran_func_name(void)
{
  ran_function_name_t dst = {
    .name = cp_str_to_ba(SM_RAN_CTRL_SHORT_NAME),
    .oid = cp_str_to_ba(SM_RAN_CTRL_OID),
    .description = cp_str_to_ba(SM_RAN_CTRL_DESCRIPTION),
    .instance = NULL
  };
  return dst;
}

e2sm_rc_func_def_t fill_rc_ran_def_gnb(void)
{
  e2sm_rc_func_def_t def = {0};

  // RAN Function Name
  // Mandatory
  // 9.3.2
  def.name = fill_rc_ran_func_name();

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

  // RAN Function Name
  // Mandatory
  // 9.3.2
  def.name = fill_rc_ran_func_name();

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

  // RAN Function Name
  // Mandatory
  // 9.3.2
  def.name = fill_rc_ran_func_name();

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

  // RAN Function Name
  // Mandatory
  // 9.3.2
  def.name = fill_rc_ran_func_name();

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
  const ngran_node_t node_type = get_e2_node_type();
  rc->ran_func_def = ran_def_rc[node_type]();

  // E2 Setup Request is sent periodically until the connection is established
  // RC subscritpion data should be initialized only once
  const int ret = pthread_once(&once_rc_mutex, init_once_rc);
  DevAssert(ret == 0);
}

static seq_ran_param_t fill_rrc_state_change_seq_ran(const rrc_state_e2sm_rc_e rrc_state)
{
  seq_ran_param_t seq_ran_param = {0};

  seq_ran_param.ran_param_id = E2SM_RC_RS4_RRC_STATE_CHANGED_TO;
  seq_ran_param.ran_param_val.type = ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE;
  seq_ran_param.ran_param_val.flag_false = calloc(1, sizeof(ran_parameter_value_t));
  assert(seq_ran_param.ran_param_val.flag_false != NULL && "Memory exhausted");
  seq_ran_param.ran_param_val.flag_false->type = INTEGER_RAN_PARAMETER_VALUE;
  seq_ran_param.ran_param_val.flag_false->int_ran = rrc_state;  

  return seq_ran_param;
}

static rc_ind_data_t* fill_ue_rrc_state_change(const gNB_RRC_UE_t *rrc_ue_context, const rrc_state_e2sm_rc_e rrc_state, const uint16_t cond_id)
{
  rc_ind_data_t* rc_ind = calloc(1, sizeof(rc_ind_data_t));
  assert(rc_ind != NULL && "Memory exhausted");

  // Generate Indication Header
  rc_ind->hdr.format = FORMAT_1_E2SM_RC_IND_HDR;
  rc_ind->hdr.frmt_1.ev_trigger_id = malloc_or_fail(sizeof(uint32_t));
  *rc_ind->hdr.frmt_1.ev_trigger_id = cond_id;

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

static void send_aper_ric_ind(const uint32_t ric_req_id, rc_ind_data_t* rc_ind_data)
{
  async_event_agent_api(ric_req_id, rc_ind_data);
  printf("[E2 AGENT] Event for RIC request ID %d generated\n", ric_req_id);
}

#ifdef NGRAN_GNB_CUCP
static nr_cgi_t fill_nr_cgi(plmn_id_t *plmn, uint64_t cell_id)
{
  nr_cgi_t cgi = {0};
  cgi.plmn_id.mcc = plmn->mcc;
  cgi.plmn_id.mnc = plmn->mnc;
  cgi.plmn_id.mnc_digit_len = plmn->mnc_digit_length;
  cgi.nr_cell_id = cell_id;

  return cgi;
}

#endif

/* REPORT Service Style 5 ("On Demand Report", E2SM-RC v01.03 7.4.6).
 * Check if and which RAN Parameter IDs does the Action Definition carry.
 * Choices are described in section 8.2.5.
 * The Indication Header Format 1 with ev_trigger_id is optional, so left unset. */
static rc_ind_data_t* fill_context_info_on_demand(const e2sm_rc_act_def_frmt_1_t *ad)
{
  assert(ad != NULL && "[E2 AGENT] Action Definition Format 1 empty.\n");
  rc_ind_data_t* rc_ind = calloc(1, sizeof(rc_ind_data_t));
  assert(rc_ind != NULL && "Memory exhausted");

  rc_ind->hdr.format = FORMAT_1_E2SM_RC_IND_HDR;
  rc_ind->msg.format = FORMAT_4_E2SM_RC_IND_MSG;

  const ngran_node_t node_type = get_e2_node_type();
#ifdef NGRAN_GNB_CUCP
  e2sm_rc_ind_msg_frmt_4_t *frmt_4 = &rc_ind->msg.frmt_4;
  gNB_RRC_INST* rrc = RC.nrrrc[0];
  for (size_t i = 0; i < ad->sz_param_report_def; i++) {
    if (ad->param_report_def[i].ran_param_id == E2SM_RC_RS5_UE_CONTEXT_INFORMATION) {
      size_t sz = 0;
      struct rrc_gNB_ue_context_s* rrc_ue_context = NULL;
      RB_FOREACH(rrc_ue_context, rrc_nr_ue_tree_s, &rrc->rrc_ue_head)
        sz++;

      frmt_4->sz_seq_ue_info = sz;
      frmt_4->seq_ue_info = calloc(sz, sizeof(seq_ue_info_t));
      assert(frmt_4->seq_ue_info != NULL && "Memory exhausted");

      size_t j = 0;
      RB_FOREACH(rrc_ue_context, rrc_nr_ue_tree_s, &rrc->rrc_ue_head) {
        gNB_RRC_UE_t* ue = &rrc_ue_context->ue_context;
        frmt_4->seq_ue_info[j].ue_id = fill_ue_id_data[node_type](ue, 0, 0);
        frmt_4->seq_ue_info[j].ue_ctx_info = NULL;
        nr_rrc_cell_container_t *pcell = rrc_get_pcell_for_ue(rrc, ue);
        AssertFatal(pcell != NULL, "[E2 AGENT] UE %d not connected to any cell\n", ue->rrc_ue_id);
        frmt_4->seq_ue_info[j].cell_global_id.type = NR_CGI_RAT_TYPE;
        frmt_4->seq_ue_info[j].cell_global_id.nr_cgi = fill_nr_cgi(&pcell->info.plmn, pcell->info.cell_id);
        j++;
      }

    } else if (ad->param_report_def[i].ran_param_id == E2SM_RC_RS5_NEIGHBOUR_RELATION_TABLE) {
      frmt_4->sz_seq_cell_info_2 = rrc->num_cells;
      if (rrc->num_cells == 0)
        continue;
      frmt_4->seq_cell_info_2 = calloc(frmt_4->sz_seq_cell_info_2, sizeof(*frmt_4->seq_cell_info_2));
      assert(frmt_4->seq_cell_info_2 != NULL && "Memory exhausted");

      size_t j = 0;
      nr_rrc_cell_container_t *cell;
      RB_FOREACH(cell, rrc_cell_tree, &rrc->cells) {
        frmt_4->seq_cell_info_2[j].cell_global_id.type = NR_CGI_RAT_TYPE;
        frmt_4->seq_cell_info_2[j].cell_global_id.nr_cgi = fill_nr_cgi(&cell->info.plmn, cell->info.cell_id);
        const neighbour_cell_configuration_t *neighbour_cfg = get_neighbour_cell_config(RC.nrrrc[0], cell->info.cell_id);
        if (!neighbour_cfg)
          continue;

        neighbour_rela_tbl_t *neighbour_tbl = calloc(1, sizeof(*neighbour_tbl));
        assert(neighbour_tbl != NULL && "Memory exhausted");
        frmt_4->seq_cell_info_2[j].neighbour_rela_tbl = neighbour_tbl;

        // Serving Cell PCI
        neighbour_tbl->serving_cell_pci.type = NR_SERVING_CELL_E2SM_RC;
        neighbour_tbl->serving_cell_pci.nr_pci = cell->info.pci;
        // Serving Cell ARFCN
        neighbour_tbl->serving_cell_arfcn.type = NR_SERVING_CELL_E2SM_RC;
        // DL Frequency Info for FDD Mode. Mandatory per spec.
        // If TDD, then place 0.
        neighbour_tbl->serving_cell_arfcn.nr_arfcn = (cell->info.mode == NR_MODE_FDD) ? cell->info.fdd.dl.arfcn : 0;
        neighbour_tbl->sz_neighbour_cell_list = neighbour_cfg->neighbour_cells.size;
        neighbour_tbl->neighbour_cell_list = calloc(neighbour_tbl->sz_neighbour_cell_list, sizeof(*neighbour_tbl->neighbour_cell_list));
        assert(neighbour_tbl->neighbour_cell_list != NULL && "Memory exhausted");

        size_t z = 0;
        FOR_EACH_SEQ_ARR(nr_neighbour_cell_t *, nc, &neighbour_cfg->neighbour_cells) {
          neighbour_cell_item_t *item = &neighbour_tbl->neighbour_cell_list[z];
          item->type = NR_NEIGHBOUR_CELL_E2SM_RC;
          item->choice_nr.nr_cgi = fill_nr_cgi(&nc->plmn, nc->nrcell_id);
          item->choice_nr.nr_pci = nc->physicalCellId;
          item->choice_nr.five_gs_tac = nc->tac;
          // nr_neighbour_cell_t doesn't contain mode info. Inherit from serving cell.
          item->choice_nr.nr_mode_info = (cell->info.mode == NR_MODE_FDD) ? FDD_NR_MODE_INFO_E2SM_RC : TDD_NR_MODE_INFO_E2SM_RC;
          // DL Frequency Info for FDD Mode. NR Frequency Info IE in TS 38.473 9.3.1.17. Mandatory per spec.
          // If TDD, fill anyway.
          item->choice_nr.nr_freq_info.nr_arfcn = nc->absoluteFrequencySSB;
          item->choice_nr.nr_freq_info.sz_freq_band_list = 1;
          item->choice_nr.nr_freq_info.freq_band_list = calloc(item->choice_nr.nr_freq_info.sz_freq_band_list, sizeof(*item->choice_nr.nr_freq_info.freq_band_list));
          assert(item->choice_nr.nr_freq_info.freq_band_list != NULL && "Memory exhausted");
          item->choice_nr.nr_freq_info.freq_band_list[0].freq_band_indicator_nr = nc->band;
          item->choice_nr.nr_freq_info.freq_band_list[0].sz_sul_band_list = 0;
          item->choice_nr.nr_freq_info.freq_band_list[0].sul_band_list = NULL;
          item->choice_nr.nr_freq_info.freq_shift_7p5khz = NULL;
          // neither Xn nor X2 handover not yet supported
          item->choice_nr.xn_x2_established = FALSE_XN_X2_ESTABLISHED_E2SM_RC;
          item->choice_nr.ho_validated = nc->stats.ho_success_count > 0 ? TRUE_HO_VALIDATED_E2SM_RC : FALSE_HO_VALIDATED_E2SM_RC;
          /* For every neighbour information, the value is incremented by 1.
           * When there is a change in the existing neighbour information,
           * a new incremented version number is used.
           * The deleted neighbours are not sent. */
          item->choice_nr.version = nc->stats.version;
          z++;
        }
        j++;
      }
    }
  }
#else
    printf("[E2 AGENT] \"UE Context Information\" not supported by %d.\n", node_type);
#endif

  return rc_ind;
}

static rc_ind_data_t* fill_ue_id(const gNB_RRC_UE_t *rrc_ue_context, const uint16_t cond_id)
{
  rc_ind_data_t* rc_ind = malloc_or_fail(sizeof(rc_ind_data_t));

  // Generate Indication Header
  rc_ind->hdr.format = FORMAT_1_E2SM_RC_IND_HDR;
  rc_ind->hdr.frmt_1.ev_trigger_id = malloc_or_fail(sizeof(uint32_t));
  *rc_ind->hdr.frmt_1.ev_trigger_id = cond_id;

  // Generate Indication Message
  rc_ind->msg.format = FORMAT_1_E2SM_RC_IND_MSG;

  // Initialize RAN Parameter
  rc_ind->msg.frmt_1.sz_seq_ran_param = 1;
  rc_ind->msg.frmt_1.seq_ran_param = calloc_or_fail(rc_ind->msg.frmt_1.sz_seq_ran_param, sizeof(seq_ran_param_t));

  // Fill the RAN Parameter details for UE ID
  rc_ind->msg.frmt_1.seq_ran_param[0].ran_param_id = E2SM_RC_RS1_UE_ID;
  rc_ind->msg.frmt_1.seq_ran_param[0].ran_param_val.type = ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE;
  rc_ind->msg.frmt_1.seq_ran_param[0].ran_param_val.flag_false = malloc_or_fail(sizeof(ran_parameter_value_t));
  rc_ind->msg.frmt_1.seq_ran_param[0].ran_param_val.flag_false->type = OCTET_STRING_RAN_PARAMETER_VALUE;

  const ngran_node_t node_type = get_e2_node_type();
  ue_id_e2sm_t ue_id_data = fill_ue_id_data[node_type](rrc_ue_context, 0, 0);

  UEID_t enc_ue_id_data = enc_ue_id_asn(&ue_id_data);

  const enum asn_transfer_syntax syntax = ATS_ALIGNED_BASIC_PER;
  asn_encode_to_new_buffer_result_t res = asn_encode_to_new_buffer(NULL, syntax, &asn_DEF_UEID, &enc_ue_id_data);
  assert(res.buffer != NULL && res.result.encoded > 0 && "[E2 agent] Failed to encode UE ID.");
  byte_array_t ba = {.buf = res.buffer, .len = res.result.encoded};

  rc_ind->msg.frmt_1.seq_ran_param[0].ran_param_val.flag_false->octet_str_ran = copy_byte_array(ba);

  // Call Process ID
  rc_ind->proc_id = NULL;

  free_ue_id_e2sm(&ue_id_data);
  ASN_STRUCT_RESET(asn_DEF_UEID, &enc_ue_id_data);
  free(res.buffer);

  return rc_ind;
}

static void check_ue_id_cond(const gNB_RRC_UE_t *rrc_ue_context, const uint16_t class, const uint32_t msg_id, const uint32_t ric_req_id, const e2sm_rc_ev_trg_frmt_1_t *frmt_1)
{
  for (size_t i = 0; i < frmt_1->sz_msg_ev_trg; i++) {
    msg_ev_trg_t *ev_item = &frmt_1->msg_ev_trg[i];
    if ((ev_item->msg_type == RRC_MSG_MSG_TYPE_EV_TRG && ev_item->rrc_msg.type == NR_RRC_MESSAGE_ID && ev_item->rrc_msg.nr == class && ev_item->rrc_msg.rrc_msg_id == msg_id)  // rrcSetupComplete
         || (ev_item->msg_type == NETWORK_INTERFACE_MSG_TYPE_EV_TRG && ev_item->net.ni_type == class)) {  // "F1 UE Context Setup Request", but in the subscription the class (F1) can only be specified which translates to any F1 msg
      rc_ind_data_t* rc_ind_data = fill_ue_id(rrc_ue_context, ev_item->ev_trigger_cond_id);
      send_aper_ric_ind(ric_req_id, rc_ind_data);
    }
  }
}

void signal_ue_id(const gNB_RRC_UE_t *rrc_ue_context, const uint16_t class, const uint32_t msg_id)
{
  pthread_mutex_lock(&rc_mutex);
  if (rc_subs_data.rs1_param4.data == NULL) {
    pthread_mutex_unlock(&rc_mutex);
    return;
  }

  const size_t num_subs = seq_arr_size(&rc_subs_data.rs1_param4);
  for (size_t sub_idx = 0; sub_idx < num_subs; sub_idx++) {
    const ran_param_data_t data = *(const ran_param_data_t *)seq_arr_at(&rc_subs_data.rs1_param4, sub_idx);
    check_ue_id_cond(rrc_ue_context, class, msg_id, data.ric_req_id, &data.ev_tr.frmt_1);
  }

  pthread_mutex_unlock(&rc_mutex);
}

static void check_rrc_state(const gNB_RRC_UE_t *rrc_ue_context, const rrc_state_e2sm_rc_e rrc_state, const uint32_t ric_req_id, const e2sm_rc_ev_trg_frmt_4_t *frmt_4)
{
  for (size_t i = 0; i < frmt_4->sz_ue_info_chng; i++) {
    const uint16_t cond_id = frmt_4->ue_info_chng[i].ev_trig_cond_id;
    const rrc_state_lst_t *rrc_elem = &frmt_4->ue_info_chng[i].rrc_state;
    for (size_t j = 0; j < rrc_elem->sz_rrc_state; j++) {
      const rrc_state_e2sm_rc_e ev_tr_rrc_state = rrc_elem->state_chng_to[j].state_chngd_to;
      if (ev_tr_rrc_state == rrc_state || ev_tr_rrc_state == ANY_RRC_STATE_E2SM_RC) {
        rc_ind_data_t* rc_ind_data = fill_ue_rrc_state_change(rrc_ue_context, rrc_state, cond_id);
        send_aper_ric_ind(ric_req_id, rc_ind_data);
      }
    }
  }
}

void signal_rrc_state_changed_to(const gNB_RRC_UE_t *rrc_ue_context, const rrc_state_e2sm_rc_e rrc_state)
{
  pthread_mutex_lock(&rc_mutex);
  if (rc_subs_data.rs4_param202.data == NULL) {
    pthread_mutex_unlock(&rc_mutex);
    return;
  }

  const size_t num_subs = seq_arr_size(&rc_subs_data.rs4_param202);
  for (size_t sub_idx = 0; sub_idx < num_subs; sub_idx++) {
    const ran_param_data_t data = *(const ran_param_data_t *)seq_arr_at(&rc_subs_data.rs4_param202, sub_idx);
    check_rrc_state(rrc_ue_context, rrc_state, data.ric_req_id, &data.ev_tr.frmt_4);
  }

  pthread_mutex_unlock(&rc_mutex);
}

static rc_ind_data_t* fill_rrc_msg_copy(const byte_array_t rrc_ba, const uint16_t cond_id)
{
  rc_ind_data_t* rc_ind = malloc_or_fail(sizeof(rc_ind_data_t));

  // Generate Indication Header
  rc_ind->hdr.format = FORMAT_1_E2SM_RC_IND_HDR;
  rc_ind->hdr.frmt_1.ev_trigger_id = malloc_or_fail(sizeof(uint32_t));
  *rc_ind->hdr.frmt_1.ev_trigger_id = cond_id;

  // Generate Indication Message
  rc_ind->msg.format = FORMAT_1_E2SM_RC_IND_MSG;

  // Sequence of
  // RAN Parameter
  rc_ind->msg.frmt_1.sz_seq_ran_param = 1;
  rc_ind->msg.frmt_1.seq_ran_param = calloc_or_fail(rc_ind->msg.frmt_1.sz_seq_ran_param, sizeof(seq_ran_param_t));

  rc_ind->msg.frmt_1.seq_ran_param[0].ran_param_id = E2SM_RC_RS1_RRC_MESSAGE;
  rc_ind->msg.frmt_1.seq_ran_param[0].ran_param_val.type = ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE;
  rc_ind->msg.frmt_1.seq_ran_param[0].ran_param_val.flag_false = malloc_or_fail(sizeof(ran_parameter_value_t));
  rc_ind->msg.frmt_1.seq_ran_param[0].ran_param_val.flag_false->type = OCTET_STRING_RAN_PARAMETER_VALUE;
  
  rc_ind->msg.frmt_1.seq_ran_param[0].ran_param_val.flag_false->octet_str_ran = copy_byte_array(rrc_ba);

  // Call Process ID
  rc_ind->proc_id = NULL;

  return rc_ind;
}

static void check_rrc_msg_copy(const nr_rrc_class_e nr_channel, const uint32_t rrc_msg_id, const byte_array_t rrc_ba, const uint32_t ric_req_id, const e2sm_rc_ev_trg_frmt_1_t *frmt_1)
{
  for (size_t i = 0; i < frmt_1->sz_msg_ev_trg; i++) {
    if (frmt_1->msg_ev_trg[i].msg_type != RRC_MSG_MSG_TYPE_EV_TRG)
      continue;
    if (frmt_1->msg_ev_trg[i].rrc_msg.type != NR_RRC_MESSAGE_ID)
      continue;
    if (frmt_1->msg_ev_trg[i].rrc_msg.nr == nr_channel && frmt_1->msg_ev_trg[i].rrc_msg.rrc_msg_id == rrc_msg_id) {
      rc_ind_data_t* rc_ind_data = fill_rrc_msg_copy(rrc_ba, frmt_1->msg_ev_trg[i].ev_trigger_cond_id);
      send_aper_ric_ind(ric_req_id, rc_ind_data);
    }
  }
}

void signal_rrc_msg(const nr_rrc_class_e nr_channel, const uint32_t rrc_msg_id, const byte_array_t rrc_ba)
{
  pthread_mutex_lock(&rc_mutex);
  if (rc_subs_data.rs1_param3.data == NULL) {
    pthread_mutex_unlock(&rc_mutex);
    return;
  }

  const size_t num_subs = seq_arr_size(&rc_subs_data.rs1_param3);
  for (size_t sub_idx = 0; sub_idx < num_subs; sub_idx++) {
    const ran_param_data_t data = *(const ran_param_data_t *)seq_arr_at(&rc_subs_data.rs1_param3, sub_idx);
    check_rrc_msg_copy(nr_channel, rrc_msg_id, rrc_ba, data.ric_req_id, &data.ev_tr.frmt_1);
  }

  pthread_mutex_unlock(&rc_mutex);
}

static void free_aperiodic_subscription(uint32_t ric_req_id)
{
  remove_rc_subs_data(&rc_subs_data, ric_req_id);
}

static seq_arr_t *get_sa(const e2sm_rc_event_trigger_t *ev_tr, const uint32_t ran_param_id)
{
  seq_arr_t *sa = NULL;

  switch (ev_tr->format) {
    case FORMAT_1_E2SM_RC_EV_TRIGGER_FORMAT:
      if (ran_param_id == E2SM_RC_RS1_RRC_MESSAGE) {
        sa = &rc_subs_data.rs1_param3;
      } else if (ran_param_id == E2SM_RC_RS1_UE_ID) {
        sa = &rc_subs_data.rs1_param4;
      }
      break;

    case FORMAT_4_E2SM_RC_EV_TRIGGER_FORMAT:
      if (ran_param_id == E2SM_RC_RS4_RRC_STATE_CHANGED_TO) {
        sa = &rc_subs_data.rs4_param202;
      }
      break;

    default:
      printf("[E2 AGENT] RC REPORT Style %d not yet implemented.\n", ev_tr->format + 1);
      break;
  }

  return sa;
}

static void get_list_for_report_style(const uint32_t ric_req_id, const e2sm_rc_event_trigger_t *ev_tr, const size_t sz, const param_report_def_t *param_def)
{
  for (size_t i = 0; i < sz; i++) {
    seq_arr_t *sa = get_sa(ev_tr, param_def[i].ran_param_id);
    if (!sa) {
      printf("[E2 AGENT] Requested RAN Parameter ID %d not yet implemented", param_def[i].ran_param_id);
    } else {
      struct ran_param_data data = { .ric_req_id = ric_req_id, .ev_tr = cp_e2sm_rc_event_trigger(ev_tr) };
      insert_rc_subs_data(sa, &data);
    }
  }
}

sm_ag_if_ans_t write_subs_rc_sm(void const* src)
{
  assert(src != NULL); // && src->type == RAN_CTRL_SUBS_V1_03);
  wr_rc_sub_data_t* wr_rc = (wr_rc_sub_data_t*)src;
  assert(wr_rc->rc.ad != NULL && "Cannot be NULL");

  sm_ag_if_ans_t ans = {0};

  const uint32_t ric_req_id = wr_rc->ric_req_id;
  const uint32_t report_style = wr_rc->rc.ad->ric_style_type;
  // 9.2.1.2  RIC ACTION DEFINITION IE
  switch (wr_rc->rc.ad->format) {
    case FORMAT_1_E2SM_RC_ACT_DEF: { // for all REPORT styles
      // Parameters to be Reported List
      // [1-65535]
      if (wr_rc->rc.et.format + 1 != report_style) { // wr_rc->rc.et.format is an enum -> initialization starts from 0
        AssertError(false, return ans, "[E2 AGENT] Event Trigger Definition Format %d doesn't correspond to REPORT style %d.\n", wr_rc->rc.et.format + 1, report_style);
      }
      /* RAN Parameter IDs from `On Demand` Report Style 5 are not included in rc_subs_data_t.
       * Its intention is to send immeditately RIC Indication Message upon RIC Subscription reception. */
      if (report_style != 5)
        get_list_for_report_style(ric_req_id, &wr_rc->rc.et, wr_rc->rc.ad->frmt_1.sz_param_report_def, wr_rc->rc.ad->frmt_1.param_report_def);
      break;
    }

    default:
      AssertError(wr_rc->rc.ad->format == FORMAT_1_E2SM_RC_ACT_DEF, return ans, "[E2 AGENT] Action Definition Format %d not yet implemented", wr_rc->rc.ad->format + 1);
  }

  ans.type = SUBS_OUTCOME_SM_AG_IF_ANS_V0;
  ans.subs_out.type = APERIODIC_SUBSCRIPTION_FLRC;
  ans.subs_out.aper.free_aper_subs = free_aperiodic_subscription;
  ans.subs_out.aper.imm_ind_data = NULL;

  // REPORT Service Style 5 ("On Demand Report"): report immediately. The
  // actual send is deferred to the caller (after this subscription is fully
  // registered with the agent) - it can't happen here, since we are still
  // inside on_subscription and the agent hasn't registered ric_req_id yet.
  if (report_style == 5)
    ans.subs_out.aper.imm_ind_data = fill_context_info_on_demand(&wr_rc->rc.ad->frmt_1);

  return ans;
}

/* Radio Bearer Control, 7.6.2. Decodes and reports the requested QoS flow
 * mapping; applying it is not implemented. */
static void write_ctrl_radio_bearer(rc_ctrl_req_data_t const* ctrl)
{
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
}

#ifdef NGRAN_GNB_CUCP
/** @brief One member of a STRUCTURE RAN parameter value, by RAN parameter ID */
static const seq_ran_param_t* strct_member(const seq_ran_param_t* param, uint32_t id, const char* what)
{
  if (param->ran_param_val.type != STRUCTURE_RAN_PARAMETER_VAL_TYPE) {
    printf("[E2 AGENT] RC Control: %s is not a structure\n", what);
    return NULL;
  }

  const ran_param_struct_t* strct = param->ran_param_val.strct;
  for (size_t i = 0; i < strct->sz_ran_param_struct; i++)
    if (strct->ran_param_struct[i].ran_param_id == id)
      return &strct->ran_param_struct[i];

  printf("[E2 AGENT] RC Control: %s has no member with RAN Parameter ID %u\n", what, id);
  return NULL;
}

static uint64_t nr_cgi_cell_id(const byte_array_t* nr_cgi)
{
  /* NR CGI is 3 octets of PLMN identity followed by the 36-bit NR Cell Identity
   * left-aligned in 5 octets (TS 38.413 9.3.1.7). */
  const size_t NR_CGI_LEN = 8;
  if (nr_cgi->buf == NULL || nr_cgi->len != NR_CGI_LEN) {
    printf("[E2 AGENT] RC Control: NR CGI is %zu octets, expected %zu\n", nr_cgi->len, NR_CGI_LEN);
    return 0;
  }

  const uint8_t* b = nr_cgi->buf;
  return ((uint64_t)b[3] << 28) | ((uint64_t)b[4] << 20) | ((uint64_t)b[5] << 12) | ((uint64_t)b[6] << 4) | ((uint64_t)b[7] >> 4);
}

/* Connected mode mobility control, 7.6.4: hand the UE over to the cell the RIC
 * names in Target Primary Cell ID. */
static void write_ctrl_conn_mode_mobility(rc_ctrl_req_data_t const* ctrl)
{
  if (ctrl->hdr.frmt_1.ctrl_act_id != HANDOVER_CONTROL_7_6_4_1) {
    printf("[E2 AGENT] RC Control: Style 3 Action %d not supported, only Handover Control\n", ctrl->hdr.frmt_1.ctrl_act_id);
    return;
  }

  if (ctrl->hdr.frmt_1.ue_id.type != GNB_UE_ID_E2SM) {
    printf("[E2 AGENT] RC Control: %d not supported for handover.\n", ctrl->hdr.frmt_1.ue_id.type);
    return;
  }
  if (ctrl->hdr.frmt_1.ue_id.gnb.ran_ue_id == NULL) {
    printf("[E2 AGENT] RC Control: RAN UE ID not found. Aborting handover.\n");
    return;
  }
  uint32_t rrc_ue_id = *ctrl->hdr.frmt_1.ue_id.gnb.ran_ue_id;
  rrc_gNB_ue_context_t *ue_context_p = rrc_gNB_get_ue_context(RC.nrrrc[0], rrc_ue_id);
  if (ue_context_p == NULL) {
    printf("[E2 AGENT] RC Control: UE with rrc_ue_id %d not found. Aborting handover.\n", rrc_ue_id);
    return;
  }
  gNB_RRC_UE_t *ue = &ue_context_p->ue_context;

  if (ctrl->msg.frmt_1.sz_ran_param < 1) {
    printf("[E2 AGENT] RC Control: Handover Control carries no RAN parameter\n");
    return;
  }

  const seq_ran_param_t* target = &ctrl->msg.frmt_1.ran_param[0];
  if (target->ran_param_id != TARGET_PRIMARY_CELL_ID_8_4_4_1) {
    printf("[E2 AGENT] RC Control: expected Target Primary Cell ID, got RAN Parameter ID %u\n", target->ran_param_id);
    return;
  }

  /* Target Primary Cell ID > CHOICE Target Cell > NR Cell ID > NR CGI. Only the
   * NR branch of the CHOICE is advertised, so only it is accepted here. */
  const seq_ran_param_t* choice = strct_member(target, CHOICE_TARGET_CELL_8_4_4_1, "Target Primary Cell ID");
  if (choice == NULL)
    return;

  const seq_ran_param_t* nr_cell = strct_member(choice, NR_CELL_8_4_4_1, "CHOICE Target Cell");
  if (nr_cell == NULL) {
    printf("[E2 AGENT] RC Control: only the NR branch of CHOICE Target Cell is supported\n");
    return;
  }

  const seq_ran_param_t* nr_cgi = strct_member(nr_cell, NR_CGI_8_4_4_1, "NR Cell ID");
  if (nr_cgi == NULL)
    return;

  if (nr_cgi->ran_param_val.type != ELEMENT_KEY_FLAG_FALSE_RAN_PARAMETER_VAL_TYPE
      || nr_cgi->ran_param_val.flag_false->type != OCTET_STRING_RAN_PARAMETER_VALUE) {
    printf("[E2 AGENT] RC Control: NR CGI is not an octet string element\n");
    return;
  }

  uint64_t target_cell_id = nr_cgi_cell_id(&nr_cgi->ran_param_val.flag_false->octet_str_ran);
  nr_rrc_cell_container_t *source_cell = rrc_get_pcell_for_ue(RC.nrrrc[0], ue);
  if (source_cell == NULL) {
    printf("[E2 AGENT] RC Control: cannot get source cell for UE %u\n", rrc_ue_id);
    return;
  }
  nr_rrc_cell_container_t *target_cell = get_cell_by_cell_id(&RC.nrrrc[0]->cells, target_cell_id);
  if (target_cell != NULL) {
    printf("[E2 AGENT] RC Control: F1 Handover Control for UE %u to NR Cell Identity %lu\n", rrc_ue_id, target_cell_id);
    nr_rrc_trigger_f1_ho(RC.nrrrc[0], ue, source_cell, target_cell);
    return;
  }
  const neighbour_cell_configuration_t *cell_cfg = get_neighbour_cell_config(RC.nrrrc[0], source_cell->info.cell_id);
  const nr_neighbour_cell_t *neighbour = cell_cfg != NULL ? get_neighbour_cell_by_cell_id(cell_cfg, target_cell_id) : NULL;
  if (neighbour == NULL) {
    printf("[E2 AGENT] RC Control: target NR Cell Identity %lu unknown. Cannot do neither F1 nor N2 handover for UE %u\n",
          target_cell_id,
          rrc_ue_id);
    return;
  }

  printf("[E2 AGENT] RC Control: N2 Handover Control for UE %u to neighbour NR Cell Identity %lu\n", rrc_ue_id, target_cell_id);
  nr_rrc_trigger_n2_ho(RC.nrrrc[0], ue, neighbour);
}
#endif

sm_ag_if_ans_t write_ctrl_rc_sm(void const* data)
{
  assert(data != NULL);

  rc_ctrl_req_data_t const* ctrl = (rc_ctrl_req_data_t const*)data;

  assert(ctrl->hdr.format == FORMAT_1_E2SM_RC_CTRL_HDR && "Indication Header Format received not valid");
  assert(ctrl->msg.format == FORMAT_1_E2SM_RC_CTRL_MSG && "Indication Message Format received not valid");

  /* A CONTROL comes from the RIC, so everything below the format is input, not
   * an invariant: report what cannot be honoured and leave the RAN running. */
  switch (ctrl->hdr.frmt_1.ric_style_type) {
    case RC_CTRL_STYLE_RADIO_BEARER:
      write_ctrl_radio_bearer(ctrl);
      break;

    case RC_CTRL_STYLE_CONN_MODE_MOBILITY:
 #ifdef NGRAN_GNB_CUCP
      write_ctrl_conn_mode_mobility(ctrl);
 #else
      printf("[E2 AGENT] \"Connected Mode Mobility Control\" not supported by %d.\n", get_e2_node_type());
 #endif
      break;

    default:
      printf("[E2 AGENT] RC Control Style %d not supported\n", ctrl->hdr.frmt_1.ric_style_type);
      break;
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
