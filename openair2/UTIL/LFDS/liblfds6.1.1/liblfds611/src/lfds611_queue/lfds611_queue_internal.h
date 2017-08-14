/***** the library wide include file *****/
#include "liblfds611_internal.h"

/***** pragmas *****/

/***** defines *****/
#define LFDS611_QUEUE_STATE_UNKNOWN               -1
#define LFDS611_QUEUE_STATE_EMPTY                  0
#define LFDS611_QUEUE_STATE_ENQUEUE_OUT_OF_PLACE   1
#define LFDS611_QUEUE_STATE_ATTEMPT_DELFDS611_QUEUE        2

#define LFDS611_QUEUE_POINTER 0
#define LFDS611_QUEUE_COUNTER 1
#define LFDS611_QUEUE_PAC_SIZE 2

/***** structures *****/
#pragma pack( push, LFDS611_ALIGN_DOUBLE_POINTER )

struct lfds611_queue_state {
  struct lfds611_queue_element
      *volatile enqueue[LFDS611_QUEUE_PAC_SIZE],
      *volatile dequeue[LFDS611_QUEUE_PAC_SIZE];

  lfds611_atom_t
  aba_counter;

  struct lfds611_freelist_state
      *fs;
};

struct lfds611_queue_element {
  // TRD : next in a lfds611_queue requires volatile as it is target of CAS
  struct lfds611_queue_element
      *volatile next[LFDS611_QUEUE_PAC_SIZE];

  struct lfds611_freelist_element
      *fe;

  void
  *user_data;
};

#pragma pack( pop )

/***** externs *****/

/***** private prototypes *****/
int lfds611_queue_internal_freelist_init_function( void **user_data, void *user_state );
void lfds611_queue_internal_freelist_delete_function( void *user_data, void *user_state );

void lfds611_queue_internal_new_element_from_freelist( struct lfds611_queue_state *qs, struct lfds611_queue_element *qe[LFDS611_QUEUE_PAC_SIZE], void *user_data );
void lfds611_queue_internal_guaranteed_new_element_from_freelist( struct lfds611_queue_state *qs, struct lfds611_queue_element * qe[LFDS611_QUEUE_PAC_SIZE], void *user_data );
void lfds611_queue_internal_init_element( struct lfds611_queue_state *qs, struct lfds611_queue_element *qe[LFDS611_QUEUE_PAC_SIZE], struct lfds611_freelist_element *fe, void *user_data );

void lfds611_queue_internal_queue( struct lfds611_queue_state *qs, struct lfds611_queue_element *qe[LFDS611_QUEUE_PAC_SIZE] );

void lfds611_queue_internal_validate( struct lfds611_queue_state *qs, struct lfds611_validation_info *vi, enum lfds611_data_structure_validity *lfds611_queue_validity,
                                      enum lfds611_data_structure_validity *lfds611_freelist_validity );

