#ifndef EMM_TIMERS_H
#define EMM_TIMERS_H

/*
 * Retransmission timer handlers
 */

void *emm_attach_t3410_handler(void *);
void *emm_service_t3417_handler(void *);
void *emm_detach_t3421_handler(void *);
void *emm_tau_t3430_handler(void *);


#endif
