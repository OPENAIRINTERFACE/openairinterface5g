/****************************************************************************/
#if( defined __GNUC__ )
  // TRD : makes checking GCC versions much tidier
  #define LFDS700_PAL_GCC_VERSION ( __GNUC__ * 100 + __GNUC_MINOR__ * 10 + __GNUC_PATCHLEVEL__ )
#endif





/****************************************************************************/
#if( defined _MSC_VER && _MSC_VER >= 1400 )

  /* TRD : MSVC 8.0 and greater

           _MSC_VER  indicates Microsoft C compiler and version
                       - __declspec(align)                  requires 7.1 (1310)
                       - __nop                              requires 8.0 (1400)
                       - _ReadBarrier                       requires 8.0 (1400)
                       - _WriteBarrier                      requires 8.0 (1400)
                       - _ReadWriteBarrier                  requires 7.1 (1310)
                       - _InterlockedCompareExchangePointer requires 8.0 (1400)
                       - _InterlockedExchange               requires 7.1 (1310)
                       - _InterlockedExchangePointer        requires 8.0 (1400)
                       - _InterlockedCompareExchange64      requires 8.0 (1400) (seems to, docs unclear)
                       - _InterlockedCompareExchange128     requires 9.0 (1500)

           load/store barriers are mandatory for liblfds, which means the earliest viable version of MSCV is 1400
           strictly we could get away with 1310 and use _ReadWriteBarrier, but the difference between 1310 and 1400 is small, so WTH

           _InterlockedCompareExchange128 is needed on 64-bit platforms to provide DWCAS, but DWCAS is not mandatory,
           so we check against the compiler version - remember, any unimplemented atomic will be masked by its dummy define,
           so everything will compile -  it just means you can't use data structures which require that atomic
  */

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_COMPILER
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_compiler.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_COMPILER

  #define LFDS700_PAL_COMPILER_STRING            "MSVC"

  #define LFDS700_PAL_ALIGN(alignment)           __declspec( align(alignment) )
  #define LFDS700_PAL_INLINE                     __forceinline

  #define LFDS700_PAL_BARRIER_COMPILER_LOAD      _ReadBarrier()
  #define LFDS700_PAL_BARRIER_COMPILER_STORE     _WriteBarrier()
  #define LFDS700_PAL_BARRIER_COMPILER_FULL      _ReadWriteBarrier()

  /* TRD : there are four processors to consider;

           . ARM32    (32 bit, CAS, DWCAS) (defined _M_ARM)
           . Itanium  (64 bit, CAS)        (defined _M_IA64)
           . x64      (64 bit, CAS, DWCAS) (defined _M_X64 || defined _M_AMD64)
           . x86      (32 bit, CAS, DWCAS) (defined _M_IX86)

           can't find any indications of 64-bit ARM support yet

           ARM has better intrinsics than the others, as there are no-fence variants

           in theory we also have to deal with 32-bit Windows on a 64-bit platform,
           and I presume we'd see the compiler properly indicate this in its macros,
           but this would require that we use 32-bit atomics on the 64-bit platforms,
           while keeping 64-bit cache line lengths and so on, and this is just so
           wierd a thing to do these days that it's not supported

           note that _InterlockedCompareExchangePointer performs CAS on all processors
           however, it is documented as being available for x86 when in fact it is not
           so we have to #if for processor type and use the length specific intrinsics
  */

  #if( defined _M_ARM )
    #define LFDS700_PAL_BARRIER_PROCESSOR_LOAD   __dmb( _ARM_BARRIER_ISH )
    #define LFDS700_PAL_BARRIER_PROCESSOR_STORE  __dmb( _ARM_BARRIER_ISHST )
    #define LFDS700_PAL_BARRIER_PROCESSOR_FULL   __dmb( _ARM_BARRIER_ISH )

    #define LFDS700_PAL_ATOMIC_CAS( pointer_to_destination, pointer_to_compare, new_destination, cas_strength, result )                                                                                          \
    {                                                                                                                                                                                                            \
      lfds700_pal_atom_t                                                                                                                                                                                         \
        original_compare;                                                                                                                                                                                        \
                                                                                                                                                                                                                 \
      /* LFDS700_PAL_ASSERT( (pointer_to_destination) != NULL ); */                                                                                                                                              \
      /* LFDS700_PAL_ASSERT( (pointer_to_compare) != NULL ); */                                                                                                                                                  \
      /* TRD : new_destination can be any value in its range */                                                                                                                                                  \
      /* TRD : cas_strength can be any value in its range */                                                                                                                                                     \
      /* TRD : result can be any value in its range */                                                                                                                                                           \
                                                                                                                                                                                                                 \
      original_compare = (lfds700_pal_atom_t) *(pointer_to_compare);                                                                                                                                             \
                                                                                                                                                                                                                 \
      LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                                                         \
      *(lfds700_pal_atom_t *) (pointer_to_compare) = (lfds700_pal_atom_t) _InterlockedCompareExchange_nf( (long volatile *) (pointer_to_destination), (long) (new_destination), (long) *(pointer_to_compare) );  \
      LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                                                         \
                                                                                                                                                                                                                 \
      result = (char unsigned) ( original_compare == (lfds700_pal_atom_t) *(pointer_to_compare) );                                                                                                               \
    }

    #define LFDS700_PAL_ATOMIC_DWCAS( pointer_to_destination, pointer_to_compare, pointer_to_new_destination, cas_strength, result )                                                                        \
    {                                                                                                                                                                                                       \
      __int64                                                                                                                                                                                               \
        original_compare;                                                                                                                                                                                   \
                                                                                                                                                                                                            \
      /* LFDS700_PAL_ASSERT( (pointer_to_destination) != NULL ); */                                                                                                                                         \
      /* LFDS700_PAL_ASSERT( (pointer_to_compare) != NULL ); */                                                                                                                                             \
      /* LFDS700_PAL_ASSERT( (pointer_to_new_destination) != NULL ); */                                                                                                                                     \
      /* TRD : cas_strength can be any value in its range */                                                                                                                                                \
      /* TRD : result can be any value in its range */                                                                                                                                                      \
                                                                                                                                                                                                            \
      original_compare = *(__int64 *) (pointer_to_compare);                                                                                                                                                 \
                                                                                                                                                                                                            \
      LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                                                    \
      *(__int64 *) (pointer_to_compare) = _InterlockedCompareExchange64_nf( (__int64 volatile *) (pointer_to_destination), *(__int64 *) (pointer_to_new_destination), *(__int64 *) (pointer_to_compare) );  \
      LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                                                    \
                                                                                                                                                                                                            \
      result = (char unsigned) ( *(__int64 *) (pointer_to_compare) == original_compare );                                                                                                                   \
    }

    #define LFDS700_PAL_ATOMIC_EXCHANGE( pointer_to_destination, pointer_to_exchange )                                                                                                    \
    {                                                                                                                                                                                     \
      /* LFDS700_PAL_ASSERT( (pointer_to_destination) != NULL ); */                                                                                                                       \
      /* LFDS700_PAL_ASSERT( (pointer_to_exchange) != NULL ); */                                                                                                                          \
                                                                                                                                                                                          \
      LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                                  \
      *(lfds700_pal_atom_t *) (pointer_to_exchange) = (lfds700_pal_atom_t) _InterlockedExchange_nf( (int long volatile *) (pointer_to_destination), (int long) *(pointer_to_exchange) );  \
      LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                                  \
    }
  #endif

  #if( defined _M_IA64 )
    #define LFDS700_PAL_BARRIER_PROCESSOR_LOAD   __mf()
    #define LFDS700_PAL_BARRIER_PROCESSOR_STORE  __mf()
    #define LFDS700_PAL_BARRIER_PROCESSOR_FULL   __mf()

    #define LFDS700_PAL_ATOMIC_CAS( pointer_to_destination, pointer_to_compare, new_destination, cas_strength, result )                                                                                         \
    {                                                                                                                                                                                                           \
      lfds700_pal_atom_t                                                                                                                                                                                        \
        original_compare;                                                                                                                                                                                       \
                                                                                                                                                                                                                \
      /* LFDS700_PAL_ASSERT( (pointer_to_destination) != NULL ); */                                                                                                                                             \
      /* LFDS700_PAL_ASSERT( (pointer_to_compare) != NULL ); */                                                                                                                                                 \
      /* TRD : new_destination can be any value in its range */                                                                                                                                                 \
      /* TRD : cas_strength can be any value in its range */                                                                                                                                                    \
      /* TRD : result can be any value in its range */                                                                                                                                                          \
                                                                                                                                                                                                                \
      original_compare = (lfds700_pal_atom_t) *(pointer_to_compare);                                                                                                                                            \
                                                                                                                                                                                                                \
      LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                                                        \
      *(lfds700_pal_atom_t *) (pointer_to_compare) = (lfds700_pal_atom_t) _InterlockedCompareExchange64_acq( (__int64 volatile *) (pointer_to_destination), (__int64) (new_destination), (__int64) *(pointer_to_compare) );  \
      LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                                                        \
                                                                                                                                                                                                                \
      result = (char unsigned) ( original_compare == (lfds700_pal_atom_t) *(pointer_to_compare) );                                                                                                              \
    }

    #define LFDS700_PAL_ATOMIC_EXCHANGE( pointer_to_destination, pointer_to_exchange )                                                                                                     \
    {                                                                                                                                                                                      \
      /* LFDS700_PAL_ASSERT( (pointer_to_destination) != NULL ); */                                                                                                                        \
      /* LFDS700_PAL_ASSERT( (pointer_to_exchange) != NULL ); */                                                                                                                           \
                                                                                                                                                                                           \
      LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                                   \
      *(lfds700_pal_atom_t *) (pointer_to_exchange) = (lfds700_pal_atom_t) _InterlockedExchange64_acq( (__int64 volatile *) (pointer_to_destination), (__int64) *(pointer_to_exchange) );  \
      LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                                   \
    }
  #endif

  #if( defined _M_X64 || defined _M_AMD64 )
    #define LFDS700_PAL_BARRIER_PROCESSOR_LOAD   _mm_lfence()
    #define LFDS700_PAL_BARRIER_PROCESSOR_STORE  _mm_sfence()
    #define LFDS700_PAL_BARRIER_PROCESSOR_FULL   _mm_mfence()

    #define LFDS700_PAL_ATOMIC_CAS( pointer_to_destination, pointer_to_compare, new_destination, cas_strength, result )                                                                                         \
    {                                                                                                                                                                                                           \
      lfds700_pal_atom_t                                                                                                                                                                                        \
        original_compare;                                                                                                                                                                                       \
                                                                                                                                                                                                                \
      /* LFDS700_PAL_ASSERT( (pointer_to_destination) != NULL ); */                                                                                                                                             \
      /* LFDS700_PAL_ASSERT( (pointer_to_compare) != NULL ); */                                                                                                                                                 \
      /* TRD : new_destination can be any value in its range */                                                                                                                                                 \
      /* TRD : cas_strength can be any value in its range */                                                                                                                                                    \
      /* TRD : result can be any value in its range */                                                                                                                                                          \
                                                                                                                                                                                                                \
      original_compare = (lfds700_pal_atom_t) *(pointer_to_compare);                                                                                                                                            \
                                                                                                                                                                                                                \
      LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                                                        \
      *(lfds700_pal_atom_t *) (pointer_to_compare) = (lfds700_pal_atom_t) _InterlockedCompareExchange64( (__int64 volatile *) (pointer_to_destination), (__int64) (new_destination), (__int64) *(pointer_to_compare) );  \
      LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                                                        \
                                                                                                                                                                                                                \
      result = (char unsigned) ( original_compare == (lfds700_pal_atom_t) *(pointer_to_compare) );                                                                                                              \
    }

    #if( _MSC_VER >= 1500 )
      #define LFDS700_PAL_ATOMIC_DWCAS( pointer_to_destination, pointer_to_compare, pointer_to_new_destination, cas_strength, result )                                                                                                     \
      {                                                                                                                                                                                                                                    \
        /* LFDS700_PAL_ASSERT( (pointer_to_new_destination) != NULL ); */                                                                                                                                                                  \
        /* LFDS700_PAL_ASSERT( (pointer_to_destination) != NULL ); */                                                                                                                                                                      \
        /* LFDS700_PAL_ASSERT( (pointer_to_compare) != NULL ); */                                                                                                                                                                          \
        /* TRD : cas_strength can be any value in its range */                                                                                                                                                                             \
        /* TRD : result can be any value in its range */                                                                                                                                                                                   \
                                                                                                                                                                                                                                           \
        LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                                                                                 \
        result = (char unsigned) _InterlockedCompareExchange128( (__int64 volatile *) (pointer_to_destination), (__int64) (pointer_to_new_destination[1]), (__int64) (pointer_to_new_destination[0]), (__int64 *) (pointer_to_compare) );  \
        LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                                                                                 \
      }
    #endif

    #define LFDS700_PAL_ATOMIC_EXCHANGE( pointer_to_destination, pointer_to_exchange )                                                                                                    \
    {                                                                                                                                                                                     \
      /* LFDS700_PAL_ASSERT( (pointer_to_destination) != NULL ); */                                                                                                                       \
      /* LFDS700_PAL_ASSERT( (pointer_to_exchange) != NULL ); */                                                                                                                          \
                                                                                                                                                                                          \
      LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                                  \
      *(lfds700_pal_atom_t *) (pointer_to_exchange) = (lfds700_pal_atom_t) _InterlockedExchangePointer( (void * volatile *) (pointer_to_destination), (void *) *(pointer_to_exchange) );  \
      LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                                  \
    }
  #endif

  #if( defined _M_IX86 )
    #define LFDS700_PAL_BARRIER_PROCESSOR_LOAD   lfds700_misc_force_store()
    #define LFDS700_PAL_BARRIER_PROCESSOR_STORE  lfds700_misc_force_store()
    #define LFDS700_PAL_BARRIER_PROCESSOR_FULL   lfds700_misc_force_store()

    #define LFDS700_PAL_ATOMIC_CAS( pointer_to_destination, pointer_to_compare, new_destination, cas_strength, result )                                                                                       \
    {                                                                                                                                                                                                         \
      lfds700_pal_atom_t                                                                                                                                                                                      \
        original_compare;                                                                                                                                                                                     \
                                                                                                                                                                                                              \
      /* LFDS700_PAL_ASSERT( (pointer_to_destination) != NULL ); */                                                                                                                                           \
      /* LFDS700_PAL_ASSERT( (pointer_to_compare) != NULL ); */                                                                                                                                               \
      /* TRD : new_destination can be any value in its range */                                                                                                                                               \
      /* TRD : cas_strength can be any value in its range */                                                                                                                                                  \
      /* TRD : result can be any value in its range */                                                                                                                                                        \
                                                                                                                                                                                                              \
      original_compare = (lfds700_pal_atom_t) *(pointer_to_compare);                                                                                                                                          \
                                                                                                                                                                                                              \
      LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                                                      \
      *(lfds700_pal_atom_t *) (pointer_to_compare) = (lfds700_pal_atom_t) _InterlockedCompareExchange( (long volatile *) (pointer_to_destination), (long) (new_destination), (long) *(pointer_to_compare) );  \
      LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                                                      \
                                                                                                                                                                                                              \
      result = (char unsigned) ( original_compare == (lfds700_pal_atom_t) *(pointer_to_compare) );                                                                                                            \
    }

    #define LFDS700_PAL_ATOMIC_DWCAS( pointer_to_destination, pointer_to_compare, pointer_to_new_destination, cas_strength, result )                                                                     \
    {                                                                                                                                                                                                    \
      __int64                                                                                                                                                                                            \
        original_compare;                                                                                                                                                                                \
                                                                                                                                                                                                         \
      /* LFDS700_PAL_ASSERT( (pointer_to_destination) != NULL ); */                                                                                                                                      \
      /* LFDS700_PAL_ASSERT( (pointer_to_compare) != NULL ); */                                                                                                                                          \
      /* LFDS700_PAL_ASSERT( (pointer_to_new_destination) != NULL ); */                                                                                                                                  \
      /* TRD : cas_strength can be any value in its range */                                                                                                                                             \
      /* TRD : result can be any value in its range */                                                                                                                                                   \
                                                                                                                                                                                                         \
      original_compare = *(__int64 *) (pointer_to_compare);                                                                                                                                              \
                                                                                                                                                                                                         \
      LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                                                 \
      *(__int64 *) (pointer_to_compare) = _InterlockedCompareExchange64( (__int64 volatile *) (pointer_to_destination), *(__int64 *) (pointer_to_new_destination), *(__int64 *) (pointer_to_compare) );  \
      LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                                                 \
                                                                                                                                                                                                         \
      result = (char unsigned) ( *(__int64 *) (pointer_to_compare) == original_compare );                                                                                                                \
    }

    #define LFDS700_PAL_ATOMIC_EXCHANGE( pointer_to_destination, pointer_to_exchange )                                                                                                 \
    {                                                                                                                                                                                  \
      /* LFDS700_PAL_ASSERT( (pointer_to_destination) != NULL ); */                                                                                                                    \
      /* LFDS700_PAL_ASSERT( (pointer_to_exchange) != NULL ); */                                                                                                                       \
                                                                                                                                                                                       \
      LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                               \
      *(lfds700_pal_atom_t *) (pointer_to_exchange) = (lfds700_pal_atom_t) _InterlockedExchange( (int long volatile *) (pointer_to_destination), (int long) *(pointer_to_exchange) );  \
      LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                               \
    }
  #endif

