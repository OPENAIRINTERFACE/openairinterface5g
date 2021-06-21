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
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

/*! \file nr_nas_msg_sim.c
 * \brief simulator for nr nas message
 * \author Yoshio INOUE, Masayuki HARADA
 * \email yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
 * \date 2020
 * \version 0.1
 */


#include <string.h> // memset
#include <stdlib.h> // malloc, free

#include "nas_log.h"
#include "TLVDecoder.h"
#include "TLVEncoder.h"
#include "nr_nas_msg_sim.h"
#include "aka_functions.h"
#include "secu_defs.h"
#include "PduSessionEstablishRequest.h"
#include "intertask_interface.h"
#include "openair2/RRC/NAS/nas_config.h"
#include <openair3/UICC/usim_interface.h>
#include <openair3/NAS/COMMON/NR_NAS_defs.h>
#include <openair1/PHY/phy_extern_nr_ue.h>

uint8_t  *registration_request_buf;
uint32_t  registration_request_len;
extern char *baseNetAddress;

static int nas_protected_security_header_encode(
  char                                       *buffer,
  const fgs_nas_message_security_header_t    *header,
  int                                         length)
{
  LOG_FUNC_IN;

  int size = 0;

  /* Encode the protocol discriminator) */
  ENCODE_U8(buffer, header->protocol_discriminator, size);

  /* Encode the security header type */
  ENCODE_U8(buffer+size, (header->security_header_type & 0xf), size);

  /* Encode the message authentication code */
  ENCODE_U32(buffer+size, header->message_authentication_code, size);
  /* Encode the sequence number */
  ENCODE_U8(buffer+size, header->sequence_number, size);

  LOG_FUNC_RETURN (size);
}

static int _nas_mm_msg_encode_header(const mm_msg_header_t *header,
                                  uint8_t *buffer, uint32_t len) {
  int size = 0;

  /* Check the buffer length */
  if (len < sizeof(mm_msg_header_t)) {
    return (TLV_ENCODE_BUFFER_TOO_SHORT);
  }

  /* Check the protocol discriminator */
  if (header->ex_protocol_discriminator != FGS_MOBILITY_MANAGEMENT_MESSAGE) {
    LOG_TRACE(ERROR, "ESM-MSG   - Unexpected extened protocol discriminator: 0x%x",
              header->ex_protocol_discriminator);
    return (TLV_ENCODE_PROTOCOL_NOT_SUPPORTED);
  }

  /* Encode the extendedprotocol discriminator */
  ENCODE_U8(buffer + size, header->ex_protocol_discriminator, size);
  /* Encode the security header type */
  ENCODE_U8(buffer + size, (header->security_header_type & 0xf), size);
  /* Encode the message type */
  ENCODE_U8(buffer + size, header->message_type, size);
  return (size);
}


int mm_msg_encode(MM_msg *mm_msg, uint8_t *buffer, uint32_t len) {
  LOG_FUNC_IN;
  int header_result;
  int encode_result;
  uint8_t msg_type = mm_msg->header.message_type;


  /* First encode the EMM message header */
  header_result = _nas_mm_msg_encode_header(&mm_msg->header, buffer, len);

  if (header_result < 0) {
    LOG_TRACE(ERROR, "EMM-MSG   - Failed to encode EMM message header "
              "(%d)", header_result);
    LOG_FUNC_RETURN(header_result);
  }

  buffer += header_result;
  len -= header_result;

  switch(msg_type) {
    case REGISTRATION_REQUEST:
      encode_result = encode_registration_request(&mm_msg->registration_request, buffer, len);
      break;
    case FGS_IDENTITY_RESPONSE:
      encode_result = encode_identiy_response(&mm_msg->fgs_identity_response, buffer, len);
      break;
    case FGS_AUTHENTICATION_RESPONSE:
      encode_result = encode_fgs_authentication_response(&mm_msg->fgs_auth_response, buffer, len);
      break;
    case FGS_SECURITY_MODE_COMPLETE:
      encode_result = encode_fgs_security_mode_complete(&mm_msg->fgs_security_mode_complete, buffer, len);
      break;
    case FGS_UPLINK_NAS_TRANSPORT:
      encode_result = encode_fgs_uplink_nas_transport(&mm_msg->uplink_nas_transport, buffer, len);
      break;
    default:
      LOG_TRACE(ERROR, "EMM-MSG   - Unexpected message type: 0x%x",
    		  mm_msg->header.message_type);
      encode_result = TLV_ENCODE_WRONG_MESSAGE_TYPE;
      break;
      /* TODO: Handle not standard layer 3 messages: SERVICE_REQUEST */
  }

  if (encode_result < 0) {
    LOG_TRACE(ERROR, "EMM-MSG   - Failed to encode L3 EMM message 0x%x "
              "(%d)", mm_msg->header.message_type, encode_result);
  }

  if (encode_result < 0)
    LOG_FUNC_RETURN (encode_result);

  LOG_FUNC_RETURN (header_result + encode_result);
}

