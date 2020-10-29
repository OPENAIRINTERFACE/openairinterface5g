/*! \file nr_nas_msg_sim.h

\brief simulator for nr nas message
\author Yoshio INOUE, Masayuki HARADA
\email: yoshio.inoue@fujitsu.com,masayuki.harada@fujitsu.com
\date 2020
\version 0.1
*/


#ifndef __NR_NAS_MSG_SIM_H__
#define __NR_NAS_MSG_SIM_H__

#include "RegistrationRequest.h"
#include "RegistrationComplete.h"
#include "as_message.h"


#define PLAIN_5GS_MSG                                      0b0000
#define INTEGRITY_PROTECTED                                0b0001
#define INTEGRITY_PROTECTED_AND_CIPHERED                   0b0010

#define INITIAL_REGISTRATION                               0b001


#define REGISTRATION_REQUEST                               0b01000001 /* 65 = 0x41 */
#define REGISTRATION_ACCEPT                                0b01000010 /* 66 = 0x42 */
#define REGISTRATION_COMPLETE                              0b01000011 /* 67 = 0x43 */
#define REGISTRATION_REJECT                                0b01000100 /* 68 = 0x44 */
#define DEREGISTRATION_REQUEST_UE_ORIGINATING              0b01000101 /* 69 = 0x45 */
#define DEREGISTRATION_ACCEPT_UE_ORIGINATING               0b01000110 /* 70 = 0x46 */
#define DEREGISTRATION_REQUEST_UE_TERMINATED               0b01000111 /* 71 = 0x47 */
#define DEREGISTRATION_ACCEPT_UE_TERMINATED                0b01001000 /* 72 = 0x48 */

#define FIVEGMM_SERVICE_REQUEST                            0b01001100 /* 76 = 0x4c */
#define FIVEGMM_SERVICE_REJECT                             0b01001101 /* 77 = 0x4d */
#define FIVEGMM_SERVICE_ACCEPT                             0b01001110 /* 78 = 0x4e */

#define CONFIGURATION_UPDATE_COMMAND                       0b01010100 /* 84 = 0x54 */
#define CONFIGURATION_UPDATE_COMPLETE                      0b01010101 /* 85 = 0x55 */
#define AUTHENTICATION_REQUEST                             0b01010110 /* 86 = 0x56 */
#define AUTHENTICATION_RESPONSE                            0b01010111 /* 87 = 0x57 */
#define AUTHENTICATION_REJECT                              0b01011000 /* 88 = 0x58 */
#define AUTHENTICATION_FAILURE                             0b01011001 /* 89 = 0x59 */
#define AUTHENTICATION_RESULT                              0b01011010 /* 90 = 0x5a */
#define FIVEGMM_IDENTITY_REQUEST                           0b01011011 /* 91 = 0x5b */
#define FIVEGMM_IDENTITY_RESPONSE                          0b01011100 /* 92 = 0x5c */
#define FIVEGMM_SECURITY_MODE_COMMAND                      0b01011101 /* 93 = 0x5d */
#define FIVEGMM_SECURITY_MODE_COMPLETE                     0b01011110 /* 94 = 0x5e */
#define FIVEGMM_SECURITY_MODE_REJECT 	                     0b01011111 /* 95 = 0x5f */
#define FIVEGMM_STATUS                                     0b01100100 /* 100 = 0x64 */
#define NOTIFICATION                                       0b01100101 /* 101 = 0x65 */
#define NOTIFICATION_RESPONSE                              0b01100110 /* 102 = 0x66 */
#define UL_NAS_TRANSPORT                                   0b01100111 /* 103 = 0x67 */
#define DL_NAS_TRANSPORT                                   0b01101000 /* 104 = 0x68 */



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
  registration_complete_msg              registration_complete;
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

void generateRegistrationRequest(as_nas_info_t *initialNasMsg);
void generateRegistrationComplete(as_nas_info_t *ulNasMsg, SORTransparentContainer *sortransparentcontainer);
#endif /* __NR_NAS_MSG_SIM_H__*/
