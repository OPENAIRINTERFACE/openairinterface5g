/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "common/config/config_load_configmodule.h"
#include "common/config/config_userapi.h"
#include "common/utils/assertions.h"
#include "common/utils/ds/byte_array_producer.h"
#include "common/utils/LOG/log.h"
#include "openair1/SIMULATION/TOOLS/sim.h"

#include "lib/nrup_dl_data_delivery_status.h"
#include "lib/nrup_dl_user_data.h"

configmodule_interface_t *uniqCfg = NULL;

#define rand_bool() ((taus() & 1) != 0)
#define rand_sn24() (taus() & NRUP_NR_U_SN_MAX)
#define rand_pdcp_sn() (taus() & NRUP_NR_PDCP_SN_MAX)
#define rand_in_range(min, max) ((min) + taus() % ((max) - (min) + 1))

void exit_function(const char *file, const char *function, const int line, const char *s, const int assert)
{
  UNUSED(assert);
  printf("detected error at %s:%d:%s: %s\n", file, line, function, s);
  abort();
}

static void nrup_dl_user_encdec_test(const nrup_dl_user_data_t *orig)
{
  uint8_t buf[32];
  byte_array_producer_t b = byte_array_producer_from_buffer(buf, sizeof(buf));
  AssertFatal(encode_nrup_dl_user_data(&b, orig) == 1, "encode_nrup_dl_user_data() failed\n");

  nrup_dl_user_data_t decoded;
  AssertFatal(decode_nrup_dl_user_data(buf, b.pos, &decoded), "decode_nrup_dl_user_data() failed\n");
  AssertFatal(eq_nrup_dl_user_data(orig, &decoded), "eq_nrup_dl_user_data(): decoded message doesn't match\n");

  /* TS 38.425 clause 5.5.1: ignore remaining octets */
  buf[b.pos] = 0xa5;
  AssertFatal(decode_nrup_dl_user_data(buf, b.pos + 1, &decoded), "decode_nrup_dl_user_data() failed\n");
  AssertFatal(eq_nrup_dl_user_data(orig, &decoded), "eq_nrup_dl_user_data(): decoded message doesn't match\n");
}

/** @brief DL USER DATA round-trip and Report Delivered truncation rejection (TS 38.425 Figure 5.5.2.1-1)
 * @note Random IEs use taus() */
static void test_dl_user_data(void)
{
  set_taus_seed(0);

  nrup_dl_user_encdec_test(&(const nrup_dl_user_data_t){
      .nru_sequence_number = rand_sn24(),
  });

  nrup_dl_user_data_t discard_msg = {
      .nru_sequence_number = rand_sn24(),
      .dl_discard_blocks_present = true,
      .n_dl_discard_blocks = 3,
  };
  for (uint8_t i = 0; i < discard_msg.n_dl_discard_blocks; i++) {
    discard_msg.dl_discard_blocks[i].dl_discard_nr_pdcp_pdu_sn_start = rand_pdcp_sn();
    discard_msg.dl_discard_blocks[i].discarded_block_size =
        rand_in_range(NRUP_DISCARDED_BLOCK_SIZE_MIN, NRUP_DISCARDED_BLOCK_SIZE_MAX);
  }
  nrup_dl_user_encdec_test(&discard_msg);

  nrup_dl_user_data_t full_msg = {
      .report_polling = rand_bool(),
      .retransmission = rand_bool(),
      .assistance_info_report_polling = rand_bool(),
      .user_data_existence = rand_bool(),
      .request_out_of_seq_report = rand_bool(),
      .nru_sequence_number = rand_sn24(),
  };
  if (rand_bool()) {
    full_msg.dl_flush = true;
    full_msg.dl_discard_nr_pdcp_pdu_sn = rand_pdcp_sn();
    LOG_I(NR_UP, "DL USER DATA random IE enabled: DL Flush\n");
  }
  if (rand_bool()) {
    full_msg.report_delivered = true;
    full_msg.nr_pdcp_pdu_sn = rand_sn24();
    LOG_I(NR_UP, "DL USER DATA random IE enabled: Report Delivered\n");
  }

  nrup_dl_user_encdec_test(&full_msg);

  uint8_t buf[32];
  byte_array_producer_t b = byte_array_producer_from_buffer(buf, sizeof(buf));
  const nrup_dl_user_data_t msg = {.nru_sequence_number = 1};
  AssertFatal(encode_nrup_dl_user_data(&b, &msg) == 1, "encode_nrup_dl_user_data() failed\n");
  AssertFatal(b.pos % 4 == 2, "TS 38.425 clause 5.5.3.24: NR-U PDU length shall be n*4-2\n");
  buf[1] |= 1u << NRUP_DL_USER_DATA_REPORT_DELIVERED; /* Report Delivered flag set without nr_pdcp_pdu_sn in orig */

  nrup_dl_user_data_t decoded;
  /* Report Delivered flag set but DL report NR PDCP PDU SN body missing (TS 38.425 §5.5.3.41-42) */
  LOG_A(NR_UP, "Expected failure: Report Delivered flag without NR PDCP PDU SN body\n");
  AssertFatal(!decode_nrup_dl_user_data(buf, b.pos, &decoded), "decode must reject Report Delivered without NR PDCP PDU SN\n");
}

