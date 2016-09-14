#include "types.h"

#ifndef ENUM_TYPE_H_
#define ENUM_TYPE_H_

char *enum_type_get_name_from_value(types_t *type, uint32_t value);

int enum_type_dissect_from_buffer(
    types_t *type, ui_set_signal_text_cb_t ui_set_signal_text_cb, gpointer user_data,
    buffer_t *buffer, uint32_t offset, uint32_t parent_offset, int indent, gboolean new_line);

int enum_type_file_print(types_t *type, int indent, FILE *file);

int enum_type_hr_display(types_t *type, int indent);

#endif /* ENUM_TYPE_H_ */
