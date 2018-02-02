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
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define G_LOG_DOMAIN ("PARSER")

#include "enum_value_type.h"
#include "ui_interface.h"

uint32_t last_enum_value;

int enum_value_dissect_from_buffer(
    types_t *type, ui_set_signal_text_cb_t ui_set_signal_text_cb, gpointer user_data,
    buffer_t *buffer, uint32_t offset, uint32_t parent_offset, int indent, gboolean new_line)
{
    types_t *type_parent = NULL;
    uint32_t value = 0;

    DISPLAY_PARSE_INFO("enum_value", type->name, offset, parent_offset);

    value = buffer_get_uint32_t(buffer, parent_offset + offset);
    if (type->name) {
        int length = 0;
        char cbuf[50 + strlen(type->name)];

        /* Search for enum variable name and check if it is used in association with a union type */
        {
            if (type->parent != NULL) {
                /* Ignore ENUMERATION and TYPEDEF parents */
                for (type_parent = type->parent; (type_parent != NULL) && ((type_parent->type == TYPE_ENUMERATION) || (type_parent->type == TYPE_TYPEDEF));
                        type_parent = type_parent->parent) {
                }
            }
            if ((type_parent != NULL) && (type_parent->name != NULL) && (strcmp(type_parent->name, "present") == 0))
            {
                /* ASN1 union, keep the "present" member */
                last_enum_value = value;
            }
            else
            {
                last_enum_value = 0;
            }
        }
        length = sprintf(cbuf, "(0x%08x) %s;\n", value, type->name);
        ui_set_signal_text_cb(user_data, cbuf, length);
    }

    return 0;
}

int enum_value_file_print(types_t *type, int indent, FILE *file)
{
    if (type == NULL)
        return -1;
    INDENTED(file, indent,   fprintf(file, "<Value>\n"));
    INDENTED(file, indent+4, fprintf(file, "Name .......: %s\n", type->name));
    INDENTED(file, indent+4, fprintf(file, "Value ......: %d\n", type->init_value));
    if (type->file_ref != NULL)
        type->file_ref->type_file_print(type->file_ref, indent+4, file);
    INDENTED(file, indent, fprintf(file, "</Value>\n"));

    return 0;
}

int enum_value_type_hr_display(types_t *type, int indent)
{
    if (type == NULL)
        return -1;
    INDENTED(stdout, indent,  printf("<Value>\n"));
    INDENTED(stdout, indent+4, printf("Name .......: %s\n", type->name));
    INDENTED(stdout, indent+4, printf("Value ......: %d\n", type->init_value));
    if (type->file_ref != NULL)
        type->file_ref->type_hr_display(type->file_ref, indent+4);
    INDENTED(stdout, indent, printf("</Value>\n"));

    return 0;
}
