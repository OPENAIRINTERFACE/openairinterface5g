#include <signal.h>

#ifndef BACKTRACE_H_
#define BACKTRACE_H_

void display_backtrace(void);

void backtrace_handle_signal(siginfo_t *info);

#endif /* BACKTRACE_H_ */