void transferRES(uint8_t ck[16], uint8_t ik[16], uint8_t *input, uint8_t rand[16], uint8_t *output, uicc_t* uicc) {
  uint8_t S[100]={0};
  S[0] = 0x6B;
  servingNetworkName (S+1, uicc->imsiStr, uicc->nmc_size);
  int netNamesize = strlen((char*)S+1);
  S[1 + netNamesize] = (netNamesize & 0xff00) >> 8;
  S[2 + netNamesize] = (netNamesize & 0x00ff);
  for (int i = 0; i < 16; i++)
    S[3 + netNamesize + i] = rand[i];
  S[19 + netNamesize] = 0x00;
  S[20 + netNamesize] = 0x10;
  for (int i = 0; i < 8; i++)
    S[21 + netNamesize + i] = input[i];
  S[29 + netNamesize] = 0x00;
  S[30 + netNamesize] = 0x08;

  uint8_t plmn[3] = { 0x02, 0xf8, 0x39 };
  uint8_t oldS[100];
  oldS[0] = 0x6B;
  memcpy(&oldS[1], plmn, 3);
  oldS[4] = 0x00;
  oldS[5] = 0x03;
  for (int i = 0; i < 16; i++)
    oldS[6 + i] = rand[i];
  oldS[22] = 0x00;
  oldS[23] = 0x10;
  for (int i = 0; i < 8; i++)
    oldS[24 + i] = input[i];
  oldS[32] = 0x00;
  oldS[33] = 0x08;


  uint8_t key[32];
  memcpy(&key[0], ck, 16);
  memcpy(&key[16], ik, 16);  //KEY
  uint8_t out[32];
  kdf(key, 32, S, 31 + netNamesize, out, 32);
  for (int i = 0; i < 16; i++)
    output[i] = out[16 + i];
}

void derive_kausf(uint8_t ck[16], uint8_t ik[16], uint8_t sqn[6], uint8_t kausf[32], uicc_t *uicc) {
  uint8_t S[100]={0};
  uint8_t key[32];

  memcpy(&key[0], ck, 16);
  memcpy(&key[16], ik, 16);  //KEY
  S[0] = 0x6A;
  servingNetworkName (S+1, uicc->imsiStr, uicc->nmc_size);
  int netNamesize = strlen((char*)S+1);
  S[1 + netNamesize] = (uint8_t)((netNamesize & 0xff00) >> 8);
  S[2 + netNamesize] = (uint8_t)(netNamesize & 0x00ff);
  for (int i = 0; i < 6; i++) {
   S[3 + netNamesize + i] = sqn[i];
  }
  S[9 + netNamesize] = 0x00;
  S[10 + netNamesize] = 0x06;
  kdf(key, 32, S, 11 + netNamesize, kausf, 32);
}

void derive_kseaf(uint8_t kausf[32], uint8_t kseaf[32], uicc_t *uicc) {
  uint8_t S[100]={0};
  S[0] = 0x6C;  //FC
  servingNetworkName (S+1, uicc->imsiStr, uicc->nmc_size);
  int netNamesize = strlen((char*)S+1);
  S[1 + netNamesize] = (uint8_t)((netNamesize & 0xff00) >> 8);
  S[2 + netNamesize] = (uint8_t)(netNamesize & 0x00ff);
  kdf(kausf, 32, S, 3 + netNamesize, kseaf, 32);
}

void derive_kamf(uint8_t *kseaf, uint8_t *kamf, uint16_t abba, uicc_t* uicc) {
  int imsiLen = strlen(uicc->imsiStr);
  uint8_t S[100];
  S[0] = 0x6D;  //FC = 0x6D
  memcpy(&S[1], uicc->imsiStr, imsiLen );
  S[1 + imsiLen] = (uint8_t)((imsiLen & 0xff00) >> 8);
  S[2 + imsiLen] = (uint8_t)(imsiLen & 0x00ff);
  S[3 + imsiLen] = abba & 0x00ff;
  S[4 + imsiLen] = (abba & 0xff00) >> 8;
  S[5 + imsiLen] = 0x00;
  S[6 + imsiLen] = 0x02;
  kdf(kseaf, 32, S, 7 + imsiLen, kamf, 32);
}

//------------------------------------------------------------------------------
void derive_knas(algorithm_type_dist_t nas_alg_type, uint8_t nas_alg_id, uint8_t kamf[32], uint8_t *knas_int) {
  uint8_t S[20];
  uint8_t out[32] = { 0 };
  S[0] = 0x69;  //FC
  S[1] = (uint8_t)(nas_alg_type & 0xFF);
  S[2] = 0x00;
  S[3] = 0x01;
  S[4] = nas_alg_id;
  S[5] = 0x00;
  S[6] = 0x01;
  kdf(kamf, 32, S, 7, out, 32);
  for (int i = 0; i < 16; i++)
    knas_int[i] = out[16 + i];
}

