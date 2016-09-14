#ifndef MEMORY_POOLS_H_
#define MEMORY_POOLS_H_

#include <stdint.h>

typedef void * memory_pools_handle_t;
typedef void * memory_pool_item_handle_t;

memory_pools_handle_t memory_pools_create (uint32_t pools_number);

char *memory_pools_statistics(memory_pools_handle_t memory_pools_handle);

int memory_pools_add_pool (memory_pools_handle_t memory_pools_handle, uint32_t pool_items_number, uint32_t pool_item_size);

memory_pool_item_handle_t memory_pools_allocate (memory_pools_handle_t memory_pools_handle, uint32_t item_size, uint16_t info_0, uint16_t info_1);

int memory_pools_free (memory_pools_handle_t memory_pools_handle, memory_pool_item_handle_t memory_pool_item_handle, uint16_t info_0);

void memory_pools_set_info (memory_pools_handle_t memory_pools_handle, memory_pool_item_handle_t memory_pool_item_handle, int index, uint16_t info);

#endif /* MEMORY_POOLS_H_ */
