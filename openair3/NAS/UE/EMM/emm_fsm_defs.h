#ifndef _EMM_FSM_DEFS_H
#define _EMM_FSM_DEFS_H

/****************************************************************************/
/************************  G L O B A L    T Y P E S  ************************/
/****************************************************************************/

/*
 * States of the EPS Mobility Management sublayer
 * ----------------------------------------------
 * The EMM protocol of the UE and the network is described by means of two
 * different state machines.
 */
typedef enum {
  EMM_INVALID,
  EMM_NULL,
  EMM_DEREGISTERED,
  EMM_REGISTERED,
  EMM_DEREGISTERED_INITIATED,
  EMM_DEREGISTERED_NORMAL_SERVICE,
  EMM_DEREGISTERED_LIMITED_SERVICE,
  EMM_DEREGISTERED_ATTEMPTING_TO_ATTACH,
  EMM_DEREGISTERED_PLMN_SEARCH,
  EMM_DEREGISTERED_NO_IMSI,
  EMM_DEREGISTERED_ATTACH_NEEDED,
  EMM_DEREGISTERED_NO_CELL_AVAILABLE,
  EMM_REGISTERED_INITIATED,
  EMM_REGISTERED_NORMAL_SERVICE,
  EMM_REGISTERED_ATTEMPTING_TO_UPDATE,
  EMM_REGISTERED_LIMITED_SERVICE,
  EMM_REGISTERED_PLMN_SEARCH,
  EMM_REGISTERED_UPDATE_NEEDED,
  EMM_REGISTERED_NO_CELL_AVAILABLE,
  EMM_REGISTERED_ATTEMPTING_TO_UPDATE_MM,
  EMM_REGISTERED_IMSI_DETACH_INITIATED,
  EMM_TRACKING_AREA_UPDATING_INITIATED,
  EMM_SERVICE_REQUEST_INITIATED,
  EMM_STATE_MAX
} emm_fsm_state_t;


#endif
