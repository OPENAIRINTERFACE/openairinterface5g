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

/*****************************************************************************

Source    nas_message.h

Version   0.1

Date    2012/26/09

Product   NAS stack

Subsystem Application Programming Interface

Author    Frederic Maurel

Description Defines the layer 3 messages supported by the NAS sublayer
    protocol and functions used to encode and decode

*****************************************************************************/

#include "nas_message.h"
#include "nas_log.h"

#include "TLVDecoder.h"
#include "TLVEncoder.h"

#include <stdlib.h> // malloc, free
#include <string.h> // memcpy

#if ((defined(NAS_BUILT_IN_EPC) && defined(NAS_MME)) || (defined(ENABLE_NAS_UE_LOGGING) && defined(NAS_BUILT_IN_UE) && defined(NAS_UE)))
# include "nas_itti_messaging.h"
#endif
#include "secu_defs.h"
#include "emmData.h"

//#define DEBUG_NAS_MESSAGE

/****************************************************************************/
/****************  E X T E R N A L    D E F I N I T I O N S  ****************/
/****************************************************************************/

/****************************************************************************/
/*******************  L O C A L    D E F I N I T I O N S  *******************/
/****************************************************************************/


/* Functions used to decode layer 3 NAS messages */
static int _nas_message_header_decode(const char* buffer,
                                      nas_message_security_header_t* header, int length);
static int _nas_message_plain_decode(const char* buffer,
                                     const nas_message_security_header_t* header,
                                     nas_message_plain_t* msg, int length);
static int _nas_message_protected_decode(const char* buffer,
    nas_message_security_header_t* header,
    nas_message_plain_t* msg, int length,
    const emm_security_context_t * const emm_security_context);

/* Functions used to encode layer 3 NAS messages */
static int _nas_message_header_encode(char* buffer,
                                      const nas_message_security_header_t* header, int length);
static int _nas_message_plain_encode(char* buffer,
                                     const nas_message_security_header_t* header,
                                     const nas_message_plain_t* msg, int length);

static int _nas_message_protected_encode(
  char                                    *buffer,
  const nas_message_security_protected_t  *msg,
  int                                      length,
  void                                    *security);

/* Functions used to decrypt and encrypt layer 3 NAS messages */
static int _nas_message_decrypt(
  char* dest,
  const char* src,
  uint8_t type,
  uint32_t code,
  uint8_t seq,
  int length,
  const emm_security_context_t * const emm_security_context);


static int
_nas_message_encrypt(
  char       *dest,
  const char *src,
  uint8_t     type,
  uint32_t    code,
  uint8_t     seq,
  int         const direction,
  int         length,
  const emm_security_context_t * const emm_security_context);

/* Functions used for integrity protection of layer 3 NAS messages */
static uint32_t _nas_message_get_mac(
  const char                   * const buffer,
  int                            const length,
  int                            const direction,
  const emm_security_context_t * const emm_security_context);

/****************************************************************************/
/******************  E X P O R T E D    F U N C T I O N S  ******************/
/****************************************************************************/

/****************************************************************************
 **                                                                        **
 ** Name:  nas_message_encrypt()                                     **
 **                                                                        **
 ** Description: Encripts plain NAS message into security protected NAS    **
 **    message                                                   **
 **                                                                        **
 ** Inputs:  inbuf:   Input buffer containing plain NAS message  **
 **    header:  Security protected header to be applied    **
 **      length:  Number of bytes that should be encrypted   **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   outbuf:  Output buffer containing security protec-  **
 **       ted message                                **
 **      Return:  The number of bytes in the output buffer   **
 **       if the input buffer has been successfully  **
 **       encrypted; Negative error code otherwise.  **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int
nas_message_encrypt(
  const char                          *inbuf,
  char                                *outbuf,
  const nas_message_security_header_t *header,
  int                                  length,
  void                                *security)
{
  LOG_FUNC_IN;
  emm_security_context_t *emm_security_context   = (emm_security_context_t*)security;
  int bytes = length;

  /* Encode the header */
  int size = _nas_message_header_encode(outbuf, header, length);

  if (size < 0) {
    LOG_FUNC_RETURN (TLV_ENCODE_BUFFER_TOO_SHORT);
  } else if (size > 1) {
    /* Encrypt the plain NAS message */
    bytes = _nas_message_encrypt(outbuf + size, inbuf,
                                 header->security_header_type,
                                 header->message_authentication_code,
                                 header->sequence_number,
#ifdef NAS_MME
                                 SECU_DIRECTION_DOWNLINK,
#else
                                 SECU_DIRECTION_UPLINK,
#endif
                                 length - size,
                                 emm_security_context);

    /* Integrity protected the NAS message */
    if (bytes > 0) {
      /* Compute offset of the sequence number field */
      int offset = size - sizeof(uint8_t);
      /* Compute the NAS message authentication code */
      uint32_t mac = _nas_message_get_mac(
                       outbuf + offset,
                       bytes + size - offset,
#ifdef NAS_MME
                       SECU_DIRECTION_DOWNLINK,
#else
                       SECU_DIRECTION_UPLINK,
#endif
                       emm_security_context);
      /* Set the message authentication code of the NAS message */
      *(uint32_t*)(outbuf + sizeof(uint8_t)) = htonl(mac);
    }
  } else {
    /* The input buffer does not need to be encrypted */
    memcpy(outbuf, inbuf, length);
  }

#ifdef NAS_MME
  /* TS 124.301, section 4.4.3.1
   * The NAS sequence number part of the NAS COUNT shall be
   * exchanged between the UE and the MME as part of the
   * NAS signalling. After each new or retransmitted outbound
   * security protected NAS message, the sender shall increase
   * the NAS COUNT number by one. Specifically, on the sender
   * side, the NAS sequence number shall be increased by one,
   * and if the result is zero (due to wrap around), the NAS
   * overflow counter shall also be incremented by one (see
   * subclause 4.4.3.5).
   */
  emm_security_context->dl_count.seq_num += 1;

  if ( ! emm_security_context->dl_count.seq_num) {
    emm_security_context->dl_count.overflow += 1;
  }

  LOG_TRACE(DEBUG,
            "Incremented emm_security_context.dl_count.seq_num -> %u",
            emm_security_context->dl_count.seq_num);
