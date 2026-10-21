/*
 * SPDX-License-Identifier: LicenseRef-CSSL-1.0
 */

#include "PHY/TOOLS/tools_defs.h"
#include "PHY/defs_common.h"
#include "common/platform_types.h"
#include "softmodem-common.h"
#include "utils.h"
#include <chrono>
#include <cstdint>
#include <limits>
#include <stddef.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <sys/epoll.h>
#include <netdb.h>

#include <common/utils/assertions.h>
#include <common/utils/LOG/log.h>
#include <common/config/config_userapi.h>
#include "common_lib.h"
#include <queue>
#include <mutex>
#include <vector>
#include <sstream>
#include <algorithm>
#include <numeric>
#include <thread>
#include <atomic>
#include <condition_variable>
#include <ring_buffer.h>
#include <zmq.h>
#include "zmq_imported.h"
#include "zmq_simd.h"

#define ZMQ_SECTION "zmq"
#define ZMQ_TX_CHANNELS "tx_channels"
#define ZMQ_RX_CHANNELS "rx_channels"

#define ZMQ_PARAMS_DESC                                                                                                            \
  {                                                                                                                                \
      STRINGLISTPARAM(ZMQ_TX_CHANNELS, "list of zmq addresses represeting tx channels_\n", PARAMFLAG_MANDATORY, nullptr, nullptr), \
      STRINGLISTPARAM(ZMQ_RX_CHANNELS, "list of zmq addresses represeting rx channels_\n", PARAMFLAG_MANDATORY, nullptr, nullptr), \
  };

const size_t sample_size = sizeof(cf_t);
const size_t rx_buffer_size = sample_size * 300000;

struct zmq_state_t {
  void *context;
  zmq_tx_stream tx_stream;
  zmq_rx_stream rx_stream;
  std::vector<std::thread> tx_poll_threads;
  std::vector<std::thread> rx_poll_threads;
  std::atomic<bool> poll_thread_running;
  bool stopped = false;
  double sample_rate;
};

static void tx_poll_thread(zmq_tx_channel *chan, size_t i, std::atomic<bool> *poll_thread_running)
{
  zmq_pollitem_t item = {chan->socket_, 0, ZMQ_POLLIN, 0};

  while (*poll_thread_running) {
    // Wait for a REQ from the peer.
    int rc = zmq_poll(&item, 1, 10); // 10ms timeout
    if (rc < 0) {
      if (errno == EINTR)
        continue;
      LOG_E(HW, "[ZMQ] tx_poll_thread zmq_poll failed for TX antenna %d: %s\n", (int)i, zmq_strerror(errno));
      break;
    }
    if (rc == 0 || !(item.revents & ZMQ_POLLIN)) {
      continue; // timeout, no request
    }
    char dummy;
    rc = zmq_recv(chan->socket_, &dummy, 1, 0);
    if (rc < 0) {
      LOG_E(HW, "[ZMQ] tx_poll_thread zmq_recv for TX antenna %d failed: %s\n", (int)i, zmq_strerror(errno));
      continue;
    }

    // Got a REQ: block until a message is ready (woken by queue_cvar_ the moment
    // transmit()/align() enqueues one), then reply. Replaces re-checking the queue
    // only on the 10ms poll timeout, which stalled every slot the PHY was briefly
    // behind the peer's prefetched request.
    zmq_msg_t msg;
    while (*poll_thread_running) {
      if (chan->wait_and_pop_message(&msg, std::chrono::milliseconds(10), poll_thread_running)) {
        rc = zmq_msg_send(&msg, chan->socket_, 0);
        if (rc < 0) {
          LOG_E(HW, "[ZMQ] tx_poll_thread zmq_msg_send for TX antenna %d failed: %s\n", (int)i, zmq_strerror(errno));
        }
        zmq_msg_close(&msg);
        break;
      }
    }
  }
}

