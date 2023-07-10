#ifndef RAN_FUNC_SM_RLC_READ_WRITE_AGENT_H
#define RAN_FUNC_SM_RLC_READ_WRITE_AGENT_H

#include "../../flexric/src/agent/e2_agent_api.h"

void read_rlc_sm(void*);

void read_rlc_setup_sm(void* data);

sm_ag_if_ans_t write_ctrl_rlc_sm(void const* data);

#endif