#else
  emm_security_context->ul_count.seq_num += 1;

  if ( ! emm_security_context->ul_count.seq_num) {
    emm_security_context->ul_count.overflow += 1;
  }

  LOG_TRACE(DEBUG,
            "Incremented emm_security_context.ul_count.seq_num -> %u",
            emm_security_context->ul_count.seq_num);
#endif

  if (bytes < 0) {
    LOG_FUNC_RETURN (bytes);
  }

  if (size > 1) {
    LOG_FUNC_RETURN (size + bytes);
  }

  LOG_FUNC_RETURN (bytes);
}

/****************************************************************************
 **                                                                        **
 ** Name:  nas_message_decrypt()                                     **
 **                                                                        **
 ** Description: Decripts security protected NAS message into plain NAS    **
 **    message                                                   **
 **                                                                        **
 ** Inputs:  inbuf:   Input buffer containing security protected **
 **       NAS message                                **
 **      length:  Number of bytes that should be decrypted   **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   outbuf:  Output buffer containing plain NAS message **
 **    header:  Security protected header applied          **
 **      Return:  The number of bytes in the output buffer   **
 **       if the input buffer has been successfully  **
 **       decrypted; Negative error code otherwise.  **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_message_decrypt(
  const char                         *inbuf,
  char                               *outbuf,
  nas_message_security_header_t      *header,
  int                                 length,
  void                               *security)
{
  LOG_FUNC_IN;

  emm_security_context_t *emm_security_context   = (emm_security_context_t*)security;
  int                     bytes                  = length;

  /* Decode the header */
  int size = _nas_message_header_decode(inbuf, header, length);

  if (size < 0) {
    LOG_TRACE(DEBUG, "MESSAGE TOO SHORT");
    LOG_FUNC_RETURN (TLV_DECODE_BUFFER_TOO_SHORT);
  } else if (size > 1) {
    if (emm_security_context) {
#if defined(NAS_MME)

      if (emm_security_context->ul_count.seq_num > header->sequence_number) {
        emm_security_context->ul_count.overflow += 1;
      }

      emm_security_context->ul_count.seq_num = header->sequence_number;
#else

      if (emm_security_context->dl_count.seq_num > header->sequence_number) {
        emm_security_context->dl_count.overflow += 1;
      }

      emm_security_context->dl_count.seq_num = header->sequence_number;
#endif
    }

    /* Compute offset of the sequence number field */
    int offset = size - sizeof(uint8_t);
    /* Compute the NAS message authentication code */
    uint32_t mac = _nas_message_get_mac(
                     inbuf + offset,
                     length - offset,
#ifdef NAS_MME
                     SECU_DIRECTION_UPLINK,
#else
                     SECU_DIRECTION_DOWNLINK,
#endif
                     emm_security_context);

    /* Check NAS message integrity */
    if (mac != header->message_authentication_code) {

      LOG_TRACE(DEBUG,
                "MAC Failure MSG:%08X(%u) <> INT ALGO:%08X(%u) Type of security context %u",
                header->message_authentication_code,header->message_authentication_code,
                mac,mac, (emm_security_context!=NULL)?emm_security_context->type:88);
#if defined(NAS_MME)
      LOG_FUNC_RETURN (TLV_DECODE_MAC_MISMATCH);
#else
//#warning "added test on integrity algorithm because of SECURITY_MODE_COMMAND not correctly handled in UE (check integrity)"

      if (emm_security_context->selected_algorithms.integrity !=
          NAS_SECURITY_ALGORITHMS_EIA0) {
        LOG_FUNC_RETURN (TLV_DECODE_MAC_MISMATCH);
      } else {
        LOG_TRACE(WARNING,
                  "MAC failure but continue due to EIA0 selected");
      }

#endif
    } else {
      LOG_TRACE(DEBUG, "Integrity: MAC Success");
    }

    /* Decrypt the security protected NAS message */
    header->protocol_discriminator =
      _nas_message_decrypt(outbuf, inbuf + size,
                           header->security_header_type,
                           header->message_authentication_code,
                           header->sequence_number,
                           length - size,
                           emm_security_context);
    bytes = length - size;
  } else {
    LOG_TRACE(DEBUG, "Plain NAS message found");
    /* The input buffer contains a plain NAS message */
    memcpy(outbuf, inbuf, length);
  }

  LOG_FUNC_RETURN (bytes);
}

