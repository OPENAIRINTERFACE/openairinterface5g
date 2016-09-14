#include "types.h"

#ifndef REFERENCE_TYPE_H_
#define REFERENCE_TYPE_H_

int reference_dissect_from_buffer(
    types_t *type, ui_set_signal_text_cb_t ui_set_signal_text_cb, gpointer user_data,
    buffer_t *buffer, uint32_t offset, uint32_t parent_offset, int indent, gboolean new_line);

int reference_type_file_print(types_t *type, int indent, FILE *file);

int reference_type_hr_display(types_t *type, int indent);

#endif /* REFERENCE_TYPE_H_ */
