#include "ran_func_rlc.h"
#include "../../flexric/test/rnd/fill_rnd_data_rlc.h"

void read_rlc_sm(void* data)
{
  assert(data != NULL);
//  assert(data->type ==  RLC_STATS_V0);

  rlc_ind_data_t* rlc = (rlc_ind_data_t*)data;
  fill_rlc_ind_data(rlc);
}

void read_rlc_setup_sm(void* data)
{
  assert(data != NULL);
//  assert(data->type == RLC_AGENT_IF_E2_SETUP_ANS_V0 );
  assert(0 !=0 && "Not supported");
}

sm_ag_if_ans_t write_ctrl_rlc_sm(void const* data)
{
  (void)data;
  assert(0!=0 && "Not supported");
}

