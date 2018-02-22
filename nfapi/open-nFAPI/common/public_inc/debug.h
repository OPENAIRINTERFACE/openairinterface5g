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


#ifndef _DEBUG_H_
#define _DEBUG_H_

/*! The trace levels used by the nfapi libraries */
typedef enum nfapi_trace_level
{
	NFAPI_TRACE_ERROR = 1,
	NFAPI_TRACE_WARN,
	NFAPI_TRACE_NOTE,
	NFAPI_TRACE_INFO,

	NFAPI_TRACE_LEVEL_MAX
} nfapi_trace_level_t;

/*! The trace function pointer */
typedef void (*nfapi_trace_fn_t)(nfapi_trace_level_t level, const char* format, ...);

/*! Global trace function */
extern nfapi_trace_fn_t nfapi_trace_g;

/*! Global trace level */
extern nfapi_trace_level_t nfapi_trace_level_g;

/*! NFAPI trace macro */
//#define NFAPI_TRACE(level, format, ...) { if(nfapi_trace_g && ((nfapi_trace_level_t)level <= nfapi_trace_level_g)) (*nfapi_trace_g)(level, format, ##__VA_ARGS__); }
#define NFAPI_TRACE(level, format, ...) { if (nfapi_trace_g) (*nfapi_trace_g)(level, format, ##__VA_ARGS__); }

/*! Function to change the trace level 
 * \param new_level The modified trace level
 */

void nfapi_set_trace_level(nfapi_trace_level_t new_level);

#endif /* _DEBUG_H_ */
