#include "lfds611_abstraction_internal_body.h"





/****************************************************************************/
#if (defined _WIN64 && defined _MSC_VER)

/* TRD : 64 bit Windows (user-mode or kernel) on any CPU with the Microsoft C compiler

         _WIN64    indicates 64 bit Windows
         _MSC_VER  indicates Microsoft C compiler
*/

static LFDS611_INLINE unsigned char lfds611_abstraction_dcas( volatile lfds611_atom_t *destination, lfds611_atom_t *exchange, lfds611_atom_t *compare )
{
  unsigned char
  cas_result;

  assert( destination != NULL );
  assert( exchange != NULL );
  assert( compare != NULL );

  LFDS611_BARRIER_COMPILER_FULL;

  cas_result = _InterlockedCompareExchange128( (volatile __int64 *) destination, (__int64) *(exchange+1), (__int64) *exchange, (__int64 *) compare );

  LFDS611_BARRIER_COMPILER_FULL;

  return( cas_result ) ;
}

#endif





/****************************************************************************/
#if (!defined _WIN64 && defined _WIN32 && defined _MSC_VER)

/* TRD : 32 bit Windows (user-mode or kernel) on any CPU with the Microsoft C compiler

         (!defined _WIN64 && defined _WIN32)  indicates 32 bit Windows
         _MSC_VER                             indicates Microsoft C compiler
*/

static LFDS611_INLINE unsigned char lfds611_abstraction_dcas( volatile lfds611_atom_t *destination, lfds611_atom_t *exchange, lfds611_atom_t *compare )
{
  __int64
  original_compare;

  assert( destination != NULL );
  assert( exchange != NULL );
  assert( compare != NULL );

  *(__int64 *) &original_compare = *(__int64 *) compare;

  LFDS611_BARRIER_COMPILER_FULL;

  *(__int64 *) compare = _InterlockedCompareExchange64( (volatile __int64 *) destination, *(__int64 *) exchange, *(__int64 *) compare );

  LFDS611_BARRIER_COMPILER_FULL;

  return( (unsigned char) (*(__int64 *) compare == *(__int64 *) &original_compare) );
}

#endif





/****************************************************************************/
#if (defined __x86_64__ && defined __GNUC__)

/* TRD : any OS on x64 with GCC

         __x86_64__  indicates x64
         __GNUC__    indicates GCC
*/

static LFDS611_INLINE unsigned char lfds611_abstraction_dcas( volatile lfds611_atom_t *destination, lfds611_atom_t *exchange, lfds611_atom_t *compare )
{
  unsigned char
  cas_result;

  assert( destination != NULL );
  assert( exchange != NULL );
  assert( compare != NULL );

  // TRD : __asm__ with "memory" in the clobber list is for GCC a full compiler barrier
  __asm__ __volatile__
  (
    "lock;"           // make cmpxchg16b atomic
    "cmpxchg16b %0;"  // cmpxchg16b sets ZF on success
    "setz       %3;"  // if ZF set, set cas_result to 1

    // output
    : "+m" (*(volatile lfds611_atom_t (*)[2]) destination), "+a" (*compare), "+d" (*(compare+1)), "=q" (cas_result)

    // input
    : "b" (*exchange), "c" (*(exchange+1))

    // clobbered
    : "cc", "memory"
  );

  return( cas_result );
}

#endif





/****************************************************************************/
#if ((defined __i686__ || defined __arm__) && __GNUC__ >= 4 && __GNUC_MINOR__ >= 1 && __GNUC_PATCHLEVEL__ >= 0)

/* TRD : any OS on x86 or ARM with GCC 4.1.0 or better

         GCC 4.1.0 introduced the __sync_*() atomic intrinsics

         __GNUC__ / __GNUC_MINOR__ / __GNUC_PATCHLEVEL__  indicates GCC and which version
*/

static LFDS611_INLINE unsigned char lfds611_abstraction_dcas( volatile lfds611_atom_t *destination, lfds611_atom_t *exchange, lfds611_atom_t *compare )
{
  unsigned char
  cas_result = 0;

  unsigned long long int
  original_destination;

  assert( destination != NULL );
  assert( exchange != NULL );
  assert( compare != NULL );

  LFDS611_BARRIER_COMPILER_FULL;

  original_destination = __sync_val_compare_and_swap( (volatile unsigned long long int *) destination, *(unsigned long long int *) compare, *(unsigned long long int *) exchange );

  LFDS611_BARRIER_COMPILER_FULL;

  if( original_destination == *(unsigned long long int *) compare )
    cas_result = 1;

  *(unsigned long long int *) compare = original_destination;

  return( cas_result );
}

#endif


