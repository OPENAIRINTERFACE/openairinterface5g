
/*! \file nr_nas_msg_sim.c

\brief simulator for nr nas message
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/

#include <string.h> // memset
#include <stdlib.h> // malloc, free

#include "nas_log.h"
#include "TLVDecoder.h"
#include "TLVEncoder.h"
#include "nr_nas_msg_sim.h"
#include "aka_functions.h"
#include "secu_defs.h"


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

void transferRES(uint8_t ck[16], uint8_t ik[16], uint8_t *input, uint8_t rand[16], uint8_t *output) {
  char netName[] = "5G:mnc093.mcc208.3gppnetwork.org";
  uint8_t S[100];
  S[0] = 0x6B;
  int netNamesize = strlen(netName);
  memcpy(&S[1], netName, netNamesize);
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


void generateRegistrationRequest(as_nas_info_t *initialNasMsg) {
  int size = sizeof(mm_msg_header_t);
  fgs_nas_message_t nas_msg;
  memset(&nas_msg, 0, sizeof(fgs_nas_message_t));
  MM_msg *mm_msg;

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
  if(1){
    mm_msg->registration_request.fgsmobileidentity.guti.typeofidentity = FGS_MOBILE_IDENTITY_5G_GUTI;
    mm_msg->registration_request.fgsmobileidentity.guti.amfregionid = 0xca;
    mm_msg->registration_request.fgsmobileidentity.guti.amfpointer = 0;
    mm_msg->registration_request.fgsmobileidentity.guti.amfsetid = 1016;
    mm_msg->registration_request.fgsmobileidentity.guti.tmsi = 10;
    mm_msg->registration_request.fgsmobileidentity.guti.mncdigit1 = 9;
    mm_msg->registration_request.fgsmobileidentity.guti.mncdigit2 = 3;
    mm_msg->registration_request.fgsmobileidentity.guti.mncdigit3 = 0xf;
    mm_msg->registration_request.fgsmobileidentity.guti.mccdigit1 = 2;
    mm_msg->registration_request.fgsmobileidentity.guti.mccdigit2 = 0;
    mm_msg->registration_request.fgsmobileidentity.guti.mccdigit3 = 8;

    size += 13;

  } else {
    mm_msg->registration_request.fgsmobileidentity.suci.typeofidentity = FGS_MOBILE_IDENTITY_SUCI;
    mm_msg->registration_request.fgsmobileidentity.suci.mncdigit1 = 9;
    mm_msg->registration_request.fgsmobileidentity.suci.mncdigit2 = 3;
    mm_msg->registration_request.fgsmobileidentity.suci.mncdigit3 = 0xf;
    mm_msg->registration_request.fgsmobileidentity.suci.mccdigit1 = 2;
    mm_msg->registration_request.fgsmobileidentity.suci.mccdigit2 = 0;
    mm_msg->registration_request.fgsmobileidentity.suci.mccdigit3 = 8;
    mm_msg->registration_request.fgsmobileidentity.suci.schemeoutput = 0x4778;

    size += 14;
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

  initialNasMsg->length = mm_msg_encode(mm_msg, (uint8_t*)(initialNasMsg->data), size);


}

void generateIdentityResponse(as_nas_info_t *initialNasMsg, uint8_t identitytype) {
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
    mm_msg->fgs_identity_response.fgsmobileidentity.suci.mncdigit1 = 9;
    mm_msg->fgs_identity_response.fgsmobileidentity.suci.mncdigit2 = 3;
    mm_msg->fgs_identity_response.fgsmobileidentity.suci.mncdigit3 = 0xf;
    mm_msg->fgs_identity_response.fgsmobileidentity.suci.mccdigit1 = 2;
    mm_msg->fgs_identity_response.fgsmobileidentity.suci.mccdigit2 = 0;
    mm_msg->fgs_identity_response.fgsmobileidentity.suci.mccdigit3 = 8;
    mm_msg->fgs_identity_response.fgsmobileidentity.suci.schemeoutput = 0x4778;

    size += 14;
  }

  // encode the message
  initialNasMsg->data = (Byte_t *)malloc(size * sizeof(Byte_t));

  initialNasMsg->length = mm_msg_encode(mm_msg, (uint8_t*)(initialNasMsg->data), size);

}

void generateAuthenticationResp(as_nas_info_t *initialNasMsg, uint8_t *buf){

  uint8_t ak[6];

  // USIM_API_K: 5122250214c33e723a5dd523fc145fc0
  uint8_t k[16] = {0x51, 0x22, 0x25, 0x02, 0x14,0xc3, 0x3e, 0x72, 0x3a, 0x5d, 0xd5, 0x23, 0xfc, 0x14, 0x5f, 0xc0};
  // OPC: 981d464c7c52eb6e5036234984ad0bcf
  const uint8_t opc[16] = {0x98, 0x1d, 0x46, 0x4c,0x7c,0x52,0xeb, 0x6e, 0x50, 0x36, 0x23, 0x49, 0x84, 0xad, 0x0b, 0xcf};

  // get RAND for authentication request
  unsigned char rand[16];
  for(int index = 0; index < 16;index++){
    rand[index] = buf[8+index];
  }

  uint8_t resTemp[16];
  uint8_t ck[16], ik[16], output[16];
  f2345(k, rand, resTemp, ck, ik, ak, opc);

  transferRES(ck, ik, resTemp, rand, output);

  OctetString res;
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