/****************************************************************************
 **                                                                        **
 ** Name:  nas_message_decode()                                      **
 **                                                                        **
 ** Description: Decode layer 3 NAS message                                **
 **                                                                        **
 ** Inputs:  buffer:  Pointer to the buffer containing layer 3   **
 **       NAS message data                           **
 **      length:  Number of bytes that should be decoded     **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   msg:   L3 NAS message structure to be filled      **
 **      Return:  The number of bytes in the buffer if the   **
 **       data have been successfully decoded;       **
 **       A negative error code otherwise.           **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_message_decode(
  const char* const   buffer,
  nas_message_t      *msg,
  int                 length,
  void               *security)
{
  LOG_FUNC_IN;
  emm_security_context_t *emm_security_context   = (emm_security_context_t*)security;
  int bytes;

  /* Decode the header */
  int size = _nas_message_header_decode(buffer, &msg->header, length);

  if (size < 0) {
    LOG_FUNC_RETURN (TLV_DECODE_BUFFER_TOO_SHORT);
  } else if (size > 1) {
    if (emm_security_context) {
#if defined(NAS_MME)

      if (emm_security_context->ul_count.seq_num > msg->header.sequence_number) {
        emm_security_context->ul_count.overflow += 1;
      }

      emm_security_context->ul_count.seq_num = msg->header.sequence_number;

#else

      if (emm_security_context->dl_count.seq_num > msg->header.sequence_number) {
        emm_security_context->dl_count.overflow += 1;
      }

      emm_security_context->dl_count.seq_num = msg->header.sequence_number;
#endif
    }

    /* Compute offset of the sequence number field */
    int offset = size - sizeof(uint8_t);
    /* Compute the NAS message authentication code */
    uint32_t mac = _nas_message_get_mac(
                     buffer + offset,
                     length - offset,
#ifdef NAS_MME
                     SECU_DIRECTION_UPLINK,
#else
                     SECU_DIRECTION_DOWNLINK,
#endif
                     emm_security_context
                   );

#define NAS_CODE_TO_BE_MODIFIED 1
#ifdef NAS_CODE_TO_BE_MODIFIED

    // According to 3GPP TS 24.301 version 10.15.0 Release 10, 4.4.4.3 Integrity checking of NAS signalling messages in the MME
    if ((!emm_security_context) && (mac == 0) && (msg->header.message_authentication_code != 0)) {
      // force mac to be the same, but we should check for message types.
      // TODO Reverse order of processing in NAS code: decode message type then check MAC
      LOG_TRACE(DEBUG,
                "Forced computed MAC to be the MSG MAC %04x",
                msg->header.message_authentication_code);
      mac = msg->header.message_authentication_code;
      LOG_TRACE(DEBUG,
                "Forced computed MAC to be the same as MAC in message %04x",
                msg->header.message_authentication_code);
    }

#endif

    /* Check NAS message integrity */
    if (mac != msg->header.message_authentication_code) {
      LOG_TRACE(DEBUG,
                "msg->header.message_authentication_code = %04X computed = %04X",
                msg->header.message_authentication_code,
                mac);
      LOG_FUNC_RETURN (TLV_DECODE_MAC_MISMATCH);
    }

#if ((defined(NAS_BUILT_IN_EPC) && defined(NAS_MME)) || (defined(ENABLE_NAS_UE_LOGGING) && defined(NAS_BUILT_IN_UE) && defined(NAS_UE)))
    /* Log message header */
#endif

    /* Decode security protected NAS message */
    bytes = _nas_message_protected_decode(buffer + size,
                                          &msg->header,
                                          &msg->plain,
                                          length - size,
                                          emm_security_context);
  } else {
    /* Decode plain NAS message */
    bytes = _nas_message_plain_decode(buffer,
                                      &msg->header,
                                      &msg->plain,
                                      length);
  }

  if (bytes < 0) {
    LOG_FUNC_RETURN (bytes);
  }

  if (size > 1) {
    LOG_FUNC_RETURN (size + bytes);
  }

  LOG_FUNC_RETURN (bytes);
}

/****************************************************************************
 **                                                                        **
 ** Name:  nas_message_encode()                                      **
 **                                                                        **
 ** Description: Encode layer 3 NAS message                                **
 **                                                                        **
 ** Inputs   msg:   L3 NAS message structure to encode         **
 **    length:  Maximal capacity of the output buffer      **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of bytes in the buffer if the   **
 **       data have been successfully encoded;       **
 **       A negative error code otherwise.           **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
int nas_message_encode(
  char                *buffer,
  const nas_message_t *const msg,
  int                  length,
  void                *security)
{
  LOG_FUNC_IN;

  emm_security_context_t *emm_security_context   = (emm_security_context_t*)security;
  int bytes;


  /* Encode the header */
  int size = _nas_message_header_encode(buffer, &msg->header, length);

  if (size < 0) {
    LOG_FUNC_RETURN (TLV_ENCODE_BUFFER_TOO_SHORT);
  } else if (size > 1) {
    /* Encode security protected NAS message */
    bytes = _nas_message_protected_encode(
              buffer + size,
              &msg->security_protected,
              length - size,
              emm_security_context);

    /* Integrity protect the NAS message */
    if (bytes > 0) {
      /* Compute offset of the sequence number field */
      int offset = size - sizeof(uint8_t);
      /* Compute the NAS message authentication code */
      LOG_TRACE(DEBUG,
                "offset %d = %d - %d, hdr encode = %d, length = %d bytes = %d",
                offset, size, (int)sizeof(uint8_t),
                size, length, bytes);
      uint32_t mac = _nas_message_get_mac(
                       buffer + offset,
                       bytes + size - offset,
#ifdef NAS_MME
                       SECU_DIRECTION_DOWNLINK,
#else
                       SECU_DIRECTION_UPLINK,
#endif
                       emm_security_context);
      /* Set the message authentication code of the NAS message */
      *(uint32_t*)(buffer + sizeof(uint8_t)) = htonl(mac);

      if (emm_security_context) {
#ifdef NAS_MME
        /* TS 124.301, section 4.4.3.1
         * The NAS sequence number part of the NAS COUNT shall be
         * exchanged between the UE and the MME as part of the
         * NAS signalling. After each new or retransmitted outbound
         * security protected NAS message, the sender shall increase
         * the NAS COUNT number by one. Specifically, on the sender
         * side, the NAS sequence number shall be increased by one,
         * and if the result is zero (due to wrap around), the NAS
         * overflow counter shall also be incremented by one (see
         * subclause 4.4.3.5).
         */

        emm_security_context->dl_count.seq_num += 1;

        if ( ! emm_security_context->dl_count.seq_num) {
          emm_security_context->dl_count.overflow += 1;
        }

        LOG_TRACE(DEBUG,
                  "Incremented emm_security_context.dl_count.seq_num -> %u",
                  emm_security_context->dl_count.seq_num);
#else
        emm_security_context->ul_count.seq_num += 1;

        if ( ! emm_security_context->ul_count.seq_num) {
          emm_security_context->ul_count.overflow += 1;
        }

        LOG_TRACE(DEBUG,
                  "Incremented emm_security_context.ul_count.seq_num -> %u",
                  emm_security_context->ul_count.seq_num);
#endif
      } else {
        LOG_TRACE(DEBUG,
                  "Did not increment emm_security_context.dl_count.seq_num because no security context");
      }
    }

#if ((defined(NAS_BUILT_IN_EPC) && defined(NAS_MME)) || (defined(ENABLE_NAS_UE_LOGGING) && defined(NAS_BUILT_IN_UE) && defined(NAS_UE)))
    /* Log message header */
#endif
  } else {
    /* Encode plain NAS message */
    bytes = _nas_message_plain_encode(buffer, &msg->header,
                                      &msg->plain, length);
  }

  if (bytes < 0) {
    LOG_FUNC_RETURN (bytes);
  }

  if (size > 1) {
    LOG_FUNC_RETURN (size + bytes);
  }

  LOG_FUNC_RETURN (bytes);
}

