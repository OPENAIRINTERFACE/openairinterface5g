/*
 * Copyright (c) 2015, EURECOM (www.eurecom.fr)
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * The views and conclusions contained in the software and documentation are those
 * of the authors and should not be interpreted as representing official policies,
 * either expressed or implied, of the FreeBSD Project.
 */


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
