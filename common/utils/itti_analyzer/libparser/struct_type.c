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

#include "struct_type.h"
#include "buffers.h"
#include "ui_callbacks.h"
#include "ui_interface.h"

int struct_dissect_from_buffer(
    types_t *type, ui_set_signal_text_cb_t ui_set_signal_text_cb, gpointer user_data,
    buffer_t *buffer, uint32_t offset, uint32_t parent_offset, int indent, gboolean new_line)
{
    int i;
    char cbuf[50 + (type->name ? strlen (type->name) : 0)];
    int length = 0;
    char *name;

    memset (cbuf, 0, sizeof(cbuf));

    DISPLAY_PARSE_INFO("structure", type->name, offset, parent_offset);

    if (new_line) {
        DISPLAY_TYPE("Str");
    }

    if (type->name) {
        name = type->name;
    }
    else {
        name = "_anonymous_";
    }

    if ((strcmp (name, "IttiMsgText_s") == 0) &&
        (type->members_child[0] != NULL) && (strcmp (type->members_child[0]->name, "size") == 0) &&
        (type->members_child[1] != NULL) && (strcmp (type->members_child[1]->name, "text") == 0)) {
        uint8_t *buf;

        length = buffer_get_uint32_t (buffer, offset + parent_offset);
        buf = malloc (length + 1);
        buf[0] = '\n';
        buffer_fetch_nbytes(buffer, parent_offset + offset + 32, length, &buf[1]);
        length = ui_callback_check_string ((char *) &buf[1], length, 0);
        ui_set_signal_text_cb(user_data, (char *) buf, length + 1);
    }
    else
    {
        INDENTED_STRING(cbuf, new_line ? indent : 0, length = sprintf (cbuf, "%s :", name));
        DISPLAY_BRACE(length += sprintf(&cbuf[length], " {"););
        length += sprintf(&cbuf[length], "\n");
        ui_set_signal_text_cb(user_data, cbuf, length);

        for (i = 0; i < type->nb_members; i++) {
            if (type->members_child[i] != NULL)
                type->members_child[i]->type_dissect_from_buffer (
                    type->members_child[i], ui_set_signal_text_cb, user_data,
                    buffer, offset, parent_offset, indent + DISPLAY_TAB_SIZE, TRUE);
        }
    }

    DISPLAY_BRACE(
            DISPLAY_TYPE("Str");

            INDENTED_STRING(cbuf, indent, length = sprintf(cbuf, "};\n"));
            ui_set_signal_text_cb(user_data, cbuf, length);)

    return 0;
}

int struct_type_file_print(types_t *type, int indent, FILE *file) {
    int i;
    if (type == NULL)
        return -1;
    if (file == NULL)
        return -1;
    INDENTED(file, indent, fprintf(file, "<Struct>\n"));
    INDENTED(file, indent+4, fprintf(file, "Name .......: %s\n", type->name));
    INDENTED(file, indent+4, fprintf(file, "Id .........: %d\n", type->id));
    INDENTED(file, indent+4, fprintf(file, "Size .......: %d\n", type->size));
    INDENTED(file, indent+4, fprintf(file, "Align ......: %d\n", type->align));
    INDENTED(file, indent+4, fprintf(file, "Artificial .: %d\n", type->artificial));
    INDENTED(file, indent+4, fprintf(file, "File .......: %s\n", type->file));
    INDENTED(file, indent+4, fprintf(file, "Line .......: %d\n", type->line));
    INDENTED(file, indent+4, fprintf(file, "Members ....: %s\n", type->members));
    INDENTED(file, indent+4, fprintf(file, "Mangled ....: %s\n", type->mangled));
    INDENTED(file, indent+4, fprintf(file, "Demangled ..: %s\n", type->demangled));
    if (type->file_ref != NULL)
        type->file_ref->type_file_print (type->file_ref, indent + 4, file);
    for (i = 0; i < type->nb_members; i++) {
        if (type->members_child[i] != NULL)
            type->members_child[i]->type_file_print (type->members_child[i], indent + 4, file);
    }
    INDENTED(file, indent, fprintf(file, "</Struct>\n"));

    return 0;
}

int struct_type_hr_display(types_t *type, int indent) {
    int i;
    if (type == NULL)
        return -1;
    INDENTED(stdout, indent, printf("<Struct>\n"));
    INDENTED(stdout, indent+4, printf("Name .......: %s\n", type->name));
    INDENTED(stdout, indent+4, printf("Id .........: %d\n", type->id));
    INDENTED(stdout, indent+4, printf("Size .......: %d\n", type->size));
    INDENTED(stdout, indent+4, printf("Align ......: %d\n", type->align));
    INDENTED(stdout, indent+4, printf("Artificial .: %d\n", type->artificial));
    INDENTED(stdout, indent+4, printf("File .......: %s\n", type->file));
    INDENTED(stdout, indent+4, printf("Line .......: %d\n", type->line));
    INDENTED(stdout, indent+4, printf("Members ....: %s\n", type->members));
    INDENTED(stdout, indent+4, printf("Mangled ....: %s\n", type->mangled));
    INDENTED(stdout, indent+4, printf("Demangled ..: %s\n", type->demangled));
    if (type->file_ref != NULL)
        type->file_ref->type_hr_display (type->file_ref, indent + 4);
    for (i = 0; i < type->nb_members; i++) {
        if (type->members_child[i] != NULL)
            type->members_child[i]->type_hr_display (type->members_child[i], indent + 4);
    }
    INDENTED(stdout, indent, printf("</Struct>\n"));

    return 0;
}
