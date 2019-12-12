#ifndef _LOG_H_
#define _LOG_H_

#include <stdio.h>

#define LOG_E(x, ...) printf(__VA_ARGS__)
#define LOG_D(x, ...) printf(__VA_ARGS__)
#define LOG_W(x, ...) printf(__VA_ARGS__)

#endif /* _LOG_H_ */
