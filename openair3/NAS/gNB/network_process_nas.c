#include <openair2/LAYER2/NR_MAC_gNB/nr_mac_gNB.h>

void SGSabortUE(void *msg, NRUEcontext_t *UE) {
}


/*
 *Message reception
 */

void SGSregistrationReq(void *msg, NRUEcontext_t *UE) {
}

void SGSderegistrationUEReq(void *msg, NRUEcontext_t *UE) {
}

void SGSauthenticationResp(void *msg, NRUEcontext_t *UE) {
}

void SGSidentityResp(void *msg, NRUEcontext_t *UE) {
}

void SGSsecurityModeComplete(void *msg, NRUEcontext_t *UE) {
}

void SGSregistrationComplete(void *msg, NRUEcontext_t *UE) {
}

void processNAS(void *msg, NRUEcontext_t *UE) {
  SGScommonHeader_t *header=(SGScommonHeader_t *) msg;

  if ( header->sh > 4 )
    SGSabortUE(msg, UE);
  else {
    switch  (header->epd) {
SGSmobilitymanagementmessages:

        switch (header->mt) {
Registrationrequest:
            SGSregistrationReq(msg, UE);
            break;
DeregistrationrequestUEoriginating:
            SGSderegistrationUEReq(msg, UE);
            break;
Authenticationresponse:
            SGSauthenticationResp(msg, UE);
            break;
Identityresponse:
            SGSidentityResp(msg, UE);
            break;
Securitymodecomplete:
            SGSsecurityModeComplete(msg, UE);
            break;
Registrationcomplete:
            SGSregistrationComplete(msg, UE);
            break;

          default:
            SGSabortUE(msg, UE);
        }

        break;
SGSsessionmanagementmessages:
        SGSabortUE(msg, UE);
        break;

      default:
        SGSabortUE(msg, UE);
    }
  }
}

/*
 * Messages emission
 */

void identityRequest(void *msg, NRUEcontext_t *UE) {
}

void authenticationRequest(void *msg, NRUEcontext_t *UE) {
}

void securityModeCommand(void *msg, NRUEcontext_t *UE) {
}