void generateRegistrationRequest(as_nas_info_t *initialNasMsg, int Mod_id) {
  int size = sizeof(mm_msg_header_t);
  fgs_nas_message_t nas_msg={0};
  MM_msg *mm_msg;
  uicc_t * uicc=checkUicc(Mod_id);

  mm_msg = &nas_msg.plain.mm_msg;
  // set header
  mm_msg->header.ex_protocol_discriminator = FGS_MOBILITY_MANAGEMENT_MESSAGE;
  mm_msg->header.security_header_type = PLAIN_5GS_MSG;
  mm_msg->header.message_type = REGISTRATION_REQUEST;


  // set registration request
  mm_msg->registration_request.protocoldiscriminator = FGS_MOBILITY_MANAGEMENT_MESSAGE;
  size += 1;
  mm_msg->registration_request.securityheadertype = PLAIN_5GS_MSG;
  size += 1;
  mm_msg->registration_request.messagetype = REGISTRATION_REQUEST;
  size += 1;
  mm_msg->registration_request.fgsregistrationtype = INITIAL_REGISTRATION;
  mm_msg->registration_request.naskeysetidentifier.naskeysetidentifier = 1;
  size += 1;
  if(0){
    mm_msg->registration_request.fgsmobileidentity.guti.typeofidentity = FGS_MOBILE_IDENTITY_5G_GUTI;
    mm_msg->registration_request.fgsmobileidentity.guti.amfregionid = 0xca;
    mm_msg->registration_request.fgsmobileidentity.guti.amfpointer = 0;
    mm_msg->registration_request.fgsmobileidentity.guti.amfsetid = 1016;
    mm_msg->registration_request.fgsmobileidentity.guti.tmsi = 10;
    mm_msg->registration_request.fgsmobileidentity.guti.mncdigit1 =
      uicc->nmc_size==2 ? uicc->imsiStr[3]-'0' :  uicc->imsiStr[4]-'0';
    mm_msg->registration_request.fgsmobileidentity.guti.mncdigit2 =
      uicc->nmc_size==2 ? uicc->imsiStr[4]-'0' :  uicc->imsiStr[5]-'0';
    mm_msg->registration_request.fgsmobileidentity.guti.mncdigit3 =
      uicc->nmc_size==2 ? 0xf : uicc->imsiStr[3]-'0';
    mm_msg->registration_request.fgsmobileidentity.guti.mccdigit1 = uicc->imsiStr[0]-'0';
    mm_msg->registration_request.fgsmobileidentity.guti.mccdigit2 = uicc->imsiStr[1]-'0';
    mm_msg->registration_request.fgsmobileidentity.guti.mccdigit3 = uicc->imsiStr[2]-'0';

    size += 13;

  } else {
    mm_msg->registration_request.fgsmobileidentity.suci.typeofidentity = FGS_MOBILE_IDENTITY_SUCI;
    mm_msg->registration_request.fgsmobileidentity.suci.mncdigit1 =
     uicc->nmc_size==2 ? uicc->imsiStr[3]-'0' :  uicc->imsiStr[4]-'0';
    mm_msg->registration_request.fgsmobileidentity.suci.mncdigit2 =
      uicc->nmc_size==2 ? uicc->imsiStr[4]-'0' :  uicc->imsiStr[5]-'0';
    mm_msg->registration_request.fgsmobileidentity.suci.mncdigit3 =
      uicc->nmc_size==2 ? 0xf : uicc->imsiStr[3]-'0';
    mm_msg->registration_request.fgsmobileidentity.suci.mccdigit1 = uicc->imsiStr[0]-'0';
    mm_msg->registration_request.fgsmobileidentity.suci.mccdigit2 = uicc->imsiStr[1]-'0'; 
    mm_msg->registration_request.fgsmobileidentity.suci.mccdigit3 = uicc->imsiStr[2]-'0';
    memcpy(mm_msg->registration_request.fgsmobileidentity.suci.schemeoutput, uicc->imsiStr+3+uicc->nmc_size, strlen(uicc->imsiStr) - (3+uicc->nmc_size));
    size += sizeof(Suci5GSMobileIdentity_t);
  }

  mm_msg->registration_request.presencemask |= REGISTRATION_REQUEST_5GMM_CAPABILITY_PRESENT;
  mm_msg->registration_request.fgmmcapability.iei = REGISTRATION_REQUEST_5GMM_CAPABILITY_IEI;
  mm_msg->registration_request.fgmmcapability.length = 1;
  mm_msg->registration_request.fgmmcapability.value = 0x7;
  size += 3;

  mm_msg->registration_request.presencemask |= REGISTRATION_REQUEST_UE_SECURITY_CAPABILITY_PRESENT;
  mm_msg->registration_request.nruesecuritycapability.iei = REGISTRATION_REQUEST_UE_SECURITY_CAPABILITY_IEI;
  mm_msg->registration_request.nruesecuritycapability.length = 8;
  mm_msg->registration_request.nruesecuritycapability.fg_EA = 0x80;
  mm_msg->registration_request.nruesecuritycapability.fg_IA = 0x20;
  mm_msg->registration_request.nruesecuritycapability.EEA = 0;
  mm_msg->registration_request.nruesecuritycapability.EIA = 0;
  size += 10;

  // encode the message
  initialNasMsg->data = (Byte_t *)malloc(size * sizeof(Byte_t));
  registration_request_buf = initialNasMsg->data;

  initialNasMsg->length = mm_msg_encode(mm_msg, (uint8_t*)(initialNasMsg->data), size);
  registration_request_len = initialNasMsg->length;

}

