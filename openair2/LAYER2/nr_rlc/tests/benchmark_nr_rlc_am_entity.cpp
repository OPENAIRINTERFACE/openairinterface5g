/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */
#include "benchmark/benchmark.h"
extern "C" {
#include "common/platform_types.h"
#include "openair2/LAYER2/nr_rlc/nr_rlc_entity_am.h"
#include "openair2/LAYER2/nr_rlc/nr_rlc_entity.h"
#include "common/utils/LOG/log.h"
#include "common/config/config_load_configmodule.h"
extern configmodule_interface_t *uniqCfg;
}
#include <iostream>

void deliver_sdu(void *deliver_sdu_data, nr_rlc_entity_t *entity, char *buf, int size)
{
  UNUSED(deliver_sdu_data);
  UNUSED(entity);
  UNUSED(buf);
  UNUSED(size);
}
void sdu_successful_delivery(void *sdu_successful_delivery_data, nr_rlc_entity_t *entity, int sdu_id)
{
  UNUSED(sdu_successful_delivery_data);
  UNUSED(entity);
  UNUSED(sdu_id);
}
void max_retx_reached(void *max_retx_reached_data, nr_rlc_entity_t *entity)
{
  UNUSED(max_retx_reached_data);
  UNUSED(entity);
}

static void BM_nr_rlc_am_entity(benchmark::State &state)
{
  nr_rlc_entity_t *tx_entity = new_nr_rlc_entity_am(10000,
                                                    10000,
                                                    deliver_sdu,
                                                    NULL,
                                                    sdu_successful_delivery,
                                                    NULL,
                                                    max_retx_reached,
                                                    NULL,
                                                    5,
                                                    35,
                                                    0,
                                                    4,
                                                    -1,
                                                    8,
                                                    12);

  nr_rlc_entity_t *rx_entity = new_nr_rlc_entity_am(10000,
                                                    10000,
                                                    deliver_sdu,
                                                    NULL,
                                                    sdu_successful_delivery,
                                                    NULL,
                                                    max_retx_reached,
                                                    NULL,
                                                    5,
                                                    35,
                                                    0,
                                                    4,
                                                    -1,
                                                    8,
                                                    12);

  char message[] = "Message to the other RLC AM entity.";
  for (auto _ : state) {
    char pdu_buf[40];
    for (int i = 0; i < state.range(0); i++) {
      tx_entity->recv_sdu(tx_entity, message, sizeof(message), i);
      int size = tx_entity->generate_pdu(tx_entity, pdu_buf, sizeof(pdu_buf));
      if (size != 0) {
        // 10% packet loss
        if (i % 10 != 0)
          rx_entity->recv_pdu(rx_entity, pdu_buf, size);
      }
      // Allow the lists to build up before sending messages in the other direction
      if (i % 100 == 0) {
        size = rx_entity->generate_pdu(rx_entity, pdu_buf, sizeof(pdu_buf));
        if (size != 0) {
          // No packet lost in the other direction
          tx_entity->recv_pdu(tx_entity, pdu_buf, size);
        }
      }
      tx_entity->set_time(tx_entity, i + 1);
      rx_entity->set_time(rx_entity, i + 1);
    }
  }
  tx_entity->delete_entity(tx_entity);
  rx_entity->delete_entity(rx_entity);
}

BENCHMARK(BM_nr_rlc_am_entity)->RangeMultiplier(4)->Range(100, 20000);

int main(int argc, char **argv)
{
  logInit();
  uniqCfg = load_configmodule(argc, argv, CONFIG_ENABLECMDLINEONLY);
  g_log->log_component[RLC].level = -1;
  benchmark::Initialize(&argc, argv);
  benchmark::RunSpecifiedBenchmarks();
  return 0;
}