/****************************************************************************/
/*********************  L O C A L    F U N C T I O N S  *********************/
/****************************************************************************/

/*
 * -----------------------------------------------------------------------------
 *    Functions used to decode layer 3 NAS messages
 * -----------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:  _nas_message_header_decode()                              **
 **                                                                        **
 ** Description: Decode header of a security protected NAS message         **
 **                                                                        **
 ** Inputs:  buffer:  Pointer to the buffer containing layer 3   **
 **       message data                               **
 **      length:  Number of bytes that should be decoded     **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   header:  Security header structure to be filled     **
 **      Return:  The size in bytes of the security header   **
 **       if data have been successfully decoded;    **
 **       1, if the header is not a security header  **
 **       (header of plain NAS message);             **
 **       -1 otherwise.                              **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int
_nas_message_header_decode(
  const char                         *buffer,
  nas_message_security_header_t      *header,
  int                                 length)
{
  LOG_FUNC_IN;

  int size = 0;

  /* Decode the first octet of the header (security header type or EPS bearer
   * identity, and protocol discriminator) */
  DECODE_U8(buffer, *(uint8_t*)(header), size);

  if (header->protocol_discriminator == EPS_MOBILITY_MANAGEMENT_MESSAGE) {
    if (header->security_header_type != SECURITY_HEADER_TYPE_NOT_PROTECTED) {
      if (length < NAS_MESSAGE_SECURITY_HEADER_SIZE) {
        /* The buffer is not big enough to contain security header */
        LOG_TRACE(WARNING, "NET-API   - The size of the header (%u) "
                  "exceeds the buffer length (%u)",
                  NAS_MESSAGE_SECURITY_HEADER_SIZE, length);
        LOG_FUNC_RETURN (-1);
      }

      /* Decode the message authentication code */
      DECODE_U32(buffer+size, header->message_authentication_code, size);
      /* Decode the sequence number */
      DECODE_U8(buffer+size, header->sequence_number, size);
    }
  }

  LOG_FUNC_RETURN (size);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _nas_message_plain_decode()                               **
 **                                                                        **
 ** Description: Decode plain NAS message                                  **
 **                                                                        **
 ** Inputs:  buffer:  Pointer to the buffer containing plain NAS **
 **       message data                               **
 **    header:  Header of the plain NAS message            **
 **      length:  Number of bytes that should be decoded     **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   msg:   Decoded NAS message                        **
 **      Return:  The number of bytes in the buffer if the   **
 **       data have been successfully decoded;       **
 **       A negative error code otherwise.           **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int
