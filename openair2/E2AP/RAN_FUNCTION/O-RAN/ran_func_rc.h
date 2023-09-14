#ifndef RAN_FUNC_SM_RAN_CTRL_READ_WRITE_AGENT_H
#define RAN_FUNC_SM_RAN_CTRL_READ_WRITE_AGENT_H

#include "openair2/E2AP/flexric/src/agent/../sm/sm_io.h"

void read_rc_sm(void *);

void read_rc_setup_sm(void* data);

sm_ag_if_ans_t write_ctrl_rc_sm(void const* data);

sm_ag_if_ans_t write_subs_rc_sm(void const* src);

#endif

