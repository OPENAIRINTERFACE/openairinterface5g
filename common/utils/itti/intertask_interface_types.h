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

/** @defgroup _intertask_interface_impl_ Intertask Interface Mechanisms
 * Implementation
 * @ingroup _ref_implementation_
 * @{
 */

#ifndef INTERTASK_INTERFACE_TYPES_H_
#define INTERTASK_INTERFACE_TYPES_H_

#include "itti_types.h"
#include "platform_types.h"

/* Defines to handle bit fields on unsigned long values */
#define UL_BIT_MASK(lENGTH)             ((1UL << (lENGTH)) - 1UL)
#define UL_BIT_SHIFT(vALUE, oFFSET)     ((vALUE) << (oFFSET))
#define UL_BIT_UNSHIFT(vALUE, oFFSET)   ((vALUE) >> (oFFSET))

#define UL_FIELD_MASK(oFFSET, lENGTH)                   UL_BIT_SHIFT(UL_BIT_MASK(lENGTH), (oFFSET))
#define UL_FIELD_INSERT(vALUE, fIELD, oFFSET, lENGTH)   (((vALUE) & (~UL_FIELD_MASK(oFFSET, lENGTH))) | UL_BIT_SHIFT(((fIELD) & UL_BIT_MASK(lENGTH)), oFFSET))
#define UL_FIELD_EXTRACT(vALUE, oFFSET, lENGTH)         (UL_BIT_UNSHIFT((vALUE), (oFFSET)) & UL_BIT_MASK(lENGTH))

/* Definitions of task ID fields */
#define TASK_THREAD_ID_OFFSET   8
#define TASK_THREAD_ID_LENGTH   8

#define TASK_SUB_TASK_ID_OFFSET 0
#define TASK_SUB_TASK_ID_LENGTH 8

/* Defines to extract task ID fields */
#define TASK_GET_THREAD_ID(tASKiD)          (itti_desc.tasks_info[tASKiD].thread)
#define TASK_GET_PARENT_TASK_ID(tASKiD)     (itti_desc.tasks_info[tASKiD].parent_task)
/* Extract the instance from a message */
#define ITTI_MESSAGE_GET_INSTANCE(mESSAGE)  ((mESSAGE)->ittiMsgHeader.instance)

#include <messages_types.h>

/* This enum defines messages ids. Each one is unique. */
typedef enum {
#define MESSAGE_DEF(iD, pRIO, sTRUCT, fIELDnAME) iD,
#include <messages_def.h>
#undef MESSAGE_DEF

  MESSAGES_ID_MAX,
} MessagesIds;

//! Thread id of each task
typedef enum {
  THREAD_NULL = 0,

#define TASK_DEF(tHREADiD, pRIO, qUEUEsIZE)             THREAD_##tHREADiD,
#define SUB_TASK_DEF(tHREADiD, sUBtASKiD, qUEUEsIZE)
#include <tasks_def.h>
#undef SUB_TASK_DEF
#undef TASK_DEF

  THREAD_MAX,
  THREAD_FIRST = 1,
} thread_id_t;

//! Sub-tasks id, to defined offset form thread id
typedef enum {
#define TASK_DEF(tHREADiD, pRIO, qUEUEsIZE)             tHREADiD##_THREAD = THREAD_##tHREADiD,
#define SUB_TASK_DEF(tHREADiD, sUBtASKiD, qUEUEsIZE)    sUBtASKiD##_THREAD = THREAD_##tHREADiD,
#include <tasks_def.h>
#undef SUB_TASK_DEF
#undef TASK_DEF
} task_thread_id_t;

//! Tasks id of each task
typedef enum {
  TASK_UNKNOWN = 0,

#define TASK_DEF(tHREADiD, pRIO, qUEUEsIZE)             tHREADiD,
#define SUB_TASK_DEF(tHREADiD, sUBtASKiD, qUEUEsIZE)    sUBtASKiD,
#include <tasks_def.h>
#undef SUB_TASK_DEF
#undef TASK_DEF

  TASK_MAX,
  TASK_FIRST = 1,
} task_id_t;

typedef union msg_s {
#define MESSAGE_DEF(iD, pRIO, sTRUCT, fIELDnAME) sTRUCT fIELDnAME;
#include <messages_def.h>
#undef MESSAGE_DEF
} msg_t;

typedef uint16_t MessageHeaderSize;

typedef struct itti_lte_time_s {
  uint32_t frame;
  uint8_t slot;
} itti_lte_time_t;

/** @struct MessageHeader
 *  @brief Message Header structure for inter-task communication.
 */
typedef struct MessageHeader_s {
  MessagesIds messageId;          /**< Unique message id as referenced in enum MessagesIds */

  task_id_t  originTaskId;        /**< ID of the sender task */
  task_id_t  destinationTaskId;   /**< ID of the destination task */
  instance_t instance;            /**< Task instance for virtualization */

  MessageHeaderSize ittiMsgSize;         /**< Message size (not including header size) */

  itti_lte_time_t lte_time;       /**< Reference LTE time */
} MessageHeader;

/** @struct MessageDef
 *  @brief Message structure for inter-task communication.
 *  \internal
 *  The attached attribute \c __packed__ is neccessary, because the memory allocation code expects \ref ittiMsg directly following \ref ittiMsgHeader.
 */
typedef struct __attribute__ ((__packed__)) MessageDef_s {
  MessageHeader ittiMsgHeader; /**< Message header */
  msg_t         ittiMsg; /**< Union of payloads as defined in x_messages_def.h headers */
} MessageDef;

#endif /* INTERTASK_INTERFACE_TYPES_H_ */
/* @} */