_nas_message_plain_decode(
  const char                          *buffer,
  const nas_message_security_header_t *header,
  nas_message_plain_t                 *msg,
  int                                 length)
{
  LOG_FUNC_IN;

  int bytes = TLV_DECODE_PROTOCOL_NOT_SUPPORTED;

  if (header->protocol_discriminator == EPS_MOBILITY_MANAGEMENT_MESSAGE) {
    /* Decode EPS Mobility Management L3 message */
    bytes = emm_msg_decode(&msg->emm, (uint8_t *)buffer, length);
  } else if (header->protocol_discriminator == EPS_SESSION_MANAGEMENT_MESSAGE) {
    /* Decode EPS Session Management L3 message */
    bytes = esm_msg_decode(&msg->esm, (uint8_t *)buffer, length);
  } else {
    /* Discard L3 messages with not supported protocol discriminator */
    LOG_TRACE(WARNING,"NET-API   - Protocol discriminator 0x%x is "
              "not supported", header->protocol_discriminator);
  }

  LOG_FUNC_RETURN (bytes);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _nas_message_protected_decode()                               **
 **                                                                        **
 ** Description: Decode security protected NAS message                     **
 **                                                                        **
 ** Inputs:  buffer:  Pointer to the buffer containing the secu-           **
 **                     rity protected NAS message data                    **
 **          header:  Header of the security protected NAS message       **
 **      length:  Number of bytes that should be decoded             **
 **      emm_security_context: security context                       **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   msg:   Decoded NAS message                        **
 **      Return:  The number of bytes in the buffer if the   **
 **       data have been successfully decoded;       **
 **       A negative error code otherwise.           **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _nas_message_protected_decode(
  const char                         *buffer,
  nas_message_security_header_t      *header,
  nas_message_plain_t                *msg,
  int                                 length,
  const emm_security_context_t * const emm_security_context)
{
  LOG_FUNC_IN;

  int bytes = TLV_DECODE_BUFFER_TOO_SHORT;

  char* plain_msg = (char*)calloc(1,length);

  if (plain_msg) {
    /* Decrypt the security protected NAS message */
    header->protocol_discriminator =
      _nas_message_decrypt(plain_msg,
                           buffer,
                           header->security_header_type,
                           header->message_authentication_code,
                           header->sequence_number,
                           length,
                           emm_security_context);
    /* Decode the decrypted message as plain NAS message */
    bytes = _nas_message_plain_decode(plain_msg, header, msg, length);
    free(plain_msg);
  }

  LOG_FUNC_RETURN (bytes);
}

/*
 * -----------------------------------------------------------------------------
 *    Functions used to encode layer 3 NAS messages
 * -----------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:  _nas_message_header_encode()                              **
 **                                                                        **
 ** Description: Encode header of a security protected NAS message         **
 **                                                                        **
 ** Inputs   header:  Security header structure to encode        **
 **    length:  Maximal capacity of the output buffer      **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of bytes in the buffer if the   **
 **       data have been successfully encoded;       **
 **       1, if the header is not a security header  **
 **       (header of plain NAS message);             **
 **       -1 otherwise.                              **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _nas_message_header_encode(
  char                                       *buffer,
  const nas_message_security_header_t        *header,
  int                                         length)
{
  LOG_FUNC_IN;

  int size = 0;

  /* Encode the first octet of the header (security header type or EPS bearer
   * identity, and protocol discriminator) */
  ENCODE_U8(buffer, *(uint8_t*)(header), size);

  if (header->protocol_discriminator == EPS_MOBILITY_MANAGEMENT_MESSAGE) {
    if (header->security_header_type != SECURITY_HEADER_TYPE_NOT_PROTECTED) {
      //static uint8_t seq = 0;
      if (length < NAS_MESSAGE_SECURITY_HEADER_SIZE) {
        /* The buffer is not big enough to contain security header */
        LOG_TRACE(WARNING, "NET-API   - The size of the header (%u) "
                  "exceeds the buffer length (%u)",
                  NAS_MESSAGE_SECURITY_HEADER_SIZE, length);
        LOG_FUNC_RETURN (-1);
      }

      /* Encode the message authentication code */
      ENCODE_U32(buffer+size, header->message_authentication_code, size);
      /* Encode the sequence number */
      ENCODE_U8(buffer+size, header->sequence_number, size);
      //ENCODE_U8(buffer+size, seq, size);
      //seq++;
    }
  }

  LOG_FUNC_RETURN (size);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _nas_message_plain_encode()                               **
 **                                                                        **
 ** Description: Encode plain NAS message                                  **
 **                                                                        **
 ** Inputs:  pd:    Protocol discriminator of the NAS message  **
 **       to encode                                  **
 **      msg:   Plain NAS message structure to encode      **
 **    length:  Maximal capacity of the output buffer      **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   buffer:  Pointer to the encoded data buffer         **
 **      Return:  The number of bytes in the buffer if the   **
 **       data have been successfully encoded;       **
 **       A negative error code otherwise.           **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _nas_message_plain_encode(
  char                                *buffer,
  const nas_message_security_header_t *header,
  const nas_message_plain_t           *msg,
  int                                  length)
{
  LOG_FUNC_IN;

  int bytes = TLV_ENCODE_PROTOCOL_NOT_SUPPORTED;

  if (header->protocol_discriminator == EPS_MOBILITY_MANAGEMENT_MESSAGE) {
    /* Encode EPS Mobility Management L3 message */
    bytes = emm_msg_encode((EMM_msg*)(&msg->emm), (uint8_t*)buffer, length);
  } else if (header->protocol_discriminator == EPS_SESSION_MANAGEMENT_MESSAGE) {
    /* Encode EPS Session Management L3 message */
    bytes = esm_msg_encode((ESM_msg*)(&msg->esm), (uint8_t*)buffer, length);
  } else {
    /* Discard L3 messages with not supported protocol discriminator */
    LOG_TRACE(WARNING,"NET-API   - Protocol discriminator 0x%x is "
              "not supported", header->protocol_discriminator);
  }

  LOG_FUNC_RETURN (bytes);
}

/****************************************************************************
 **                                                                        **
 ** Name:  _nas_message_protected_encode()                               **
 **                                                                        **
 ** Description: Encode security protected NAS message                     **
 **                                                                        **
 ** Inputs    msg:     Security protected NAS message structure            **
 **                    to encode                                           **
 **           length:  Maximal capacity of the output buffer               **
 **           Others:  None                                                **
 **                                                                        **
 ** Outputs:  buffer:  Pointer to the encoded data buffer                  **
 ** Return:   The number of bytes in the buffer if the                     **
 **           data have been successfully encoded;                         **
 **           A negative error code otherwise.                             **
 ** Others:   None                                                         **
 **                                                                        **
 ***************************************************************************/
static int _nas_message_protected_encode(
  char                                       *buffer,
  const nas_message_security_protected_t     *msg,
  int                                         length,
  void                                       *security)
{
  LOG_FUNC_IN;

  emm_security_context_t *emm_security_context   = (emm_security_context_t*)security;
  int bytes = TLV_ENCODE_BUFFER_TOO_SHORT;

  char* plain_msg = (char*)calloc(1,length);

  if (plain_msg) {
    /* Encode the security protected NAS message as plain NAS message */
    int size = _nas_message_plain_encode(plain_msg, &msg->header,
                                         &msg->plain, length);

    if (size > 0) {
      //static uint8_t seq = 0;
      /* Encrypt the encoded plain NAS message */
      bytes = _nas_message_encrypt(buffer,
                                   plain_msg,
                                   msg->header.security_header_type,
                                   msg->header.message_authentication_code,
                                   msg->header.sequence_number,
#ifdef NAS_MME
                                   SECU_DIRECTION_DOWNLINK,
#else
                                   SECU_DIRECTION_UPLINK,
#endif
                                   size,
                                   emm_security_context);
      //seq, size);
      //seq ++;
    }

    free(plain_msg);
  }

  LOG_FUNC_RETURN (bytes);
}

/*
 * -----------------------------------------------------------------------------
 *      Functions used to decrypt and encrypt layer 3 NAS messages
 * -----------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:  _nas_message_decrypt()                                    **
 **                                                                        **
 ** Description: Decrypt security protected NAS message                    **
 **                                                                        **
 ** Inputs   src:   Pointer to the encrypted data buffer       **
 **    security_header_type:    The security header type                   **
 **    code:    The message authentication code            **
 **    seq:   The sequence number                        **
 **    length:  Maximal capacity of the output buffer      **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   dest:    Pointer to the decrypted data buffer       **
 **      Return:  The protocol discriminator of the message  **
 **       that has been decrypted;                   **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _nas_message_decrypt(
  char               *dest,
  const char         *src,
  uint8_t             security_header_type,
  uint32_t            code,
  uint8_t             seq,
  int                 length,
  const emm_security_context_t * const emm_security_context)
{
  nas_stream_cipher_t stream_cipher;
  uint32_t            count;
  uint8_t             direction;

  LOG_FUNC_IN;

  int size = 0;
  nas_message_security_header_t header;

#ifdef NAS_MME
  direction = SECU_DIRECTION_UPLINK;
#else
  direction = SECU_DIRECTION_DOWNLINK;
#endif

  switch (security_header_type) {
  case SECURITY_HEADER_TYPE_NOT_PROTECTED:
  case SECURITY_HEADER_TYPE_SERVICE_REQUEST:
  case SECURITY_HEADER_TYPE_INTEGRITY_PROTECTED:
  case SECURITY_HEADER_TYPE_INTEGRITY_PROTECTED_NEW:
    LOG_TRACE(DEBUG,
              "No decryption of message length %u according to security header type 0x%02x",
              length, security_header_type);
    memcpy(dest, src, length);
    DECODE_U8(dest, *(uint8_t*)(&header), size);
    LOG_FUNC_RETURN (header.protocol_discriminator);
    //LOG_FUNC_RETURN (length);
    break;

  case SECURITY_HEADER_TYPE_INTEGRITY_PROTECTED_CYPHERED:
  case SECURITY_HEADER_TYPE_INTEGRITY_PROTECTED_CYPHERED_NEW:
    switch (emm_security_context->selected_algorithms.encryption) {

    case NAS_SECURITY_ALGORITHMS_EEA1: {
      if (direction == SECU_DIRECTION_UPLINK) {
        count = 0x00000000 |
                ((emm_security_context->ul_count.overflow & 0x0000FFFF) << 8) |
                (emm_security_context->ul_count.seq_num & 0x000000FF);
      } else {
        count = 0x00000000 |
                ((emm_security_context->dl_count.overflow & 0x0000FFFF) << 8) |
                (emm_security_context->dl_count.seq_num & 0x000000FF);
      }

      LOG_TRACE(DEBUG,
                "NAS_SECURITY_ALGORITHMS_EEA1 dir %s count.seq_num %u count %u",
                (direction == SECU_DIRECTION_UPLINK) ? "UPLINK":"DOWNLINK",
                (direction == SECU_DIRECTION_UPLINK) ? emm_security_context->ul_count.seq_num:emm_security_context->dl_count.seq_num,
                count);
      stream_cipher.key        = emm_security_context->knas_enc.value;
      stream_cipher.key_length = AUTH_KNAS_ENC_SIZE;
      stream_cipher.count      = count;
      stream_cipher.bearer     = 0x00; //33.401 section 8.1.1
      stream_cipher.direction  = direction;
      stream_cipher.message    = (unsigned char *)src;
      /* length in bits */
      stream_cipher.blength    = length << 3;
      nas_stream_encrypt_eea1(&stream_cipher, (unsigned char *)dest);
      /* Decode the first octet (security header type or EPS bearer identity,
       * and protocol discriminator) */
      DECODE_U8(dest, *(uint8_t*)(&header), size);

      LOG_FUNC_RETURN (header.protocol_discriminator);

    }
    break;

    case NAS_SECURITY_ALGORITHMS_EEA2: {
      if (direction == SECU_DIRECTION_UPLINK) {
        count = 0x00000000 |
                ((emm_security_context->ul_count.overflow & 0x0000FFFF) << 8) |
                (emm_security_context->ul_count.seq_num & 0x000000FF);
      } else {
        count = 0x00000000 |
                ((emm_security_context->dl_count.overflow & 0x0000FFFF) << 8) |
                (emm_security_context->dl_count.seq_num & 0x000000FF);
      }

      LOG_TRACE(DEBUG,
                "NAS_SECURITY_ALGORITHMS_EEA2 dir %s count.seq_num %u count %u",
                (direction == SECU_DIRECTION_UPLINK) ? "UPLINK":"DOWNLINK",
                (direction == SECU_DIRECTION_UPLINK) ? emm_security_context->ul_count.seq_num:emm_security_context->dl_count.seq_num,
                count);
      stream_cipher.key        = emm_security_context->knas_enc.value;
      stream_cipher.key_length = AUTH_KNAS_ENC_SIZE;
      stream_cipher.count      = count;
      stream_cipher.bearer     = 0x00; //33.401 section 8.1.1
      stream_cipher.direction  = direction;
      stream_cipher.message    = (unsigned char *)src;
      /* length in bits */
      stream_cipher.blength    = length << 3;
      nas_stream_encrypt_eea1(&stream_cipher, (unsigned char *)dest);
      /* Decode the first octet (security header type or EPS bearer identity,
       * and protocol discriminator) */
      DECODE_U8(dest, *(uint8_t*)(&header), size);

      LOG_FUNC_RETURN (header.protocol_discriminator);
    }
    break;

    case NAS_SECURITY_ALGORITHMS_EEA0:
      LOG_TRACE(DEBUG,
                "NAS_SECURITY_ALGORITHMS_EEA0 dir %d ul_count.seq_num %d dl_count.seq_num %d",
                direction,
                emm_security_context->ul_count.seq_num,
                emm_security_context->dl_count.seq_num);
      memcpy(dest, src, length);
      /* Decode the first octet (security header type or EPS bearer identity,
       * and protocol discriminator) */
      DECODE_U8(dest, *(uint8_t*)(&header), size);

      LOG_FUNC_RETURN (header.protocol_discriminator);

      break;

    default:
      LOG_TRACE(ERROR,
                "Unknown Cyphering protection algorithm %d",
                emm_security_context->selected_algorithms.encryption);
      memcpy(dest, src, length);
      /* Decode the first octet (security header type or EPS bearer identity,
       * and protocol discriminator) */
      DECODE_U8(dest, *(uint8_t*)(&header), size);

      LOG_FUNC_RETURN (header.protocol_discriminator);
      break;
    }

    break;

  default:
    LOG_TRACE(ERROR,
              "Unknown security header type %u", security_header_type);
    LOG_FUNC_RETURN (0);
  };

}

