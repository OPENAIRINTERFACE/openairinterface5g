/****************************************************************************/
#if( defined _MSC_VER && _MSC_VER >= 1400 && defined _M_IX86 )

  /* TRD : MSVC, x86
           x86 is CAS, so isolation is cache-line length
  */

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_processor.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR

  typedef int long unsigned lfds700_pal_atom_t;
  typedef int long unsigned lfds700_pal_uint_t;

  #define LFDS700_PAL_PROCESSOR_STRING            "x86"

  #define LFDS700_PAL_ALIGN_SINGLE_POINTER        4
  #define LFDS700_PAL_ALIGN_DOUBLE_POINTER        8

  #define LFDS700_PAL_CACHE_LINE_LENGTH_IN_BYTES  32
  #define LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES   32

#endif





/****************************************************************************/
#if( defined _MSC_VER && _MSC_VER >= 1400 && (defined _M_X64 || defined _M_AMD64) )

  /* TRD : MSVC, x64
           x64 is CAS, so isolation is cache-line length
  */

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_processor.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR

  typedef int long long unsigned lfds700_pal_atom_t;
  typedef int long long unsigned lfds700_pal_uint_t;

  #define LFDS700_PAL_PROCESSOR_STRING            "x64"

  #define LFDS700_PAL_ALIGN_SINGLE_POINTER        8
  #define LFDS700_PAL_ALIGN_DOUBLE_POINTER        16

  #define LFDS700_PAL_CACHE_LINE_LENGTH_IN_BYTES  64
  #define LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES   64

#endif





/****************************************************************************/
#if( defined _MSC_VER && _MSC_VER >= 1400 && defined _M_IA64 )

  /* TRD : MSVC, Itanium
           IA64 is CAS, so isolation is cache-line length
  */

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_processor.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR

  typedef int long long unsigned lfds700_pal_atom_t;
  typedef int long long unsigned lfds700_pal_uint_t;

  #define LFDS700_PAL_PROCESSOR_STRING            "IA64"

  #define LFDS700_PAL_ALIGN_SINGLE_POINTER        8
  #define LFDS700_PAL_ALIGN_DOUBLE_POINTER        16

  #define LFDS700_PAL_CACHE_LINE_LENGTH_IN_BYTES  64
  #define LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES   64

#endif





  /****************************************************************************/
#if( defined _MSC_VER && _MSC_VER >= 1400 && defined _M_ARM )

  /* TRD : MSVC, 32-bit ARM

  ARM is LL/SC and uses a reservation granule of 8 to 2048 bytes
  so the isolation value used here is worst-case - be sure to set
  this correctly, otherwise structures are painfully large
  */

#ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR
#error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_processor.h
#endif

#define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR

  typedef int long unsigned lfds700_pal_atom_t;
  typedef int long unsigned lfds700_pal_uint_t;

#define LFDS700_PAL_PROCESSOR_STRING            "ARM (32-bit)"

#define LFDS700_PAL_ALIGN_SINGLE_POINTER        4
#define LFDS700_PAL_ALIGN_DOUBLE_POINTER        8

#define LFDS700_PAL_CACHE_LINE_LENGTH_IN_BYTES  32
#define LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES   2048

#endif
  
  
  
  
  
/****************************************************************************/
#if( defined __GNUC__ && defined __arm__ )

  /* TRD : GCC, 32-bit ARM

           ARM is LL/SC and uses a reservation granule of 8 to 2048 bytes
           so the isolation value used here is worst-case - be sure to set
           this correctly, otherwise structures are painfully large
  */

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_processor.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR

  typedef int long unsigned lfds700_pal_atom_t;
  typedef int long unsigned lfds700_pal_uint_t;

  #define LFDS700_PAL_PROCESSOR_STRING            "ARM (32-bit)"

  #define LFDS700_PAL_ALIGN_SINGLE_POINTER        4
  #define LFDS700_PAL_ALIGN_DOUBLE_POINTER        8

  #define LFDS700_PAL_CACHE_LINE_LENGTH_IN_BYTES  32
  #define LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES   2048

#endif





