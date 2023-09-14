#ifndef RAN_FUNC_SM_PDCP_READ_WRITE_AGENT_H
#define RAN_FUNC_SM_PDCP_READ_WRITE_AGENT_H

#include "openair2/E2AP/flexric/src/agent/../sm/sm_io.h"
#include "common/ran_context.h"
#include "openair2/RRC/NR/rrc_gNB_UE_context.h"
#include "openair2/LAYER2/nr_pdcp/nr_pdcp_oai_api.h"
#include "openair2/LAYER2/nr_rlc/nr_rlc_oai_api.h"
#include "openair2/LAYER2/NR_MAC_gNB/mac_proto.h"
#include "openair2/E2AP/flexric/src/util/time_now_us.h"

void read_pdcp_sm(void*);

void read_pdcp_setup_sm(void* data);

sm_ag_if_ans_t write_ctrl_pdcp_sm(void const* data);

#endif

