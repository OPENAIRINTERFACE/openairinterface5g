/***** defines *****/
#define LFDS700_MISC_VERSION_STRING   "7.0.0"
#define LFDS700_MISC_VERSION_INTEGER  700

#ifndef NULL
  #define NULL ( (void *) 0 )
#endif

#define POINTER   0
#define COUNTER   1
#define PAC_SIZE  2

#define LFDS700_MISC_ABSTRACTION_BACKOFF_INITIAL_VALUE  0
#define LFDS700_MISC_PRNG_MAX                           ( (lfds700_pal_uint_t) -1 )
#define LFDS700_MISC_DELIBERATELY_CRASH                 { char *c = 0; *c = 0; }
#define LFDS700_MISC_PRNG_SEED                          0x0a34655d34c092feULL
  /* TRD : from an on-line hardware RNG, using atmospheric noise
           the URL eblow will generate another 16 random hex digits (e.g. a 64-bit number) and is
           the RNG used to generate the number above (0x0a34655d34c092fe)
           http://www.random.org/integers/?num=16&min=0&max=15&col=1&base=16&format=plain&rnd=new

           this seed is a fixed seed which is used for the slow, high quality PRNG,
           which in turn is used when thread start to generate a single high quality seed
           for the fast, low quality PRNG used for the CAS exponential backoff
  */

#if( LFDS700_PAL_ALIGN_SINGLE_POINTER == 4 ) // TRD : any 32-bit platform
  // TRD : PRNG is a 32-bit xorshift, numbers suggested by George Marsaglia, in his paper http://www.jstatsoft.org/v08/i14/paper
  #define LFDS700_MISC_PRNG_GENERATE( pointer_to_lfds700_misc_prng_state )  ( (pointer_to_lfds700_misc_prng_state)->prng_state ^= (pointer_to_lfds700_misc_prng_state)->prng_state >> 13, (pointer_to_lfds700_misc_prng_state)->prng_state ^= (pointer_to_lfds700_misc_prng_state)->prng_state << 17, (pointer_to_lfds700_misc_prng_state)->prng_state ^= (pointer_to_lfds700_misc_prng_state)->prng_state >> 5 )
#endif

#if( LFDS700_PAL_ALIGN_SINGLE_POINTER == 8 ) // TRD : any 64-bit platform
  // TRD : PRNG is 64-bit xorshift (xorshift64*), from Sebastiano Vigna (vigna at acm dot org), http://creativecommons.org/publicdomain/zero/1.0/
  #define LFDS700_MISC_PRNG_GENERATE( pointer_to_lfds700_misc_prng_state )  ( (pointer_to_lfds700_misc_prng_state)->prng_state ^= (pointer_to_lfds700_misc_prng_state)->prng_state >> 12, (pointer_to_lfds700_misc_prng_state)->prng_state ^= (pointer_to_lfds700_misc_prng_state)->prng_state << 25, (pointer_to_lfds700_misc_prng_state)->prng_state ^= (pointer_to_lfds700_misc_prng_state)->prng_state >> 27, (pointer_to_lfds700_misc_prng_state)->prng_state *= 2685821657736338717LL )
#endif

#if( !defined LFDS700_PAL_ATOMIC_CAS )
  #define LFDS700_PAL_NO_ATOMIC_CAS

  // TRD : lfds700_pal_atom_t volatile *destination, lfds700_pal_atom_t *compare, lfds700_pal_atom_t new_destination, enum lfds700_misc_cas_strength cas_strength, char unsigned result

  #define LFDS700_PAL_ATOMIC_CAS( pointer_to_destination, pointer_to_compare, new_destination, cas_strength, result )  \
  {                                                                                                                    \
    LFDS700_PAL_ASSERT( !"LFDS700_PAL_ATOMIC_CAS not implemented for this platform." );                                \
    LFDS700_MISC_DELIBERATELY_CRASH;                                                                                   \
    (result) = (char unsigned) 1;                                                                                      \
  }
#endif

#if( !defined LFDS700_PAL_ATOMIC_DWCAS )
  #define LFDS700_PAL_NO_ATOMIC_DWCAS

  // TRD : lfds700_pal_atom_t volatile (*destination)[2], lfds700_pal_atom_t (*compare)[2], lfds700_pal_atom_t (*new_destination)[2], enum lfds700_misc_cas_strength cas_strength, unsigned char result

  #define LFDS700_PAL_ATOMIC_DWCAS( pointer_to_destination, pointer_to_compare, pointer_to_new_destination, cas_strength, result )  \
  {                                                                                                                                 \
    LFDS700_PAL_ASSERT( !"LFDS700_PAL_ATOMIC_DWCAS not implemented for this platform." );                                           \
    LFDS700_MISC_DELIBERATELY_CRASH;                                                                                                \
    (result) = (char unsigned) 1;                                                                                                   \
  }
#endif

#if( !defined LFDS700_PAL_ATOMIC_EXCHANGE )
  #define LFDS700_PAL_NO_ATOMIC_EXCHANGE
  // TRD : lfds700_pal_atom_t volatile *destination, lfds700_pal_atom_t *exchange
  #define LFDS700_PAL_ATOMIC_EXCHANGE( pointer_to_destination, pointer_to_exchange )          \
  {                                                                                           \
    LFDS700_PAL_ASSERT( !"LFDS700_PAL_ATOMIC_EXCHANGE not implemented for this platform." );  \
    LFDS700_MISC_DELIBERATELY_CRASH;                                                          \
  }
#endif

#if( defined LFDS700_PAL_NO_COMPILER_BARRIERS )
  #define LFDS700_MISC_BARRIER_LOAD   ( LFDS700_PAL_BARRIER_PROCESSOR_LOAD  )
  #define LFDS700_MISC_BARRIER_STORE  ( LFDS700_PAL_BARRIER_PROCESSOR_STORE )
  #define LFDS700_MISC_BARRIER_FULL   ( LFDS700_PAL_BARRIER_PROCESSOR_FULL  )
