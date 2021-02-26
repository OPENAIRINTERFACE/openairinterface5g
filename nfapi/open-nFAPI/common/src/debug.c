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


#include <stdio.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <pthread.h>
#include <syslog.h>

#include <debug.h>

#define MAX_MSG_LENGTH 			2096
#define TRACE_HEADER_LENGTH		44

void nfapi_trace_dbg(nfapi_trace_level_t level, const char *format, ...);

// initialize the trace function to 0
void (*nfapi_trace_g)(nfapi_trace_level_t level, const char* format, ...) = &nfapi_trace_dbg;

nfapi_trace_level_t nfapi_trace_level_g = NFAPI_TRACE_INFO;
//nfapi_trace_level_t nfapi_trace_level_g = NFAPI_TRACE_WARN;

void nfapi_set_trace_level(nfapi_trace_level_t new_level)
{
	nfapi_trace_level_g = new_level;
}

void nfapi_trace_dbg(nfapi_trace_level_t level, const char *format, ...)
{
	char trace_buff[MAX_MSG_LENGTH + TRACE_HEADER_LENGTH];
	va_list p_args;
	struct timeval tv;
	pthread_t tid = pthread_self();

	(void)gettimeofday(&tv, NULL);

	snprintf(trace_buff, sizeof(trace_buff), "%04u.%06u: 0x%02x: %10u: ", ((uint32_t)tv.tv_sec) & 0x1FFF, (uint32_t)tv.tv_usec, (uint32_t)level, (uint32_t)tid);
	int n = strlen(trace_buff);
	va_start(p_args, format);
	vsnprintf(trace_buff + n, sizeof(trace_buff) - n, format, p_args);
	va_end(p_args);
	fputs(trace_buff, stdout);
	fflush(stdout);
}