/****************************************************************************/
#if( defined __GNUC__ && defined __aarch64__ )

  /* TRD : GCC, 64-bit ARM

           ARM is LL/SC and uses a reservation granule of 8 to 2048 bytes
           so the isolation value used here is worst-case - be sure to set
           this correctly, otherwise structures are painfully large
  */

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_processor.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR

  typedef int long long unsigned lfds700_pal_atom_t;
  typedef int long long unsigned lfds700_pal_uint_t;

  #define LFDS700_PAL_PROCESSOR_STRING            "ARM (64-bit)"

  #define LFDS700_PAL_ALIGN_SINGLE_POINTER        8
  #define LFDS700_PAL_ALIGN_DOUBLE_POINTER        16

  #define LFDS700_PAL_CACHE_LINE_LENGTH_IN_BYTES  64
  #define LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES   2048

#endif





/****************************************************************************/
#if( defined __GNUC__ && (defined __i686__ || defined __i586__ || defined __i486__) )

  /* TRD : GCC, x86

           x86 is CAS, so isolation is cache-line length
  */

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_processor.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR

  typedef int long unsigned lfds700_pal_atom_t;
  typedef int long unsigned lfds700_pal_uint_t;

  #define LFDS700_PAL_PROCESSOR_STRING            "x86"

  #define LFDS700_PAL_ALIGN_SINGLE_POINTER        4
  #define LFDS700_PAL_ALIGN_DOUBLE_POINTER        8

  #define LFDS700_PAL_CACHE_LINE_LENGTH_IN_BYTES  32
  #define LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES   32

#endif





/****************************************************************************/
#if( defined __GNUC__ && defined __x86_64__ )

  /* TRD : GCC, x86

           x64 is CAS, so isolation is cache-line length
  */

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_processor.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR

  typedef int long long unsigned lfds700_pal_atom_t;
  typedef int long long unsigned lfds700_pal_uint_t;

  #define LFDS700_PAL_PROCESSOR_STRING            "x64"

  #define LFDS700_PAL_ALIGN_SINGLE_POINTER        8
  #define LFDS700_PAL_ALIGN_DOUBLE_POINTER        16

  #define LFDS700_PAL_CACHE_LINE_LENGTH_IN_BYTES  64
  #define LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES   64

#endif





/****************************************************************************/
#if( defined __GNUC__ && defined __alpha__ )

  /* TRD : GCC, alpha

           alpha is LL/SC, but there is only one reservation per processor,
           so the isolation value used here is cache-line length
  */

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_processor.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR

  typedef int long unsigned lfds700_pal_atom_t;
  typedef int long unsigned lfds700_pal_uint_t;

  #define LFDS700_PAL_PROCESSOR_STRING            "alpha"

  #define LFDS700_PAL_ALIGN_SINGLE_POINTER        8
  #define LFDS700_PAL_ALIGN_DOUBLE_POINTER        16

  #define LFDS700_PAL_CACHE_LINE_LENGTH_IN_BYTES  32
  #define LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES   64

#endif





/****************************************************************************/
#if( defined __GNUC__ && defined __ia64__ )

  /* TRD : GCC, Itanium

           Itanium is CAS, so isolation is cache-line length
  */

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_processor.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR

  typedef int long long unsigned lfds700_pal_atom_t;
  typedef int long long unsigned lfds700_pal_uint_t;

  #define LFDS700_PAL_PROCESSOR_STRING            "IA64"

  #define LFDS700_PAL_ALIGN_SINGLE_POINTER        8
  #define LFDS700_PAL_ALIGN_DOUBLE_POINTER        16

  #define LFDS700_PAL_CACHE_LINE_LENGTH_IN_BYTES  64
  #define LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES   64

#endif





/****************************************************************************/
#if( defined __GNUC__ && defined __mips__ )

  /* TRD : GCC, MIPS (32-bit)

           MIPS is LL/SC, but there is only one reservation per processor,
           so the isolation value used here is cache-line length
  */

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_processor.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR

  typedef int long unsigned lfds700_pal_atom_t;
  typedef int long unsigned lfds700_pal_uint_t;

  #define LFDS700_PAL_PROCESSOR_STRING            "MIPS (32-bit)"

  #define LFDS700_PAL_ALIGN_SINGLE_POINTER        4
  #define LFDS700_PAL_ALIGN_DOUBLE_POINTER        8

  #define LFDS700_PAL_CACHE_LINE_LENGTH_IN_BYTES  32
  #define LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES   32

#endif





/****************************************************************************/
#if( defined __GNUC__ && defined __mips64 )

  /* TRD : GCC, MIPS (64-bit)

           MIPS is LL/SC, but there is only one reservation per processor,
           so the isolation value used here is cache-line length
  */

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_processor.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR

  typedef int long long unsigned lfds700_pal_atom_t;
  typedef int long long unsigned lfds700_pal_uint_t;

  #define LFDS700_PAL_PROCESSOR_STRING            "MIPS (64-bit)"

  #define LFDS700_PAL_ALIGN_SINGLE_POINTER        8
  #define LFDS700_PAL_ALIGN_DOUBLE_POINTER        16

  #define LFDS700_PAL_CACHE_LINE_LENGTH_IN_BYTES  64
  #define LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES   64

