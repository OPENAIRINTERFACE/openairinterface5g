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