void generateIdentityResponse(as_nas_info_t *initialNasMsg, uint8_t identitytype, uicc_t* uicc) {
  int size = sizeof(mm_msg_header_t);
  fgs_nas_message_t nas_msg;
  memset(&nas_msg, 0, sizeof(fgs_nas_message_t));
  MM_msg *mm_msg;

  mm_msg = &nas_msg.plain.mm_msg;
  // set header
  mm_msg->header.ex_protocol_discriminator = FGS_MOBILITY_MANAGEMENT_MESSAGE;
  mm_msg->header.security_header_type = PLAIN_5GS_MSG;
  mm_msg->header.message_type = FGS_IDENTITY_RESPONSE;


  // set identity response
  mm_msg->fgs_identity_response.protocoldiscriminator = FGS_MOBILITY_MANAGEMENT_MESSAGE;
  size += 1;
  mm_msg->fgs_identity_response.securityheadertype = PLAIN_5GS_MSG;
  size += 1;
  mm_msg->fgs_identity_response.messagetype = FGS_IDENTITY_RESPONSE;
  size += 1;
  if(identitytype == FGS_MOBILE_IDENTITY_SUCI){
    mm_msg->fgs_identity_response.fgsmobileidentity.suci.typeofidentity = FGS_MOBILE_IDENTITY_SUCI;
    mm_msg->fgs_identity_response.fgsmobileidentity.suci.mncdigit1 =
      uicc->nmc_size==2 ? uicc->imsiStr[3]-'0' :  uicc->imsiStr[4]-'0';
    mm_msg->fgs_identity_response.fgsmobileidentity.suci.mncdigit2 =
      uicc->nmc_size==2 ? uicc->imsiStr[4]-'0' :  uicc->imsiStr[5]-'0';
    mm_msg->fgs_identity_response.fgsmobileidentity.suci.mncdigit3 =
      uicc->nmc_size==2? 0xF : uicc->imsiStr[3]-'0';
    mm_msg->fgs_identity_response.fgsmobileidentity.suci.mccdigit1 = uicc->imsiStr[0]-'0';
    mm_msg->fgs_identity_response.fgsmobileidentity.suci.mccdigit2 = uicc->imsiStr[1]-'0';
    mm_msg->fgs_identity_response.fgsmobileidentity.suci.mccdigit3 = uicc->imsiStr[2]-'0';
    memcpy(mm_msg->registration_request.fgsmobileidentity.suci.schemeoutput, uicc->imsiStr+3+uicc->nmc_size, strlen(uicc->imsiStr) - (3+uicc->nmc_size));
    size += sizeof(Suci5GSMobileIdentity_t);
  }

  // encode the message
  initialNasMsg->data = (Byte_t *)malloc(size * sizeof(Byte_t));

  initialNasMsg->length = mm_msg_encode(mm_msg, (uint8_t*)(initialNasMsg->data), size);

}

OctetString knas_int;
static void generateAuthenticationResp(as_nas_info_t *initialNasMsg, uint8_t *buf, uicc_t *uicc){

  uint8_t ak[6];

  uint8_t kausf[32];
  uint8_t sqn[6];
  uint8_t kseaf[32];
  uint8_t kamf[32];
  OctetString res;

  // get RAND for authentication request
  unsigned char rand[16];
  for(int index = 0; index < 16;index++){
    rand[index] = buf[8+index];
  }

  uint8_t resTemp[16];
  uint8_t ck[16], ik[16], output[16];
  f2345(uicc->key, rand, resTemp, ck, ik, ak, uicc->opc);

  transferRES(ck, ik, resTemp, rand, output, uicc);

  // get knas_int
  knas_int.length = 16;
  knas_int.value = malloc(knas_int.length);
  for(int index = 0; index < 6; index++){
    sqn[index] = buf[26+index];
  }

  derive_kausf(ck, ik, sqn, kausf, uicc);
  derive_kseaf(kausf, kseaf, uicc);
  derive_kamf(kseaf, kamf, 0x0000, uicc);
  derive_knas(0x02, 2, kamf, knas_int.value);

  printf("kausf:");
  for(int i = 0; i < 32; i++){
    printf("%x ", kausf[i]);
  }
  printf("\n");

  printf("kseaf:");
  for(int i = 0; i < 32; i++){
    printf("%x ", kseaf[i]);
  }

  printf("\n");

  printf("kamf:");
  for(int i = 0; i < 32; i++){
    printf("%x ", kamf[i]);
  }
  printf("\n");

  printf("knas_int:\n");
  for(int i = 0; i < 16; i++){
    printf("%x ", knas_int.value[i]);
  }
  printf("\n");

  // set res
  res.length = 16;
  res.value = output;

  int size = sizeof(mm_msg_header_t);
  fgs_nas_message_t nas_msg;
  memset(&nas_msg, 0, sizeof(fgs_nas_message_t));
  MM_msg *mm_msg;

  mm_msg = &nas_msg.plain.mm_msg;
  // set header
  mm_msg->header.ex_protocol_discriminator = FGS_MOBILITY_MANAGEMENT_MESSAGE;
  mm_msg->header.security_header_type = PLAIN_5GS_MSG;
  mm_msg->header.message_type = FGS_AUTHENTICATION_RESPONSE;

  // set authentication response
  mm_msg->fgs_identity_response.protocoldiscriminator = FGS_MOBILITY_MANAGEMENT_MESSAGE;
  size += 1;
  mm_msg->fgs_identity_response.securityheadertype = PLAIN_5GS_MSG;
  size += 1;
  mm_msg->fgs_identity_response.messagetype = FGS_AUTHENTICATION_RESPONSE;
  size += 1;

  //set response parameter
  mm_msg->fgs_auth_response.authenticationresponseparameter.res = res;
  size += 18;
  // encode the message
  initialNasMsg->data = (Byte_t *)malloc(size * sizeof(Byte_t));

  initialNasMsg->length = mm_msg_encode(mm_msg, (uint8_t*)(initialNasMsg->data), size);
}

