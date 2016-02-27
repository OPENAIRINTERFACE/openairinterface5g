/***** enums *****/
#define LFDS700_RINGBUFFER_GET_USER_STATE_FROM_STATE( ringbuffer_state )  ( (ringbuffer_state).user_state )

/***** enums *****/
enum lfds700_ringbuffer_query
{
  LFDS700_RINGBUFFER_QUERY_SINGLETHREADED_GET_COUNT,
  LFDS700_RINGBUFFER_QUERY_SINGLETHREADED_VALIDATE
};

/***** structures *****/
struct lfds700_ringbuffer_element
{
  struct lfds700_freelist_element LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    fe;

  struct lfds700_queue_element LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    qe;

  struct lfds700_queue_element
    *qe_use; // TRD : hack for 7.0.0; we need a new queue with no dummy element

  void
    *key,
    *value;
};

struct lfds700_ringbuffer_state
{
  struct lfds700_freelist_state LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    fs;

  struct lfds700_queue_state LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    qs;

  void
    (*element_cleanup_callback)( struct lfds700_ringbuffer_state *rs, void *key, void *value, enum lfds700_misc_flag unread_flag ),
    *user_state;
};

/***** public prototypes *****/
void lfds700_ringbuffer_init_valid_on_current_logical_core( struct lfds700_ringbuffer_state *rs,
                                                            struct lfds700_ringbuffer_element *re_array_inc_dummy,
                                                            lfds700_pal_uint_t number_elements,
                                                            struct lfds700_misc_prng_state *ps,
                                                            void *user_state );
  // TRD : used in conjunction with the #define LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE

void lfds700_ringbuffer_cleanup( struct lfds700_ringbuffer_state *rs,
                                 void (*element_cleanup_callback)(struct lfds700_ringbuffer_state *rs, void *key, void *value, enum lfds700_misc_flag unread_flag) );

int lfds700_ringbuffer_read( struct lfds700_ringbuffer_state *rs,
                             void **key,
                             void **value,
                             struct lfds700_misc_prng_state *ps );

void lfds700_ringbuffer_write( struct lfds700_ringbuffer_state *rs,
                               void *key,
                               void *value,
                               enum lfds700_misc_flag *overwrite_occurred_flag,
                               void **overwritten_key,
                               void **overwritten_value,
                               struct lfds700_misc_prng_state *ps );

void lfds700_ringbuffer_query( struct lfds700_ringbuffer_state *rs,
                               enum lfds700_ringbuffer_query query_type,
                               void *query_input,
                               void *query_output );

