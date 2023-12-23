/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#ifndef __MEMORY_H__
#define __MEMORY_H__

#include <stddef.h>
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
#include "common/platform_constants.h"
//-----------------------------------------------------------------------------

typedef unsigned char uint8_t;

//-----------------------------------------------------------------------------

char* memory_get_path(const char* dirname, const char* filename);

char* memory_get_path_from_ueid(const char* dirname, const char* filename, int ueid);

int memory_read(const char* datafile, void* data, size_t size);

int memory_write(const char* datafile, const void* data, size_t size);

#ifdef __cplusplus
}
#endif
#endif /* __MEMORY_H__*/
