/***** the library wide include file *****/
#include "liblfds611_internal.h"

/***** defines *****/

/***** structures *****/
#pragma pack( push, LFDS611_ALIGN_DOUBLE_POINTER )

struct lfds611_ringbuffer_state {
  struct lfds611_queue_state
      *qs;

  struct lfds611_freelist_state
      *fs;
};

#pragma pack( pop )

/***** externs *****/

/***** private prototypes *****/
void lfds611_ringbuffer_internal_validate( struct lfds611_ringbuffer_state *rs, struct lfds611_validation_info *vi, enum lfds611_data_structure_validity *lfds611_queue_validity,
    enum lfds611_data_structure_validity *lfds611_freelist_validity );

