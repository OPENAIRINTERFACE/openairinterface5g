/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include <cstdint>
#include "gtest/gtest.h"

// returns the number of other SI DCIs checked, or -1 on a wrong CORESET0
extern "C" int test_other_sib_ssb_index(uint64_t ssb_bitmap);

// parameter: ssb_PositionsInBurst bitmap, bit i = SSB index i (e.g. 0x2: only SSB index 1)
class OtherSibSsbIndex : public testing::TestWithParam<uint64_t> {};

TEST_P(OtherSibSsbIndex, OtherSiInCoreset0)
{
  EXPECT_GT(test_other_sib_ssb_index(GetParam()), 0);
}

// 0x1: SSB index 0, 0x2: index 1, 0x6: index 1 and 2
INSTANTIATE_TEST_SUITE_P(SsbBitmap, OtherSibSsbIndex, testing::Values(0x1, 0x2, 0x6), testing::PrintToStringParamName());
