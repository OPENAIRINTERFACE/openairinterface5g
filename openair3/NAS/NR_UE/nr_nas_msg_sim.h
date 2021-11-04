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

/*! \file nr_nas_msg_sim.h
 * \brief simulator for nr nas message
 * \author Yoshio INOUE, Masayuki HARADA
 * \email yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
 * \date 2020
 * \version 0.1
 */



#ifndef __NR_NAS_MSG_SIM_H__
#define __NR_NAS_MSG_SIM_H__

#include "RegistrationRequest.h"
#include "FGSIdentityResponse.h"
#include "FGSAuthenticationResponse.h"
#include "FGSNASSecurityModeComplete.h"
#include "RegistrationComplete.h"
#include "as_message.h"
#include "FGSUplinkNasTransport.h"

#define PLAIN_5GS_MSG                                      0b0000
#define INTEGRITY_PROTECTED                                0b0001
#define INTEGRITY_PROTECTED_AND_CIPHERED                   0b0010
#define INTEGRITY_PROTECTED_WITH_NEW_SECU_CTX              0b0011          // only for SECURITY MODE COMMAND
#define INTEGRITY_PROTECTED_AND_CIPHERED_WITH_NEW_SECU_CTX 0b0100         // only for SECURITY MODE COMPLETE

#define REGISTRATION_REQUEST                               0b01000001 /* 65 = 0x41 */
#define REGISTRATION_ACCEPT                                0b01000010 /* 66 = 0x42 */
#define REGISTRATION_COMPLETE                              0b01000011 /* 67 = 0x43 */
#define FGS_AUTHENTICATION_REQUEST                         0b01010110 /* 86 = 0x56 */
#define FGS_AUTHENTICATION_RESPONSE                        0b01010111 /* 87 = 0x57 */
#define FGS_IDENTITY_REQUEST                               0b01011011 /* 91 = 0x5b */
#define FGS_IDENTITY_RESPONSE                              0b01011100 /* 92 = 0x5c */
#define FGS_SECURITY_MODE_COMMAND                          0b01011101 /* 93 = 0x5d */
#define FGS_SECURITY_MODE_COMPLETE                         0b01011110 /* 94 = 0x5e */
#define FGS_UPLINK_NAS_TRANSPORT                           0b01100111 /* 103= 0x67 */
#define FGS_DOWNLINK_NAS_TRANSPORT                         0b01101000 /* 104= 0x68 */

// message type for 5GS session management
#define FGS_PDU_SESSION_ESTABLISHMENT_REQ                  0b11000001 /* 193= 0xc1 */
#define FGS_PDU_SESSION_ESTABLISHMENT_ACC                  0b11000010 /* 194= 0xc2 */

#define INITIAL_REGISTRATION                               0b001

/* Security Key for SA UE */
typedef struct {
  uint8_t kausf[32];
  uint8_t kseaf[32];
  uint8_t kamf[32];
  uint8_t knas_int[16];
  uint8_t res[16];
  uint8_t rand[16];
  uint8_t kgnb[32];
} ue_sa_security_key_t;

typedef enum fgs_protocol_discriminator_e {
  /* Protocol discriminator identifier for 5GS Mobility Management */
  FGS_MOBILITY_MANAGEMENT_MESSAGE =   0x7E,

  /* Protocol discriminator identifier for 5GS Session Management */
  FGS_SESSION_MANAGEMENT_MESSAGE =    0x2E,
} fgs_protocol_discriminator_t;


typedef struct {
  uint8_t ex_protocol_discriminator;
  uint8_t security_header_type;
  uint8_t message_type;
} mm_msg_header_t;

/* Structure of security protected header */
typedef struct {
  fgs_protocol_discriminator_t    protocol_discriminator;
  uint8_t                         security_header_type;
  uint32_t                        message_authentication_code;
  uint8_t                         sequence_number;
} fgs_nas_message_security_header_t;

typedef union {
  mm_msg_header_t                        header;
  registration_request_msg               registration_request;
  fgs_identiy_response_msg               fgs_identity_response;
  fgs_authentication_response_msg        fgs_auth_response;
  fgs_security_mode_complete_msg         fgs_security_mode_complete;
  registration_complete_msg              registration_complete;
  fgs_uplink_nas_transport_msg           uplink_nas_transport;
} MM_msg;



typedef struct {
  MM_msg mm_msg;    /* 5GS Mobility Management messages */
} fgs_nas_message_plain_t;

typedef struct {
  fgs_nas_message_security_header_t header;
  fgs_nas_message_plain_t plain;
} fgs_nas_message_security_protected_t;


typedef union {
  fgs_nas_message_security_header_t header;
  fgs_nas_message_security_protected_t security_protected;
  fgs_nas_message_plain_t plain;
} fgs_nas_message_t;

void generateRegistrationRequest(as_nas_info_t *initialNasMsg, int Mod_id);
void *nas_nrue_task(void *args_p);

#endif /* __NR_NAS_MSG_SIM_H__*/



