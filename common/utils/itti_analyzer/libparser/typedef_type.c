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

#include "typedef_type.h"
#include "ui_interface.h"

int typedef_dissect_from_buffer(
    types_t *type, ui_set_signal_text_cb_t ui_set_signal_text_cb, gpointer user_data,
    buffer_t *buffer, uint32_t offset, uint32_t parent_offset, int indent, gboolean new_line)
{
    int length = 0;
    char cbuf[50 + (type->name ? strlen (type->name) : 0)];
    types_t *type_child = NULL;

    DISPLAY_PARSE_INFO("typedef", type->name, offset, parent_offset);

    if (type->child != NULL) {
        /* Ignore TYPEDEF children */
        for (type_child = type->child; type_child != NULL && type_child->type == TYPE_TYPEDEF; type_child =
                type_child->child) {
        }
    }

    if ((type_child == NULL) || (type_child->type != TYPE_FUNDAMENTAL))
    {
        INDENTED_STRING(cbuf, new_line ? indent : 0, length = sprintf (cbuf, "%s, ", type->name));
        ui_set_signal_text_cb(user_data, cbuf, length);

        new_line = FALSE;
    }

    /* Simply call next_type */
    if (type->child != NULL) {
        type->child->parent = type;
        type->child->type_dissect_from_buffer(
            type->child, ui_set_signal_text_cb, user_data, buffer, offset, parent_offset, indent, new_line);
    }

    return 0;
}

int typedef_type_file_print(types_t *type, int indent, FILE *file)
{
    if (type == NULL)
        return -1;
    INDENTED(file, indent,   fprintf(file, "<Typedef>\n"));
    INDENTED(file, indent+4, fprintf(file, "Name .......: %s\n", type->name));
    INDENTED(file, indent+4, fprintf(file, "Type .......: %d\n", type->type_xml));
    INDENTED(file, indent+4, fprintf(file, "Size .......: %d\n", type->size));
    INDENTED(file, indent+4, fprintf(file, "Align ......: %d\n", type->align));
    INDENTED(file, indent+4, fprintf(file, "Artificial .: %d\n", type->artificial));
    INDENTED(file, indent+4, fprintf(file, "File .......: %s\n", type->file));
    INDENTED(file, indent+4, fprintf(file, "Line .......: %d\n", type->line));
    INDENTED(file, indent+4, fprintf(file, "Members ....: %s\n", type->members));
    INDENTED(file, indent+4, fprintf(file, "Mangled ....: %s\n", type->mangled));
    INDENTED(file, indent+4, fprintf(file, "Demangled ..: %s\n", type->demangled));
    if (type->file_ref != NULL)
        type->file_ref->type_file_print(type->file_ref, indent+4, file);
    if (type->child != NULL)
        type->child->type_file_print(type->child, indent+4, file);
    INDENTED(file, indent,  fprintf(file, "</Typedef>\n"));

    return 0;
}

int typedef_type_hr_display(types_t *type, int indent)
{
    if (type == NULL)
        return -1;
    INDENTED(stdout, indent,  printf("<Typedef>\n"));
    INDENTED(stdout, indent+4, printf("Name .......: %s\n", type->name));
    INDENTED(stdout, indent+4, printf("Type .......: %d\n", type->type_xml));
    INDENTED(stdout, indent+4, printf("Size .......: %d\n", type->size));
    INDENTED(stdout, indent+4, printf("Align ......: %d\n", type->align));
    INDENTED(stdout, indent+4, printf("Artificial .: %d\n", type->artificial));
    INDENTED(stdout, indent+4, printf("File .......: %s\n", type->file));
    INDENTED(stdout, indent+4, printf("Line .......: %d\n", type->line));
    INDENTED(stdout, indent+4, printf("Members ....: %s\n", type->members));
    INDENTED(stdout, indent+4, printf("Mangled ....: %s\n", type->mangled));
    INDENTED(stdout, indent+4, printf("Demangled ..: %s\n", type->demangled));
    if (type->file_ref != NULL)
        type->file_ref->type_hr_display(type->file_ref, indent+4);
    if (type->child != NULL)
        type->child->type_hr_display(type->child, indent+4);
    INDENTED(stdout, indent, printf("</Typedef>\n"));

    return 0;
}