static void nrup_dl_data_delivery_status_encdec_test(const nrup_dl_data_delivery_status_t *orig)
{
  uint8_t buf[64];
  byte_array_producer_t b = byte_array_producer_from_buffer(buf, sizeof(buf));
  AssertFatal(encode_nrup_dl_data_delivery_status(&b, orig) == 1, "encode_nrup_dl_data_delivery_status() failed\n");

  nrup_dl_data_delivery_status_t decoded;
  AssertFatal(decode_nrup_dl_data_delivery_status(buf, b.pos, &decoded), "decode_nrup_dl_data_delivery_status() failed\n");
  AssertFatal(eq_nrup_dl_data_delivery_status(orig, &decoded),
              "eq_nrup_dl_data_delivery_status(): decoded message doesn't match\n");

  /* TS 38.425 clause 5.5.1: ignore remaining octets */
  buf[b.pos] = 0xa5;
  AssertFatal(decode_nrup_dl_data_delivery_status(buf, b.pos + 1, &decoded), "decode_nrup_dl_data_delivery_status() failed\n");
  AssertFatal(eq_nrup_dl_data_delivery_status(orig, &decoded),
              "eq_nrup_dl_data_delivery_status(): decoded message doesn't match\n");
}

/** @brief DL DATA DELIVERY STATUS round-trip and malformed PDU rejection (TS 38.425 Figure 5.5.2.2-1)
 * @note Random IEs use taus() */
