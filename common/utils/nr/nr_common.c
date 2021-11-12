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

/* \file config_ue.c
 * \brief common utility functions for NR (gNB and UE)
 * \author R. Knopp,
 * \date 2019
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */

#include <stdint.h>
#include "assertions.h"
#include "nr_common.h"

const char *duplex_mode[]={"FDD","TDD"};

// Table 5.2-1 NR operating bands in FR1 & FR2 (3GPP TS 38.101)
// Table 5.4.2.3-1 Applicable NR-ARFCN per operating band in FR1 & FR2 (3GPP TS 38.101)
// Notes:
// - N_OFFs for bands from 80 to 89 and band 95 is referred to UL
// - Frequencies are expressed in KHz
// - col: NR_band ul_min  ul_max  dl_min  dl_max  step  N_OFFs_DL  deltaf_raster
nr_bandentry_t nr_bandtable[] = {
  {1,   1920000, 1980000, 2110000, 2170000, 20, 422000, 100},
  {2,   1850000, 1910000, 1930000, 1990000, 20, 386000, 100},
  {3,   1710000, 1785000, 1805000, 1880000, 20, 361000, 100},
  {5,    824000,  849000,  869000,  894000, 20, 173800, 100},
  {7,   2500000, 2570000, 2620000, 2690000, 20, 524000, 100},
  {8,    880000,  915000,  925000,  960000, 20, 185000, 100},
  {12,   698000,  716000,  729000,  746000, 20, 145800, 100},
  {14,   788000,  798000,  758000,  768000, 20, 151600, 100},
  {18,   815000,  830000,  860000,  875000, 20, 172000, 100},
  {20,   832000,  862000,  791000,  821000, 20, 158200, 100},
  {25,  1850000, 1915000, 1930000, 1995000, 20, 386000, 100},
  {26,   814000,  849000,  859000,  894000, 20, 171800, 100},
  {28,   703000,  758000,  758000,  813000, 20, 151600, 100},
  {29,      000,     000,  717000,  728000, 20, 143400, 100},
  {30,  2305000, 2315000, 2350000, 2360000, 20, 470000, 100},
  {34,  2010000, 2025000, 2010000, 2025000, 20, 402000, 100},
  {38,  2570000, 2620000, 2570000, 2630000, 20, 514000, 100},
  {39,  1880000, 1920000, 1880000, 1920000, 20, 376000, 100},
  {40,  2300000, 2400000, 2300000, 2400000, 20, 460000, 100},
  {41,  2496000, 2690000, 2496000, 2690000,  3, 499200,  15},
  {41,  2496000, 2690000, 2496000, 2690000,  6, 499200,  30},
  {47,  5855000, 5925000, 5855000, 5925000,  1, 790334,  15},
  //{48,  3550000, 3700000, 3550000, 3700000,  1, 636667,  15},
  //{48,  3550000, 3700000, 3550000, 3700000,  2, 636668,  30},
  {50,  1432000, 1517000, 1432000, 1517000, 20, 286400, 100},
  {51,  1427000, 1432000, 1427000, 1432000, 20, 285400, 100},
  {53,  2483500, 2495000, 2483500, 2495000, 20, 496700, 100},
  {65,  1920000, 2010000, 2110000, 2200000, 20, 422000, 100},
  {66,  1710000, 1780000, 2110000, 2200000, 20, 422000, 100},
  {70,  1695000, 1710000, 1995000, 2020000, 20, 399000, 100},
  {71,   663000,  698000,  617000,  652000, 20, 123400, 100},
  {74,  1427000, 1470000, 1475000, 1518000, 20, 295000, 100},
  {75,      000,     000, 1432000, 1517000, 20, 286400, 100},
  {76,      000,     000, 1427000, 1432000, 20, 285400, 100},
  {77,  3300000, 4200000, 3300000, 4200000,  1, 620000,  15},
  {77,  3300000, 4200000, 3300000, 4200000,  2, 620000,  30},
  {78,  3300000, 3800000, 3300000, 3800000,  1, 620000,  15},
  {78,  3300000, 3800000, 3300000, 3800000,  2, 620000,  30},
  {79,  4400010, 5000000, 4400010, 5000000,  1, 693334,  15},
  {79,  4400010, 5000000, 4400010, 5000000,  2, 693334,  30},
  {80,  1710000, 1785000,     000,     000, 20, 342000, 100},
  {81,   880000,  915000,     000,     000, 20, 176000, 100},
  {82,   832000,  862000,     000,     000, 20, 166400, 100},
  {83,   703000,  748000,     000,     000, 20, 140600, 100},
  {84,  1920000, 1980000,     000,     000, 20, 384000, 100},
  {86,  1710000, 1785000,     000,     000, 20, 342000, 100},
  {89,   824000,  849000,     000,     000, 20, 342000, 100},
  {90,  2496000, 2690000, 2496000, 2690000, 3,  499200,  15},
  {90,  2496000, 2690000, 2496000, 2690000, 6,  499200,  30},
  {90,  2496000, 2690000, 2496000, 2690000, 20, 499200, 100},
  {91,   832000,  862000, 1427000, 1432000, 20, 285400, 100},
  {92,   832000,  862000, 1432000, 1517000, 20, 286400, 100},
  {93,   880000,  915000, 1427000, 1432000, 20, 285400, 100},
  {94,   880000,  915000, 1432000, 1517000, 20, 286400, 100},
  {95,  2010000, 2025000,     000,     000, 20, 402000, 100},
  {257,26500020,29500000,26500020,29500000,  1,2054166,  60},
  {257,26500080,29500000,26500080,29500000,  2,2054167, 120},
  {258,24250080,27500000,24250080,27500000,  1,2016667,  60},
  {258,24250080,27500000,24250080,27500000,  2,2016667, 120},
  {260,37000020,40000000,37000020,40000000,  1,2229166,  60},
  {260,37000080,40000000,37000080,40000000,  2,2229167, 120},
  {261,27500040,28350000,27500040,28350000,  1,2070833,  60},
  {261,27500040,28350000,27500040,28350000,  2,2070833, 120}
};

