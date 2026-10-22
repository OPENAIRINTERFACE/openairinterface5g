/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

/* gNB MAC unit tests. The MAC headers are not C++ compatible, so each test is a C function
 * (test_*.c) called from a GoogleTest case in a C++ file of the same name (test_*.cpp).
 * The MAC (and RLC) can be initialized only once per process, so each test runs in its own
 * process (gtest_discover_tests). */

#include "gtest/gtest.h"

extern "C" {
#include "common/config/config_load_configmodule.h"
#include "common/utils/assertions.h"

void nr_mac_gnb_test_init(void);
extern configmodule_interface_t *uniqCfg;
}

int main(int argc, char **argv)
{
  uniqCfg = load_configmodule(argc, argv, CONFIG_ENABLECMDLINEONLY);
  AssertFatal(uniqCfg != NULL, "configuration module init failed\n");
  nr_mac_gnb_test_init();
  testing::InitGoogleTest(&argc, argv);
  int ret = RUN_ALL_TESTS();
  end_configmodule(uniqCfg);
  return ret;
}
