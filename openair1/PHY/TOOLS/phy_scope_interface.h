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

/*! \file phy_scope_interface.h
 * \brief softscope interface API include file
 * \author Nokia BellLabs France, francois Taburet
 * \date 2019
 * \version 0.1
 * \company Nokia BellLabs France
 * \email: francois.taburet@nokia-bell-labs.com
 * \note
 * \warning
 */
#ifndef __PHY_SCOPE_INTERFACE_H__
#define __PHY_SCOPE_INTERFACE_H__
#include <openair1/PHY/defs_gNB.h>
typedef struct {
  int *argc;
  char **argv;
  RU_t *ru;
  PHY_VARS_gNB *gNB;
} scopeParms_t;


typedef struct scopeData_s {
  int *argc;
  char **argv;
  RU_t *ru;
  PHY_VARS_gNB *gNB;
  int32_t * rxdataF;
  void (*slotFunc)(int32_t* data, int slot,  void * scopeData);
} scopeData_t;

int load_softscope(char *exectype, void *initarg);
int end_forms(void) ;
#endif
