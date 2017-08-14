/***** defines *****/
#define LFDS700_LIST_AOS_GET_START( list_aos_state )                                             ( LFDS700_MISC_BARRIER_LOAD, (list_aos_state).start->next )
#define LFDS700_LIST_AOS_GET_NEXT( list_aos_element )                                            ( LFDS700_MISC_BARRIER_LOAD, (list_aos_element).next )
#define LFDS700_LIST_AOS_GET_START_AND_THEN_NEXT( list_aos_state, pointer_to_list_aos_element )  ( (pointer_to_list_aos_element) == NULL ? ( (pointer_to_list_aos_element) = LFDS700_LIST_AOS_GET_START(list_aos_state) ) : ( (pointer_to_list_aos_element) = LFDS700_LIST_AOS_GET_NEXT(*(pointer_to_list_aos_element)) ) )
#define LFDS700_LIST_AOS_GET_KEY_FROM_ELEMENT( list_aos_element )                                ( (list_aos_element).key )
#define LFDS700_LIST_AOS_SET_KEY_IN_ELEMENT( list_aos_element, new_key )                         ( (list_aos_element).key = (void *) (lfds700_pal_uint_t) (new_key) )
#define LFDS700_LIST_AOS_GET_VALUE_FROM_ELEMENT( list_aos_element )                              ( LFDS700_MISC_BARRIER_LOAD, (list_aos_element).value )
#define LFDS700_LIST_AOS_SET_VALUE_IN_ELEMENT( list_aos_element, new_value )                     { void *local_new_value = (void *) (lfds700_pal_uint_t) (new_value); LFDS700_PAL_ATOMIC_EXCHANGE( &(list_aos_element).value, &local_new_value ); }
#define LFDS700_LIST_AOS_GET_USER_STATE_FROM_STATE( list_aos_state )                             ( (list_aos_state).user_state )

/***** enums *****/
enum lfds700_list_aos_existing_key
{
  LFDS700_LIST_AOS_EXISTING_KEY_OVERWRITE,
  LFDS700_LIST_AOS_EXISTING_KEY_FAIL
};

enum lfds700_list_aos_insert_result
{
  LFDS700_LIST_AOS_INSERT_RESULT_FAILURE_EXISTING_KEY,
  LFDS700_LIST_AOS_INSERT_RESULT_SUCCESS_OVERWRITE,
  LFDS700_LIST_AOS_INSERT_RESULT_SUCCESS
};

enum lfds700_list_aos_query
{
  LFDS700_LIST_AOS_QUERY_GET_POTENTIALLY_INACCURATE_COUNT,
  LFDS700_LIST_AOS_QUERY_SINGLETHREADED_VALIDATE
};

/***** structures *****/
struct lfds700_list_aos_element
{
  struct lfds700_list_aos_element LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    *volatile next;

  void LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    *volatile value;

  void LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    *key;
};

struct lfds700_list_aos_state
{
  struct lfds700_list_aos_element LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    *volatile start;

  struct lfds700_list_aos_element LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    dummy_element;

  int LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    (*key_compare_function)( void const *new_key, void const *existing_key );

  enum lfds700_list_aos_existing_key
    existing_key;

  void
    *user_state;
};

/***** public prototypes *****/
void lfds700_list_aos_init_valid_on_current_logical_core( struct lfds700_list_aos_state *laoss,
                                                          int (*key_compare_function)(void const *new_key, void const *existing_key),
                                                          enum lfds700_list_aos_existing_key existing_key,
                                                          void *user_state );
  // TRD : used in conjunction with the #define LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE

void lfds700_list_aos_cleanup( struct lfds700_list_aos_state *laoss,
                               void (*element_cleanup_callback)(struct lfds700_list_aos_state *laoss, struct lfds700_list_aos_element *laose) );

enum lfds700_list_aos_insert_result lfds700_list_aos_insert( struct lfds700_list_aos_state *laoss,
                                                             struct lfds700_list_aos_element *laose,
                                                             struct lfds700_list_aos_element **existing_laose,
                                                             struct lfds700_misc_prng_state *ps );

int lfds700_list_aos_get_by_key( struct lfds700_list_aos_state *laoss,
                                 void *key,
                                 struct lfds700_list_aos_element **laose );

void lfds700_list_aos_query( struct lfds700_list_aos_state *laoss,
                             enum lfds700_list_aos_query query_type,
                             void *query_input,
                             void *query_output );

