#include <errno.h>
#include <string.h>

#ifndef RC_H_
#define RC_H_

#define RC_OK 0
#define RC_FAIL         -1
#define RC_BAD_PARAM    -2
#define RC_NULL_POINTER -3

static const char * const rc_strings[] =
    {"Ok", "fail", "bad parameter", "null pointer"};

#define CHECK_FCT(fCT)              \
do {                                \
    int rET;                        \
    if ((rET = fCT) != RC_OK) {     \
        fprintf(stderr, #fCT" has failed (%s:%d)\n", __FILE__, __LINE__);   \
        return rET;                 \
    }                               \
} while(0)

#define CHECK_FCT_POSIX(fCT)        \
do {                                \
    if (fCT == -1) {                \
        fprintf(stderr, #fCT" has failed (%d:%s) (%s:%d)\n", errno, \
                strerror(errno), __FILE__, __LINE__);               \
        return RC_FAIL;             \
    }                               \
} while(0)

#define CHECK_FCT_DO(fCT, dO)       \
do {                                \
    int rET;                        \
    if ((rET = fCT) != RC_OK) {     \
        fprintf(stderr, #fCT" has returned %d (%s:%d)\n", rET, __FILE__, __LINE__);   \
        dO;                         \
    }                               \
} while(0)

#define CHECK_BUFFER(bUFFER)        \
do {                                \
    if ((bUFFER) == NULL) {         \
        fprintf(stderr, #bUFFER" is NULL (%s:%d)\n", __FILE__, __LINE__);   \
        return RC_NULL_POINTER;     \
    }                               \
} while(0)

#endif /* RC_H_ */