static void test_dl_data_delivery_status(void)
{
  set_taus_seed(0);

  nrup_dl_data_delivery_status_encdec_test(&(const nrup_dl_data_delivery_status_t){
      .desired_buffer_size = taus(),
  });

  /* Desired Buffer Size with the most significant octet >= 0x80, independent of the seed */
  nrup_dl_data_delivery_status_encdec_test(&(const nrup_dl_data_delivery_status_t){
      .desired_buffer_size = 0xdb000000,
  });

  nrup_sn_range_t lost_range = {.start = rand_pdcp_sn(), .end = rand_pdcp_sn()};
  nrup_sn_range_t delivered_range = {.start = rand_pdcp_sn(), .end = rand_pdcp_sn()};

  nrup_dl_data_delivery_status_t full_msg = {
      .final_frame_ind = rand_bool(),
      .desired_buffer_size = taus(),
  };
  if (rand_bool()) {
    full_msg.desired_data_rate_present = true;
    full_msg.desired_data_rate = taus();
    LOG_I(NR_UP, "DDDS random IE enabled: desired data rate\n");
  }
  if (rand_bool()) {
    full_msg.lost_nru_ranges_present = true;
    full_msg.lost_nru_ranges.n_ranges = 1;
    full_msg.lost_nru_ranges.ranges[0] = lost_range;
    LOG_I(NR_UP, "DDDS random IE enabled: lost NR-U SN ranges\n");
  }
  if (rand_bool()) {
    full_msg.highest_delivered_nr_pdcp_sn_present = true;
    full_msg.highest_delivered_nr_pdcp_sn = rand_pdcp_sn();
    LOG_I(NR_UP, "DDDS random IE enabled: highest delivered NR PDCP SN\n");
  }
  if (rand_bool()) {
    full_msg.highest_transmitted_nr_pdcp_sn_present = true;
    full_msg.highest_transmitted_nr_pdcp_sn = rand_pdcp_sn();
    LOG_I(NR_UP, "DDDS random IE enabled: highest transmitted NR PDCP SN\n");
  }
  if (rand_bool()) {
    full_msg.cause_value_present = true;
    full_msg.cause_value = taus() & 0xff;
    LOG_I(NR_UP, "DDDS random IE enabled: cause value\n");
  }
  if (rand_bool()) {
    full_msg.delivered_retransmitted_nr_pdcp_sn_present = true;
    full_msg.delivered_retransmitted_nr_pdcp_sn = rand_pdcp_sn();
    LOG_I(NR_UP, "DDDS random IE enabled: delivered retransmitted NR PDCP SN\n");
  }
  if (rand_bool()) {
    full_msg.retransmitted_nr_pdcp_sn_present = true;
    full_msg.retransmitted_nr_pdcp_sn = rand_pdcp_sn();
    LOG_I(NR_UP, "DDDS random IE enabled: retransmitted NR PDCP SN\n");
  }
  if (rand_bool()) {
    full_msg.delivered_oos_ranges_present = true;
    full_msg.delivered_oos_ranges.n_ranges = 1;
    full_msg.delivered_oos_ranges.ranges[0] = delivered_range;
    LOG_I(NR_UP, "DDDS random IE enabled: delivered out-of-sequence NR PDCP SN ranges\n");
  }

  nrup_dl_data_delivery_status_encdec_test(&full_msg);

  uint8_t buf[64];
  byte_array_producer_t b = byte_array_producer_from_buffer(buf, sizeof(buf));
  const nrup_dl_data_delivery_status_t msg = {.desired_buffer_size = 1};
  AssertFatal(encode_nrup_dl_data_delivery_status(&b, &msg) == 1, "encode_nrup_dl_data_delivery_status() failed\n");
  AssertFatal(b.pos % 4 == 2, "TS 38.425 clause 5.5.3.24: NR-U PDU length shall be n*4-2\n");
  buf[0] |= 1u << NRUP_DDDS_HIGHEST_TRANSMITTED; /* flag set without highest_transmitted_nr_pdcp_sn in orig */

  nrup_dl_data_delivery_status_t decoded;
  LOG_A(NR_UP, "Expected failure: Highest Transmitted flag without NR PDCP PDU SN body\n");
  AssertFatal(!decode_nrup_dl_data_delivery_status(buf, b.pos, &decoded),
              "decode must reject Highest Transmitted without NR PDCP PDU SN\n");

  b = byte_array_producer_from_buffer(buf, sizeof(buf));
  AssertFatal(encode_nrup_dl_data_delivery_status(&b, &msg) == 1, "encode_nrup_dl_data_delivery_status() failed\n");
  buf[0] = (buf[0] & 0x0f) | (NRUP_PDU_DL_USER_DATA << NRUP_DDDS_PDU_TYPE_SHIFT);

  LOG_A(NR_UP, "Expected failure: invalid DL DATA DELIVERY STATUS PDU type\n");
  AssertFatal(!decode_nrup_dl_data_delivery_status(buf, b.pos, &decoded), "decode must reject wrong PDU type\n");
}

/** 125k iterations */
#define NRUP_BENCH_ITERATIONS 125000u