#endif





/****************************************************************************/
#if( defined __GNUC__ && LFDS700_PAL_GCC_VERSION >= 412 && LFDS700_PAL_GCC_VERSION < 473 )

  /* TRD : GCC 4.1.2 up to 4.7.3

           __GNUC__                 indicates GCC
           LFDS700_PAL_GCC_VERSION  indicates which version
                                      - __sync_synchronize requires 4.1.2

           GCC 4.1.2 introduced the __sync_*() atomic intrinsics
  */

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_COMPILER
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_compiler.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_COMPILER

  #define LFDS700_PAL_COMPILER_STRING          "GCC < 4.7.3"

  #define LFDS700_PAL_ALIGN(alignment)         __attribute__( (aligned(alignment)) )
  #define LFDS700_PAL_INLINE                   inline

  static LFDS700_PAL_INLINE void lfds700_pal_barrier_compiler( void )
  {
    __asm__ __volatile__ ( "" : : : "memory" );
  }

  #define LFDS700_PAL_BARRIER_COMPILER_LOAD    lfds700_pal_barrier_compiler()
  #define LFDS700_PAL_BARRIER_COMPILER_STORE   lfds700_pal_barrier_compiler()
  #define LFDS700_PAL_BARRIER_COMPILER_FULL    lfds700_pal_barrier_compiler()

  #define LFDS700_PAL_BARRIER_PROCESSOR_LOAD   __sync_synchronize()
  #define LFDS700_PAL_BARRIER_PROCESSOR_STORE  __sync_synchronize()
  #define LFDS700_PAL_BARRIER_PROCESSOR_FULL   __sync_synchronize()

  #define LFDS700_PAL_ATOMIC_CAS( pointer_to_destination, pointer_to_compare, new_destination, cas_strength, result )       \
  {                                                                                                                         \
    lfds700_pal_atom_t                                                                                                      \
      original_compare;                                                                                                     \
                                                                                                                            \
    /* LFDS700_PAL_ASSERT( (pointer_to_destination) != NULL ); */                                                           \
    /* LFDS700_PAL_ASSERT( (pointer_to_compare) != NULL ); */                                                               \
    /* TRD : new_destination can be any value in its range */                                                               \
    /* TRD : cas_strength can be any value in its range */                                                                  \
    /* TRD : result can be any value in its range */                                                                        \
                                                                                                                            \
    original_compare = (lfds700_pal_atom_t) *(pointer_to_compare);                                                          \
                                                                                                                            \
    LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                      \
    *(pointer_to_compare) = __sync_val_compare_and_swap( pointer_to_destination, *(pointer_to_compare), new_destination );  \
    LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                      \
                                                                                                                            \
    result = (unsigned char) ( original_compare == (lfds700_pal_atom_t) *(pointer_to_compare) );                            \
  }

  // TRD : ARM and x86 have DWCAS which we can get via GCC intrinsics
  #if( defined __arm__ || defined __i686__ || defined __i586__ || defined __i486__ )
    #define LFDS700_PAL_ATOMIC_DWCAS( pointer_to_destination, pointer_to_compare, pointer_to_new_destination, cas_strength, result )                                                                                                   \
    {                                                                                                                                                                                                                                  \
      int long long unsigned                                                                                                                                                                                                           \
        original_destination;                                                                                                                                                                                                          \
                                                                                                                                                                                                                                       \
      /* LFDS700_PAL_ASSERT( (pointer_to_destination) != NULL ); */                                                                                                                                                                    \
      /* LFDS700_PAL_ASSERT( (pointer_to_compare) != NULL ); */                                                                                                                                                                        \
      /* LFDS700_PAL_ASSERT( (pointer_to_new_destination) != NULL ); */                                                                                                                                                                \
      /* TRD : cas_strength can be any value in its range */                                                                                                                                                                           \
      /* TRD : result can be any value in its range */                                                                                                                                                                                 \
                                                                                                                                                                                                                                       \
      LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                                                                               \
      original_destination = __sync_val_compare_and_swap( (int long long unsigned volatile *) (pointer_to_destination), *(int long long unsigned *) (pointer_to_compare), *(int long long unsigned *) (pointer_to_new_destination) );  \
      LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                                                                                               \
                                                                                                                                                                                                                                       \
      result = (char unsigned) ( original_destination == *(int long long unsigned *) (pointer_to_compare) );                                                                                                                           \
                                                                                                                                                                                                                                       \
      *(int long long unsigned *) (pointer_to_compare) = original_destination;                                                                                                                                                         \
    }
  #endif

  #define LFDS700_PAL_ATOMIC_EXCHANGE( pointer_to_destination, pointer_to_exchange )                                                                   \
  {                                                                                                                                                    \
    /* LFDS700_PAL_ASSERT( (pointer_to_destination) != NULL ); */                                                                                      \
    /* LFDS700_PAL_ASSERT( (pointer_to_exchange) != NULL ); */                                                                                         \
                                                                                                                                                       \
    LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                 \
    *( (lfds700_pal_atom_t *) pointer_to_exchange) = (lfds700_pal_atom_t) __sync_lock_test_and_set( pointer_to_destination, *(pointer_to_exchange) );  \
    LFDS700_PAL_BARRIER_COMPILER_FULL;                                                                                                                 \
  }

