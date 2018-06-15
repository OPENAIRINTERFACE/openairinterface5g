
#ifndef __PROTO_AGENT_RLC_PRIMITIVES_H__
#define __PROTO_AGENT_RLC_PRIMITIVES_H__

#include "RRC/LTE/rrc_defs.h"
#include "LAYER2/PROTO_AGENT/proto_agent.h"
// PROTO AGENT
pthread_t async_server_thread;
int async_server_thread_finalize (void);
void async_server_thread_init (void);

pthread_mutex_t async_server_lock;
pthread_cond_t async_server_notify;
int async_server_shutdown;

#endif