static void rx_poll_thread(zmq_rx_channel *chan, size_t i, std::atomic<bool> *poll_thread_running)
{
  unsigned char *rx_buffer = static_cast<unsigned char *>(malloc(rx_buffer_size));
  c16_t *rx_buffer_c16 = static_cast<c16_t *>(malloc(rx_buffer_size / sizeof(cf_t) * sizeof(c16_t)));
  zmq_pollitem_t item = {chan->socket_, 0, ZMQ_POLLIN, 0};

  while (*poll_thread_running) {
    int rc = zmq_poll(&item, 1, 10); // 10ms timeout
    if (rc < 0) {
      if (errno == EINTR)
        continue;
      LOG_E(HW, "[ZMQ] rx_poll_thread zmq_poll failed for RX antenna %d: %s\n", (int)i, zmq_strerror(errno));
      break;
    }
    if (rc == 0) {
      continue; // timeout
    }

    if (item.revents & ZMQ_POLLIN) {
      rc = zmq_recv(chan->socket_, rx_buffer, rx_buffer_size, 0);
      if (rc < 0) {
        LOG_E(HW, "[ZMQ] rx_poll_thread zmq_recv for RX antenna %d failed: %s\n", (int)i, zmq_strerror(errno));
      } else {
        size_t received_bytes = rc;
        if (rx_buffer_size < received_bytes) {
          LOG_W(HW,
                "[ZMQ] the RX buffer is too small! The received message size is %lu while the buffer is %lu. Message truncated\n",
                received_bytes,
                rx_buffer_size);
        }
        size_t num_samples_received = std::min(received_bytes, rx_buffer_size) / sizeof(cf_t);
        cf_t *samples = reinterpret_cast<cf_t *>(rx_buffer);
        convert_samples_avx512_rx(reinterpret_cast<const float *>(samples),
                                  reinterpret_cast<int16_t *>(rx_buffer_c16),
                                  num_samples_received * 2,
                                  c16_t_to_cf_t_factor);
        size_t overflow = chan->buffer_.push_samples(rx_buffer_c16, num_samples_received);
        if (rx_buffer_size < received_bytes) {
          overflow += chan->buffer_.push_zeros((received_bytes - rx_buffer_size) / sizeof(cf_t));
        }
        if (overflow) {
          LOG_W(HW, "Overflow on receive\n");
        }
        // After receiving, send next request to keep the stream flowing
        char dummy = 0;
        if (zmq_send(chan->socket_, &dummy, 1, 0) != 1) {
          LOG_E(HW, "[ZMQ] rx_poll_thread zmq_send for RX antenna %d failed: %s\n", (int)i, zmq_strerror(errno));
        }
      }
    }
  }
  free(rx_buffer);
  free(rx_buffer_c16);
}

static int zmq_write(openair0_device_t *device, openair0_timestamp_t timestamp, void **buff, int nsamps, int cc, int flags)
{
  zmq_state_t *s = static_cast<zmq_state_t *>(device->priv);
  AssertFatal((uint)cc == s->tx_stream.channels_.size(),
              "Request to write on more antennas (%d) than configured (%d)",
              cc,
              (int)s->tx_stream.channels_.size());

  s->tx_stream.transmit((c16_t **)buff, nsamps, timestamp);

  return nsamps;
}

static int zmq_read(openair0_device_t *device, openair0_timestamp_t *ptimestamp, void **samplesVoid, int nsamps, int nbAnt)
{
  zmq_state_t *s = static_cast<zmq_state_t *>(device->priv);
  AssertFatal((uint)nbAnt == s->rx_stream.channels_.size(),
              "Request to read on more antennas (%d) than configured (%d)",
              nbAnt,
              (int)s->rx_stream.channels_.size());
  uint64_t timestamp;
  s->rx_stream.receive((c16_t **)samplesVoid, nsamps, &timestamp);
  *ptimestamp = timestamp;
  return nsamps;
}

static int zmq_get_stats(openair0_device_t *device)
{
  return 0;
}
static int zmq_reset_stats(openair0_device_t *device)
{
  return 0;
}
static void zmq_end(openair0_device_t *device)
{
  zmq_state_t *s = static_cast<zmq_state_t *>(device->priv);
  if (s) {
    if (s->poll_thread_running) {
      s->poll_thread_running = false;
      for (auto &t : s->tx_poll_threads) {
        if (t.joinable()) {
          t.join();
        }
      }
      for (auto &t : s->rx_poll_threads) {
        if (t.joinable()) {
          t.join();
        }
      }
    }
    for (auto &chan : s->tx_stream.channels_) {
      if (chan->socket_)
        zmq_close(chan->socket_);
      delete chan;
    }
    s->tx_stream.channels_.clear();

    for (auto &chan : s->rx_stream.channels_) {
      if (chan->socket_)
        zmq_close(chan->socket_);
      delete chan;
    }
    s->rx_stream.channels_.clear();

    if (s->context)
      zmq_ctx_destroy(s->context);
    delete s;
  }
}

