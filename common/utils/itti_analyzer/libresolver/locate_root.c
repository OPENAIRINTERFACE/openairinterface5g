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

#include <stdio.h>
#include <string.h>

#define G_LOG_DOMAIN ("RESOLVER")

#include <glib.h>

#include "logs.h"
#include "rc.h"

#include "types.h"
#include "enum_type.h"
#include "locate_root.h"
#include "xml_parse.h"

types_t *messages_id_enum;
types_t *lte_time_type;
types_t *lte_time_frame_type;
types_t *lte_time_slot_type;
types_t *origin_task_id_type;
types_t *destination_task_id_type;
types_t *instance_type;
types_t *message_header_type;
types_t *message_type;
types_t *message_size_type;

int locate_root(const char *root_name, types_t *head, types_t **root_elm) {
    types_t *next_type;
    int next_counter = 0;

    /* The root element is for example : MessageDef.
     * This element is the entry for other sub-types.
     */
    if (!root_name || (strlen (root_name) == 0)) {
        g_warning("FATAL: no root element name provided");
        return -1;
    }
    if (!head) {
        g_warning("Empty list detected");
        return -1;
    }
    if (!root_elm) {
        g_warning("NULL root reference");
        return -1;
    }

    for (next_type = head; next_type; next_type = next_type->next) {
        if (next_type->name == NULL)
            continue;
        if (strcmp (root_name, next_type->name) == 0) {
            /* Matching reference */
            break;
        }
        next_counter ++;
    }
    g_info("locate_root: %s %d", root_name, next_counter);

    *root_elm = next_type;
    return (next_type == NULL) ? -2 : 0;
}

int locate_type(const char *type_name, types_t *head, types_t **type) {
    types_t *next_type;
    int next_counter = 0;

    /* The root element is for example : MessageDef.
     * This element is the entry for other sub-types.
     */
    if (!type_name) {
        g_warning("FATAL: no element name provided");
        return RC_BAD_PARAM;
    }
    if (!head) {
        g_warning("Empty list detected");
        return RC_BAD_PARAM;
    }

    for (next_type = head; next_type; next_type = next_type->next) {
        if (next_type->name == NULL)
            continue;
        if (strcmp (type_name, next_type->name) == 0) {
            /* Matching reference */
            break;
        }
        next_counter ++;
    }
    g_info("locate_type: %s %d %p", type_name, next_counter, next_type);

    if (type)
        *type = next_type;
    return (next_type == NULL) ? RC_FAIL : RC_OK;
}

int locate_type_children(const char *type_name, types_t *head, types_t **type) {
    types_t *found_type = NULL;
    int i;

    /* The root element is for example : MessageDef.
     * This element is the entry for other sub-types.
     */
    if (!type_name) {
        g_warning("FATAL: no element name provided");
        return RC_BAD_PARAM;
    }
    if (!head) {
        g_warning("Empty list detected");
        return RC_BAD_PARAM;
    }

    for (i = 0; i < head->nb_members; i++) {
        if (head->members_child[i]->name == NULL)
            continue;
        if (strcmp (type_name, head->members_child[i]->name) == 0) {
            /* Matching reference */
            found_type = head->members_child[i];
            break;
        }
    }
    g_info("locate_type: %s %d %p", type_name, i, found_type);

    if (type)
        *type = found_type;
    return (found_type == NULL) ? RC_FAIL : RC_OK;
}

uint32_t get_message_header_type_size(void)
{
    /* Typedef */
    if (message_header_type->child != NULL) {
        /* Struct */
        if (message_header_type->child->child != NULL) {
            return message_header_type->child->child->size;
        }
    }
    return 0;
}

uint32_t get_message_size(buffer_t *buffer)
{
    uint32_t value = 0;

    if (message_size_type != NULL)
    {
        types_t *temp = message_size_type;

        while (temp->size == -1) {
            temp = temp->child;
        }

        /* Fetch instance value */
        buffer_fetch_bits (buffer, message_size_type->offset, temp->size, &value);
    }

    return value;
}

uint32_t get_lte_frame(buffer_t *buffer) {
    uint32_t  value = 0;

    if (lte_time_type !=NULL)
    {
        /* Fetch instance value */
        buffer_fetch_bits (buffer, lte_time_type->offset + lte_time_frame_type->offset, lte_time_frame_type->child->child->size, &value);
    }

    return value;
}

uint32_t get_lte_slot(buffer_t *buffer) {
    uint32_t  value = 0;

    if (lte_time_type !=NULL)
    {
        /* Fetch instance value */
        buffer_fetch_bits (buffer, lte_time_type->offset + lte_time_slot_type->offset, lte_time_slot_type->child->child->size, &value);
    }

    return value;
}

uint32_t get_message_id(types_t *head, buffer_t *buffer, uint32_t *message_id) {
    uint32_t value;

    g_assert(message_id != NULL);
    g_assert(buffer != NULL);

    /* MessageId is an offset from start of buffer */
    value = buffer_get_uint32_t(buffer, messages_id_enum->offset);

    *message_id = value;
    return RC_OK;
}

uint32_t get_task_id(buffer_t *buffer, types_t *task_id_type) {
    uint32_t task_id_value;

    /* Fetch task id value */
    if (buffer_fetch_bits (buffer, task_id_type->offset, task_id_type->child->size, &task_id_value) != RC_OK)
    {
        return ~0;
    }

    return task_id_value;
}

char *task_id_to_string(uint32_t task_id_value, types_t *task_id_type) {
    char *task_id = "UNKNOWN";

    if (task_id_value < ((uint32_t) ~0))
    {
        /* Search task id name */
        task_id = enum_type_get_name_from_value (task_id_type->child, task_id_value);
    }

    return task_id;
}

uint32_t get_instance(buffer_t *buffer) {
    uint32_t  instance_value;

    /* Fetch instance value */
    buffer_fetch_bits (buffer, instance_type->offset, instance_type->child->child->child->size, &instance_value);

    return instance_value;
}

char *message_id_to_string(uint32_t message_id)
{
    return enum_type_get_name_from_value(messages_id_enum, message_id);
}