uint16_t get_band(uint64_t downlink_frequency, int32_t delta_duplex)
{
  const uint64_t dl_freq_khz = downlink_frequency / 1000;
  const int32_t  delta_duplex_khz = delta_duplex / 1000;

  uint64_t center_freq_diff_khz = 999999999999999999; // 2^64
  uint16_t current_band = 0;

  for (int ind = 0; ind < nr_bandtable_size; ind++) {

    if (dl_freq_khz < nr_bandtable[ind].dl_min || dl_freq_khz > nr_bandtable[ind].dl_max)
      continue;

    int32_t current_offset_khz = nr_bandtable[ind].ul_min - nr_bandtable[ind].dl_min;

    if (current_offset_khz != delta_duplex_khz)
      continue;

    uint64_t center_frequency_khz = (nr_bandtable[ind].dl_max + nr_bandtable[ind].dl_min) / 2;

    if (abs(dl_freq_khz - center_frequency_khz) < center_freq_diff_khz){
      current_band = nr_bandtable[ind].band;
      center_freq_diff_khz = abs(dl_freq_khz - center_frequency_khz);
    }
  }

  printf("DL frequency %"PRIu64": band %d, UL frequency %"PRIu64"\n",
        downlink_frequency, current_band, downlink_frequency+delta_duplex);

  AssertFatal(current_band != 0, "Can't find EUTRA band for frequency %"PRIu64" and duplex_spacing %u\n", downlink_frequency, delta_duplex);

  return current_band;
}

const size_t nr_bandtable_size = sizeof(nr_bandtable) / sizeof(nr_bandentry_t);

int NRRIV2BW(int locationAndBandwidth,int N_RB) {
  int tmp = locationAndBandwidth/N_RB;
  int tmp2 = locationAndBandwidth%N_RB;
  if (tmp <= ((N_RB>>1)+1) && (tmp+tmp2)<N_RB) return(tmp+1);
  else                      return(N_RB+1-tmp);

}

int NRRIV2PRBOFFSET(int locationAndBandwidth,int N_RB) {
  int tmp = locationAndBandwidth/N_RB;
  int tmp2 = locationAndBandwidth%N_RB;
  if (tmp <= ((N_RB>>1)+1) && (tmp+tmp2)<N_RB) return(tmp2);
  else                      return(N_RB-1-tmp2);
}

