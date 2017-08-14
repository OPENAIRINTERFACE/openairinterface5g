/***** includes *****/
#include "lfds700_misc_internal.h"





/****************************************************************************/
void lfds700_misc_library_init_valid_on_current_logical_core()
{
  /* TRD : the PRNG arrangement is that each thread has its own state, for a maximum-speed PRNG, where output
           quality is second consideration to performance

           on 64-bit platforms this is xorshift64*, on 32-bit platforms, an unadorned xorshift32

           the seed for each thread however comes from a single, global, maximum-quality PRNG, where quality of
           output is the primary consideration

           for this, I'm using a xorshift1024*

           since the generation from this global PRNG state is not thread safe, but is still quick in
           thread start-up terms, I run a little spin-lock around it

           regarding the seed for this high quality PRNG; it is customary to use time(), but this has a number of
           drawbacks;

           1. liblfds would depend on time() (currently it does not depend on a hosted implementation of standard library)
           2. the output from time may only be 32 bit, and even when it isn't, the top 32 bits are currently all zero...
           3. many threads can begin in the same second; I'd need to add in their thread number,
              which means I'd need to *get* their thread number...

           as such, I've decided to use a *fixed* 64-bit seed for the high-quality PRNG; this seed is run
           through the MurmerHash3 avalanche phase to generate successive 64-bit values, which populate
           the 1024 state of xorshift1024*

           if you have access to a high-frequency clock (often 64-bit), you can use this for the seed
           (don't use it for the per-thread PRNG, unless you know the clock can be read without a context switch)

           murmurhash3 code from here; http://xorshift.di.unimi.it/murmurhash3.c
  */

  lfds700_misc_prng_internal_big_slow_high_quality_init( LFDS700_MISC_PRNG_SEED );

  lfds700_misc_globals.exponential_backoff_timeslot_length_in_loop_iterations_for_cas = EXPONENTIAL_BACKOFF_TIMESLOT_LENGTH_IN_INCS_FOR_CAS;
  lfds700_misc_globals.exponential_backoff_timeslot_length_in_loop_iterations_for_dwcas = EXPONENTIAL_BACKOFF_TIMESLOT_LENGTH_IN_INCS_FOR_DWCAS;

  LFDS700_MISC_BARRIER_STORE;

  lfds700_misc_force_store();

  return;
}

