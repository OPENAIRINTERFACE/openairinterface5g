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

/*! \file nr_mac_common.c
 * \brief Common MAC functions for NR UE and gNB
 * \author  Florian Kaltenberger and Raymond Knopp
 * \date 2019
 * \version 0.1
 * \company Eurecom, NTUST
 * \email: florian.kalteberger@eurecom.fr, raymond.knopp@eurecom.fr
 * @ingroup _mac

 */

#include "LAYER2/NR_MAC_gNB/mac_proto.h"

nr_bandentry_t nr_bandtable[] = {
  {1,  1920000, 1980000, 2110000, 2170000, 20, 422000},
  {2,  1850000, 1910000, 1930000, 1990000, 20, 386000},
  {3,  1710000, 1785000, 1805000, 1880000, 20, 361000},
  {5,   824000,  849000,  869000,  894000, 20, 173800},
  {7,  2500000, 2570000, 2620000, 2690000, 20, 524000},
  {8,   880000,  915000,  925000,  960000, 20, 185000},
  {12,  698000,  716000,  728000,  746000, 20, 145800},
  {20,  832000,  862000,  791000,  821000, 20, 158200},
  {25, 1850000, 1915000, 1930000, 1995000, 20, 386000},
  {28,  703000,  758000,  758000,  813000, 20, 151600},
  {34, 2010000, 2025000, 2010000, 2025000, 20, 402000},
  {38, 2570000, 2620000, 2570000, 2630000, 20, 514000},
  {39, 1880000, 1920000, 1880000, 1920000, 20, 376000},
  {40, 2300000, 2400000, 2300000, 2400000, 20, 460000},
  {41, 2496000, 2690000, 2496000, 2690000,  3, 499200},
  {50, 1432000, 1517000, 1432000, 1517000, 20, 286400},
  {51, 1427000, 1432000, 1427000, 1432000, 20, 285400},
  {66, 1710000, 1780000, 2110000, 2200000, 20, 422000},
  {70, 1695000, 1710000, 1995000, 2020000, 20, 399000},
  {71,  663000,  698000,  617000,  652000, 20, 123400},
  {74, 1427000, 1470000, 1475000, 1518000, 20, 295000},
  {75,     000,     000, 1432000, 1517000, 20, 286400},
  {76,     000,     000, 1427000, 1432000, 20, 285400},
  {77, 3300000, 4200000, 3300000, 4200000,  1, 620000},
  {78, 3300000, 3800000, 3300000, 3800000,  1, 620000},
  {79, 4400000, 5000000, 4400000, 5000000,  2, 693334},
  {80, 1710000, 1785000,     000,     000, 20, 342000},
  {81,  860000,  915000,     000,     000, 20, 176000},
  {82,  832000,  862000,     000,     000, 20, 166400},
  {83,  703000,  748000,     000,     000, 20, 140600},
  {84, 1920000, 1980000,     000,     000, 20, 384000},
  {86, 1710000, 1785000,     000,     000, 20, 342000}
};

#define NR_BANDTABLE_SIZE (sizeof(nr_bandtable)/sizeof(nr_bandentry_t))

void get_band(uint32_t downlink_frequency,
              uint16_t *current_band,
              int32_t *current_offset,
              lte_frame_type_t *current_type)
{
    int ind;
    uint64_t center_frequency_khz;
    uint64_t center_freq_diff_khz;
    uint64_t dl_freq_khz = downlink_frequency/1000;

    center_freq_diff_khz = 999999999999999999; // 2^64
    *current_band = 0;

    for ( ind=0;
          ind < sizeof(nr_bandtable) / sizeof(nr_bandtable[0]);
          ind++) {

      LOG_I(PHY, "Scanning band %d, dl_min %"PRIu64", ul_min %"PRIu64"\n", nr_bandtable[ind].band, nr_bandtable[ind].dl_min,nr_bandtable[ind].ul_min);

      if ( nr_bandtable[ind].dl_min <= dl_freq_khz && nr_bandtable[ind].dl_max >= dl_freq_khz ) {

        center_frequency_khz = (nr_bandtable[ind].dl_max + nr_bandtable[ind].dl_min)/2;

        if (abs(dl_freq_khz - center_frequency_khz) < center_freq_diff_khz){

          *current_band = nr_bandtable[ind].band;
	        *current_offset = (nr_bandtable[ind].ul_min - nr_bandtable[ind].dl_min)*1000;
          center_freq_diff_khz = abs(dl_freq_khz - center_frequency_khz);

	        if (*current_offset == 0)
	          *current_type = TDD;
	        else
	          *current_type = FDD;
        }
      }
    }

    LOG_I( PHY, "DL frequency %"PRIu32": band %d, frame_type %d, UL frequency %"PRIu32"\n",
         downlink_frequency, *current_band, *current_type, downlink_frequency+*current_offset);

    AssertFatal(*current_band != 0,
	    "Can't find EUTRA band for frequency %u\n", downlink_frequency);
}

