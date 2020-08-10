#include <openair2/LAYER2/NR_MAC_gNB/nr_mac_gNB.h>
#include <openair3/NAS/NR_UE/nr_user_def.h>

void SGSabortNet(void *msg, nr_user_nas_t *UE) {
}


/*
 *Message reception
 */



void SGSauthenticationReq(void *msg, nr_user_nas_t *UE) {
}

void SGSidentityReq(void *msg, nr_user_nas_t *UE) {
}

void SGSsecurityModeCommand(void *msg, nr_user_nas_t *UE) {
}


void UEprocessNAS(void *msg,nr_user_nas_t *UE) {
  SGScommonHeader_t *header=(SGScommonHeader_t *) msg;

  if ( header->sh > 4 )
    SGSabortNet(msg, UE);
  else {
    switch  (header->epd) {
SGSmobilitymanagementmessages:

        switch (header->mt) {
Authenticationrequest:
            SGSauthenticationReq(msg, UE);
            break;
Identityrequest:
            SGSidentityReq(msg, UE);
            break;
Securitymodecommand:
            SGSsecurityModeCommand(msg, UE);
            break;

          default:
            SGSabortNet(msg, UE);
        }

        break;
SGSsessionmanagementmessages:
        SGSabortNet(msg, UE);
        break;

      default:
        SGSabortNet(msg, UE);
    }
  }
}

/*
 * Messages emission
 */

void identityResponse(void *msg, nr_user_nas_t *UE) {
}

void authenticationResponse(void *msg,nr_user_nas_t *UE) {
}

void securityModeComplete(void *msg, nr_user_nas_t *UE) {
}

void registrationComplete(void *msg, nr_user_nas_t *UE) {
}
