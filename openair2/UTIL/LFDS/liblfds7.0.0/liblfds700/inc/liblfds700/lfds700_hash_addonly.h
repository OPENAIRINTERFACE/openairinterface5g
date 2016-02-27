/***** defines *****/
#define LFDS700_HASH_A_GET_KEY_FROM_ELEMENT( hash_a_element )             ( (hash_a_element).key )
#define LFDS700_HASH_A_SET_KEY_IN_ELEMENT( hash_a_element, new_key )      ( (hash_a_element).key = (void *) (lfds700_pal_uint_t) (new_key) )
#define LFDS700_HASH_A_GET_VALUE_FROM_ELEMENT( hash_a_element )           ( LFDS700_MISC_BARRIER_LOAD, (hash_a_element).value )
#define LFDS700_HASH_A_SET_VALUE_IN_ELEMENT( hash_a_element, new_value )  { void *local_new_value = (void *) (lfds700_pal_uint_t) (new_value); LFDS700_PAL_ATOMIC_EXCHANGE( &(hash_a_element).value, &local_new_value ); }
#define LFDS700_HASH_A_GET_USER_STATE_FROM_STATE( hash_a_state )          ( (hash_a_state).user_state )

#define LFDS700_HASH_A_32BIT_HASH_FUNCTION( data, data_length_in_bytes, hash )  {                                                           \
                                                                                  lfds700_pal_uint_t                                        \
                                                                                    loop;                                                   \
                                                                                                                                            \
                                                                                  for( loop = 0 ; loop < (data_length_in_bytes) ; loop++ )  \
                                                                                  {                                                         \
                                                                                    (hash) += *( (char unsigned *) (data) + loop );         \
                                                                                    (hash) += ((hash) << 10);                               \
                                                                                    (hash) ^= ((hash) >> 6);                                \
                                                                                  }                                                         \
                                                                                                                                            \
                                                                                  (hash) += ((hash) << 3);                                  \
                                                                                  (hash) ^= ((hash) >> 11);                                 \
                                                                                  (hash) += ((hash) << 15);                                 \
                                                                                }
  /* TRD : this is the Jenkins one-at-a-time hash
           it produces a 32 bit hash
           http://en.wikipedia.org/wiki/Jenkins_hash_function

           we ourselves do *not* initialize the value of *hash, so that
           our caller has the option to call us multiple times, each
           time with for example a different member of a struct, which is
           then hashed into the existing, built-up-so-far hash value, and
           so build up a quality hash
  */

/***** enums *****/
enum lfds700_hash_a_existing_key
{
  LFDS700_HASH_A_EXISTING_KEY_OVERWRITE,
  LFDS700_HASH_A_EXISTING_KEY_FAIL
};

enum lfds700_hash_a_insert_result
{
  LFDS700_HASH_A_PUT_RESULT_FAILURE_EXISTING_KEY,
  LFDS700_HASH_A_PUT_RESULT_SUCCESS_OVERWRITE,
  LFDS700_HASH_A_PUT_RESULT_SUCCESS
};

enum lfds700_hash_a_query
{
  LFDS700_HASH_A_QUERY_GET_POTENTIALLY_INACCURATE_COUNT,
  LFDS700_HASH_A_QUERY_SINGLETHREADED_VALIDATE
};

/***** structs *****/
struct lfds700_hash_a_element
{
  struct lfds700_btree_au_element
    baue;

  void
    *key;

  void LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    *volatile value;
};

struct lfds700_hash_a_iterate
{
  struct lfds700_btree_au_element
    *baue;

  struct lfds700_btree_au_state
    *baus,
    *baus_end;
};

struct lfds700_hash_a_state
{
  enum lfds700_hash_a_existing_key
    existing_key;

  int
    (*key_compare_function)( void const *new_key, void const *existing_key );

  lfds700_pal_uint_t
    array_size;

  struct lfds700_btree_au_state
    *baus_array;

  void
    (*element_cleanup_callback)( struct lfds700_hash_a_state *has, struct lfds700_hash_a_element *hae ),
    (*key_hash_function)( void const *key, lfds700_pal_uint_t *hash ),
    *user_state;
};

/***** public prototypes *****/
void lfds700_hash_a_init_valid_on_current_logical_core( struct lfds700_hash_a_state *has,
                                                        struct lfds700_btree_au_state *baus_array,
                                                        lfds700_pal_uint_t array_size,
                                                        int (*key_compare_function)(void const *new_key, void const *existing_key),
                                                        void (*key_hash_function)(void const *key, lfds700_pal_uint_t *hash),
                                                        enum lfds700_hash_a_existing_key existing_key,
                                                        void *user_state );
  // TRD : used in conjunction with the #define LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE

void lfds700_hash_a_cleanup( struct lfds700_hash_a_state *has,
                             void (*element_cleanup_function)(struct lfds700_hash_a_state *has, struct lfds700_hash_a_element *hae) );

enum lfds700_hash_a_insert_result lfds700_hash_a_insert( struct lfds700_hash_a_state *has,
                                                         struct lfds700_hash_a_element *hae,
                                                         struct lfds700_hash_a_element **existing_hae,
                                                         struct lfds700_misc_prng_state *ps );
  // TRD : if existing_value is not NULL and the key exists, existing_value is set to the value of the existing key

int lfds700_hash_a_get_by_key( struct lfds700_hash_a_state *has,
                               void *key,
                               struct lfds700_hash_a_element **hae );

void lfds700_hash_a_iterate_init( struct lfds700_hash_a_state *has, struct lfds700_hash_a_iterate *hai );
int lfds700_hash_a_iterate( struct lfds700_hash_a_iterate *hai, struct lfds700_hash_a_element **hae );

void lfds700_hash_a_query( struct lfds700_hash_a_state *has,
                           enum lfds700_hash_a_query query_type,
                           void *query_input,
                           void *query_output );

