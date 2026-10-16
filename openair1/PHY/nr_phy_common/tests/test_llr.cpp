/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "gtest/gtest.h"
#include <stdint.h>
#include <vector>
#include <algorithm>
#include <numeric>
extern "C" {
#include "nr_compute_llr.h"
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
#include "openair1/PHY/TOOLS/tools_defs.h"
#include "openair1/SIMULATION/TOOLS/sim.h"
}
#include <cstdio>
#include "common/utils/LOG/log.h"
#include <cstdlib>
#include <memory>
#include "openair1/PHY/TOOLS/phy_test_tools.hpp"
#include <random>

int16_t saturating_sub(int a, int b)
{
  int result = a - abs(b);
  if (result < -INT16_MAX) {
    return -INT16_MAX;
  } else if (result > INT16_MAX) {
    return INT16_MAX;
  } else {
    return (int16_t)result;
  }
}

void nr_16qam_llr_ref(const c16_t *rxdataF_comp, const c16_t *ch_mag, int16_t *llr, uint32_t nb_re)
{
  for (auto i = 0U; i < nb_re; i++) {
    int16_t real = rxdataF_comp[i].r;
    int16_t imag = rxdataF_comp[i].i;
    llr[4 * i] = real;
    llr[4 * i + 1] = imag;
    llr[4 * i + 2] = saturating_sub(ch_mag[i].r, real);
    llr[4 * i + 3] = saturating_sub(ch_mag[i].i, imag);
  }
}

void nr_64qam_llr_ref(const c16_t *rxdataF_comp, const c16_t *ch_mag, const c16_t *ch_magb, int16_t *llr, uint32_t nb_re)
{
  for (auto i = 0U; i < nb_re; i++) {
    int16_t real = rxdataF_comp[i].r;
    int16_t imag = rxdataF_comp[i].i;
    llr[6 * i] = real;
    llr[6 * i + 1] = imag;
    llr[6 * i + 2] = saturating_sub(ch_mag[i].r, real);
    llr[6 * i + 3] = saturating_sub(ch_mag[i].i, imag);
    llr[6 * i + 4] = saturating_sub(ch_magb[i].r, llr[6 * i + 2]);
    llr[6 * i + 5] = saturating_sub(ch_magb[i].i, llr[6 * i + 3]);
  }
}

void nr_256qam_llr_ref(const c16_t *rxdataF_comp, const c16_t *ch_mag, const c16_t *ch_magb, const c16_t *ch_magc, int16_t *llr, uint32_t nb_re)
{
  for (auto i = 0U; i < nb_re; i++) {
    int16_t real = rxdataF_comp[i].r;
    int16_t imag = rxdataF_comp[i].i;
    llr[8 * i] = real;
    llr[8 * i + 1] = imag;
    llr[8 * i + 2] = saturating_sub(ch_mag[i].r, real);
    llr[8 * i + 3] = saturating_sub(ch_mag[i].i, imag);
    llr[8 * i + 4] = saturating_sub(ch_magb[i].r, llr[8 * i + 2]);
    llr[8 * i + 5] = saturating_sub(ch_magb[i].i, llr[8 * i + 3]);
    llr[8 * i + 6] = saturating_sub(ch_magc[i].r, llr[8 * i + 4]);
    llr[8 * i + 7] = saturating_sub(ch_magc[i].i, llr[8 * i + 5]);
  }
}

constexpr int16_t llr_sentinel = 0x5a5a;

// Output buffer of nb_llr LLRs starting llr_offset int16 after a 64-byte boundary, followed by a sentinel
struct LlrBuffer {
  LlrBuffer(uint32_t nb_llr, uint32_t llr_offset) : buf(nb_llr + llr_offset + 1, 0), nb_llr(nb_llr)
  {
    llr = buf.data() + llr_offset;
    llr[nb_llr] = llr_sentinel;
  }
  bool sentinel_intact() const
  {
    return llr[nb_llr] == llr_sentinel;
  }
  AlignedVector512<int16_t> buf;
  int16_t *llr;
  uint32_t nb_llr;
};

