/***** public prototypes *****/
#include "../inc/liblfds700.h"

/***** defines *****/
#define and &&
#define or  ||

#define NO_FLAGS 0x0

#define LFDS700_ABSTRACTION_BACKOFF_LIMIT  (0x1 << 10)

#if( defined _KERNEL_MODE )
  #define MODE_TYPE_STRING "kernel-mode"
#endif

#if( !defined _KERNEL_MODE )
  #define MODE_TYPE_STRING "user-mode"
#endif

#if( defined NDEBUG && !defined COVERAGE && !defined TSAN )
  #define BUILD_TYPE_STRING "release"
#endif

#if( !defined NDEBUG && !defined COVERAGE && !defined TSAN )
  #define BUILD_TYPE_STRING "debug"
#endif

#if( !defined NDEBUG && defined COVERAGE && !defined TSAN )
  #define BUILD_TYPE_STRING "coverage"
#endif

#if( !defined NDEBUG && !defined COVERAGE && defined TSAN )
  #define BUILD_TYPE_STRING "threadsanitizer"
#endif

// TRD : lfds700_pal_atom_t volatile *destination, lfds700_pal_atom_t *compare, lfds700_pal_atom_t new_destination, enum lfds700_misc_cas_strength cas_strength, char unsigned result, lfds700_pal_uint_t *backoff_iteration
#define LFDS700_PAL_ATOMIC_CAS_WITH_BACKOFF( pointer_to_destination, pointer_to_compare, new_destination, cas_strength, result, backoff_iteration, ps )              \
{                                                                                                                                                                    \
  LFDS700_PAL_ATOMIC_CAS( pointer_to_destination, pointer_to_compare, new_destination, cas_strength, result );                                                       \
                                                                                                                                                                     \
  if( result == 0 )                                                                                                                                                  \
  {                                                                                                                                                                  \
    lfds700_pal_uint_t                                                                                                                                               \
      endloop;                                                                                                                                                       \
                                                                                                                                                                     \
    lfds700_pal_uint_t volatile                                                                                                                                      \
      loop;                                                                                                                                                          \
                                                                                                                                                                     \
    if( (backoff_iteration) == LFDS700_ABSTRACTION_BACKOFF_LIMIT )                                                                                                   \
      (backoff_iteration) = LFDS700_MISC_ABSTRACTION_BACKOFF_INITIAL_VALUE;                                                                                          \
                                                                                                                                                                     \
    if( (backoff_iteration) == LFDS700_MISC_ABSTRACTION_BACKOFF_INITIAL_VALUE )                                                                                      \
      (backoff_iteration) = 1;                                                                                                                                       \
    else                                                                                                                                                             \
    {                                                                                                                                                                \
      endloop = ( LFDS700_MISC_PRNG_GENERATE(ps) % (backoff_iteration) ) * ps->local_copy_of_global_exponential_backoff_timeslot_length_in_loop_iterations_for_cas;  \
      for( loop = 0 ; loop < endloop ; loop++ );                                                                                                                     \
    }                                                                                                                                                                \
                                                                                                                                                                     \
    (backoff_iteration) <<= 1;                                                                                                                                       \
  }                                                                                                                                                                  \
}

// TRD : lfds700_pal_atom_t volatile (*destination)[2], lfds700_pal_atom_t (*compare)[2], lfds700_pal_atom_t (*new_destination)[2], enum lfds700_misc_cas_strength cas_strength, char unsigned result, lfds700_pal_uint_t *backoff_iteration
#define LFDS700_PAL_ATOMIC_DWCAS_WITH_BACKOFF( pointer_to_destination, pointer_to_compare, pointer_to_new_destination, cas_strength, result, backoff_iteration, ps )   \
{                                                                                                                                                                      \
  LFDS700_PAL_ATOMIC_DWCAS( pointer_to_destination, pointer_to_compare, pointer_to_new_destination, cas_strength, result );                                            \
                                                                                                                                                                       \
  if( result == 0 )                                                                                                                                                    \
  {                                                                                                                                                                    \
    lfds700_pal_uint_t                                                                                                                                                 \
      endloop;                                                                                                                                                         \
                                                                                                                                                                       \
    lfds700_pal_uint_t volatile                                                                                                                                        \
      loop;                                                                                                                                                            \
                                                                                                                                                                       \
    if( (backoff_iteration) == LFDS700_ABSTRACTION_BACKOFF_LIMIT )                                                                                                     \
      (backoff_iteration) = LFDS700_MISC_ABSTRACTION_BACKOFF_INITIAL_VALUE;                                                                                            \
                                                                                                                                                                       \
    if( (backoff_iteration) == LFDS700_MISC_ABSTRACTION_BACKOFF_INITIAL_VALUE )                                                                                        \
      (backoff_iteration) = 1;                                                                                                                                         \
    else                                                                                                                                                               \
    {                                                                                                                                                                  \
      endloop = ( LFDS700_MISC_PRNG_GENERATE(ps) % (backoff_iteration) ) * ps->local_copy_of_global_exponential_backoff_timeslot_length_in_loop_iterations_for_dwcas;  \
      for( loop = 0 ; loop < endloop ; loop++ );                                                                                                                       \
    }                                                                                                                                                                  \
                                                                                                                                                                       \
    (backoff_iteration) <<= 1;                                                                                                                                         \
  }                                                                                                                                                                    \
}

/***** library-wide prototypes *****/