#endif





/****************************************************************************/
#if( defined __GNUC__ && defined __ppc__ )

  /* TRD : GCC, POWERPC (32-bit)

           POWERPC is LL/SC and uses a reservation granule but I can't find
           canonical documentation for its size - 128 bytes seems to be the
           largest value I've found
  */

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_processor.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR

  typedef int long unsigned lfds700_pal_atom_t;
  typedef int long unsigned lfds700_pal_uint_t;

  #define LFDS700_PAL_PROCESSOR_STRING            "POWERPC (32-bit)"

  #define LFDS700_PAL_ALIGN_SINGLE_POINTER        4
  #define LFDS700_PAL_ALIGN_DOUBLE_POINTER        8

  #define LFDS700_PAL_CACHE_LINE_LENGTH_IN_BYTES  32
  #define LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES   128

#endif





/****************************************************************************/
#if( defined __GNUC__ && defined __ppc64__ )

  /* TRD : GCC, POWERPC (64-bit)

           POWERPC is LL/SC and uses a reservation granule but I can't find
           canonical documentation for its size - 128 bytes seems to be the
           largest value I've found
  */

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_processor.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR

  typedef int long long unsigned lfds700_pal_atom_t;
  typedef int long long unsigned lfds700_pal_uint_t;

  #define LFDS700_PAL_PROCESSOR_STRING            "POWERPC (64-bit)"

  #define LFDS700_PAL_ALIGN_SINGLE_POINTER        8
  #define LFDS700_PAL_ALIGN_DOUBLE_POINTER        16

  #define LFDS700_PAL_CACHE_LINE_LENGTH_IN_BYTES  64
  #define LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES   128

#endif





/****************************************************************************/
#if( defined __GNUC__ && defined __sparc__ && !defined __sparc_v9__ )

  /* TRD : GCC, SPARC (32-bit)

           SPARC is CAS, so isolation is cache-line length
  */

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_processor.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR

  typedef int long unsigned lfds700_pal_atom_t;
  typedef int long unsigned lfds700_pal_uint_t;

  #define LFDS700_PAL_PROCESSOR_STRING            "SPARC (32-bit)"

  #define LFDS700_PAL_ALIGN_SINGLE_POINTER        4
  #define LFDS700_PAL_ALIGN_DOUBLE_POINTER        8

  #define LFDS700_PAL_CACHE_LINE_LENGTH_IN_BYTES  32
  #define LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES   32

#endif





/****************************************************************************/
#if( defined __GNUC__ && defined __sparc__ && defined __sparc_v9__ )

  /* TRD : GCC, SPARC (64-bit)

           SPARC is CAS, so isolation is cache-line length
  */

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_processor.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR

  typedef int long long unsigned lfds700_pal_atom_t;
  typedef int long long unsigned lfds700_pal_uint_t;

  #define LFDS700_PAL_PROCESSOR_STRING            "SPARC (64-bit)"

  #define LFDS700_PAL_ALIGN_SINGLE_POINTER        8
  #define LFDS700_PAL_ALIGN_DOUBLE_POINTER        16

  #define LFDS700_PAL_CACHE_LINE_LENGTH_IN_BYTES  64
  #define LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES   64

#endif





/****************************************************************************/
#if( defined __GNUC__ && defined __m68k__ )

  /* TRD : GCC, 680x0

           680x0 is CAS, so isolation is cache-line length
  */

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_processor.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR

  typedef int long unsigned lfds700_pal_atom_t;
  typedef int long unsigned lfds700_pal_uint_t;

  #define LFDS700_PAL_PROCESSOR_STRING            "680x0"

  #define LFDS700_PAL_ALIGN_SINGLE_POINTER        4
  #define LFDS700_PAL_ALIGN_DOUBLE_POINTER        8

  #define LFDS700_PAL_CACHE_LINE_LENGTH_IN_BYTES  32
  #define LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES   32

#endif





/****************************************************************************/
#if( !defined LFDS700_PAL_PORTING_ABSTRACTION_LAYER_PROCESSOR )

  #error No matching porting abstraction layer in lfds700_porting_abstraction_layer_processor.h

#endif

