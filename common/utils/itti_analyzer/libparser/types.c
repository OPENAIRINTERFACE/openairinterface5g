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

#define G_LOG_DOMAIN ("PARSER")

#include "types.h"
#include "array_type.h"
#include "enum_type.h"
#include "fundamental_type.h"
#include "struct_type.h"
#include "union_type.h"
#include "typedef_type.h"
#include "enum_value_type.h"
#include "file_type.h"
#include "field_type.h"
#include "reference_type.h"
#include "pointer_type.h"

types_t *type_new(enum type_e type)
{
    types_t *new_p;

    new_p = calloc(1, sizeof(types_t));

    new_p->type = type;

    switch(type) {
        case TYPE_FUNCTION:
            /* Nothing to do for now. Display is done by pointer type */
            break;
        case TYPE_ENUMERATION:
            new_p->type_hr_display = enum_type_hr_display;
            new_p->type_file_print = enum_type_file_print;
            new_p->type_dissect_from_buffer = enum_type_dissect_from_buffer;
            break;
        case TYPE_FUNDAMENTAL:
            new_p->type_hr_display = fundamental_type_hr_display;
            new_p->type_file_print = fundamental_type_file_print;
            new_p->type_dissect_from_buffer = fundamental_dissect_from_buffer;
            break;
        case TYPE_STRUCT:
            new_p->type_hr_display = struct_type_hr_display;
            new_p->type_file_print = struct_type_file_print;
            new_p->type_dissect_from_buffer = struct_dissect_from_buffer;
            break;
        case TYPE_UNION:
            new_p->type_hr_display = union_type_hr_display;
            new_p->type_file_print = union_type_file_print;
            new_p->type_dissect_from_buffer = union_dissect_from_buffer;
            break;
        case TYPE_TYPEDEF:
            new_p->type_hr_display = typedef_type_hr_display;
            new_p->type_file_print = typedef_type_file_print;
            new_p->type_dissect_from_buffer = typedef_dissect_from_buffer;
            break;
        case TYPE_ENUMERATION_VALUE:
            new_p->type_hr_display = enum_value_type_hr_display;
            new_p->type_file_print = enum_value_file_print;
            new_p->type_dissect_from_buffer = enum_value_dissect_from_buffer;
            break;
        case TYPE_FILE:
            new_p->type_hr_display = file_type_hr_display;
            new_p->type_file_print = file_type_file_print;
//             new_p->type_dissect_from_buffer = file_dissect_from_buffer;
            break;
        case TYPE_FIELD:
            new_p->type_hr_display = field_type_hr_display;
            new_p->type_file_print = field_type_file_print;
            new_p->type_dissect_from_buffer = field_dissect_from_buffer;
            break;
        case TYPE_REFERENCE:
            new_p->type_hr_display = reference_type_hr_display;
            new_p->type_file_print = reference_type_file_print;
            new_p->type_dissect_from_buffer = reference_dissect_from_buffer;
            break;
        case TYPE_ARRAY:
            new_p->type_hr_display = array_type_hr_display;
            new_p->type_file_print = array_type_file_print;
            new_p->type_dissect_from_buffer = array_dissect_from_buffer;
            break;
        case TYPE_POINTER:
            new_p->type_hr_display = pointer_type_hr_display;
            new_p->type_file_print = pointer_type_file_print;
            new_p->type_dissect_from_buffer = pointer_dissect_from_buffer;
            break;
        default:
            break;
    }

    return new_p;
}

int types_insert_tail(types_t **head, types_t *to_insert)
{
    if (to_insert == NULL || head == NULL)
        return -1;
    if (*head == NULL) {
        *head = to_insert;
    } else {
        types_t *last = *head;
        while(last->next != NULL) {
            last = last->next;
        }
        last->next = to_insert;
        to_insert->previous = last;
    }
    to_insert->head = *head;
    return 0;
}

void types_hr_display(types_t *head)
{
    types_t *current = head;

    if (head == NULL) {
        printf("Empty list\n");
        /* Empty list */
        return;
    }

    while((current = current->next) != NULL)
    {
        current->type_hr_display(current, 0);
    }
}
