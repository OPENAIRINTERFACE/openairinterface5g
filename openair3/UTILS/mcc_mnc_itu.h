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

/*****************************************************************************

Source      mcc_mnc.h

Version     0.1

Date        {2014/10/02

Product

Subsystem

Author      Lionel GAUTHIER

Description Defines the MCC/MNC list delivered by the ITU

*****************************************************************************/
#ifndef __MCC_MNC_H__
#define __MCC_MNC_H__


typedef struct mcc_mnc_list_s {
  uint16_t mcc;
  char     mnc[4];
} mcc_mnc_list_t;

int find_mnc_length(const char mcc_digit1P,
                    const char mcc_digit2P,
                    const char mcc_digit3P,
                    const char mnc_digit1P,
                    const char mnc_digit2P,
                    const char mnc_digit3P);
#endif
