#ifndef INTERTASK_INTERFACE_DUMP_H_
#define INTERTASK_INTERFACE_DUMP_H_

int itti_dump_queue_message(task_id_t sender_task, message_number_t message_number, MessageDef *message_p, const char *message_name,
                            const uint32_t message_size);

int itti_dump_init(const char * const messages_definition_xml, const char * const dump_file_name);

void itti_dump_exit(void);

void itti_dump_thread_use_ring_buffer(void);

#endif /* INTERTASK_INTERFACE_DUMP_H_ */
