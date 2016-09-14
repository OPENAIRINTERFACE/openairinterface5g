#ifndef LOCATE_ROOT_H_
#define LOCATE_ROOT_H_

#include "types.h"

extern types_t *messages_id_enum;
extern types_t *lte_time_type;
extern types_t *lte_time_frame_type;
extern types_t *lte_time_slot_type;
extern types_t *origin_task_id_type;
extern types_t *destination_task_id_type;
extern types_t *instance_type;
extern types_t *message_header_type;
extern types_t *message_type;
extern types_t *message_size_type;

int locate_root(const char *root_name, types_t *head, types_t **root);

int locate_type(const char *type_name, types_t *head, types_t **type);

int locate_type_children(const char *type_name, types_t *head, types_t **type);

uint32_t get_message_header_type_size(void);

uint32_t get_message_size(buffer_t *buffer);

uint32_t get_lte_frame(buffer_t *buffer);

uint32_t get_lte_slot(buffer_t *buffer);

uint32_t get_message_id(types_t *head, buffer_t *buffer, uint32_t *message_id);

char *message_id_to_string(uint32_t message_id);

uint32_t get_task_id(buffer_t *buffer, types_t *task_id_type);

char *task_id_to_string(uint32_t task_id_value, types_t *task_id_type);

uint32_t get_instance(buffer_t *buffer);

#endif /* LOCATE_ROOT_H_ */
