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

#ifndef INTERTASK_INTERFACE_DUMP_H_
#define INTERTASK_INTERFACE_DUMP_H_

int itti_dump_queue_message(task_id_t sender_task, message_number_t message_number, MessageDef *message_p, const char *message_name,
                            const uint32_t message_size);

int itti_dump_init(const char * const messages_definition_xml, const char * const dump_file_name);

void itti_dump_exit(void);

void itti_dump_thread_use_ring_buffer(void);

#endif /* INTERTASK_INTERFACE_DUMP_H_ */
