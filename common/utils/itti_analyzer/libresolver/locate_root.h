/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */


#ifndef LOCATE_ROOT_H_
#define LOCATE_ROOT_H_

#include "types.h"

extern types_t *messages_id_enum;
extern types_t *lte_time_type;
extern types_t *lte_time_frame_type;
extern types_t *lte_time_slot_type;
extern types_t *origin_task_id_type;
extern types_t *destination_task_id_type;
extern types_t *instance_type;
extern types_t *message_header_type;
extern types_t *message_type;
extern types_t *message_size_type;

int locate_root(const char *root_name, types_t *head, types_t **root);

int locate_type(const char *type_name, types_t *head, types_t **type);

int locate_type_children(const char *type_name, types_t *head, types_t **type);

uint32_t get_message_header_type_size(void);

uint32_t get_message_size(buffer_t *buffer);

uint32_t get_lte_frame(buffer_t *buffer);

uint32_t get_lte_slot(buffer_t *buffer);

uint32_t get_message_id(types_t *head, buffer_t *buffer, uint32_t *message_id);

char *message_id_to_string(uint32_t message_id);

uint32_t get_task_id(buffer_t *buffer, types_t *task_id_type);

char *task_id_to_string(uint32_t task_id_value, types_t *task_id_type);

uint32_t get_instance(buffer_t *buffer);

#endif /* LOCATE_ROOT_H_ */
