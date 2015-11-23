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

#define G_LOG_DOMAIN ("RESOLVER")

#include <glib.h>

#include "types.h"
#include "resolvers.h"

typedef enum {
    RESOLV_OK = 0,
    RESOLV_ERROR = -1,
    RESOLV_LIST_EMPTY = -2,
    RESOLV_NOT_FOUND = -3,
} resolv_rc_e;

static int search_id(types_t *head, types_t **found, int id)
{
    types_t *next_type;

    if (!head) {
        g_warning("Empty list detected");
        return RESOLV_LIST_EMPTY;
    }

    for (next_type = head; next_type; next_type = next_type->next)
    {
        if (next_type->id == id && next_type->type != TYPE_FILE)
            break;
    }
    if (found)
        *found = next_type;

    return next_type == NULL ? RESOLV_NOT_FOUND : RESOLV_OK;
}

int search_file(types_t *head, types_t **found, int file_id)
{
    types_t *next_type;

    if (!head) {
        g_warning("Empty list detected");
        return RESOLV_LIST_EMPTY;
    }

    for (next_type = head; next_type; next_type = next_type->next)
    {
        if (next_type->type != TYPE_FILE)
            continue;
        if (file_id == next_type->id)
            break;
    }
    if (found)
        *found = next_type;

    return next_type == NULL ? RESOLV_NOT_FOUND : RESOLV_OK;
}

int resolve_typedefs(types_t **head)
{
    types_t *next_type;

    if (!head) {
        g_warning("Empty list detected");
        return RESOLV_LIST_EMPTY;
    }

    for (next_type = *head; next_type; next_type = next_type->next)
    {
        /* Only resolve typedef */
        if (next_type->type != TYPE_TYPEDEF)
            continue;

//         printf("Trying to resolve typedef %s with type %d\n", next_type->name, next_type->id);

        if (search_id(*head, &next_type->child, next_type->type_xml) != RESOLV_OK) {
            /* We have to remove this reference */
        }/* else {
            next_type->type_hr_display(next_type, 0);
        }*/
    }

    return 0;
}

int resolve_struct(types_t **head)
{
    types_t *next_type;

    if (!head) {
        g_warning("Empty list detected");
        return RESOLV_LIST_EMPTY;
    }

    for (next_type = *head; next_type; next_type = next_type->next)
    {
        char *member;
        char *members;

        /* Only resolve typedef */
        if (next_type->type != TYPE_STRUCT)
            continue;

        g_debug("Trying to resolve struct members %s with type %d", next_type->name, next_type->id);

        /* Struct may have no member */
        if (next_type->members == NULL)
            continue;

        /* We have to copy the string as strtok will split the string in argument */
        members = strdup(next_type->members);

        /* Split the string on spaces and _ */
        member = strtok(members, " _");
        while(member != NULL)
        {
            if (next_type->nb_members == 0) {
                next_type->members_child = malloc(sizeof(types_t *));
            } else {
                next_type->members_child = realloc(next_type->members_child,
                                                (next_type->nb_members + 1) * sizeof(types_t *));
            }
            if (search_id(*head, &next_type->members_child[next_type->nb_members], atoi(member)) != RESOLV_OK) {
                /* We have to remove this reference */
            }
            if ((next_type->members_child[next_type->nb_members] != NULL)
                    && (next_type->members_child[next_type->nb_members]->type != TYPE_FIELD))
            {
                /* Only keep field child for structure, other member can be present
                 *  for defining types (union or struct) used by fields but should not be considered. */
                next_type->members_child[next_type->nb_members] = NULL;
                /* We have to remove this reference */
            }
            next_type->nb_members++;
            /* Pick up the next string available */
            member = strtok(NULL, " _");
        }

        free(members);
    }

    return 0;
}

int resolve_union(types_t **head)
{
    types_t *next_type;

    if (!head) {
        g_warning("Empty list detected");
        return RESOLV_LIST_EMPTY;
    }

    for (next_type = *head; next_type; next_type = next_type->next)
    {
        char *member;
        char *members;

        /* Only resolve typedef */
        if (next_type->type != TYPE_UNION)
            continue;

        g_debug("Trying to resolve union members %s with type %d\n", next_type->name, next_type->id);

        /* Struct may have no member */
        if (next_type->members == NULL)
            continue;

        /* We have to copy the string as strtok will modify the string in argument */
        members = strdup(next_type->members);

        /* Split the string on spaces and _ */
        member = strtok(members, " _");
        while(member != NULL)
        {
            if (next_type->nb_members == 0) {
                next_type->members_child = malloc(sizeof(types_t *));
            } else {
                next_type->members_child = realloc(next_type->members_child,
                                                (next_type->nb_members + 1) * sizeof(types_t *));
            }
            if (search_id(*head, &next_type->members_child[next_type->nb_members], atoi(member)) != RESOLV_OK) {
                /* We have to remove this reference */
            }
            next_type->nb_members++;
            /* Pick up the next string available */
            member = strtok(NULL, " _");
        }

        free(members);
    }

    return 0;
}

