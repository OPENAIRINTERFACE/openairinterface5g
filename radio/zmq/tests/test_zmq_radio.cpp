/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "common_lib.h"
#include <gtest/gtest.h>
#include "common/config/config_userapi.h"
#include <zmq.h>
extern "C" {
#include "common/config/config_userapi.h"
#include "openair1/SIMULATION/TOOLS/sim.h"
extern int device_init(openair0_device_t *device, openair0_config_t *openair0_cfg);
static softmodem_params_t softmodem_params;
softmodem_params_t *get_softmodem_params(void)
{
  return &softmodem_params;
}
}
#include "common/platform_types.h"
#include <thread>
#include <vector>
#include "../zmq_simd.h"

configmodule_interface_t *uniqCfg = NULL;

extern "C" void exit_function(const char *file, const char *function, const int line, const char *s, const int assert)
{
  fprintf(stderr, "FATAL: %s at %s:%s:%d\n", s, file, function, line);
  exit(EXIT_FAILURE);
}

class ZMQTest : public ::testing::Test {
 protected:
  configmodule_interface_t *cfg1 = nullptr;
  configmodule_interface_t *cfg2 = nullptr;
  openair0_device_t device1 = {0};
  openair0_device_t device2 = {0};
  openair0_config_t config1 = {0};
  openair0_config_t config2 = {0};
  char *argv[4];

  void SetUp() override
  {
    argv[0] = strdup("--zmq.[0].tx_channels");
    argv[1] = strdup("tcp://127.0.0.1:5555");
    argv[2] = strdup("--zmq.[0].rx_channels");
    argv[3] = strdup("tcp://127.0.0.1:5556");

    cfg1 = load_configmodule(sizeofArray(argv), argv, CONFIG_ENABLECMDLINEONLY);
    uniqCfg = cfg1;
    // 2. Initialize the ZMQ device
    config1.tx_num_channels = 1;
    config1.rx_num_channels = 1;
    config1.sample_rate = 30;
    ASSERT_EQ(device_init(&device1, &config1), 0);
    ASSERT_EQ(device1.trx_start_func(&device1), 0);

    // Swap the RX with TX for second device
    char *tmp = argv[0];
    argv[0] = argv[2];
    argv[2] = tmp;
    cfg2 = load_configmodule(sizeofArray(argv), argv, CONFIG_ENABLECMDLINEONLY);
    uniqCfg = cfg2;
    config2.tx_num_channels = 1;
    config2.rx_num_channels = 1;
    config2.sample_rate = 1500;
    ASSERT_EQ(device_init(&device2, &config2), 0);
    ASSERT_EQ(device2.trx_start_func(&device2), 0);
  }

  void TearDown() override
  {
    if (device1.trx_end_func) {
      device1.trx_end_func(&device1);
    }
    if (device2.trx_end_func) {
      device2.trx_end_func(&device2);
    }
    if (cfg1) {
      end_configmodule(cfg1);
    }
    if (cfg2) {
      end_configmodule(cfg2);
    }
    for (auto i = 0U; i < sizeofArray(argv); i++) {
      free(argv[i]);
    }
  }
};

TEST_F(ZMQTest, RXSamples)
{
  std::thread t1([this]() {
    c16_t rx_samples[10];
    openair0_timestamp_t rx_timestamp;
    void *samples[1] = {rx_samples};
    ASSERT_EQ(device1.trx_read_func(&device1, &rx_timestamp, samples, 10, 1), 10);
    for (int i = 0; i < 10; i++) {
      ASSERT_EQ(rx_samples[i].r, 0);
      ASSERT_EQ(rx_samples[i].i, 0);
    }
  });
  std::thread t2([this]() {
    c16_t rx_samples[10];
    openair0_timestamp_t rx_timestamp;
    void *samples[1] = {rx_samples};
    ASSERT_EQ(device2.trx_read_func(&device2, &rx_timestamp, samples, 10, 1), 10);
    for (int i = 0; i < 10; i++) {
      ASSERT_EQ(rx_samples[i].r, 0);
      ASSERT_EQ(rx_samples[i].i, 0);
    }
  });
  t1.join();
  t2.join();
}

TEST_F(ZMQTest, TxRxSamples)
{
  std::thread t1([this]() {
    c16_t rx_samples[10];
    openair0_timestamp_t rx_timestamp;
    void *samples[1] = {rx_samples};
    ASSERT_EQ(device1.trx_read_func(&device1, &rx_timestamp, samples, 10, 1), 10);
    for (int i = 0; i < 10; i++) {
      ASSERT_EQ(rx_samples[i].r, 0);
      ASSERT_EQ(rx_samples[i].i, 0);
    }
    c16_t tx_samples[10];
    openair0_timestamp_t tx_timestamp = rx_timestamp + 10;
    for (int i = 0; i < 10; i++) {
      tx_samples[i].r = i;
      tx_samples[i].i = i + 1;
    }
    samples[0] = tx_samples;
    ASSERT_EQ(device1.trx_write_func(&device1, tx_timestamp, samples, 10, 1, 0), 10);
  });
  std::thread t2([this]() {
    c16_t rx_samples[10];
    openair0_timestamp_t rx_timestamp;
    void *samples[1] = {rx_samples};
    ASSERT_EQ(device2.trx_read_func(&device2, &rx_timestamp, samples, 10, 1), 10);
    for (int i = 0; i < 10; i++) {
      ASSERT_EQ(rx_samples[i].r, 0);
      ASSERT_EQ(rx_samples[i].i, 0);
    }
    openair0_timestamp_t rx_timestamp2;
    ASSERT_EQ(device2.trx_read_func(&device2, &rx_timestamp2, samples, 10, 1), 10);
    for (int i = 0; i < 10; i++) {
      ASSERT_EQ(rx_samples[i].r, i);
      ASSERT_EQ(rx_samples[i].i, i + 1);
    }
    ASSERT_EQ(rx_timestamp + 10, rx_timestamp2);
  });
  t1.join();
  t2.join();
}

