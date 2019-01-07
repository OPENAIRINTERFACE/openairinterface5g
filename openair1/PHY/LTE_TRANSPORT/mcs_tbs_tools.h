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

#define modOrder(I_MCS,I_TBS) ((I_MCS-I_TBS)*2+2) // Find modulation order from I_TBS and I_MCS

/** \fn unsigned char I_TBS2I_MCS(unsigned char I_TBS);
\brief This function maps I_tbs to I_mcs according to Table 7.1.7.1-1 in 3GPP TS 36.213 V8.6.0. Where there is two supported modulation orders for the same I_TBS then either high or low modulation is chosen by changing the equality of the two first comparisons in the if-else statement.
\param I_TBS Index of Transport Block Size
\return I_MCS given I_TBS
*/
unsigned char I_TBS2I_MCS(unsigned char I_TBS);

/** \fn unsigned char SE2I_TBS(float SE,
        unsigned char N_PRB,
        unsigned char symbPerRB);
\brief This function maps a requested throughput in number of bits to I_tbs. The throughput is calculated as a function of modulation order, RB allocation and number of symbols per RB. The mapping orginates in the "Transport block size table" (Table 7.1.7.2.1-1 in 3GPP TS 36.213 V8.6.0)
\param SE Spectral Efficiency (before casting to integer, multiply by 1024, remember to divide result by 1024!)
\param N_PRB Number of PhysicalResourceBlocks allocated \sa lte_frame_parms->N_RB_DL
\param symbPerRB Number of symbols per resource block allocated to this channel
\return I_TBS given an SE and an N_PRB
*/
unsigned char SE2I_TBS(float SE,
                       unsigned char N_PRB,
                       unsigned char symbPerRB);