void test_function_16_qam(AlignedVector512<uint32_t> nb_res, uint32_t llr_offset = 0)
{
  for (auto i = 0U; i < nb_res.size(); i++) {
    uint32_t nb_re = nb_res[i];
    auto rf_data = generate_random_c16(nb_re);
    auto magnitude_data = generate_random_uint16(nb_re * 2);
    const c16_t *rf = (const c16_t *)rf_data.data();
    const c16_t *mag = (const c16_t *)magnitude_data.data();
    AlignedVector512<int16_t> llr_ref;
    llr_ref.resize(nb_re * 4);
    std::fill(llr_ref.begin(), llr_ref.end(), 0);
    nr_16qam_llr_ref(rf, mag, (int16_t *)llr_ref.data(), nb_re);

    LlrBuffer out(nb_re * 4, llr_offset);
    int16_t *llr = out.llr;
    nr_16qam_llr(rf, mag, llr, nb_re);
    EXPECT_TRUE(out.sentinel_intact()) << "16qam llr written past the end, nb res " << nb_re;

    int num_errors = 0;
    for (auto i = 0U; i < llr_ref.size(); i++) {
      EXPECT_LE(llr_ref[i] - llr[i] ? (llr_ref[i] - llr[i]) / (float)llr[i] : 0, 0.001)
          << "Mismatch 16qam REF " << std::hex << llr_ref[i] << " != DUT " << llr[i] << " at " << std::dec << i
          << "total err: " << ++num_errors;
    }
    EXPECT_EQ(num_errors, 0) << " Errors during testing 16qam llr " << num_errors << " nb res " << nb_re;
  }
}

void test_function_64_qam(AlignedVector512<uint32_t> nb_res, uint32_t llr_offset = 0)
{
  for (auto i = 0U; i < nb_res.size(); i++) {
    uint32_t nb_re = nb_res[i];
    auto rf_data = generate_random_c16(nb_re);
    auto magnitude_data = generate_random_uint16(nb_re * 2);
    auto magnitude_b_data = generate_random_uint16(nb_re * 2);
    const c16_t *rf = (const c16_t *)rf_data.data();
    const c16_t *mag = (const c16_t *)magnitude_data.data();
    const c16_t *mag_b = (const c16_t *)magnitude_b_data.data();
    AlignedVector512<int16_t> llr_ref;
    llr_ref.resize(nb_re * 6);
    std::fill(llr_ref.begin(), llr_ref.end(), 0);
    nr_64qam_llr_ref(rf, mag, mag_b, (int16_t *)llr_ref.data(), nb_re);

    LlrBuffer out(nb_re * 6, llr_offset);
    int16_t *llr = out.llr;
    nr_64qam_llr(rf, mag, mag_b, llr, nb_re);
    EXPECT_TRUE(out.sentinel_intact()) << "64qam llr written past the end, nb res " << nb_re;

    int num_errors = 0;
    for (auto i = 0U; i < llr_ref.size(); i++) {
      EXPECT_LE(llr_ref[i] - llr[i] ? (llr_ref[i] - llr[i]) / (float)llr[i] : 0, 0.001)
          << "Mismatch 64qam REF " << std::hex << llr_ref[i] << " != DUT " << llr[i] << " at " << std::dec << i
          << "total err: " << ++num_errors;
    }
    EXPECT_EQ(num_errors, 0) << " Errors during testing 64qam llr " << num_errors << " nb res " << nb_re;
  }
}