void generateSecurityModeComplete(as_nas_info_t *initialNasMsg)
{
  int size = sizeof(mm_msg_header_t);
  fgs_nas_message_t nas_msg;
  memset(&nas_msg, 0, sizeof(fgs_nas_message_t));

  MM_msg *mm_msg;
  nas_stream_cipher_t stream_cipher;
  uint8_t             mac[4];
  // set security protected header
  nas_msg.header.protocol_discriminator = FGS_MOBILITY_MANAGEMENT_MESSAGE;
  nas_msg.header.security_header_type = INTEGRITY_PROTECTED_AND_CIPHERED_WITH_NEW_SECU_CTX;
  size += 7;

  mm_msg = &nas_msg.security_protected.plain.mm_msg;

  // set header
  mm_msg->header.ex_protocol_discriminator = FGS_MOBILITY_MANAGEMENT_MESSAGE;
  mm_msg->header.security_header_type = PLAIN_5GS_MSG;
  mm_msg->header.message_type = FGS_SECURITY_MODE_COMPLETE;

  // set security mode complete
  mm_msg->fgs_security_mode_complete.protocoldiscriminator = FGS_MOBILITY_MANAGEMENT_MESSAGE;
  size += 1;
  mm_msg->fgs_security_mode_complete.securityheadertype    = PLAIN_5GS_MSG;
  size += 1;
  mm_msg->fgs_security_mode_complete.messagetype           = FGS_SECURITY_MODE_COMPLETE;
  size += 1;

  mm_msg->fgs_security_mode_complete.fgsmobileidentity.imeisv.typeofidentity = FGS_MOBILE_IDENTITY_IMEISV;
  mm_msg->fgs_security_mode_complete.fgsmobileidentity.imeisv.digit1  = 1;
  mm_msg->fgs_security_mode_complete.fgsmobileidentity.imeisv.digitp1 = 1;
  mm_msg->fgs_security_mode_complete.fgsmobileidentity.imeisv.digitp  = 1;
  mm_msg->fgs_security_mode_complete.fgsmobileidentity.imeisv.oddeven = 0;
  size += 5;

  mm_msg->fgs_security_mode_complete.fgsnasmessagecontainer.nasmessagecontainercontents.value  = registration_request_buf;
  mm_msg->fgs_security_mode_complete.fgsnasmessagecontainer.nasmessagecontainercontents.length = registration_request_len;
  size += (registration_request_len + 2);

  // encode the message
  initialNasMsg->data = (Byte_t *)malloc(size * sizeof(Byte_t));

  int security_header_len = nas_protected_security_header_encode((char*)(initialNasMsg->data),&(nas_msg.header), size);

  initialNasMsg->length = security_header_len + mm_msg_encode(mm_msg, (uint8_t*)(initialNasMsg->data+security_header_len), size-security_header_len);

  stream_cipher.key        = knas_int.value;
  stream_cipher.key_length = 16;
  stream_cipher.count      = 0;
  stream_cipher.bearer     = 1;
  stream_cipher.direction  = 0;
  stream_cipher.message    = (unsigned char *)(initialNasMsg->data + 6);
  /* length in bits */
  stream_cipher.blength    = (initialNasMsg->length - 6) << 3;

  // only for Type of integrity protection algorithm: 128-5G-IA2 (2)
  nas_stream_encrypt_eia2(
    &stream_cipher,
    mac);

  printf("mac %x %x %x %x \n", mac[0], mac[1], mac[2], mac[3]);
  for(int i = 0; i < 4; i++){
     initialNasMsg->data[2+i] = mac[i];
  }
}

void generateRegistrationComplete(as_nas_info_t *initialNasMsg, SORTransparentContainer               *sortransparentcontainer) {
  //wait send RRCReconfigurationComplete and InitialContextSetupResponse
  sleep(1);
  int length = 0;
  int size = 0;
  fgs_nas_message_t nas_msg;
  nas_stream_cipher_t stream_cipher;
  uint8_t             mac[4];
  memset(&nas_msg, 0, sizeof(fgs_nas_message_t));
  fgs_nas_message_security_protected_t *sp_msg;

  sp_msg = &nas_msg.security_protected;
  // set header
  sp_msg->header.protocol_discriminator = FGS_MOBILITY_MANAGEMENT_MESSAGE;
  sp_msg->header.security_header_type   = INTEGRITY_PROTECTED_AND_CIPHERED;
  sp_msg->header.message_authentication_code = 0;
  sp_msg->header.sequence_number        = 1;
  length = 7;
  sp_msg->plain.mm_msg.registration_complete.protocoldiscriminator = FGS_MOBILITY_MANAGEMENT_MESSAGE;
  length += 1;
  sp_msg->plain.mm_msg.registration_complete.securityheadertype    = PLAIN_5GS_MSG;
  sp_msg->plain.mm_msg.registration_complete.sparehalfoctet        = 0;
  length += 1;
  sp_msg->plain.mm_msg.registration_complete.messagetype = REGISTRATION_COMPLETE;
  length += 1;

  if(sortransparentcontainer) {
    length += sortransparentcontainer->sortransparentcontainercontents.length;
  }

  // encode the message
  initialNasMsg->data = (Byte_t *)malloc(length * sizeof(Byte_t));

  /* Encode the first octet of the header (extended protocol discriminator) */
  ENCODE_U8(initialNasMsg->data + size, sp_msg->header.protocol_discriminator, size);
  
  /* Encode the security header type */
  ENCODE_U8(initialNasMsg->data + size, sp_msg->header.security_header_type, size);
  
  /* Encode the message authentication code */
  ENCODE_U32(initialNasMsg->data + size, sp_msg->header.message_authentication_code, size);
  
  /* Encode the sequence number */
  ENCODE_U8(initialNasMsg->data + size, sp_msg->header.sequence_number, size);
  
  
  /* Encode the extended protocol discriminator */
  ENCODE_U8(initialNasMsg->data + size, sp_msg->plain.mm_msg.registration_complete.protocoldiscriminator, size);
    
  /* Encode the security header type */
  ENCODE_U8(initialNasMsg->data + size, sp_msg->plain.mm_msg.registration_complete.securityheadertype, size);
    
  /* Encode the message type */
  ENCODE_U8(initialNasMsg->data + size, sp_msg->plain.mm_msg.registration_complete.messagetype, size);

  if(sortransparentcontainer) {
    encode_registration_complete(&sp_msg->plain.mm_msg.registration_complete, initialNasMsg->data + size, length - size);
  }
  
  initialNasMsg->length = length;
  stream_cipher.key        = knas_int.value;
  stream_cipher.key_length = 16;
  stream_cipher.count      = 1;
  stream_cipher.bearer     = 1;
  stream_cipher.direction  = 0;
  stream_cipher.message    = (unsigned char *)(initialNasMsg->data + 6);
  /* length in bits */
  stream_cipher.blength    = (initialNasMsg->length - 6) << 3;

  // only for Type of integrity protection algorithm: 128-5G-IA2 (2)
  nas_stream_encrypt_eia2(
    &stream_cipher,
    mac);

  printf("mac %x %x %x %x \n", mac[0], mac[1], mac[2], mac[3]);
  for(int i = 0; i < 4; i++){
     initialNasMsg->data[2+i] = mac[i];
  }
}

