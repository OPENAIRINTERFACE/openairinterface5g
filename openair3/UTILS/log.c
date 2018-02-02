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

#if HAVE_CONFIG_H
# include "config.h"
#endif

#include "log.h"

/* mme log */
int log_enabled = 0;

int log_init(const mme_config_t *mme_config_p,
             log_specific_init_t specific_init)
{
  if (mme_config_p->verbosity_level == 1) {
    log_enabled = 1;
  } else if (mme_config_p->verbosity_level == 2) {
    log_enabled = 1;
  } else {
    log_enabled = 0;
  }

  return specific_init(mme_config_p->verbosity_level);
}
