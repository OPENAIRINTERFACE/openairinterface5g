#include "types.h"

#ifndef STRUCT_TYPE_H_
#define STRUCT_TYPE_H_

int struct_dissect_from_buffer(
    types_t *type, ui_set_signal_text_cb_t ui_set_signal_text_cb, gpointer user_data,
    buffer_t *buffer, uint32_t offset, uint32_t parent_offset, int indent, gboolean new_line);

int struct_type_hr_display(types_t *type, int indent);

int struct_type_file_print(types_t *type, int indent, FILE *file);

#endif /* STRUCT_TYPE_H_ */