void test_function_256_qam(AlignedVector512<uint32_t> nb_res, uint32_t llr_offset = 0)
{
  for (auto i = 0U; i < nb_res.size(); i++) {
    uint32_t nb_re = nb_res[i];
    auto rf_data = generate_random_c16(nb_re);
    auto magnitude_data = generate_random_uint16(nb_re * 2);
    auto magnitude_b_data = generate_random_uint16(nb_re * 2);
    auto magnitude_c_data = generate_random_uint16(nb_re * 2);
    const c16_t *rf = (const c16_t *)rf_data.data();
    const c16_t *mag = (const c16_t *)magnitude_data.data();
    const c16_t *mag_b = (const c16_t *)magnitude_b_data.data();
    const c16_t *mag_c = (const c16_t *)magnitude_c_data.data();
    AlignedVector512<int16_t> llr_ref;
    llr_ref.resize(nb_re * 8);
    std::fill(llr_ref.begin(), llr_ref.end(), 0);
    nr_256qam_llr_ref(rf, mag, mag_b, mag_c, (int16_t *)llr_ref.data(), nb_re);

    LlrBuffer out(nb_re * 8, llr_offset);
    int16_t *llr = out.llr;
    nr_256qam_llr(rf, mag, mag_b, mag_c, llr, nb_re);
    EXPECT_TRUE(out.sentinel_intact()) << "256qam llr written past the end, nb res " << nb_re;

    int num_errors = 0;
    for (auto i = 0U; i < llr_ref.size(); i++) {
      EXPECT_LE(llr_ref[i] - llr[i] ? (llr_ref[i] - llr[i]) / (float)llr[i] : 0, 0.001)
          << "Mismatch 256qam REF " << std::hex << llr_ref[i] << " != DUT " << llr[i] << " at " << std::dec << i
          << "total err: " << ++num_errors;
    }
    EXPECT_EQ(num_errors, 0) << " Errors during testing 256qam llr " << num_errors << " nb res " << nb_re;
  }
}

TEST(test_llr, verify_reference_implementation_16qam)
{
  test_function_16_qam({16, 32, 24, 40, 48, 8 * 300});
}

TEST(test_llr, test_8_res_16qam)
{
  test_function_16_qam({8});
}

TEST(test_llr, test_4_res_16qam)
{
  test_function_16_qam({4});
}

TEST(test_llr, test_5_res_16qam)
{
  test_function_16_qam({5});
}

// This is a "normal" segfault because the function assumed extra buffer for reading non-existent REs
TEST(test_llr, no_segmentation_fault_at_12_res_16qam)
{
  test_function_16_qam({12});
}

// This is a "normal" segfault because the function assumed extra buffer for reading non-existent REs
TEST(test_llr, no_segmentation_fault_at_36_res_16qam)
{
  test_function_16_qam({36});
}

// any number of REs should work
TEST(test_llr, no_segfault_any_number_of_re_16qam)
{
  for (uint32_t i = 0U; i < 1000U; i++) {
    test_function_16_qam({i});
  }
}

TEST(test_llr, verify_reference_implementation_64qam)
{
  test_function_64_qam({16, 24, 32, 80, 8 * 300});
}

TEST(test_llr, test_8_res_64qam)
{
  test_function_64_qam({8});
}

TEST(test_llr, test_4_res_64qam)
{
  test_function_64_qam({4});
}

// This is a "normal" segfault because the function assumed extra buffer for reading non-existent REs
TEST(test_llr, no_segmentation_fault_at_12_res_64qam)
{
  test_function_64_qam({12});
}

// This is a "normal" segfault because the function assumed extra buffer for reading non-existent REs
TEST(test_llr, no_segmentation_fault_at_36_res_64qam)
{
  test_function_64_qam({36});
}

// any number of REs should work
TEST(test_llr, no_segfault_any_number_of_re_64qam)
{
  for (uint32_t i = 0U; i < 1000U; i++) {
    test_function_64_qam({i});
  }
}

TEST(test_llr, verify_reference_implementation_256qam)
{
  test_function_256_qam({16, 24, 32, 80, 8 * 300});
}

TEST(test_llr, test_8_res_256qam)
{
  test_function_256_qam({8});
}

TEST(test_llr, test_4_res_256qam)
{
  test_function_256_qam({4});
}

// This is a "normal" segfault because the function assumed extra buffer for reading non-existent REs
TEST(test_llr, no_segmentation_fault_at_12_res_256qam)
{
  test_function_256_qam({12});
}

// This is a "normal" segfault because the function assumed extra buffer for reading non-existent REs
TEST(test_llr, no_segmentation_fault_at_36_res_256qam)
{
  test_function_256_qam({36});
}

// any number of REs should work
TEST(test_llr, no_segfault_any_number_of_re_256qam)
{
  for (uint32_t i = 0U; i < 1000U; i++) {
    test_function_256_qam({i});
  }
}

