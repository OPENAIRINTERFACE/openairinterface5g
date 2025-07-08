#ifndef _NR_RU_STREAMER_H_
#define _NR_RU_STREAMER_H_

#include <stdint.h>
#include <stdbool.h>
#include <stdatomic.h>

/**
 * @brief Global atomic counter to control the number of TX samples to stream.
 *
 * This variable is thread-safe.
 * Semantics:
 * - 0:  TX Streaming is off.
 * - -1: Continuous TX streaming.
 * - >0: Stream this many TX packets and then stop (by setting itself to 0).
 */
extern atomic_int g_stream_tx_sample_count;

/**
 * @brief Global atomic counter to control the number of RX samples to stream.
 *
 * This variable is thread-safe.
 * Semantics:
 * - 0:  RX Streaming is off.
 * - -1: Continuous RX streaming.
 * - >0: Stream this many RX packets and then stop (by setting itself to 0).
 */
extern atomic_int g_stream_rx_sample_count;

/**
 * @brief Initializes the ZeroMQ publisher for data and the reply socket for control.
 *
 * This function sets up the PUB socket for IQ data streaming and spawns a separate
 * thread to listen for control commands (like setting the number of samples to stream)
 * on the REP socket.
 *
 * @param pub_endpoint The endpoint for the publisher socket (e.g., "tcp://127.0.0.1:5555").
 * @param control_endpoint The endpoint for the control socket (e.g., "tcp://127.0.0.1:5556").
 */
void streamer_setup(const char* pub_endpoint, const char* control_endpoint);

/**
 * @brief Streams a buffer of TX samples if TX streaming is enabled.
 *
 * Checks the value of g_stream_tx_sample_count. If it's non-zero, the samples are
 * published via ZeroMQ. If the count is positive, it is decremented.
 *
 * @param timestamp The timestamp of the samples.
 * @param tx_buffs An array of pointers to the TX sample buffers for each antenna.
 * @param num_samples The number of samples in each buffer.
 * @param num_antennas The number of antennas.
 */
void stream_tx_iq(uint64_t timestamp, void **tx_buffs, int num_samples, int num_antennas);

/**
 * @brief Streams a buffer of RX samples if RX streaming is enabled.
 *
 * Checks the value of g_stream_rx_sample_count. If it's non-zero, the samples are
 * published via ZeroMQ. If the count is positive, it is decremented.
 *
 * @param timestamp The timestamp of the samples.
 * @param rx_buffs An array of pointers to the RX sample buffers for each antenna.
 * @param num_samples The number of samples in each buffer.
 * @param num_antennas The number of antennas.
 */
void stream_rx_iq(uint64_t timestamp, void **rx_buffs, int num_samples, int num_antennas);

/**
 * @brief Gracefully shuts down all ZeroMQ sockets and terminates the control thread.
 *
 * This should be called on application exit to release all resources.
 */
void streamer_teardown(void);

#endif // _NR_RU_STREAMER_H_
