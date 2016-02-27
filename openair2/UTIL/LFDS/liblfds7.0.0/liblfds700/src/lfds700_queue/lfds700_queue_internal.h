/***** the library wide include file *****/
#include "../liblfds700_internal.h"

/***** enums *****/
enum lfds700_queue_queue_state
{
  LFDS700_QUEUE_QUEUE_STATE_UNKNOWN, 
  LFDS700_QUEUE_QUEUE_STATE_EMPTY,
  LFDS700_QUEUE_QUEUE_STATE_ENQUEUE_OUT_OF_PLACE,
  LFDS700_QUEUE_QUEUE_STATE_ATTEMPT_DEQUEUE
};

/***** private prototypes *****/

