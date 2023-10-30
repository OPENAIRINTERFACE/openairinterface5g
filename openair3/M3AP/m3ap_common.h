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
#ifndef M3AP_COMMON_H_
#define M3AP_COMMON_H_
#include "oai_asn1.h"
#include "M3AP_ProtocolIE-Field.h"
#include "M3AP_M3AP-PDU.h"
#include "M3AP_InitiatingMessage.h"
#include "M3AP_SuccessfulOutcome.h"
#include "M3AP_UnsuccessfulOutcome.h"
#include "M3AP_ProtocolIE-FieldPair.h"
#include "M3AP_ProtocolIE-ContainerPair.h"
#include "M3AP_ProtocolExtensionField.h"
#include "M3AP_ProtocolExtensionContainer.h"
#include "M3AP_asn_constant.h"
#include "intertask_interface.h"


/** @defgroup _m3ap_impl_ M3AP Layer Reference Implementation
 * @ingroup _ref_implementation_
 * @{
 */

/* Checking version of ASN1C compiler */
#if (ASN1C_ENVIRONMENT_VERSION < 923)
# error "You are compiling m3ap with the wrong version of ASN1C"
#endif

#ifndef M3AP_PORT
# define M3AP_PORT 36444
#endif

extern int asn1_xer_print;

#include "common/utils/LOG/log.h"
#include "m3ap_default_values.h"
#define M3AP_INFO(x, args...) LOG_I(M3AP, x, ##args)
#define M3AP_ERROR(x, args...) LOG_E(M3AP, x, ##args)
#define M3AP_WARN(x, args...)  LOG_W(M3AP, x, ##args)
#define M3AP_DEBUG(x, args...) LOG_D(M3AP, x, ##args)

#define M3AP_FIND_PROTOCOLIE_BY_ID(IE_TYPE, ie, container, IE_ID, mandatory) \
  do {\
    IE_TYPE **ptr; \
    ie = NULL; \
    for (ptr = container->protocolIEs.list.array; \
         ptr < &container->protocolIEs.list.array[container->protocolIEs.list.count]; \
         ptr++) { \
      if((*ptr)->id == IE_ID) { \
        ie = *ptr; \
        break; \
      } \
    } \
    if (mandatory) DevAssert(ie != NULL); \
  } while(0)

/** \brief Function callback prototype.
 **/
typedef int (*m3ap_message_decoded_callback)(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, M3AP_M3AP_PDU_t *pdu);

/** \brief Function callback prototype.
 **/
typedef int (*m3ap_MCE_message_decoded_callback)(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, M3AP_M3AP_PDU_t *pdu);

/** \brief Function callback prototype.
 **/
typedef int (*m3ap_MME_message_decoded_callback)(instance_t instance, sctp_assoc_t assoc_id, uint32_t stream, M3AP_M3AP_PDU_t *pdu);

/** \brief Encode a successfull outcome message
 \param buffer pointer to buffer in which data will be encoded
 \param length pointer to the length of buffer
 \param procedureCode Procedure code for the message
 \param criticality Criticality of the message
 \param td ASN1C type descriptor of the sptr
 \param sptr Deferenced pointer to the structure to encode
 @returns size in bytes encded on success or 0 on failure
 **/
ssize_t m3ap_generate_successfull_outcome(
  uint8_t               **buffer,
  uint32_t               *length,
  M3AP_ProcedureCode_t         procedureCode,
  M3AP_Criticality_t           criticality,
  asn_TYPE_descriptor_t  *td,
  void                   *sptr);

/** \brief Encode an initiating message
 \param buffer pointer to buffer in which data will be encoded
 \param length pointer to the length of buffer
 \param procedureCode Procedure code for the message
 \param criticality Criticality of the message
 \param td ASN1C type descriptor of the sptr
 \param sptr Deferenced pointer to the structure to encode
 @returns size in bytes encded on success or 0 on failure
 **/
ssize_t m3ap_generate_initiating_message(
  uint8_t               **buffer,
  uint32_t               *length,
  M3AP_ProcedureCode_t    procedureCode,
  M3AP_Criticality_t      criticality,
  asn_TYPE_descriptor_t  *td,
  void                   *sptr);

/** \brief Encode an unsuccessfull outcome message
 \param buffer pointer to buffer in which data will be encoded
 \param length pointer to the length of buffer
 \param procedureCode Procedure code for the message
 \param criticality Criticality of the message
 \param td ASN1C type descriptor of the sptr
 \param sptr Deferenced pointer to the structure to encode
 @returns size in bytes encded on success or 0 on failure
 **/
ssize_t m3ap_generate_unsuccessfull_outcome(
  uint8_t               **buffer,
  uint32_t               *length,
  M3AP_ProcedureCode_t         procedureCode,
  M3AP_Criticality_t           criticality,
  asn_TYPE_descriptor_t  *td,
  void                   *sptr);

/** \brief Handle criticality
 \param criticality Criticality of the IE
 @returns void
 **/
void m3ap_handle_criticality(M3AP_Criticality_t criticality);
/** @}*/

#endif /* M3AP_COMMON_H_ */
