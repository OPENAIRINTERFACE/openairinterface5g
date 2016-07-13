#ifndef _EMM_PROC_DEFS_H
#define _EMM_PROC_DEFS_H

/* Type of network attachment */
typedef enum {
  EMM_ATTACH_TYPE_EPS = 0,
  EMM_ATTACH_TYPE_IMSI,
  EMM_ATTACH_TYPE_EMERGENCY,
  EMM_ATTACH_TYPE_RESERVED,
} emm_proc_attach_type_t;

/* Type of network detach */
typedef enum {
  EMM_DETACH_TYPE_EPS = 0,
  EMM_DETACH_TYPE_IMSI,
  EMM_DETACH_TYPE_EPS_IMSI,
  EMM_DETACH_TYPE_REATTACH,
  EMM_DETACH_TYPE_NOT_REATTACH,
  EMM_DETACH_TYPE_RESERVED,
} emm_proc_detach_type_t;

/* Type of requested identity */
typedef enum {
  EMM_IDENT_TYPE_NOT_AVAILABLE = 0,
  EMM_IDENT_TYPE_IMSI,
  EMM_IDENT_TYPE_IMEI,
  EMM_IDENT_TYPE_IMEISV,
  EMM_IDENT_TYPE_TMSI
} emm_proc_identity_type_t;

#endif
