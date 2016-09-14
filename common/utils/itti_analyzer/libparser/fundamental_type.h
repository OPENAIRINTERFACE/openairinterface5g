#include "types.h"

#ifndef FUNDAMENTAL_TYPE_H_
#define FUNDAMENTAL_TYPE_H_

uint64_t fundamental_read_from_buffer(types_t *type, buffer_t *buffer, uint32_t offset, uint32_t parent_offset);

int fundamental_dissect_from_buffer(
    types_t *type, ui_set_signal_text_cb_t ui_set_signal_text_cb, gpointer user_data,
    buffer_t *buffer, uint32_t offset, uint32_t parent_offset, int indent, gboolean new_line);

int fundamental_type_file_print(types_t *type, int indent, FILE *file);

int fundamental_type_hr_display(types_t *type, int indent);

#endif /* FUNDAMENTAL_TYPE_H_ */
