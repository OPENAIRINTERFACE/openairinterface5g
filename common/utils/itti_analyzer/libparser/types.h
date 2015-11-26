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

#include "buffers.h"
#include "ui_main_screen.h"
#include "ui_interface.h"

#ifndef TYPES_H_
#define TYPES_H_

/* Activate to display the type at the beginning of the line (debug option) */
#define ENABLE_DISPLAY_TYPE         0

/* Activate to display the parse information before processing each item (debug option) */
#define ENABLE_DISPLAY_PARSE_INFO   0

/* Activate to show braces, in increase the number of displayed lines (formating option)*/
#define ENABLE_DISPLAY_BRACE        1

#if (ENABLE_DISPLAY_TYPE != 0)
# define DISPLAY_TYPE(tYPE) ui_set_signal_text_cb(user_data, tYPE" ", strlen(tYPE) + 1);
#else
# define DISPLAY_TYPE(tYPE)
#endif

#define DISPLAY_TAB_SIZE        (2)

#if (ENABLE_DISPLAY_PARSE_INFO != 0)
# define DISPLAY_PARSE_INFO(tYPE, nAME, oFFSET, pARENToFFSET)                       \
    {                                                                               \
        char buf[200];                                                              \
        sprintf(buf, "/* %s \"%s\" %d %d */\n", tYPE, nAME, oFFSET, pARENToFFSET);  \
        ui_set_signal_text_cb(user_data, buf, strlen(buf));                         \
    }
#else
# define DISPLAY_PARSE_INFO(tYPE, nAME, oFFSET, pARENToFFSET)
#endif

#if (ENABLE_DISPLAY_BRACE != 0)
# define DISPLAY_BRACE(cODE) if (ui_main_data.display_brace) {cODE}
#else
# define DISPLAY_BRACE(cODE)
#endif

enum type_e {
    TYPE_ENUMERATION,
    TYPE_ENUMERATION_VALUE,
    TYPE_STRUCT,
    TYPE_UNION,
    TYPE_FUNDAMENTAL,
    TYPE_TYPEDEF,
    TYPE_ARRAY,
    TYPE_REFERENCE,
    TYPE_FIELD,
    TYPE_FUNCTION,
    TYPE_ARGUMENT,
    TYPE_POINTER,
    TYPE_FILE,
};

/* Forward declarations */
struct types_s;

typedef int (*type_hr_display_t)(struct types_s *type, int indent);
typedef int (*type_ui_display_t)(struct types_s *type, int indent);
typedef int (*type_file_print_t)(struct types_s *type, int indent, FILE *file);

/**
 * type_dissect_from_buffer_t
 * @param type The current type
 * @param ui_set_signal_text_cb GUI display function
 * @param user_data Transparent data to pass to the GUI display function
 * @param buffer The buffer containing data to dissect
 * @param offset offset of field from the beginning of the parent
 * @param parent_offset offset of the parent from begining
 **/
typedef int (*type_dissect_from_buffer_t)(
    struct types_s *type, ui_set_signal_text_cb_t ui_set_signal_text_cb, gpointer user_data,
    buffer_t *buffer, uint32_t offset, uint32_t parent_offset, int indent, gboolean new_line);

typedef struct types_s {
    /* The type of the current description */
    enum type_e type;

    /* Printable name for the current type */
    char *name;

    int   type_xml;

    int   size;
    int   align;
    int   bits;

    /* Used only for arrays */
    int   min;
    int   max;

    int   context;

    /* Init value for enumerations */
    int init_value;

    int incomplete;

    /* Id of the type as defined in XML file */
    int id;
    int artificial;

    char *mangled;
    char *demangled;

    /* List of members in constructed types */
    char *members;

    /* The file containing the definition */
    char *file;
    /* Line number of the current definition */
    int   line;

    /* offset of the field in the parent type
     * -1 means no parent
     */
    int offset;

    struct types_s *previous;
    struct types_s *next;
    struct types_s *parent;
    struct types_s *child;
    struct types_s *file_ref;
    /* Reference to the head */
    struct types_s *head;

    /* For structures or union */
    int nb_members;
    struct types_s **members_child;

    /* Some procedures to display the type on terminal */
    type_hr_display_t type_hr_display;
    /* Some procedures to display the type on UI */
    type_ui_display_t type_ui_display;
    /* Procedure to display the type to a file */
    type_file_print_t type_file_print;
    /* Dissect the type */
    type_dissect_from_buffer_t type_dissect_from_buffer;
} types_t;

types_t *type_new(enum type_e type);

int types_insert_tail(types_t **head, types_t *to_insert);

void types_hr_display(types_t *head);

#define INDENTED(fILE, x, y)            \
do {                                    \
    int indentation = x;                \
    while(indentation--) fprintf(fILE, " ");  \
    y;                                  \
} while(0)

#define INDENTED_STRING(sTR, x, y)              \
do {                                            \
    int indentation = x;                        \
    while(indentation--) ui_set_signal_text_cb(user_data, " ", 1);     \
    y;                                          \
} while(0)

#endif /* TYPES_H_ */
