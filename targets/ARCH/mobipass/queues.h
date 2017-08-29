#ifndef _QUEUES_H_
#define _QUEUES_H_

#include <stdint.h>

void enqueue_to_mobipass(void *qstate, void *data);
void dequeue_to_mobipass(void *qstate, uint32_t timestamp, void *data);

void enqueue_from_mobipass(void *qstate, void *receive_packet);
void dequeue_from_mobipass(void *qstate, uint32_t timestamp, void *data);

/* returns a queue state type, as opaque data structure */
void *init_queues(int samples_per_1024_frames);

#endif /* _QUEUES_H_ */
