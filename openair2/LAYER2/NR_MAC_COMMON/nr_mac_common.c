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
 * \brief Common MAC/PHY functions for NR UE and gNB
 * \author  Florian Kaltenberger and Raymond Knopp
 * \date 2019
 * \version 0.1
 * \company Eurecom, NTUST
 * \email: florian.kalteberger@eurecom.fr, raymond.knopp@eurecom.fr
 * @ingroup _mac

 */

#include "LAYER2/NR_MAC_gNB/mac_proto.h"

nr_bandentry_t nr_bandtable[] = {
  {1,   1920000, 1980000, 2110000, 2170000, 20, 422000, 100},
  {2,   1850000, 1910000, 1930000, 1990000, 20, 386000, 100},
  {3,   1710000, 1785000, 1805000, 1880000, 20, 361000, 100},
  {5,    824000,  849000,  869000,  894000, 20, 173800, 100},
  {7,   2500000, 2570000, 2620000, 2690000, 20, 524000, 100},
  {8,    880000,  915000,  925000,  960000, 20, 185000, 100},
  {12,   698000,  716000,  728000,  746000, 20, 145800, 100},
  {20,   832000,  862000,  791000,  821000, 20, 158200, 100},
  {25,  1850000, 1915000, 1930000, 1995000, 20, 386000, 100},
  {28,   703000,  758000,  758000,  813000, 20, 151600, 100},
  {34,  2010000, 2025000, 2010000, 2025000, 20, 402000, 100},
  {38,  2570000, 2620000, 2570000, 2630000, 20, 514000, 100},
  {39,  1880000, 1920000, 1880000, 1920000, 20, 376000, 100},
  {40,  2300000, 2400000, 2300000, 2400000, 20, 460000, 100},
  {41,  2496000, 2690000, 2496000, 2690000,  3, 499200,  15},
  {41,  2496000, 2690000, 2496000, 2690000,  6, 499200,  30},
  {50,  1432000, 1517000, 1432000, 1517000, 20, 286400, 100},
  {51,  1427000, 1432000, 1427000, 1432000, 20, 285400, 100},
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
  {79,  4400000, 5000000, 4400000, 5000000,  1, 693334,  15},
  {79,  4400000, 5000000, 4400000, 5000000,  2, 693334,  30},
  {80,  1710000, 1785000,     000,     000, 20, 342000, 100},
  {81,   860000,  915000,     000,     000, 20, 176000, 100},
  {82,   832000,  862000,     000,     000, 20, 166400, 100},
  {83,   703000,  748000,     000,     000, 20, 140600, 100},
  {84,  1920000, 1980000,     000,     000, 20, 384000, 100},
  {86,  1710000, 1785000,     000,     000, 20, 342000, 100},
  {257,26500000,29500000,26500000,29500000,  1,2054166,  60},
  {257,26500000,29500000,26500000,29500000,  2,2054167, 120},
  {258,24250000,27500000,24250000,27500000,  1,2016667,  60},
  {258,24250000,27500000,24250000,27500000,  2,2016667, 120},
  {260,37000000,40000000,37000000,40000000,  1,2229166,  60},
  {260,37000000,40000000,37000000,40000000,  2,2229167, 120},
  {261,27500000,28350000,27500000,28350000,  1,2070833,  60},
  {261,27500000,28350000,27500000,28350000,  2,2070833, 120}
};

#define NR_BANDTABLE_SIZE (sizeof(nr_bandtable)/sizeof(nr_bandentry_t))