void generatePduSessionEstablishRequest(as_nas_info_t *initialNasMsg){
  //wait send RegistrationComplete
  usleep(100*150);
  int size = 0;
  fgs_nas_message_t nas_msg;
  memset(&nas_msg, 0, sizeof(fgs_nas_message_t));

  // setup pdu session establishment request
  uint16_t req_length = 7;
  uint8_t *req_buffer = malloc(req_length);
  pdu_session_establishment_request_msg pdu_session_establish;
  pdu_session_establish.protocoldiscriminator = FGS_SESSION_MANAGEMENT_MESSAGE;
  pdu_session_establish.pdusessionid = 10;
  pdu_session_establish.pti = 1;
  pdu_session_establish.pdusessionestblishmsgtype = FGS_PDU_SESSION_ESTABLISHMENT_REQ;
  pdu_session_establish.maxdatarate = 0xffff;
  pdu_session_establish.pdusessiontype = 0x91;
  encode_pdu_session_establishment_request(&pdu_session_establish, req_buffer);



  MM_msg *mm_msg;
  nas_stream_cipher_t stream_cipher;
  uint8_t             mac[4];
  uint8_t             nssai[]={1,0,0,1}; //Corresponding to SST:1, SD:1
  uint8_t            dnn[4]={0x4,0x6f,0x61,0x69}; //Corresponding to dnn:"oai"
  nas_msg.header.protocol_discriminator = FGS_MOBILITY_MANAGEMENT_MESSAGE;
  nas_msg.header.security_header_type = INTEGRITY_PROTECTED_AND_CIPHERED_WITH_NEW_SECU_CTX;
  size += 7;

  mm_msg = &nas_msg.security_protected.plain.mm_msg;

  // set header
  mm_msg->header.ex_protocol_discriminator = FGS_MOBILITY_MANAGEMENT_MESSAGE;
  mm_msg->header.security_header_type = PLAIN_5GS_MSG;
  mm_msg->header.message_type = FGS_UPLINK_NAS_TRANSPORT;

  // set uplink nas transport
  mm_msg->uplink_nas_transport.protocoldiscriminator = FGS_MOBILITY_MANAGEMENT_MESSAGE;
  size += 1;
  mm_msg->uplink_nas_transport.securityheadertype    = PLAIN_5GS_MSG;
  size += 1;
  mm_msg->uplink_nas_transport.messagetype = FGS_UPLINK_NAS_TRANSPORT;
  size += 1;

  mm_msg->uplink_nas_transport.payloadcontainertype.iei = 0;
  mm_msg->uplink_nas_transport.payloadcontainertype.type = 1;
  size += 1;
  mm_msg->uplink_nas_transport.fgspayloadcontainer.payloadcontainercontents.length = req_length;
  mm_msg->uplink_nas_transport.fgspayloadcontainer.payloadcontainercontents.value = req_buffer;
  size += (2+req_length);
  mm_msg->uplink_nas_transport.pdusessionid = 10;
  mm_msg->uplink_nas_transport.requesttype = 1;
  size += 3;
  mm_msg->uplink_nas_transport.snssai.length = 4;
  mm_msg->uplink_nas_transport.snssai.value = nssai;
  size += (1+1+4);
  mm_msg->uplink_nas_transport.dnn.length = 4;
  mm_msg->uplink_nas_transport.dnn.value = dnn;
  size += (1+1+4);

  // encode the message
  initialNasMsg->data = (Byte_t *)malloc(size * sizeof(Byte_t));
  int security_header_len = nas_protected_security_header_encode((char*)(initialNasMsg->data),&(nas_msg.header), size);

  initialNasMsg->length = security_header_len + mm_msg_encode(mm_msg, (uint8_t*)(initialNasMsg->data+security_header_len), size-security_header_len);

  stream_cipher.key        = knas_int.value;
  stream_cipher.key_length = 16;
  stream_cipher.count      = 0;
  stream_cipher.bearer     = 1;
  stream_cipher.direction  = 0;
  stream_cipher.message    = (unsigned char *)(initialNasMsg->data + 6);
  /* length in bits */
  stream_cipher.blength    = (initialNasMsg->length - 6) << 3;

  // only for Type of integrity protection algorithm: 128-5G-IA2 (2)
  nas_stream_encrypt_eia2(
    &stream_cipher,
    mac);

  printf("mac %x %x %x %x \n", mac[0], mac[1], mac[2], mac[3]);
  for(int i = 0; i < 4; i++){
     initialNasMsg->data[2+i] = mac[i];
  }
}