#endif





/****************************************************************************/
#if( defined __GNUC__ && LFDS700_PAL_GCC_VERSION >= 473 )

  /* TRD : GCC 4.7.3 and greater

           __GNUC__                 indicates GCC
           LFDS700_PAL_GCC_VERSION  indicates which version
                                      - __atomic_thread_fence requires 4.7.3

           GCC 4.7.3 introduced the better __atomic*() atomic intrinsics
  */

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_COMPILER
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_compiler.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_COMPILER

  #define LFDS700_PAL_COMPILER_STRING          "GCC >= 4.7.3"

  #define LFDS700_PAL_ALIGN(alignment)         __attribute__( (aligned(alignment)) )
  #define LFDS700_PAL_INLINE                   inline

  // TRD : GCC >= 4.7.3 compiler barriers are built into the intrinsics
  #define LFDS700_PAL_NO_COMPILER_BARRIERS

  #define LFDS700_PAL_BARRIER_PROCESSOR_LOAD   __atomic_thread_fence( __ATOMIC_ACQUIRE )
  #define LFDS700_PAL_BARRIER_PROCESSOR_STORE  __atomic_thread_fence( __ATOMIC_RELEASE )
  #define LFDS700_PAL_BARRIER_PROCESSOR_FULL   __atomic_thread_fence( __ATOMIC_ACQ_REL )

  #define LFDS700_PAL_ATOMIC_CAS( pointer_to_destination, pointer_to_compare, new_destination, cas_strength, result )                                                                      \
  {                                                                                                                                                                                        \
    /* LFDS700_PAL_ASSERT( (pointer_to_destination) != NULL ); */                                                                                                                          \
    /* LFDS700_PAL_ASSERT( (pointer_to_compare) != NULL ); */                                                                                                                              \
    /* TRD : new_destination can be any value in its range */                                                                                                                              \
    /* TRD : cas_strength can be any value in its range */                                                                                                                                 \
    /* TRD : result can be any value in its range */                                                                                                                                       \
                                                                                                                                                                                           \
    result = (char unsigned) __atomic_compare_exchange_n( pointer_to_destination, (void *) (pointer_to_compare), (new_destination), (cas_strength), __ATOMIC_RELAXED, __ATOMIC_RELAXED );  \
  }

  // TRD : ARM and x86 have DWCAS which we can get via GCC intrinsics
  #if( defined __arm__ || defined __i686__ || defined __i586__ || defined __i486__ )
    #define LFDS700_PAL_ATOMIC_DWCAS( pointer_to_destination, pointer_to_compare, pointer_to_new_destination, cas_strength, result )                                                                                                                                                          \
    {                                                                                                                                                                                                                                                                                         \
      /* LFDS700_PAL_ASSERT( (pointer_to_destination) != NULL ); */                                                                                                                                                                                                                           \
      /* LFDS700_PAL_ASSERT( (pointer_to_compare) != NULL ); */                                                                                                                                                                                                                               \
      /* LFDS700_PAL_ASSERT( (pointer_to_new_destination) != NULL ); */                                                                                                                                                                                                                       \
      /* TRD : cas_strength can be any value in its range */                                                                                                                                                                                                                                  \
      /* TRD : result can be any value in its range */                                                                                                                                                                                                                                        \
                                                                                                                                                                                                                                                                                              \
      (result) = (char unsigned) __atomic_compare_exchange_n( (int long long unsigned volatile *) (pointer_to_destination), (int long long unsigned *) (pointer_to_compare), *(int long long unsigned *) (pointer_to_new_destination), (cas_strength), __ATOMIC_RELAXED, __ATOMIC_RELAXED );  \
    }
  #endif

  #if( defined __x86_64__ )
    /* TRD : __GNUC__    indicates GCC
                           - __asm__ requires GCC
                           - __volatile__ requires GCC
             __x86_64__  indicates x64
                           - cmpxchg16b requires x64

             On 64 bit platforms, unsigned long long int is 64 bit, so we must manually use cmpxchg16b, 
             as __sync_val_compare_and_swap() will only emit cmpxchg8b
    */

    // TRD : lfds700_pal_atom_t volatile (*destination)[2], lfds700_pal_atom_t (*compare)[2], lfds700_pal_atom_t (*new_destination)[2], enum lfds700_misc_cas_strength cas_strength, char unsigned result

    #define LFDS700_PAL_ATOMIC_DWCAS( pointer_to_destination, pointer_to_compare, pointer_to_new_destination, cas_strength, result )  \
    {                                                                                                                                 \
      /* LFDS700_PAL_ASSERT( (pointer_to_destination) != NULL ); */                                                                   \
      /* LFDS700_PAL_ASSERT( (pointer_to_compare) != NULL ); */                                                                       \
      /* LFDS700_PAL_ASSERT( (pointer_to_new_destination) != NULL ); */                                                               \
      /* TRD : cas_strength can be any value in its range */                                                                          \
      /* TRD : result can be any value in its range */                                                                                \
                                                                                                                                      \
      (result) = 0;                                                                                                                   \
                                                                                                                                      \
      __asm__ __volatile__                                                                                                            \
      (                                                                                                                               \
        "lock;"           /* make cmpxchg16b atomic        */                                                                         \
        "cmpxchg16b %0;"  /* cmpxchg16b sets ZF on success */                                                                         \
        "setz       %3;"  /* if ZF set, set result to 1    */                                                                         \
                                                                                                                                      \
        /* output */                                                                                                                  \
        : "+m" (*pointer_to_destination), "+a" ((pointer_to_compare)[0]), "+d" ((pointer_to_compare)[1]), "=q" (result)               \
                                                                                                                                      \
        /* input */                                                                                                                   \
        : "b" ((pointer_to_new_destination)[0]), "c" ((pointer_to_new_destination)[1])                                                \
                                                                                                                                      \
        /* clobbered */                                                                                                               \
        : "cc", "memory"                                                                                                              \
      );                                                                                                                              \
    }
  #endif

  #define LFDS700_PAL_ATOMIC_EXCHANGE( pointer_to_destination, pointer_to_exchange )                                     \
  {                                                                                                                      \
    /* LFDS700_PAL_ASSERT( (pointer_to_destination) != NULL ); */                                                        \
    /* LFDS700_PAL_ASSERT( (pointer_to_exchange) != NULL ); */                                                           \
                                                                                                                         \
    *(pointer_to_exchange) = __atomic_exchange_n( (pointer_to_destination), *(pointer_to_exchange), __ATOMIC_RELAXED );  \
  }

#endif

