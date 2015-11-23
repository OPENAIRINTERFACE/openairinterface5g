#include "lfds611_abstraction_internal_body.h"





/****************************************************************************/
#if (defined _WIN32 && defined _MSC_VER)

/* TRD : 64 bit and 32 bit Windows (user-mode or kernel) on any CPU with the Microsoft C compiler

         _WIN32    indicates 64-bit or 32-bit Windows
         _MSC_VER  indicates Microsoft C compiler
*/

static LFDS611_INLINE lfds611_atom_t lfds611_abstraction_cas( volatile lfds611_atom_t *destination, lfds611_atom_t exchange, lfds611_atom_t compare )
{
  lfds611_atom_t
  rv;

  assert( destination != NULL );
  // TRD : exchange can be any value in its range
  // TRD : compare can be any value in its range

  LFDS611_BARRIER_COMPILER_FULL;

  rv = (lfds611_atom_t) _InterlockedCompareExchangePointer( (void * volatile *) destination, (void *) exchange, (void *) compare );

  LFDS611_BARRIER_COMPILER_FULL;

  return( rv );
}

#endif





/****************************************************************************/
#if (__GNUC__ >= 4 && __GNUC_MINOR__ >= 1 && __GNUC_PATCHLEVEL__ >= 0)

/* TRD : any OS on any CPU with GCC 4.1.0 or better

         GCC 4.1.0 introduced the __sync_*() atomic intrinsics

         __GNUC__ / __GNUC_MINOR__ / __GNUC_PATCHLEVEL__  indicates GCC and which version
*/

static LFDS611_INLINE lfds611_atom_t lfds611_abstraction_cas( volatile lfds611_atom_t *destination, lfds611_atom_t exchange, lfds611_atom_t compare )
{
  lfds611_atom_t
  rv;

  assert( destination != NULL );
  // TRD : exchange can be any value in its range
  // TRD : compare can be any value in its range

  // TRD : note the different argument order for the GCC instrinsic to the MSVC instrinsic

  LFDS611_BARRIER_COMPILER_FULL;

  rv = (lfds611_atom_t) __sync_val_compare_and_swap( destination, compare, exchange );

  LFDS611_BARRIER_COMPILER_FULL;

  return( rv );
}

#endif

