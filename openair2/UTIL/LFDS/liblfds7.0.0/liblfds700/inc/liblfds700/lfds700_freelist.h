/***** defines *****/
#define LFDS700_FREELIST_GET_KEY_FROM_ELEMENT( freelist_element )             ( (freelist_element).key )
#define LFDS700_FREELIST_SET_KEY_IN_ELEMENT( freelist_element, new_key )      ( (freelist_element).key = (void *) (lfds700_pal_uint_t) (new_key) )
#define LFDS700_FREELIST_GET_VALUE_FROM_ELEMENT( freelist_element )           ( (freelist_element).value )
#define LFDS700_FREELIST_SET_VALUE_IN_ELEMENT( freelist_element, new_value )  ( (freelist_element).value = (void *) (lfds700_pal_uint_t) (new_value) )
#define LFDS700_FREELIST_GET_USER_STATE_FROM_STATE( freelist_state )          ( (freelist_state).user_state )

/***** enums *****/
enum lfds700_freelist_query
{
  LFDS700_FREELIST_QUERY_SINGLETHREADED_GET_COUNT,
  LFDS700_FREELIST_QUERY_SINGLETHREADED_VALIDATE
};

/***** structures *****/
struct lfds700_freelist_element
{
  struct lfds700_freelist_element
    *volatile next;

  void
    *key,
    *value;
};

struct lfds700_freelist_state
{
  struct lfds700_freelist_element LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    *volatile top[PAC_SIZE];

  void LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    *user_state;
};

/***** public prototypes *****/
void lfds700_freelist_init_valid_on_current_logical_core( struct lfds700_freelist_state *fs, void *user_state );
  // TRD : used in conjunction with the #define LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE

void lfds700_freelist_cleanup( struct lfds700_freelist_state *fs,
                               void (*element_cleanup_callback)(struct lfds700_freelist_state *fs, struct lfds700_freelist_element *fe) );

void lfds700_freelist_push( struct lfds700_freelist_state *fs,
                            struct lfds700_freelist_element *fe,
                            struct lfds700_misc_prng_state *ps );

int lfds700_freelist_pop( struct lfds700_freelist_state *fs,
                          struct lfds700_freelist_element **fe,
                          struct lfds700_misc_prng_state *ps );

void lfds700_freelist_query( struct lfds700_freelist_state *fs,
                             enum lfds700_freelist_query query_type,
                             void *query_input,
                             void *query_output );

