#include <openair3/NAS/NR_UE/nr_user_def.h>
#include <openair3/NAS/COMMON/milenage.h>
#include <openair3/NAS/gNB/network_process_nas.h>

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
      case SGSmobilitymanagementmessages:
        switch (header->mt) {
          case Registrationrequest:
            SGSregistrationReq(msg, UE);
            break;

          case DeregistrationrequestUEoriginating:
            SGSderegistrationUEReq(msg, UE);
            break;

          case Authenticationresponse:
            SGSauthenticationResp(msg, UE);
            break;

          case Identityresponse:
            SGSidentityResp(msg, UE);
            break;

          case Securitymodecomplete:
            SGSsecurityModeComplete(msg, UE);
            break;

          case Registrationcomplete:
            SGSregistrationComplete(msg, UE);
            break;

          default:
            SGSabortUE(msg, UE);
        }

        break;

      case SGSsessionmanagementmessages:
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

int identityRequest(void **msg, NRUEcontext_t *UE) {
  myCalloc(req, Identityrequest_t);
  req->epd=SGSmobilitymanagementmessages;
  req->sh=0;
  req->mt=Identityrequest;
  req->it=SUCI;
  *msg=req;
  return sizeof(Identityrequest_t);
}

int authenticationRequest(void **msg, NRUEcontext_t *UE) {
  if (UE->uicc == NULL)
    // config file section hardcoded as "uicc", nevertheless it opens to manage several UEs or a multi SIM UE
    UE->uicc=init_uicc("uicc");

  myCalloc(req, authenticationrequestHeader_t);
  req->epd=SGSmobilitymanagementmessages;
  req->sh=0;
  req->mt=Authenticationrequest;
  // native security context => bit 4 to 0
  // probably from TS 33.501, table A-8.1
  // N-NAS-int-alg (Native NAS integrity)
  /*
     N-NAS-enc-alg 0x01
     N-NAS-int-alg 0x02
     N-RRC-enc-alg 0x03
     N-RRC-int-alg 0x04
     N-UP-enc-alg 0x05
     N-UP-int-alg 0x06
  */
  req->ngKSI=2;
  // TS 33.501, Annex A.7.1: Initial set of security features defined for 5GS.
  req->ABBALen=2;
  req->ABBA=0;
  //rand (TV)
  req->ieiRAND=IEI_RAND;
  FILE *h=fopen("/dev/random","r");

  if ( sizeof(req->RAND) != fread(req->RAND,1,sizeof(req->RAND),h) )
    LOG_E(NAS, "can't read /dev/random\n");

  fclose(h);
  // challenge/AUTN (TLV)
  req->ieiAUTN=IEI_AUTN;
  req->AUTNlen=sizeof(req->AUTN);
  uint8_t ik[16], ck[16], res[8];
  milenage_generate(UE->uicc->opc, UE->uicc->amf, UE->uicc->key,
                    UE->uicc->sqn, req->RAND, req->AUTN, ik, ck, res);
  // EAP message (TLV-E)
  // not developped
  *msg=req;
  return sizeof(authenticationrequestHeader_t);
}

int securityModeCommand(void **msg, NRUEcontext_t *UE) {
  *msg=NULL;
  return 0;
}
