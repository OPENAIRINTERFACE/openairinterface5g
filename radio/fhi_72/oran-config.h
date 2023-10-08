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

#ifndef ORAN_CONFIG_H
#define ORAN_CONFIG_H

#include "stdbool.h"
#include "stdint.h"

struct xran_fh_init;
void print_fh_init(const struct xran_fh_init *fh_init);
struct xran_fh_config;
void print_fh_config(const struct xran_fh_config *fh_config);

bool set_fh_init(struct xran_fh_init *fh_init);
struct openair0_config;
bool set_fh_config(int ru_idx, int num_rus, const struct openair0_config *oai0_cfg, struct xran_fh_config *fh_config);

// hack to workaround LiteOn limitation
extern int g_kbar;

#endif /* ORAN_CONFIG_H */
