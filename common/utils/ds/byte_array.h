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

#ifndef BYTE_ARRAY_H 
#define BYTE_ARRAY_H 

/* WARNING: This file is also defined at XXXXX. Both files need to be completely equal. Same applies for *.c */

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

typedef struct {
  size_t len;
  uint8_t* buf;
} byte_array_t;

typedef struct {
  uint8_t buf[32];
} byte_array_32_t;

byte_array_t copy_byte_array(byte_array_t src);
void free_byte_array(byte_array_t ba);
bool eq_byte_array(const byte_array_t* m0, const byte_array_t* m1);

byte_array_t cp_str_to_ba(const char* str);

#endif
