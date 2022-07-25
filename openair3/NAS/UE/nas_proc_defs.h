#ifndef _NAS_PROC_DEFS_H
#define _NAS_PROC_DEFS_H

#include <stdbool.h>

/*
 * Local NAS data
 */
typedef struct {
  /* EPS capibility status */
  bool EPS_capability_status;
  /* Reference signal received quality    */
  int rsrq;
  /* Reference signal received power      */
  int rsrp;
} proc_data_t;

/*
 * MT SIM pending status (see ETSI TS 127 007 V10.6.0, Note 2)
 * Commands which interact with MT that are accepted when MT is pending SIM PIN,
 * SIM PUK, or PH-SIM are: +CGMI, +CGMM, +CGMR, +CGSN, D112; (emergency call),
 * +CPAS, +CFUN, +CPIN, +CPINR, +CDIS (read and test command only), and +CIND
 * (read and test command only).
*/
typedef enum {
  NAS_USER_READY, /* MT is not pending for any password   */
  NAS_USER_SIM_PIN,   /* MT is waiting SIM PIN to be given    */
  NAS_USER_SIM_PUK,   /* MT is waiting SIM PUK to be given    */
  NAS_USER_PH_SIM_PIN /* MT is waiting phone-to-SIM card
             * password to be given         */
} nas_user_sim_status;

/*
 * The local UE context
 */
typedef struct {
  /* Firmware version number          */
  const char *version;
  /* SIM pending status           */
  nas_user_sim_status sim_status;
  /* Level of functionality           */
  int fun;
} nas_user_context_t;

#endif