static int zmq_start(openair0_device_t *device)
{
  zmq_state_t *s = static_cast<zmq_state_t *>(device->priv);
  s->rx_stream.start(s->sample_rate / 100);
  s->tx_stream.start(s->sample_rate / 100);
  for (size_t i = 0; i < s->rx_stream.channels_.size(); i++) {
    auto channel = s->rx_stream.channels_[i];
    // Send initial request to start data flow
    char dummy = 0;
    if (zmq_send(channel->socket_, &dummy, 1, 0) != 1) {
      LOG_E(HW, "[ZMQ] zmq_send for initial RX request failed for antenna %lu: %s\n", i, zmq_strerror(errno));
      return -1;
    }
  }
  s->poll_thread_running = true;
  for (size_t i = 0; i < s->tx_stream.channels_.size(); ++i) {
    s->tx_poll_threads.push_back(std::thread(tx_poll_thread, s->tx_stream.channels_[i], i, &s->poll_thread_running));
  }
  for (size_t i = 0; i < s->rx_stream.channels_.size(); ++i) {
    s->rx_poll_threads.push_back(std::thread(rx_poll_thread, s->rx_stream.channels_[i], i, &s->poll_thread_running));
  }
  return 0;
}

static int zmq_stop(openair0_device_t *device)
{
  zmq_state_t *s = static_cast<zmq_state_t *>(device->priv);
  s->rx_stream.stop();
  // Stop and join the poll threads here so a stop->end sequence is idempotent:
  // zmq_end() skips its join block once poll_thread_running is false.
  if (s->poll_thread_running) {
    s->poll_thread_running = false;
    for (auto &t : s->tx_poll_threads) {
      if (t.joinable()) {
        t.join();
      }
    }
    for (auto &t : s->rx_poll_threads) {
      if (t.joinable()) {
        t.join();
      }
    }
  }
  return 0;
}

static int zmq_set_freq(openair0_device_t *device, openair0_config_t *openair0_cfg)
{
  return 0;
}
static int zmq_set_gains(openair0_device_t *device, openair0_config_t *openair0_cfg)
{
  return 0;
}
static int zmq_write_init(openair0_device_t *device)
{
  return 0;
}

