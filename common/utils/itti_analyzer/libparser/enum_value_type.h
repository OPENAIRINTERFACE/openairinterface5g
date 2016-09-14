#include "types.h"

#ifndef ENUM_VALUE_TYPE_H_
#define ENUM_VALUE_TYPE_H_

extern uint32_t last_enum_value;

int enum_value_dissect_from_buffer(
    types_t *type, ui_set_signal_text_cb_t ui_set_signal_text_cb, gpointer user_data,
    buffer_t *buffer, uint32_t offset, uint32_t parent_offset, int indent, gboolean new_line);

int enum_value_file_print(types_t *type, int indent, FILE *file);

int enum_value_type_hr_display(types_t *type, int indent);

#endif /* ENUM_VALUE_TYPE_H_ */
