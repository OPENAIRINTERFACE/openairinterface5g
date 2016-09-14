#ifndef UDP_MESSAGES_TYPES_H_
#define UDP_MESSAGES_TYPES_H_

#define UDP_INIT(mSGpTR)    (mSGpTR)->ittiMsg.udp_init

typedef struct {
  uint32_t  port;
  char     *address;
} udp_init_t;

typedef struct {
  uint8_t  *buffer;
  uint32_t  buffer_length;
  uint32_t  buffer_offset;
  uint32_t  peer_address;
  uint32_t  peer_port;
} udp_data_req_t;

typedef struct {
  uint8_t  *buffer;
  uint32_t  buffer_length;
  uint32_t  peer_address;
  uint32_t  peer_port;
} udp_data_ind_t;

#endif /* UDP_MESSAGES_TYPES_H_ */