void *nas_nrue_task(void *args_p)
{
  MessageDef           *msg_p;
  instance_t            instance;
  unsigned int          Mod_id;
  int                   result;
  uint8_t               msg_type = 0;
  uint8_t              *pdu_buffer = NULL;

  itti_mark_task_ready (TASK_NAS_NRUE);
  MSC_START_USE();
  
  while(1) {
    // Wait for a message or an event
    itti_receive_msg (TASK_NAS_NRUE, &msg_p);

    if (msg_p != NULL) {
      instance = msg_p->ittiMsgHeader.originInstance;
      Mod_id = instance ;
      uicc_t *uicc=checkUicc(Mod_id);
      
      if (instance == INSTANCE_DEFAULT) {
        printf("%s:%d: FATAL: instance is INSTANCE_DEFAULT, should not happen.\n",
               __FILE__, __LINE__);
        exit_fun("exit... \n");
      }

      switch (ITTI_MSG_ID(msg_p)) {
      case INITIALIZE_MESSAGE:
        LOG_I(NAS, "[UE %d] Received %s\n", Mod_id,  ITTI_MSG_NAME (msg_p));

        break;

      case TERMINATE_MESSAGE:
        itti_exit_task ();
        break;

      case MESSAGE_TEST:
        LOG_I(NAS, "[UE %d] Received %s\n", Mod_id,  ITTI_MSG_NAME (msg_p));
        break;

      case NAS_CELL_SELECTION_CNF:
        LOG_I(NAS, "[UE %d] Received %s: errCode %u, cellID %u, tac %u\n", Mod_id,  ITTI_MSG_NAME (msg_p),
              NAS_CELL_SELECTION_CNF (msg_p).errCode, NAS_CELL_SELECTION_CNF (msg_p).cellID, NAS_CELL_SELECTION_CNF (msg_p).tac);
        // as_stmsi_t s_tmsi={0, 0};
        // as_nas_info_t nas_info;
        // plmn_t plmnID={0, 0, 0, 0};
        // generateRegistrationRequest(&nas_info);
        // nr_nas_itti_nas_establish_req(0, AS_TYPE_ORIGINATING_SIGNAL, s_tmsi, plmnID, nas_info.data, nas_info.length, 0);
        break;

      case NAS_CELL_SELECTION_IND:
        LOG_I(NAS, "[UE %d] Received %s: cellID %u, tac %u\n", Mod_id,  ITTI_MSG_NAME (msg_p),
              NAS_CELL_SELECTION_IND (msg_p).cellID, NAS_CELL_SELECTION_IND (msg_p).tac);

        /* TODO not processed by NAS currently */
        break;

      case NAS_PAGING_IND:
        LOG_I(NAS, "[UE %d] Received %s: cause %u\n", Mod_id,  ITTI_MSG_NAME (msg_p),
              NAS_PAGING_IND (msg_p).cause);

        /* TODO not processed by NAS currently */
        break;

      case NAS_CONN_ESTABLI_CNF:
      {
        LOG_I(NAS, "[UE %d] Received %s: errCode %u, length %u\n", Mod_id,  ITTI_MSG_NAME (msg_p),
              NAS_CONN_ESTABLI_CNF (msg_p).errCode, NAS_CONN_ESTABLI_CNF (msg_p).nasMsg.length);

        pdu_buffer = NAS_CONN_ESTABLI_CNF (msg_p).nasMsg.data;
        if((pdu_buffer + 1) != NULL){
          if (*(pdu_buffer + 1) > 0 ) {
            if((pdu_buffer + 9) != NULL){
                msg_type = *(pdu_buffer + 9);
            } else {
              LOG_W(NAS, "[UE] Received invalid downlink message\n");
              break;
            }
          } else {
            if((pdu_buffer + 2) != NULL){
              msg_type = *(pdu_buffer + 2);
            } else {
                LOG_W(NAS, "[UE] Received invalid downlink message\n");
                break;
            }
          }
        }
        if(msg_type == REGISTRATION_ACCEPT){
          LOG_I(NAS, "[UE] Received REGISTRATION ACCEPT message\n");

          as_nas_info_t initialNasMsg;
          memset(&initialNasMsg, 0, sizeof(as_nas_info_t));
          generateRegistrationComplete(&initialNasMsg, NULL);
          if(initialNasMsg.length > 0){
            MessageDef *message_p;
            message_p = itti_alloc_new_message(TASK_NAS_NRUE, 0, NAS_UPLINK_DATA_REQ);
            NAS_UPLINK_DATA_REQ(message_p).UEid          = Mod_id;
            NAS_UPLINK_DATA_REQ(message_p).nasMsg.data   = (uint8_t *)initialNasMsg.data;
            NAS_UPLINK_DATA_REQ(message_p).nasMsg.length = initialNasMsg.length;
            itti_send_msg_to_task(TASK_RRC_NRUE, instance, message_p);
            LOG_I(NAS, "Send NAS_UPLINK_DATA_REQ message(RegistrationComplete)\n");
          }

          as_nas_info_t pduEstablishMsg;
          memset(&pduEstablishMsg, 0, sizeof(as_nas_info_t));
          generatePduSessionEstablishRequest(&pduEstablishMsg);
          if(pduEstablishMsg.length > 0){
            MessageDef *message_p;
            message_p = itti_alloc_new_message(TASK_NAS_NRUE, 0, NAS_UPLINK_DATA_REQ);
            NAS_UPLINK_DATA_REQ(message_p).UEid          = Mod_id;
            NAS_UPLINK_DATA_REQ(message_p).nasMsg.data   = (uint8_t *)pduEstablishMsg.data;
            NAS_UPLINK_DATA_REQ(message_p).nasMsg.length = pduEstablishMsg.length;
            itti_send_msg_to_task(TASK_RRC_NRUE, instance, message_p);
            LOG_I(NAS, "Send NAS_UPLINK_DATA_REQ message(PduSessionEstablishRequest)\n");
          }
        }
        else if((pdu_buffer + 16) != NULL){
          msg_type = *(pdu_buffer + 16);
          if(msg_type == FGS_PDU_SESSION_ESTABLISHMENT_ACC){
            sprintf(baseNetAddress, "%d.%d", *(pdu_buffer + 39),*(pdu_buffer + 40));
            int third_octet = *(pdu_buffer + 41);
            int fourth_octet = *(pdu_buffer + 42);
            LOG_I(NAS, "Received PDU Session Establishment Accept\n");
            nas_config(1,third_octet,fourth_octet,"ue");
          }
        }

        break;
      }

      case NAS_CONN_RELEASE_IND:
        LOG_I(NAS, "[UE %d] Received %s: cause %u\n", Mod_id, ITTI_MSG_NAME (msg_p),
              NAS_CONN_RELEASE_IND (msg_p).cause);

        break;

      case NAS_UPLINK_DATA_CNF:
        LOG_I(NAS, "[UE %d] Received %s: UEid %u, errCode %u\n", Mod_id, ITTI_MSG_NAME (msg_p),
              NAS_UPLINK_DATA_CNF (msg_p).UEid, NAS_UPLINK_DATA_CNF (msg_p).errCode);

        break;

      case NAS_DOWNLINK_DATA_IND:
      {
        LOG_I(NAS, "[UE %d] Received %s: UEid %u, length %u , buffer %p\n", Mod_id,
                                                                            ITTI_MSG_NAME (msg_p),
                                                                            Mod_id,
                                                                            NAS_DOWNLINK_DATA_IND(msg_p).nasMsg.length,
                                                                            NAS_DOWNLINK_DATA_IND(msg_p).nasMsg.data);
        as_nas_info_t initialNasMsg;
        memset(&initialNasMsg, 0, sizeof(as_nas_info_t));

        pdu_buffer = NAS_DOWNLINK_DATA_IND(msg_p).nasMsg.data;
        if((pdu_buffer + 1) != NULL){
          if (*(pdu_buffer + 1) > 0 ) {
            msg_type = *(pdu_buffer + 9);
          } else {
            msg_type = *(pdu_buffer + 2);
          }
        }
        if((pdu_buffer + 2) == NULL){
          LOG_W(NAS, "[UE] Received invalid downlink message\n");
          return 0;
        }

        switch(msg_type){
          case FGS_IDENTITY_REQUEST:
	    generateIdentityResponse(&initialNasMsg,*(pdu_buffer+3), uicc);
              break;
          case FGS_AUTHENTICATION_REQUEST:
	    generateAuthenticationResp(&initialNasMsg, pdu_buffer, uicc);
              break;
          case FGS_SECURITY_MODE_COMMAND:
            generateSecurityModeComplete(&initialNasMsg);
            break;
          default:
              LOG_W(NR_RRC,"unknow message type %d\n",msg_type);
              break;
        }

        if(initialNasMsg.length > 0){
          MessageDef *message_p;
          message_p = itti_alloc_new_message(TASK_NAS_NRUE, 0, NAS_UPLINK_DATA_REQ);
          NAS_UPLINK_DATA_REQ(message_p).UEid          = Mod_id;
          NAS_UPLINK_DATA_REQ(message_p).nasMsg.data   = (uint8_t *)initialNasMsg.data;
          NAS_UPLINK_DATA_REQ(message_p).nasMsg.length = initialNasMsg.length;
          itti_send_msg_to_task(TASK_RRC_NRUE, instance, message_p);
          LOG_I(NAS, "Send NAS_UPLINK_DATA_REQ message\n");
        }

        break;
      }

      default:
        LOG_E(NAS, "[UE %d] Received unexpected message %s\n", Mod_id,  ITTI_MSG_NAME (msg_p));
        break;
      }

      result = itti_free (ITTI_MSG_ORIGIN_ID(msg_p), msg_p);
      AssertFatal (result == EXIT_SUCCESS, "Failed to free memory (%d)!\n", result);
      msg_p = NULL;
    }
  }

  return NULL;
}