int resolve_pointer_type(types_t **head)
{
    types_t *next_type;

    if (!head) {
        g_warning("Empty list detected");
        return RESOLV_LIST_EMPTY;
    }

    for (next_type = *head; next_type; next_type = next_type->next)
    {
        /* Only resolve typedef */
        if (next_type->type != TYPE_POINTER)
            continue;

        g_debug("Trying to resolve pointer id %d with type %d",
                next_type->id, next_type->type_xml);

        if (search_id(*head, &next_type->child, next_type->type_xml) != RESOLV_OK) {
            /* We have to remove this reference */
        }
//         next_type->type_hr_display(next_type, 0);
    }

    return 0;
}

int resolve_reference(types_t **head)
{
    types_t *next_type;

    if (!head) {
        g_warning("Empty list detected");
        return RESOLV_LIST_EMPTY;
    }

    for (next_type = *head; next_type; next_type = next_type->next)
    {
        /* Only resolve typedef */
        if (next_type->type != TYPE_REFERENCE)
            continue;

        g_debug("Trying to resolve reference id %d with type %d\n",
                next_type->id, next_type->type_xml);

        if (search_id(*head, &next_type->child, next_type->type_xml) != RESOLV_OK) {
            /* We have to remove this reference */
        }
//         next_type->type_hr_display(next_type, 0);
    }

    return 0;
}

int resolve_field(types_t **head)
{
    types_t *next_type;

    if (!head) {
        g_warning("Empty list detected");
        return RESOLV_LIST_EMPTY;
    }

    for (next_type = *head; next_type; next_type = next_type->next)
    {
        /* Only resolve typedef */
        if (next_type->type != TYPE_FIELD)
            continue;

        g_debug("Trying to resolve pointer id %d with type %d\n",
                next_type->id, next_type->type_xml);

        if (search_id(*head, &next_type->child, next_type->type_xml) != RESOLV_OK) {
            /* We have to remove this reference */
        }
//         next_type->type_hr_display(next_type, 0);
    }

    return 0;
}

int resolve_array(types_t **head)
{
    types_t *next_type;

    if (!head) {
        g_warning("Empty list detected");
        return RESOLV_LIST_EMPTY;
    }

    for (next_type = *head; next_type; next_type = next_type->next)
    {
        /* Only resolve typedef */
        if (next_type->type != TYPE_ARRAY)
            continue;

        g_debug("Trying to resolve array id %d with type %d\n",
                next_type->id, next_type->type_xml);

        if (search_id(*head, &next_type->child, next_type->type_xml) != RESOLV_OK) {
            /* We have to remove this reference */
        }
//         next_type->type_hr_display(next_type, 0);
    }

    return 0;
}

int resolve_function(types_t **head)
{
    types_t *next_type;

    if (!head) {
        g_warning("Empty list detected");
        return RESOLV_LIST_EMPTY;
    }

    for (next_type = *head; next_type; next_type = next_type->next)
    {
        /* Only resolve typedef */
        if (next_type->type != TYPE_FUNCTION)
            continue;

        g_debug("Trying to resolve function id %d with type %d",
                next_type->id, next_type->type_xml);

        if (search_id(*head, &next_type->child, next_type->type_xml) != RESOLV_OK) {
            /* We have to remove this reference */
        }
//         next_type->type_hr_display(next_type, 0);
    }

    return 0;
}

int resolve_file(types_t **head)
{
    types_t *next_type;

    if (!head) {
        g_warning("Empty list detected");
        return RESOLV_LIST_EMPTY;
    }

    for (next_type = *head; next_type; next_type = next_type->next)
    {
        /* Only resolve typedef */
        if (next_type->type == TYPE_FILE)
            continue;

        /* No reference to a file */
        if (next_type->file == NULL) {
            continue;
        }

        g_debug("Trying to resolve file %s\n", next_type->file);

        if (search_file(*head, &next_type->file_ref, atoi(&next_type->file[1])) != RESOLV_OK) {
            /* We have to remove this reference */
        }
//         next_type->type_hr_display(next_type, 0);
    }

    return 0;
}

