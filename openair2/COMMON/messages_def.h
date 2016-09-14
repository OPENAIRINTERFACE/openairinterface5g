// These messages files are mandatory and must always be placed in first position
#include "intertask_messages_def.h"
#include "timer_messages_def.h"

// Messages files used between tasks
#include "phy_messages_def.h"
#include "mac_messages_def.h"
#include "rlc_messages_def.h"
#include "pdcp_messages_def.h"
#include "rrc_messages_def.h"
#include "nas_messages_def.h"
#if ENABLE_RAL
#include "ral_messages_def.h"
#endif
#include "s1ap_messages_def.h"
#include "x2ap_messages_def.h"
#include "sctp_messages_def.h"
#include "udp_messages_def.h"
#include "gtpv1_u_messages_def.h"

