#ifndef _NAS_PROC_DEFS_H
#define _NAS_PROC_DEFS_H

/*
 * Local NAS data
 */
typedef struct {
  /* EPS capibility status */
  int EPS_capability_status;
  /* Reference signal received quality    */
  int rsrq;
  /* Reference signal received power      */
  int rsrp;
} proc_data_t;

#endif
