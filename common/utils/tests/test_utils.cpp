/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include <gtest/gtest.h>
#include "utils.h"

TEST(read_version, full)
{
  uint8_t major = 0, minor = 0, patch = 0;
  EXPECT_EQ(read_version("17.3.0", &major, &minor, &patch), 3);
  EXPECT_EQ(major, 17);
  EXPECT_EQ(minor, 3);
  EXPECT_EQ(patch, 0);
}

TEST(read_version, partial)
{
  uint8_t major = 0, minor = 0, patch = 0xaa;
  EXPECT_EQ(read_version("17.3", &major, &minor, &patch), 2);
  EXPECT_EQ(major, 17);
  EXPECT_EQ(minor, 3);
  EXPECT_EQ(patch, 0xaa);
}

TEST(read_version, no_match)
{
  uint8_t major = 0xaa, minor = 0xaa, patch = 0xaa;
  EXPECT_EQ(read_version("abc", &major, &minor, &patch), 0);
  EXPECT_EQ(major, 0xaa);
}

TEST(read_version, empty)
{
  uint8_t major = 0xaa, minor = 0xaa, patch = 0xaa;
  EXPECT_EQ(read_version("", &major, &minor, &patch), 0);
  EXPECT_EQ(read_version("   ", &major, &minor, &patch), 0);
  EXPECT_EQ(major, 0xaa);
  EXPECT_EQ(minor, 0xaa);
  EXPECT_EQ(patch, 0xaa);
}

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
