/***** defines *****/
#define LFDS700_BTREE_AU_GET_KEY_FROM_ELEMENT( btree_au_element )             ( (btree_au_element).key )
#define LFDS700_BTREE_AU_SET_KEY_IN_ELEMENT( btree_au_element, new_key )      ( (btree_au_element).key = (void *) (lfds700_pal_uint_t) (new_key) )
#define LFDS700_BTREE_AU_GET_VALUE_FROM_ELEMENT( btree_au_element )           ( LFDS700_MISC_BARRIER_LOAD, (btree_au_element).value )
#define LFDS700_BTREE_AU_SET_VALUE_IN_ELEMENT( btree_au_element, new_value )  { void *local_new_value = (void *) (lfds700_pal_uint_t) (new_value); LFDS700_PAL_ATOMIC_EXCHANGE( &(btree_au_element).value, &local_new_value ); }
#define LFDS700_BTREE_AU_GET_USER_STATE_FROM_STATE( btree_au_state )          ( (btree_au_state).user_state )

/***** enums *****/
enum lfds700_btree_au_absolute_position
{
  LFDS700_BTREE_AU_ABSOLUTE_POSITION_ROOT,
  LFDS700_BTREE_AU_ABSOLUTE_POSITION_SMALLEST_IN_TREE,
  LFDS700_BTREE_AU_ABSOLUTE_POSITION_LARGEST_IN_TREE
};

enum lfds700_btree_au_existing_key
{
  LFDS700_BTREE_AU_EXISTING_KEY_OVERWRITE,
  LFDS700_BTREE_AU_EXISTING_KEY_FAIL
};

enum lfds700_btree_au_insert_result
{
  LFDS700_BTREE_AU_INSERT_RESULT_FAILURE_EXISTING_KEY,
  LFDS700_BTREE_AU_INSERT_RESULT_SUCCESS_OVERWRITE,
  LFDS700_BTREE_AU_INSERT_RESULT_SUCCESS
};

enum lfds700_btree_au_relative_position
{
  LFDS700_BTREE_AU_RELATIVE_POSITION_UP,
  LFDS700_BTREE_AU_RELATIVE_POSITION_LEFT,
  LFDS700_BTREE_AU_RELATIVE_POSITION_RIGHT,
  LFDS700_BTREE_AU_RELATIVE_POSITION_SMALLEST_ELEMENT_BELOW_CURRENT_ELEMENT,
  LFDS700_BTREE_AU_RELATIVE_POSITION_LARGEST_ELEMENT_BELOW_CURRENT_ELEMENT,
  LFDS700_BTREE_AU_RELATIVE_POSITION_NEXT_SMALLER_ELEMENT_IN_ENTIRE_TREE,
  LFDS700_BTREE_AU_RELATIVE_POSITION_NEXT_LARGER_ELEMENT_IN_ENTIRE_TREE
};

enum lfds700_btree_au_query
{
  LFDS700_BTREE_AU_QUERY_GET_POTENTIALLY_INACCURATE_COUNT,
  LFDS700_BTREE_AU_QUERY_SINGLETHREADED_VALIDATE
};

/***** structs *****/
struct lfds700_btree_au_element
{
  struct lfds700_btree_au_element LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    *volatile left,
    *volatile right,
    *volatile up;

  void LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    *volatile value;

  void LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    *key;
};

struct lfds700_btree_au_state
{
  struct lfds700_btree_au_element LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    *volatile root;

  int LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    (*key_compare_function)( void const *new_key, void const *existing_key );

  enum lfds700_btree_au_existing_key 
    existing_key;

  void
    *user_state;
};

/***** public prototypes *****/
void lfds700_btree_au_init_valid_on_current_logical_core( struct lfds700_btree_au_state *baus,
                                                          int (*key_compare_function)(void const *new_key, void const *existing_key),
                                                          enum lfds700_btree_au_existing_key existing_key,
                                                          void *user_state );
  // TRD : used in conjunction with the #define LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE

void lfds700_btree_au_cleanup( struct lfds700_btree_au_state *baus,
                               void (*element_cleanup_callback)(struct lfds700_btree_au_state *baus, struct lfds700_btree_au_element *baue) );

enum lfds700_btree_au_insert_result lfds700_btree_au_insert( struct lfds700_btree_au_state *baus,
                                                             struct lfds700_btree_au_element *baue,
                                                             struct lfds700_btree_au_element **existing_baue,
                                                             struct lfds700_misc_prng_state *ps );
  // TRD : if a link collides with an existing key and existing_baue is non-NULL, existing_baue is set to the existing element

int lfds700_btree_au_get_by_key( struct lfds700_btree_au_state *baus, 
                                 void *key,
                                 struct lfds700_btree_au_element **baue );

int lfds700_btree_au_get_by_absolute_position_and_then_by_relative_position( struct lfds700_btree_au_state *baus,
                                                                             struct lfds700_btree_au_element **baue,
                                                                             enum lfds700_btree_au_absolute_position absolute_position,
                                                                             enum lfds700_btree_au_relative_position relative_position );
  // TRD : if *baue is NULL, we get the element at position, otherwise we move from *baue according to direction

int lfds700_btree_au_get_by_absolute_position( struct lfds700_btree_au_state *baus,
                                               struct lfds700_btree_au_element **baue,
                                               enum lfds700_btree_au_absolute_position absolute_position );

int lfds700_btree_au_get_by_relative_position( struct lfds700_btree_au_element **baue,
                                               enum lfds700_btree_au_relative_position relative_position );

void lfds700_btree_au_query( struct lfds700_btree_au_state *baus,
                             enum lfds700_btree_au_query query_type,
                             void *query_input,
                             void *query_output );