/* TS 38.214 ch. 6.1.2.2.2 - Resource allocation type 1 for DL and UL */
int PRBalloc_to_locationandbandwidth0(int NPRB,int RBstart,int BWPsize) {
  AssertFatal(NPRB>0 && (NPRB + RBstart <= BWPsize),"Illegal NPRB/RBstart Configuration (%d,%d) for BWPsize %d\n",NPRB,RBstart,BWPsize);

  if (NPRB <= 1+(BWPsize>>1)) return(BWPsize*(NPRB-1)+RBstart);
  else                        return(BWPsize*(BWPsize+1-NPRB) + (BWPsize-1-RBstart));
}

int PRBalloc_to_locationandbandwidth(int NPRB,int RBstart) {
  return(PRBalloc_to_locationandbandwidth0(NPRB,RBstart,275));
}

/// Target code rate tables indexed by Imcs
/* TS 38.214 table 5.1.3.1-1 - MCS index table 1 for PDSCH */
uint16_t nr_target_code_rate_table1[29] = {120, 157, 193, 251, 308, 379, 449, 526, 602, 679, 340, 378, 434, 490, 553, \
                                            616, 658, 438, 466, 517, 567, 616, 666, 719, 772, 822, 873, 910, 948};

/* TS 38.214 table 5.1.3.1-2 - MCS index table 2 for PDSCH */
// Imcs values 20 and 26 have been multiplied by 2 to avoid the floating point
uint16_t nr_target_code_rate_table2[28] = {120, 193, 308, 449, 602, 378, 434, 490, 553, 616, 658, 466, 517, 567, \
                                            616, 666, 719, 772, 822, 873, 1365, 711, 754, 797, 841, 885, 1833, 948};

/* TS 38.214 table 5.1.3.1-3 - MCS index table 3 for PDSCH */
uint16_t nr_target_code_rate_table3[29] = {30, 40, 50, 64, 78, 99, 120, 157, 193, 251, 308, 379, 449, 526, 602, 340, \
                                            378, 434, 490, 553, 616, 438, 466, 517, 567, 616, 666, 719, 772};

uint16_t nr_tbs_table[93] = {24, 32, 40, 48, 56, 64, 72, 80, 88, 96, 104, 112, 120, 128, 136, 144, 152, 160, 168, 176, 184, 192, 208, 224, 240, 256, 272, 288, 304, 320, \
                              336, 352, 368, 384, 408, 432, 456, 480, 504, 528, 552, 576, 608, 640, 672, 704, 736, 768, 808, 848, 888, 928, 984, 1032, 1064, 1128, 1160, 1192, 1224, 1256, \
                              1288, 1320, 1352, 1416, 1480, 1544, 1608, 1672, 1736, 1800, 1864, 1928, 2024, 2088, 2152, 2216, 2280, 2408, 2472, 2536, 2600, 2664, 2728, 2792, 2856, 2976, \
                              3104, 3240, 3368, 3496, 3624, 3752, 3824};

uint8_t nr_get_Qm(uint8_t Imcs, uint8_t table_idx) {
  switch(table_idx) {
    case 1:
      return (((Imcs<10)||(Imcs==29))?2:((Imcs<17)||(Imcs==30))?4:((Imcs<29)||(Imcs==31))?6:-1);
    break;

    case 2:
      return (((Imcs<5)||(Imcs==28))?2:((Imcs<11)||(Imcs==29))?4:((Imcs<20)||(Imcs==30))?6:((Imcs<28)||(Imcs==31))?8:-1);
    break;

    case 3:
      return (((Imcs<15)||(Imcs==29))?2:((Imcs<21)||(Imcs==30))?4:((Imcs<29)||(Imcs==31))?6:-1);
    break;

    default:
      AssertFatal(0, "Invalid MCS table index %d (expected in range [1,3])\n", table_idx);
      return(0);
      break;
  }
}

uint32_t nr_get_code_rate(uint8_t Imcs, uint8_t table_idx) {
  switch(table_idx) {
    case 1:
      return (nr_target_code_rate_table1[Imcs]);
    break;

    case 2:
      return (nr_target_code_rate_table2[Imcs]);
    break;

    case 3:
      return (nr_target_code_rate_table3[Imcs]);
    break;

    default:
      AssertFatal(0, "Invalid MCS table index %d (expected in range [1,3])\n", table_idx);
      return(0);
      break;
  }
}