// Inputs stay aligned, the llr output starts this many int16 after a 64-byte boundary, i.e. it is
// misaligned for 16 and 32 byte access so an aligned SIMD store to it segfaults
static const uint32_t unaligned_llr_offsets[] = {1, 2, 3, 4, 5, 6, 7, 9, 10, 14, 18};

// Covers the 256-bit loop, the 128-bit loop (4..7 RE remainder), the scalar tail and a 273 PRB symbol
static AlignedVector512<uint32_t> unaligned_llr_nb_res()
{
  AlignedVector512<uint32_t> nb_res;
  for (uint32_t i = 1U; i <= 40U; i++)
    nb_res.push_back(i);
  nb_res.push_back(273 * 12);
  nb_res.push_back(273 * 12 - 1);
  return nb_res;
}

TEST(test_llr, unaligned_llr_16qam)
{
  for (auto offset : unaligned_llr_offsets)
    test_function_16_qam(unaligned_llr_nb_res(), offset);
}

TEST(test_llr, unaligned_llr_64qam)
{
  for (auto offset : unaligned_llr_offsets)
    test_function_64_qam(unaligned_llr_nb_res(), offset);
}

TEST(test_llr, unaligned_llr_256qam)
{
  for (auto offset : unaligned_llr_offsets)
    test_function_256_qam(unaligned_llr_nb_res(), offset);
}

// It is possible to implement an AVX accelerated llr computation for multiples of 2REs.
// This testcase can be used to verify this implementation as it visualizes LLR data with printfs
TEST(test_llr, check_2_res_256_qam)
{
  AlignedVector512<c16_t> rf_data = {{1, 1}, {2, 2}};
  AlignedVector512<int16_t> magnitude_data = {1, 1, 1, 1};
  AlignedVector512<int16_t> magnitude_b_data = {2, 2, 2, 2};
  AlignedVector512<int16_t> magnitude_c_data = {3, 3, 3, 3};
  AlignedVector512<int16_t> llr_ref;
  llr_ref.resize(2 * 8);
  std::fill(llr_ref.begin(), llr_ref.end(), 0);
  nr_256qam_llr_ref((const c16_t *)rf_data.data(),
                    (const c16_t *)magnitude_data.data(),
                    (const c16_t *)magnitude_b_data.data(),
                    (const c16_t *)magnitude_c_data.data(),
                    (int16_t *)llr_ref.data(),
                    2);

  AlignedVector512<int16_t> llr;
  llr.resize(2 * 8);
  std::fill(llr.begin(), llr.end(), 0);
  nr_256qam_llr((const c16_t *)rf_data.data(),
                (const c16_t *)magnitude_data.data(),
                (const c16_t *)magnitude_b_data.data(),
                (const c16_t *)magnitude_c_data.data(),
                (int16_t *)llr.data(),
                2);

  printf("\nDUT:\n");
  for (auto i = 0U; i < 2; i++) {
    printf("%d %d %d %d %d %d %d %d\n",
           llr[i * 8],
           llr[i * 8 + 1],
           llr[i * 8 + 2],
           llr[i * 8 + 3],
           llr[i * 8 + 4],
           llr[i * 8 + 5],
           llr[i * 8 + 6],
           llr[i * 8 + 7]);
  }

  printf("\nREF:\n");
  for (auto i = 0U; i < 2; i++) {
    printf("%d %d %d %d %d %d %d %d\n",
           llr_ref[i * 8],
           llr_ref[i * 8 + 1],
           llr_ref[i * 8 + 2],
           llr_ref[i * 8 + 3],
           llr_ref[i * 8 + 4],
           llr_ref[i * 8 + 5],
           llr_ref[i * 8 + 6],
           llr_ref[i * 8 + 7]);
  }

  int num_errors = 0;
  for (auto i = 0U; i < 2 * 8; i++) {
    EXPECT_LE(llr_ref[i] - llr[i] ? (llr_ref[i] - llr[i]) / (float)llr[i] : 0, 0.001)
        << "Mismatch 256qam REF " << std::hex << llr_ref[i] << " != DUT " << llr[i] << " at " << std::dec << i
        << "total err: " << ++num_errors;
  }
  EXPECT_EQ(num_errors, 0) << " Errors during testing 256qam llr " << num_errors << " nb res " << 2;
}

