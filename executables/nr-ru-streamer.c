#include "nr-ru-streamer.h"
#include <zmq.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include "common/utils/LOG/log.h"

// Define the global atomic counters for streaming control
atomic_int g_stream_tx_sample_count = 0;
atomic_int g_stream_rx_sample_count = 0;

// ZeroMQ context and sockets
static void *context = NULL;
static void *publisher_rx = NULL;
static void *publisher_tx = NULL;

static void *control_socket = NULL;

// Control thread
static pthread_t control_thread;
static bool control_thread_running = false;

/**
 * @brief The main function for the ZeroMQ control thread.
 *
 * This thread runs a loop that waits for commands on the REP socket.
 * It can receive commands like "set_tx 100" or "set_rx -1".
 *
 * @param arg The endpoint string for binding the control socket.
 * @return NULL.
 */
static void *streamer_control_thread_func(void *arg) {
    // Detach the thread so resources are automatically released on exit
    pthread_detach(pthread_self());
    
    char* endpoint = (char*)arg;
    
    // Create and bind the REP socket
    control_socket = zmq_socket(context, ZMQ_REP);
    assert(control_socket);
    if (zmq_bind(control_socket, endpoint) != 0) {
        LOG_E(HW, "[STREAMER] Control thread failed to bind to %s: %s\n", endpoint, zmq_strerror(errno));
        zmq_close(control_socket);
        control_socket = NULL;
        free(endpoint);
        return NULL;
    }
    
    LOG_I(HW, "[STREAMER] Control thread listening on %s\n", endpoint);
    free(endpoint);

    char buffer[64];
    char command[16];
    int value;

    while (control_thread_running) {
        // Wait for a command (this is a blocking call)
        int nbytes = zmq_recv(control_socket, buffer, sizeof(buffer) - 1, 0);
        if (nbytes > 0) {
            buffer[nbytes] = '\0';
            
            // Parse the command, expecting format: "command_name value"
            int items_parsed = sscanf(buffer, "%15s %d", command, &value);
            
            if (items_parsed == 2) {
                if (strcmp(command, "set_tx") == 0) {
                    atomic_store(&g_stream_tx_sample_count, value);
                    LOG_I(HW, "[STREAMER] Command received: Set TX stream count to %d\n", value);
                    zmq_send(control_socket, "OK", 2, 0);
                } else if (strcmp(command, "set_rx") == 0) {
                    atomic_store(&g_stream_rx_sample_count, value);
                    LOG_I(HW, "[STREAMER] Command received: Set RX stream count to %d\n", value);
                    zmq_send(control_socket, "OK", 2, 0);
                }
                else if(strcmp(command, "all") == 0)
                {
                    atomic_store(&g_stream_rx_sample_count, value);
                    atomic_store(&g_stream_tx_sample_count, value);
                    LOG_I(HW, "[STREAMER] Command received: Set all stream count to %d\n", value);
                    zmq_send(control_socket, "OK", 2, 0);
                }
                else {
                    LOG_W(HW, "[STREAMER] Received unknown control command: %s\n", command);
                    zmq_send(control_socket, "Error: Unknown command", 22, 0);
                }
            } else {
                LOG_W(HW, "[STREAMER] Received invalid command format: %s\n", buffer);
                zmq_send(control_socket, "Error: Invalid format", 21, 0);
            }
        }
    }
    
    zmq_close(control_socket);
    control_socket = NULL;
    LOG_I(HW, "[STREAMER] Control thread terminated.\n");
    return NULL;
}


