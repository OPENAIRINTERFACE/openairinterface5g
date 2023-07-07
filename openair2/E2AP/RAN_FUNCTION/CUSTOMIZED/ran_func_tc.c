#include "ran_func_tc.h"
#include "../../flexric/test/rnd/fill_rnd_data_tc.h"
#include <assert.h>

void read_tc_sm(void* data)
{
  assert(data != NULL);
  //assert(data->type == TC_STATS_V0);

  tc_ind_data_t* tc = (tc_ind_data_t*)data;
  fill_tc_ind_data(tc);
}

void read_tc_setup_sm(void* data)
{
  assert(data != NULL);
//  assert(data->type == TC_AGENT_IF_E2_SETUP_ANS_V0 );

  assert(0 !=0 && "Not supported");
}

sm_ag_if_ans_t write_ctrl_tc_sm(void const* data)
{
  assert(data != NULL);
//  assert(data->type == TC_CTRL_REQ_V0 );

  tc_ctrl_req_data_t const* ctrl = (tc_ctrl_req_data_t const*)data;

  tc_ctrl_msg_e const t = ctrl->msg.type;

  assert(t == TC_CTRL_SM_V0_CLS || t == TC_CTRL_SM_V0_PLC 
      || t == TC_CTRL_SM_V0_QUEUE || t ==TC_CTRL_SM_V0_SCH 
      || t == TC_CTRL_SM_V0_SHP || t == TC_CTRL_SM_V0_PCR);

  sm_ag_if_ans_t ans = {.type = CTRL_OUTCOME_SM_AG_IF_ANS_V0};
  ans.ctrl_out.type = TC_AGENT_IF_CTRL_ANS_V0;
  return ans;
}

