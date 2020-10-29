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

void SGSabortUE(void *msg, NRUEcontext_t *UE) {
}
void nas_schedule(void) {
}
/*
 *Message reception
 */

void SGSregistrationReq(void *msg, NRUEcontext_t *UE) {
}

void SGSderegistrationUEReq(void *msg, NRUEcontext_t *UE) {
}

void SGSidentityResp(void *msg, NRUEcontext_t *UE) {
  // use the UE SUPI to load the USIM
  IdentityresponseIMSI_t *resp=(IdentityresponseIMSI_t *)msg;
  char configName[100]={0};
  char * ptr= stpcpy(configName,"uicc");
  *ptr++=resp->mcc1+'0';
  *ptr++=resp->mcc2+'0';
  *ptr++=resp->mcc3+'0';
  *ptr++=resp->mnc1+'0';
  *ptr++=resp->mnc2+'0';
  if ( resp->mnc3 != 0xF )
    *ptr++=resp->mnc3 +'0';
  int respSize=ntohs(resp->common.len);
  int msinByteSize=respSize-sizeof(IdentityresponseIMSI_t);
  uint8_t * msin=(uint8_t *) (resp+1);
  for(int i=3 ; i<msinByteSize; i++) {
    char digit1=(msin[i] >> 4) ;
    *ptr++=(msin[i] & 0xf) +'0';
    if (digit1 != 0xf)
      *ptr++=digit1 +'0';
  }
  *ptr=0;
  if (UE->uicc == NULL)
    // config file section hardcoded as "uicc", nevertheless it opens to manage several UEs or a multi SIM UE
    UE->uicc=init_uicc(configName);
  //We schedule Authentication request
  nas_schedule();
}

void SGSauthenticationResp(void *msg, NRUEcontext_t *UE) {
  // Let's check the answer
  authenticationresponse_t* resp=(authenticationresponse_t*) msg;
  uint8_t resStarLocal[16];
  resToresStar(resStarLocal,UE->uicc);
  if ( memcmp(resStarLocal, resp->RES, sizeof(resStarLocal)) == 0 ) {
    LOG_I(NAS,"UE answer to authentication resquest is correct\n");
    // we will send securityModeCommand
    nas_schedule();
  } else {
    LOG_E(NAS,"UE authentication response not good\n");
    // To be implemented ?
  }
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
        LOG_I(NAS,"Received message: %s\n", idStr(message_text_info, header->mt));

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
  memcpy(UE->uicc->rand, req->RAND, sizeof(UE->uicc->rand));
  // challenge/AUTN (TLV)
  req->ieiAUTN=IEI_AUTN;
  req->AUTNlen=sizeof(req->AUTN);
  uicc_milenage_generate( req->AUTN, UE->uicc);
  // EAP message (TLV-E)
  // not developped
  *msg=req;
  return sizeof(authenticationrequestHeader_t);
}

int securityModeCommand(void **msg, NRUEcontext_t *UE) {
  *msg=NULL;
  myCalloc(req, securityModeCommand_t);
  req->epd=SGSmobilitymanagementmessages;
  req->sh=0;
  req->mt=Securitymodecommand;

  // integrity algo from 5G-IA0 (null algo) to 5G-IA7 in first 4 bits
  unsigned int ia=0;
  // cyphering algo from 5G-EA0 (null algo) to 5G-IA7 in MSB 4 bits
  unsigned int ea=0;
  // trace from commercial: 5G-IA0 (0) 128-5G-IA2 (2)
  req->selectedNASsecurityalgorithms= ea<<4 | ia;

  // KSI: N-NAS-int-alg 0x02
  req->ngKSI=2;
  
  *msg=req;
  return 0;
}
