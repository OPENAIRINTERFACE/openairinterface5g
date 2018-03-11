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

#include <sys/epoll.h>

#ifndef INTERTASK_INTERFACE_H_
#define INTERTASK_INTERFACE_H_

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "intertask_interface_conf.h"
#include "intertask_interface_types.h"

#define ITTI_MSG_ID(mSGpTR)                 ((mSGpTR)->ittiMsgHeader.messageId)
#define ITTI_MSG_ORIGIN_ID(mSGpTR)          ((mSGpTR)->ittiMsgHeader.originTaskId)
#define ITTI_MSG_DESTINATION_ID(mSGpTR)     ((mSGpTR)->ittiMsgHeader.destinationTaskId)
#define ITTI_MSG_INSTANCE(mSGpTR)           ((mSGpTR)->ittiMsgHeader.instance)
#define ITTI_MSG_NAME(mSGpTR)               itti_get_message_name(ITTI_MSG_ID(mSGpTR))
#define ITTI_MSG_ORIGIN_NAME(mSGpTR)        itti_get_task_name(ITTI_MSG_ORIGIN_ID(mSGpTR))
#define ITTI_MSG_DESTINATION_NAME(mSGpTR)   itti_get_task_name(ITTI_MSG_DESTINATION_ID(mSGpTR))

/* Make the message number platform specific */
typedef unsigned long message_number_t;
#define MESSAGE_NUMBER_SIZE (sizeof(unsigned long))

typedef enum message_priorities_e {
  MESSAGE_PRIORITY_MAX       = 100,
  MESSAGE_PRIORITY_MAX_LEAST = 85,
  MESSAGE_PRIORITY_MED_PLUS  = 70,
  MESSAGE_PRIORITY_MED       = 55,
  MESSAGE_PRIORITY_MED_LEAST = 40,
  MESSAGE_PRIORITY_MIN_PLUS  = 25,
  MESSAGE_PRIORITY_MIN       = 10,
} message_priorities_t;

typedef struct message_info_s {
  task_id_t id;
  message_priorities_t priority;
  /* Message payload size */
  MessageHeaderSize size;
  /* Printable name */
  const char * const name;
} message_info_t;

typedef enum task_priorities_e {
  TASK_PRIORITY_MAX       = 100,
  TASK_PRIORITY_MAX_LEAST = 85,
  TASK_PRIORITY_MED_PLUS  = 70,
  TASK_PRIORITY_MED       = 55,
  TASK_PRIORITY_MED_LEAST = 40,
  TASK_PRIORITY_MIN_PLUS  = 25,
  TASK_PRIORITY_MIN       = 10,
} task_priorities_t;

typedef struct task_info_s {
  thread_id_t thread;
  task_id_t   parent_task;
  task_priorities_t priority;
  unsigned int queue_size;
  /* Printable name */
  const char * const name;
} task_info_t;

/** \brief Update the itti LTE time reference for messages
 \param current reference frame
 \param current reference slot
 @returns < 0 on failure, 0 otherwise
 **/
void itti_update_lte_time(uint32_t frame, uint8_t slot);

/** \brief Send a broadcast message to every task
 \param message_p Pointer to the message to send
 @returns < 0 on failure, 0 otherwise
 **/
int itti_send_broadcast_message(MessageDef *message_p);

/** \brief Send a message to a task (could be itself)
 \param task_id Task ID
 \param instance Instance of the task used for virtualization
 \param message Pointer to the message to send
 @returns -1 on failure, 0 otherwise
 **/
int itti_send_msg_to_task(task_id_t task_id, instance_t instance, MessageDef *message);

/* TODO: this is a hack. Almost no caller of itti_send_msg_to_task checks
 * the return value so it has been changed to crash the program in case
 * of failure instead of returning -1 as the documentation above says.
 * The RLC UM code may receive too much data when doing UDP at a higher
 * throughput than the link allows and so for this specific case we need
 * a version that actually returns -1 on failure.
 *
 * This needs to be cleaned at some point.
 */
