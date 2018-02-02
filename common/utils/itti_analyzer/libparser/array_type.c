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
#include <ctype.h>

#include <tgmath.h>

#define G_LOG_DOMAIN ("PARSER")

#include "array_type.h"
#include "fundamental_type.h"
#include "ui_interface.h"

int array_dissect_from_buffer(
    types_t *type, ui_set_signal_text_cb_t ui_set_signal_text_cb, gpointer user_data,
    buffer_t *buffer, uint32_t offset, uint32_t parent_offset, int indent, gboolean new_line)
{
    types_t *type_child;
    int  length = 0;
    char cbuf[50];

    DISPLAY_PARSE_INFO("array", type->name, offset, parent_offset);

    /* Ignore TYPEDEF children */
    for (type_child = type->child; type_child != NULL && type_child->type == TYPE_TYPEDEF;
            type_child = type_child->child) {
    }

    if (type->name) {
        INDENTED(stdout, indent, fprintf(stdout, "<%s>\n", type->name));
    }
    if (type->child != NULL) {
        int items = type->size / type_child->size;
        int i;
        int zero_counter = 0;
        gboolean is_string = FALSE;
        char *string;
        int nb_digits = 0;

        /* Factorizes trailing 0 */
        if ((items > 1) && (type_child->type == TYPE_FUNDAMENTAL))
        {
            for (i = items - 1; i >= 0; i--)
            {
                if (fundamental_read_from_buffer(type_child, buffer, parent_offset, offset + i * type_child->size) == 0)
                {
                    zero_counter ++;
                }
                else
                {
                    break;
                }
            }

            /* Check if this is an array of 8 bits items and if at least the firsts ones are not null */
            if ((type_child->size == 8) && (zero_counter >= 1) && ((items - zero_counter) >= 2))
            {
                int end = items - zero_counter;

                /* check if this is a printable string */
                is_string = TRUE;
                string = malloc(end + 1);

                for (i = 0; i < end; i++)
                {
                    string[i] = fundamental_read_from_buffer(type_child, buffer, parent_offset, offset + i * type_child->size);
                    if (isprint(string[i]) == 0)
                    {
                        /* This is not a printable string */
                        is_string = FALSE;
                        break;
                    }
                }

                if (is_string)
                {
                    string[i] = '\0';
                    INDENTED_STRING(cbuf, indent, length = sprintf(cbuf, "[0 .. %d] \"%s\"\n", end - 1, string));
                    ui_set_signal_text_cb(user_data, cbuf, length);
                }
            }

            /* Do not factorize if there is only one null item */
            if (zero_counter <= 1)
            {
                zero_counter = 0;
            }
        }

        if (is_string == FALSE)
        {
            nb_digits = log10(items - zero_counter) + 1;

            for (i = 0; i < (items - zero_counter); i++)
            {
                INDENTED_STRING(cbuf, indent, length = sprintf(cbuf, "[%*d] ", nb_digits, i));
                ui_set_signal_text_cb(user_data, cbuf, length);

                type->child->type_dissect_from_buffer (
                    type->child, ui_set_signal_text_cb, user_data, buffer, parent_offset,
                    offset + i * type_child->size, indent,
                    FALSE);
            }
            if (zero_counter > 0)
            {
                INDENTED_STRING(cbuf, indent, length = sprintf(cbuf, "[%d .. %d] ", i, items -1));
                ui_set_signal_text_cb(user_data, cbuf, length);

                type->child->type_dissect_from_buffer (
                    type->child, ui_set_signal_text_cb, user_data,
                    buffer, parent_offset, offset + i * type_child->size, indent, FALSE);
            }
        }
    }
    if (type->name) {
        INDENTED(stdout, indent, fprintf(stdout, "</%s>\n", type->name));
    }

    return 0;
}

int array_type_file_print(types_t *type, int indent, FILE *file) {
    if (type == NULL)
        return -1;
    INDENTED(file, indent, fprintf(file, "<Array>\n"));
    INDENTED(file, indent+4, fprintf(file, "Id .........: %d\n", type->id));
    INDENTED(file, indent+4, fprintf(file, "Min ........: %d\n", type->min));
    INDENTED(file, indent+4, fprintf(file, "Max ........: %d\n", type->max));
    INDENTED(file, indent+4, fprintf(file, "Type .......: %d\n", type->type_xml));
    INDENTED(file, indent+4, fprintf(file, "Size .......: %d\n", type->size));
    INDENTED(file, indent+4, fprintf(file, "Align ......: %d\n", type->align));
    if (type->file_ref != NULL)
        type->file_ref->type_file_print (type->file_ref, indent + 4, file);
    if (type->child != NULL)
        type->child->type_file_print (type->child, indent + 4, file);
    INDENTED(file, indent, fprintf(file, "</Array>\n"));

    return 0;
}

int array_type_hr_display(types_t *type, int indent) {
    if (type == NULL)
        return -1;
    INDENTED(stdout, indent, printf("<Array>\n"));
    INDENTED(stdout, indent+4, printf("Id .........: %d\n", type->id));
    INDENTED(stdout, indent+4, printf("Min ........: %d\n", type->min));
    INDENTED(stdout, indent+4, printf("Max ........: %d\n", type->max));
    INDENTED(stdout, indent+4, printf("Type .......: %d\n", type->type_xml));
    INDENTED(stdout, indent+4, printf("Size .......: %d\n", type->size));
    INDENTED(stdout, indent+4, printf("Align ......: %d\n", type->align));
    if (type->file_ref != NULL)
        type->file_ref->type_hr_display (type->file_ref, indent + 4);
    if (type->child != NULL)
        type->child->type_hr_display (type->child, indent + 4);
    INDENTED(stdout, indent, printf("</Array>\n"));

    return 0;
}
