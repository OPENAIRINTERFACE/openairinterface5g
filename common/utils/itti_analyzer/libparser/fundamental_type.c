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
#include <string.h>
#include <inttypes.h>

#define G_LOG_DOMAIN ("PARSER")

#include "fundamental_type.h"
#include "ui_interface.h"

uint64_t fundamental_read_from_buffer(types_t *type, buffer_t *buffer, uint32_t offset, uint32_t parent_offset) {
    uint64_t value;

    switch (type->size) {
        case 8:
            value = buffer_get_uint8_t (buffer, offset + parent_offset);
            break;

        case 16:
            value = buffer_get_uint16_t (buffer, offset + parent_offset);
            break;

        case 32:
            value = buffer_get_uint32_t (buffer, offset + parent_offset);
            break;

        case 64:
            value = buffer_get_uint64_t (buffer, offset + parent_offset);
            break;

        default:
            /* ??? */
            value = 0;
            break;
    }
    return value;
}

int fundamental_dissect_from_buffer(
    types_t *type, ui_set_signal_text_cb_t ui_set_signal_text_cb, gpointer user_data,
    buffer_t *buffer, uint32_t offset, uint32_t parent_offset, int indent, gboolean new_line)
{
    int         length = 0;
    char        cbuf[200];
    int         type_unsigned;
    uint8_t     value8;
    uint16_t    value16;
    uint32_t    value32;
    uint64_t    value64;

    DISPLAY_PARSE_INFO("fundamental", type->name, offset, parent_offset);

    memset (cbuf, 0, 200);

    type_unsigned = strstr (type->name, "unsigned") == NULL ? 0 : 1;

    value64 = fundamental_read_from_buffer(type, buffer, offset, parent_offset);

    indent = new_line ? indent : 0;
    if (indent > 0)
    {
        DISPLAY_TYPE("Fun");
    }
    switch (type->size) {
        case 8:
            value8 = (uint8_t) value64;
            INDENTED_STRING(cbuf, indent,
                            length = sprintf(cbuf, type_unsigned ? "(0x%02" PRIx8 ") %3" PRIu8 ";\n" : "(0x%02" PRIx8 ") %4" PRId8 ";\n", value8, value8));
            break;

        case 16:
            value16 = (uint16_t) value64;
            INDENTED_STRING(cbuf, indent,
                            length = sprintf(cbuf, type_unsigned ? "(0x%04" PRIx16 ") %5" PRIu16 ";\n" : "(0x%04" PRIx16 ") %6" PRId16 ";\n", value16, value16));
            break;

        case 32:
            value32 = (uint32_t) value64;
            INDENTED_STRING(cbuf, indent,
                            length = sprintf(cbuf, type_unsigned ? "(0x%08" PRIx32 ") %9" PRIu32 ";\n" : "(0x%08" PRIx32 ") %10" PRId32 ";\n", value32, value32));
            break;

        case 64:
            INDENTED_STRING(cbuf, indent,
                            length = sprintf(cbuf, type_unsigned ? "(0x%016" PRIx64 ") %20" PRIu64 ";\n" : "(0x%016" PRIx64 ") %20" PRId64 ";\n", value64, value64));
            break;

        default:
            /* ??? */
            break;
    }
    ui_set_signal_text_cb(user_data, cbuf, length);

    return 0;
}

int fundamental_type_file_print(types_t *type, int indent, FILE *file) {
    if (type == NULL)
        return -1;
    INDENTED(file, indent, fprintf(file, "<Fundamental>\n"));
    INDENTED(file, indent+4, fprintf(file, "Name .......: %s\n", type->name));
    INDENTED(file, indent+4, fprintf(file, "Id .........: %d\n", type->id));
    INDENTED(file, indent+4, fprintf(file, "Size .......: %d\n", type->size));
    INDENTED(file, indent+4, fprintf(file, "Align ......: %d\n", type->align));
    if (type->file_ref != NULL)
        type->file_ref->type_file_print (type->file_ref, indent + 4, file);
    INDENTED(file, indent, fprintf(file, "</Fundamental>\n"));
    return 0;
}

int fundamental_type_hr_display(types_t *type, int indent) {
    if (type == NULL)
        return -1;
    INDENTED(stdout, indent, printf("<Fundamental>\n"));
    INDENTED(stdout, indent+4, printf("Name .......: %s\n", type->name));
    INDENTED(stdout, indent+4, printf("Id .........: %d\n", type->id));
    INDENTED(stdout, indent+4, printf("Size .......: %d\n", type->size));
    INDENTED(stdout, indent+4, printf("Align ......: %d\n", type->align));
    if (type->file_ref != NULL)
        type->file_ref->type_hr_display (type->file_ref, indent + 4);
    INDENTED(stdout, indent, printf("</Fundamental>\n"));
    return 0;
}