/****************************************************************************
 **                                                                        **
 ** Name:  _nas_message_encrypt()                                    **
 **                                                                        **
 ** Description: Encrypt plain NAS message                                 **
 **                                                                        **
 ** Inputs   src:   Pointer to the decrypted data buffer       **
 **    security_header_type:    The security header type                   **
 **    code:    The message authentication code            **
 **    seq:   The sequence number                        **
 **    direction: The sequence number                        **
 **    length:  Maximal capacity of the output buffer      **
 **    Others:  None                                       **
 **                                                                        **
 ** Outputs:   dest:    Pointer to the encrypted data buffer       **
 **      Return:  The number of bytes in the output buffer   **
 **       if data have been successfully encrypted;  **
 **       RETURNerror otherwise.                     **
 **    Others:  None                                       **
 **                                                                        **
 ***************************************************************************/
static int _nas_message_encrypt(
  char        *dest,
  const char  *src,
  uint8_t     security_header_type,
  uint32_t    code,
  uint8_t     seq,
  int         const direction,
  int         length,
  const emm_security_context_t * const emm_security_context)
{
  nas_stream_cipher_t stream_cipher;
  uint32_t            count;
  LOG_FUNC_IN;

  if (!emm_security_context) {
    LOG_TRACE(ERROR,
              "No security context set for encryption protection algorithm");
    LOG_FUNC_RETURN (0);
  }

  switch (security_header_type) {
  case SECURITY_HEADER_TYPE_NOT_PROTECTED:
  case SECURITY_HEADER_TYPE_SERVICE_REQUEST:
  case SECURITY_HEADER_TYPE_INTEGRITY_PROTECTED:
  case SECURITY_HEADER_TYPE_INTEGRITY_PROTECTED_NEW:
    LOG_TRACE(DEBUG,
              "No encryption of message according to security header type 0x%02x",
              security_header_type);
    memcpy(dest, src, length);
    LOG_FUNC_RETURN (length);
    break;

  case SECURITY_HEADER_TYPE_INTEGRITY_PROTECTED_CYPHERED:
  case SECURITY_HEADER_TYPE_INTEGRITY_PROTECTED_CYPHERED_NEW:
    switch (emm_security_context->selected_algorithms.encryption) {

    case NAS_SECURITY_ALGORITHMS_EEA1: {
      if (direction == SECU_DIRECTION_UPLINK) {
        count = 0x00000000 |
                ((emm_security_context->ul_count.overflow & 0x0000FFFF) << 8) |
                (emm_security_context->ul_count.seq_num & 0x000000FF);
      } else {
        count = 0x00000000 |
                ((emm_security_context->dl_count.overflow & 0x0000FFFF) << 8) |
                (emm_security_context->dl_count.seq_num & 0x000000FF);
      }

      LOG_TRACE(DEBUG,
                "NAS_SECURITY_ALGORITHMS_EEA1 dir %s count.seq_num %u count %u",
                (direction == SECU_DIRECTION_UPLINK) ? "UPLINK":"DOWNLINK",
                (direction == SECU_DIRECTION_UPLINK) ? emm_security_context->ul_count.seq_num:emm_security_context->dl_count.seq_num,
                count);
      stream_cipher.key        = emm_security_context->knas_enc.value;
      stream_cipher.key_length = AUTH_KNAS_ENC_SIZE;
      stream_cipher.count      = count;
      stream_cipher.bearer     = 0x00; //33.401 section 8.1.1
      stream_cipher.direction  = direction;
      stream_cipher.message    = (unsigned char *)src;
      /* length in bits */
      stream_cipher.blength    = length << 3;
      nas_stream_encrypt_eea1(&stream_cipher, (unsigned char *)dest);

      LOG_FUNC_RETURN (length);

    }
    break;

    case NAS_SECURITY_ALGORITHMS_EEA2: {
      if (direction == SECU_DIRECTION_UPLINK) {
        count = 0x00000000 |
                ((emm_security_context->ul_count.overflow & 0x0000FFFF) << 8) |
                (emm_security_context->ul_count.seq_num & 0x000000FF);
      } else {
        count = 0x00000000 |
                ((emm_security_context->dl_count.overflow & 0x0000FFFF) << 8) |
                (emm_security_context->dl_count.seq_num & 0x000000FF);
      }

      LOG_TRACE(DEBUG,
                "NAS_SECURITY_ALGORITHMS_EEA2 dir %s count.seq_num %u count %u",
                (direction == SECU_DIRECTION_UPLINK) ? "UPLINK":"DOWNLINK",
                (direction == SECU_DIRECTION_UPLINK) ? emm_security_context->ul_count.seq_num:emm_security_context->dl_count.seq_num,
                count);
      stream_cipher.key        = emm_security_context->knas_enc.value;
      stream_cipher.key_length = AUTH_KNAS_ENC_SIZE;
      stream_cipher.count      = count;
      stream_cipher.bearer     = 0x00; //33.401 section 8.1.1
      stream_cipher.direction  = direction;
      stream_cipher.message    = (unsigned char *)src;
      /* length in bits */
      stream_cipher.blength    = length << 3;
      nas_stream_encrypt_eea2(&stream_cipher, (unsigned char *)dest);

      LOG_FUNC_RETURN (length);

    }
    break;

    case NAS_SECURITY_ALGORITHMS_EEA0:
      LOG_TRACE(DEBUG,
                "NAS_SECURITY_ALGORITHMS_EEA0 dir %d ul_count.seq_num %d dl_count.seq_num %d",
                direction,
                emm_security_context->ul_count.seq_num,
                emm_security_context->dl_count.seq_num);
      memcpy(dest, src, length);
      LOG_FUNC_RETURN (length);

      break;

    default:
      LOG_TRACE(ERROR,
                "Unknown Cyphering protection algorithm %d",
                emm_security_context->selected_algorithms.encryption);
      break;
    }

    break;

  default:
    LOG_TRACE(ERROR,
              "Unknown security header type %u", security_header_type);
    LOG_FUNC_RETURN (0);
  }

  LOG_FUNC_RETURN (length);
}

