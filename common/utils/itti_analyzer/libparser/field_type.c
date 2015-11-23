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

#include "rc.h"

#include "field_type.h"
#include "buffers.h"
#include "ui_interface.h"

int field_dissect_from_buffer(
    types_t *type, ui_set_signal_text_cb_t ui_set_signal_text_cb, gpointer user_data,
    buffer_t *buffer, uint32_t offset, uint32_t parent_offset, int indent, gboolean new_line)
{
    int length = 0;
    char cbuf[50 + (type->name ? strlen (type->name) : 0)];
    types_t *type_child;
    char array_info[50];
    new_line = FALSE;

    DISPLAY_PARSE_INFO("field", type->name, offset, parent_offset);

    CHECK_FCT(buffer_has_enouch_data(buffer, parent_offset + offset, type->size / 8));

    if (type->bits == -1) {
        if ((type->name != NULL) && (strcmp(type->name, "_asn_ctx") == 0)) {
           /* Hide ASN1 asn_struct_ctx_t struct that hold context for parsing across buffer boundaries */
           /*
           INDENTED_STRING(cbuf, indent, sprintf(cbuf, ".asn_ctx ...\n"));
           length = strlen (cbuf);

           ui_set_signal_text_cb(user_data, cbuf, length);
           */
        }
        else {
            if (type->child != NULL) {
                /* Ignore TYPEDEF children */
                for (type_child = type->child; type_child != NULL && type_child->type == TYPE_TYPEDEF; type_child =
                        type_child->child) {
                }

                if (type_child->type == TYPE_ARRAY) {
                    types_t *type_array_child;

                    /* Ignore TYPEDEF children */
                    for (type_array_child = type_child->child;
                            type_array_child != NULL && type_array_child->type == TYPE_TYPEDEF; type_array_child =
                                    type_array_child->child) {
                    }

                    sprintf (array_info, "[%d]", type_child->size / type_array_child->size);
                }
                else {
                    array_info[0] = '\0';
                }

                DISPLAY_TYPE("Fld");

                INDENTED_STRING(cbuf, indent, length = sprintf(cbuf, ".%s%s = ", type->name ? type->name : "Field", array_info));
                ui_set_signal_text_cb(user_data, cbuf, length);

                if (type_child->type == TYPE_ARRAY) {
                    DISPLAY_BRACE(ui_set_signal_text_cb(user_data, "{", 1);)
                    ui_set_signal_text_cb(user_data, "\n", 1);
                    new_line = TRUE;
                }
                /*
                if (type_child->type == TYPE_STRUCT || type_child->type == TYPE_UNION) {
                    DISPLAY_BRACE(ui_set_signal_text_cb(user_data, "{", 1);)
                    ui_set_signal_text_cb(user_data, "\n", 1);
                    new_line = TRUE;
                }
                */

                type->child->parent = type;
                type->child->type_dissect_from_buffer(
                    type->child, ui_set_signal_text_cb, user_data, buffer,
                    parent_offset, offset + type->offset, new_line ? indent + DISPLAY_TAB_SIZE : indent, new_line);

                DISPLAY_BRACE(
                        if (type_child->type == TYPE_ARRAY) {
                            DISPLAY_TYPE("Fld");

                            INDENTED_STRING(cbuf, indent, length = sprintf(cbuf, "};\n"));
                            ui_set_signal_text_cb(user_data, cbuf, length);
                        });
                /*
                DISPLAY_BRACE(
                        if (type_child->type == TYPE_STRUCT || type_child->type == TYPE_UNION) {
                            DISPLAY_TYPE("Fld");
                            INDENTED_STRING(cbuf, indent, sprintf(cbuf, "};\n"));
                            length = strlen (cbuf);

                            ui_set_signal_text_cb(user_data, cbuf, length);
                        });
                        */
            }
        }
    }
    else {
        /* The field is only composed of bits */
        uint32_t value = 0;

        CHECK_FCT(buffer_fetch_bits(buffer, offset + type->offset + parent_offset, type->bits, &value));

        DISPLAY_TYPE("Fld");

        INDENTED_STRING(cbuf, indent,
                length = sprintf(cbuf, ".%s:%d = (0x%0*x) %d;\n", type->name ? type->name : "Field", type->bits, (type->bits + 3) / 4, value, value));
        ui_set_signal_text_cb(user_data, cbuf, length);
    }

    return 0;
}

int field_type_file_print(types_t *type, int indent, FILE *file) {
    if (type == NULL)
        return -1;
    INDENTED(file, indent, fprintf(file, "<Field>\n"));
    INDENTED(file, indent+4, fprintf(file, "Id .........: %d\n", type->id));
    INDENTED(file, indent+4, fprintf(file, "Name .......: %s\n", type->name));
    INDENTED(file, indent+4, fprintf(file, "Bits .......: %d\n", type->bits));
    INDENTED(file, indent+4, fprintf(file, "Type .......: %d\n", type->type_xml));
    INDENTED(file, indent+4, fprintf(file, "Offset .....: %d\n", type->offset));
    INDENTED(file, indent+4, fprintf(file, "Context ....: %d\n", type->context));
    INDENTED(file, indent+4, fprintf(file, "File .......: %s\n", type->file));
    INDENTED(file, indent+4, fprintf(file, "Line .......: %d\n", type->line));
    if (type->file_ref != NULL)
        type->file_ref->type_file_print (type->file_ref, indent + 4, file);
    if (type->child != NULL)
        type->child->type_file_print (type->child, indent + 4, file);
    INDENTED(file, indent, fprintf(file, "</Field>\n"));

    return 0;
}

int field_type_hr_display(types_t *type, int indent) {
    if (type == NULL)
        return -1;
    INDENTED(stdout, indent, printf("<Field>\n"));
    INDENTED(stdout, indent+4, printf("Id .........: %d\n", type->id));
    INDENTED(stdout, indent+4, printf("Name .......: %s\n", type->name));
    INDENTED(stdout, indent+4, printf("Bits .......: %d\n", type->bits));
    INDENTED(stdout, indent+4, printf("Type .......: %d\n", type->type_xml));
    INDENTED(stdout, indent+4, printf("Offset .....: %d\n", type->offset));
    INDENTED(stdout, indent+4, printf("Context ....: %d\n", type->context));
    INDENTED(stdout, indent+4, printf("File .......: %s\n", type->file));
    INDENTED(stdout, indent+4, printf("Line .......: %d\n", type->line));
    if (type->file_ref != NULL)
        type->file_ref->type_hr_display (type->file_ref, indent + 4);
    if (type->child != NULL)
        type->child->type_hr_display (type->child, indent + 4);
    INDENTED(stdout, indent, printf("</Field>\n"));

    return 0;
}