void streamer_setup(const char* pub_rx_endpoint, const char *pub_tx_endpoint, const char* control_endpoint) {
    if (context != NULL) {
        LOG_W(HW, "[STREAMER] ZeroMQ already initialized.\n");
        return;
    }
    
    context = zmq_ctx_new();
    assert(context);

    // Setup RX publisher socket for IQ data
    publisher_rx = zmq_socket(context, ZMQ_PUB);
    assert(publisher_rx);
    if (zmq_bind(publisher_rx, pub_rx_endpoint) != 0) {
        LOG_E(HW, "[STREAMER] Failed to bind RX ZeroMQ publisher to %s: %s\n", pub_rx_endpoint, zmq_strerror(errno));
        zmq_close(publisher_rx);
        zmq_ctx_term(context);
        publisher_rx = NULL;
        context = NULL;
        return;
    }
    LOG_I(HW, "[STREAMER] ZeroMQ RX Publisher initialized and bound to %s\n", pub_rx_endpoint);


    // Setup TX publisher socket for IQ data
    publisher_tx = zmq_socket(context, ZMQ_PUB);
    assert(publisher_tx);
    
    if(zmq_bind(publisher_tx, pub_tx_endpoint) != 0)
    {
        LOG_E(HW, "[STREAMER] Failed to bind ZeroMQ TX publisher to %s: %s\n", pub_tx_endpoint, zmq_strerror(errno));
        zmq_close(publisher_tx);
        zmq_ctx_term(context);
        publisher_tx = NULL;
        context = NULL;
        return;
    }
    LOG_I(HW, "[STREAMER] ZeroMQ TX Publisher initialized and bound to %s\n", pub_tx_endpoint);
    
    // Setup control thread
    control_thread_running = true;
    char *control_endpoint_copy = strdup(control_endpoint); // Create a copy for the thread
    if (pthread_create(&control_thread, NULL, streamer_control_thread_func, control_endpoint_copy) != 0) {
        LOG_E(HW, "[STREAMER] Failed to create control thread!\n");
        control_thread_running = false;
        free(control_endpoint_copy);
    }
    
    // Allow some time for sockets to get ready
    usleep(100000); // 100ms
}


void streamer_teardown(void) {
    // Signal the control thread to stop
    if (control_thread_running) {
        control_thread_running = false;
    }

    if (publisher_rx) {
        LOG_I(HW, "[STREAMER] Closing ZeroMQ rx publisher socket.\n");
        zmq_close(publisher_rx);
        publisher_rx = NULL;
    }

    if(publisher_tx)
    {
        LOG_I(HW, "[STREAMER] Closing ZeroMQ tx publisher socket.\n");
        zmq_close(publisher_tx);
        publisher_tx = NULL;
    }

    if (context) {
        LOG_I(HW, "[STREAMER] Terminating ZeroMQ context.\n");
        zmq_ctx_term(context); // This will unblock the control thread's zmq_recv
        context = NULL;
    }
}


// Common function to send a multipart message.
static void send_multipart_data(const char* topic, uint64_t timestamp, void **buffs, int num_samples, int num_antennas) {
    
    if(strcmp(topic, "rx_stream"))
    {
        if(!publisher_rx) return;
        // Part 1: Topic
        zmq_send(publisher_rx, topic, strlen(topic), ZMQ_SNDMORE);
        
        // Part 2: Timestamp
        zmq_send(publisher_rx, &timestamp, sizeof(uint64_t), ZMQ_SNDMORE);
        
        // Part 3...N: IQ data for each antenna
        size_t data_size = num_samples * sizeof(int32_t); // sizeof(c16_t)
        for (int i = 0; i < num_antennas; i++) {
            zmq_send(publisher_rx, buffs[i], data_size, (i == num_antennas - 1) ? 0 : ZMQ_SNDMORE);
        }


    }
    else
    {
        if(!publisher_tx) return;
        // Part 1: Topic
        zmq_send(publisher_tx, topic, strlen(topic), ZMQ_SNDMORE);
        
        // Part 2: Timestamp
        zmq_send(publisher_tx, &timestamp, sizeof(uint64_t), ZMQ_SNDMORE);
        
        // Part 3...N: IQ data for each antenna
        size_t data_size = num_samples * sizeof(int32_t); // sizeof(c16_t)
        for (int i = 0; i < num_antennas; i++) {
            zmq_send(publisher_tx, buffs[i], data_size, (i == num_antennas - 1) ? 0 : ZMQ_SNDMORE);
        }

    }
    
}

void stream_tx_iq(uint64_t timestamp, void **tx_buffs, int num_samples, int num_antennas) {
    int count = atomic_load(&g_stream_tx_sample_count);
    if (count == 0) return; // Streaming is off

    send_multipart_data("tx_stream", timestamp, tx_buffs, num_samples, num_antennas);
    
    // If count is positive (not continuous), decrement it
    if (count > 0) {
        atomic_fetch_sub(&g_stream_tx_sample_count, 1);
    }
}

void stream_rx_iq(uint64_t timestamp, void **rx_buffs, int num_samples, int num_antennas) {
    int count = atomic_load(&g_stream_rx_sample_count);
    if (count == 0) return; // Streaming is off

    send_multipart_data("rx_stream", timestamp, rx_buffs, num_samples, num_antennas);

    // If count is positive (not continuous), decrement it
    if (count > 0) {
        atomic_fetch_sub(&g_stream_rx_sample_count, 1);
    }
}