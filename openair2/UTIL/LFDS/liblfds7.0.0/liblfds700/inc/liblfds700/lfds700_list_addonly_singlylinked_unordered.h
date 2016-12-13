/***** defines *****/
#define LFDS700_LIST_ASU_GET_START( list_asu_state )                                             ( LFDS700_MISC_BARRIER_LOAD, (list_asu_state).start->next )
#define LFDS700_LIST_ASU_GET_NEXT( list_asu_element )                                            ( LFDS700_MISC_BARRIER_LOAD, (list_asu_element).next )
#define LFDS700_LIST_ASU_GET_START_AND_THEN_NEXT( list_asu_state, pointer_to_list_asu_element )  ( (pointer_to_list_asu_element) == NULL ? ( (pointer_to_list_asu_element) = LFDS700_LIST_ASU_GET_START(list_asu_state) ) : ( (pointer_to_list_asu_element) = LFDS700_LIST_ASU_GET_NEXT(*(pointer_to_list_asu_element)) ) )
#define LFDS700_LIST_ASU_GET_KEY_FROM_ELEMENT( list_asu_element )                                ( (list_asu_element).key )
#define LFDS700_LIST_ASU_SET_KEY_IN_ELEMENT( list_asu_element, new_key )                         ( (list_asu_element).key = (void *) (lfds700_pal_uint_t) (new_key) )
#define LFDS700_LIST_ASU_GET_VALUE_FROM_ELEMENT( list_asu_element )                              ( LFDS700_MISC_BARRIER_LOAD, (list_asu_element).value )
#define LFDS700_LIST_ASU_SET_VALUE_IN_ELEMENT( list_asu_element, new_value )                     { void *local_new_value = (void *) (lfds700_pal_uint_t) (new_value); LFDS700_PAL_ATOMIC_EXCHANGE( &(list_asu_element).value, &local_new_value ); }
#define LFDS700_LIST_ASU_GET_USER_STATE_FROM_STATE( list_asu_state )                             ( (list_asu_state).user_state )

/***** enums *****/
enum lfds700_list_asu_position
{
  LFDS700_LIST_ASU_POSITION_START,
  LFDS700_LIST_ASU_POSITION_END,
  LFDS700_LIST_ASU_POSITION_AFTER
};

enum lfds700_list_asu_query
{
  LFDS700_LIST_ASU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT,
  LFDS700_LIST_ASU_QUERY_SINGLETHREADED_VALIDATE
};

/***** structures *****/
struct lfds700_list_asu_element
{
  struct lfds700_list_asu_element LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    *volatile next;

  void LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    *volatile value;

  void LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    *key;
};

struct lfds700_list_asu_state
{
  struct lfds700_list_asu_element LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    *volatile end,
    *volatile start;

  struct lfds700_list_asu_element LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    dummy_element;

  int LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    (*key_compare_function)( void const *new_key, void const *existing_key );

  void
    *user_state;
};

/***** public prototypes *****/
void lfds700_list_asu_init_valid_on_current_logical_core( struct lfds700_list_asu_state *lasus,
                                                          int (*key_compare_function)(void const *new_key, void const *existing_key),
                                                          void *user_state );
  // TRD : used in conjunction with the #define LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE

void lfds700_list_asu_cleanup( struct lfds700_list_asu_state *lasus,
                               void (*element_cleanup_callback)(struct lfds700_list_asu_state *lasus, struct lfds700_list_asu_element *lasue) );

void lfds700_list_asu_insert_at_position( struct lfds700_list_asu_state *lasus,
                                          struct lfds700_list_asu_element *lasue,
                                          struct lfds700_list_asu_element *lasue_predecessor,
                                          enum lfds700_list_asu_position position,
                                          struct lfds700_misc_prng_state *ps );

void lfds700_list_asu_insert_at_start( struct lfds700_list_asu_state *lasus,
                                       struct lfds700_list_asu_element *lasue,
                                       struct lfds700_misc_prng_state *ps );

void lfds700_list_asu_insert_at_end( struct lfds700_list_asu_state *lasus,
                                     struct lfds700_list_asu_element *lasue,
                                     struct lfds700_misc_prng_state *ps );

void lfds700_list_asu_insert_after_element( struct lfds700_list_asu_state *lasus,
                                            struct lfds700_list_asu_element *lasue,
                                            struct lfds700_list_asu_element *lasue_predecessor,
                                            struct lfds700_misc_prng_state *ps );

int lfds700_list_asu_get_by_key( struct lfds700_list_asu_state *lasus,
                                 void *key,
                                 struct lfds700_list_asu_element **lasue );

void lfds700_list_asu_query( struct lfds700_list_asu_state *lasus,
                             enum lfds700_list_asu_query query_type,
                             void *query_input,
                             void *query_output );