void get_coreset_rballoc(uint8_t *FreqDomainResource,int *n_rb,int *rb_offset) {

  uint8_t count=0, start=0, start_set=0;

  uint64_t bitmap = (((uint64_t)FreqDomainResource[0])<<37)|
    (((uint64_t)FreqDomainResource[1])<<29)|
    (((uint64_t)FreqDomainResource[2])<<21)|
    (((uint64_t)FreqDomainResource[3])<<13)|
    (((uint64_t)FreqDomainResource[4])<<5)|
    (((uint64_t)FreqDomainResource[5])>>3);

  for (int i=0; i<45; i++)
    if ((bitmap>>(44-i))&1) {
      count++;
      if (!start_set) {
        start = i;
        start_set = 1;
      }
    }
  *rb_offset = 6*start;
  *n_rb = 6*count;
}


int get_dmrs_port(int nl, uint16_t dmrs_ports) {

  if (dmrs_ports == 0) return 0; // dci 1_0
  int p = -1;
  int found = -1;
  for (int i=0; i<12; i++) { // loop over dmrs ports
    if((dmrs_ports>>i)&0x01) { // check if current bit is 1
      found++;
      if (found == nl) { // found antenna port number corresponding to current layer
        p = i;
        break;
      }
    }
  }
  AssertFatal(p>-1,"No dmrs port corresponding to layer %d found\n",nl);
  return p;
}

int get_num_dmrs(uint16_t dmrs_mask ) {

  int num_dmrs=0;

  for (int i=0;i<16;i++) num_dmrs+=((dmrs_mask>>i)&1);
  return(num_dmrs);
}

lte_frame_type_t get_frame_type(uint16_t current_band, uint8_t scs_index)
{
  lte_frame_type_t current_type;
  int32_t delta_duplex = get_delta_duplex(current_band, scs_index);

  if (delta_duplex == 0)
    current_type = TDD;
  else
    current_type = FDD;

  LOG_I(NR_MAC, "NR band %d, duplex mode %s, duplex spacing = %d KHz\n", current_band, duplex_mode[current_type], delta_duplex);

  return current_type;
}

// Computes the duplex spacing (either positive or negative) in KHz
int32_t get_delta_duplex(int nr_bandP, uint8_t scs_index)
{
  int nr_table_idx = get_nr_table_idx(nr_bandP, scs_index);

  int32_t delta_duplex = (nr_bandtable[nr_table_idx].ul_min - nr_bandtable[nr_table_idx].dl_min);

  LOG_I(NR_MAC, "NR band duplex spacing is %d KHz (nr_bandtable[%d].band = %d)\n", delta_duplex, nr_table_idx, nr_bandtable[nr_table_idx].band);

  return delta_duplex;
}

