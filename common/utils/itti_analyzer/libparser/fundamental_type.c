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
