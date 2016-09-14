#include "types.h"

#ifndef TYPEDEF_TYPE_H_
#define TYPEDEF_TYPE_H_

int typedef_type_file_print(types_t *type, int indent, FILE *file);

int typedef_type_hr_display(types_t *type, int indent);

int typedef_dissect_from_buffer(
    types_t *type, ui_set_signal_text_cb_t ui_set_signal_text_cb, gpointer user_data,
    buffer_t *buffer, uint32_t offset, uint32_t parent_offset, int indent, gboolean new_line);

#endif /* TYPEDEF_TYPE_H_ */
