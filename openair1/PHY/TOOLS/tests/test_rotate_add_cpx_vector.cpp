/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include <stdint.h>
#include <string>
#include <vector>
extern "C" {
#include "openair1/PHY/TOOLS/tools_defs.h"
struct configmodule_interface_s;
struct configmodule_interface_s *uniqCfg = NULL;
void exit_function(const char *file, const char *function, const int line, const char *s, const int assert)
{
  if (assert) {
    abort();
  } else {
    exit(EXIT_SUCCESS);
  }
}
}
// tools_defs.h drags in common/utils/T/T.h, whose T(...) tracing macro shadows gtest's pervasive
// "T" template parameter name - undo it before pulling gtest in (same fix as
// common/utils/simple_executable.h).
#ifdef T
#undef T
#endif
#include <gtest/gtest.h>
#include "benchmark/benchmark.h"
#include "openair1/PHY/TOOLS/phy_test_tools.hpp"

// Reference model matching production semantics exactly: the shifted product is saturated to
// Q1.15 (like the SIMD tiers' packs_epi32), then the accumulate saturates on top of that.
static c16_t rotate_add_ref_term(c16_t x, c16_t alpha, int shift)
{
  return c16mulShiftSat(x, alpha, shift);
}

static int16_t sat16_ref(int32_t v)
{
  if (v > INT16_MAX)
    return INT16_MAX;
  if (v < INT16_MIN)
    return INT16_MIN;
  return (int16_t)v;
}

static void rotate_add_cpx_vector_ref(const c16_t *x, c16_t alpha, c16_t *y, uint32_t N, int shift)
{
  for (uint32_t k = 0; k < N; k++) {
    c16_t term = rotate_add_ref_term(x[k], alpha, shift);
    y[k].r = sat16_ref((int32_t)y[k].r + term.r);
    y[k].i = sat16_ref((int32_t)y[k].i + term.i);
  }
}

// On aarch64, rotate_cpx_vector() rounds for output_shift == 15 (NEON vqrdmulh/vqrshrn), while
// rotate_add_cpx_vector() truncates like the reference above, so the two can differ by 1 LSB there.
#ifdef __aarch64__
static const int rotate_q15_tolerance = 1;
#else
static const int rotate_q15_tolerance = 0;
#endif

static AlignedVector512<c16_t> small_c16(size_t num, int16_t base)
{
  AlignedVector512<c16_t> vec;
  vec.resize(num);
  for (size_t i = 0; i < num; i++) {
    vec[i] = {(int16_t)(base + (int)i), (int16_t)(base - (int)i)};
  }
  return vec;
}

class RotateAddCpxVector : public ::testing::TestWithParam<int> {};

TEST_P(RotateAddCpxVector, MatchesReferenceAcrossTierAndTail)
{
  const int N = GetParam();
  auto x = small_c16(N, 50);
  c16_t alpha = {23170, -12000};
  const int shift = 15;

  AlignedVector512<c16_t> y(N), y_ref(N);
  for (int i = 0; i < N; i++) {
    y[i] = y_ref[i] = {(int16_t)(10 - i), (int16_t)(i - 10)};
  }

  rotate_add_cpx_vector(x.data(), alpha, y.data(), N, shift);
  rotate_add_cpx_vector_ref(x.data(), alpha, y_ref.data(), N, shift);

  for (int i = 0; i < N; i++) {
    EXPECT_EQ(y[i].r, y_ref[i].r) << "real mismatch at " << i << " (N=" << N << ")";
    EXPECT_EQ(y[i].i, y_ref[i].i) << "imag mismatch at " << i << " (N=" << N << ")";
  }
}

INSTANTIATE_TEST_SUITE_P(Sizes, RotateAddCpxVector, ::testing::Values(1, 2, 3, 4, 5, 7, 8, 13, 100, 131));

// x and y may start at any sample (the UL beam combine works on a PRB range inside an FFT window,
// which for odd PRB counts starts 8 bytes off a 16-byte boundary): every tier must accept that.
TEST(RotateAddCpxVector, AcceptsUnalignedPointers)
{
  const int N = 36; // exercises the 128-bit tier after any wider one
  c16_t alpha = {23170, -12000};
  for (int off = 1; off < 16; off++) {
    auto x = small_c16(N + off, 50);
    AlignedVector512<c16_t> y(N + off), y_ref(N + off);
    for (int i = 0; i < N + off; i++) {
      y[i] = y_ref[i] = {(int16_t)(10 - i), (int16_t)(i - 10)};
    }
    rotate_add_cpx_vector(x.data() + off, alpha, y.data() + off, N, 15);
    rotate_add_cpx_vector_ref(x.data() + off, alpha, y_ref.data() + off, N, 15);
    for (int i = 0; i < N + off; i++) {
      ASSERT_EQ(y[i].r, y_ref[i].r) << "off " << off << " i " << i;
      ASSERT_EQ(y[i].i, y_ref[i].i) << "off " << off << " i " << i;
    }
  }
}

TEST(RotateAddCpxVector, SaturatesInsteadOfWrapping)
{
  const int N = 8;
  AlignedVector512<c16_t> x(N);
  std::fill(x.begin(), x.end(), c16_t{32767, 32767});
  c16_t alpha = {32767, 0}; // near-identity: term ~= x

  AlignedVector512<c16_t> y(N);
  std::fill(y.begin(), y.end(), c16_t{32767, 32767}); // already saturated - adding more must not wrap

  rotate_add_cpx_vector(x.data(), alpha, y.data(), N, 0 /* no shift: force a large term */);

  for (int i = 0; i < N; i++) {
    EXPECT_EQ(y[i].r, INT16_MAX) << "index " << i;
    EXPECT_EQ(y[i].i, INT16_MAX) << "index " << i;
  }
}

