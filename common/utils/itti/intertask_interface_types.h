/*******************************************************************************
    OpenAirInterface
    Copyright(c) 1999 - 2014 Eurecom

    OpenAirInterface is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.


    OpenAirInterface is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with OpenAirInterface.The full GNU General Public License is
   included in this distribution in the file called "COPYING". If not,
   see <http://www.gnu.org/licenses/>.

  Contact Information
  OpenAirInterface Admin: openair_admin@eurecom.fr
  OpenAirInterface Tech : openair_tech@eurecom.fr
  OpenAirInterface Dev  : openair4g-devel@eurecom.fr

  Address      : Eurecom, Campus SophiaTech, 450 Route des Chappes, CS 50193 - 06904 Biot Sophia Antipolis cedex, FRANCE

 *******************************************************************************/

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
