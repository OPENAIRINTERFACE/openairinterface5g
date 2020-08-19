#ifndef NET_PROCESS_NAS_H
#define NET_PROCESS_NAS_H
#include <openair2/LAYER2/NR_MAC_gNB/nr_mac_gNB.h>

void SGSabortUE(void *msg, NRUEcontext_t *UE) ;
void SGSregistrationReq(void *msg, NRUEcontext_t *UE);
void SGSderegistrationUEReq(void *msg, NRUEcontext_t *UE);
void SGSauthenticationResp(void *msg, NRUEcontext_t *UE);
void SGSidentityResp(void *msg, NRUEcontext_t *UE);
void SGSsecurityModeComplete(void *msg, NRUEcontext_t *UE);
void SGSregistrationComplete(void *msg, NRUEcontext_t *UE);
void processNAS(void *msg, NRUEcontext_t *UE);
int identityRequest(void **msg, NRUEcontext_t *UE);
int authenticationRequest(void **msg, NRUEcontext_t *UE);
int securityModeCommand(void **msg, NRUEcontext_t *UE);
#endif