/* look for HACK_RLC_UM_LIMIT for others places related to the hack. Please do not remove this comment. */
int itti_try_send_msg_to_task(task_id_t task_id, instance_t instance, MessageDef *message);

/** \brief Add a new fd to monitor.
 * NOTE: it is up to the user to read data associated with the fd
 *  \param task_id Task ID of the receiving task
 *  \param fd The file descriptor to monitor
 **/
void itti_subscribe_event_fd(task_id_t task_id, int fd);

/** \brief Remove a fd from the list of fd to monitor
 *  \param task_id Task ID of the task
 *  \param fd The file descriptor to remove
 **/
void itti_unsubscribe_event_fd(task_id_t task_id, int fd);

/** \brief Return the list of events excluding the fd associated with itti
 *  \param task_id Task ID of the task
 *  \param events events list
 *  @returns number of events to handle
 **/
int itti_get_events(task_id_t task_id, struct epoll_event **events);

/** \brief Retrieves a message in the queue associated to task_id.
 * If the queue is empty, the thread is blocked till a new message arrives.
 \param task_id Task ID of the receiving task
 \param received_msg Pointer to the allocated message
 **/
void itti_receive_msg(task_id_t task_id, MessageDef **received_msg);

/** \brief Try to retrieves a message in the queue associated to task_id.
 \param task_id Task ID of the receiving task
 \param received_msg Pointer to the allocated message
 **/
void itti_poll_msg(task_id_t task_id, MessageDef **received_msg);

/** \brief Start thread associated to the task
 * \param task_id task to start
 * \param start_routine entry point for the task
 * \param args_p Optional argument to pass to the start routine
 * @returns -1 on failure, 0 otherwise
 **/
int itti_create_task(task_id_t task_id,
                     void *(*start_routine) (void *),
                     void *args_p);

/** \brief Mark the task as a real time task
 * \param task_id task to mark as real time
 **/
void itti_set_task_real_time(task_id_t task_id);

/** \brief Indicates to ITTI if newly created tasks should wait for all tasks to be ready
 * \param wait_tasks non 0 to make new created tasks to wait, 0 to let created tasks to run
 **/
void itti_wait_ready(int wait_tasks);

/** \brief Mark the task as in ready state
 * \param task_id task to mark as ready
 **/
void itti_mark_task_ready(task_id_t task_id);

/** \brief Exit the current task.
 **/
void itti_exit_task(void);

/** \brief Indicate that the task is completed and initiate termination of all tasks.
 * \param task_id task that is completed
 **/
void itti_terminate_tasks(task_id_t task_id);

/** \brief Return the printable string associated with the message
 * \param message_id Id of the message
 **/
const char *itti_get_message_name(MessagesIds message_id);

/** \brief Return the printable string associated with a task id
 * \param thread_id Id of the task
 **/
const char *itti_get_task_name(task_id_t task_id);

/** \brief Alloc and memset(0) a new itti message.
 * \param origin_task_id Task ID of the sending task
 * \param message_id Message ID
 * @returns NULL in case of failure or newly allocated mesage ref
 **/
MessageDef *itti_alloc_new_message(
  task_id_t         origin_task_id,
  MessagesIds       message_id);

/** \brief Alloc and memset(0) a new itti message.
 * \param origin_task_id Task ID of the sending task
 * \param message_id Message ID
 * \param size size of the payload to send
 * @returns NULL in case of failure or newly allocated mesage ref
 **/
MessageDef *itti_alloc_new_message_sized(
  task_id_t         origin_task_id,
  MessagesIds       message_id,
  MessageHeaderSize size);

/** \brief handle signals and wait for all threads to join when the process complete.
 * This function should be called from the main thread after having created all ITTI tasks.
 **/
void itti_wait_tasks_end(void);

/** \brief Send a termination message to all tasks.
 * \param task_id task that is broadcasting the message.
 **/
void itti_send_terminate_message(task_id_t task_id);

void *itti_malloc(task_id_t origin_task_id, task_id_t destination_task_id, ssize_t size);

int itti_free(task_id_t task_id, void *ptr);

#endif /* INTERTASK_INTERFACE_H_ */
/* @} */
