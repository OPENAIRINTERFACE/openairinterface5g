#ifndef TIMER_MESSAGES_TYPES_H_
#define TIMER_MESSAGES_TYPES_H_

//-------------------------------------------------------------------------------------------//
// Defines to access message fields.
#define TIMER_HAS_EXPIRED(mSGpTR)   (mSGpTR)->ittiMsg.timer_has_expired

//-------------------------------------------------------------------------------------------//
typedef struct {
  void *arg;
  long  timer_id;
} timer_has_expired_t;

#endif /* TIMER_MESSAGES_TYPES_H_ */
