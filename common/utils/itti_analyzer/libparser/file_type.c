#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>

#define G_LOG_DOMAIN ("PARSER")

#include "file_type.h"

int file_type_file_print(types_t *type, int indent, FILE *file)
{
    if (type == NULL)
        return -1;
    INDENTED(file, indent,   fprintf(file, "<File>\n"));
    INDENTED(file, indent+4, fprintf(file, "Id .........: %d\n", type->id));
    INDENTED(file, indent+4, fprintf(file, "Name .......: %s\n", type->name));
    INDENTED(file, indent,   fprintf(file, "</File>\n"));

    return 0;
}

int file_type_hr_display(types_t *type, int indent)
{
    if (type == NULL)
        return -1;
    INDENTED(stdout, indent,  printf("<File>\n"));
    INDENTED(stdout, indent+4, printf("Id .........: %d\n", type->id));
    INDENTED(stdout, indent+4, printf("Name .......: %s\n", type->name));
    INDENTED(stdout, indent, printf("</File>\n"));

    return 0;
}
