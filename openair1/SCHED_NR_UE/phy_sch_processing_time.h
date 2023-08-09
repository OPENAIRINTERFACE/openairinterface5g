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

/*! \file       phy_sch_processing_time.h
 \brief         tables for UE PDSCH processing and UE PUSCH preparation procedure time
                from TS 38.214 Physical procedures for data v15.6.0
 \author        Guido Casati
 \date 	        2019
 \version       0.1
 \company       Fraunhofer IIS
 \email:        guido.casati@iis.fraunhofer.de
 \note
 \warning
*/

/* TS 38.214 Table 5.3-1: PDSCH processing time for PDSCH processing capability 1
//   corresponding to the PDSCH decoding time N_1 [symbols]
// where mu corresponds to the one of {mu_PDCCH, mu_PDSCH, mu_UL}
//   resulting with the largest T_proc_1
// where mu_PDCCH is the SCS of PDCCH scheduling PDSCH
//   mu_PDSCH is the SCS of the scheduled PDSCH
//   mu_UL is the SCS of the UL channel with which the HARQ-ACK is to be transmitted
// column A is N_1 corresponding to dmrs-AdditionalPosition pos0 in DMRS-DownlinkConfig
//   in both dmrs-DownlinkForPDSCH-MappingTypeA and dmrs-DownlinkForPDSCH-MappingTypeB
// column B is N_1 corresponding to corresponds to dmrs-AdditionalPosition !0
//   in DMRS-DownlinkConfig in both dmrs-DownlinkForPDSCH-MappingTypeA,
//   dmrs-DownlinkForPDSCH-MappingTypeB or if the higher layer param is not configured
//   when PDSCH DM-RS position l1 for the additional DM-RS is l1 = 1,2
// column C is N_1 corresponding to corresponds to dmrs-AdditionalPosition !0
//   in DMRS-DownlinkConfig in both dmrs-DownlinkForPDSCH-MappingTypeA,
//   dmrs-DownlinkForPDSCH-MappingTypeB or if the higher layer param is not configured
//   when PDSCH DM-RS position l1 for the additional DM-RS is != 1,2

*/
static const int8_t pdsch_N_1_capability_1[4][4] = {
/* mu      A            B            C   */
{  0,      8,           14,          13  },
{  1,      10,          13,          13  },
{  2,      17,          20,          20  },
{  3,      20,          24,          24  },
};

/* TS 38.214 Table 5.3-2: PDSCH processing time for PDSCH processing capability 2
//   corresponding to the PDSCH decoding time N_1 [symbols]
// where mu corresponds to the one of {mu_PDCCH, mu_PDSCH, mu_UL}
//   resulting with the largest T_proc_1
// where mu_PDCCH is the SCS of PDCCH scheduling PDSCH
//   mu_PDSCH is the SCS of the scheduled PDSCH
//   mu_UL is the SCS of the UL channel with which the HARQ-ACK is to be transmitted
// column A is N_1 corresponding to dmrs-AdditionalPosition pos0 in DMRS-DownlinkConfig in both
//   dmrs-DownlinkForPDSCH-MappingTypeA and dmrs-DownlinkForPDSCH-MappingTypeB
// mu == 2 is for FR1 only
*/
static const float pdsch_N_1_capability_2[3][2] = {
/* mu      A */   
{  0,      3   },
{  1,      4.5 },
{  2,      9   },
};

/* TS 38.214 Table 6.4-1: PUSCH preparation time for PUSCH timing capability 1
//   corresponding to the PUSCH preparation time N_2 [symbols]
// where mu corresponds to the one of {mu_DL, mu_UL}
//   resulting with the largest T_proc_2
// where mu_DL is the SCS with which the PDCCH
//   carrying the DCI scheduling the PUSCH was transmitted
//   mu_UL is the SCS of the UL channel with which PUSCH to be transmitted
*/
static const int8_t pusch_N_2_timing_capability_1[4][2] = {
/* mu      N_2   */
{  0,      10 },
{  1,      12 },
{  2,      23 },
{  3,      36 },
};

/* TS 38.214 Table 6.4-2: PUSCH preparation time for PUSCH timing capability 2
//   corresponding to the PUSCH preparation time N_2 [symbols]
// where mu corresponds to the one of {mu_DL, mu_UL}
//   resulting with the largest T_proc_2
// where mu_DL is the SCS with which the PDCCH
//   carrying the DCI scheduling the PUSCH was transmitted
//   mu_UL is the SCS of the UL channel with which PUSCH to be transmitted
// mu == 2 is for FR1 only
*/
static const float pusch_N_2_timing_capability_2[3][2] = {
/* mu      N_2   */
{  0,      5   },
{  1,      5.5 },
{  2,      11  },
};

/* TS 38.133 Table 8.6.2-1: BWP switch delay capability 1
//   corresponding to the PUSCH preparation time d_2_2 [slots]
// where mu corresponds to the one of {mu_DL, mu_UL}
//   resulting with the largest T_proc_2
// where mu_DL is the SCS with which the PDCCH
//   carrying the DCI scheduling the PUSCH was transmitted
//   mu_UL is the SCS of the UL channel with which PUSCH to be transmitted
*/
static const float pusch_d_2_2_timing_capability_1[4][2] = {
/* mu      d_2_2   */
{  0,      1   },
{  1,      2   },
{  2,      3   },
{  3,      6   },
};

/* TS 38.133 Table 8.6.2-1: BWP switch delay capability 2
//   corresponding to the PUSCH preparation time d_2_2 [slots]
// where mu corresponds to the one of {mu_DL, mu_UL}
//   resulting with the largest T_proc_2
// where mu_DL is the SCS with which the PDCCH
//   carrying the DCI scheduling the PUSCH was transmitted
//   mu_UL is the SCS of the UL channel with which PUSCH to be transmitted
*/
static const float pusch_d_2_2_timing_capability_2[4][2] = {
/* mu      d_2_2   */
{  0,      3   },
{  1,      5   },
{  2,      9   },
{  3,      18  },
};
