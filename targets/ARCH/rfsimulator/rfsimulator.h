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
* Author and copyright: Laurent Thomas, open-cells.com
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

#ifndef __RFSIMULATOR_H
#define  __RFSIMULATOR_H
double gaussZiggurat(double mean, double variance);
void tableNor(unsigned long seed);
void rxAddInput( struct complex16 *input_sig,
                 struct complex16 *after_channel_sig,
                 int rxAnt,
                 channel_desc_t *channelDesc,
                 int nbSamples,
                 uint64_t TS,
                 uint32_t CirSize
               );

#endif
