/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Author and copyright: Laurent Thomas, open-cells.com
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */
#include <openair3/NAS/COMMON/NR_NAS_defs.h>
#include <openair3/SECU/secu_defs.h>


void SGSabortNet(void *msg, nr_user_nas_t *UE) {
}

void ue_nas_schedule(void) {
}

/*
 *Message reception
 */

void SGSauthenticationReq(void *msg, nr_user_nas_t *UE) {
  authenticationrequestHeader_t *amsg=(authenticationrequestHeader_t *) msg;
  arrayCpy(UE->uicc->rand,amsg->RAND);
  arrayCpy(UE->uicc->autn,amsg->AUTN);
  // AUTHENTICATION REQUEST message that contains a valid ngKSI, SQN and MAC is received
  // TBD verify ngKSI (we set it as '2', see gNB code)
  // SQN and MAC are tested in auth resp processing
  ue_nas_schedule();
}

void SGSidentityReq(void *msg, nr_user_nas_t *UE) {
  Identityrequest_t *idmsg=(Identityrequest_t *) msg;

  if (idmsg->it == SUCI ) {
    LOG_I(NAS,"Received Identity request, scheduling answer\n");
    ue_nas_schedule();
  } else
    LOG_E(NAS,"Not developped: identity request for %d\n", idmsg->it);
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
        LOG_I(NAS,"Received message: %s\n", idStr(message_text_info, header->mt));

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
  int imsiL=strlen(UE->uicc->imsiStr);
  int msinL=imsiL-3-UE->uicc->nmc_size;
  int respSize=sizeof(IdentityresponseIMSI_t) + (msinL+1)/2;
  IdentityresponseIMSI_t *resp=(IdentityresponseIMSI_t *) calloc(respSize,1);
  resp->common.epd=SGSmobilitymanagementmessages;
  resp->common.sh=0;
  resp->common.mt=Identityresponse;
  resp->common.len=htons(respSize-sizeof(Identityresponse_t));
  resp->mi=SUCI;
  resp->mcc1=UE->uicc->imsiStr[0]-'0';
  resp->mcc2=UE->uicc->imsiStr[1]-'0';
  resp->mcc3=UE->uicc->imsiStr[2]-'0';
  resp->mnc1=UE->uicc->imsiStr[3]-'0';
  resp->mnc2=UE->uicc->imsiStr[4]-'0';
  resp->mnc3=UE->uicc->nmc_size==2? 0xF : UE->uicc->imsiStr[3]-'0';
  // TBD: routing to fill (FF ?)
  char *out=(char *)(resp+1);
  char *ptr=UE->uicc->imsiStr + 3 + UE->uicc->nmc_size;

  while ( ptr < UE->uicc->imsiStr+strlen(UE->uicc->imsiStr) ) {
    *out=((*(ptr+1)-'0')<<4) | (*(ptr) -'0');
    out++;
    ptr+=2;
  }

  if (msinL%2 == 1)
    *out=((*(ptr-1)-'0')) | 0xF0;

  *msg=resp;
  return respSize;
}

int authenticationResponse(void **msg,nr_user_nas_t *UE) {
  if (UE->uicc == NULL)
    // config file section hardcoded as "uicc", nevertheless it opens to manage several UEs or a multi SIM UE
    UE->uicc=init_uicc("uicc");

  myCalloc(resp, authenticationresponse_t);
  resp->epd=SGSmobilitymanagementmessages;
  resp->sh=0;
  resp->mt=Authenticationresponse;
  resp->iei=IEI_AuthenticationResponse;
  resp->RESlen=sizeof(resp->RES); // always 16 see TS 24.501 Table 8.2.2.1.1
  // Verify the AUTN
  // a full implementation need to test several SQN
  // as the last 5bits can be any value
  // and the value can also be greater and accepted (if it is in the accepted window)
  uint8_t AUTN[16];
  uicc_milenage_generate(AUTN, UE->uicc);

  if ( memcmp(UE->uicc->autn, AUTN, sizeof(AUTN)) == 0 ) {
    // prepare and send good answer
    resToresStar(resp->RES,UE->uicc);
  } else {
    // prepare and send autn is not compatible with our data
    abort();
  }

  *msg=resp;
  return sizeof(authenticationresponse_t);
}

int securityModeComplete(void **msg, nr_user_nas_t *UE) {
  return -1;
}

int registrationComplete(void **msg, nr_user_nas_t *UE) {
  return -1;
}