static void nrup_bench_print(uint32_t pdu_bytes, const struct timespec *start, const struct timespec *end)
{
  const double elapsed_ns = (end->tv_sec - start->tv_sec) * 1e9 + (end->tv_nsec - start->tv_nsec);
  const double ns_per_cycle = elapsed_ns / NRUP_BENCH_ITERATIONS;

  printf("pdu_bytes=%u elapsed=%.2f ms ns/cycle=%.1f\n", pdu_bytes, elapsed_ns / 1e6, ns_per_cycle);
}

static void bench_dl_user_data(const nrup_dl_user_data_t *orig)
{
  uint8_t buf[32];
  byte_array_producer_t probe = byte_array_producer_from_buffer(buf, sizeof(buf));
  AssertFatal(encode_nrup_dl_user_data(&probe, orig) == 1, "encode_nrup_dl_user_data() failed\n");
  const uint32_t pdu_bytes = probe.pos;

  nrup_dl_user_data_t decoded;
  struct timespec start;
  struct timespec end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  for (uint32_t i = 0; i < NRUP_BENCH_ITERATIONS; i++) {
    byte_array_producer_t b = byte_array_producer_from_buffer(buf, sizeof(buf));
    AssertFatal(encode_nrup_dl_user_data(&b, orig) == 1, "encode_nrup_dl_user_data() failed\n");
    AssertFatal(decode_nrup_dl_user_data(buf, b.pos, &decoded), "decode_nrup_dl_user_data() failed\n");
  }
  clock_gettime(CLOCK_MONOTONIC, &end);
  nrup_bench_print(pdu_bytes, &start, &end);
}

static void bench_ddds(const nrup_dl_data_delivery_status_t *orig)
{
  uint8_t buf[64];
  byte_array_producer_t probe = byte_array_producer_from_buffer(buf, sizeof(buf));
  AssertFatal(encode_nrup_dl_data_delivery_status(&probe, orig) == 1, "encode_nrup_dl_data_delivery_status() failed\n");
  const uint32_t pdu_bytes = probe.pos;

  nrup_dl_data_delivery_status_t decoded;
  struct timespec start;
  struct timespec end;
  clock_gettime(CLOCK_MONOTONIC, &start);
  for (uint32_t i = 0; i < NRUP_BENCH_ITERATIONS; i++) {
    byte_array_producer_t b = byte_array_producer_from_buffer(buf, sizeof(buf));
    AssertFatal(encode_nrup_dl_data_delivery_status(&b, orig) == 1, "encode_nrup_dl_data_delivery_status() failed\n");
    AssertFatal(decode_nrup_dl_data_delivery_status(buf, b.pos, &decoded), "decode_nrup_dl_data_delivery_status() failed\n");
  }
  clock_gettime(CLOCK_MONOTONIC, &end);
  nrup_bench_print(pdu_bytes, &start, &end);
}

static void run_nrup_benchmark(void)
{
  printf("nrup_lib bench: %u enc/dec iterations per scenario\n", NRUP_BENCH_ITERATIONS);

  printf("dl_user_data_minimal\n");
  bench_dl_user_data(&(const nrup_dl_user_data_t){.nru_sequence_number = 1});

  printf("dl_user_data_report_delivered\n");
  bench_dl_user_data(&(const nrup_dl_user_data_t){
      .nru_sequence_number = 1,
      .report_delivered = true,
      .nr_pdcp_pdu_sn = 42,
  });

  printf("ddds_minimal\n");
  bench_ddds(&(const nrup_dl_data_delivery_status_t){.desired_buffer_size = 4096});

  printf("ddds_oai_tx\n");
  bench_ddds(&(const nrup_dl_data_delivery_status_t){
      .desired_buffer_size = 4096,
      .highest_transmitted_nr_pdcp_sn_present = true,
      .highest_transmitted_nr_pdcp_sn = 99,
  });
}

int main(int argc, char **argv)
{
  uniqCfg = load_configmodule(argc, argv, CONFIG_ENABLECMDLINEONLY);
  AssertFatal(uniqCfg != NULL, "configuration module init failed\n");
  logInit();
  set_log(NR_UP, OAILOG_INFO);
  test_dl_user_data();
  test_dl_data_delivery_status();
  run_nrup_benchmark();
  return 0;
}