extern "C" __attribute__((__visibility__("default"))) int device_init(openair0_device_t *device, openair0_config_t *openair0_cfg)
{
  auto *zmq_state = new zmq_state_t();
  zmq_state->context = zmq_ctx_new();
  AssertFatal(zmq_state->context != NULL, "zmq_ctx_new failed");

  LOG_I(HW, "[ZMQ] tx_antennas: %d, rx_antennas: %d\n", openair0_cfg->tx_num_channels, openair0_cfg->rx_num_channels);
  configmodule_interface_t *cfg = config_get_if();
  paramdef_t param_desc[] = ZMQ_PARAMS_DESC;
  std::string zmq_section = std::string(ZMQ_SECTION);
  int ru_id = openair0_cfg->ru_id;
  std::string zmq_array_section = std::string(ZMQ_SECTION) + ".[" + std::to_string(ru_id) + "]";
  int ret = config_get(cfg, param_desc, sizeofArray(param_desc), zmq_array_section.c_str());
  AssertFatal(ret >= 0, "configuration couldn't be performed\n");
  int num_configured_tx_channels = gpd(param_desc, sizeofArray(param_desc), ZMQ_TX_CHANNELS)->numelt;
  AssertFatal(num_configured_tx_channels == openair0_cfg->tx_num_channels,
              "Incorrect configuration: Number of zmq tx channels (%d) != number of configured tx channels (%d)\n",
              num_configured_tx_channels,
              openair0_cfg->tx_num_channels);
  int num_configured_rx_channels = gpd(param_desc, sizeofArray(param_desc), ZMQ_RX_CHANNELS)->numelt;
  AssertFatal(num_configured_rx_channels == openair0_cfg->rx_num_channels,
              "Incorrect configuration: Number of zmq rx channels (%d) != number of configured rx channels (%d)\n",
              num_configured_rx_channels,
              openair0_cfg->rx_num_channels);
  char **tx_channels = gpd(param_desc, sizeofArray(param_desc), ZMQ_TX_CHANNELS)->strlistptr;
  char **rx_channels = gpd(param_desc, sizeofArray(param_desc), ZMQ_RX_CHANNELS)->strlistptr;

  // Setup TX sockets (one per antenna)
  if (openair0_cfg->tx_num_channels > 0) {
    zmq_state->tx_stream.channels_.resize(openair0_cfg->tx_num_channels);
    for (int i = 0; i < openair0_cfg->tx_num_channels; i++) {
      void *socket = zmq_socket(zmq_state->context, ZMQ_REP);
      AssertFatal(socket != NULL, "zmq_socket(ZMQ_REP) for TX antenna %d failed", i);
      int linger = 0;
      zmq_setsockopt(socket, ZMQ_LINGER, &linger, sizeof(linger));
      int timeout = 1000; // ms: bound socket calls so poll threads can exit on shutdown
      zmq_setsockopt(socket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
      zmq_setsockopt(socket, ZMQ_SNDTIMEO, &timeout, sizeof(timeout));
      AssertFatal(zmq_bind(socket, tx_channels[i]) == 0, "zmq_bind for TX antenna %d failed on %s", i, tx_channels[i]);
      auto channel = new zmq_tx_channel(socket, openair0_cfg->sample_rate);
      LOG_I(HW, "[ZMQ] TX socket for antenna %d bound to %s\n", i, tx_channels[i]);
      zmq_state->tx_stream.channels_[i] = channel;
    }
  }
  zmq_state->sample_rate = openair0_cfg->sample_rate;

  // Setup RX sockets (one per antenna)
  if (openair0_cfg->rx_num_channels > 0) {
    zmq_state->rx_stream.channels_.resize(openair0_cfg->rx_num_channels);
    for (int i = 0; i < openair0_cfg->rx_num_channels; i++) {
      void *socket = zmq_socket(zmq_state->context, ZMQ_REQ);
      AssertFatal(socket != NULL, "zmq_socket(ZMQ_REQ) for RX antenna %d failed", i);
      int linger = 0;
      zmq_setsockopt(socket, ZMQ_LINGER, &linger, sizeof(linger));
      int timeout = 1000; // ms: bound socket calls so poll threads can exit on shutdown
      zmq_setsockopt(socket, ZMQ_RCVTIMEO, &timeout, sizeof(timeout));
      zmq_setsockopt(socket, ZMQ_SNDTIMEO, &timeout, sizeof(timeout));
      AssertFatal(zmq_connect(socket, rx_channels[i]) == 0, "zmq_connect for RX antenna %d failed on %s", i, rx_channels[i]);
      auto channel = new zmq_rx_channel(socket, openair0_cfg->sample_rate);
      LOG_I(HW, "[ZMQ] RX socket for antenna %d connected to %s\n", i, rx_channels[i]);
      zmq_state->rx_stream.channels_[i] = channel;
    }
    zmq_state->rx_stream.tx_stream_ = &zmq_state->tx_stream;
  }

  device->trx_start_func = zmq_start;
  device->trx_get_stats_func = zmq_get_stats;
  device->trx_reset_stats_func = zmq_reset_stats;
  device->trx_end_func = zmq_end;
  device->trx_stop_func = zmq_stop;
  device->trx_set_freq_func = zmq_set_freq;
  device->trx_set_gains_func = zmq_set_gains;
  device->trx_write_func = zmq_write;
  device->trx_read_func = zmq_read;
  device->type = RFSIMULATOR;
  IS_SOFTMODEM_RFSIM = 1U;
  openair0_cfg->rx_gain[0] = 0;
  device->openair0_cfg = openair0_cfg;
  device->priv = zmq_state;
  device->trx_write_init = zmq_write_init;

  return 0;
}
