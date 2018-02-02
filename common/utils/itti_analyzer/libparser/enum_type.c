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

#include "enum_type.h"
#include "ui_interface.h"

char *enum_type_get_name_from_value(types_t *type, uint32_t value)
{
    char    *enum_name = "UNKNOWN";
    types_t *enum_value;

    /* Loop on each enumeration values */
    for (enum_value = type->child; enum_value; enum_value = enum_value->next) {
        if (value == enum_value->init_value) {
            enum_name = enum_value->name;
            break;
        }
    }
    return enum_name;
}

int enum_type_dissect_from_buffer(
    types_t *type, ui_set_signal_text_cb_t ui_set_signal_text_cb, gpointer user_data,
    buffer_t *buffer, uint32_t offset, uint32_t parent_offset, int indent, gboolean new_line)
{
    uint32_t value = 0;
    types_t *values;

    DISPLAY_PARSE_INFO("enum", type->name, offset, parent_offset);

    value = buffer_get_uint32_t(buffer, parent_offset + offset);

    for (values = type->child; values; values = values->next) {
        if (value == values->init_value) {
            values->parent = type;
            values->type_dissect_from_buffer(
                values, ui_set_signal_text_cb, user_data, buffer, offset, parent_offset,
                type->name == NULL ? indent: indent + DISPLAY_TAB_SIZE, FALSE);
            break;
        }
    }
    if (values == NULL) {
        int length = 0;
        char cbuf[50];

        length = sprintf(cbuf, "(0x%08x) UNKNOWN;\n", value);
        ui_set_signal_text_cb(user_data, cbuf, length);
    }

    return 0;
}

int enum_type_file_print(types_t *type, int indent, FILE *file)
{
    types_t *values;

    if (type == NULL)
        return -1;
    INDENTED(file, indent,   fprintf(file, "<Enumeration>\n"));
    INDENTED(file, indent+4, fprintf(file, "Name .......: %s\n", type->name));
    INDENTED(file, indent+4, fprintf(file, "Size .......: %d\n", type->size));
    INDENTED(file, indent+4, fprintf(file, "Align ......: %d\n", type->align));
    INDENTED(file, indent+4, fprintf(file, "Artificial .: %d\n", type->artificial));
    INDENTED(file, indent+4, fprintf(file, "File .......: %s\n", type->file));
    INDENTED(file, indent+4, fprintf(file, "Line .......: %d\n", type->line));
    if (type->file_ref != NULL)
        type->file_ref->type_file_print(type->file_ref, indent+4, file);
    /* Call enum values display */
    for (values = type->child; values; values = values->next) {
        values->type_file_print(values, indent + 4, file);
    }
    INDENTED(file, indent, fprintf(file, "</Enumeration>\n"));

    return 0;
}

int enum_type_hr_display(types_t *type, int indent)
{
    types_t *values;

    if (type == NULL)
        return -1;
    INDENTED(stdout, indent,   printf("<Enumeration>\n"));
    INDENTED(stdout, indent+4, printf("Name .......: %s\n", type->name));
    INDENTED(stdout, indent+4, printf("Size .......: %d\n", type->size));
    INDENTED(stdout, indent+4, printf("Align ......: %d\n", type->align));
    INDENTED(stdout, indent+4, printf("Artificial .: %d\n", type->artificial));
    INDENTED(stdout, indent+4, printf("File .......: %s\n", type->file));
    INDENTED(stdout, indent+4, printf("Line .......: %d\n", type->line));
    if (type->file_ref != NULL)
        type->file_ref->type_hr_display(type->file_ref, indent+4);
    /* Call enum values display */
    for (values = type->child; values; values = values->next) {
        values->type_hr_display(values, indent + 4);
    }
    INDENTED(stdout, indent, printf("</Enumeration>\n"));

    return 0;
}
