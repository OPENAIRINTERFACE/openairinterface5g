/*
 * SPDX-License-Identifier: BSD-3-Clause-Open-MPI
 * Based on zmq library in OCUDU project: ocudu/lib/radio/zmq
 * Refer to https://gitlab.com/ocudu/ocudu/-/raw/dev/LICENSE?ref_type=heads
 */

#ifndef ZMQ_IMPORTED_H
#define ZMQ_IMPORTED_H

#include <zmq.h>
#include "ring_buffer.h"
#include <condition_variable>
#include <atomic>
#include <mutex>
#include <vector>
#include <queue>

class zmq_tx_channel {
 public:
  void *socket_;
  std::queue<zmq_msg_t> queue_;
  std::mutex queue_mutex_;
  // Signalled whenever a message is pushed onto queue_, so tx_poll_thread can
  // block for the next message instead of re-checking on a 10ms poll timeout.
  std::condition_variable queue_cvar_;
  std::atomic<uint64_t> sample_count_ = 0;
  std::atomic<bool> is_tx_enabled_ = false;
  std::mutex transmit_alignment_mutex_;
  std::condition_variable transmit_alignment_cvar_;

  zmq_tx_channel(void *s, uint64_t buffer_size) : socket_(s)
  {
  }

  ~zmq_tx_channel()
  {
    std::lock_guard<std::mutex> lock(queue_mutex_);
    while (!queue_.empty()) {
      zmq_msg_close(&queue_.front());
      queue_.pop();
    }
  }

  void transmit(c16_t *samples, size_t nsamps, uint64_t timestamp);
  bool pop_message(zmq_msg_t *msg);
  // Block until a message is available (woken by queue_cvar_) or timeout/stop.
  bool wait_and_pop_message(zmq_msg_t *msg, std::chrono::milliseconds timeout, std::atomic<bool> *running);

  void start(uint64_t init_time);

  bool align(uint64_t timestamp, std::chrono::milliseconds timeout);
};

class zmq_rx_channel {
 public:
   void *socket_;
   overflow_buffer<c16_t> buffer_;
   bool request_sent_;
   std::atomic<bool> stopped_;
   zmq_rx_channel(void *s, uint64_t buffer_size) : socket_(s), buffer_(buffer_size), stopped_(false)
  {
  }
  void receive(c16_t *samples, size_t nsamps);
  void stop();
};

class zmq_tx_stream {
 public:
  std::vector<zmq_tx_channel *> channels_;
  void start(uint64_t init_time);
  bool align(uint64_t timestamp, std::chrono::milliseconds timeout);
  void transmit(c16_t **samples, size_t nsamps, uint64_t timestamp);
};

class zmq_rx_stream {
 public:
  std::vector<zmq_rx_channel *> channels_;
  zmq_tx_stream *tx_stream_;
  uint64_t sample_count_ = 0;
  zmq_rx_stream() : sample_count_(0)
  {
  }
  void start(uint64_t init_time);
  void stop();
  void receive(c16_t **samples, size_t nsamps, uint64_t *timestamp);
};

#endif
