#ifndef _MOBIPASS_H_
#define _MOBIPASS_H_

#include <stdint.h>
#include "ethernet_lib.h"

typedef struct {
  /* this has to come first */
  eth_state_t eth;

  void *qstate;

  uint8_t eth_local[6];
  uint8_t eth_remote[6];

  int samples_per_1024_frames;

  /* variables used by the function interface.c:mobipass_read */
  uint32_t mobipass_read_ts;
  unsigned char mobipass_read_seqno;

  /* variables used by the function interface.c:mobipass_write */
  uint32_t mobipass_write_last_timestamp;

  /* variables used by the function mobipass.c:[init_time|synch_time] */
  uint64_t t0;

  /* variables used by the function mobipass.c:synch_time */
  uint32_t synch_time_last_ts;
  uint64_t synch_time_mega_ts;

  /* sock is used in mobipass.c */
  int sock;
} mobipass_state_t;

void init_mobipass(mobipass_state_t *mobi);

#endif /* _MOBIPASS_H_ */
