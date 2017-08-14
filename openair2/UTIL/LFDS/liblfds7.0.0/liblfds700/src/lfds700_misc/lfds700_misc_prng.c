/***** includes *****/
#include "lfds700_misc_internal.h"

/***** defines *****/
#define LFDS700_PRNG_STATE_SIZE  16

/***** struct *****/
struct lfds700_misc_prng_big_slow_high_quality_state
{
  lfds700_pal_atom_t LFDS700_PAL_ALIGN(LFDS700_PAL_ATOMIC_ISOLATION_IN_BYTES)
    xorshift1024star_spinlock;

  // TRD : must be a 32 bit signed int
  int
    xorshift1024star_index;

  int long long unsigned
    xorshift1024star_state[LFDS700_PRNG_STATE_SIZE];
};

/***** locals *****/
struct lfds700_misc_prng_big_slow_high_quality_state
  pbshqs;

/***** private prototypes *****/
static void lfds700_misc_prng_internal_hash_murmurhash3( int long long unsigned *murmurhash3_state );
static void lfds700_misc_prng_internal_big_slow_high_quality_generate( struct lfds700_misc_prng_big_slow_high_quality_state *ps, lfds700_pal_uint_t *random_value );





/****************************************************************************/
void lfds700_misc_prng_init( struct lfds700_misc_prng_state *ps )
{
  LFDS700_PAL_ASSERT( ps != NULL );

  /* TRD : we use the big, slow, high quality PRNG to generate the initial value
           for the small, fast, low qulity PRNG, which is used in exponential backoff

           we need the load barrier to catch any changes to the backoff periods
  */

  lfds700_misc_prng_internal_big_slow_high_quality_generate( &pbshqs, &ps->prng_state );

  LFDS700_MISC_BARRIER_LOAD;

  ps->local_copy_of_global_exponential_backoff_timeslot_length_in_loop_iterations_for_cas = lfds700_misc_globals.exponential_backoff_timeslot_length_in_loop_iterations_for_cas;
  ps->local_copy_of_global_exponential_backoff_timeslot_length_in_loop_iterations_for_dwcas = lfds700_misc_globals.exponential_backoff_timeslot_length_in_loop_iterations_for_dwcas;

  return;
}





/****************************************************************************/
void lfds700_misc_prng_internal_big_slow_high_quality_init( int long long unsigned seed )
{
  lfds700_pal_uint_t
    loop;

  LFDS700_PAL_ASSERT( seed != 0 );   // TRD : a 0 seed causes all zeros in the entropy state, so is forbidden

  pbshqs.xorshift1024star_spinlock = LFDS700_MISC_FLAG_LOWERED;

  for( loop = 0 ; loop < LFDS700_PRNG_STATE_SIZE ; loop++ )
  {
    lfds700_misc_prng_internal_hash_murmurhash3( &seed );
    pbshqs.xorshift1024star_state[loop] = seed;
  }

  pbshqs.xorshift1024star_index = 0;

  return;
}





/****************************************************************************/
static void lfds700_misc_prng_internal_hash_murmurhash3( int long long unsigned *murmurhash3_state )
{
  LFDS700_PAL_ASSERT( murmurhash3_state != NULL );

	*murmurhash3_state ^= *murmurhash3_state >> 33;
	*murmurhash3_state *= 0xff51afd7ed558ccdULL;
	*murmurhash3_state ^= *murmurhash3_state >> 33;
	*murmurhash3_state *= 0xc4ceb9fe1a85ec53ULL;
	*murmurhash3_state ^= *murmurhash3_state >> 33;

  return;
}





/****************************************************************************/
static void lfds700_misc_prng_internal_big_slow_high_quality_generate( struct lfds700_misc_prng_big_slow_high_quality_state *ps, lfds700_pal_uint_t *random_value )
{
  char unsigned 
    result;

  int long long unsigned
    xs_temp_one,
    xs_temp_two;

  lfds700_pal_atom_t
    compare = LFDS700_MISC_FLAG_LOWERED,
    exchange = LFDS700_MISC_FLAG_LOWERED;

  LFDS700_PAL_ASSERT( ps != NULL );
  LFDS700_PAL_ASSERT( random_value != NULL );

  // TRD : this is single-threaded code, on a per-state basis
  do
  {
    compare = LFDS700_MISC_FLAG_LOWERED;
    LFDS700_PAL_ATOMIC_CAS( &ps->xorshift1024star_spinlock, &compare, (lfds700_pal_atom_t) LFDS700_MISC_FLAG_RAISED, LFDS700_MISC_CAS_STRENGTH_STRONG, result );
  }
  while( result == 0 );

  // TRD : xorshift1024* code from here; http://xorshift.di.unimi.it/xorshift1024star.c

  xs_temp_one = ps->xorshift1024star_state[ ps->xorshift1024star_index ];
  ps->xorshift1024star_index = ( ps->xorshift1024star_index + 1 ) & 15;
  xs_temp_two = ps->xorshift1024star_state[ ps->xorshift1024star_index ];

  xs_temp_two ^= xs_temp_two << 31;
  xs_temp_two ^= xs_temp_two >> 11;
  xs_temp_one ^= xs_temp_one >> 30;

  ps->xorshift1024star_state[ ps->xorshift1024star_index ] = xs_temp_one ^ xs_temp_two;

  *random_value = (lfds700_pal_uint_t) ( ps->xorshift1024star_state[ ps->xorshift1024star_index ] * 1181783497276652981LL );

  LFDS700_PAL_ATOMIC_EXCHANGE( &ps->xorshift1024star_spinlock, &exchange );

  return;
}

