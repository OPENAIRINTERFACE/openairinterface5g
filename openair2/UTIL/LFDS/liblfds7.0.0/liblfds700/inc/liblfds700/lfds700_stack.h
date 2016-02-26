/***** defines *****/
#define LFDS700_STACK_GET_KEY_FROM_ELEMENT( stack_element )             ( (stack_element).key )
#define LFDS700_STACK_SET_KEY_IN_ELEMENT( stack_element, new_key )      ( (stack_element).key = (void *) (lfds700_pal_uint_t) (new_key) )
#define LFDS700_STACK_GET_VALUE_FROM_ELEMENT( stack_element )           ( (stack_element).value )
#define LFDS700_STACK_SET_VALUE_IN_ELEMENT( stack_element, new_value )  ( (stack_element).value = (void *) (lfds700_pal_uint_t) (new_value) )
#define LFDS700_STACK_GET_USER_STATE_FROM_STATE( stack_state )          ( (stack_state).user_state )

/***** enums *****/
enum lfds700_stack_query
{
  LFDS700_STACK_QUERY_SINGLETHREADED_GET_COUNT,
  LFDS700_STACK_QUERY_SINGLETHREADED_VALIDATE
};

/***** structures *****/
struct lfds700_stack_element
{
  struct lfds700_stack_element
    *volatile next;

  void
    *key,
    *value;
};

struct lfds700_stack_state
{
  struct lfds700_stack_element LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    *volatile top[PAC_SIZE];

  void LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    *user_state;
};

/***** public prototypes *****/
void lfds700_stack_init_valid_on_current_logical_core( struct lfds700_stack_state *ss, void *user_state );
  // TRD : used in conjunction with the #define LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE

void lfds700_stack_cleanup( struct lfds700_stack_state *ss,
                            void (*element_cleanup_callback)(struct lfds700_stack_state *ss, struct lfds700_stack_element *se) );

void lfds700_stack_push( struct lfds700_stack_state *ss,
                         struct lfds700_stack_element *se,
                         struct lfds700_misc_prng_state *ps );

int lfds700_stack_pop( struct lfds700_stack_state *ss,
                       struct lfds700_stack_element **se,
                       struct lfds700_misc_prng_state *ps );

void lfds700_stack_query( struct lfds700_stack_state *ss,
                          enum lfds700_stack_query query_type,
                          void *query_input,
                          void *query_output );


