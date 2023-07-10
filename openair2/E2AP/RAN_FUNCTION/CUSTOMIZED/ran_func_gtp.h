#ifndef RAN_FUNC_SM_GTP_READ_WRITE_AGENT_H
#define RAN_FUNC_SM_GTP_READ_WRITE_AGENT_H

#include "../../flexric/src/agent/e2_agent_api.h"

void read_gtp_sm(void*);

void read_gtp_setup_sm(void*);

sm_ag_if_ans_t write_ctrl_gtp_sm(void const*);

#endif