TEST_F(ZMQTest, TxRxSamplesSIMD)
{
  const int size = 100;
  std::thread t1([this, size]() {
    c16_t rx_samples[size];
    openair0_timestamp_t rx_timestamp;
    void *samples[1] = {rx_samples};
    ASSERT_EQ(device1.trx_read_func(&device1, &rx_timestamp, samples, size, 1), size);
    for (int i = 0; i < size; i++) {
      ASSERT_EQ(rx_samples[i].r, 0);
      ASSERT_EQ(rx_samples[i].i, 0);
    }
    c16_t tx_samples[size];
    openair0_timestamp_t tx_timestamp = rx_timestamp + size;
    for (int i = 0; i < size; i++) {
      tx_samples[i].r = (int16_t)(i - size / 2);
      tx_samples[i].i = (int16_t)(size / 2 - i);
    }
    samples[0] = tx_samples;
    ASSERT_EQ(device1.trx_write_func(&device1, tx_timestamp, samples, size, 1, 0), size);
  });
  std::thread t2([this, size]() {
    c16_t rx_samples[size];
    openair0_timestamp_t rx_timestamp;
    void *samples[1] = {rx_samples};
    ASSERT_EQ(device2.trx_read_func(&device2, &rx_timestamp, samples, size, 1), size);
    for (int i = 0; i < size; i++) {
      ASSERT_EQ(rx_samples[i].r, 0);
      ASSERT_EQ(rx_samples[i].i, 0);
    }
    openair0_timestamp_t rx_timestamp2;
    ASSERT_EQ(device2.trx_read_func(&device2, &rx_timestamp2, samples, size, 1), size);
    for (int i = 0; i < size; i++) {
      ASSERT_EQ(rx_samples[i].r, (int16_t)(i - size / 2));
      ASSERT_EQ(rx_samples[i].i, (int16_t)(size / 2 - i));
    }
    ASSERT_EQ(rx_timestamp + size, rx_timestamp2);
  });
  t1.join();
  t2.join();
}

TEST(ZMQConvert, RoundTripAllValues)
{
  // Odd length so both the SIMD body and the scalar tail are exercised
  const size_t n = 65536 + 7;
  std::vector<int16_t> in(n), out(n);
  std::vector<float> f(n);
  for (size_t i = 0; i < n; i++)
    in[i] = (int16_t)(i - 32768);
  convert_samples_avx512_tx(f.data(), in.data(), n, c16_t_to_cf_t_factor);
  convert_samples_avx512_rx(f.data(), out.data(), n, c16_t_to_cf_t_factor);
  for (size_t i = 0; i < n; i++)
    ASSERT_EQ(in[i], out[i]) << "index " << i;
}

TEST(ZMQConvert, ScalarTailRoundsAndSaturates)
{
  const float in[] = {-1.3f, -1.7f, -0.4f, -2.0f, 1.3f, 1.7f, 2.0f, 40000.0f, -40000.0f};
  const int16_t expected[] = {-1, -2, 0, -2, 1, 2, 2, 32767, -32768};
  const size_t n = sizeof(in) / sizeof(in[0]);
  int16_t out[n];
  convert_samples_avx512_rx(in, out, n, 1.0f);
  for (size_t i = 0; i < n; i++)
    EXPECT_EQ(out[i], expected[i]) << "input " << in[i];
}

TEST_F(ZMQTest, BenchmarkThroughput)
{
  int level = g_log->log_component[HW].level;
  g_log->log_component[HW].level = OAILOG_ERR;
  const size_t nsamps = 10000;
  const size_t num_iters = 100000;
  c16_t tx_samples[nsamps];
  for (size_t i = 0; i < nsamps; i++) {
    tx_samples[i].r = i;
    tx_samples[i].i = i + 1;
  }

  void *samples[1] = {tx_samples};
  openair0_timestamp_t tx_timestamp = 0;

  auto start = std::chrono::high_resolution_clock::now();

  for (size_t i = 0; i < num_iters; i++) {
    ASSERT_EQ(device1.trx_write_func(&device1, tx_timestamp, samples, nsamps, 1, 0), nsamps);
    tx_timestamp += nsamps;
  }

  auto end = std::chrono::high_resolution_clock::now();
  std::chrono::duration<double> diff = end - start;

  std::cout << "Benchmark:" << std::endl;
  std::cout << "Time taken: " << diff.count() << " s\n";
  std::cout << "Throughput: " << (nsamps * num_iters) / diff.count() / 1e6 << " MSamples/s\n";

  // No need to drain messages as device shutdown handles cleanup
  g_log->log_component[HW].level = level;
}

int main(int argc, char **argv)
{
  logInit();
  g_log->log_component[HW].level = OAILOG_DEBUG;
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