/*
 * -----------------------------------------------------------------------------
 *  Functions used for integrity protection of layer 3 NAS messages
 * -----------------------------------------------------------------------------
 */

/****************************************************************************
 **                                                                        **
 ** Name:  _nas_message_get_mac()                                        **
 **                                                                        **
 ** Description: Run integrity algorithm onto cyphered or uncyphered NAS   **
 **    message encoded in the input buffer and return the compu-         **
 **    ted message authentication code                                   **
 **                                                                        **
 ** Inputs   buffer:  Pointer to the integrity protected data            **
 **       buffer                                                     **
 **    count:   Value of the uplink NAS counter                        **
 **    length:  Length of the input buffer                             **
 **      direction                                                         **
 **    Others:  None                                                   **
 **                                                                        **
 ** Outputs:   None                                                      **
 **      Return:  The message authentication code                    **
 **    Others:  None                                                   **
 **                                                                        **
 ***************************************************************************/
static uint32_t _nas_message_get_mac(
  const char                   * const buffer,
  int                            const length,
  int                            const direction,
  const emm_security_context_t * const emm_security_context)
{

  LOG_FUNC_IN;

  if (!emm_security_context) {
    LOG_TRACE(DEBUG,
              "No security context set for integrity protection algorithm");
#if defined(NAS_BUILT_IN_EPC) || defined(NAS_BUILT_IN_UE)
    LOG_FUNC_RETURN (0);
#else
    LOG_FUNC_RETURN (0xabababab);
#endif
  }

  switch (emm_security_context->selected_algorithms.integrity) {

  case NAS_SECURITY_ALGORITHMS_EIA1: {
    uint8_t             mac[4];
    nas_stream_cipher_t stream_cipher;
    uint32_t            count;
    uint32_t           *mac32;

#ifdef DEBUG_NAS_MESSAGE
    int i,bytes = 0;
#endif

    if (direction == SECU_DIRECTION_UPLINK) {
      count = 0x00000000 |
              ((emm_security_context->ul_count.overflow & 0x0000FFFF) << 8) |
              (emm_security_context->ul_count.seq_num & 0x000000FF);
    } else {
      count = 0x00000000 |
              ((emm_security_context->dl_count.overflow & 0x0000FFFF) << 8) |
              (emm_security_context->dl_count.seq_num & 0x000000FF);
    }

    LOG_TRACE(DEBUG,
              "NAS_SECURITY_ALGORITHMS_EIA1 dir %s count.seq_num %u count %u",
              (direction == SECU_DIRECTION_UPLINK) ? "UPLINK":"DOWNLINK",
              (direction == SECU_DIRECTION_UPLINK) ? emm_security_context->ul_count.seq_num:emm_security_context->dl_count.seq_num,
              count);

#ifdef DEBUG_NAS_MESSAGE
    fprintf(stderr, "\n[NAS]\t");

    for (i=0; i < length; i++) {
      fprintf(stderr, "%.2hx ", (const unsigned char) buffer[i]);

      /* Add new line when the number of displayed bytes exceeds
       * the line's size */
      if ( ++bytes > (16 - 1) ) {
        bytes = 0;
        fprintf(stderr, "\n[NAS]\t");
      }
    }

    if (bytes % 16) {
      fprintf(stderr, "\n");
    }

    fprintf(stderr, "\n");
    fflush(stderr);
#endif

    stream_cipher.key        = emm_security_context->knas_int.value;
    stream_cipher.key_length = AUTH_KNAS_INT_SIZE;
    stream_cipher.count      = count;
    stream_cipher.bearer     = 0x00; //33.401 section 8.1.1
    stream_cipher.direction  = direction;
    stream_cipher.message    = (unsigned char *)buffer;
    /* length in bits */
    stream_cipher.blength    = length << 3;

    nas_stream_encrypt_eia1(
      &stream_cipher,
      mac);
    LOG_TRACE(DEBUG,
              "NAS_SECURITY_ALGORITHMS_EIA1 returned MAC %x.%x.%x.%x(%u) for length %d direction %d, count %d",
              mac[0], mac[1], mac[2],mac[3],
              *((uint32_t*)&mac),
              length,
              direction,
              count);
    mac32 = (uint32_t*)&mac;
    LOG_FUNC_RETURN (ntohl(*mac32));
  }
  break;

  case NAS_SECURITY_ALGORITHMS_EIA2: {
    uint8_t             mac[4];
    nas_stream_cipher_t stream_cipher;
    uint32_t            count;
    uint32_t           *mac32;

    if (direction == SECU_DIRECTION_UPLINK) {
      count = 0x00000000 |
              ((emm_security_context->ul_count.overflow & 0x0000FFFF) << 8) |
              (emm_security_context->ul_count.seq_num & 0x000000FF);
    } else {
      count = 0x00000000 |
              ((emm_security_context->dl_count.overflow & 0x0000FFFF) << 8) |
              (emm_security_context->dl_count.seq_num & 0x000000FF);
    }

    LOG_TRACE(DEBUG,
              "NAS_SECURITY_ALGORITHMS_EIA2 dir %s count.seq_num %u count %u",
              (direction == SECU_DIRECTION_UPLINK) ? "UPLINK":"DOWNLINK",
              (direction == SECU_DIRECTION_UPLINK) ? emm_security_context->ul_count.seq_num:emm_security_context->dl_count.seq_num,
              count);

    stream_cipher.key        = emm_security_context->knas_int.value;
    stream_cipher.key_length = AUTH_KNAS_INT_SIZE;
    stream_cipher.count      = count;
    stream_cipher.bearer     = 0x00; //33.401 section 8.1.1
    stream_cipher.direction  = direction;
    stream_cipher.message    = (unsigned char *)buffer;
    /* length in bits */
    stream_cipher.blength    = length << 3;

    nas_stream_encrypt_eia2(
      &stream_cipher,
      mac);
    LOG_TRACE(DEBUG,
              "NAS_SECURITY_ALGORITHMS_EIA2 returned MAC %x.%x.%x.%x(%u) for length %d direction %d, count %d",
              mac[0], mac[1], mac[2],mac[3],
              *((uint32_t*)&mac),
              length,
              direction,
              count);
    mac32 = (uint32_t*)&mac;
    LOG_FUNC_RETURN (ntohl(*mac32));
  }
  break;

  case NAS_SECURITY_ALGORITHMS_EIA0:
    LOG_TRACE(DEBUG,
              "NAS_SECURITY_ALGORITHMS_EIA0 dir %s count.seq_num %u",
              (direction == SECU_DIRECTION_UPLINK) ? "UPLINK":"DOWNLINK",
              (direction == SECU_DIRECTION_UPLINK) ? emm_security_context->ul_count.seq_num:emm_security_context->dl_count.seq_num
             );

#if defined(NAS_BUILT_IN_EPC) || defined(NAS_BUILT_IN_UE)
    LOG_FUNC_RETURN (0);
#else
    LOG_FUNC_RETURN (0xabababab);
#endif
    break;

  default:
    LOG_TRACE(ERROR,
              "Unknown integrity protection algorithm %d",
              emm_security_context->selected_algorithms.integrity);
    break;
  }

  LOG_FUNC_RETURN (0);
}