// Equivalence with rotate_cpx_vector(): accumulating onto zero must match overwrite (bit-identical
// except for the aarch64 rounding difference, see rotate_q15_tolerance).
TEST(RotateAddCpxVector, EquivalentToRotateWhenStartingFromZero)
{
  const int N = 37; // spans the SIMD tier and the scalar tail
  auto x = generate_random_c16(N);
  c16_t alpha = {20000, -9000};
  const int shift = 15;

  AlignedVector512<c16_t> y_fused(N, c16_t{0, 0});
  AlignedVector512<c16_t> y_overwrite(N);

  rotate_add_cpx_vector(x.data(), alpha, y_fused.data(), N, shift);
  rotate_cpx_vector(x.data(), alpha, y_overwrite.data(), N, shift);

  for (int i = 0; i < N; i++) {
    EXPECT_NEAR(y_fused[i].r, y_overwrite[i].r, rotate_q15_tolerance) << "index " << i;
    EXPECT_NEAR(y_fused[i].i, y_overwrite[i].i, rotate_q15_tolerance) << "index " << i;
  }
}


TEST(RotateAddCpxVector, EquivalentToRotateThenAdd)
{
  const int N = 41;
  auto x = generate_random_c16(N);
  c16_t alpha = {-15000, 27000};
  const int shift = 15;

  AlignedVector512<c16_t> y_fused = generate_random_c16(N);
  AlignedVector512<c16_t> y_twopass = y_fused;

  rotate_add_cpx_vector(x.data(), alpha, y_fused.data(), N, shift);

  AlignedVector512<c16_t> term(N);
  rotate_cpx_vector(x.data(), alpha, term.data(), N, shift);
  add_cpx_vector(term.data(), y_twopass.data(), N);

  for (int i = 0; i < N; i++) {
    EXPECT_NEAR(y_fused[i].r, y_twopass[i].r, rotate_q15_tolerance) << "index " << i;
    EXPECT_NEAR(y_fused[i].i, y_twopass[i].i, rotate_q15_tolerance) << "index " << i;
  }
}

TEST(RotateAddCpxVector, AccumulatesAcrossMultipleCalls)
{
  const int N = 12;
  auto x1 = small_c16(N, 20);
  auto x2 = small_c16(N, -30);
  c16_t alpha1 = {23170, 5000};
  c16_t alpha2 = {-9000, 18000};
  const int shift = 15;

  AlignedVector512<c16_t> y(N, c16_t{0, 0});
  rotate_add_cpx_vector(x1.data(), alpha1, y.data(), N, shift);
  rotate_add_cpx_vector(x2.data(), alpha2, y.data(), N, shift);

  AlignedVector512<c16_t> y_ref(N, c16_t{0, 0});
  rotate_add_cpx_vector_ref(x1.data(), alpha1, y_ref.data(), N, shift);
  rotate_add_cpx_vector_ref(x2.data(), alpha2, y_ref.data(), N, shift);

  for (int i = 0; i < N; i++) {
    EXPECT_EQ(y[i].r, y_ref[i].r) << "index " << i;
    EXPECT_EQ(y[i].i, y_ref[i].i) << "index " << i;
  }
}

// --- Benchmarks: not run by default, see main() below (pass --run_benchmarks) ---

static void BM_rotate_add_cpx_vector_fused(benchmark::State &state)
{
  int vector_size = state.range(0);
  auto x = generate_random_c16(vector_size);
  auto alpha = generate_random_c16(1);
  AlignedVector512<c16_t> y(vector_size, c16_t{0, 0});
  for (auto _ : state) {
    rotate_add_cpx_vector(x.data(), alpha.data()[0], y.data(), vector_size, 15);
  }
}
BENCHMARK(BM_rotate_add_cpx_vector_fused)->RangeMultiplier(4)->Range(12, 3276);

static void BM_rotate_then_add_cpx_vector(benchmark::State &state)
{
  int vector_size = state.range(0);
  auto x = generate_random_c16(vector_size);
  auto alpha = generate_random_c16(1);
  AlignedVector512<c16_t> term(vector_size);
  AlignedVector512<c16_t> y(vector_size, c16_t{0, 0});
  for (auto _ : state) {
    rotate_cpx_vector(x.data(), alpha.data()[0], term.data(), vector_size, 15);
    add_cpx_vector(term.data(), y.data(), vector_size);
  }
}
BENCHMARK(BM_rotate_then_add_cpx_vector)->RangeMultiplier(4)->Range(12, 3276);

int main(int argc, char **argv)
{
  bool benchmark_mode = false;
  std::vector<char *> filtered;
  filtered.push_back(argv[0]);
  for (int i = 1; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--mode=benchmark") {
      benchmark_mode = true;
    } else if (arg == "--mode=test") {
      benchmark_mode = false;
    } else {
      filtered.push_back(argv[i]);
    }
  }
  int argc2 = (int)filtered.size();
  char **argv2 = filtered.data();

  if (benchmark_mode) {
    benchmark::Initialize(&argc2, argv2);
    benchmark::RunSpecifiedBenchmarks();
    return 0;
  }

  testing::InitGoogleTest(&argc2, argv2);
  return RUN_ALL_TESTS();
}
