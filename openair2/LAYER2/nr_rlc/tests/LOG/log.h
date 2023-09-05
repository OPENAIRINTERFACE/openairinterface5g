#ifndef _NR_RLC_TESTS_LOG_H_
#define _NR_RLC_TESTS_LOG_H_

#include <stdio.h>

#define LOG_E(x, ...) do { printf("LOG_E: "); printf(__VA_ARGS__); } while (0)
#define LOG_D(x, ...) do { printf("LOG_D: "); printf(__VA_ARGS__); } while (0)
#define LOG_W(x, ...) do { printf("LOG_W: "); printf(__VA_ARGS__); } while (0)

#endif /* _NR_RLC_TESTS_LOG_H_ */
