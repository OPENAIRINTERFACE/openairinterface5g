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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "OctetString.h"

#ifndef CLI_H_
#define CLI_H_

#define CLI_MINIMUM_LENGTH 3
#define CLI_MAXIMUM_LENGTH 14

typedef struct Cli_tag {
  OctetString clivalue;
} Cli;

int encode_cli(Cli *cli, uint8_t iei, uint8_t *buffer, uint32_t len);

int decode_cli(Cli *cli, uint8_t iei, uint8_t *buffer, uint32_t len);

void dump_cli_xml(Cli *cli, uint8_t iei);

#endif /* CLI_H_ */

