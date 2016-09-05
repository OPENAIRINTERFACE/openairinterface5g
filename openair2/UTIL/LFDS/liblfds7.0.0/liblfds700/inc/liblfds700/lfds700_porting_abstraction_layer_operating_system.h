/****************************************************************************/
#if( defined _MSC_VER )
  /* TRD : MSVC compiler

           an unfortunately necessary hack for MSVC
           MSVC only defines __STDC__ if /Za is given, where /Za turns off MSVC C extensions - 
           which prevents Windows header files from compiling.
  */

  #define __STDC__         1
  #define __STDC_HOSTED__  1
#endif





/****************************************************************************/
#if( defined _MSC_VER && _MSC_VER >= 1400 && __STDC_HOSTED__ == 1 && !defined _KERNEL_MODE )

  // TRD : MSVC

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_OPERATING_SYSTEM
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_operating_system.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_OPERATING_SYSTEM

  #include <assert.h>

  #define LFDS700_PAL_OS_STRING             "Windows"
  #define LFDS700_PAL_ASSERT( expression )  assert( expression )

#endif





/****************************************************************************/
#if( defined _MSC_VER && _MSC_VER >= 1400 && defined __STDC_HOSTED__ && __STDC_HOSTED__ == 1 && defined _WIN32 && defined _KERNEL_MODE )

  // TRD : MSVC, Windows kernel-mode

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_OPERATING_SYSTEM
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_operating_system.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_OPERATING_SYSTEM

  #include <assert.h>
  #include <intrin.h>

  #define LFDS700_PAL_OS_STRING             "Windows"
  #define LFDS700_PAL_ASSERT( expression )  assert( expression )

#endif





/****************************************************************************/
#if( defined __GNUC__ && __STDC_HOSTED__ == 1 && !(defined __linux__ && defined _KERNEL_MODE) )

  // TRD : GCC, hosted implementation (except for Linux kernel mode)

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_OPERATING_SYSTEM
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_operating_system.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_OPERATING_SYSTEM

  #include <assert.h>

  #define LFDS700_PAL_OS_STRING             "Embedded (hosted)"
  #define LFDS700_PAL_ASSERT( expression )  assert( expression )

#endif





/****************************************************************************/
#if( defined __GNUC__ && __STDC_HOSTED__ == 0 )

  // TRD : GCC, freestanding or bare implementation

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_OPERATING_SYSTEM
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_operating_system.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_OPERATING_SYSTEM

  #define LFDS700_PAL_OS_STRING             "Embedded (freestanding/bare)"
  #define LFDS700_PAL_ASSERT( expression )

#endif





/****************************************************************************/
#if( defined __GNUC__ && defined __linux__ && defined _KERNEL_MODE )

  // TRD : GCC, Linux kernel-mode

  #ifdef LFDS700_PAL_PORTING_ABSTRACTION_LAYER_OPERATING_SYSTEM
    #error More than one porting abstraction layer matches the current platform in lfds700_porting_abstraction_layer_operating_system.h
  #endif

  #define LFDS700_PAL_PORTING_ABSTRACTION_LAYER_OPERATING_SYSTEM

  #include <linux/module.h>

  #define LFDS700_PAL_OS_STRING             "Linux"
  #define LFDS700_PAL_ASSERT( expression )  BUG_ON( expression )

#endif





/****************************************************************************/
#if( !defined LFDS700_PAL_PORTING_ABSTRACTION_LAYER_OPERATING_SYSTEM )

  #error No matching porting abstraction layer in lfds700_porting_abstraction_layer_operating_system.h

#endif

