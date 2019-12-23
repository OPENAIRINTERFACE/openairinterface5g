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

#if HAVE_CONFIG_H_
# include "config.h"
#endif

#include "M2AP_ProtocolIE-Field.h"
#include "M2AP_M2AP-PDU.h"
#include "M2AP_InitiatingMessage.h"
#include "M2AP_SuccessfulOutcome.h"
#include "M2AP_UnsuccessfulOutcome.h"
#include "M2AP_ProtocolIE-FieldPair.h"
#include "M2AP_ProtocolIE-ContainerPair.h"
#include "M2AP_ProtocolExtensionField.h"
#include "M2AP_ProtocolExtensionContainer.h"
#include "M2AP_PMCH-Configuration-Item.h"
#include "M2AP_asn_constant.h"
#include "intertask_interface.h"

#include "common/ran_context.h"

#ifndef M2AP_COMMON_H_
#define M2AP_COMMON_H_

/** @defgroup _m2ap_impl_ M2AP Layer Reference Implementation
 * @ingroup _ref_implementation_
 * @{
 */

/* Checking version of ASN1C compiler */
#if (ASN1C_ENVIRONMENT_VERSION < 923)
# error "You are compiling m2ap with the wrong version of ASN1C"
#endif

#ifndef M2AP_PORT
# define M2AP_PORT 36423
#endif

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

extern int asn1_xer_print;

#if defined(ENB_MODE)
# include "common/utils/LOG/log.h"
# include "m2ap_default_values.h"
# define M2AP_INFO(x, args...) LOG_I(M2AP, x, ##args)
# define M2AP_ERROR(x, args...) LOG_E(M2AP, x, ##args)
# define M2AP_WARN(x, args...)  LOG_W(M2AP, x, ##args)
# define M2AP_DEBUG(x, args...) LOG_D(M2AP, x, ##args)
#else
# define M2AP_INFO(x, args...) do { fprintf(stdout, "[M2AP][I]"x, ##args); } while(0)
# define M2AP_ERROR(x, args...) do { fprintf(stdout, "[M2AP][E]"x, ##args); } while(0)
# define M2AP_WARN(x, args...)  do { fprintf(stdout, "[M2AP][W]"x, ##args); } while(0)
# define M2AP_DEBUG(x, args...) do { fprintf(stdout, "[M2AP][D]"x, ##args); } while(0)
#endif

#define M2AP_FIND_PROTOCOLIE_BY_ID(IE_TYPE, ie, container, IE_ID, mandatory) \
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
typedef int (*m2ap_message_decoded_callback)(
  instance_t instance,
  uint32_t assocId,
  uint32_t stream,
  M2AP_M2AP_PDU_t *pdu);

typedef int (*m2ap_MCE_message_decoded_callback)(
  instance_t instance,
  uint32_t assocId,
  uint32_t stream,
  M2AP_M2AP_PDU_t *pdu);

typedef int (*m2ap_eNB_message_decoded_callback)(
  instance_t instance,
  uint32_t assocId,
  uint32_t stream,
  M2AP_M2AP_PDU_t *pdu);



/** \brief Encode a successfull outcome message
 \param buffer pointer to buffer in which data will be encoded
 \param length pointer to the length of buffer
 \param procedureCode Procedure code for the message
 \param criticality Criticality of the message
 \param td ASN1C type descriptor of the sptr
 \param sptr Deferenced pointer to the structure to encode
 @returns size in bytes encded on success or 0 on failure
 **/
ssize_t m2ap_generate_successfull_outcome(
  uint8_t               **buffer,
  uint32_t               *length,
  M2AP_ProcedureCode_t         procedureCode,
  M2AP_Criticality_t           criticality,
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
ssize_t m2ap_generate_initiating_message(
  uint8_t               **buffer,
  uint32_t               *length,
  M2AP_ProcedureCode_t    procedureCode,
  M2AP_Criticality_t      criticality,
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
ssize_t m2ap_generate_unsuccessfull_outcome(
  uint8_t               **buffer,
  uint32_t               *length,
  M2AP_ProcedureCode_t         procedureCode,
  M2AP_Criticality_t           criticality,
  asn_TYPE_descriptor_t  *td,
  void                   *sptr);

/** \brief Handle criticality
 \param criticality Criticality of the IE
 @returns void
 **/
void m2ap_handle_criticality(M2AP_Criticality_t criticality);

#endif /* M2AP_COMMON_H_ */
