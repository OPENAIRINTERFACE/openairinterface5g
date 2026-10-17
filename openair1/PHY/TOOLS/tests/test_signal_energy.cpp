#include <gtest/gtest.h>
extern "C" {
#include "openair1/PHY/TOOLS/tools_defs.h"
extern uint32_t signal_energy_nodc(const c16_t *input, uint32_t length);
}
#include <vector>
#include <algorithm>

int32_t signal_energy_nodc_ref(const c16_t *input, uint32_t length)
{
  float sum = 0;
  for (auto i = 0U; i < length; i++) {
    sum += input[i].r * input[i].r + input[i].i * input[i].i;
  }
  return sum / length;
}

TEST(signal_energy_nodc, size_6)
{
  std::vector<c16_t> input;
  input.resize(6);
  std::fill(input.begin(), input.end(), (c16_t){42, 42});
  EXPECT_EQ(signal_energy_nodc(input.data(), input.size()),
            signal_energy_nodc_ref(input.data(), input.size()));
}

// samples (a, -a) have re + im = 0, so the DC term vanishes and signal_energy() is the mean power 2 * a^2
TEST(signal_energy, length_not_multiple_of_4)
{
  const double expected = 2 * 42 * 42;
  for (uint32_t length = 1; length <= 16; length++) {
    std::vector<c16_t> input(length, (c16_t){42, -42});
    EXPECT_NEAR(signal_energy((int32_t *)input.data(), length), expected, expected / 100) << "length " << length;
  }
}

TEST(signal_energy, last_samples_counted)
{
  const uint32_t length = 839;
  std::vector<c16_t> input(length, (c16_t){0, 0});
  for (auto i = length - 3; i < length; i++)
    input[i] = (c16_t){1000, -1000};
  const double expected = 3 * 2 * 1000.0 * 1000.0 / length;
  EXPECT_NEAR(signal_energy((int32_t *)input.data(), length), expected, expected / 100);
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