uint16_t config_bandwidth(int mu, int nb_rb, int nr_band)
{

  if (nr_band < 100)  { //FR1
   switch(mu) {
    case 0 :
      if (nb_rb<=25)
        return 5;
      if (nb_rb<=52)
        return 10;
      if (nb_rb<=79)
        return 15;
      if (nb_rb<=106)
        return 20;
      if (nb_rb<=133)
        return 25;
      if (nb_rb<=160)
        return 30;
      if (nb_rb<=216)
        return 40;
      if (nb_rb<=270)
        return 50;
      AssertFatal(1==0,"Number of DL resource blocks %d undefined for mu %d and band %d\n", nb_rb, mu, nr_band);
      break;
    case 1 :
      if (nb_rb<=11)
        return 5;
      if (nb_rb<=24)
        return 10;
      if (nb_rb<=38)
        return 15;
      if (nb_rb<=51)
        return 20;
      if (nb_rb<=65)
        return 25;
      if (nb_rb<=78)
        return 30;
      if (nb_rb<=106)
        return 40;
      if (nb_rb<=133)
        return 50;
      if (nb_rb<=162)
        return 60;
      if (nb_rb<=189)
        return 70;
      if (nb_rb<=217)
        return 80;
      if (nb_rb<=245)
        return 90;
      if (nb_rb<=273)
        return 100;
      AssertFatal(1==0,"Number of DL resource blocks %d undefined for mu %d and band %d\n", nb_rb, mu, nr_band);
      break;
    case 2 :
      if (nb_rb<=11)
        return 10;
      if (nb_rb<=18)
        return 15;
      if (nb_rb<=24)
        return 20;
      if (nb_rb<=31)
        return 25;
      if (nb_rb<=38)
        return 30;
      if (nb_rb<=51)
        return 40;
      if (nb_rb<=65)
        return 50;
      if (nb_rb<=79)
        return 60;
      if (nb_rb<=93)
        return 70;
      if (nb_rb<=107)
        return 80;
      if (nb_rb<=121)
        return 90;
      if (nb_rb<=135)
        return 100;
      AssertFatal(1==0,"Number of DL resource blocks %d undefined for mu %d and band %d\n", nb_rb, mu, nr_band);
      break;
    default:
      AssertFatal(1==0,"Numerology %d undefined for band %d in FR1\n", mu,nr_band);
   }
  }
  else {
   switch(mu) {
    case 2 :
      if (nb_rb<=66)
        return 50;
      if (nb_rb<=132)
        return 100;
      if (nb_rb<=264)
        return 200;
      AssertFatal(1==0,"Number of DL resource blocks %d undefined for mu %d and band %d\n", nb_rb, mu, nr_band);
      break;
    case 3 :
      if (nb_rb<=32)
        return 50;
      if (nb_rb<=66)
        return 100;
      if (nb_rb<=132)
        return 200;
      if (nb_rb<=264)
        return 400;
      AssertFatal(1==0,"Number of DL resource blocks %d undefined for mu %d and band %d\n", nb_rb, mu, nr_band);
      break;
    default:
      AssertFatal(1==0,"Numerology %d undefined for band %d in FR1\n", mu,nr_band);
   }
  }
}

// Returns the corresponding row index of the NR table
int get_nr_table_idx(int nr_bandP, uint8_t scs_index) {
  int i, j;
  int scs_khz = 15 << scs_index;
  int supplementary_bands[] = {29,75,76,80,81,82,83,84,86,89,95};
  size_t s = sizeof(supplementary_bands)/sizeof(supplementary_bands[0]);

  for(j = 0; j < s; j++){
    if (nr_bandP == supplementary_bands[j])
      AssertFatal(0 == 1, "Band %d is a supplementary band (%d). This is not supported yet.\n", nr_bandP, supplementary_bands[j]);
  }

  AssertFatal(nr_bandP <= nr_bandtable[nr_bandtable_size-1].band, "NR band %d exceeds NR bands table maximum limit %d\n", nr_bandP, nr_bandtable[nr_bandtable_size-1].band);
  for (i = 0; i < nr_bandtable_size && nr_bandtable[i].band != nr_bandP; i++);

  // selection of correct Deltaf raster according to SCS
  if ((nr_bandtable[i].deltaf_raster != 100) && (nr_bandtable[i].deltaf_raster != scs_khz))
    i++;

  LOG_D(PHY, "NR band table index %d (Band %d, dl_min %lu, ul_min %lu)\n", i, nr_bandtable[i].band, nr_bandtable[i].dl_min,nr_bandtable[i].ul_min);

  return i;
}

int get_subband_size(int NPRB,int size) {
  // implements table  5.2.1.4-2 from 36.214
  //
  //Bandwidth part (PRBs)	Subband size (PRBs)
  // < 24	                   N/A
  //24 – 72	                   4, 8
  //73 – 144	                   8, 16
  //145 – 275	                  16, 32

  if (NPRB<24) return(1);
  if (NPRB<72) return (size==0 ? 4 : 8);
  if (NPRB<144) return (size==0 ? 8 : 16);
  if (NPRB<275) return (size==0 ? 16 : 32);
  AssertFatal(1==0,"Shouldn't get here, NPRB %d\n",NPRB);
 
}

void SLIV2SL(int SLIV,int *S,int *L) {

  int SLIVdiv14 = SLIV/14;
  int SLIVmod14 = SLIV%14;
  // Either SLIV = 14*(L-1) + S, or SLIV = 14*(14-L+1) + (14-1-S). Condition is 0 <= L <= 14-S
  if ((SLIVdiv14 + 1) >= 0 && (SLIVdiv14 <= 13-SLIVmod14)) {
    *L=SLIVdiv14+1;
    *S=SLIVmod14;
  } else  {
    *L=15-SLIVdiv14;
    *S=13-SLIVmod14;
  }

}
