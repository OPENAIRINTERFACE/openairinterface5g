#ifndef RAN_FUNC_SM_GTP_READ_WRITE_AGENT_H
#define RAN_FUNC_SM_GTP_READ_WRITE_AGENT_H

#include "openair2/E2AP/flexric/src/agent/../sm/sm_io.h"
#include "common/ran_context.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include "openair2/E2AP/flexric/src/util/time_now_us.h"
#include "openair2/RRC/NR/rrc_gNB_UE_context.h"

void read_gtp_sm(void*);

void read_gtp_setup_sm(void*);

sm_ag_if_ans_t write_ctrl_gtp_sm(void const*);

#endif

