/*
 * Copyright 2017 Cisco Systems, Inc.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 * 
 * http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#pragma once

#include <string.h>
#include <errno.h>

#define ERR strerror(errno)

/*! The trace levels used by the nfapi libraries */
typedef enum nfapi_trace_level
{
    NFAPI_TRACE_NONE,
    NFAPI_TRACE_ERROR,
    NFAPI_TRACE_WARN,
    NFAPI_TRACE_NOTE,
    NFAPI_TRACE_INFO,
    NFAPI_TRACE_DEBUG,
} nfapi_trace_level_t;

void nfapi_trace(nfapi_trace_level_t, char const *caller, const char *format, ...)
    __attribute__((format(printf, 3, 4)));

nfapi_trace_level_t nfapi_trace_level(void);

#define NFAPI_TRACE(LEVEL, FORMAT, ...) do {                            \
    if (nfapi_trace_level() >= (LEVEL))                                 \
        nfapi_trace(LEVEL, __func__, FORMAT, ##__VA_ARGS__);            \
} while (0)