void get_band(uint64_t downlink_frequency,
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

    LOG_I( PHY, "DL frequency %"PRIu64": band %d, frame_type %d, UL frequency %"PRIu64"\n",
         downlink_frequency, *current_band, *current_type, downlink_frequency+*current_offset);

    AssertFatal(*current_band != 0,
	    "Can't find EUTRA band for frequency %lu\n", downlink_frequency);
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

uint32_t to_nrarfcn(int nr_bandP,
                    uint64_t dl_CarrierFreq,
                    uint8_t scs_index,
                    uint32_t bw)
{
  uint64_t dl_CarrierFreq_by_1k = dl_CarrierFreq / 1000;
  int bw_kHz = bw / 1000;
  int scs_khz = 15<<scs_index;
  int i;
  uint32_t nrarfcn, delta_arfcn;

  LOG_I(MAC,"Searching for nr band %d DL Carrier frequency %llu bw %u\n",nr_bandP,(long long unsigned int)dl_CarrierFreq,bw);
  AssertFatal(nr_bandP <= 261, "nr_band %d > 260\n", nr_bandP);
  for (i = 0; i < NR_BANDTABLE_SIZE && nr_bandtable[i].band != nr_bandP; i++);

  // selection of correct Deltaf raster according to SCS
  if ( (nr_bandtable[i].deltaf_raster != 100) && (nr_bandtable[i].deltaf_raster != scs_khz))
   i++;

  AssertFatal(dl_CarrierFreq_by_1k >= nr_bandtable[i].dl_min,
        "Band %d, bw %u : DL carrier frequency %llu kHz < %llu\n",
	      nr_bandP, bw, (long long unsigned int)dl_CarrierFreq_by_1k,
	      (long long unsigned int)nr_bandtable[i].dl_min);
  AssertFatal(dl_CarrierFreq_by_1k <= (nr_bandtable[i].dl_max - bw_kHz),
        "Band %d, dl_CarrierFreq %llu bw %u: DL carrier frequency %llu kHz > %llu\n",
	      nr_bandP, (long long unsigned int)dl_CarrierFreq,bw, (long long unsigned int)dl_CarrierFreq_by_1k,
	      (long long unsigned int)(nr_bandtable[i].dl_max - bw_kHz));
 
  int deltaFglobal = 60;

  if (dl_CarrierFreq < 3e9) deltaFglobal = 15;
  if (dl_CarrierFreq < 24.25e9) deltaFglobal = 5;

  // This is equation before Table 5.4.2.1-1 in 38101-1-f30
  // F_REF=F_REF_Offs + deltaF_Global(N_REF-NREF_REF_Offs)
  nrarfcn =  (((dl_CarrierFreq_by_1k - nr_bandtable[i].dl_min)/deltaFglobal)+nr_bandtable[i].N_OFFs_DL);

  delta_arfcn = nrarfcn - nr_bandtable[i].N_OFFs_DL;
  if(delta_arfcn%(nr_bandtable[i].step_size)!=0)
    AssertFatal(1==0,"dl_CarrierFreq %lu corresponds to %u which is not on the raster for step size %lu",
                dl_CarrierFreq,nrarfcn,nr_bandtable[i].step_size);

  return nrarfcn;
}


uint64_t from_nrarfcn(int nr_bandP,
                      uint8_t scs_index,
                      uint32_t dl_nrarfcn)
{
  int i;
  int deltaFglobal = 5;
  int scs_khz = 15<<scs_index;
  uint32_t delta_arfcn;

  if (dl_nrarfcn > 599999 && dl_nrarfcn < 2016667)
    deltaFglobal = 15; 
  if (dl_nrarfcn > 2016666 && dl_nrarfcn < 3279166)
    deltaFglobal = 60; 
  
  AssertFatal(nr_bandP <= 261, "nr_band %d > 260\n", nr_bandP);
  for (i = 0; i < NR_BANDTABLE_SIZE && nr_bandtable[i].band != nr_bandP; i++);
  AssertFatal(dl_nrarfcn>=nr_bandtable[i].N_OFFs_DL,"dl_nrarfcn %u < N_OFFs_DL %llu\n",dl_nrarfcn, (long long unsigned int)nr_bandtable[i].N_OFFs_DL);
 
  // selection of correct Deltaf raster according to SCS
  if ( (nr_bandtable[i].deltaf_raster != 100) && (nr_bandtable[i].deltaf_raster != scs_khz))
   i++;

  delta_arfcn = dl_nrarfcn - nr_bandtable[i].N_OFFs_DL;
  if(delta_arfcn%(nr_bandtable[i].step_size)!=0)
    AssertFatal(1==0,"dl_nrarfcn %u is not on the raster for step size %lu",dl_nrarfcn,nr_bandtable[i].step_size);

  LOG_I(PHY,"Computing dl_frequency (pointA %llu => %llu (dlmin %llu, nr_bandtable[%d].N_OFFs_DL %llu))\n",
	(unsigned long long)dl_nrarfcn,
	(unsigned long long)(1000*(nr_bandtable[i].dl_min + (dl_nrarfcn - nr_bandtable[i].N_OFFs_DL) * deltaFglobal)),
	(unsigned long long)nr_bandtable[i].dl_min,
	i,
	(unsigned long long)nr_bandtable[i].N_OFFs_DL); 

  return 1000*(nr_bandtable[i].dl_min + (dl_nrarfcn - nr_bandtable[i].N_OFFs_DL) * deltaFglobal);
}


int32_t get_nr_uldl_offset(int nr_bandP)
{
  int i;

  for (i = 0; i < NR_BANDTABLE_SIZE && nr_bandtable[i].band != nr_bandP; i++);

  AssertFatal(i < NR_BANDTABLE_SIZE, "i %d >= BANDTABLE_SIZE %ld\n", i, NR_BANDTABLE_SIZE);

  return (nr_bandtable[i].dl_min - nr_bandtable[i].ul_min);
}


void nr_get_tbs_dl(nfapi_nr_dl_tti_pdsch_pdu *pdsch_pdu,
		   int x_overhead) {

  LOG_D(MAC, "TBS calculation\n");

  nfapi_nr_dl_tti_pdsch_pdu_rel15_t *pdsch_rel15 = &pdsch_pdu->pdsch_pdu_rel15;
  uint16_t N_PRB_oh = x_overhead;
  uint8_t N_PRB_DMRS = (pdsch_rel15->dmrsConfigType == NFAPI_NR_DMRS_TYPE1)?6:4; //This only works for antenna port 1000
  uint8_t N_sh_symb = pdsch_rel15->NrOfSymbols;
  uint8_t Imcs = pdsch_rel15->mcsIndex[0];
  uint16_t N_RE_prime = NR_NB_SC_PER_RB*N_sh_symb - N_PRB_DMRS - N_PRB_oh;
  LOG_D(MAC, "N_RE_prime %d for %d symbols %d DMRS per PRB and %d overhead\n", N_RE_prime, N_sh_symb, N_PRB_DMRS, N_PRB_oh);

  uint16_t R;
  uint32_t TBS=0;
  uint8_t table_idx, Qm;

  /*uint8_t mcs_table = config.pdsch_config.mcs_table.value;
  uint8_t ss_type = params_rel15.search_space_type;
  uint8_t dci_format = params_rel15.dci_format;
  get_table_idx(mcs_table, dci_format, rnti_type, ss_type);*/
  table_idx = 0;
  R = nr_get_code_rate_dl(Imcs, table_idx);
  Qm = nr_get_Qm_dl(Imcs, table_idx);

  TBS = nr_compute_tbs(Qm,
                       R,
		       pdsch_rel15->rbSize,
		       N_sh_symb,
		       N_PRB_DMRS,
		       N_PRB_oh,
		       pdsch_rel15->nrOfLayers)>>3;

  pdsch_rel15->targetCodeRate[0] = R;
  pdsch_rel15->qamModOrder[0] = Qm;
  pdsch_rel15->TBSize[0] = TBS;
  //  pdsch_rel15->nb_mod_symbols = N_RE_prime*pdsch_rel15->n_prb*pdsch_rel15->nb_codewords;
  pdsch_rel15->mcsTable[0] = table_idx;

  LOG_D(MAC, "TBS %d bytes: N_PRB_DMRS %d N_sh_symb %d N_PRB_oh %d R %d Qm %d table %d nb_symbols %d\n",
  TBS, N_PRB_DMRS, N_sh_symb, N_PRB_oh, R, Qm, table_idx,N_RE_prime*pdsch_rel15->rbSize*pdsch_rel15->NrOfCodewords );
}

//Table 5.1.3.1-1 of 38.214
uint16_t Table_51311[29][2] = {{2,120},{2,157},{2,193},{2,251},{2,308},{2,379},{2,449},{2,526},{2,602},{2,679},{4,340},{4,378},{4,434},{4,490},{4,553},{4,616},
		{4,658},{6,438},{6,466},{6,517},{6,567},{6,616},{6,666},{6,719},{6,772},{6,822},{6,873}, {6,910}, {6,948}};

//Table 5.1.3.1-2 of 38.214
// Imcs values 20 and 26 have been multiplied by 2 to avoid the floating point
uint16_t Table_51312[28][2] = {{2,120},{2,193},{2,308},{2,449},{2,602},{4,378},{4,434},{4,490},{4,553},{4,616},{4,658},{6,466},{6,517},{6,567},{6,616},{6,666},
		{6,719},{6,772},{6,822},{6,873},{8,1365},{8,711},{8,754},{8,797},{8,841},{8,885},{8,1833},{8,948}};

//Table 5.1.3.1-3 of 38.214
uint16_t Table_51313[29][2] = {{2,30},{2,40},{2,50},{2,64},{2,78},{2,99},{2,120},{2,157},{2,193},{2,251},{2,308},{2,379},{2,449},{2,526},{2,602},{4,340},
		{4,378},{4,434},{4,490},{4,553},{4,616},{6,438},{6,466},{6,517},{6,567},{6,616},{6,666}, {6,719}, {6,772}};

//Table 6.1.4.1-1 of 38.214 TODO fix for tp-pi2BPSK
uint16_t Table_61411[28][2] = {{2,120},{2,157},{2,193},{2,251},{2,308},{2,379},{2,449},{2,526},{2,602},{2,679},{4,340},{4,378},{4,434},{4,490},{4,553},{4,616},
		{4,658},{6,466},{6,517},{6,567},{6,616},{6,666},{6,719},{6,772},{6,822},{6,873}, {6,910}, {6,948}};

//Table 6.1.4.1-2 of 38.214 TODO fix for tp-pi2BPSK
uint16_t Table_61412[28][2] = {{2,30},{2,40},{2,50},{2,64},{2,78},{2,99},{2,120},{2,157},{2,193},{2,251},{2,308},{2,379},{2,449},{2,526},{2,602},{2,679},
		{4,378},{4,434},{4,490},{4,553},{4,616},{4,658},{4,699},{4,772},{6,567},{6,616},{6,666}, {6,772}};



uint8_t nr_get_Qm_dl(uint8_t Imcs, uint8_t table_idx) {
  switch(table_idx) {
    case 0:
      return (Table_51311[Imcs][0]);
    break;

    case 1:
      return (Table_51312[Imcs][0]);
    break;

    case 2:
      return (Table_51313[Imcs][0]);
    break;

    default:
      AssertFatal(0, "Invalid MCS table index %d (expected in range [1,3])\n", table_idx);
  }
}

uint32_t nr_get_code_rate_dl(uint8_t Imcs, uint8_t table_idx) {
  switch(table_idx) {
    case 0:
      return (Table_51311[Imcs][1]);
    break;

    case 1:
      return (Table_51312[Imcs][1]);
    break;

    case 2:
      return (Table_51313[Imcs][1]);
    break;

    default:
      AssertFatal(0, "Invalid MCS table index %d (expected in range [1,3])\n", table_idx);
  }
}

uint8_t nr_get_Qm_ul(uint8_t Imcs, uint8_t table_idx) {
  switch(table_idx) {
    case 0:
      return (Table_51311[Imcs][0]);
    break;

    case 1:
      return (Table_51312[Imcs][0]);
    break;

    case 2:
      return (Table_51313[Imcs][0]);
    break;

    case 3:
      return (Table_61411[Imcs][0]);
    break;

    case 4:
      return (Table_61412[Imcs][0]);
    break;

    default:
      AssertFatal(0, "Invalid MCS table index %d (expected in range [1,2])\n", table_idx);
  }
}

uint32_t nr_get_code_rate_ul(uint8_t Imcs, uint8_t table_idx) {
  switch(table_idx) {
    case 0:
      return (Table_51311[Imcs][1]);
    break;

    case 1:
      return (Table_51312[Imcs][1]);
    break;

    case 2:
      return (Table_51313[Imcs][1]);
    break;

    case 3:
      return (Table_61411[Imcs][1]);
    break;

    case 4:
      return (Table_61412[Imcs][1]);
    break;

    default:
      AssertFatal(0, "Invalid MCS table index %d (expected in range [1,2])\n", table_idx);
  }
}

static inline uint8_t is_codeword_disabled(uint8_t format, uint8_t Imcs, uint8_t rv) {
  return ((format==NFAPI_NR_DL_DCI_FORMAT_1_1)&&(Imcs==26)&&(rv==1));
}

static inline uint8_t get_table_idx(uint8_t mcs_table, uint8_t dci_format, uint8_t rnti_type, uint8_t ss_type) {
  if ((mcs_table == NFAPI_NR_MCS_TABLE_QAM256) && (dci_format == NFAPI_NR_DL_DCI_FORMAT_1_1) && ((rnti_type==NFAPI_NR_RNTI_C)||(rnti_type==NFAPI_NR_RNTI_CS)))
    return 2;
  else if ((mcs_table == NFAPI_NR_MCS_TABLE_QAM64_LOW_SE) && (rnti_type!=NFAPI_NR_RNTI_new) && (rnti_type==NFAPI_NR_RNTI_C) && (ss_type==NFAPI_NR_SEARCH_SPACE_TYPE_UE_SPECIFIC))
    return 3;
  else if (rnti_type==NFAPI_NR_RNTI_new)
    return 3;
  else if ((mcs_table == NFAPI_NR_MCS_TABLE_QAM256) && (rnti_type==NFAPI_NR_RNTI_CS) && (dci_format == NFAPI_NR_DL_DCI_FORMAT_1_1))
    return 2; // Condition mcs_table not configured in sps_config necessary here but not yet implemented
  /*else if((mcs_table == NFAPI_NR_MCS_TABLE_QAM64_LOW_SE) &&  (rnti_type==NFAPI_NR_RNTI_CS))
   *  table_idx = 3;
   * Note: the commented block refers to the case where the mcs_table is from sps_config*/
  else
    return 1;
}

int get_num_dmrs(uint16_t dmrs_mask ) {

  int num_dmrs=0;

  for (int i=0;i<16;i++) num_dmrs+=((dmrs_mask>>i)&1);
  return(num_dmrs);
}

uint16_t nr_dci_size(nr_dci_format_t format,
		     nr_rnti_type_t rnti_type,
		     uint16_t N_RB) {

  uint16_t size = 0;

  switch(format) {
    /*Only sizes for 0_0 and 1_0 are correct at the moment*/
    case NR_UL_DCI_FORMAT_0_0:
      /// fixed: Format identifier 1, Hop flag 1, MCS 5, NDI 1, RV 2, HARQ PID 4, PUSCH TPC 2 Time Domain assgnmt 4 --20
      size += 20;
      size += (uint8_t)ceil( log2( (N_RB*(N_RB+1))>>1 ) ); // Freq domain assignment -- hopping scenario to be updated
      size += nr_dci_size(NR_DL_DCI_FORMAT_1_0, rnti_type, N_RB) - size; // Padding to match 1_0 size
      // UL/SUL indicator assumed to be 0
      break;

    case NR_UL_DCI_FORMAT_0_1:
      /// fixed: Format identifier 1, MCS 5, NDI 1, RV 2, HARQ PID 4, PUSCH TPC 2, SRS request 2 --17
      size += 17;
      // Carrier indicator
      // UL/SUL indicator
      // BWP Indicator
      // Freq domain assignment
      // Time domain assignment
      // VRB to PRB mapping
      // Frequency Hopping flag
      // 1st DAI
      // 2nd DAI
      // SRS resource indicator
      // Precoding info and number of layers
      // Antenna ports
      // CSI request
      // CBGTI
      // PTRS - DMRS association
      // beta offset indicator
      // DMRS sequence init
      break;

    case NR_DL_DCI_FORMAT_1_0:
      /// fixed: Format identifier 1, VRB2PRB 1, MCS 5, NDI 1, RV 2, HARQ PID 4, DAI 2, PUCCH TPC 2, PUCCH RInd 3, PDSCH to HARQ TInd 3 Time Domain assgnmt 4 -- 28
      size += 28;
      size += (uint8_t)ceil( log2( (N_RB*(N_RB+1))>>1 ) ); // Freq domain assignment
      break;

    case NR_DL_DCI_FORMAT_1_1:
      // Carrier indicator
      size += 1; // Format identifier
      // BWP Indicator
      // Freq domain assignment
      // Time domain assignment
      // VRB to PRB mapping
      // PRB bundling size indicator
      // Rate matching indicator
      // ZP CSI-RS trigger
      /// TB1- MCS 5, NDI 1, RV 2
      size += 8;
      // TB2
      size += 4 ;  // HARQ PID
      // DAI
      size += 2; // TPC PUCCH
      size += 3; // PUCCH resource indicator
      size += 3; // PDSCH to HARQ timing indicator
      // Antenna ports
      // Tx Config Indication
      size += 2; // SRS request
      // CBGTI
      // CBGFI
      size += 1; // DMRS sequence init
      break;

    case NR_DL_DCI_FORMAT_2_0:
      break;

    case NR_DL_DCI_FORMAT_2_1:
      break;

    case NR_DL_DCI_FORMAT_2_2:
      break;

    case NR_DL_DCI_FORMAT_2_3:
      break;

    default:
      AssertFatal(1==0, "Invalid NR DCI format %d\n", format);
  }

  return size;
}

int tdd_period_to_num[8] = {500,625,1000,1250,2000,2500,5000,10000};

int is_nr_DL_slot(NR_ServingCellConfigCommon_t *scc,slot_t slot) {

  int period,period1,period2=0;

  if (scc->tdd_UL_DL_ConfigurationCommon==NULL) return(1);

  if (scc->tdd_UL_DL_ConfigurationCommon->pattern1.ext1 &&
      scc->tdd_UL_DL_ConfigurationCommon->pattern1.ext1->dl_UL_TransmissionPeriodicity_v1530)
    period1 = 3000+*scc->tdd_UL_DL_ConfigurationCommon->pattern1.ext1->dl_UL_TransmissionPeriodicity_v1530;
  else
    period1 = tdd_period_to_num[scc->tdd_UL_DL_ConfigurationCommon->pattern1.dl_UL_TransmissionPeriodicity];
			       
  if (scc->tdd_UL_DL_ConfigurationCommon->pattern2) {
    if (scc->tdd_UL_DL_ConfigurationCommon->pattern2->ext1 &&
	scc->tdd_UL_DL_ConfigurationCommon->pattern2->ext1->dl_UL_TransmissionPeriodicity_v1530)
      period2 = 3000+*scc->tdd_UL_DL_ConfigurationCommon->pattern2->ext1->dl_UL_TransmissionPeriodicity_v1530;
    else
      period2 = tdd_period_to_num[scc->tdd_UL_DL_ConfigurationCommon->pattern2->dl_UL_TransmissionPeriodicity];
  }    
  period = period1+period2;
  int scs=scc->tdd_UL_DL_ConfigurationCommon->referenceSubcarrierSpacing;
  int slots=period*(1<<scs)/1000;
  int slots1=period1*(1<<scs)/1000;
  int slot_in_period = slot % slots;
  if (slot_in_period < slots1) return(slot_in_period <= scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSlots ? 1 : 0);
  else return(slot_in_period <= slots1+scc->tdd_UL_DL_ConfigurationCommon->pattern2->nrofDownlinkSlots ? 1 : 0);    
}

int is_nr_UL_slot(NR_ServingCellConfigCommon_t *scc,slot_t slot) {

  int period,period1,period2=0;

  if (scc->tdd_UL_DL_ConfigurationCommon==NULL) return(1);

  if (scc->tdd_UL_DL_ConfigurationCommon->pattern1.ext1 &&
      scc->tdd_UL_DL_ConfigurationCommon->pattern1.ext1->dl_UL_TransmissionPeriodicity_v1530)
    period1 = 3000+*scc->tdd_UL_DL_ConfigurationCommon->pattern1.ext1->dl_UL_TransmissionPeriodicity_v1530;
  else
    period1 = tdd_period_to_num[scc->tdd_UL_DL_ConfigurationCommon->pattern1.dl_UL_TransmissionPeriodicity];
			       
  if (scc->tdd_UL_DL_ConfigurationCommon->pattern2) {
    if (scc->tdd_UL_DL_ConfigurationCommon->pattern2->ext1 &&
	scc->tdd_UL_DL_ConfigurationCommon->pattern2->ext1->dl_UL_TransmissionPeriodicity_v1530)
      period2 = 3000+*scc->tdd_UL_DL_ConfigurationCommon->pattern2->ext1->dl_UL_TransmissionPeriodicity_v1530;
    else
      period2 = tdd_period_to_num[scc->tdd_UL_DL_ConfigurationCommon->pattern2->dl_UL_TransmissionPeriodicity];
  }    
  period = period1+period2;
  int scs=scc->tdd_UL_DL_ConfigurationCommon->referenceSubcarrierSpacing;
  int slots=period*(1<<scs)/1000;
  int slots1=period1*(1<<scs)/1000;
  int slot_in_period = slot % slots;
  if (slot_in_period < slots1) return(slot_in_period >= scc->tdd_UL_DL_ConfigurationCommon->pattern1.nrofDownlinkSlots ? 1 : 0);
  else return(slot_in_period >= slots1+scc->tdd_UL_DL_ConfigurationCommon->pattern2->nrofDownlinkSlots ? 1 : 0);    
}

int16_t fill_dmrs_mask(NR_PDSCH_Config_t *pdsch_Config,int dmrs_TypeA_Position,int NrOfSymbols) {

  int l0;
  if (dmrs_TypeA_Position == NR_ServingCellConfigCommon__dmrs_TypeA_Position_pos2) l0=2;
  else if (dmrs_TypeA_Position == NR_ServingCellConfigCommon__dmrs_TypeA_Position_pos3) l0=3;
  else AssertFatal(1==0,"Illegal dmrs_TypeA_Position %d\n",(int)dmrs_TypeA_Position);
  if (pdsch_Config == NULL) { // Initial BWP
    return(1<<l0);
  }
  else {
    if (pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeA &&
	pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeA->present == NR_SetupRelease_DMRS_DownlinkConfig_PR_setup) {
      // Relative to start of slot
      NR_DMRS_DownlinkConfig_t *dmrs_config = (NR_DMRS_DownlinkConfig_t *)pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeA->choice.setup;
      AssertFatal(NrOfSymbols>1 && NrOfSymbols < 15,"Illegal NrOfSymbols %d\n",NrOfSymbols);
      int pos2=0;
      if (dmrs_config->maxLength == NULL) {
	// this is Table 7.4.1.1.2-3: PDSCH DM-RS positions l for single-symbol DM-RS
	if (dmrs_config->dmrs_AdditionalPosition == NULL) pos2=1;
	else if (dmrs_config->dmrs_AdditionalPosition && *dmrs_config->dmrs_AdditionalPosition == NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos0 )
	  return(1<<l0);
	
	
	switch (NrOfSymbols) {
	case 2 :
	case 3 :
	case 4 :
	case 5 :
	case 6 :
	case 7 :
	  AssertFatal(1==0,"Incoompatible NrOfSymbols %d and dmrs_Additional_Position %d\n",
		      NrOfSymbols,(int)*dmrs_config->dmrs_AdditionalPosition);
	  break;
	case 8 :
	case 9:
	  return(1<<l0 | 1<<7);
	  break;
	case 10:
	case 11:
	  if (*dmrs_config->dmrs_AdditionalPosition==NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos1)
	    return(1<<l0 | 1<<9);
	  else
	    return(1<<l0 | 1<<6 | 1<<9);
	  break;
	case 12:
	  if (*dmrs_config->dmrs_AdditionalPosition==NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos1)
	    return(1<<l0 | 1<<9);
	  else if (pos2==1)
	    return(1<<l0 | 1<<6 | 1<<9);
	  else if (*dmrs_config->dmrs_AdditionalPosition==NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos3)
	    return(1<<l0 | 1<<5 | 1<<8 | 1<<11);
	  break;
	case 13:
	case 14:
	  if (*dmrs_config->dmrs_AdditionalPosition==NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos1)
	    return(1<<l0 | 1<<11);
	  else if (pos2==1)
	    return(1<<l0 | 1<<7 | 1<<11);
	  else if (*dmrs_config->dmrs_AdditionalPosition==NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos3)
	    return(1<<l0 | 1<<5 | 1<<8 | 1<<11);
	  break;
	}
      }
      else {
	// Table 7.4.1.1.2-4: PDSCH DM-RS positions l for double-symbol DM-RS.
	AssertFatal(NrOfSymbols>3,"Illegal NrOfSymbols %d for len2 DMRS\n",NrOfSymbols);
	if (NrOfSymbols < 10) return(1<<l0);
	if (NrOfSymbols < 13 && *dmrs_config->dmrs_AdditionalPosition==NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos0) return(1<<l0);
	if (NrOfSymbols < 13 && *dmrs_config->dmrs_AdditionalPosition!=NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos0) return(1<<l0 | 1<<8);
	if (*dmrs_config->dmrs_AdditionalPosition!=NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos0) return(1<<l0);
	if (*dmrs_config->dmrs_AdditionalPosition!=NR_DMRS_DownlinkConfig__dmrs_AdditionalPosition_pos1) return(1<<l0 | 1<<10);
      }
    }
    else if (pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeB &&
	     pdsch_Config->dmrs_DownlinkForPDSCH_MappingTypeA->present == NR_SetupRelease_DMRS_DownlinkConfig_PR_setup) {
      // Relative to start of PDSCH resource
      AssertFatal(1==0,"TypeB DMRS not supported yet\n");
    }
  }
  AssertFatal(1==0,"Shouldn't get here\n");
  return(-1);
}
