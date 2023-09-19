#ifndef RAN_FUNC_SM_TC_READ_WRITE_AGENT_H
#define RAN_FUNC_SM_TC_READ_WRITE_AGENT_H

#include "openair2/E2AP/flexric/src/agent/../sm/sm_io.h"

void read_tc_sm(void*);

void read_tc_setup_sm(void* data);

sm_ag_if_ans_t write_ctrl_tc_sm(void const* data);

#endif