#else
  #define LFDS700_MISC_BARRIER_LOAD   ( LFDS700_PAL_BARRIER_COMPILER_LOAD,  LFDS700_PAL_BARRIER_PROCESSOR_LOAD,  LFDS700_PAL_BARRIER_COMPILER_LOAD  )
  #define LFDS700_MISC_BARRIER_STORE  ( LFDS700_PAL_BARRIER_COMPILER_STORE, LFDS700_PAL_BARRIER_PROCESSOR_STORE, LFDS700_PAL_BARRIER_COMPILER_STORE )
  #define LFDS700_MISC_BARRIER_FULL   ( LFDS700_PAL_BARRIER_COMPILER_FULL,  LFDS700_PAL_BARRIER_PROCESSOR_FULL,  LFDS700_PAL_BARRIER_COMPILER_FULL  )
#endif

#define LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE  LFDS700_MISC_BARRIER_LOAD

#if( defined LFDS700_PAL_NO_ATOMIC_CAS )
  #define LFDS700_MISC_ATOMIC_SUPPORT_CAS 0
#else
  #define LFDS700_MISC_ATOMIC_SUPPORT_CAS 1
#endif

#if( defined LFDS700_PAL_NO_ATOMIC_DWCAS )
  #define LFDS700_MISC_ATOMIC_SUPPORT_DWCAS 0
#else
  #define LFDS700_MISC_ATOMIC_SUPPORT_DWCAS 1
#endif

#if( defined LFDS700_PAL_NO_ATOMIC_EXCHANGE )
  #define LFDS700_MISC_ATOMIC_SUPPORT_EXCHANGE 0
#else
  #define LFDS700_MISC_ATOMIC_SUPPORT_EXCHANGE 1
#endif

/***** enums *****/
enum lfds700_misc_cas_strength
{
  // TRD : yes, weak is 1 (one) - blame GCC!
  LFDS700_MISC_CAS_STRENGTH_WEAK   = 1,
  LFDS700_MISC_CAS_STRENGTH_STRONG = 0
};

enum lfds700_misc_validity
{
  LFDS700_MISC_VALIDITY_VALID,
  LFDS700_MISC_VALIDITY_INVALID_LOOP,
  LFDS700_MISC_VALIDITY_INVALID_MISSING_ELEMENTS,
  LFDS700_MISC_VALIDITY_INVALID_ADDITIONAL_ELEMENTS,
  LFDS700_MISC_VALIDITY_INVALID_TEST_DATA,
  LFDS700_MISC_VALIDITY_INVALID_ORDER
};

enum lfds700_misc_flag
{
  LFDS700_MISC_FLAG_LOWERED,
  LFDS700_MISC_FLAG_RAISED
};

enum lfds700_misc_query
{
  LFDS700_MISC_QUERY_GET_EXPONENTIAL_BACKOFF_TIMESLOT_LENGTH_IN_LOOP_ITERATIONS_FOR_CAS,
  LFDS700_MISC_QUERY_SET_EXPONENTIAL_BACKOFF_TIMESLOT_LENGTH_IN_LOOP_ITERATIONS_FOR_CAS,
  LFDS700_MISC_QUERY_GET_EXPONENTIAL_BACKOFF_TIMESLOT_LENGTH_IN_LOOP_ITERATIONS_FOR_DWCAS,
  LFDS700_MISC_QUERY_SET_EXPONENTIAL_BACKOFF_TIMESLOT_LENGTH_IN_LOOP_ITERATIONS_FOR_DWCAS,
  LFDS700_MISC_QUERY_GET_BUILD_AND_VERSION_STRING
};

/***** struct *****/
struct lfds700_misc_globals
{
  lfds700_pal_atom_t
    exponential_backoff_timeslot_length_in_loop_iterations_for_cas,
    exponential_backoff_timeslot_length_in_loop_iterations_for_dwcas;
};

struct lfds700_misc_prng_state
{
  lfds700_pal_uint_t
    prng_state;

  // TRD : here to be on the same cache-line as prng_state, and so all are obtained from one cache-line read
  lfds700_pal_atom_t
    local_copy_of_global_exponential_backoff_timeslot_length_in_loop_iterations_for_cas,
    local_copy_of_global_exponential_backoff_timeslot_length_in_loop_iterations_for_dwcas;
};

struct lfds700_misc_validation_info
{
  lfds700_pal_uint_t
    min_elements,
    max_elements;
};

/***** externs *****/
extern struct lfds700_misc_globals
  lfds700_misc_globals;

/***** public prototypes *****/
void lfds700_misc_library_init_valid_on_current_logical_core( void );
  // TRD : used in conjunction with the #define LFDS700_MISC_MAKE_VALID_ON_CURRENT_LOGICAL_CORE_INITS_COMPLETED_BEFORE_NOW_ON_ANY_OTHER_LOGICAL_CORE
void lfds700_misc_library_cleanup( void );

static LFDS700_PAL_INLINE void lfds700_misc_force_store( void );

void lfds700_misc_prng_init( struct lfds700_misc_prng_state *ps );

void lfds700_misc_query( enum lfds700_misc_query query_type, void *query_input, void *query_output );

/***** public in-line functions *****/
// #pragma prefast( disable : 28112, "blah" )

static LFDS700_PAL_INLINE void lfds700_misc_force_store()
{
  lfds700_pal_uint_t
    exchange = 0;

  lfds700_pal_atom_t volatile LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    destination;

  LFDS700_PAL_ATOMIC_EXCHANGE( &destination, &exchange );

  return;
}

