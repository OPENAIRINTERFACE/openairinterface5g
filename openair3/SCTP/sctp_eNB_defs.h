#include "queue.h"

#ifndef SCTP_ENB_DEFS_H_
#define SCTP_ENB_DEFS_H_

typedef struct sctp_queue_item_s {
  /* Pair of stream on which we received this packet */
  uint16_t  local_stream;

  /* Remote port */
  uint16_t remote_port;
  /* Remote address */
  uint32_t remote_addr;
  uint32_t assoc_id;

  /* PPID used for the packet */
  uint32_t ppid;

  /* Buffer and buffer length for the packet */
  uint32_t length;
  uint8_t *buffer;

  /* queue.h internal data */
  TAILQ_ENTRY(sctp_queue_item_s) entry;
} sctp_queue_item_t;

typedef struct {
  /* Socket descriptor used to send/recv data on SCTP */
  int sd;

  /* Unique SCTP association ID (Local to host), used to distinguish
   * associations between MME and eNB.
   */
  int32_t assoc_id;

  /* Current remote port used for transmission */
  uint16_t  remote_port;
  /* Remote IP addresses */
  struct sockaddr *remote_ip_addresses;
  int nb_remote_addresses;

  /* Local port to use for transmission (dynamically allocated) */
  uint16_t  local_port;
  /* Local IP address to use for transmission */
  struct sockaddr *local_ip_addr;
  int nb_local_addresses;

  /* Number of input/output streams used over this association.
   * The number is negotiated between peers at connect and the minimum value
   * of both peers is used over the association.
   */
  uint16_t instreams;
  uint16_t outstreams;

  /* Queue of messages received on SCTP. Messages will be processed later by
   * upper layer. This will allow data prioritization and data de-fragmentation
   * (if any on interface).
   */
  TAILQ_HEAD(sctp_queue_s, sctp_queue_item_s) sctp_queue;
  /* Queue size in bytes (may be used to limit eNB processing) */
  uint32_t queue_size;
  /* Number of items in the queue */
  uint32_t queue_length;
} sctp_data_t;

#endif /* SCTP_ENB_DEFS_H_ */
