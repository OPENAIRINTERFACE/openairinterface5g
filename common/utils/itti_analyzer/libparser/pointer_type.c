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

#include "pointer_type.h"
#include "ui_interface.h"

int pointer_dissect_from_buffer(
    types_t *type, ui_set_signal_text_cb_t ui_set_signal_text_cb, gpointer user_data,
    buffer_t *buffer, uint32_t offset, uint32_t parent_offset, int indent, gboolean new_line)
{
    int          length = 0, i;
    char         cbuf[200];

    DISPLAY_PARSE_INFO("pointer", type->name, offset, parent_offset);

    memset (cbuf, 0, 200);

    uint8_t value[type->size / 8];

    buffer_fetch_nbytes(buffer, parent_offset + offset, type->size / 8, value);

    indent = new_line ? indent : 0;
    if (indent > 0)
    {
        DISPLAY_TYPE("Ptr");
    }
    if (type->child && type->child->name) {
        /*
         INDENTED(stdout, indent, fprintf(stdout, "<%s>0x%08x</%s>\n",
         type->child->name, value, type->child->name));
         */
         INDENTED_STRING(cbuf, indent, length = sprintf(cbuf, "(%s *) 0x", type->child->name));
    }
    else {
        /*
         INDENTED(stdout, indent, fprintf(stdout, "<Pointer>0x%08x</Pointer>\n",
         value));
         */
        INDENTED_STRING(cbuf, indent, length = sprintf(cbuf, "(void *) 0x"));
    }

    /* Append the value */
    for (i = type->size / 8 - 1; i >= 0; i--) {
        length += sprintf(&cbuf[length], "%02x", value[i]);
    }
    length += sprintf(&cbuf[length], ";\n");
    ui_set_signal_text_cb(user_data, cbuf, length);

    return 0;
}

int pointer_type_file_print(types_t *type, int indent, FILE *file) {
    if (type == NULL)
        return -1;
    INDENTED(file, indent, fprintf(file, "<Pointer>\n"));
    INDENTED(file, indent+4, fprintf(file, "Id .........: %d\n", type->id));
    INDENTED(file, indent+4, fprintf(file, "Type .......: %d\n", type->type_xml));
    INDENTED(file, indent+4, fprintf(file, "Size .......: %d\n", type->size));
    INDENTED(file, indent+4, fprintf(file, "Align ......: %d\n", type->align));
    if (type->child != NULL) {
        if (type->child->type == TYPE_FUNCTION) {
            INDENTED(file, indent+4, fprintf(file, "<Function>\n"));
            INDENTED(file, indent+8, fprintf(file, "<Args>To be done</Args>\n"));
            INDENTED(file, indent+4, fprintf(file, "</Function>\n"));
        } else {
            type->child->type_file_print (type->child, indent + 4, file);
        }
    }
    if (type->file_ref != NULL)
        type->file_ref->type_file_print (type->file_ref, indent + 4, file);
    INDENTED(file, indent, fprintf(file, "</Pointer>\n"));

    return 0;
}

int pointer_type_hr_display(types_t *type, int indent) {
    return pointer_type_file_print(type, indent, stdout);
}
