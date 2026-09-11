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

void test_function_16_qam(AlignedVector512<uint32_t> nb_res)
{
  for (auto i = 0U; i < nb_res.size(); i++) {
    uint32_t nb_re = nb_res[i];
    auto rf_data = generate_random_c16(nb_re);
    auto magnitude_data = generate_random_uint16(nb_re * 2);
    AlignedVector512<int16_t> llr_ref;
    llr_ref.resize(nb_re * 4);
    std::fill(llr_ref.begin(), llr_ref.end(), 0);
    nr_16qam_llr_ref((const c16_t *)rf_data.data(), (const c16_t *)magnitude_data.data(), (int16_t *)llr_ref.data(), nb_re);

    AlignedVector512<int16_t> llr;
    llr.resize(nb_re * 4);
    std::fill(llr.begin(), llr.end(), 0);
    nr_16qam_llr((const c16_t *)rf_data.data(), (const c16_t *)magnitude_data.data(), (int16_t *)llr.data(), nb_re);

    int num_errors = 0;
    for (auto i = 0U; i < llr_ref.size(); i++) {
      EXPECT_LE(llr_ref[i] - llr[i] ? (llr_ref[i] - llr[i]) / (float)llr[i] : 0, 0.001)
          << "Mismatch 16qam REF " << std::hex << llr_ref[i] << " != DUT " << llr[i] << " at " << std::dec << i
          << "total err: " << ++num_errors;
    }
    EXPECT_EQ(num_errors, 0) << " Errors during testing 16qam llr " << num_errors << " nb res " << nb_re;
  }
}

void test_function_64_qam(AlignedVector512<uint32_t> nb_res)
{
  for (auto i = 0U; i < nb_res.size(); i++) {
    uint32_t nb_re = nb_res[i];
    auto rf_data = generate_random_c16(nb_re);
    auto magnitude_data = generate_random_uint16(nb_re * 2);
    auto magnitude_b_data = generate_random_uint16(nb_re * 2);
    AlignedVector512<int16_t> llr_ref;
    llr_ref.resize(nb_re * 6);
    std::fill(llr_ref.begin(), llr_ref.end(), 0);
    nr_64qam_llr_ref((const c16_t *)rf_data.data(),
                     (const c16_t *)magnitude_data.data(),
                     (const c16_t *)magnitude_b_data.data(),
                     (int16_t *)llr_ref.data(),
                     nb_re);

    AlignedVector512<int16_t> llr;
    llr.resize(nb_re * 6);
    std::fill(llr.begin(), llr.end(), 0);
    nr_64qam_llr((const c16_t *)rf_data.data(),
                 (const c16_t *)magnitude_data.data(),
                 (const c16_t *)magnitude_b_data.data(),
                 (int16_t *)llr.data(),
                 nb_re);

    int num_errors = 0;
    for (auto i = 0U; i < llr.size(); i++) {
      EXPECT_LE(llr_ref[i] - llr[i] ? (llr_ref[i] - llr[i]) / (float)llr[i] : 0, 0.001)
          << "Mismatch 64qam REF " << std::hex << llr_ref[i] << " != DUT " << llr[i] << " at " << std::dec << i
          << "total err: " << ++num_errors;
    }
    EXPECT_EQ(num_errors, 0) << " Errors during testing 64qam llr " << num_errors << " nb res " << nb_re;
  }
}

void test_function_256_qam(AlignedVector512<uint32_t> nb_res)
{
  for (auto i = 0U; i < nb_res.size(); i++) {
    uint32_t nb_re = nb_res[i];
    auto rf_data = generate_random_c16(nb_re);
    auto magnitude_data = generate_random_uint16(nb_re * 2);
    auto magnitude_b_data = generate_random_uint16(nb_re * 2);
    auto magnitude_c_data = generate_random_uint16(nb_re * 2);
    AlignedVector512<int16_t> llr_ref;
    llr_ref.resize(nb_re * 8);
    std::fill(llr_ref.begin(), llr_ref.end(), 0);
    nr_256qam_llr_ref((const c16_t *)rf_data.data(),
                      (const c16_t *)magnitude_data.data(),
                      (const c16_t *)magnitude_b_data.data(),
                      (const c16_t *)magnitude_c_data.data(),
                      (int16_t *)llr_ref.data(),
                      nb_re);

    AlignedVector512<int16_t> llr;
    llr.resize(nb_re * 8);
    std::fill(llr.begin(), llr.end(), 0);
    nr_256qam_llr((const c16_t *)rf_data.data(),
                  (const c16_t *)magnitude_data.data(),
                  (const c16_t *)magnitude_b_data.data(),
                  (const c16_t *)magnitude_c_data.data(),
                  (int16_t *)llr.data(),
                  nb_re);

    int num_errors = 0;
    for (auto i = 0U; i < llr.size(); i++) {
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

int main(int argc, char **argv)
{
  logInit();
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
