#include <openair2/LAYER2/NR_MAC_gNB/nr_mac_gNB.h>
#include <openair3/NAS/NR_UE/nr_user_def.h>


void SGSabortNet(void *msg, nr_user_nas_t *UE) {
}

void nas_schedule(void) {
}

/*
 *Message reception
 */

void SGSauthenticationReq(void *msg, nr_user_nas_t *UE) {
  Identityrequest_t *idmsg=(Identityrequest_t *) msg;

  if (idmsg->it == SUCI ) {
    LOG_I(NAS,"Received Identity request, scheduling answer\n");
    nas_schedule();
  } else
    LOG_E(NAS,"Not developped: identity request for %d\n", idmsg->it);
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
      case SGSmobilitymanagementmessages:
        switch (header->mt) {
          case Authenticationrequest:
            SGSauthenticationReq(msg, UE);
            break;

          case Identityrequest:
            SGSidentityReq(msg, UE);
            break;

          case Securitymodecommand:
            SGSsecurityModeCommand(msg, UE);
            break;

          default:
            SGSabortNet(msg, UE);
        }

        break;

      case SGSsessionmanagementmessages:
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

int identityResponse(void **msg, nr_user_nas_t *UE) {
  if (UE->uicc == NULL)
    // config file section hardcoded as "uicc", nevertheless it opens to manage several UEs or a multi SIM UE
    UE->uicc=init_uicc("uicc");

  // TS 24.501 9.11.3.4
  int imsiL=strlen(UE->uicc->imsi);
  int msinL=imsiL-3-UE->uicc->nmc_size;
  int respSize=sizeof(IdentityresponseIMSI_t) + (msinL+1)/2;
  IdentityresponseIMSI_t *resp=(IdentityresponseIMSI_t *) calloc(respSize,1);
  resp->common.epd=SGSmobilitymanagementmessages;
  resp->common.sh=0;
  resp->common.mt=Identityresponse;
  resp->common.len=htons(respSize-sizeof(Identityresponse_t));
  resp->mi=SUCI;
  resp->mcc1=UE->uicc->imsi[0]-'0';
  resp->mcc2=UE->uicc->imsi[1]-'0';
  resp->mcc3=UE->uicc->imsi[2]-'0';
  resp->mnc1=UE->uicc->imsi[3]-'0';
  resp->mnc2=UE->uicc->imsi[4]-'0';
  resp->mnc3=UE->uicc->nmc_size==2? 0xF : UE->uicc->imsi[3]-'0';
  // TBD: routing to fill (FF ?)
  char *out=(char *)(resp+1);
  char *ptr=UE->uicc->imsi + 3 + UE->uicc->nmc_size;

  while ( ptr < UE->uicc->imsi+strlen(UE->uicc->imsi) ) {
    *out=((*(ptr+1)-'0')<<4) | (*(ptr) -'0');
    out++;
    ptr+=2;
  }

  if (msinL%2 == 1)
    *out=((*(ptr-1)-'0')) | 0xF0;

  *msg=resp;
  log_dump(NAS, resp,  respSize, LOG_DUMP_CHAR, "\n");
  return respSize;
}

int authenticationResponse(void **msg,nr_user_nas_t *UE) {
  return -1;
}

int securityModeComplete(void **msg, nr_user_nas_t *UE) {
  return -1;
}

int registrationComplete(void **msg, nr_user_nas_t *UE) {
  return -1;
}
