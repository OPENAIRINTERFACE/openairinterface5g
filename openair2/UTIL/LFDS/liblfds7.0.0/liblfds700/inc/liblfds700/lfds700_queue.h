/***** defines *****/
#define LFDS700_QUEUE_GET_KEY_FROM_ELEMENT( queue_element )             ( (queue_element).key )
#define LFDS700_QUEUE_SET_KEY_IN_ELEMENT( queue_element, new_key )      ( (queue_element).key = (void *) (lfds700_pal_uint_t) (new_key) )
#define LFDS700_QUEUE_GET_VALUE_FROM_ELEMENT( queue_element )           ( (queue_element).value )
#define LFDS700_QUEUE_SET_VALUE_IN_ELEMENT( queue_element, new_value )  ( (queue_element).value = (void *) (lfds700_pal_uint_t) (new_value) )
#define LFDS700_QUEUE_GET_USER_STATE_FROM_STATE( queue_state )          ( (queue_state).user_state )

/***** enums *****/
enum lfds700_queue_query
{
  LFDS700_QUEUE_QUERY_SINGLETHREADED_GET_COUNT,
  LFDS700_QUEUE_QUERY_SINGLETHREADED_VALIDATE
};

/***** structures *****/
struct lfds700_queue_element
{
  struct lfds700_queue_element LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    *volatile next[PAC_SIZE];

  void LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    *key;

  void
    *value;
};

struct lfds700_queue_state
{
  struct lfds700_queue_element LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    *volatile enqueue[PAC_SIZE],
    *volatile dequeue[PAC_SIZE];

  void LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    *user_state;
};

/***** public prototypes *****/
void lfds700_queue_init_valid_on_current_logical_core( struct lfds700_queue_state *qs,
                                                       struct lfds700_queue_element *qe_dummy,
                                                       struct lfds700_misc_prng_state *ps,
                                                       void *user_state );
  // TRD : used in conjunction with the #define LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE

void lfds700_queue_cleanup( struct lfds700_queue_state *qs,
                            void (*element_cleanup_callback)(struct lfds700_queue_state *qs, struct lfds700_queue_element *qe, enum lfds700_misc_flag dummy_element_flag) );

void lfds700_queue_enqueue( struct lfds700_queue_state *qs,
                            struct lfds700_queue_element *qe,
                            struct lfds700_misc_prng_state *ps );

int lfds700_queue_dequeue( struct lfds700_queue_state *qs,
                           struct lfds700_queue_element **qe,
                           struct lfds700_misc_prng_state *ps );

void lfds700_queue_query( struct lfds700_queue_state *qs,
                          enum lfds700_queue_query query_type,
                          void *query_input,
                          void *query_output );

