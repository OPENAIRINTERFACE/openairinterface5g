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

/*! \file prach_utils.h
 * \brief computation of some PRACH variables used in both MAC and PHY
 * \author R. Knopp
 * \date 2020
 * \version 0.1
 * \company Eurecom
 * \email: raymond.knopp@eurecom.fr, florian.kaltenberger@eurecom.fr, oscar.tonelli@yahoo.it
 * \note
 * \warning
 */
uint8_t get_prach_fmt(int prach_ConfigIndex,int frame_type);

uint8_t get_prach_prb_offset(int frame_type,
                             int tdd_config,
                             int N_RB_UL,
                             uint8_t prach_ConfigIndex,
                             uint8_t n_ra_prboffset,
                             uint8_t tdd_mapindex, uint16_t Nf);
int is_prach_subframe0(int tdd_config,int frame_type,uint8_t prach_ConfigIndex,uint32_t frame, uint8_t subframe);