uint32_t to_nrarfcn(int nr_bandP,
                    uint64_t dl_CarrierFreq,
                    uint32_t bw)
{
  uint64_t dl_CarrierFreq_by_1k = dl_CarrierFreq / 1000;
  int bw_kHz = bw / 1000;

  int i;

  LOG_I(MAC,"Searching for nr band %d DL Carrier frequency %llu bw %u\n",nr_bandP,(long long unsigned int)dl_CarrierFreq,bw);
  AssertFatal(nr_bandP < 86, "nr_band %d > 86\n", nr_bandP);
  for (i = 0; i < 30 && nr_bandtable[i].band != nr_bandP; i++);

  AssertFatal(dl_CarrierFreq_by_1k >= nr_bandtable[i].dl_min,
        "Band %d, bw %u : DL carrier frequency %llu kHz < %llu\n",
	      nr_bandP, bw, (long long unsigned int)dl_CarrierFreq_by_1k,
	      (long long unsigned int)nr_bandtable[i].dl_min);
  AssertFatal(dl_CarrierFreq_by_1k <= (nr_bandtable[i].dl_max - bw_kHz),
        "Band %d, dl_CarrierFreq %llu bw %u: DL carrier frequency %llu kHz > %llu\n",
	      nr_bandP, (long long unsigned int)dl_CarrierFreq,bw, (long long unsigned int)dl_CarrierFreq_by_1k,
	      (long long unsigned int)(nr_bandtable[i].dl_max - bw_kHz));
 
  int deltaFglobal;

  if (dl_CarrierFreq < 3e9) deltaFglobal = 5;
  else                      deltaFglobal = 15;

  // This is equation before Table 5.4.2.1-1 in 38101-1-f30
  // F_REF=F_REF_Offs + deltaF_Global(N_REF-NREF_REF_Offs)
  return (((dl_CarrierFreq_by_1k - nr_bandtable[i].dl_min)/deltaFglobal) +
	  nr_bandtable[i].N_OFFs_DL);
}


uint64_t from_nrarfcn(int nr_bandP,
                      uint32_t dl_nrarfcn)
{
  int i;
  int deltaFglobal;

  if (nr_bandP < 77 || nr_bandP > 79) deltaFglobal = 5;
  else                                deltaFglobal = 15;
  
  AssertFatal(nr_bandP < 87, "nr_band %d > 86\n", nr_bandP);
  for (i = 0; i < 31 && nr_bandtable[i].band != nr_bandP; i++);
  AssertFatal(dl_nrarfcn>=nr_bandtable[i].N_OFFs_DL,"dl_nrarfcn %u < N_OFFs_DL %llu\n",dl_nrarfcn, (long long unsigned int)nr_bandtable[i].N_OFFs_DL);
 
  return 1000*(nr_bandtable[i].dl_min + (dl_nrarfcn - nr_bandtable[i].N_OFFs_DL) * deltaFglobal);
}


int32_t get_nr_uldl_offset(int nr_bandP)
{
  int i;

  for (i = 0; i < NR_BANDTABLE_SIZE && nr_bandtable[i].band != nr_bandP; i++);

  AssertFatal(i < NR_BANDTABLE_SIZE, "i %d >= BANDTABLE_SIZE %ld\n", i, NR_BANDTABLE_SIZE);

  return (nr_bandtable[i].dl_min - nr_bandtable[i].ul_min);
}