// Integer in [lo, hi] from the OAI Tausworthe generator
static int16_t taus_range(int lo, int hi)
{
  return lo + taus() % (hi - lo + 1);
}

// 2-layer ML LLRs are computed independently per RE, so running nr_compute_ML_llr() on nb_re REs must give the
// same LLRs for those REs as running it on nb_re rounded up to 16. Callers pad per-symbol buffers to 16 REs, so
// the LLRs must not be written past that padding.
void test_function_ml_llr(uint8_t mod_order, uint32_t nb_re)
{
  SCOPED_TRACE(::testing::Message() << "mod_order " << int(mod_order) << " nb_re " << nb_re);
  const uint32_t len = (nb_re + 15) & ~15U;
  AlignedVector512<c16_t> comp0(len), comp1(len), mag0(len), mag1(len), rho0(len), rho1(len);
  for (auto i = 0U; i < len; i++) {
    comp0[i] = {taus_range(-2000, 2000), taus_range(-2000, 2000)};
    comp1[i] = {taus_range(-2000, 2000), taus_range(-2000, 2000)};
    const int16_t m0 = taus_range(500, 1500), m1 = taus_range(500, 1500);
    mag0[i] = {m0, m0};
    mag1[i] = {m1, m1};
    rho0[i] = {taus_range(-300, 300), taus_range(-300, 300)};
    rho1[i] = {rho0[i].r, (int16_t)-rho0[i].i};
  }

  auto run = [&](uint32_t n, AlignedVector512<int16_t> &llr0, AlignedVector512<int16_t> &llr1) {
    std::fill(llr0.begin(), llr0.end(), llr_sentinel);
    std::fill(llr1.begin(), llr1.end(), llr_sentinel);
    nr_compute_ML_llr(comp0.data(),
                      comp1.data(),
                      mag0.data(),
                      mag1.data(),
                      llr0.data(),
                      llr1.data(),
                      rho0.data(),
                      rho1.data(),
                      n,
                      mod_order);
  };

  // one extra LLR past the padded length for the sentinel
  const uint32_t nb_llr = len * mod_order + 1;
  AlignedVector512<int16_t> llr0(nb_llr), llr1(nb_llr), ref0(nb_llr), ref1(nb_llr);
  run(len, ref0, ref1);
  run(nb_re, llr0, llr1);
  for (auto k = 0U; k < nb_re * mod_order; k++) {
    ASSERT_EQ(llr0[k], ref0[k]) << "layer 0, RE " << k / mod_order << " bit " << k % mod_order;
    ASSERT_EQ(llr1[k], ref1[k]) << "layer 1, RE " << k / mod_order << " bit " << k % mod_order;
  }
  EXPECT_EQ(llr0[len * mod_order], llr_sentinel) << "ML llr layer 0 written past the padded end";
  EXPECT_EQ(llr1[len * mod_order], llr_sentinel) << "ML llr layer 1 written past the padded end";
}

class test_ml_llr : public ::testing::TestWithParam<uint8_t> {};

// any number of REs should work
TEST_P(test_ml_llr, any_number_of_re)
{
  set_taus_seed(GetParam());
  for (uint32_t nb_re = 1; nb_re <= 96; nb_re++)
    test_function_ml_llr(GetParam(), nb_re);
}

// PUSCH/PDSCH symbol sizes: type-1 DMRS symbol (6 data REs per PRB) and full symbols
TEST_P(test_ml_llr, symbol_sizes)
{
  set_taus_seed(100 + GetParam());
  for (uint32_t nb_rb : {1u, 3u, 7u, 105u, 106u, 107u, 272u, 273u}) {
    test_function_ml_llr(GetParam(), 6 * nb_rb);
    test_function_ml_llr(GetParam(), 12 * nb_rb);
  }
}

INSTANTIATE_TEST_SUITE_P(nr_compute_ML_llr, test_ml_llr, ::testing::Values(2, 4, 6), [](const auto &info) {
  return "qam" + std::to_string(1 << info.param);
});

int main(int argc, char **argv)
{
  logInit();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
