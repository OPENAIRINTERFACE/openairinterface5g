/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include  "sim.h"

static unsigned int urseed, iy, ir[98]; /// uniformrandom
static bool tableNordDone = false; /// gaussZiggurat

/*!\brief Generate a random number form `/dev/urandom`. */
void fill_random(void *buf, size_t sz)
{
  const char* fn = "/dev/urandom";
  FILE* f = fopen(fn, "rb");
  if (f == NULL) {
    fprintf(stderr, "could not open %s for seed generation: %d %s\n", fn, errno, strerror(errno));
    abort();
  }
  int rc = fread(buf, sz, 1, f);
  if (rc < 0) {
    fprintf(stderr, "could not read %s for seed generation: %d %s\n", fn, errno, strerror(errno));
    abort();
  }
  fclose(f);
}


static const unsigned int a = 1664525lu;

/*!\brief Initialization routine for Uniform/Gaussian random number generators. */
void randominit(unsigned long seed_init)
{
  unsigned long seed = seed_init;
  if (seed_init == 0)
    fill_random(&seed, sizeof(seed));
  const char* str_oai_rngseed = getenv("OAI_RNGSEED");
  if (str_oai_rngseed != NULL)
    seed = atoi(str_oai_rngseed);
  printf("Initializing random number generator, seed %lu\n", seed);

  // initialize uniformrandom RNG
  urseed = (unsigned int) seed;
  if (urseed % 2 == 0)
    urseed += 1; /* urseed and mod are relative prime */
  for (int i = 1; i <= 97; i++) {
    urseed = a * urseed; /* mod 2**32  */
    ir[i] = urseed; /* initialize the shuffle table    */
  }
  iy = 1;

  // initialize gaussZiggurat RNG
  tableNor(seed);
  tableNordDone = true;
}

/*
@defgroup _uniformdouble
@ingroup numerical Uniform linear congruential random number generator.
*/

/*!\brief Uniform linear congruential random number generator on \f$[0,1)\f$.  Returns a double-precision floating-point number.*/

double uniformrandom(void)
{
  const double mod = 4294967296.0; /* is 2**32 */
  int j = 1 + 97.0 * iy / mod;
  iy = ir[j];
  urseed = a * urseed; /* mod 2**32 */
  ir[j] = urseed;
  return (double)iy / mod;
}

/*
@defgroup _gaussdouble Gaussian random number generator based on modified Box-Muller transformation.
@ingroup numerical
*/

/*!\brief Gaussian random number generator based on modified Box-Muller transformation.Returns a double-precision floating-point number. */

double __attribute__ ((no_sanitize_address)) gaussdouble(double mean, double variance)
{
  static int iset=0;
  static double gset;
  double fac,r,v1,v2;

  if (iset == 0) {
    do {
      v1 = 2.0*uniformrandom()-1.0;
      v2 = 2.0*uniformrandom()-1.0;
      r = v1*v1+v2*v2;
    }  while (r >= 1.0);

    fac = sqrt(-2.0*log(r)/r);
    gset= v1*fac;
    iset=1;
    return(sqrt(variance)*v2*fac + mean);
  } else {
    iset=0;
    return(sqrt(variance)*gset + mean);
  }
}


/*
@defgroup _gaussZiggurat
@ingroup numerical ziggurat random number generator for exponentially distributed numbers
*/
// Ziggurat
static double wn[128], fn[128];
static uint32_t iz, jz, jsr = 123456789, kn[128];
static int32_t hz;
#define SHR3 (jz = jsr, jsr ^= (jsr << 13), jsr ^= (jsr >> 17), jsr ^= (jsr << 5), jz + jsr)
#define UNI (0.5 + (signed)SHR3 * 0.2328306e-9)

double nfix(void)
{
  const double r = 3.442620;
  static double x, y;

  for (;;) {
    x = hz * wn[iz];

    if (iz == 0) {
      do {
        x = -0.2904764 * log(UNI);
        y = -log(UNI);
      } while (y + y < x * x);

      return (hz > 0) ? r + x : -r - x;
    }

    if (fn[iz] + UNI * (fn[iz - 1] - fn[iz]) < exp(-0.5 * x * x)) {
      return x;
    }

    hz = SHR3;
    iz = hz & 127;

    if (abs(hz) < kn[iz]) {
      return ((hz)*wn[iz]);
    }
  }
}

/*!Procedure to create tables for normal distribution kn,wn and fn. */
void tableNor(unsigned long seed)
{
  jsr = seed;
  double dn = 3.442619855899;
  int i;
  const double m1 = 2147483648.0;
  double q;
  double tn = 3.442619855899;
  const double vn = 9.91256303526217E-03;
  q = vn / exp(-0.5 * dn * dn);
  kn[0] = ((dn / q) * m1);
  kn[1] = 0;
  wn[0] = (q / m1);
  wn[127] = (dn / m1);
  fn[0] = 1.0;
  fn[127] = (exp(-0.5 * dn * dn));

  for (i = 126; 1 <= i; i--) {
    dn = sqrt(-2.0 * log(vn / dn + exp(-0.5 * dn * dn)));
    kn[i + 1] = ((dn / tn) * m1);
    tn = dn;
    fn[i] = (exp(-0.5 * dn * dn));
    wn[i] = (dn / m1);
  }
  tableNordDone=true;
  return;
}

double __attribute__ ((no_sanitize("address", "undefined"))) gaussZiggurat(double mean, double variance)
{
  if (!tableNordDone) {
    // let's make reasonnable constant tables
    unsigned long seed;
    fill_random(&seed, sizeof(seed));
    tableNor(seed);
  }
  hz = SHR3;
  iz = hz & 127;
  return hz != INT32_MIN && abs(hz) < kn[iz] ? hz * wn[iz] : nfix();
}

#ifdef MAIN
main(int argc,char **argv)
{

  int i;

  randominit();

  for (i=0; i<10; i++) {
    printf("%f\n",gaussdouble(0.0,1.0));
  }
}
#endif

