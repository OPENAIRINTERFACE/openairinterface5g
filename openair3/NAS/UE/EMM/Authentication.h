#ifndef _AUTHENTICATION_H
#define _AUTHENTICATION_H

/*
 * Internal data used for authentication procedure
 */
typedef struct {
  uint8_t rand[AUTH_RAND_SIZE];   /* Random challenge number  */
  uint8_t res[AUTH_RES_SIZE];     /* Authentication response  */
  uint8_t ck[AUTH_CK_SIZE];       /* Ciphering key        */
  uint8_t ik[AUTH_IK_SIZE];       /* Integrity key        */
#define AUTHENTICATION_T3410    0x01
#define AUTHENTICATION_T3417    0x02
#define AUTHENTICATION_T3421    0x04
#define AUTHENTICATION_T3430    0x08
  unsigned char timers;       /* Timer restart bitmap     */
#define AUTHENTICATION_COUNTER_MAX 3
  unsigned char mac_count:2;  /* MAC failure counter (#20)        */
  unsigned char umts_count:2; /* UMTS challenge failure counter (#26) */
  unsigned char sync_count:2; /* Sync failure counter (#21)       */
} authentication_data_t;

#endif
