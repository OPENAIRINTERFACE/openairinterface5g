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

/*! \file PHY/LTE_TRANSPORT/dci_nr.c
 * \brief Implements PDCCH physical channel TX/RX procedures (36.211) and DCI encoding/decoding (36.212/36.213). Current LTE compliance V8.6 2009-03.
 * \author R. Knopp, A. Mico Pereperez
 * \date 2018
 * \version 0.1
 * \company Eurecom
 * \email: knopp@eurecom.fr
 * \note
 * \warning
 */
#ifdef USER_MODE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#endif
//#include "PHY/defs.h"
#include "PHY/defs_nr_UE.h"
//#include "PHY/extern.h"
//#include "SCHED/defs.h"
//#include "SIMULATION/TOOLS/defs.h" // for taus 
#include "PHY/sse_intrin.h"

#include "assertions.h" 
#include "T.h"

//#define DEBUG_DCI_ENCODING 1
//#define DEBUG_DCI_DECODING 1
//#define DEBUG_PHY

//#define NR_LTE_PDCCH_DCI_SWITCH
#define NR_PDCCH_DCI_RUN              // activates new nr functions
#define NR_PDCCH_DCI_DEBUG            // activates NR_PDCCH_DCI_DEBUG logs
#define NR_NBR_CORESET_ACT_BWP 3      // The number of CoreSets per BWP is limited to 3 (including initial CORESET: ControlResourceId 0)
#define NR_NBR_SEARCHSPACE_ACT_BWP 10 // The number of SearSpaces per BWP is limited to 10 (including initial SEARCHSPACE: SearchSpaceId 0)

//#undef ALL_AGGREGATION

//extern uint16_t phich_reg[MAX_NUM_PHICH_GROUPS][3];
//extern uint16_t pcfich_reg[4];

/*uint32_t check_phich_reg(NR_DL_FRAME_PARMS *frame_parms,uint32_t kprime,uint8_t lprime,uint8_t mi)
{

  uint16_t i;
  uint16_t Ngroup_PHICH = (frame_parms->phich_config_common.phich_resource*frame_parms->N_RB_DL)/48;
  uint16_t mprime;
  uint16_t *pcfich_reg = frame_parms->pcfich_reg;

  if ((lprime>0) && (frame_parms->Ncp==0) )
    return(0);

  //  printf("check_phich_reg : mi %d\n",mi);

  // compute REG based on symbol
  if ((lprime == 0)||
      ((lprime==1)&&(frame_parms->nb_antenna_ports_eNB == 4)))
    mprime = kprime/6;
  else
    mprime = kprime>>2;

  // check if PCFICH uses mprime
  if ((lprime==0) &&
      ((mprime == pcfich_reg[0]) ||
       (mprime == pcfich_reg[1]) ||
       (mprime == pcfich_reg[2]) ||
       (mprime == pcfich_reg[3]))) {
#ifdef DEBUG_DCI_ENCODING
    printf("[PHY] REG %d allocated to PCFICH\n",mprime);
#endif
    return(1);
  }

  // handle Special subframe case for TDD !!!

  //  printf("Checking phich_reg %d\n",mprime);
  if (mi > 0) {
    if (((frame_parms->phich_config_common.phich_resource*frame_parms->N_RB_DL)%48) > 0)
      Ngroup_PHICH++;

    if (frame_parms->Ncp == 1) {
      Ngroup_PHICH<<=1;
    }



    for (i=0; i<Ngroup_PHICH; i++) {
      if ((mprime == frame_parms->phich_reg[i][0]) ||
          (mprime == frame_parms->phich_reg[i][1]) ||
          (mprime == frame_parms->phich_reg[i][2]))  {
#ifdef DEBUG_DCI_ENCODING
        printf("[PHY] REG %d (lprime %d) allocated to PHICH\n",mprime,lprime);
#endif
        return(1);
      }
    }
  }

  return(0);
}*/

uint16_t extract_crc(uint8_t *dci,uint8_t dci_len)
{

  uint16_t crc16;
  //  uint8_t i;

  /*
  uint8_t crc;
  crc = ((uint16_t *)dci)[DCI_LENGTH>>4];
  printf("crc1: %x, shift %d (DCI_LENGTH %d)\n",crc,DCI_LENGTH&0xf,DCI_LENGTH);
  crc = (crc>>(DCI_LENGTH&0xf));
  // clear crc bits
  ((uint16_t *)dci)[DCI_LENGTH>>4] &= (0xffff>>(16-(DCI_LENGTH&0xf)));
  printf("crc2: %x, dci0 %x\n",crc,((int16_t *)dci)[DCI_LENGTH>>4]);
  crc |= (((uint16_t *)dci)[1+(DCI_LENGTH>>4)])<<(16-(DCI_LENGTH&0xf));
  // clear crc bits
  (((uint16_t *)dci)[1+(DCI_LENGTH>>4)]) = 0;
  printf("extract_crc: crc %x\n",crc);
  */
#ifdef DEBUG_DCI_DECODING
  LOG_I(PHY,"dci_crc (%x,%x,%x), dci_len&0x7=%d\n",dci[dci_len>>3],dci[1+(dci_len>>3)],dci[2+(dci_len>>3)],
      dci_len&0x7);
#endif

  if ((dci_len&0x7) > 0) {
    ((uint8_t *)&crc16)[0] = dci[1+(dci_len>>3)]<<(dci_len&0x7) | dci[2+(dci_len>>3)]>>(8-(dci_len&0x7));
    ((uint8_t *)&crc16)[1] = dci[(dci_len>>3)]<<(dci_len&0x7) | dci[1+(dci_len>>3)]>>(8-(dci_len&0x7));
  } else {
    ((uint8_t *)&crc16)[0] = dci[1+(dci_len>>3)];
    ((uint8_t *)&crc16)[1] = dci[(dci_len>>3)];
  }

#ifdef DEBUG_DCI_DECODING
  LOG_I(PHY,"dci_crc =>%x\n",crc16);
#endif

  //  dci[(dci_len>>3)]&=(0xffff<<(dci_len&0xf));
  //  dci[(dci_len>>3)+1] = 0;
  //  dci[(dci_len>>3)+2] = 0;
  return((uint16_t)crc16);
  
}



static uint8_t d[3*(MAX_DCI_SIZE_BITS + 16) + 96];
static uint8_t w[3*3*(MAX_DCI_SIZE_BITS+16)];

void dci_encoding(uint8_t *a,
                  uint8_t A,
                  uint16_t E,
                  uint8_t *e,
                  uint16_t rnti)
{


  uint8_t D = (A + 16);
  uint32_t RCC;

#ifdef DEBUG_DCI_ENCODING
  int32_t i;
#endif
  // encode dci

#ifdef DEBUG_DCI_ENCODING
  printf("Doing DCI encoding for %d bits, e %p, rnti %x\n",A,e,rnti);
#endif

  memset((void *)d,LTE_NULL,96);

  ccodelte_encode(A,2,a,d+96,rnti);

#ifdef DEBUG_DCI_ENCODING

  for (i=0; i<16+A; i++)
    printf("%d : (%d,%d,%d)\n",i,*(d+96+(3*i)),*(d+97+(3*i)),*(d+98+(3*i)));

#endif

#ifdef DEBUG_DCI_ENCODING
  printf("Doing DCI interleaving for %d coded bits, e %p\n",D*3,e);
#endif
  RCC = sub_block_interleaving_cc(D,d+96,w);

#ifdef DEBUG_DCI_ENCODING
  printf("Doing DCI rate matching for %d channel bits, RCC %d, e %p\n",E,RCC,e);
#endif
  lte_rate_matching_cc(RCC,E,w,e);


}


uint8_t *generate_dci0(uint8_t *dci,
                       uint8_t *e,
                       uint8_t DCI_LENGTH,
                       uint8_t aggregation_level,
                       uint16_t rnti)
{

  uint16_t coded_bits;
  uint8_t dci_flip[8];

  if (aggregation_level>3) {
    printf("dci.c: generate_dci FATAL, illegal aggregation_level %d\n",aggregation_level);
    return NULL;
  }

  coded_bits = 72 * (1<<aggregation_level);

  /*

  #ifdef DEBUG_DCI_ENCODING
  for (i=0;i<1+((DCI_LENGTH+16)/8);i++)
    printf("i %d : %x\n",i,dci[i]);
  #endif
  */
  if (DCI_LENGTH<=32) {
    dci_flip[0] = dci[3];
    dci_flip[1] = dci[2];
    dci_flip[2] = dci[1];
    dci_flip[3] = dci[0];
  } else {
    dci_flip[0] = dci[7];
    dci_flip[1] = dci[6];
    dci_flip[2] = dci[5];
    dci_flip[3] = dci[4];
    dci_flip[4] = dci[3];
    dci_flip[5] = dci[2];
    dci_flip[6] = dci[1];
    dci_flip[7] = dci[0];
#ifdef DEBUG_DCI_ENCODING
    printf("DCI => %x,%x,%x,%x,%x,%x,%x,%x\n",
        dci_flip[0],dci_flip[1],dci_flip[2],dci_flip[3],
        dci_flip[4],dci_flip[5],dci_flip[6],dci_flip[7]);
#endif
  }

  dci_encoding(dci_flip,DCI_LENGTH,coded_bits,e,rnti);

  return(e+coded_bits);
}

uint32_t Y;

#define CCEBITS 72
#define CCEPERSYMBOL 33  // This is for 1200 RE
#define CCEPERSYMBOL0 22  // This is for 1200 RE
#define DCI_BITS_MAX ((2*CCEPERSYMBOL+CCEPERSYMBOL0)*CCEBITS)
#define Msymb (DCI_BITS_MAX/2)
//#define Mquad (Msymb/4)

static uint32_t bitrev_cc_dci[32] = {1,17,9,25,5,21,13,29,3,19,11,27,7,23,15,31,0,16,8,24,4,20,12,28,2,18,10,26,6,22,14,30};
static int32_t wtemp[2][Msymb];

void pdcch_interleaving(NR_DL_FRAME_PARMS *frame_parms,int32_t **z, int32_t **wbar,uint8_t n_symbols_pdcch,uint8_t mi)
{

  int32_t *wptr,*wptr2,*zptr;
  uint32_t Mquad = get_nquad(n_symbols_pdcch,frame_parms,mi);
  uint32_t RCC = (Mquad>>5), ND;
  uint32_t row,col,Kpi,index;
  int32_t i,k,a;
#ifdef RM_DEBUG
  int32_t nulled=0;
#endif

  //  printf("[PHY] PDCCH Interleaving Mquad %d (Nsymb %d)\n",Mquad,n_symbols_pdcch);
  if ((Mquad&0x1f) > 0)
    RCC++;

  Kpi = (RCC<<5);
  ND = Kpi - Mquad;

  k=0;

  for (col=0; col<32; col++) {
    index = bitrev_cc_dci[col];

    for (row=0; row<RCC; row++) {
      //printf("col %d, index %d, row %d\n",col,index,row);
      if (index>=ND) {
        for (a=0; a<frame_parms->nb_antenna_ports_eNB; a++) {
          //printf("a %d k %d\n",a,k);

          wptr = &wtemp[a][k<<2];
          zptr = &z[a][(index-ND)<<2];

          //printf("wptr=%p, zptr=%p\n",wptr,zptr);

          wptr[0] = zptr[0];
          wptr[1] = zptr[1];
          wptr[2] = zptr[2];
          wptr[3] = zptr[3];
        }

        k++;
      }

      index+=32;
    }
  }

  // permutation
  for (i=0; i<Mquad; i++) {

    for (a=0; a<frame_parms->nb_antenna_ports_eNB; a++) {

      //wptr  = &wtemp[a][i<<2];
      //wptr2 = &wbar[a][((i+frame_parms->Nid_cell)%Mquad)<<2];
      wptr = &wtemp[a][((i+frame_parms->Nid_cell)%Mquad)<<2];
      wptr2 = &wbar[a][i<<2];
      wptr2[0] = wptr[0];
      wptr2[1] = wptr[1];
      wptr2[2] = wptr[2];
      wptr2[3] = wptr[3];
    }
  }
}



#ifdef NR_PDCCH_DCI_RUN
void nr_pdcch_demapping(uint16_t *llr, uint16_t *wbar,
           NR_DL_FRAME_PARMS *frame_parms,
           uint8_t coreset_time_dur, uint32_t coreset_nbr_rb) {
/*
 * LLR contains the PDCCH for the coreset_time_dur symbols in the following sequence:
 * 
 * The REGs have to be numbered in increasing order in a time-first manner,
 * starting with 0 for the first OFDM symbol and the lowest-numbered resource
 * block in the control resource set
 *
 * |   ...    |    ...    |   ...    |
 * |  REG 3   |   REG 4   |  REG 5   |
 * |  REG 0   |   REG 1   |  REG 2   |
 * | symbol0  |  symbol1  | symbol2  |
 *
 * .................
 * ___________REG 1+2l
 * ___________REG 1+l
 * symbol 1___REG 1
 * .................
 * ___________REG 2l
 * ___________REG l
 * symbol 0___REG 0
 *
 * WBAR will contain the PDCCH organized in REGx where x will be consecutive:
 * REG 0
 * REG 1
 * REG 2
 * ...
 * REG l
 * REG 1+l
 * REG 2+l
 * ...
 *
 */

  uint32_t m,i,k,l;
  uint32_t num_re_pdcch = 12 * coreset_nbr_rb;
  i=0;
  m=0;
  for (k=0; k<num_re_pdcch; k++) {
    for (l=0; l < coreset_time_dur ; l++) {
      if ((k%12==1)||(k%12==5)||(k%12==9)){
        #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_demapping)-> k,l=(%d,%d) DM-RS PDCCH signal\n",k,l);
        #endif
      } else {
        if ((m%9)==0 && (m !=0) && (l==0)) {
          #ifdef NR_PDCCH_DCI_DEBUG
          printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_demapping)-> we have modified m: old_m=%d, new_m=%d\n",
                  m,m+(9*(coreset_time_dur-1)));
          #endif
          m=m+(9*(coreset_time_dur-1)); // to avoid overwriting m+1 when a whole REG has been completed
        }
        wbar[(l*9)+m] = llr[(l*9*coreset_nbr_rb)+i];
        #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_demapping)-> k,l=(%d,%d) i,m=(%d,%d) REG (%d) > wbar(%d,%d) \t llr[%d]->wbar[%d]\n",
                k,l,i,m, (m+(l*9)) / 9, *(char*) &wbar[m+(l*9)], *(1 + (char*) &wbar[m+(l*9)]),
                (l*9*coreset_nbr_rb)+i,m+(l*9));
        #endif
        if (l==coreset_time_dur-1) {
          i++;
          m++;
        }
      }
    }
  }
}
#endif


void pdcch_demapping(uint16_t *llr,uint16_t *wbar,NR_DL_FRAME_PARMS *frame_parms,uint8_t num_pdcch_symbols,uint8_t mi)
{

  uint32_t i, lprime;
  uint16_t kprime,kprime_mod12,mprime,symbol_offset,tti_offset,tti_offset0;
  int16_t re_offset,re_offset0;

  // This is the REG allocation algorithm from 36-211, second part of Section 6.8.5

  int Msymb2;

  switch (frame_parms->N_RB_DL) {
  case 100:
    Msymb2 = Msymb;
    break;

  case 75:
    Msymb2 = 3*Msymb/4;
    break;

  case 50:
    Msymb2 = Msymb>>1;
    break;

  case 25:
    Msymb2 = Msymb>>2;
    break;

  case 15:
    Msymb2 = Msymb*15/100;
    break;

  case 6:
    Msymb2 = Msymb*6/100;
    break;

  default:
    Msymb2 = Msymb>>2;
    break;
  }

  mprime=0;


  re_offset = 0;
  re_offset0 = 0; // counter for symbol with pilots (extracted outside!)

  for (kprime=0; kprime<frame_parms->N_RB_DL*12; kprime++) {
    for (lprime=0; lprime<num_pdcch_symbols; lprime++) {

      symbol_offset = (uint32_t)frame_parms->N_RB_DL*12*lprime;

      tti_offset = symbol_offset + re_offset;
      tti_offset0 = symbol_offset + re_offset0;

      // if REG is allocated to PHICH, skip it
      if (check_phich_reg(frame_parms,kprime,lprime,mi) == 1) {
	//        printf("dci_demapping : skipping REG %d (RE %d)\n",(lprime==0)?kprime/6 : kprime>>2,kprime);
	if ((lprime == 0)&&((kprime%6)==0))
	  re_offset0+=4;
      } else { // not allocated to PHICH/PCFICH
	//        printf("dci_demapping: REG %d\n",(lprime==0)?kprime/6 : kprime>>2);
        if (lprime == 0) {
          // first symbol, or second symbol+4 TX antennas skip pilots
          kprime_mod12 = kprime%12;

          if ((kprime_mod12 == 0) || (kprime_mod12 == 6)) {
            // kprime represents REG

            for (i=0; i<4; i++) {
              wbar[mprime] = llr[tti_offset0+i];
#ifdef DEBUG_DCI_DECODING
//              LOG_I(PHY,"PDCCH demapping mprime %d.%d <= llr %d (symbol %d re %d) -> (%d,%d)\n",mprime/4,i,tti_offset0+i,symbol_offset,re_offset0,*(char*)&wbar[mprime],*(1+(char*)&wbar[mprime]));
#endif
              mprime++;
              re_offset0++;
            }
          }
        } else if ((lprime==1)&&(frame_parms->nb_antenna_ports_eNB == 4)) {
          // LATER!!!!
        } else { // no pilots in this symbol
          kprime_mod12 = kprime%12;

          if ((kprime_mod12 == 0) || (kprime_mod12 == 4) || (kprime_mod12 == 8)) {
            // kprime represents REG
            for (i=0; i<4; i++) {
              wbar[mprime] = llr[tti_offset+i];
#ifdef DEBUG_DCI_DECODING
//              LOG_I(PHY,"PDCCH demapping mprime %d.%d <= llr %d (symbol %d re %d) -> (%d,%d)\n",mprime/4,i,tti_offset+i,symbol_offset,re_offset+i,*(char*)&wbar[mprime],*(1+(char*)&wbar[mprime]));
#endif
              mprime++;
            }
          }  // is representative
        } // no pilots case
      } // not allocated to PHICH/PCFICH

      // Stop when all REGs are copied in
      if (mprime>=Msymb2)
        break;
    } //lprime loop

    re_offset++;

  } // kprime loop
}

static uint16_t wtemp_rx[Msymb];


#ifdef NR_PDCCH_DCI_RUN
void nr_pdcch_deinterleaving(NR_DL_FRAME_PARMS *frame_parms, uint16_t *z,
          uint16_t *wbar, uint8_t coreset_time_dur, uint8_t reg_bundle_size_L,
          uint8_t coreset_interleaver_size_R, uint8_t n_shift, uint32_t coreset_nbr_rb)
{
/*
 * This function will perform deinterleaving described in 38.211 Section 7.3.2.2
 * coreset_freq_dom (bit map 45 bits: each bit indicates 6 RB in CORESET -> 1 bit MSB indicates PRB 0..6 are part of CORESET)
 * coreset_time_dur (1,2,3)
 * coreset_CCE_REG_mapping_type (interleaved, non-interleaved)
 * reg_bundle_size (2,3,6)
 */
#ifdef NR_PDCCH_DCI_DEBUG
  printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_deinterleaving)-> coreset_nbr_rb=(%lld), reg_bundle_size_L=(%d)\n",
		  coreset_nbr_rb,reg_bundle_size_L);
#endif
/*
 * First verify that CORESET is interleaved or not interleaved depending on parameter cce-REG-MappingType
 * To be done
 * if non-interleaved then do nothing: wbar table stays as it is
 * if interleaved then do this: wbar table has bundles interleaved. We have to de-interleave then
 * following procedure described in 38.211 Section 7.3.2.2:
  */
  int coreset_interleaved = 1;
  uint32_t bundle_id, bundle_interleaved, c=0 ,r=-1, k, l, i=0;
  uint32_t coreset_C = (uint32_t)(coreset_nbr_rb / (coreset_interleaver_size_R*reg_bundle_size_L));
  uint16_t *wptr;
  wptr = &wtemp_rx[0];
  z = &wtemp_rx[0];
  bundle_id=0;
  for (k=0 ; k<9*coreset_nbr_rb*coreset_time_dur; k++){
#ifdef NR_PDCCH_DCI_DEBUG
    printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_deinterleaving)-> k=%d \t coreset_interleaved=%d reg_bundle_size_L=%d coreset_C=%d coreset_interleaver_R=%d",
           k,coreset_interleaved,reg_bundle_size_L, coreset_C,coreset_interleaver_size_R);
#endif
    if (k%(9*reg_bundle_size_L)==0) {
      // calculate offset properly
      if (r==coreset_interleaver_size_R-1) {
      //if (bundle_id>=(c+1)*coreset_interleaver_size_R) {
        c++;
        r=0;
      } else{
        r++;
      }
#ifdef NR_PDCCH_DCI_DEBUG
      printf("\t --> time to modify bundle_interleaved and bundle_id --> r=%d c=%d",r,c);
#endif
      bundle_id=c*coreset_interleaver_size_R+r;
      bundle_interleaved=(r*coreset_C+c+n_shift)%(coreset_nbr_rb * coreset_time_dur/reg_bundle_size_L);
    }
    if (coreset_interleaved == 1){
      //wptr[i+(bundle_interleaved-bundle_id)*9*reg_bundle_size_L]=wbar[i];
#ifdef NR_PDCCH_DCI_DEBUG
      printf("\n\t\t\t\t\t\t\t\t\t wptr[%d] <-> wbar[%d]",i,i+(bundle_interleaved-bundle_id)*9*reg_bundle_size_L);
#endif
      wptr[i]=wbar[i+(bundle_interleaved-bundle_id)*9*reg_bundle_size_L];
#ifdef NR_PDCCH_DCI_DEBUG
      printf("\t\t bundle_id = %d \t bundle_interleaved = %d\n",bundle_id,bundle_interleaved);
#endif
      i++;
    } else {
      wptr[i]=wbar[i];
      i++;
    }
    //bundle_id=c*coreset_interleaver_size_R+r;
  }
}
#endif







void pdcch_deinterleaving(NR_DL_FRAME_PARMS *frame_parms,uint16_t *z, uint16_t *wbar,uint8_t number_pdcch_symbols,uint8_t mi)
{

  uint16_t *wptr,*zptr,*wptr2;

  uint16_t Mquad=get_nquad(number_pdcch_symbols,frame_parms,mi);
  uint32_t RCC = (Mquad>>5), ND;
  uint32_t row,col,Kpi,index;
  int32_t i,k;


  //  printf("Mquad %d, RCC %d\n",Mquad,RCC);

  if (!z) {
    printf("dci.c: pdcch_deinterleaving: FATAL z is Null\n");
    return;
  }

  // undo permutation
  for (i=0; i<Mquad; i++) {
    wptr = &wtemp_rx[((i+frame_parms->Nid_cell)%Mquad)<<2];
    wptr2 = &wbar[i<<2];

    wptr[0] = wptr2[0];
    wptr[1] = wptr2[1];
    wptr[2] = wptr2[2];
    wptr[3] = wptr2[3];
    /*    
    printf("pdcch_deinterleaving (%p,%p): quad %d (%d) -> (%d,%d %d,%d %d,%d %d,%d)\n",wptr,wptr2,i,(i+frame_parms->Nid_cell)%Mquad,
	   ((char*)wptr2)[0],
	   ((char*)wptr2)[1],
	   ((char*)wptr2)[2],
	   ((char*)wptr2)[3],
	   ((char*)wptr2)[4],
	   ((char*)wptr2)[5],
	   ((char*)wptr2)[6],
	   ((char*)wptr2)[7]);
    */

  }

  if ((Mquad&0x1f) > 0)
    RCC++;

  Kpi = (RCC<<5);
  ND = Kpi - Mquad;

  k=0;

  for (col=0; col<32; col++) {
    index = bitrev_cc_dci[col];

    for (row=0; row<RCC; row++) {
      //      printf("row %d, index %d, Nd %d\n",row,index,ND);
      if (index>=ND) {



        wptr = &wtemp_rx[k<<2];
        zptr = &z[(index-ND)<<2];

        zptr[0] = wptr[0];
        zptr[1] = wptr[1];
        zptr[2] = wptr[2];
        zptr[3] = wptr[3];

	/*        
        printf("deinterleaving ; k %d, index-Nd %d  => (%d,%d,%d,%d,%d,%d,%d,%d)\n",k,(index-ND),
               ((int8_t *)wptr)[0],
               ((int8_t *)wptr)[1],
               ((int8_t *)wptr)[2],
               ((int8_t *)wptr)[3],
               ((int8_t *)wptr)[4],
               ((int8_t *)wptr)[5],
               ((int8_t *)wptr)[6],
               ((int8_t *)wptr)[7]);
	*/
        k++;
      }

      index+=32;

    }
  }

  for (i=0; i<Mquad; i++) {
    zptr = &z[i<<2];
    /*    
    printf("deinterleaving ; quad %d  => (%d,%d,%d,%d,%d,%d,%d,%d)\n",i,
     ((int8_t *)zptr)[0],
     ((int8_t *)zptr)[1],
     ((int8_t *)zptr)[2],
     ((int8_t *)zptr)[3],
     ((int8_t *)zptr)[4],
     ((int8_t *)zptr)[5],
     ((int8_t *)zptr)[6],
     ((int8_t *)zptr)[7]);
    */  
  }

}


int32_t pdcch_qpsk_qpsk_llr(NR_DL_FRAME_PARMS *frame_parms,
                            int32_t **rxdataF_comp,
                            int32_t **rxdataF_comp_i,
                            int32_t **rho_i,
                            int16_t *pdcch_llr16,
                            int16_t *pdcch_llr8in,
                            uint8_t symbol)
{

  int16_t *rxF=(int16_t*)&rxdataF_comp[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *rxF_i=(int16_t*)&rxdataF_comp_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *rho=(int16_t*)&rho_i[0][(symbol*frame_parms->N_RB_DL*12)];
  int16_t *llr128;
  int32_t i;
  char *pdcch_llr8;
  int16_t *pdcch_llr;
  pdcch_llr8 = (char *)&pdcch_llr8in[symbol*frame_parms->N_RB_DL*12];
  pdcch_llr = &pdcch_llr16[symbol*frame_parms->N_RB_DL*12];

  //  printf("dlsch_qpsk_qpsk: symbol %d\n",symbol);

  llr128 = (int16_t*)pdcch_llr;

  if (!llr128) {
    printf("dlsch_qpsk_qpsk_llr: llr is null, symbol %d\n",symbol);
    return -1;
  }

  qpsk_qpsk(rxF,
            rxF_i,
            llr128,
            rho,
            frame_parms->N_RB_DL*12);

  //prepare for Viterbi which accepts 8 bit, but prefers 4 bit, soft input.
  for (i=0; i<(frame_parms->N_RB_DL*24); i++) {
    if (*pdcch_llr>7)
      *pdcch_llr8=7;
    else if (*pdcch_llr<-8)
      *pdcch_llr8=-8;
    else
      *pdcch_llr8 = (char)(*pdcch_llr);

    pdcch_llr++;
    pdcch_llr8++;
  }

  return(0);
}


#ifdef NR_PDCCH_DCI_RUN
int32_t nr_pdcch_llr(NR_DL_FRAME_PARMS *frame_parms, int32_t **rxdataF_comp,
		char *pdcch_llr, uint8_t symbol,uint32_t coreset_nbr_rb) {

	int16_t *rxF = (int16_t*) &rxdataF_comp[0][(symbol * frame_parms->N_RB_DL * 12)];
	int32_t i;
	char *pdcch_llr8;

	pdcch_llr8 = &pdcch_llr[2 * symbol * frame_parms->N_RB_DL * 12];

	if (!pdcch_llr8) {
		printf("pdcch_qpsk_llr: llr is null, symbol %d\n", symbol);
		return (-1);
	}
#ifdef NR_PDCCH_DCI_DEBUG
	printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_llr)-> llr logs: pdcch qpsk llr for symbol %d (pos %d), llr offset %d\n",symbol,(symbol*frame_parms->N_RB_DL*12),pdcch_llr8-pdcch_llr);
#endif
	//for (i = 0; i < (frame_parms->N_RB_DL * ((symbol == 0) ? 16 : 24)); i++) {
	for (i = 0; i < (coreset_nbr_rb * ((symbol == 0) ? 18 : 18)); i++) {

		if (*rxF > 31)
			*pdcch_llr8 = 31;
		else if (*rxF < -32)
			*pdcch_llr8 = -32;
		else
			*pdcch_llr8 = (char) (*rxF);
#ifdef NR_PDCCH_DCI_DEBUG
		    printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_llr)-> llr logs: i=%d *rxF:%d => *pdcch_llr8:%d\n",i/18,i,*rxF,*pdcch_llr8);
#endif
		rxF++;
		pdcch_llr8++;
	}

	return (0);

}
#endif



int32_t pdcch_llr(NR_DL_FRAME_PARMS *frame_parms,
                  int32_t **rxdataF_comp,
                  char *pdcch_llr,
                  uint8_t symbol)
{

  int16_t *rxF= (int16_t*) &rxdataF_comp[0][(symbol*frame_parms->N_RB_DL*12)];
  int32_t i;
  char *pdcch_llr8;

  pdcch_llr8 = &pdcch_llr[2*symbol*frame_parms->N_RB_DL*12];

  if (!pdcch_llr8) {
    printf("pdcch_qpsk_llr: llr is null, symbol %d\n",symbol);
    return(-1);
  }

  //    printf("pdcch qpsk llr for symbol %d (pos %d), llr offset %d\n",symbol,(symbol*frame_parms->N_RB_DL*12),pdcch_llr8-pdcch_llr);

  for (i=0; i<(frame_parms->N_RB_DL*((symbol==0) ? 16 : 24)); i++) {

    if (*rxF>31)
      *pdcch_llr8=31;
    else if (*rxF<-32)
      *pdcch_llr8=-32;
    else
      *pdcch_llr8 = (char)(*rxF);

    //    printf("%d %d => %d\n",i,*rxF,*pdcch_llr8);
    rxF++;
    pdcch_llr8++;
  }

  return(0);

}

//__m128i avg128P;

//compute average channel_level on each (TX,RX) antenna pair
void pdcch_channel_level(int32_t **dl_ch_estimates_ext,
                         NR_DL_FRAME_PARMS *frame_parms,
                         int32_t *avg,
                         uint8_t nb_rb)
{

  int16_t rb;
  uint8_t aatx,aarx;
#if defined(__x86_64__) || defined(__i386__)
  __m128i *dl_ch128;
  __m128i avg128P;
#elif defined(__arm__)
  int16x8_t *dl_ch128;
  int32x4_t *avg128P;
#endif
  for (aatx=0; aatx<frame_parms->nb_antenna_ports_eNB; aatx++)
    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {
      //clear average level
#if defined(__x86_64__) || defined(__i386__)
      avg128P = _mm_setzero_si128();
      dl_ch128=(__m128i *)&dl_ch_estimates_ext[(aatx<<1)+aarx][0];
#elif defined(__arm__)

#endif
      for (rb=0; rb<nb_rb; rb++) {

#if defined(__x86_64__) || defined(__i386__)
        avg128P = _mm_add_epi32(avg128P,_mm_madd_epi16(dl_ch128[0],dl_ch128[0]));
        avg128P = _mm_add_epi32(avg128P,_mm_madd_epi16(dl_ch128[1],dl_ch128[1]));
        avg128P = _mm_add_epi32(avg128P,_mm_madd_epi16(dl_ch128[2],dl_ch128[2]));
#elif defined(__arm__)

#endif
        dl_ch128+=3;
        /*
          if (rb==0) {
          print_shorts("dl_ch128",&dl_ch128[0]);
          print_shorts("dl_ch128",&dl_ch128[1]);
          print_shorts("dl_ch128",&dl_ch128[2]);
          }
        */
      }

      DevAssert( nb_rb );
      avg[(aatx<<1)+aarx] = (((int32_t*)&avg128P)[0] +
                             ((int32_t*)&avg128P)[1] +
                             ((int32_t*)&avg128P)[2] +
                             ((int32_t*)&avg128P)[3])/(nb_rb*12);

      //            printf("Channel level : %d\n",avg[(aatx<<1)+aarx]);
    }

#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif

}

#if defined(__x86_64) || defined(__i386__)
__m128i mmtmpPD0,mmtmpPD1,mmtmpPD2,mmtmpPD3;
#elif defined(__arm__)

#endif
/*
void pdcch_dual_stream_correlation(NR_DL_FRAME_PARMS *frame_parms,
                                   uint8_t symbol,
                                   int32_t **dl_ch_estimates_ext,
                                   int32_t **dl_ch_estimates_ext_i,
                                   int32_t **dl_ch_rho_ext,
                                   uint8_t output_shift)
{

  uint16_t rb;
#if defined(__x86_64__) || defined(__i386__)
  __m128i *dl_ch128,*dl_ch128i,*dl_ch_rho128;
#elif defined(__arm__)

#endif
  uint8_t aarx;

  //  printf("dlsch_dual_stream_correlation: symbol %d\n",symbol);


  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

#if defined(__x86_64__) || defined(__i386__)
    dl_ch128          = (__m128i *)&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch128i         = (__m128i *)&dl_ch_estimates_ext_i[aarx][symbol*frame_parms->N_RB_DL*12];
    dl_ch_rho128      = (__m128i *)&dl_ch_rho_ext[aarx][symbol*frame_parms->N_RB_DL*12];

#elif defined(__arm__)

#endif

    for (rb=0; rb<frame_parms->N_RB_DL; rb++) {
      // multiply by conjugated channel
#if defined(__x86_64__) || defined(__i386__)
      mmtmpPD0 = _mm_madd_epi16(dl_ch128[0],dl_ch128i[0]);
      //  print_ints("re",&mmtmpPD0);

      // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpPD1 = _mm_shufflelo_epi16(dl_ch128[0],_MM_SHUFFLE(2,3,0,1));
      mmtmpPD1 = _mm_shufflehi_epi16(mmtmpPD1,_MM_SHUFFLE(2,3,0,1));
      mmtmpPD1 = _mm_sign_epi16(mmtmpPD1,*(__m128i*)&conjugate[0]);
      //  print_ints("im",&mmtmpPD1);
      mmtmpPD1 = _mm_madd_epi16(mmtmpPD1,dl_ch128i[0]);
      // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
      mmtmpPD0 = _mm_srai_epi32(mmtmpPD0,output_shift);
      //  print_ints("re(shift)",&mmtmpPD0);
      mmtmpPD1 = _mm_srai_epi32(mmtmpPD1,output_shift);
      //  print_ints("im(shift)",&mmtmpPD1);
      mmtmpPD2 = _mm_unpacklo_epi32(mmtmpPD0,mmtmpPD1);
      mmtmpPD3 = _mm_unpackhi_epi32(mmtmpPD0,mmtmpPD1);
      //        print_ints("c0",&mmtmpPD2);
      //  print_ints("c1",&mmtmpPD3);
      dl_ch_rho128[0] = _mm_packs_epi32(mmtmpPD2,mmtmpPD3);

      //print_shorts("rx:",dl_ch128_2);
      //print_shorts("ch:",dl_ch128);
      //print_shorts("pack:",rho128);

      // multiply by conjugated channel
      mmtmpPD0 = _mm_madd_epi16(dl_ch128[1],dl_ch128i[1]);
      // mmtmpPD0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpPD1 = _mm_shufflelo_epi16(dl_ch128[1],_MM_SHUFFLE(2,3,0,1));
      mmtmpPD1 = _mm_shufflehi_epi16(mmtmpPD1,_MM_SHUFFLE(2,3,0,1));
      mmtmpPD1 = _mm_sign_epi16(mmtmpPD1,*(__m128i*)conjugate);
      mmtmpPD1 = _mm_madd_epi16(mmtmpPD1,dl_ch128i[1]);
      // mmtmpPD1 contains imag part of 4 consecutive outputs (32-bit)
      mmtmpPD0 = _mm_srai_epi32(mmtmpPD0,output_shift);
      mmtmpPD1 = _mm_srai_epi32(mmtmpPD1,output_shift);
      mmtmpPD2 = _mm_unpacklo_epi32(mmtmpPD0,mmtmpPD1);
      mmtmpPD3 = _mm_unpackhi_epi32(mmtmpPD0,mmtmpPD1);


      dl_ch_rho128[1] =_mm_packs_epi32(mmtmpPD2,mmtmpPD3);
      //print_shorts("rx:",dl_ch128_2+1);
      //print_shorts("ch:",dl_ch128+1);
      //print_shorts("pack:",rho128+1);
      // multiply by conjugated channel
      mmtmpPD0 = _mm_madd_epi16(dl_ch128[2],dl_ch128i[2]);
      // mmtmpPD0 contains real part of 4 consecutive outputs (32-bit)
      mmtmpPD1 = _mm_shufflelo_epi16(dl_ch128[2],_MM_SHUFFLE(2,3,0,1));
      mmtmpPD1 = _mm_shufflehi_epi16(mmtmpPD1,_MM_SHUFFLE(2,3,0,1));
      mmtmpPD1 = _mm_sign_epi16(mmtmpPD1,*(__m128i*)conjugate);
      mmtmpPD1 = _mm_madd_epi16(mmtmpPD1,dl_ch128i[2]);
      // mmtmpPD1 contains imag part of 4 consecutive outputs (32-bit)
      mmtmpPD0 = _mm_srai_epi32(mmtmpPD0,output_shift);
      mmtmpPD1 = _mm_srai_epi32(mmtmpPD1,output_shift);
      mmtmpPD2 = _mm_unpacklo_epi32(mmtmpPD0,mmtmpPD1);
      mmtmpPD3 = _mm_unpackhi_epi32(mmtmpPD0,mmtmpPD1);

      dl_ch_rho128[2] = _mm_packs_epi32(mmtmpPD2,mmtmpPD3);
      //print_shorts("rx:",dl_ch128_2+2);
      //print_shorts("ch:",dl_ch128+2);
      //print_shorts("pack:",rho128+2);

      dl_ch128+=3;
      dl_ch128i+=3;
      dl_ch_rho128+=3;


#elif defined(__arm__)

#endif
     }
  }
#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif

}
*/

void pdcch_detection_mrc_i(NR_DL_FRAME_PARMS *frame_parms,
                           int32_t **rxdataF_comp,
                           int32_t **rxdataF_comp_i,
                           int32_t **rho,
                           int32_t **rho_i,
                           uint8_t symbol)
{

  uint8_t aatx;

#if defined(__x86_64__) || defined(__i386__)
  __m128i *rxdataF_comp128_0,*rxdataF_comp128_1,*rxdataF_comp128_i0,*rxdataF_comp128_i1,*rho128_0,*rho128_1,*rho128_i0,*rho128_i1;
#elif defined(__arm__)
  int16x8_t *rxdataF_comp128_0,*rxdataF_comp128_1,*rxdataF_comp128_i0,*rxdataF_comp128_i1,*rho128_0,*rho128_1,*rho128_i0,*rho128_i1;
#endif
  int32_t i;

  if (frame_parms->nb_antennas_rx>1) {
    for (aatx=0; aatx<frame_parms->nb_antenna_ports_eNB; aatx++) {
      //if (frame_parms->mode1_flag && (aatx>0)) break;

#if defined(__x86_64__) || defined(__i386__)
      rxdataF_comp128_0   = (__m128i *)&rxdataF_comp[(aatx<<1)][symbol*frame_parms->N_RB_DL*12];
      rxdataF_comp128_1   = (__m128i *)&rxdataF_comp[(aatx<<1)+1][symbol*frame_parms->N_RB_DL*12];
#elif defined(__arm__)
      rxdataF_comp128_0   = (int16x8_t *)&rxdataF_comp[(aatx<<1)][symbol*frame_parms->N_RB_DL*12];
      rxdataF_comp128_1   = (int16x8_t *)&rxdataF_comp[(aatx<<1)+1][symbol*frame_parms->N_RB_DL*12];
#endif
      // MRC on each re of rb on MF output
      for (i=0; i<frame_parms->N_RB_DL*3; i++) {
#if defined(__x86_64__) || defined(__i386__)
        rxdataF_comp128_0[i] = _mm_adds_epi16(_mm_srai_epi16(rxdataF_comp128_0[i],1),_mm_srai_epi16(rxdataF_comp128_1[i],1));
#elif defined(__arm__)
        rxdataF_comp128_0[i] = vhaddq_s16(rxdataF_comp128_0[i],rxdataF_comp128_1[i]);
#endif
      }
    }

#if defined(__x86_64__) || defined(__i386__)
    rho128_0 = (__m128i *) &rho[0][symbol*frame_parms->N_RB_DL*12];
    rho128_1 = (__m128i *) &rho[1][symbol*frame_parms->N_RB_DL*12];
#elif defined(__arm__)
    rho128_0 = (int16x8_t *) &rho[0][symbol*frame_parms->N_RB_DL*12];
    rho128_1 = (int16x8_t *) &rho[1][symbol*frame_parms->N_RB_DL*12];
#endif
    for (i=0; i<frame_parms->N_RB_DL*3; i++) {
#if defined(__x86_64__) || defined(__i386__)
      rho128_0[i] = _mm_adds_epi16(_mm_srai_epi16(rho128_0[i],1),_mm_srai_epi16(rho128_1[i],1));
#elif defined(__arm__)
      rho128_0[i] = vhaddq_s16(rho128_0[i],rho128_1[i]);
#endif
    }

#if defined(__x86_64__) || defined(__i386__)
    rho128_i0 = (__m128i *) &rho_i[0][symbol*frame_parms->N_RB_DL*12];
    rho128_i1 = (__m128i *) &rho_i[1][symbol*frame_parms->N_RB_DL*12];
    rxdataF_comp128_i0   = (__m128i *)&rxdataF_comp_i[0][symbol*frame_parms->N_RB_DL*12];
    rxdataF_comp128_i1   = (__m128i *)&rxdataF_comp_i[1][symbol*frame_parms->N_RB_DL*12];
#elif defined(__arm__)
    rho128_i0 = (int16x8_t*) &rho_i[0][symbol*frame_parms->N_RB_DL*12];
    rho128_i1 = (int16x8_t*) &rho_i[1][symbol*frame_parms->N_RB_DL*12];
    rxdataF_comp128_i0   = (int16x8_t *)&rxdataF_comp_i[0][symbol*frame_parms->N_RB_DL*12];
    rxdataF_comp128_i1   = (int16x8_t *)&rxdataF_comp_i[1][symbol*frame_parms->N_RB_DL*12];

#endif
    // MRC on each re of rb on MF and rho
    for (i=0; i<frame_parms->N_RB_DL*3; i++) {
#if defined(__x86_64__) || defined(__i386__)
      rxdataF_comp128_i0[i] = _mm_adds_epi16(_mm_srai_epi16(rxdataF_comp128_i0[i],1),_mm_srai_epi16(rxdataF_comp128_i1[i],1));
      rho128_i0[i]          = _mm_adds_epi16(_mm_srai_epi16(rho128_i0[i],1),_mm_srai_epi16(rho128_i1[i],1));
#elif defined(__arm__)
      rxdataF_comp128_i0[i] = vhaddq_s16(rxdataF_comp128_i0[i],rxdataF_comp128_i1[i]);
      rho128_i0[i]          = vhaddq_s16(rho128_i0[i],rho128_i1[i]);

#endif
    }
  }

#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif
}


#ifdef NR_PDCCH_DCI_RUN
// This function will extract the mapped DM-RS PDCCH REs as per 38.211 Section 7.4.1.3.2 (Mapping to physical resources)
void nr_pdcch_extract_rbs_single(int32_t **rxdataF,
                                 int32_t **dl_ch_estimates,
                                 int32_t **rxdataF_ext,
                                 int32_t **dl_ch_estimates_ext,
                                 uint8_t symbol,
                                 uint32_t high_speed_flag,
                                 NR_DL_FRAME_PARMS *frame_parms,
                                 uint64_t coreset_freq_dom,
                                 uint32_t coreset_nbr_rb,
                                 uint32_t n_BWP_start) {

/*
 * This function is demapping DM-RS PDCCH RE
 * Implementing 38.211 Section 7.4.1.3.2 Mapping to physical resources
 * PDCCH DM-RS signals are mapped on RE a_k_l where:
 * k = 12*n + 4*kprime + 1
 * n=0,1,..
 * kprime=0,1,2
 * According to this equations, DM-RS PDCCH are mapped on k where k%12==1 || k%12==5 || k%12==9
 *
 */
  // the bitmap coreset_frq_domain contains 45 bits
  #define CORESET_FREQ_DOMAIN_BITMAP_SIZE   45
  // each bit is associated to 6 RBs
  #define BIT_TO_NBR_RB_CORESET_FREQ_DOMAIN  6
  #define NBR_RE_PER_RB_WITH_DMRS           12
  // after removing the 3 DMRS RE, the RB contains 9 RE with PDCCH
  #define NBR_RE_PER_RB_WITHOUT_DMRS         9

  uint16_t c_rb, c_rb_tmp, rb, nb_rb = 0;
  // this variable will be incremented by 1 each time a bit set to '0' is found in coreset_freq_dom bitmap
  uint16_t offset_discontiguous=0;
  uint8_t rb_count_bit,i, j, aarx, bitcnt_coreset_freq_dom=0;
  int32_t *dl_ch0, *dl_ch0_ext, *rxF, *rxF_ext;
  int nushiftmod3 = frame_parms->nushift % 3;
  uint8_t symbol_mod;

  symbol_mod = (symbol >= (7 - frame_parms->Ncp)) ? symbol - (7 - frame_parms->Ncp) : symbol;
  c_rb = n_BWP_start; // c_rb is the common resource block: RB within the BWP
  #ifdef DEBUG_DCI_DECODING
    LOG_I(PHY, "extract_rbs_single: symbol_mod %d\n",symbol_mod);
  #endif



  for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
    if (high_speed_flag == 1){
      dl_ch0 = &dl_ch_estimates[aarx][(symbol * (frame_parms->ofdm_symbol_size))];
      #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_extract_rbs_single)-> dl_ch0 = &dl_ch_estimates[aarx = (%d)][ (symbol * (frame_parms->ofdm_symbol_size (%d))) = (%d)]\n",
               aarx,frame_parms->ofdm_symbol_size,(symbol * (frame_parms->ofdm_symbol_size)));
      #endif
    } else {
      dl_ch0 = &dl_ch_estimates[aarx][0];
      #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_extract_rbs_single)-> dl_ch0 = &dl_ch_estimates[aarx = (%d)][0]\n",aarx);
      #endif
    }

    dl_ch0_ext = &dl_ch_estimates_ext[aarx][symbol * (frame_parms->N_RB_DL * NBR_RE_PER_RB_WITH_DMRS)];
    #ifdef NR_PDCCH_DCI_DEBUG
      printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_extract_rbs_single)-> dl_ch0_ext = &dl_ch_estimates_ext[aarx = (%d)][symbol * (frame_parms->N_RB_DL * 12) = (%d)]\n",
             aarx,symbol * (frame_parms->N_RB_DL * 12));
    #endif
    rxF_ext = &rxdataF_ext[aarx][symbol * (frame_parms->N_RB_DL * NBR_RE_PER_RB_WITH_DMRS)];
    #ifdef NR_PDCCH_DCI_DEBUG
      printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_extract_rbs_single)-> rxF_ext = &rxdataF_ext[aarx = (%d)][symbol * (frame_parms->N_RB_DL * 12) = (%d)]\n",
             aarx,symbol * (frame_parms->N_RB_DL * 12));
      printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_extract_rbs_single)-> (for symbol=%d, aarx=%d), symbol_mod=%d, nushiftmod3=%d \n",symbol,aarx,symbol_mod,nushiftmod3);
    #endif

/*
 * The following for loop handles treatment of PDCCH contained in table rxdataF (in frequency domain)
 * In NR the PDCCH IQ symbols are contained within RBs in the CORESET defined by higher layers which is located within the BWP
 * Lets consider that the first RB to be considered as part of the CORESET and part of the PDCCH is n_BWP_start
 * Several cases have to be handled differently as IQ symbols are situated in different parts of rxdataF:
 * 1. Number of RBs in the system bandwidth is even
 *    1.1 The RB is <  than the N_RB_DL/2 -> IQ symbols are in the second half of the rxdataF (from first_carrier_offset)
 *    1.2 The RB is >= than the N_RB_DL/2 -> IQ symbols are in the first half of the rxdataF (from element 1)
 * 2. Number of RBs in the system bandwidth is odd
 * (particular case when the RB with DC as it is treated differently: it is situated in symbol borders of rxdataF)
 *    2.1 The RB is <= than the N_RB_DL/2   -> IQ symbols are in the second half of the rxdataF (from first_carrier_offset)
 *    2.2 The RB is >  than the N_RB_DL/2+1 -> IQ symbols are in the first half of the rxdataF (from element 1 + 2nd half RB containing DC)
 *    2.3 The RB is == N_RB_DL/2+1          -> IQ symbols are in the lower border of the rxdataF for first 6 IQ element and the upper border of the rxdataF for the last 6 IQ elements
 * If the first RB containing PDCCH within the UE BWP and within the CORESET is higher than half of the system bandwidth (N_RB_DL),
 * then the IQ symbol is going to be found at the position 1+c_rb-N_RB_DL/2 in rxdataF and
 * we have to point the pointer at (1+c_rb-N_RB_DL/2) in rxdataF
 */

    #ifdef NR_PDCCH_DCI_DEBUG
      printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_extract_rbs_single)-> n_BWP_start=%d, coreset_nbr_rb=%d\n",n_BWP_start,coreset_nbr_rb);
    #endif

    for (c_rb = n_BWP_start; c_rb < (n_BWP_start + coreset_nbr_rb + (BIT_TO_NBR_RB_CORESET_FREQ_DOMAIN * offset_discontiguous)); c_rb++) {
      //c_rb_tmp = 0;
      if (((c_rb - n_BWP_start) % BIT_TO_NBR_RB_CORESET_FREQ_DOMAIN)==0) {
        bitcnt_coreset_freq_dom ++;
        while ((((coreset_freq_dom & 0x1FFFFFFFFFFF) >> (CORESET_FREQ_DOMAIN_BITMAP_SIZE - bitcnt_coreset_freq_dom)) & 0x1)== 0){ // 46 -> 45 is number of bits in coreset_freq_dom
          // next 6 RB are not part of the CORESET within the BWP as bit in coreset_freq_dom is set to 0
          bitcnt_coreset_freq_dom ++;
          //c_rb_tmp = c_rb_tmp + 6;
          c_rb = c_rb + BIT_TO_NBR_RB_CORESET_FREQ_DOMAIN;
          offset_discontiguous ++;
          #ifdef NR_PDCCH_DCI_DEBUG
            printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_extract_rbs_single)-> we entered here as coreset_freq_dom=%llx (bit %d) is 0, coreset_freq_domain is discontiguous\n",coreset_freq_dom,(46 - bitcnt_coreset_freq_dom));
          #endif
        }
      }
      //c_rb = c_rb + c_rb_tmp;

      #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_extract_rbs_single)-> c_rb=%d\n",c_rb);
      #endif
      // first we set initial conditions for pointer to rxdataF depending on the situation of the first RB within the CORESET (c_rb = n_BWP_start)
      if ((c_rb < (frame_parms->N_RB_DL >> 1)) && ((frame_parms->N_RB_DL & 1) == 0)) {
        //if RB to be treated is lower than middle system bandwidth then rxdataF pointed at (offset + c_br + symbol * ofdm_symbol_size): even case
        rxF = &rxdataF[aarx][(frame_parms->first_carrier_offset + 12 * c_rb + (symbol * (frame_parms->ofdm_symbol_size)))];
        #ifdef NR_PDCCH_DCI_DEBUG
          printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_extract_rbs_single)-> in even case c_rb (%d) is lower than half N_RB_DL -> rxF = &rxdataF[aarx = (%d)][(frame_parms->first_carrier_offset + 12 * c_rb + (symbol * (frame_parms->ofdm_symbol_size))) = (%d)]\n",
                  c_rb,aarx,(frame_parms->first_carrier_offset + 12 * c_rb + (symbol * (frame_parms->ofdm_symbol_size))));
        #endif
      }
      if ((c_rb >= (frame_parms->N_RB_DL >> 1)) && ((frame_parms->N_RB_DL & 1) == 0)) {
        // number of RBs is even  and c_rb is higher than half system bandwidth (we don't skip DC)
        // if these conditions are true the pointer has to be situated at the 1st part of the rxdataF
        rxF = &rxdataF[aarx][(12*(c_rb - (frame_parms->N_RB_DL>>1)) + (symbol * (frame_parms->ofdm_symbol_size)))]; // we point at the 1st part of the rxdataF in symbol
        #ifdef NR_PDCCH_DCI_DEBUG
          printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_extract_rbs_single)-> in even case c_rb (%d) is higher than half N_RB_DL (not DC) -> rxF = &rxdataF[aarx = (%d)][(12*(c_rb - (frame_parms->N_RB_DL>>1)) + (symbol * (frame_parms->ofdm_symbol_size))) = (%d)]\n",
               c_rb,aarx,(12*(c_rb - (frame_parms->N_RB_DL>>1)) + (symbol * (frame_parms->ofdm_symbol_size))));
        #endif
        //rxF = &rxdataF[aarx][(1 + 12*(c_rb - (frame_parms->N_RB_DL>>1)) + (symbol * (frame_parms->ofdm_symbol_size)))]; // we point at the 1st part of the rxdataF in symbol
        //#ifdef NR_PDCCH_DCI_DEBUG
        //  printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_extract_rbs_single)-> in even case c_rb (%d) is higher than half N_RB_DL (not DC) -> rxF = &rxdataF[aarx = (%d)][(1 + 12*(c_rb - (frame_parms->N_RB_DL>>1)) + (symbol * (frame_parms->ofdm_symbol_size))) = (%d)]\n",
        //         c_rb,aarx,(1 + 12*(c_rb - (frame_parms->N_RB_DL>>1)) + (symbol * (frame_parms->ofdm_symbol_size))));
        //#endif
      }
      if ((c_rb < (frame_parms->N_RB_DL >> 1)) && ((frame_parms->N_RB_DL & 1) != 0)){
        //if RB to be treated is lower than middle system bandwidth then rxdataF pointed at (offset + c_br + symbol * ofdm_symbol_size): odd case
        rxF = &rxdataF[aarx][(frame_parms->first_carrier_offset + 12 * c_rb + (symbol * (frame_parms->ofdm_symbol_size)))];
        #ifdef NR_PDCCH_DCI_DEBUG
          printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_extract_rbs_single)-> in odd case c_rb (%d) is lower or equal than half N_RB_DL -> rxF = &rxdataF[aarx = (%d)][(frame_parms->first_carrier_offset + 12 * c_rb + (symbol * (frame_parms->ofdm_symbol_size))) = (%d)]\n",
                 c_rb,aarx,(frame_parms->first_carrier_offset + 12 * c_rb + (symbol * (frame_parms->ofdm_symbol_size))));
        #endif
      }
      if ((c_rb > (frame_parms->N_RB_DL >> 1)) && ((frame_parms->N_RB_DL & 1) != 0)){
        // number of RBs is odd  and   c_rb is higher than half system bandwidth + 1
        // if these conditions are true the pointer has to be situated at the 1st part of the rxdataF just after the first IQ symbols of the RB containing DC
        rxF = &rxdataF[aarx][(12*(c_rb - (frame_parms->N_RB_DL>>1)) - 6 + (symbol * (frame_parms->ofdm_symbol_size)))]; // we point at the 1st part of the rxdataF in symbol
        #ifdef NR_PDCCH_DCI_DEBUG
          printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_extract_rbs_single)-> in odd case c_rb (%d) is higher than half N_RB_DL (not DC) -> rxF = &rxdataF[aarx = (%d)][(12*(c_rb - frame_parms->N_RB_DL) - 5 + (symbol * (frame_parms->ofdm_symbol_size))) = (%d)]\n",
                 c_rb,aarx,(12*(c_rb - (frame_parms->N_RB_DL>>1)) - 6 + (symbol * (frame_parms->ofdm_symbol_size))));
        #endif
      }
      if ((c_rb == (frame_parms->N_RB_DL >> 1)) && ((frame_parms->N_RB_DL & 1) != 0)){ // treatment of RB containing the DC
        // if odd number RBs in system bandwidth and first RB to be treated is higher than middle system bandwidth (around DC)
        // we have to treat the RB in two parts: first part from i=0 to 5, the data is at the end of rxdataF (pointing at the end of the table)
        rxF = &rxdataF[aarx][(frame_parms->first_carrier_offset + 12 * c_rb + (symbol * (frame_parms->ofdm_symbol_size)))];
        #ifdef NR_PDCCH_DCI_DEBUG
          printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_extract_rbs_single)-> in odd case c_rb (%d) is half N_RB_DL + 1 we treat DC case -> rxF = &rxdataF[aarx = (%d)][(frame_parms->first_carrier_offset + 12 * c_rb + (symbol * (frame_parms->ofdm_symbol_size))) = (%d)]\n",
                 c_rb,aarx,(frame_parms->first_carrier_offset + 12 * c_rb + (symbol * (frame_parms->ofdm_symbol_size))));
        #endif
        /*if (symbol_mod > 300) { // this if is going to be removed as DM-RS signals are present in all symbols of PDCCH
          for (i = 0; i < 6; i++) {
            dl_ch0_ext[i] = dl_ch0[i];
            rxF_ext[i] = rxF[i];
          }
          rxF = &rxdataF[aarx][(symbol * (frame_parms->ofdm_symbol_size))]; // we point at the 1st part of the rxdataF in symbol
          #ifdef NR_PDCCH_DCI_DEBUG
            printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_extract_rbs_single)-> in odd case c_rb (%d) is half N_RB_DL +1 we treat DC case -> rxF = &rxdataF[aarx = (%d)][(symbol * (frame_parms->ofdm_symbol_size)) = (%d)]\n",
                   c_rb,aarx,(symbol * (frame_parms->ofdm_symbol_size)));
          #endif
          for (; i < 12; i++) {
            dl_ch0_ext[i] = dl_ch0[i];
            rxF_ext[i] = rxF[(1 + i - 6)];
          }
          nb_rb++;
          dl_ch0_ext += 12;
          rxF_ext += 12;
          dl_ch0 += 12;
          rxF += 7;
          c_rb++;
          } else {*/
        j = 0;
        for (i = 0; i < 6; i++) { //treating first part of the RB note that i=5 would correspond to DC. We treat it in NR
          if ((i != 1) && (i != 5)) {
            dl_ch0_ext[j] = dl_ch0[i];
            rxF_ext[j++] = rxF[i];
            //              printf("**extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j-1],*(1+(short*)&rxF_ext[j-1]));
          }
        }
        // then we point at the begining of the symbol part of rxdataF do process second part of RB
        rxF = &rxdataF[aarx][((symbol * (frame_parms->ofdm_symbol_size)))]; // we point at the 1st part of the rxdataF in symbol
        #ifdef NR_PDCCH_DCI_DEBUG
          printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_extract_rbs_single)-> in odd case c_rb (%d) is half N_RB_DL +1 we treat DC case -> rxF = &rxdataF[aarx = (%d)][(symbol * (frame_parms->ofdm_symbol_size)) = (%d)]\n",
                 c_rb,aarx,(symbol * (frame_parms->ofdm_symbol_size)));
        #endif
        for (; i < 12; i++) {
          if ((i != 9)) {
            dl_ch0_ext[j] = dl_ch0[i];
            rxF_ext[j++] = rxF[(1 + i - 6)];
            //              printf("**extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j-1],*(1+(short*)&rxF_ext[j-1]));
          }
        }
        nb_rb++;
        dl_ch0_ext += NBR_RE_PER_RB_WITHOUT_DMRS;
        rxF_ext += NBR_RE_PER_RB_WITHOUT_DMRS;
        dl_ch0 += 12;
          //rxF += 7;
          //c_rb++;
          //n_BWP_start++; // We have to increment this variable here to be consequent in the for loop afterwards
        //}
      } else { // treatment of any RB that does not contain the DC
        /*if (symbol_mod > 300) {
          memcpy(dl_ch0_ext, dl_ch0, 12 * sizeof(int32_t));
          for (i = 0; i < 12; i++) {
            rxF_ext[i] = rxF[i];
          }
          nb_rb++;
          dl_ch0_ext += 12;
          rxF_ext += 12;
          dl_ch0 += 12;
          //rxF += 12;
        } else {*/
        j = 0;
        for (i = 0; i < 12; i++) {
          if ((i != 1) && (i != 5) && (i != 9)) {
            rxF_ext[j] = rxF[i];
            #ifdef NR_PDCCH_DCI_DEBUG
              printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_extract_rbs_single)-> RB[c_rb %d] \t RE[re %d] => rxF_ext[%d]=(%d,%d)\t rxF[%d]=(%d,%d)",
                     c_rb, i, j, *(short *) &rxF_ext[j],*(1 + (short*) &rxF_ext[j]), i,
                     *(short *) &rxF[i], *(1 + (short*) &rxF[i]));
            #endif
            dl_ch0_ext[j++] = dl_ch0[i];
            //printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_extract_rbs_single)-> ch %d => dl_ch0(%d,%d)\n", i, *(short *) &dl_ch0[i], *(1 + (short*) &dl_ch0[i]));
            printf("\t-> ch %d => dl_ch0(%d,%d)\n", i, *(short *) &dl_ch0[i], *(1 + (short*) &dl_ch0[i]));
          } else {
            #ifdef NR_PDCCH_DCI_DEBUG
              printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_extract_rbs_single)-> RB[c_rb %d] \t RE[re %d] => rxF_ext[%d]=(%d,%d)\t rxF[%d]=(%d,%d) \t\t <==> DM-RS PDCCH, this is a pilot symbol\n",
                     c_rb, i, j, *(short *) &rxF_ext[j], *(1 + (short*) &rxF_ext[j]), i,
                     *(short *) &rxF[i], *(1 + (short*) &rxF[i]));
            #endif
          }
        }
        nb_rb++;
        dl_ch0_ext += NBR_RE_PER_RB_WITHOUT_DMRS;
        rxF_ext += NBR_RE_PER_RB_WITHOUT_DMRS;
        dl_ch0 += 12;
          //rxF += 12;
        //}
      }
    }
  }
}

#endif
#ifdef NR_PDCCH_DCI_RUN_bis
// this function is just a second implementation of nr_pdcch_extract_rbs_single
// the code modification is minimum but it can be slower in processing time
// to be removed

void nr_pdcch_extract_rbs_single_bis(int32_t **rxdataF, int32_t **dl_ch_estimates,
  int32_t **rxdataF_ext, int32_t **dl_ch_estimates_ext, uint8_t symbol,
  uint32_t high_speed_flag, NR_DL_FRAME_PARMS *frame_parms, uint64_t coreset_freq_dom, uint32_t coreset_nbr_rb, uint32_t n_BWP_start) {

  uint16_t rb, nb_rb = 0;
  uint8_t i, j, aarx;
  int32_t *dl_ch0, *dl_ch0_ext, *rxF, *rxF_ext;
  int nushiftmod3 = frame_parms->nushift % 3;
  uint8_t symbol_mod;

  symbol_mod = (symbol >= (7 - frame_parms->Ncp)) ? symbol - (7 - frame_parms->Ncp) : symbol;
#ifdef DEBUG_DCI_DECODING
  LOG_I(PHY, "extract_rbs_single: symbol_mod %d\n",symbol_mod);
#endif
  for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
    if (high_speed_flag == 1){
      dl_ch0 = &dl_ch_estimates[aarx][5 + (symbol * (frame_parms->ofdm_symbol_size))];
      printf("\t\t### in function nr_pdcch_extract_rbs_single(), \t ### dl_ch0 = &dl_ch_estimates[aarx = (%d) ][5 + (symbol * (frame_parms->ofdm_symbol_size)) = (%d)]\n",
             aarx,5 + (symbol * (frame_parms->ofdm_symbol_size)));
    } else {
      dl_ch0 = &dl_ch_estimates[aarx][5];
      printf("\t\t### in function nr_pdcch_extract_rbs_single(), \t ### dl_ch0 = &dl_ch_estimates[aarx = (%d)][5]\n",aarx);
    }
    dl_ch0_ext = &dl_ch_estimates_ext[aarx][symbol * (frame_parms->N_RB_DL * 12)];
    printf("\t\t### in function nr_pdcch_extract_rbs_single(), \t ### dl_ch0_ext = &dl_ch_estimates_ext[aarx = (%d)][symbol * (frame_parms->N_RB_DL * 12) = (%d)]\n",
           aarx,symbol * (frame_parms->N_RB_DL * 12));
    rxF_ext = &rxdataF_ext[aarx][symbol * (frame_parms->N_RB_DL * 12)];
    printf("\t\t### in function nr_pdcch_extract_rbs_single(), \t ### rxF_ext = &rxdataF_ext[aarx = (%d)][symbol * (frame_parms->N_RB_DL * 12) = (%d)]\n",
           aarx,symbol * (frame_parms->N_RB_DL * 12));
    rxF = &rxdataF[aarx][(frame_parms->first_carrier_offset + (symbol * (frame_parms->ofdm_symbol_size)))];
    printf("\t\t### in function nr_pdcch_extract_rbs_single(), \t ### rxF = &rxdataF[aarx = (%d)][(frame_parms->first_carrier_offset + (symbol * (frame_parms->ofdm_symbol_size))) = (%d)]\n",
           aarx,(frame_parms->first_carrier_offset + (symbol * (frame_parms->ofdm_symbol_size))));
    printf("\t\t ###### in function pdcch_extract_rbs_single(for symbol=%d, aarx=%d), symbol_mod=%d, nushiftmod3=%d \n",symbol,aarx,symbol_mod,nushiftmod3);
    printf("\t\t ###### rxF_ext = &rxdataF_ext[aarx(%d)][symbol(%d) * (frame_parms->N_RB_DL(%d) * 12)]\n",aarx,symbol,frame_parms->N_RB_DL);
    printf("\t\t ###### rxF = &rxdataF[aarx(%d)][(frame_parms->first_carrier_offset(%d) + (symbol(%d) * (frame_parms->ofdm_symbol_size(%d))))]\n",
           aarx,frame_parms->first_carrier_offset,symbol,frame_parms->ofdm_symbol_size);

    if ((frame_parms->N_RB_DL & 1) == 0) { // even number of RBs
      for (rb = 0; rb < frame_parms->N_RB_DL; rb++) {
        printf("\t\t\t ###### rb=%d\n",rb);
        if (rb == (frame_parms->N_RB_DL >> 1)) { // For second half of RBs skip DC carrier
          rxF = &rxdataF[aarx][(1 + (symbol * (frame_parms->ofdm_symbol_size)))];
          printf("\t\t\t ###### if rb (%d) is half N_RB_DL, skip DC carrier -> rxF = &rxdataF[aarx(%d)][(1 + (symbol(%d) * (frame_parms->ofdm_symbol_size(%d))))\n",
                 rb,aarx,symbol,frame_parms->ofdm_symbol_size);
          //dl_ch0++;
        }
        if (symbol_mod > 0) {
          memcpy(dl_ch0_ext, dl_ch0, 12 * sizeof(int32_t));
          for (i = 0; i < 12; i++) {
            rxF_ext[i] = rxF[i];
          }
          nb_rb++;
          dl_ch0_ext += 12;
          rxF_ext += 12;
          dl_ch0 += 12;
          rxF += 12;
        } else {
          j = 0;
          for (i = 0; i < 12; i++) {
            if ((i != 1) && (i != 5) && (i != 9)) {
              rxF_ext[j] = rxF[i];
              printf("\textract rb %d \t re %d => rxF_ext[%d]=(%d,%d)", rb, i, j, *(short *) &rxF_ext[j], *(1 + (short*) &rxF_ext[j]));
                     dl_ch0_ext[j++] = dl_ch0[i];
              printf("\t\tch %d => dl_ch0(%d,%d)\n", i, *(short *) &dl_ch0[i], *(1 + (short*) &dl_ch0[i]));
            }
          }
          nb_rb++;
          dl_ch0_ext += 9;
          rxF_ext += 9;
          dl_ch0 += 12;
          rxF += 12;
        }
      }
    } else { // Odd number of RBs
      for (rb = 0; rb < frame_parms->N_RB_DL >> 1; rb++) {
        printf("\t\t\t ###### rb=%d (Odd number of RBs, rb < half of band)\n",rb);
          if (symbol_mod > 0) {
            memcpy(dl_ch0_ext, dl_ch0, 12 * sizeof(int32_t));
            for (i = 0; i < 12; i++)
              rxF_ext[i] = rxF[i];
            nb_rb++;
            dl_ch0_ext += 12;
            rxF_ext += 12;
            dl_ch0 += 12;
            rxF += 12;
          } else {
            j = 0;
            for (i = 0; i < 12; i++) {
              if ((i != 1) && (i != 5) && (i != 9)) {
                rxF_ext[j] = rxF[i];
                printf("\t\t\t\t ###### extract rb %d, re %d => rxF_ext[%d]=(%d,%d) rxF[%d]=(%d,%d)",
                       rb,i,j,*(short *)&rxF_ext[j],*(1+(short*)&rxF_ext[j]),i,*(short *)&rxF[i],*(1+(short*)&rxF[i]));
                dl_ch0_ext[j++] = dl_ch0[i];
                printf("\t ###### extract rb %d, re %d => dl_ch0 []=(%d,%d)\n",rb,i,*(short *)&dl_ch0[i],*(1+(short*)&dl_ch0[i]));
              } else {
                printf("\t\t\t\t ###### THIS IS a pilot  rb %d \t re %d => rxF_ext[%d]=(%d,%d)\t rxF[%d]=(%d,%d) \t this is a pilot symbol\n",
                       rb,i,j,*(short *)&rxF_ext[j],*(1+(short*)&rxF_ext[j]),i,*(short *)&rxF[i],*(1+(short*)&rxF[i]));
              }
            }
            nb_rb++;
            dl_ch0_ext += 9;
            rxF_ext += 9;
            dl_ch0 += 12;
            rxF += 12;
          }
        }
        // Do middle RB (around DC)
        //printf("dlch_ext %d\n",dl_ch0_ext-&dl_ch_estimates_ext[aarx][0]);
        if (symbol_mod == 0) {
          j = 0;
          for (i = 0; i < 6; i++) {
            if ((i != 1) && (i != 5)) {
              dl_ch0_ext[j] = dl_ch0[i];
              rxF_ext[j++] = rxF[i];
              //printf("**extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j-1],*(1+(short*)&rxF_ext[j-1]));
            }
          }
          rxF = &rxdataF[aarx][((symbol * (frame_parms->ofdm_symbol_size)))];
          for (; i < 12; i++) {
            if (i != 9) {
              dl_ch0_ext[j] = dl_ch0[i];
              rxF_ext[j++] = rxF[(1 + i - 6)];
              //printf("**extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j-1],*(1+(short*)&rxF_ext[j-1]));
            }
          }
          nb_rb++;
          dl_ch0_ext += 9;
          rxF_ext += 9;
          dl_ch0 += 12;
          rxF += 7;
          rb++;
        } else {
          for (i = 0; i < 6; i++) {
            dl_ch0_ext[i] = dl_ch0[i];
            rxF_ext[i] = rxF[i];
          }
          rxF = &rxdataF[aarx][((symbol * (frame_parms->ofdm_symbol_size)))];
          for (; i < 12; i++) {
            dl_ch0_ext[i] = dl_ch0[i];
            rxF_ext[i] = rxF[(1 + i - 6)];
          }
          nb_rb++;
          dl_ch0_ext += 12;
          rxF_ext += 12;
          dl_ch0 += 12;
          rxF += 7;
          rb++;
        }
        for (; rb < frame_parms->N_RB_DL; rb++) {
        printf("\t\t\t ###### rb=%d (Even number of RBs, rb > half of band)\n",rb);
        if (symbol_mod > 0) {
          memcpy(dl_ch0_ext, dl_ch0, 12 * sizeof(int32_t));
          for (i = 0; i < 12; i++)
            rxF_ext[i] = rxF[i];
          nb_rb++;
          dl_ch0_ext += 12;
          rxF_ext += 12;
          dl_ch0 += 12;
          rxF += 12;
        } else {
          j = 0;
          for (i = 0; i < 12; i++) {
            if ((i != 1) && (i != 5) && (i != 9)) {
              rxF_ext[j] = rxF[i];
              printf("\t\t\t\t ###### extract rb %d, re %d => rxF_ext[]=(%d,%d)",rb,i,*(short *)&rxF_ext[j],*(1+(short*)&rxF_ext[j]));
              //printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j],*(1+(short*)&rxF_ext[j]));
              dl_ch0_ext[j++] = dl_ch0[i];
              printf("\t ###### extract rb %d, re %d => dl_ch0 []=(%d,%d)\n",rb,i,*(short *)&dl_ch0[i],*(1+(short*)&dl_ch0[i]));
            } else {
              printf("\t\t\t\t ###### THIS IS a RS at re %d\n",i);
            }
          }
          nb_rb++;
          dl_ch0_ext += 9;
          rxF_ext += 9;
          dl_ch0 += 12;
          rxF += 12;
        }
      }
    }
  }

// The function has created table rxdataF_ext with the contents of rxdataF and removing the pilots at positions 1,5,9 in every RB.
// Now we need to check the contents of rxdataF_ext and keep only the values which correspond to our CORESET depending on variables:
// - coreset_freq_dom (45 bit map)
// - n_BWP_start (first RB within the active BWP and the CORESET)
  j=0;
  k=0;
  rxF_ext = &rxdataF_ext[aarx][(symbol * (frame_parms->N_RB_DL * 12))];
  printf("\t\t### in function nr_pdcch_extract_rbs_single(), \t ### rxF_ext = &rxdataF_ext[aarx = (%d)][n_BWP_start + (symbol * (frame_parms->N_RB_DL * 12)) = (%d)]\n",
         aarx,(n_BWP_start + (symbol * (frame_parms->N_RB_DL * 12))));

  int bitcnt_coreset_freq_dom = 1; // this variable will allow to check each bit of the 45 bitmap coreset_freq_dom.
                                  //Eg: if bitcnt_coreset_freq_dom = 3, (46-3) we verify bit 43 starting from LSB
  for (rb=0; rb<frame_parms->N_RB_DL;rb++) {
    if (rb < n_BWP_start) {
    // while rb is not within CORESET, then remove values
      for (i=0; i<8; i++) {
        //rxF_ext[i] = 0;
        j++;
      }
      //rxF_ext = &rxdataF_ext[aarx][(j + (symbol * (frame_parms->N_RB_DL * 12)))];
    } else if ((rb >= n_BWP_start) && (rb < (n_BWP_start + 6 * 45))) {
    // rb is within CORESET, we need to verify now whether bit in coreset_freq_dom is set or not
      if ((((coreset_freq_dom & 0x1FFFFFFFFFFF) >> (46 - bitcnt_coreset_freq_dom)) & 0x1)== 0){
      // if bit is 0, next 6 consecutive RBs do not belong to CORESET
        bitcnt_coreset_freq_dom ++;
        rb = rb + 6;
        for (i=0; i<(9*6); i++) {
          //rxF_ext[i] = 0;
          j++;
        }
        //rxF_ext = &rxdataF_ext[aarx][(j + (symbol * (frame_parms->N_RB_DL * 12)))];
      } else {
      // if bit is 1, next 6 consecutive RBs belong to CORESET
        bitcnt_coreset_freq_dom ++;
        rb = rb + 6;
        for (i=0; i<(9*6); i++) {
          rxF_ext[k] = rxF_ext[j];
          k++;
          j++;
        }
        //rxF_ext = &rxdataF_ext[aarx][(j + (symbol * (frame_parms->N_RB_DL * 12)))];
      }
    } else { // for the RBs that are in the upper side of the CORESET bitmap
     for (i=0; i<8; i++) {
        rxF_ext[k] = 0;
        k++;
        j++;
      }
      //rxF_ext = &rxdataF_ext[aarx][(j + (symbol * (frame_parms->N_RB_DL * 12)))];
    }
  }
}

#endif



void pdcch_extract_rbs_single(int32_t **rxdataF,
                              int32_t **dl_ch_estimates,
                              int32_t **rxdataF_ext,
                              int32_t **dl_ch_estimates_ext,
                              uint8_t symbol,
                              uint32_t high_speed_flag,
                              NR_DL_FRAME_PARMS *frame_parms)
{


  uint16_t rb,nb_rb=0;
  uint8_t i,j,aarx;
  int32_t *dl_ch0,*dl_ch0_ext,*rxF,*rxF_ext;


  int nushiftmod3 = frame_parms->nushift%3;
  uint8_t symbol_mod;

  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;
#ifdef DEBUG_DCI_DECODING
  LOG_I(PHY, "extract_rbs_single: symbol_mod %d\n",symbol_mod);
#endif

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

    if (high_speed_flag == 1)
      dl_ch0     = &dl_ch_estimates[aarx][5+(symbol*(frame_parms->ofdm_symbol_size))];
    else
      dl_ch0     = &dl_ch_estimates[aarx][5];

    dl_ch0_ext = &dl_ch_estimates_ext[aarx][symbol*(frame_parms->N_RB_DL*12)];

    rxF_ext   = &rxdataF_ext[aarx][symbol*(frame_parms->N_RB_DL*12)];

    rxF       = &rxdataF[aarx][(frame_parms->first_carrier_offset + (symbol*(frame_parms->ofdm_symbol_size)))];

    if ((frame_parms->N_RB_DL&1) == 0)  { // even number of RBs
      for (rb=0; rb<frame_parms->N_RB_DL; rb++) {

        // For second half of RBs skip DC carrier
        if (rb==(frame_parms->N_RB_DL>>1)) {
          rxF       = &rxdataF[aarx][(1 + (symbol*(frame_parms->ofdm_symbol_size)))];

          //dl_ch0++;
        }

        if (symbol_mod>0) {
          memcpy(dl_ch0_ext,dl_ch0,12*sizeof(int32_t));

          for (i=0; i<12; i++) {

            rxF_ext[i]=rxF[i];

          }

          nb_rb++;
          dl_ch0_ext+=12;
          rxF_ext+=12;

          dl_ch0+=12;
          rxF+=12;
        } else {
          j=0;

          for (i=0; i<12; i++) {
            if ((i!=nushiftmod3) &&
                (i!=(nushiftmod3+3)) &&
                (i!=(nushiftmod3+6)) &&
                (i!=(nushiftmod3+9))) {
              rxF_ext[j]=rxF[i];
              //                        printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j],*(1+(short*)&rxF_ext[j]));
              dl_ch0_ext[j++]=dl_ch0[i];
              //                printf("ch %d => (%d,%d)\n",i,*(short *)&dl_ch0[i],*(1+(short*)&dl_ch0[i]));
            }
          }

          nb_rb++;
          dl_ch0_ext+=8;
          rxF_ext+=8;

          dl_ch0+=12;
          rxF+=12;
        }
      }
    } else { // Odd number of RBs
      for (rb=0; rb<frame_parms->N_RB_DL>>1; rb++) {

        if (symbol_mod>0) {
          memcpy(dl_ch0_ext,dl_ch0,12*sizeof(int32_t));

          for (i=0; i<12; i++)
            rxF_ext[i]=rxF[i];

          nb_rb++;
          dl_ch0_ext+=12;
          rxF_ext+=12;

          dl_ch0+=12;
          rxF+=12;
        } else {
          j=0;

          for (i=0; i<12; i++) {
            if ((i!=nushiftmod3) &&
                (i!=(nushiftmod3+3)) &&
                (i!=(nushiftmod3+6)) &&
                (i!=(nushiftmod3+9))) {
              rxF_ext[j]=rxF[i];
              //                        printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j],*(1+(short*)&rxF_ext[j]));
              dl_ch0_ext[j++]=dl_ch0[i];
              //                printf("ch %d => (%d,%d)\n",i,*(short *)&dl_ch0[i],*(1+(short*)&dl_ch0[i]));
            }
          }

          nb_rb++;
          dl_ch0_ext+=8;
          rxF_ext+=8;

          dl_ch0+=12;
          rxF+=12;
        }
      }

      // Do middle RB (around DC)
      //  printf("dlch_ext %d\n",dl_ch0_ext-&dl_ch_estimates_ext[aarx][0]);

      if (symbol_mod==0) {
        j=0;

        for (i=0; i<6; i++) {
          if ((i!=nushiftmod3) &&
              (i!=(nushiftmod3+3))) {
            dl_ch0_ext[j]=dl_ch0[i];
            rxF_ext[j++]=rxF[i];
            //              printf("**extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j-1],*(1+(short*)&rxF_ext[j-1]));
          }
        }

        rxF       = &rxdataF[aarx][((symbol*(frame_parms->ofdm_symbol_size)))];

        for (; i<12; i++) {
          if ((i!=(nushiftmod3+6)) &&
              (i!=(nushiftmod3+9))) {
            dl_ch0_ext[j]=dl_ch0[i];
            rxF_ext[j++]=rxF[(1+i-6)];
            //              printf("**extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j-1],*(1+(short*)&rxF_ext[j-1]));
          }
        }


        nb_rb++;
        dl_ch0_ext+=8;
        rxF_ext+=8;
        dl_ch0+=12;
        rxF+=7;
        rb++;
      } else {
        for (i=0; i<6; i++) {
          dl_ch0_ext[i]=dl_ch0[i];
          rxF_ext[i]=rxF[i];
        }

        rxF       = &rxdataF[aarx][((symbol*(frame_parms->ofdm_symbol_size)))];

        for (; i<12; i++) {
          dl_ch0_ext[i]=dl_ch0[i];
          rxF_ext[i]=rxF[(1+i-6)];
        }


        nb_rb++;
        dl_ch0_ext+=12;
        rxF_ext+=12;
        dl_ch0+=12;
        rxF+=7;
        rb++;
      }

      for (; rb<frame_parms->N_RB_DL; rb++) {
        if (symbol_mod > 0) {
          memcpy(dl_ch0_ext,dl_ch0,12*sizeof(int32_t));

          for (i=0; i<12; i++)
            rxF_ext[i]=rxF[i];

          nb_rb++;
          dl_ch0_ext+=12;
          rxF_ext+=12;

          dl_ch0+=12;
          rxF+=12;
        } else {
          j=0;

          for (i=0; i<12; i++) {
            if ((i!=(nushiftmod3)) &&
                (i!=(nushiftmod3+3)) &&
                (i!=(nushiftmod3+6)) &&
                (i!=(nushiftmod3+9))) {
              rxF_ext[j]=rxF[i];
              //                printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j],*(1+(short*)&rxF_ext[j]));
              dl_ch0_ext[j++]=dl_ch0[i];
            }
          }

          nb_rb++;
          dl_ch0_ext+=8;
          rxF_ext+=8;

          dl_ch0+=12;
          rxF+=12;
        }
      }
    }
  }
}

void pdcch_extract_rbs_dual(int32_t **rxdataF,
                            int32_t **dl_ch_estimates,
                            int32_t **rxdataF_ext,
                            int32_t **dl_ch_estimates_ext,
                            uint8_t symbol,
                            uint32_t high_speed_flag,
                            NR_DL_FRAME_PARMS *frame_parms)
{


  uint16_t rb,nb_rb=0;
  uint8_t i,aarx,j;
  int32_t *dl_ch0,*dl_ch0_ext,*dl_ch1,*dl_ch1_ext,*rxF,*rxF_ext;
  uint8_t symbol_mod;
  int nushiftmod3 = frame_parms->nushift%3;

  symbol_mod = (symbol>=(7-frame_parms->Ncp)) ? symbol-(7-frame_parms->Ncp) : symbol;
#ifdef DEBUG_DCI_DECODING
  LOG_I(PHY, "extract_rbs_dual: symbol_mod %d\n",symbol_mod);
#endif

  for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

    if (high_speed_flag==1) {
      dl_ch0     = &dl_ch_estimates[aarx][5+(symbol*(frame_parms->ofdm_symbol_size))];
      dl_ch1     = &dl_ch_estimates[2+aarx][5+(symbol*(frame_parms->ofdm_symbol_size))];
    } else {
      dl_ch0     = &dl_ch_estimates[aarx][5];
      dl_ch1     = &dl_ch_estimates[2+aarx][5];
    }

    dl_ch0_ext = &dl_ch_estimates_ext[aarx][symbol*(frame_parms->N_RB_DL*12)];
    dl_ch1_ext = &dl_ch_estimates_ext[2+aarx][symbol*(frame_parms->N_RB_DL*12)];

    //    printf("pdcch extract_rbs: rxF_ext pos %d\n",symbol*(frame_parms->N_RB_DL*12));
    rxF_ext   = &rxdataF_ext[aarx][symbol*(frame_parms->N_RB_DL*12)];

    rxF       = &rxdataF[aarx][(frame_parms->first_carrier_offset + (symbol*(frame_parms->ofdm_symbol_size)))];

    if ((frame_parms->N_RB_DL&1) == 0)  // even number of RBs
      for (rb=0; rb<frame_parms->N_RB_DL; rb++) {

        // For second half of RBs skip DC carrier
        if (rb==(frame_parms->N_RB_DL>>1)) {
          rxF       = &rxdataF[aarx][(1 + (symbol*(frame_parms->ofdm_symbol_size)))];
          //    dl_ch0++;
          //dl_ch1++;
        }

        if (symbol_mod>0) {
          memcpy(dl_ch0_ext,dl_ch0,12*sizeof(int32_t));
          memcpy(dl_ch1_ext,dl_ch1,12*sizeof(int32_t));

          /*
            printf("rb %d\n",rb);
            for (i=0;i<12;i++)
            printf("(%d %d)",((int16_t *)dl_ch0)[i<<1],((int16_t*)dl_ch0)[1+(i<<1)]);
            printf("\n");
          */
          for (i=0; i<12; i++) {
            rxF_ext[i]=rxF[i];
            //      printf("%d : (%d,%d)\n",(rxF+(2*i)-&rxdataF[aarx][( (symbol*(frame_parms->ofdm_symbol_size)))*2])/2,
            //  ((int16_t*)&rxF[i<<1])[0],((int16_t*)&rxF[i<<1])[0]);
          }

          nb_rb++;
          dl_ch0_ext+=12;
          dl_ch1_ext+=12;
          rxF_ext+=12;
        } else {
          j=0;

          for (i=0; i<12; i++) {
            if ((i!=nushiftmod3) &&
                (i!=nushiftmod3+3) &&
                (i!=nushiftmod3+6) &&
                (i!=nushiftmod3+9)) {
              rxF_ext[j]=rxF[i];
              //                            printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j],*(1+(short*)&rxF_ext[j]));
              dl_ch0_ext[j]  =dl_ch0[i];
              dl_ch1_ext[j++]=dl_ch1[i];
            }
          }

          nb_rb++;
          dl_ch0_ext+=8;
          dl_ch1_ext+=8;
          rxF_ext+=8;
        }

        dl_ch0+=12;
        dl_ch1+=12;
        rxF+=12;
      }

    else {  // Odd number of RBs
      for (rb=0; rb<frame_parms->N_RB_DL>>1; rb++) {

        //  printf("rb %d: %d\n",rb,rxF-&rxdataF[aarx][(symbol*(frame_parms->ofdm_symbol_size))*2]);

        if (symbol_mod>0) {
          memcpy(dl_ch0_ext,dl_ch0,12*sizeof(int32_t));
          memcpy(dl_ch1_ext,dl_ch1,12*sizeof(int32_t));

          for (i=0; i<12; i++)
            rxF_ext[i]=rxF[i];

          nb_rb++;
          dl_ch0_ext+=12;
          dl_ch1_ext+=12;
          rxF_ext+=12;

          dl_ch0+=12;
          dl_ch1+=12;
          rxF+=12;

        } else {
          j=0;

          for (i=0; i<12; i++) {
            if ((i!=nushiftmod3) &&
                (i!=nushiftmod3+3) &&
                (i!=nushiftmod3+6) &&
                (i!=nushiftmod3+9)) {
              rxF_ext[j]=rxF[i];
              //                        printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j],*(1+(short*)&rxF_ext[j]));
              dl_ch0_ext[j]=dl_ch0[i];
              dl_ch1_ext[j++]=dl_ch1[i];
              //                printf("ch %d => (%d,%d)\n",i,*(short *)&dl_ch0[i],*(1+(short*)&dl_ch0[i]));
            }
          }

          nb_rb++;
          dl_ch0_ext+=8;
          dl_ch1_ext+=8;
          rxF_ext+=8;


          dl_ch0+=12;
          dl_ch1+=12;
          rxF+=12;
        }
      }

      // Do middle RB (around DC)

      if (symbol_mod > 0) {
        for (i=0; i<6; i++) {
          dl_ch0_ext[i]=dl_ch0[i];
          dl_ch1_ext[i]=dl_ch1[i];
          rxF_ext[i]=rxF[i];
        }

        rxF       = &rxdataF[aarx][((symbol*(frame_parms->ofdm_symbol_size)))];

        for (; i<12; i++) {
          dl_ch0_ext[i]=dl_ch0[i];
          dl_ch1_ext[i]=dl_ch1[i];
          rxF_ext[i]=rxF[(1+i)];
        }

        nb_rb++;
        dl_ch0_ext+=12;
        dl_ch1_ext+=12;
        rxF_ext+=12;

        dl_ch0+=12;
        dl_ch1+=12;
        rxF+=7;
        rb++;
      } else {
        j=0;

        for (i=0; i<6; i++) {
          if ((i!=nushiftmod3) &&
              (i!=nushiftmod3+3)) {
            dl_ch0_ext[j]=dl_ch0[i];
            dl_ch1_ext[j]=dl_ch1[i];
            rxF_ext[j++]=rxF[i];
            //              printf("**extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j-1],*(1+(short*)&rxF_ext[j-1]));
          }
        }

        rxF       = &rxdataF[aarx][((symbol*(frame_parms->ofdm_symbol_size)))];

        for (; i<12; i++) {
          if ((i!=nushiftmod3+6) &&
              (i!=nushiftmod3+9)) {
            dl_ch0_ext[j]=dl_ch0[i];
            dl_ch1_ext[j]=dl_ch1[i];
            rxF_ext[j++]=rxF[(1+i-6)];
            //              printf("**extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j-1],*(1+(short*)&rxF_ext[j-1]));
          }
        }


        nb_rb++;
        dl_ch0_ext+=8;
        dl_ch1_ext+=8;
        rxF_ext+=8;
        dl_ch0+=12;
        dl_ch1+=12;
        rxF+=7;
        rb++;
      }

      for (; rb<frame_parms->N_RB_DL; rb++) {

        if (symbol_mod>0) {
          //  printf("rb %d: %d\n",rb,rxF-&rxdataF[aarx][(symbol*(frame_parms->ofdm_symbol_size))*2]);
          memcpy(dl_ch0_ext,dl_ch0,12*sizeof(int32_t));
          memcpy(dl_ch1_ext,dl_ch1,12*sizeof(int32_t));

          for (i=0; i<12; i++)
            rxF_ext[i]=rxF[i];

          nb_rb++;
          dl_ch0_ext+=12;
          dl_ch1_ext+=12;
          rxF_ext+=12;

          dl_ch0+=12;
          dl_ch1+=12;
          rxF+=12;
        } else {
          j=0;

          for (i=0; i<12; i++) {
            if ((i!=nushiftmod3) &&
                (i!=nushiftmod3+3) &&
                (i!=nushiftmod3+6) &&
                (i!=nushiftmod3+9)) {
              rxF_ext[j]=rxF[i];
              //                printf("extract rb %d, re %d => (%d,%d)\n",rb,i,*(short *)&rxF_ext[j],*(1+(short*)&rxF_ext[j]));
              dl_ch0_ext[j]=dl_ch0[i];
              dl_ch1_ext[j++]=dl_ch1[i];
            }
          }

          nb_rb++;
          dl_ch0_ext+=8;
          dl_ch1_ext+=8;
          rxF_ext+=8;

          dl_ch0+=12;
          dl_ch1+=12;
          rxF+=12;
        }
      }
    }
  }
}



#ifdef NR_PDCCH_DCI_RUN
void nr_pdcch_channel_compensation(int32_t **rxdataF_ext,
                                   int32_t **dl_ch_estimates_ext,
                                   int32_t **rxdataF_comp,
                                   int32_t **rho,
                                   NR_DL_FRAME_PARMS *frame_parms,
                                   uint8_t symbol,
                                   uint8_t output_shift,
                                   uint32_t coreset_nbr_rb)
{

  uint16_t rb;
  #if defined(__x86_64__) || defined(__i386__)
    __m128i *dl_ch128, *rxdataF128, *rxdataF_comp128;
    __m128i *dl_ch128_2, *rho128;
  #elif defined(__arm__)
  #endif
  uint8_t aatx, aarx, pilots = 0;

short conjugate[8]__attribute__((aligned(16)))  = {-1,1,-1,1,-1,1,-1,1};
short conjugate2[8]__attribute__((aligned(16))) = {1,-1,1,-1,1,-1,1,-1};

  #ifdef DEBUG_DCI_DECODING
    LOG_I(PHY, "PDCCH comp: symbol %d\n",symbol);
  #endif
/*
	if (symbol == 0)
		pilots = 1;
*/
  for (aatx = 0; aatx < frame_parms->nb_antenna_ports_eNB; aatx++) {
    //if (frame_parms->mode1_flag && aatx>0) break; //if mode1_flag is set then there is only one stream to extract, independent of nb_antenna_ports_eNB
    for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {
      #if defined(__x86_64__) || defined(__i386__)
        // dl_ch128 = (__m128i *) &dl_ch_estimates_ext[(aatx << 1) + aarx][symbol * frame_parms->N_RB_DL * 12];
        // rxdataF128 = (__m128i *) &rxdataF_ext[aarx][symbol * frame_parms->N_RB_DL * 12];
        // rxdataF_comp128 = (__m128i *) &rxdataF_comp[(aatx << 1) + aarx][symbol * frame_parms->N_RB_DL * 12];
        dl_ch128 = (__m128i *) &dl_ch_estimates_ext[(aatx << 1) + aarx][symbol * coreset_nbr_rb * 12];
        rxdataF128 = (__m128i *) &rxdataF_ext[aarx][symbol * coreset_nbr_rb * 12];
        rxdataF_comp128 = (__m128i *) &rxdataF_comp[(aatx << 1) + aarx][symbol * coreset_nbr_rb * 12];
      #elif defined(__arm__)
      #endif
      #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_channel_compensation)-> Total of RBs to be computed (%d), and number of RE (%d) (9 RE per RB)\n",coreset_nbr_rb,coreset_nbr_rb*9);
      #endif
      uint32_t k=0;
      for (rb = 0; rb < coreset_nbr_rb; rb) { //FIXME this for loop risks an infinite loop if rb is not increased by 1 inside the loop
        #ifdef NR_PDCCH_DCI_DEBUG
          printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_channel_compensation)-> rb=%d\n", rb);
        #endif
        #if defined(__x86_64__) || defined(__i386__)
        #ifdef NR_PDCCH_DCI_DEBUG
          printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_channel_compensation)-> rxdataF x dl_ch -> RB[%d] RE[%d]\n",rb,k);
        #endif
        k++;
        if (k%9 == 0) rb++;
        #ifdef NR_PDCCH_DCI_DEBUG
          printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_channel_compensation)-> rxdataF x dl_ch -> RB[%d] RE[%d]\n",rb,k);
        #endif
        k++;
        if (k%9 == 0) rb++;
        #ifdef NR_PDCCH_DCI_DEBUG
          printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_channel_compensation)-> rxdataF x dl_ch -> RB[%d] RE[%d]\n",rb,k);
        #endif
        k++;
        if (k%9 == 0) rb++;
        #ifdef NR_PDCCH_DCI_DEBUG
          printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_channel_compensation)-> rxdataF x dl_ch -> RB[%d] RE[%d]\n",rb,k);
        #endif
        k++;
        if (k%9 == 0) rb++;
        // multiply by conjugated channel
        mmtmpPD0 = _mm_madd_epi16(dl_ch128[0], rxdataF128[0]); // mmtmpPD0 contains real part of 4 consecutive outputs (32-bit)
        // print_ints("re",&mmtmpPD0);
        mmtmpPD1 = _mm_shufflelo_epi16(dl_ch128[0], _MM_SHUFFLE(2,3,0,1));
        mmtmpPD1 = _mm_shufflehi_epi16(mmtmpPD1, _MM_SHUFFLE(2,3,0,1));
        mmtmpPD1 = _mm_sign_epi16(mmtmpPD1, *(__m128i * )&conjugate[0]);

        #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_channel_compensation)-> conjugate\t ### \t");
        for (int conjugate_index=0 ; conjugate_index< 8 ; conjugate_index++)
          printf("conjugate[%d]=%d",conjugate_index,conjugate[conjugate_index]);
        printf("\n");
        #endif
      
        // print_ints("im",&mmtmpPD1);
        mmtmpPD1 = _mm_madd_epi16(mmtmpPD1, rxdataF128[0]); // mmtmpPD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpPD0 = _mm_srai_epi32(mmtmpPD0, output_shift);
        // print_ints("re(shift)",&mmtmpPD0);
        mmtmpPD1 = _mm_srai_epi32(mmtmpPD1, output_shift);
        // print_ints("im(shift)",&mmtmpPD1);
        mmtmpPD2 = _mm_unpacklo_epi32(mmtmpPD0, mmtmpPD1);
        mmtmpPD3 = _mm_unpackhi_epi32(mmtmpPD0, mmtmpPD1);
        // print_ints("c0",&mmtmpPD2);
        // print_ints("c1",&mmtmpPD3);
        rxdataF_comp128[0] = _mm_packs_epi32(mmtmpPD2, mmtmpPD3);
        // print_shorts("rx:",rxdataF128);
        // print_shorts("ch:",dl_ch128);
        // print_shorts("pack:",rxdataF_comp128);
        #ifdef NR_PDCCH_DCI_DEBUG
          printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_channel_compensation)-> rxdataF x dl_ch -> RB[%d] RE[%d]\n",rb,k);
        #endif
        k++;
        if (k%9 == 0) rb++;
        #ifdef NR_PDCCH_DCI_DEBUG
          printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_channel_compensation)-> rxdataF x dl_ch -> RB[%d] RE[%d]\n",rb,k);
        #endif
        k++;
        if (k%9 == 0) rb++;
        #ifdef NR_PDCCH_DCI_DEBUG
          printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_channel_compensation)-> rxdataF x dl_ch -> RB[%d] RE[%d]\n",rb,k);
        #endif
        k++;
        if (k%9 == 0) rb++;
        #ifdef NR_PDCCH_DCI_DEBUG
          printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_channel_compensation)-> rxdataF x dl_ch -> RB[%d] RE[%d]\n",rb,k);
        #endif
        k++;
        if (k%9 == 0) rb++;
        // multiply by conjugated channel
        mmtmpPD0 = _mm_madd_epi16(dl_ch128[1], rxdataF128[1]); // mmtmpPD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpPD1 = _mm_shufflelo_epi16(dl_ch128[1], _MM_SHUFFLE(2,3,0,1));
        mmtmpPD1 = _mm_shufflehi_epi16(mmtmpPD1, _MM_SHUFFLE(2,3,0,1));
        mmtmpPD1 = _mm_sign_epi16(mmtmpPD1, *(__m128i * )conjugate);
        mmtmpPD1 = _mm_madd_epi16(mmtmpPD1, rxdataF128[1]); // mmtmpPD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpPD0 = _mm_srai_epi32(mmtmpPD0, output_shift);
        mmtmpPD1 = _mm_srai_epi32(mmtmpPD1, output_shift);
        mmtmpPD2 = _mm_unpacklo_epi32(mmtmpPD0, mmtmpPD1);
        mmtmpPD3 = _mm_unpackhi_epi32(mmtmpPD0, mmtmpPD1);
        rxdataF_comp128[1] = _mm_packs_epi32(mmtmpPD2, mmtmpPD3);
        // print_shorts("rx:",rxdataF128+1);
        // print_shorts("ch:",dl_ch128+1);
        // print_shorts("pack:",rxdataF_comp128+1);
        // multiply by conjugated channel
        #ifdef NR_PDCCH_DCI_DEBUG
          printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_channel_compensation)-> rxdataF x dl_ch -> RB[%d] RE[%d]\n",rb,k);
        #endif
        k++;
        if (k%9 == 0) rb++;
        #ifdef NR_PDCCH_DCI_DEBUG
          printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_channel_compensation)-> rxdataF x dl_ch -> RB[%d] RE[%d]\n",rb,k);
        #endif
        k++;
        if (k%9 == 0) rb++;
        #ifdef NR_PDCCH_DCI_DEBUG
          printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_channel_compensation)-> rxdataF x dl_ch -> RB[%d] RE[%d]\n",rb,k);
        #endif
        k++;
        if (k%9 == 0) rb++;
        #ifdef NR_PDCCH_DCI_DEBUG
          printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_channel_compensation)-> rxdataF x dl_ch -> RB[%d] RE[%d]\n",rb,k);
        #endif
        k++;
        if (k%9 == 0) rb++;
        mmtmpPD0 = _mm_madd_epi16(dl_ch128[2], rxdataF128[2]); // mmtmpPD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpPD1 = _mm_shufflelo_epi16(dl_ch128[2],_MM_SHUFFLE(2,3,0,1));
        mmtmpPD1 = _mm_shufflehi_epi16(mmtmpPD1, _MM_SHUFFLE(2,3,0,1));
        mmtmpPD1 = _mm_sign_epi16(mmtmpPD1, *(__m128i * )conjugate);
        mmtmpPD1 = _mm_madd_epi16(mmtmpPD1, rxdataF128[2]); // mmtmpPD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpPD0 = _mm_srai_epi32(mmtmpPD0, output_shift);
        mmtmpPD1 = _mm_srai_epi32(mmtmpPD1, output_shift);
        mmtmpPD2 = _mm_unpacklo_epi32(mmtmpPD0, mmtmpPD1);
        mmtmpPD3 = _mm_unpackhi_epi32(mmtmpPD0, mmtmpPD1);
        rxdataF_comp128[2] = _mm_packs_epi32(mmtmpPD2, mmtmpPD3);
        // We compute the third part of 4 symbols contained in one entire RB = 12 RE
        //if (rb < 6 || rb > 95) {
        #ifdef NR_PDCCH_DCI_DEBUG
          print_shorts("\t\trxdataF_ext:", rxdataF128);
          print_shorts("\t\tdl_ch:", dl_ch128);
          print_shorts("\t\trxdataF_comp:", rxdataF_comp128);
          print_shorts("\t\trxdataF_ext:", rxdataF128 + 1);
          print_shorts("\t\tdl_ch:", dl_ch128 + 1);
          print_shorts("\t\trxdataF_comp:", rxdataF_comp128 + 1);
          print_shorts("\t\ttrxdataF_ext:", rxdataF128 + 2);
          print_shorts("\t\tdl_ch:", dl_ch128 + 2);
          print_shorts("\t\trxdataF_comp:", rxdataF_comp128 + 2);
        #endif
        //}
        dl_ch128 += 3;
        rxdataF128 += 3;
        rxdataF_comp128 += 3;
        //if ((rb + 1) % 4 == 3)
        //rb++;
        // if rxdataF_comp does contains pilot DM-RS PDCCH, as in previous code rxdataF_comp128 is a set of 4 consecutive outputs (32-bit)
        // the computation of third part of 4 symbols contains last 9th symbol of the current rb + 3 first symbols of the next rb
        // so rb computing must be take this into consideration, and every 4 rb, rb must be increased twice
        //}

// This code will replace the code below for nr
//#else
/*
				if (pilots == 0) {
					mmtmpPD0 = _mm_madd_epi16(dl_ch128[2],rxdataF128[2]);
					// mmtmpPD0 contains real part of 4 consecutive outputs (32-bit)
					mmtmpPD1 = _mm_shufflelo_epi16(dl_ch128[2],_MM_SHUFFLE(2,3,0,1));
					mmtmpPD1 = _mm_shufflehi_epi16(mmtmpPD1,_MM_SHUFFLE(2,3,0,1));
					mmtmpPD1 = _mm_sign_epi16(mmtmpPD1,*(__m128i*)conjugate);
					mmtmpPD1 = _mm_madd_epi16(mmtmpPD1,rxdataF128[2]);
					// mmtmpPD1 contains imag part of 4 consecutive outputs (32-bit)
					mmtmpPD0 = _mm_srai_epi32(mmtmpPD0,output_shift);
					mmtmpPD1 = _mm_srai_epi32(mmtmpPD1,output_shift);
					mmtmpPD2 = _mm_unpacklo_epi32(mmtmpPD0,mmtmpPD1);
					mmtmpPD3 = _mm_unpackhi_epi32(mmtmpPD0,mmtmpPD1);

					rxdataF_comp128[2] = _mm_packs_epi32(mmtmpPD2,mmtmpPD3);
				}

				print_shorts("rx:",rxdataF128);
				print_shorts("ch:",dl_ch128);
				print_shorts("pack:",rxdataF_comp128);
				print_shorts("\trx:",rxdataF128+1);
				print_shorts("\tch:",dl_ch128+1);
				print_shorts("\tpack:",rxdataF_comp128+1);
				print_shorts("\t\trx:",rxdataF128+2);
				print_shorts("\t\tch:",dl_ch128+2);
				print_shorts("\t\tpack:",rxdataF_comp128+2);

				if (pilots==0) {
					dl_ch128+=3;
					rxdataF128+=3;
					rxdataF_comp128+=3;
				} else {
					dl_ch128+=2;
					rxdataF128+=2;
					rxdataF_comp128+=2;
				}

*/
//#endif

#elif defined(__arm__)

#endif
      }
    }
  }

	if (rho) {

		for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++) {

#if defined(__x86_64__) || defined(__i386__)
			rho128 = (__m128i *) &rho[aarx][symbol * frame_parms->N_RB_DL * 12];
			dl_ch128 = (__m128i *) &dl_ch_estimates_ext[aarx][symbol
					* frame_parms->N_RB_DL * 12];
			dl_ch128_2 = (__m128i *) &dl_ch_estimates_ext[2 + aarx][symbol
					* frame_parms->N_RB_DL * 12];

#elif defined(__arm__)

#endif

//for (rb = 0; rb < frame_parms->N_RB_DL; rb++) {
for (rb = 0; rb < coreset_nbr_rb; rb++) {
#if defined(__x86_64__) || defined(__i386__)

				// multiply by conjugated channel
				mmtmpPD0 = _mm_madd_epi16(dl_ch128[0], dl_ch128_2[0]);
				//  print_ints("re",&mmtmpD0);

				// mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
				mmtmpPD1 = _mm_shufflelo_epi16(dl_ch128[0],
						_MM_SHUFFLE(2,3,0,1));
				mmtmpPD1 = _mm_shufflehi_epi16(mmtmpPD1, _MM_SHUFFLE(2,3,0,1));
				mmtmpPD1 = _mm_sign_epi16(mmtmpPD1, *(__m128i * )&conjugate[0]);
				//  print_ints("im",&mmtmpPD1);
				mmtmpPD1 = _mm_madd_epi16(mmtmpPD1, dl_ch128_2[0]);
				// mmtmpPD1 contains imag part of 4 consecutive outputs (32-bit)
				mmtmpPD0 = _mm_srai_epi32(mmtmpPD0, output_shift);
				//  print_ints("re(shift)",&mmtmpD0);
				mmtmpPD1 = _mm_srai_epi32(mmtmpPD1, output_shift);
				//  print_ints("im(shift)",&mmtmpD1);
				mmtmpPD2 = _mm_unpacklo_epi32(mmtmpPD0, mmtmpPD1);
				mmtmpPD3 = _mm_unpackhi_epi32(mmtmpPD0, mmtmpPD1);
				//        print_ints("c0",&mmtmpPD2);
				//  print_ints("c1",&mmtmpPD3);
				rho128[0] = _mm_packs_epi32(mmtmpPD2, mmtmpPD3);

				//print_shorts("rx:",dl_ch128_2);
				//print_shorts("ch:",dl_ch128);
				//print_shorts("pack:",rho128);

				// multiply by conjugated channel
				mmtmpPD0 = _mm_madd_epi16(dl_ch128[1], dl_ch128_2[1]);
				// mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
				mmtmpPD1 = _mm_shufflelo_epi16(dl_ch128[1],
						_MM_SHUFFLE(2,3,0,1));
				mmtmpPD1 = _mm_shufflehi_epi16(mmtmpPD1, _MM_SHUFFLE(2,3,0,1));
				mmtmpPD1 = _mm_sign_epi16(mmtmpPD1, *(__m128i * )conjugate);
				mmtmpPD1 = _mm_madd_epi16(mmtmpPD1, dl_ch128_2[1]);
				// mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
				mmtmpPD0 = _mm_srai_epi32(mmtmpPD0, output_shift);
				mmtmpPD1 = _mm_srai_epi32(mmtmpPD1, output_shift);
				mmtmpPD2 = _mm_unpacklo_epi32(mmtmpPD0, mmtmpPD1);
				mmtmpPD3 = _mm_unpackhi_epi32(mmtmpPD0, mmtmpPD1);

				rho128[1] = _mm_packs_epi32(mmtmpPD2, mmtmpPD3);
				//print_shorts("rx:",dl_ch128_2+1);
				//print_shorts("ch:",dl_ch128+1);
				//print_shorts("pack:",rho128+1);
				// multiply by conjugated channel
				mmtmpPD0 = _mm_madd_epi16(dl_ch128[2], dl_ch128_2[2]);
				// mmtmpPD0 contains real part of 4 consecutive outputs (32-bit)
				mmtmpPD1 = _mm_shufflelo_epi16(dl_ch128[2],
						_MM_SHUFFLE(2,3,0,1));
				mmtmpPD1 = _mm_shufflehi_epi16(mmtmpPD1, _MM_SHUFFLE(2,3,0,1));
				mmtmpPD1 = _mm_sign_epi16(mmtmpPD1, *(__m128i * )conjugate);
				mmtmpPD1 = _mm_madd_epi16(mmtmpPD1, dl_ch128_2[2]);
				// mmtmpPD1 contains imag part of 4 consecutive outputs (32-bit)
				mmtmpPD0 = _mm_srai_epi32(mmtmpPD0, output_shift);
				mmtmpPD1 = _mm_srai_epi32(mmtmpPD1, output_shift);
				mmtmpPD2 = _mm_unpacklo_epi32(mmtmpPD0, mmtmpPD1);
				mmtmpPD3 = _mm_unpackhi_epi32(mmtmpPD0, mmtmpPD1);

				rho128[2] = _mm_packs_epi32(mmtmpPD2, mmtmpPD3);
				//print_shorts("rx:",dl_ch128_2+2);
				//print_shorts("ch:",dl_ch128+2);
				//print_shorts("pack:",rho128+2);

				dl_ch128 += 3;
				dl_ch128_2 += 3;
				rho128 += 3;

#elif defined(__arm_)

#endif
			}
		}

	}

#if defined(__x86_64__) || defined(__i386__)
	_mm_empty();
	_m_empty();
#endif
}

#endif

/*
void pdcch_channel_compensation(int32_t **rxdataF_ext,
                                int32_t **dl_ch_estimates_ext,
                                int32_t **rxdataF_comp,
                                int32_t **rho,
                                NR_DL_FRAME_PARMS *frame_parms,
                                uint8_t symbol,
                                uint8_t output_shift)
{

  uint16_t rb;
#if defined(__x86_64__) || defined(__i386__)
  __m128i *dl_ch128,*rxdataF128,*rxdataF_comp128;
  __m128i *dl_ch128_2, *rho128;
#elif defined(__arm__)

#endif
  uint8_t aatx,aarx,pilots=0;




#ifdef DEBUG_DCI_DECODING
  LOG_I(PHY, "PDCCH comp: symbol %d\n",symbol);
#endif

  if (symbol==0)
    pilots=1;

  for (aatx=0; aatx<frame_parms->nb_antenna_ports_eNB; aatx++) {
    //if (frame_parms->mode1_flag && aatx>0) break; //if mode1_flag is set then there is only one stream to extract, independent of nb_antenna_ports_eNB

    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

#if defined(__x86_64__) || defined(__i386__)
      dl_ch128          = (__m128i *)&dl_ch_estimates_ext[(aatx<<1)+aarx][symbol*frame_parms->N_RB_DL*12];
      rxdataF128        = (__m128i *)&rxdataF_ext[aarx][symbol*frame_parms->N_RB_DL*12];
      rxdataF_comp128   = (__m128i *)&rxdataF_comp[(aatx<<1)+aarx][symbol*frame_parms->N_RB_DL*12];
#elif defined(__arm__)

#endif

      for (rb=0; rb<frame_parms->N_RB_DL; rb++) {

#if defined(__x86_64__) || defined(__i386__)
        // multiply by conjugated channel
        mmtmpPD0 = _mm_madd_epi16(dl_ch128[0],rxdataF128[0]);
        //  print_ints("re",&mmtmpPD0);

        // mmtmpPD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpPD1 = _mm_shufflelo_epi16(dl_ch128[0],_MM_SHUFFLE(2,3,0,1));
        mmtmpPD1 = _mm_shufflehi_epi16(mmtmpPD1,_MM_SHUFFLE(2,3,0,1));
        mmtmpPD1 = _mm_sign_epi16(mmtmpPD1,*(__m128i*)&conjugate[0]);
        //  print_ints("im",&mmtmpPD1);
        mmtmpPD1 = _mm_madd_epi16(mmtmpPD1,rxdataF128[0]);
        // mmtmpPD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpPD0 = _mm_srai_epi32(mmtmpPD0,output_shift);
        //  print_ints("re(shift)",&mmtmpPD0);
        mmtmpPD1 = _mm_srai_epi32(mmtmpPD1,output_shift);
        //  print_ints("im(shift)",&mmtmpPD1);
        mmtmpPD2 = _mm_unpacklo_epi32(mmtmpPD0,mmtmpPD1);
        mmtmpPD3 = _mm_unpackhi_epi32(mmtmpPD0,mmtmpPD1);
        //        print_ints("c0",&mmtmpPD2);
        //  print_ints("c1",&mmtmpPD3);
        rxdataF_comp128[0] = _mm_packs_epi32(mmtmpPD2,mmtmpPD3);
        //  print_shorts("rx:",rxdataF128);
        //  print_shorts("ch:",dl_ch128);
        //  print_shorts("pack:",rxdataF_comp128);

        // multiply by conjugated channel
        mmtmpPD0 = _mm_madd_epi16(dl_ch128[1],rxdataF128[1]);
        // mmtmpPD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpPD1 = _mm_shufflelo_epi16(dl_ch128[1],_MM_SHUFFLE(2,3,0,1));
        mmtmpPD1 = _mm_shufflehi_epi16(mmtmpPD1,_MM_SHUFFLE(2,3,0,1));
        mmtmpPD1 = _mm_sign_epi16(mmtmpPD1,*(__m128i*)conjugate);
        mmtmpPD1 = _mm_madd_epi16(mmtmpPD1,rxdataF128[1]);
        // mmtmpPD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpPD0 = _mm_srai_epi32(mmtmpPD0,output_shift);
        mmtmpPD1 = _mm_srai_epi32(mmtmpPD1,output_shift);
        mmtmpPD2 = _mm_unpacklo_epi32(mmtmpPD0,mmtmpPD1);
        mmtmpPD3 = _mm_unpackhi_epi32(mmtmpPD0,mmtmpPD1);

        rxdataF_comp128[1] = _mm_packs_epi32(mmtmpPD2,mmtmpPD3);

        //  print_shorts("rx:",rxdataF128+1);
        //  print_shorts("ch:",dl_ch128+1);
        //  print_shorts("pack:",rxdataF_comp128+1);
        // multiply by conjugated channel
        if (pilots == 0) {
          mmtmpPD0 = _mm_madd_epi16(dl_ch128[2],rxdataF128[2]);
          // mmtmpPD0 contains real part of 4 consecutive outputs (32-bit)
          mmtmpPD1 = _mm_shufflelo_epi16(dl_ch128[2],_MM_SHUFFLE(2,3,0,1));
          mmtmpPD1 = _mm_shufflehi_epi16(mmtmpPD1,_MM_SHUFFLE(2,3,0,1));
          mmtmpPD1 = _mm_sign_epi16(mmtmpPD1,*(__m128i*)conjugate);
          mmtmpPD1 = _mm_madd_epi16(mmtmpPD1,rxdataF128[2]);
          // mmtmpPD1 contains imag part of 4 consecutive outputs (32-bit)
          mmtmpPD0 = _mm_srai_epi32(mmtmpPD0,output_shift);
          mmtmpPD1 = _mm_srai_epi32(mmtmpPD1,output_shift);
          mmtmpPD2 = _mm_unpacklo_epi32(mmtmpPD0,mmtmpPD1);
          mmtmpPD3 = _mm_unpackhi_epi32(mmtmpPD0,mmtmpPD1);

          rxdataF_comp128[2] = _mm_packs_epi32(mmtmpPD2,mmtmpPD3);
        }

        //  print_shorts("rx:",rxdataF128+2);
        //  print_shorts("ch:",dl_ch128+2);
        //        print_shorts("pack:",rxdataF_comp128+2);

        if (pilots==0) {
          dl_ch128+=3;
          rxdataF128+=3;
          rxdataF_comp128+=3;
        } else {
          dl_ch128+=2;
          rxdataF128+=2;
          rxdataF_comp128+=2;
        }
#elif defined(__arm__)

#endif
      }
    }
  }


  if (rho) {

    for (aarx=0; aarx<frame_parms->nb_antennas_rx; aarx++) {

#if defined(__x86_64__) || defined(__i386__)
      rho128        = (__m128i *)&rho[aarx][symbol*frame_parms->N_RB_DL*12];
      dl_ch128      = (__m128i *)&dl_ch_estimates_ext[aarx][symbol*frame_parms->N_RB_DL*12];
      dl_ch128_2    = (__m128i *)&dl_ch_estimates_ext[2+aarx][symbol*frame_parms->N_RB_DL*12];

#elif defined(__arm__)
      
#endif
      for (rb=0; rb<frame_parms->N_RB_DL; rb++) {
#if defined(__x86_64__) || defined(__i386__)

        // multiply by conjugated channel
        mmtmpPD0 = _mm_madd_epi16(dl_ch128[0],dl_ch128_2[0]);
        //  print_ints("re",&mmtmpD0);

        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpPD1 = _mm_shufflelo_epi16(dl_ch128[0],_MM_SHUFFLE(2,3,0,1));
        mmtmpPD1 = _mm_shufflehi_epi16(mmtmpPD1,_MM_SHUFFLE(2,3,0,1));
        mmtmpPD1 = _mm_sign_epi16(mmtmpPD1,*(__m128i*)&conjugate[0]);
        //  print_ints("im",&mmtmpPD1);
        mmtmpPD1 = _mm_madd_epi16(mmtmpPD1,dl_ch128_2[0]);
        // mmtmpPD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpPD0 = _mm_srai_epi32(mmtmpPD0,output_shift);
        //  print_ints("re(shift)",&mmtmpD0);
        mmtmpPD1 = _mm_srai_epi32(mmtmpPD1,output_shift);
        //  print_ints("im(shift)",&mmtmpD1);
        mmtmpPD2 = _mm_unpacklo_epi32(mmtmpPD0,mmtmpPD1);
        mmtmpPD3 = _mm_unpackhi_epi32(mmtmpPD0,mmtmpPD1);
        //        print_ints("c0",&mmtmpPD2);
        //  print_ints("c1",&mmtmpPD3);
        rho128[0] = _mm_packs_epi32(mmtmpPD2,mmtmpPD3);

        //print_shorts("rx:",dl_ch128_2);
        //print_shorts("ch:",dl_ch128);
        //print_shorts("pack:",rho128);

        // multiply by conjugated channel
        mmtmpPD0 = _mm_madd_epi16(dl_ch128[1],dl_ch128_2[1]);
        // mmtmpD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpPD1 = _mm_shufflelo_epi16(dl_ch128[1],_MM_SHUFFLE(2,3,0,1));
        mmtmpPD1 = _mm_shufflehi_epi16(mmtmpPD1,_MM_SHUFFLE(2,3,0,1));
        mmtmpPD1 = _mm_sign_epi16(mmtmpPD1,*(__m128i*)conjugate);
        mmtmpPD1 = _mm_madd_epi16(mmtmpPD1,dl_ch128_2[1]);
        // mmtmpD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpPD0 = _mm_srai_epi32(mmtmpPD0,output_shift);
        mmtmpPD1 = _mm_srai_epi32(mmtmpPD1,output_shift);
        mmtmpPD2 = _mm_unpacklo_epi32(mmtmpPD0,mmtmpPD1);
        mmtmpPD3 = _mm_unpackhi_epi32(mmtmpPD0,mmtmpPD1);


        rho128[1] =_mm_packs_epi32(mmtmpPD2,mmtmpPD3);
        //print_shorts("rx:",dl_ch128_2+1);
        //print_shorts("ch:",dl_ch128+1);
        //print_shorts("pack:",rho128+1);
        // multiply by conjugated channel
        mmtmpPD0 = _mm_madd_epi16(dl_ch128[2],dl_ch128_2[2]);
        // mmtmpPD0 contains real part of 4 consecutive outputs (32-bit)
        mmtmpPD1 = _mm_shufflelo_epi16(dl_ch128[2],_MM_SHUFFLE(2,3,0,1));
        mmtmpPD1 = _mm_shufflehi_epi16(mmtmpPD1,_MM_SHUFFLE(2,3,0,1));
        mmtmpPD1 = _mm_sign_epi16(mmtmpPD1,*(__m128i*)conjugate);
        mmtmpPD1 = _mm_madd_epi16(mmtmpPD1,dl_ch128_2[2]);
        // mmtmpPD1 contains imag part of 4 consecutive outputs (32-bit)
        mmtmpPD0 = _mm_srai_epi32(mmtmpPD0,output_shift);
        mmtmpPD1 = _mm_srai_epi32(mmtmpPD1,output_shift);
        mmtmpPD2 = _mm_unpacklo_epi32(mmtmpPD0,mmtmpPD1);
        mmtmpPD3 = _mm_unpackhi_epi32(mmtmpPD0,mmtmpPD1);

        rho128[2] = _mm_packs_epi32(mmtmpPD2,mmtmpPD3);
        //print_shorts("rx:",dl_ch128_2+2);
        //print_shorts("ch:",dl_ch128+2);
        //print_shorts("pack:",rho128+2);

        dl_ch128+=3;
        dl_ch128_2+=3;
        rho128+=3;

#elif defined(__arm_)


#endif
      }
    }

  }

#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif
}
*/
void pdcch_detection_mrc(NR_DL_FRAME_PARMS *frame_parms,
                         int32_t **rxdataF_comp,
                         uint8_t symbol)
{

  uint8_t aatx;

#if defined(__x86_64__) || defined(__i386__)
  __m128i *rxdataF_comp128_0,*rxdataF_comp128_1;
#elif defined(__arm__)
 int16x8_t *rxdataF_comp128_0,*rxdataF_comp128_1;
#endif
  int32_t i;

  if (frame_parms->nb_antennas_rx>1) {
    for (aatx=0; aatx<frame_parms->nb_antenna_ports_eNB; aatx++) {
#if defined(__x86_64__) || defined(__i386__)
      rxdataF_comp128_0   = (__m128i *)&rxdataF_comp[(aatx<<1)][symbol*frame_parms->N_RB_DL*12];
      rxdataF_comp128_1   = (__m128i *)&rxdataF_comp[(aatx<<1)+1][symbol*frame_parms->N_RB_DL*12];
#elif defined(__arm__)
      rxdataF_comp128_0   = (int16x8_t *)&rxdataF_comp[(aatx<<1)][symbol*frame_parms->N_RB_DL*12];
      rxdataF_comp128_1   = (int16x8_t *)&rxdataF_comp[(aatx<<1)+1][symbol*frame_parms->N_RB_DL*12];
#endif
      // MRC on each re of rb
      for (i=0; i<frame_parms->N_RB_DL*3; i++) {
#if defined(__x86_64__) || defined(__i386__)
        rxdataF_comp128_0[i] = _mm_adds_epi16(_mm_srai_epi16(rxdataF_comp128_0[i],1),_mm_srai_epi16(rxdataF_comp128_1[i],1));
#elif defined(__arm__)
        rxdataF_comp128_0[i] = vhaddq_s16(rxdataF_comp128_0[i],rxdataF_comp128_1[i]);
#endif
      }
    }
  }

#if defined(__x86_64__) || defined(__i386__)
  _mm_empty();
  _m_empty();
#endif

}

void pdcch_siso(NR_DL_FRAME_PARMS *frame_parms,
                int32_t **rxdataF_comp,
                uint8_t l)
{


  uint8_t rb,re,jj,ii;

  jj=0;
  ii=0;

  for (rb=0; rb<frame_parms->N_RB_DL; rb++) {

    for (re=0; re<12; re++) {

      rxdataF_comp[0][jj++] = rxdataF_comp[0][ii];
      ii++;
    }
  }
}


void pdcch_alamouti(NR_DL_FRAME_PARMS *frame_parms,
                    int32_t **rxdataF_comp,
                    uint8_t symbol)
{


  int16_t *rxF0,*rxF1;
  uint8_t rb,re;
  int32_t jj=(symbol*frame_parms->N_RB_DL*12);

  rxF0     = (int16_t*)&rxdataF_comp[0][jj];  //tx antenna 0  h0*y
  rxF1     = (int16_t*)&rxdataF_comp[2][jj];  //tx antenna 1  h1*y

  for (rb=0; rb<frame_parms->N_RB_DL; rb++) {

    for (re=0; re<12; re+=2) {

      // Alamouti RX combining

      rxF0[0] = rxF0[0] + rxF1[2];
      rxF0[1] = rxF0[1] - rxF1[3];

      rxF0[2] = rxF0[2] - rxF1[0];
      rxF0[3] = rxF0[3] + rxF1[1];

      rxF0+=4;
      rxF1+=4;
    }
  }


}

int32_t avgP[4];




#ifdef NR_PDCCH_DCI_RUN
int32_t nr_rx_pdcch(PHY_VARS_NR_UE *ue,
                    uint32_t frame,
                    uint8_t nr_tti_rx,
                    uint8_t eNB_id,
                    MIMO_mode_t mimo_mode,
                    uint32_t high_speed_flag,
                    uint8_t is_secondary_ue,
                    int nb_coreset_active,
                    uint16_t symbol_mon,
                    int do_common) {

	#ifdef MU_RECEIVER
	uint8_t eNB_id_i=eNB_id+1; //add 1 to eNB_id to separate from wanted signal, chosen as the B/F'd pilots from the SeNB are shifted by 1
#endif

  NR_UE_COMMON *common_vars      = &ue->common_vars;
  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  NR_UE_PDCCH **pdcch_vars       = ue->pdcch_vars[ue->current_thread_id[nr_tti_rx]];
  NR_UE_PDCCH *pdcch_vars2       = ue->pdcch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id];

  uint8_t log2_maxh, aatx, aarx;
  int32_t avgs;
  uint8_t n_pdcch_symbols;
  // the variable mi can be removed for NR
  uint8_t mi = get_mi(frame_parms, nr_tti_rx);

/*
 * The following variables have been extracted from higher layer parameters
 * MIB1 => pdcchConfigSIB1
 * ControlResourceSet IE
 * pdcch-Config
 * pdcch-ConfigCommon
 */

/*
 * initialize this values for testing
 */
#if 0
        pdcch_vars2->coreset[nb_coreset_active].frequencyDomainResources                  = 0x1FFF2FF00000;
        //pdcch_vars2->coreset[nb_coreset_active].frequencyDomainResources                  = 0x1E0000000000;
        pdcch_vars2->coreset[nb_coreset_active].duration                                  = 2;
        pdcch_vars2->coreset[nb_coreset_active].cce_reg_mappingType.shiftIndex            = 0;
        pdcch_vars2->coreset[nb_coreset_active].cce_reg_mappingType.reg_bundlesize        = bundle_n6;
        pdcch_vars2->coreset[nb_coreset_active].cce_reg_mappingType.interleaversize       = interleave_n2;
        pdcch_vars2->coreset[nb_coreset_active].pdcchDMRSScramblingID                     = 1;
        for (int i=0; i < NR_NBR_SEARCHSPACE_ACT_BWP; i++){
          pdcch_vars[eNB_id]->searchSpace[i].nrofCandidates_aggrlevel1                      = 7;
          pdcch_vars[eNB_id]->searchSpace[i].nrofCandidates_aggrlevel2                      = 6;
          pdcch_vars[eNB_id]->searchSpace[i].nrofCandidates_aggrlevel4                      = 4;
          pdcch_vars[eNB_id]->searchSpace[i].nrofCandidates_aggrlevel8                      = 3;
          pdcch_vars[eNB_id]->searchSpace[i].nrofCandidates_aggrlevel16                     = 1;
          pdcch_vars[eNB_id]->searchSpace[i].searchSpaceType.sfi_nrofCandidates_aggrlevel1  = 7;
          pdcch_vars[eNB_id]->searchSpace[i].searchSpaceType.sfi_nrofCandidates_aggrlevel2  = 6;
          pdcch_vars[eNB_id]->searchSpace[i].searchSpaceType.sfi_nrofCandidates_aggrlevel4  = 4;
          pdcch_vars[eNB_id]->searchSpace[i].searchSpaceType.sfi_nrofCandidates_aggrlevel8  = 3;
          pdcch_vars[eNB_id]->searchSpace[i].searchSpaceType.sfi_nrofCandidates_aggrlevel16 = 1;
        }
#endif //(0)
/*
 * to be removed after testing
 */

  // number of RB (1 symbol) or REG (12 RE) in one CORESET: higher-layer parameter CORESET-freq-dom
  // (bit map 45 bits: each bit indicates 6 RB in CORESET -> 1 bit MSB indicates PRB 0..6 are part of CORESET)
  uint64_t coreset_freq_dom                                 = pdcch_vars2->coreset[nb_coreset_active].frequencyDomainResources;
  // number of symbols in CORESET: higher-layer parameter CORESET-time-dur {1,2,3}
  int coreset_time_dur                                      = pdcch_vars2->coreset[nb_coreset_active].duration;
  // depends on higher-layer parameter CORESET-shift-index {0,1,...,274}
  int n_shift                                               = pdcch_vars2->coreset[nb_coreset_active].cce_reg_mappingType.shiftIndex;
  // higher-layer parameter CORESET-REG-bundle-size (for non-interleaved L = 6 / for interleaved L {2,6})
  NR_UE_CORESET_REG_bundlesize_t reg_bundle_size_L          = pdcch_vars2->coreset[nb_coreset_active].cce_reg_mappingType.reg_bundlesize;
  // higher-layer parameter CORESET-interleaver-size {2,3,6}
  NR_UE_CORESET_interleaversize_t coreset_interleaver_size_R= pdcch_vars2->coreset[nb_coreset_active].cce_reg_mappingType.interleaversize;
  NR_UE_CORESET_precoder_granularity_t precoder_granularity = pdcch_vars2->coreset[nb_coreset_active].precoderGranularity;
  int tci_statesPDCCH                                       = pdcch_vars2->coreset[nb_coreset_active].tciStatesPDCCH;
  int tci_present                                           = pdcch_vars2->coreset[nb_coreset_active].tciPresentInDCI;
  uint16_t pdcch_DMRS_scrambling_id                         = pdcch_vars2->coreset[nb_coreset_active].pdcchDMRSScramblingID;

  // The UE can be assigned 4 different BWP but only one active at a time.
  // For each BWP the number of CORESETs is limited to 3 (including initial CORESET Id=0 -> ControlResourceSetId (0..maxNrofControlReourceSets-1) (0..12-1)
  uint32_t n_BWP_start = 0;
  // start time position for CORESET
  // parameter symbol_mon is a 14 bits bitmap indicating monitoring symbols within a slot
  uint8_t start_symbol = 0;

  // at the moment we are considering that the PDCCH is always starting at symbol 0 of current slot
  // the following code to initialize start_symbol must be activated once we implement PDCCH demapping on symbol not equal to 0 (considering symbol_mon)
  /*for (int i=0; i < 14; i++) {
    if (symbol_mon >> (13-i) != 0) {
      start_symbol = i;
      i=14;
    }
  }*/

  //
  // according to 38.213 v15.1.0: a PDCCH monitoring pattern within a slot,
  // indicating first symbol(s) of the control resource set within a slot
  // for PDCCH monitoring, by higher layer parameter monitoringSymbolsWithinSlot
  //
  // at the moment we do not implement this and start_symbol is always 0
  // note that the bitmap symbol_mon may indicate several monitoring times within a same slot (symbols 0..13)
  // this may lead to a modification in ue scheduler

  // indicates the number of active CORESETs for the current BWP to decode PDCCH: max is 3 (this variable is not useful here, to be removed)
  uint8_t  coreset_nbr_act;
  // indicates the number of REG contained in the PDCCH (number of RBs * number of symbols, in CORESET)
  uint8_t  coreset_nbr_reg;
  uint32_t coreset_C;
  uint32_t coreset_nbr_rb = 0;

  // for (int j=0; j < coreset_nbr_act; j++) {
  // for each active CORESET (max number of active CORESETs in a BWP is 3),
  // we calculate the number of RB for each CORESET bitmap
  #ifdef NR_PDCCH_DCI_DEBUG
    printf("\t<-NR_PDCCH_DCI_DEBUG (nr_rx_pdcch)-> coreset_freq_dom=(%lld)\n",coreset_freq_dom);
  #endif
  int i; //for each bit in the coreset_freq_dom bitmap
  for (i = 0; i < 45; i++) {
    // this loop counts each bit of the bit map coreset_freq_dom, and increments nbr_RB_coreset for each bit set to '1'
    if (((coreset_freq_dom & 0x1FFFFFFFFFFF) >> i) & 0x1) coreset_nbr_rb++;
  }
  coreset_nbr_rb = 6 * coreset_nbr_rb; // coreset_nbr_rb has to be multiplied by 6 to indicate the number of PRB or REG(=12 RE) within the CORESET
  #ifdef NR_PDCCH_DCI_DEBUG
    printf("\t<-NR_PDCCH_DCI_DEBUG (nr_rx_pdcch)-> coreset_freq_dom=(%lld,%llx), coreset_nbr_rb=%d\n", coreset_freq_dom,coreset_freq_dom,coreset_nbr_rb);
  #endif
  coreset_nbr_reg = coreset_time_dur * coreset_nbr_rb;
  coreset_C = (uint32_t)(coreset_nbr_reg / (reg_bundle_size_L * coreset_interleaver_size_R));
  #ifdef NR_PDCCH_DCI_DEBUG
    printf("\t<-NR_PDCCH_DCI_DEBUG (nr_rx_pdcch)-> coreset_nbr_rb=%d, coreset_nbr_reg=%d, coreset_C=(%d/(%d*%d))=%d\n",
            coreset_nbr_rb, coreset_nbr_reg, coreset_nbr_reg, reg_bundle_size_L,coreset_interleaver_size_R, coreset_C);
  #endif

  for (int s = start_symbol; s < (start_symbol + coreset_time_dur); s++) {
    printf("\t<-NR_PDCCH_DCI_DEBUG (nr_rx_pdcch)-> we enter process pdcch ofdm symbol s=%d where coreset_time_dur=%d\n",s,coreset_time_dur);

	if (is_secondary_ue == 1) {
		pdcch_extract_rbs_single(common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].rxdataF,
				common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].dl_ch_estimates[eNB_id+1], //add 1 to eNB_id to compensate for the shifted B/F'd pilots from the SeNB
				pdcch_vars[eNB_id]->rxdataF_ext,
				pdcch_vars[eNB_id]->dl_ch_estimates_ext,
				0,
				high_speed_flag,
				frame_parms);
#ifdef MU_RECEIVER
		pdcch_extract_rbs_single(common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].rxdataF,
				common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].dl_ch_estimates[eNB_id_i - 1], //subtract 1 to eNB_id_i to compensate for the non-shifted pilots from the PeNB
				pdcch_vars[eNB_id_i]->rxdataF_ext,//shift by two to simulate transmission from a second antenna
				pdcch_vars[eNB_id_i]->dl_ch_estimates_ext,//shift by two to simulate transmission from a second antenna
				0,
				high_speed_flag,
				frame_parms);
#endif //MU_RECEIVER
	} else if (frame_parms->nb_antenna_ports_eNB>1) {
		pdcch_extract_rbs_dual(common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].rxdataF,
				common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].dl_ch_estimates[eNB_id],
				pdcch_vars[eNB_id]->rxdataF_ext,
				pdcch_vars[eNB_id]->dl_ch_estimates_ext,
				0,
				high_speed_flag,
				frame_parms);
	} else {
    #ifdef NR_PDCCH_DCI_DEBUG
      printf("\t<-NR_PDCCH_DCI_DEBUG (nr_rx_pdcch)-> we enter nr_pdcch_extract_rbs_single(is_secondary_ue=%d) to remove DM-RS PDCCH\n",
              is_secondary_ue);
    #endif
    nr_pdcch_extract_rbs_single(common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].rxdataF,
                                common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].dl_ch_estimates[eNB_id],
                                pdcch_vars[eNB_id]->rxdataF_ext,
                                pdcch_vars[eNB_id]->dl_ch_estimates_ext,
                                s,
                                high_speed_flag,
                                frame_parms,
                                coreset_freq_dom,
                                coreset_nbr_rb,
                                n_BWP_start);
/*
	printf("\t### in nr_rx_pdcch() function we enter pdcch_extract_rbs_single(is_secondary_ue=%d) to remove DM-RS PDCCH\n",is_secondary_ue);
	pdcch_extract_rbs_single(common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].rxdataF,
			common_vars->common_vars_rx_data_per_thread[ue->current_thread_id[nr_tti_rx]].dl_ch_estimates[eNB_id],
			pdcch_vars[eNB_id]->rxdataF_ext,
			pdcch_vars[eNB_id]->dl_ch_estimates_ext,
			0,
			high_speed_flag,
			frame_parms);
*/

}

    #ifdef NR_PDCCH_DCI_DEBUG
      printf("\t<-NR_PDCCH_DCI_DEBUG (nr_rx_pdcch)-> we enter pdcch_channel_level(avgP=%d) => compute channel level based on ofdm symbol 0, pdcch_vars[eNB_id]->dl_ch_estimates_ext\n",avgP);
    #endif
    // compute channel level based on ofdm symbol 0
    pdcch_channel_level(pdcch_vars[eNB_id]->dl_ch_estimates_ext,
                        frame_parms,
                        avgP,
                        frame_parms->N_RB_DL);
    avgs = 0;
    for (aatx = 0; aatx < frame_parms->nb_antenna_ports_eNB; aatx++)
      for (aarx = 0; aarx < frame_parms->nb_antennas_rx; aarx++)
        avgs = cmax(avgs, avgP[(aarx << 1) + aatx]);
    log2_maxh = (log2_approx(avgs) / 2) + 5;  //+frame_parms->nb_antennas_rx;
#ifdef UE_DEBUG_TRACE
LOG_D(PHY,"nr_tti_rx %d: pdcch log2_maxh = %d (%d,%d)\n",nr_tti_rx,log2_maxh,avgP[0],avgs);
#endif

#if T_TRACER
T(T_UE_PHY_PDCCH_ENERGY, T_INT(eNB_id), T_INT(0), T_INT(frame%1024), T_INT(nr_tti_rx),
  T_INT(avgP[0]), T_INT(avgP[1]), T_INT(avgP[2]), T_INT(avgP[3]));
#endif
    #ifdef NR_PDCCH_DCI_DEBUG
      printf("\t<-NR_PDCCH_DCI_DEBUG (nr_rx_pdcch)-> we enter nr_pdcch_channel_compensation(log2_maxh=%d)\n",log2_maxh);
    #endif
    // compute LLRs for ofdm symbol 0 only
    nr_pdcch_channel_compensation(pdcch_vars[eNB_id]->rxdataF_ext,
                                  pdcch_vars[eNB_id]->dl_ch_estimates_ext,
                                  pdcch_vars[eNB_id]->rxdataF_comp,
                                  (aatx > 1) ? pdcch_vars[eNB_id]->rho : NULL,
                                  frame_parms,
                                  s,
                                  log2_maxh,
                                  coreset_nbr_rb); // log2_maxh+I0_shift

/*
printf("\t### in nr_rx_pdcch() function we enter pdcch_channel_compensation(log2_maxh=%d) => compute LLRs for ofdm symbol 0 only, pdcch_vars[eNB_id]->rxdataF_ext ---> pdcch_vars[eNB_id]->rxdataF_comp\n",log2_maxh);

			// compute LLRs for ofdm symbol 0 only
			pdcch_channel_compensation(pdcch_vars[eNB_id]->rxdataF_ext,
					pdcch_vars[eNB_id]->dl_ch_estimates_ext,
					pdcch_vars[eNB_id]->rxdataF_comp,
					(aatx>1) ? pdcch_vars[eNB_id]->rho : NULL,
					frame_parms,
					0,
					log2_maxh);// log2_maxh+I0_shift
*/


#ifdef DEBUG_PHY

	if (nr_tti_rx==5)
	write_output("rxF_comp_d.m","rxF_c_d",&pdcch_vars[eNB_id]->rxdataF_comp[0][s*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);

#endif

#ifdef MU_RECEIVER

	if (is_secondary_ue) {
		//get MF output for interfering stream
		pdcch_channel_compensation(pdcch_vars[eNB_id_i]->rxdataF_ext,
				pdcch_vars[eNB_id_i]->dl_ch_estimates_ext,
				pdcch_vars[eNB_id_i]->rxdataF_comp,
				(aatx>1) ? pdcch_vars[eNB_id_i]->rho : NULL,
				frame_parms,
				0,
				log2_maxh);// log2_maxh+I0_shift
#ifdef DEBUG_PHY
		write_output("rxF_comp_i.m","rxF_c_i",&pdcch_vars[eNB_id_i]->rxdataF_comp[0][s*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);
#endif
		pdcch_dual_stream_correlation(frame_parms,
				0,
				pdcch_vars[eNB_id]->dl_ch_estimates_ext,
				pdcch_vars[eNB_id_i]->dl_ch_estimates_ext,
				pdcch_vars[eNB_id]->dl_ch_rho_ext,
				log2_maxh);
	}

#endif //MU_RECEIVER

    if (frame_parms->nb_antennas_rx > 1) {
#ifdef MU_RECEIVER

		if (is_secondary_ue) {
			pdcch_detection_mrc_i(frame_parms,
					pdcch_vars[eNB_id]->rxdataF_comp,
					pdcch_vars[eNB_id_i]->rxdataF_comp,
					pdcch_vars[eNB_id]->rho,
					pdcch_vars[eNB_id]->dl_ch_rho_ext,
					0);
#ifdef DEBUG_PHY
			write_output("rxF_comp_d.m","rxF_c_d",&pdcch_vars[eNB_id]->rxdataF_comp[0][s*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);
			write_output("rxF_comp_i.m","rxF_c_i",&pdcch_vars[eNB_id_i]->rxdataF_comp[0][s*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,1);
#endif
		} else
#endif //MU_RECEIVER
      #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t<-NR_PDCCH_DCI_DEBUG (nr_rx_pdcch)-> we enter pdcch_detection_mrc(frame_parms->nb_antennas_rx=%d)\n",
                frame_parms->nb_antennas_rx);
      #endif
      pdcch_detection_mrc(frame_parms, pdcch_vars[eNB_id]->rxdataF_comp,s);
    }
    if (mimo_mode == SISO) {
      #ifdef NR_PDCCH_DCI_DEBUG
       printf("\t<-NR_PDCCH_DCI_DEBUG (nr_rx_pdcch)-> we enter pdcch_siso(for symbol 0) ---> pdcch_vars[eNB_id]->rxdataF_comp\n");
      #endif
      pdcch_siso(frame_parms, pdcch_vars[eNB_id]->rxdataF_comp,s);
    } else pdcch_alamouti(frame_parms, pdcch_vars[eNB_id]->rxdataF_comp,s);

#ifdef MU_RECEIVER

	if (is_secondary_ue) {
		pdcch_qpsk_qpsk_llr(frame_parms,
				pdcch_vars[eNB_id]->rxdataF_comp,
				pdcch_vars[eNB_id_i]->rxdataF_comp,
				pdcch_vars[eNB_id]->dl_ch_rho_ext,
				pdcch_vars[eNB_id]->llr16, //subsequent function require 16 bit llr, but output must be 8 bit (actually clipped to 4, because of the Viterbi decoder)
				pdcch_vars[eNB_id]->llr,
				0);
		/*
		 #ifdef DEBUG_PHY
		 if (subframe==5) {
		 write_output("llr8_seq.m","llr8",&pdcch_vars[eNB_id]->llr[s*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,4);
		 write_output("llr16_seq.m","llr16",&pdcch_vars[eNB_id]->llr16[s*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,4);
		 }
		 #endif*/
	} else {
#endif //MU_RECEIVER

    #ifdef NR_PDCCH_DCI_DEBUG
      printf("\t<-NR_PDCCH_DCI_DEBUG (nr_rx_pdcch)-> we enter nr_pdcch_llr(for symbol %d), pdcch_vars[eNB_id]->rxdataF_comp ---> pdcch_vars[eNB_id]->llr \n",s);
    #endif
    nr_pdcch_llr(frame_parms,
                 pdcch_vars[eNB_id]->rxdataF_comp,
                 (char *) pdcch_vars[eNB_id]->llr,
                 s,
                 coreset_nbr_rb);
    /*
    printf("\t### in nr_rx_pdcch() function we enter pdcch_llr(for symbol 0), pdcch_vars[eNB_id]->rxdataF_comp ---> pdcch_vars[eNB_id]->llr \n");
    pdcch_llr(frame_parms, pdcch_vars[eNB_id]->rxdataF_comp,(char *) pdcch_vars[eNB_id]->llr, 0);
    */
    /*#ifdef DEBUG_PHY
    write_output("llr8_seq.m","llr8",&pdcch_vars[eNB_id]->llr[s*frame_parms->N_RB_DL*12],frame_parms->N_RB_DL*12,1,4);
    #endif*/

#ifdef MU_RECEIVER
}
#endif //MU_RECEIVER

#if T_TRACER
T(T_UE_PHY_PDCCH_IQ, T_INT(frame_parms->N_RB_DL), T_INT(frame_parms->N_RB_DL),
  T_INT(n_pdcch_symbols),
  T_BUFFER(pdcch_vars[eNB_id]->rxdataF_comp, frame_parms->N_RB_DL*12*n_pdcch_symbols* 4));
#endif

    /* We do not enter this function: in NR the number of PDCCH symbols is determined by higher layers parameter CORESET-time-dur
    /*/
    printf("\t<-NR_PDCCH_DCI_DEBUG (nr_rx_pdcch)-> we do not enter function rx_pcfich()\n as the number of PDCCH symbols is determined by higher layers parameter CORESET-time-dur and n_pdcch_symbols=%d\n",n_pdcch_symbols);
    /*
    // decode pcfich here and find out pdcch ofdm symbol number
    n_pdcch_symbols = rx_pcfich(frame_parms, nr_tti_rx, pdcch_vars[eNB_id],mimo_mode);
    if (n_pdcch_symbols > 3) n_pdcch_symbols = 1;
    */
#ifdef DEBUG_DCI_DECODING
	printf("demapping: nr_tti_rx %d, mi %d, tdd_config %d\n",nr_tti_rx,get_mi(frame_parms,nr_tti_rx),frame_parms->tdd_config);
#endif

  }

  #ifdef NR_PDCCH_DCI_DEBUG
    printf("\t<-NR_PDCCH_DCI_DEBUG (nr_rx_pdcch)-> we enter nr_pdcch_demapping()\n");
  #endif
  nr_pdcch_demapping(pdcch_vars[eNB_id]->llr,
                     pdcch_vars[eNB_id]->wbar,
                     frame_parms,
                     coreset_time_dur,
                     coreset_nbr_rb);
  #ifdef NR_PDCCH_DCI_DEBUG
    printf("\t<-NR_PDCCH_DCI_DEBUG (nr_rx_pdcch)-> we enter nr_pdcch_deinterleaving()\n");
  #endif
  nr_pdcch_deinterleaving(frame_parms,
                          (uint16_t*) pdcch_vars[eNB_id]->e_rx,
                          pdcch_vars[eNB_id]->wbar,
                          coreset_time_dur,
                          reg_bundle_size_L,
                          coreset_interleaver_size_R,
                          n_shift,
                          coreset_nbr_rb);
  #ifdef NR_PDCCH_DCI_DEBUG
    printf("\t<-NR_PDCCH_DCI_DEBUG (nr_rx_pdcch)-> we enter nr_pdcch_unscrambling()\n");
  #endif
  nr_pdcch_unscrambling(pdcch_vars[eNB_id]->crnti,
                        frame_parms,
                        nr_tti_rx,
                        pdcch_vars[eNB_id]->e_rx,
                        coreset_time_dur*coreset_nbr_rb*9*2,
                        // get_nCCE(n_pdcch_symbols, frame_parms, mi) * 72,
                        pdcch_DMRS_scrambling_id,
                        do_common);
/*
	printf("\t### in nr_rx_pdcch() function we enter pdcch_demapping()\n");

	pdcch_demapping(pdcch_vars[eNB_id]->llr,
			pdcch_vars[eNB_id]->wbar,
			frame_parms,
			n_pdcch_symbols,
			get_mi(frame_parms,nr_tti_rx));

	printf("\t### in nr_rx_pdcch() function we enter pdcch_deinterleaving()\n");

	pdcch_deinterleaving(frame_parms,
			(uint16_t*)pdcch_vars[eNB_id]->e_rx,
			pdcch_vars[eNB_id]->wbar,
			n_pdcch_symbols,
			mi);

	printf("\t### in nr_rx_pdcch() function we enter pdcch_unscrambling()\n");

	pdcch_unscrambling(frame_parms,
			nr_tti_rx,
			pdcch_vars[eNB_id]->e_rx,
			get_nCCE(n_pdcch_symbols,frame_parms,mi)*72);
*/

	pdcch_vars[eNB_id]->num_pdcch_symbols = n_pdcch_symbols;

  #ifdef NR_PDCCH_DCI_DEBUG
    printf("\t<-NR_PDCCH_DCI_DEBUG (nr_rx_pdcch)-> Ending nr_rx_pdcch() function\n");
  #endif
  return (0);
}
#endif



void pdcch_scrambling(NR_DL_FRAME_PARMS *frame_parms,
                      uint8_t nr_tti_rx,
                      uint8_t *e,
                      uint32_t length)
{
  int i;
  uint8_t reset;
  uint32_t x1, x2, s=0;

  reset = 1;
  // x1 is set in lte_gold_generic

  x2 = (nr_tti_rx<<9) + frame_parms->Nid_cell; //this is c_init in 36.211 Sec 6.8.2

  for (i=0; i<length; i++) {
    if ((i&0x1f)==0) {
      s = lte_gold_generic(&x1, &x2, reset);
      //printf("lte_gold[%d]=%x\n",i,s);
      reset = 0;
    }

    //    printf("scrambling %d : e %d, c %d\n",i,e[i],((s>>(i&0x1f))&1));
    if (e[i] != 2) // <NIL> element is 2
      e[i] = (e[i]&1) ^ ((s>>(i&0x1f))&1);
  }
}


#ifdef NR_PDCCH_DCI_RUN

void nr_pdcch_unscrambling(uint16_t crnti, NR_DL_FRAME_PARMS *frame_parms, uint8_t nr_tti_rx,
		int8_t* llr, uint32_t length, uint16_t pdcch_DMRS_scrambling_id, int do_common) {

	int i;
	uint8_t reset;
	uint32_t x1, x2, s = 0;
  uint16_t n_id; //{0,1,...,65535}
  uint32_t n_rnti;

	reset = 1;
	// x1 is set in first call to lte_gold_generic
	//do_common=1;
if (do_common){
  n_id = frame_parms->Nid_cell;
  n_rnti = 0;
} else {
  n_id = pdcch_DMRS_scrambling_id;
  n_rnti = (uint32_t)crnti;
}
//x2 = ((n_rnti * (1 << 16)) + n_id)%(1 << 31);
//uint32_t puissance_2_16 = ((1<<16)*n_rnti)+n_id;
//uint32_t puissance_2_31= (1<<30)*2;
//uint32_t calc_x2=puissance_2_16%puissance_2_31;
    x2 = (((1<<16)*n_rnti)+n_id)%((1<<30)*2); //this is c_init in 38.211 v15.1.0 Section 7.3.2.3
//	x2 = (nr_tti_rx << 9) + frame_parms->Nid_cell; //this is c_init in 36.211 Sec 6.8.2
#ifdef NR_PDCCH_DCI_DEBUG
printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_pdcch_unscrambling)->  (c_init=%d, n_id=%d, n_rnti=%d)\n",x2,n_id,n_rnti);
#endif
	for (i = 0; i < length; i++) {
		if ((i & 0x1f) == 0) {
			s = lte_gold_generic(&x1, &x2, reset);
			//      printf("lte_gold[%d]=%x\n",i,s);
			reset = 0;
		}

		//    printf("unscrambling %d : e %d, c %d => ",i,llr[i],((s>>(i&0x1f))&1));
		if (((s >> (i % 32)) & 1) == 0)
			llr[i] = -llr[i];
		//    printf("%d\n",llr[i]);

	}
}


#endif




void pdcch_unscrambling(NR_DL_FRAME_PARMS *frame_parms,
                        uint8_t nr_tti_rx,
                        int8_t* llr,
                        uint32_t length)
{

  int i;
  uint8_t reset;
  uint32_t x1, x2, s=0;

  reset = 1;
  // x1 is set in first call to lte_gold_generic

  x2 = (nr_tti_rx<<9) + frame_parms->Nid_cell; //this is c_init in 36.211 Sec 6.8.2

  for (i=0; i<length; i++) {
    if ((i&0x1f)==0) {
      s = lte_gold_generic(&x1, &x2, reset);
      //      printf("lte_gold[%d]=%x\n",i,s);
      reset = 0;
    }

    
    //    printf("unscrambling %d : e %d, c %d => ",i,llr[i],((s>>(i&0x1f))&1));
    if (((s>>(i%32))&1)==0)
      llr[i] = -llr[i];
    //    printf("%d\n",llr[i]);

  }
}


/*uint8_t get_num_pdcch_symbols(uint8_t num_dci,
                              DCI_ALLOC_t *dci_alloc,
                              NR_DL_FRAME_PARMS *frame_parms,
                              uint8_t nr_tti_rx)
{

  uint16_t numCCE = 0;
  uint8_t i;
  uint8_t nCCEmin = 0;
  uint16_t CCE_max_used_index = 0;
  uint16_t firstCCE_max = dci_alloc[0].firstCCE;
  uint8_t  L = dci_alloc[0].L;

  // check pdcch duration imposed by PHICH duration (Section 6.9 of 36-211)
  if (frame_parms->Ncp==1) { // extended prefix
    if ((frame_parms->frame_type == TDD) &&
        ((frame_parms->tdd_config<3)||(frame_parms->tdd_config==6)) &&
        ((nr_tti_rx==1) || (nr_tti_rx==6))) // subframes 1 and 6 (S-subframes) for 5ms switching periodicity are 2 symbols
      nCCEmin = 2;
    else {   // 10ms switching periodicity is always 3 symbols, any DL-only subframe is 3 symbols
      nCCEmin = 3;
    }
  }

  // compute numCCE
  for (i=0; i<num_dci; i++) {
    //     printf("dci %d => %d\n",i,dci_alloc[i].L);
    numCCE += (1<<(dci_alloc[i].L));

    if(firstCCE_max < dci_alloc[i].firstCCE) {
      firstCCE_max = dci_alloc[i].firstCCE;
      L            = dci_alloc[i].L;
    }
  }
  CCE_max_used_index = firstCCE_max + (1<<L) - 1;

  //if ((9*numCCE) <= (frame_parms->N_RB_DL*2))
  if (CCE_max_used_index < get_nCCE(1, frame_parms, get_mi(frame_parms, nr_tti_rx)))
    return(cmax(1,nCCEmin));
  //else if ((9*numCCE) <= (frame_parms->N_RB_DL*((frame_parms->nb_antenna_ports_eNB==4) ? 4 : 5)))
  else if (CCE_max_used_index < get_nCCE(2, frame_parms, get_mi(frame_parms, nr_tti_rx)))
    return(cmax(2,nCCEmin));
  //else if ((9*numCCE) <= (frame_parms->N_RB_DL*((frame_parms->nb_antenna_ports_eNB==4) ? 7 : 8)))
  else if (CCE_max_used_index < get_nCCE(3, frame_parms, get_mi(frame_parms, nr_tti_rx)))
    return(cmax(3,nCCEmin));
  else if (frame_parms->N_RB_DL<=10) {
    if (frame_parms->Ncp == 0) { // normal CP
      printf("numCCE %d, N_RB_DL = %d : should be returning 4 PDCCH symbols (%d,%d,%d)\n",numCCE,frame_parms->N_RB_DL,
             get_nCCE(1, frame_parms, get_mi(frame_parms, nr_tti_rx)),
             get_nCCE(2, frame_parms, get_mi(frame_parms, nr_tti_rx)),
             get_nCCE(3, frame_parms, get_mi(frame_parms, nr_tti_rx)));

      if ((9*numCCE) <= (frame_parms->N_RB_DL*((frame_parms->nb_antenna_ports_eNB==4) ? 10 : 11)))
        return(4);
    } else { // extended CP
      if ((9*numCCE) <= (frame_parms->N_RB_DL*((frame_parms->nb_antenna_ports_eNB==4) ? 9 : 10)))
        return(4);
    }
  }


  LOG_D(PHY," dci.c: get_num_pdcch_symbols nr_tti_rx %d FATAL, illegal numCCE %d (num_dci %d)\n",nr_tti_rx,numCCE,num_dci);
  //for (i=0;i<num_dci;i++) {
  //  printf("dci_alloc[%d].L = %d\n",i,dci_alloc[i].L);
  //}
  //exit(-1);
  return(0);
}

uint8_t generate_dci_top(int num_dci,
                         DCI_ALLOC_t *dci_alloc,
                         uint32_t n_rnti,
                         int16_t amp,
                         NR_DL_FRAME_PARMS *frame_parms,
                         int32_t **txdataF,
                         uint32_t nr_tti_rx)
{

  uint8_t *e_ptr,num_pdcch_symbols;
  uint32_t i, lprime;
  uint32_t gain_lin_QPSK,kprime,kprime_mod12,mprime,nsymb,symbol_offset,tti_offset;
  int16_t re_offset;
  uint8_t mi = get_mi(frame_parms,nr_tti_rx);
  static uint8_t e[DCI_BITS_MAX];
  static int32_t yseq0[Msymb],yseq1[Msymb],wbar0[Msymb],wbar1[Msymb];

  int32_t *y[2];
  int32_t *wbar[2];

  int nushiftmod3 = frame_parms->nushift%3;

  int Msymb2;
  int split_flag=0;

  switch (frame_parms->N_RB_DL) {
  case 100:
    Msymb2 = Msymb;
    break;

  case 75:
    Msymb2 = 3*Msymb/4;
    break;

  case 50:
    Msymb2 = Msymb>>1;
    break;

  case 25:
    Msymb2 = Msymb>>2;
    break;

  case 15:
    Msymb2 = Msymb*15/100;
    break;

  case 6:
    Msymb2 = Msymb*6/100;
    break;

  default:
    Msymb2 = Msymb>>2;
    break;
  }

  num_pdcch_symbols = get_num_pdcch_symbols(num_dci,dci_alloc,frame_parms,nr_tti_rx);
  //  printf("nr_tti_rx %d in generate_dci_top num_pdcch_symbols = %d, num_dci %d\n",
  //     nr_tti_rx,num_pdcch_symbols,num_dci);
  generate_pcfich(num_pdcch_symbols,
                  amp,
                  frame_parms,
                  txdataF,
                  nr_tti_rx);
  wbar[0] = &wbar0[0];
  wbar[1] = &wbar1[0];
  y[0] = &yseq0[0];
  y[1] = &yseq1[0];

  // reset all bits to <NIL>, here we set <NIL> elements as 2
  // memset(e, 2, DCI_BITS_MAX);
  // here we interpret NIL as a random QPSK sequence. That makes power estimation easier.
  for (i=0; i<DCI_BITS_MAX; i++)
    e[i]=taus()&1;

  e_ptr = e;

  // generate DCIs
  for (i=0; i<num_dci; i++) {
#ifdef DEBUG_DCI_ENCODING
    printf("Generating %s DCI %d/%d (nCCE %d) of length %d, aggregation %d (%x)\n",
           dci_alloc[i].search_space == DCI_COMMON_SPACE ? "common" : "UE",
           i,num_dci,dci_alloc[i].firstCCE,dci_alloc[i].dci_length,1<<dci_alloc[i].L,
          *(unsigned int*)dci_alloc[i].dci_pdu);
    dump_dci(frame_parms,&dci_alloc[i]);
#endif

    if (dci_alloc[i].firstCCE>=0) {
      e_ptr = generate_dci0(dci_alloc[i].dci_pdu,
                            e+(72*dci_alloc[i].firstCCE),
                            dci_alloc[i].dci_length,
                            dci_alloc[i].L,
                            dci_alloc[i].rnti);
    }
  }

  // Scrambling
  //  printf("pdcch scrambling\n");
  pdcch_scrambling(frame_parms,
                   nr_tti_rx,
                   e,
                   8*get_nquad(num_pdcch_symbols, frame_parms, mi));
  //72*get_nCCE(num_pdcch_symbols,frame_parms,mi));




  // Now do modulation
  if (frame_parms->mode1_flag==1)
    gain_lin_QPSK = (int16_t)((amp*ONE_OVER_SQRT2_Q15)>>15);
  else
    gain_lin_QPSK = amp/2;

  e_ptr = e;

#ifdef DEBUG_DCI_ENCODING
  printf(" PDCCH Modulation, Msymb %d, Msymb2 %d,gain_lin_QPSK %d\n",Msymb,Msymb2,gain_lin_QPSK);
#endif


  if (frame_parms->mode1_flag) { //SISO


    for (i=0; i<Msymb2; i++) {
      
      //((int16_t*)(&(y[0][i])))[0] = (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
      //((int16_t*)(&(y[1][i])))[0] = (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
      ((int16_t*)(&(y[0][i])))[0] = (*e_ptr == 2) ? 0 : (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
      ((int16_t*)(&(y[1][i])))[0] = (*e_ptr == 2) ? 0 : (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
      e_ptr++;
      //((int16_t*)(&(y[0][i])))[1] = (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
      //((int16_t*)(&(y[1][i])))[1] = (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
      ((int16_t*)(&(y[0][i])))[1] = (*e_ptr == 2) ? 0 : (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
      ((int16_t*)(&(y[1][i])))[1] = (*e_ptr == 2) ? 0 : (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;

      e_ptr++;
    }
  } else { //ALAMOUTI


    for (i=0; i<Msymb2; i+=2) {

#ifdef DEBUG_DCI_ENCODING
      printf(" PDCCH Modulation (TX diversity): REG %d\n",i>>2);
#endif
      // first antenna position n -> x0
      ((int16_t*)&y[0][i])[0] = (*e_ptr==2) ? 0 : (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
      e_ptr++;
      ((int16_t*)&y[0][i])[1] = (*e_ptr==2) ? 0 : (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
      e_ptr++;

      // second antenna position n -> -x1*
      ((int16_t*)&y[1][i])[0] = (*e_ptr==2) ? 0 : (*e_ptr == 1) ? gain_lin_QPSK : -gain_lin_QPSK;
      e_ptr++;
      ((int16_t*)&y[1][i])[1] = (*e_ptr==2) ? 0 : (*e_ptr == 1) ? -gain_lin_QPSK : gain_lin_QPSK;
      e_ptr++;

      // fill in the rest of the ALAMOUTI precoding
      ((int16_t*)&y[0][i+1])[0] = -((int16_t*)&y[1][i])[0];
      ((int16_t*)&y[0][i+1])[1] = ((int16_t*)&y[1][i])[1];
      ((int16_t*)&y[1][i+1])[0] = ((int16_t*)&y[0][i])[0];
      ((int16_t*)&y[1][i+1])[1] = -((int16_t*)&y[0][i])[1];

    }
  }


#ifdef DEBUG_DCI_ENCODING
  printf(" PDCCH Interleaving\n");
#endif

  //  printf("y %p (%p,%p), wbar %p (%p,%p)\n",y,y[0],y[1],wbar,wbar[0],wbar[1]);
  // This is the interleaving procedure defined in 36-211, first part of Section 6.8.5
  pdcch_interleaving(frame_parms,&y[0],&wbar[0],num_pdcch_symbols,mi);

  mprime=0;
  nsymb = (frame_parms->Ncp==0) ? 14:12;
  re_offset = frame_parms->first_carrier_offset;

  // This is the REG allocation algorithm from 36-211, second part of Section 6.8.5
  //  printf("DCI (SF %d) : txdataF %p (0 %p)\n",subframe,&txdataF[0][512*14*subframe],&txdataF[0][0]);
  for (kprime=0; kprime<frame_parms->N_RB_DL*12; kprime++) {
    for (lprime=0; lprime<num_pdcch_symbols; lprime++) {

      symbol_offset = (uint32_t)frame_parms->ofdm_symbol_size*(lprime+(nr_tti_rx*nsymb));



      tti_offset = symbol_offset + re_offset;

      (re_offset==(frame_parms->ofdm_symbol_size-2)) ? (split_flag=1) : (split_flag=0);

      //            printf("kprime %d, lprime %d => REG %d (symbol %d)\n",kprime,lprime,(lprime==0)?(kprime/6) : (kprime>>2),symbol_offset);
      // if REG is allocated to PHICH, skip it
      if (check_phich_reg(frame_parms,kprime,lprime,mi) == 1) {
#ifdef DEBUG_DCI_ENCODING
        printf("generate_dci: skipping REG %d (kprime %d, lprime %d)\n",(lprime==0)?(kprime/6) : (kprime>>2),kprime,lprime);
#endif
      } else {
        // Copy REG to TX buffer

        if ((lprime == 0)||
            ((lprime==1)&&(frame_parms->nb_antenna_ports_eNB == 4))) {
          // first symbol, or second symbol+4 TX antennas skip pilots

          kprime_mod12 = kprime%12;

          if ((kprime_mod12 == 0) || (kprime_mod12 == 6)) {
            // kprime represents REG

            for (i=0; i<6; i++) {
              if ((i!=(nushiftmod3))&&(i!=(nushiftmod3+3))) {
                txdataF[0][tti_offset+i] = wbar[0][mprime];

                if (frame_parms->nb_antenna_ports_eNB > 1)
                  txdataF[1][tti_offset+i] = wbar[1][mprime];

#ifdef DEBUG_DCI_ENCODING
                printf(" PDCCH mapping mprime %d => %d (symbol %d re %d) -> (%d,%d)\n",mprime,tti_offset,symbol_offset,re_offset+i,*(short*)&wbar[0][mprime],*(1+(short*)&wbar[0][mprime]));
#endif

                mprime++;
              }
            }
          }
        } else { // no pilots in this symbol
          kprime_mod12 = kprime%12;

          if ((kprime_mod12 == 0) || (kprime_mod12 == 4) || (kprime_mod12 == 8)) {
            // kprime represents REG
            if (split_flag==0) {
              for (i=0; i<4; i++) {
                txdataF[0][tti_offset+i] = wbar[0][mprime];

                if (frame_parms->nb_antenna_ports_eNB > 1)
                  txdataF[1][tti_offset+i] = wbar[1][mprime];

#ifdef DEBUG_DCI_ENCODING
                LOG_I(PHY," PDCCH mapping mprime %d => %d (symbol %d re %d) -> (%d,%d)\n",mprime,tti_offset,symbol_offset,re_offset+i,*(short*)&wbar[0][mprime],*(1+(short*)&wbar[0][mprime]));
#endif
                mprime++;
              }
            } else {
              txdataF[0][tti_offset+0] = wbar[0][mprime];

              if (frame_parms->nb_antenna_ports_eNB > 1)
                txdataF[1][tti_offset+0] = wbar[1][mprime];

#ifdef DEBUG_DCI_ENCODING
              printf(" PDCCH mapping mprime %d => %d (symbol %d re %d) -> (%d,%d)\n",mprime,tti_offset,symbol_offset,re_offset,*(short*)&wbar[0][mprime],*(1+(short*)&wbar[0][mprime]));
#endif
              mprime++;
              txdataF[0][tti_offset+1] = wbar[0][mprime];

              if (frame_parms->nb_antenna_ports_eNB > 1)
                txdataF[1][tti_offset+1] = wbar[1][mprime];

#ifdef DEBUG_DCI_ENCODING
              printf("PDCCH mapping mprime %d => %d (symbol %d re %d) -> (%d,%d)\n",mprime,tti_offset,symbol_offset,re_offset+1,*(short*)&wbar[0][mprime],*(1+(short*)&wbar[0][mprime]));
#endif
              mprime++;
              txdataF[0][tti_offset-frame_parms->ofdm_symbol_size+3] = wbar[0][mprime];

              if (frame_parms->nb_antenna_ports_eNB > 1)
                txdataF[1][tti_offset-frame_parms->ofdm_symbol_size+3] = wbar[1][mprime];

#ifdef DEBUG_DCI_ENCODING
              printf(" PDCCH mapping mprime %d => %d (symbol %d re %d) -> (%d,%d)\n",mprime,tti_offset,symbol_offset,re_offset-frame_parms->ofdm_symbol_size+3,*(short*)&wbar[0][mprime],
                    *(1+(short*)&wbar[0][mprime]));
#endif
              mprime++;
              txdataF[0][tti_offset-frame_parms->ofdm_symbol_size+4] = wbar[0][mprime];

              if (frame_parms->nb_antenna_ports_eNB > 1)
                txdataF[1][tti_offset-frame_parms->ofdm_symbol_size+4] = wbar[1][mprime];

#ifdef DEBUG_DCI_ENCODING
              printf(" PDCCH mapping mprime %d => %d (symbol %d re %d) -> (%d,%d)\n",mprime,tti_offset,symbol_offset,re_offset-frame_parms->ofdm_symbol_size+4,*(short*)&wbar[0][mprime],
                    *(1+(short*)&wbar[0][mprime]));
#endif
              mprime++;

            }
          }
        }

        if (mprime>=Msymb2)
          return(num_pdcch_symbols);
      } // check_phich_reg

    } //lprime loop

    re_offset++;

    if (re_offset == (frame_parms->ofdm_symbol_size))
      re_offset = 1;
  } // kprime loop

  return(num_pdcch_symbols);
}
*/
#ifdef PHY_ABSTRACTION
uint8_t generate_dci_top_emul(PHY_VARS_eNB *phy_vars_eNB,
                              int num_dci,
                              DCI_ALLOC_t *dci_alloc,
                              uint8_t subframe)
{
  int n_dci, n_dci_dl;
  uint8_t ue_id;
  LTE_eNB_DLSCH_t *dlsch_eNB;
  int num_ue_spec_dci;
  int num_common_dci;
  int i;
  uint8_t num_pdcch_symbols = get_num_pdcch_symbols(num_dci,
                              dci_alloc,
                              &phy_vars_eNB->frame_parms,
                              subframe);
  eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].cntl.cfi=num_pdcch_symbols;

  num_ue_spec_dci = 0;
  num_common_dci = 0;
  for (i = 0; i < num_dci; i++) {
    /* TODO: maybe useless test, to remove? */
    if (!(dci_alloc[i].firstCCE>=0)) abort();
    if (dci_alloc[i].search_space == DCI_COMMON_SPACE)
      num_common_dci++;
    else
      num_ue_spec_dci++;
  }

  memcpy(phy_vars_eNB->dci_alloc[subframe&1],dci_alloc,sizeof(DCI_ALLOC_t)*(num_dci));
  phy_vars_eNB->num_ue_spec_dci[subframe&1]=num_ue_spec_dci;
  phy_vars_eNB->num_common_dci[subframe&1]=num_common_dci;
  eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].num_ue_spec_dci = num_ue_spec_dci;
  eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].num_common_dci = num_common_dci;

  LOG_D(PHY,"[eNB %d][DCI][EMUL] CC id %d:  num spec dci %d num comm dci %d num PMCH %d \n",
        phy_vars_eNB->Mod_id, phy_vars_eNB->CC_id, num_ue_spec_dci,num_common_dci,
        eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].num_pmch);

  if (eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].cntl.pmch_flag == 1 )
    n_dci_dl = eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].num_pmch;
  else
    n_dci_dl = 0;

  for (n_dci =0 ;
       n_dci < (eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].num_ue_spec_dci+ eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].num_common_dci);
       n_dci++) {

    if (dci_alloc[n_dci].format > 0) { // exclude the uplink dci

      if (dci_alloc[n_dci].rnti == SI_RNTI) {
        dlsch_eNB = PHY_vars_eNB_g[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id]->dlsch_SI;
        eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].dlsch_type[n_dci_dl] = 0;//SI;
        eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].harq_pid[n_dci_dl] = 0;
        eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].tbs[n_dci_dl] = dlsch_eNB->harq_processes[0]->TBS>>3;
        LOG_D(PHY,"[DCI][EMUL]SI tbs is %d and dci index %d harq pid is %d \n",eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].tbs[n_dci_dl],n_dci_dl,
              eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].harq_pid[n_dci_dl]);
      } else if (dci_alloc[n_dci_dl].ra_flag == 1) {
        dlsch_eNB = PHY_vars_eNB_g[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id]->dlsch_ra;
        eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].dlsch_type[n_dci_dl] = 1;//RA;
        eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].harq_pid[n_dci_dl] = 0;
        eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].tbs[n_dci_dl] = dlsch_eNB->harq_processes[0]->TBS>>3;
        LOG_D(PHY,"[DCI][EMUL] RA  tbs is %d and dci index %d harq pid is %d \n",eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].tbs[n_dci_dl],n_dci_dl,
              eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].harq_pid[n_dci_dl]);
      } else {
        ue_id = find_ue(dci_alloc[n_dci_dl].rnti,PHY_vars_eNB_g[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id]);
        DevAssert( ue_id != (uint8_t)-1 );
        dlsch_eNB = PHY_vars_eNB_g[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id]->dlsch[ue_id][0];

        eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].dlsch_type[n_dci_dl] = 2;//TB0;
        eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].harq_pid[n_dci_dl] = dlsch_eNB->current_harq_pid;
        eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].ue_id[n_dci_dl] = ue_id;
        eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].tbs[n_dci_dl] = dlsch_eNB->harq_processes[dlsch_eNB->current_harq_pid]->TBS>>3;
        LOG_D(PHY,"[DCI][EMUL] TB1 tbs is %d and dci index %d harq pid is %d \n",eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].tbs[n_dci_dl],n_dci_dl,
              eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].harq_pid[n_dci_dl]);
        // check for TB1 later

      }
    }

    n_dci_dl++;
  }

  memcpy((void *)&eNB_transport_info[phy_vars_eNB->Mod_id][phy_vars_eNB->CC_id].dci_alloc,
         (void *)dci_alloc,
         n_dci*sizeof(DCI_ALLOC_t));

  return(num_pdcch_symbols);
}
#endif


void dci_decoding(uint8_t DCI_LENGTH,
                  uint8_t aggregation_level,
                  int8_t *e,
                  uint8_t *decoded_output)
{

  uint8_t dummy_w_rx[3*(MAX_DCI_SIZE_BITS+16+64)];
  int8_t w_rx[3*(MAX_DCI_SIZE_BITS+16+32)],d_rx[96+(3*(MAX_DCI_SIZE_BITS+16))];

  uint16_t RCC;

  uint16_t D=(DCI_LENGTH+16+64);
  uint16_t coded_bits;
#ifdef DEBUG_DCI_DECODING
  int32_t i;
#endif

  if (aggregation_level>3) {
    LOG_I(PHY," dci.c: dci_decoding FATAL, illegal aggregation_level %d\n",aggregation_level);
    return;
  }

  coded_bits = 72 * (1<<aggregation_level);

#ifdef DEBUG_DCI_DECODING
  LOG_I(PHY," Doing DCI decoding for %d bits, DCI_LENGTH %d,coded_bits %d, e %p\n",3*(DCI_LENGTH+16),DCI_LENGTH,coded_bits,e);
#endif

  // now do decoding
  memset((void*)dummy_w_rx,0,3*D);
  RCC = generate_dummy_w_cc(DCI_LENGTH+16,
                            dummy_w_rx);



#ifdef DEBUG_DCI_DECODING
  LOG_I(PHY," Doing DCI Rate Matching RCC %d, w %p\n",RCC,w);
#endif

  lte_rate_matching_cc_rx(RCC,coded_bits,w_rx,dummy_w_rx,e);

  sub_block_deinterleaving_cc((uint32_t)(DCI_LENGTH+16),
                              &d_rx[96],
                              &w_rx[0]);

#ifdef DEBUG_DCI_DECODING

  for (i=0; i<16+DCI_LENGTH; i++)
    LOG_I(PHY," DCI %d : (%d,%d,%d)\n",i,*(d_rx+96+(3*i)),*(d_rx+97+(3*i)),*(d_rx+98+(3*i)));

#endif
  memset(decoded_output,0,2+((16+DCI_LENGTH)>>3));

#ifdef DEBUG_DCI_DECODING
  printf("Before Viterbi\n");

  for (i=0; i<16+DCI_LENGTH; i++)
    printf("%d : (%d,%d,%d)\n",i,*(d_rx+96+(3*i)),*(d_rx+97+(3*i)),*(d_rx+98+(3*i)));

#endif
  //debug_printf("Doing DCI Viterbi \n");
  phy_viterbi_lte_sse2(d_rx+96,decoded_output,16+DCI_LENGTH);
  //debug_printf("Done DCI Viterbi \n");
}


static uint8_t dci_decoded_output[RX_NB_TH][(MAX_DCI_SIZE_BITS+64)/8];

/*uint16_t get_nCCE(uint8_t num_pdcch_symbols,NR_DL_FRAME_PARMS *frame_parms,uint8_t mi)
{
  return(get_nquad(num_pdcch_symbols,frame_parms,mi)/9);
}

uint16_t get_nquad(uint8_t num_pdcch_symbols,NR_DL_FRAME_PARMS *frame_parms,uint8_t mi)
{

  uint16_t Nreg=0;
  uint8_t Ngroup_PHICH = (frame_parms->phich_config_common.phich_resource*frame_parms->N_RB_DL)/48;

  if (((frame_parms->phich_config_common.phich_resource*frame_parms->N_RB_DL)%48) > 0)
    Ngroup_PHICH++;

  if (frame_parms->Ncp == 1) {
    Ngroup_PHICH<<=1;
  }

  Ngroup_PHICH*=mi;

  if ((num_pdcch_symbols>0) && (num_pdcch_symbols<4))
    switch (frame_parms->N_RB_DL) {
    case 6:
      Nreg=12+(num_pdcch_symbols-1)*18;
      break;

    case 25:
      Nreg=50+(num_pdcch_symbols-1)*75;
      break;

    case 50:
      Nreg=100+(num_pdcch_symbols-1)*150;
      break;

    case 100:
      Nreg=200+(num_pdcch_symbols-1)*300;
      break;

    default:
      return(0);
    }

  //   printf("Nreg %d (%d)\n",Nreg,Nreg - 4 - (3*Ngroup_PHICH));
  return(Nreg - 4 - (3*Ngroup_PHICH));
}

uint16_t get_nCCE_mac(uint8_t Mod_id,uint8_t CC_id,int num_pdcch_symbols,int nr_tti_rx)
{

  // check for eNB only !
  return(get_nCCE(num_pdcch_symbols,
		  &PHY_vars_eNB_g[Mod_id][CC_id]->frame_parms,
		  get_mi(&PHY_vars_eNB_g[Mod_id][CC_id]->frame_parms,nr_tti_rx)));
}
*/

int get_nCCE_offset_l1(int *CCE_table,
		       const unsigned char L, 
		       const int nCCE, 
		       const int common_dci, 
		       const unsigned short rnti, 
		       const unsigned char nr_tti_rx)
{

  int search_space_free,m,nb_candidates = 0,l,i;
  unsigned int Yk;
   /*
    printf("CCE Allocation: ");
    for (i=0;i<nCCE;i++)
    printf("%d.",CCE_table[i]);
    printf("\n");
  */
  if (common_dci == 1) {
    // check CCE(0 ... L-1)
    nb_candidates = (L==4) ? 4 : 2;
    nb_candidates = min(nb_candidates,nCCE/L);

    //    printf("Common DCI nb_candidates %d, L %d\n",nb_candidates,L);

    for (m = nb_candidates-1 ; m >=0 ; m--) {

      search_space_free = 1;
      for (l=0; l<L; l++) {

	//	printf("CCE_table[%d] %d\n",(m*L)+l,CCE_table[(m*L)+l]);
        if (CCE_table[(m*L) + l] == 1) {
          search_space_free = 0;
          break;
        }
      }
     
      if (search_space_free == 1) {

	//	printf("returning %d\n",m*L);

        for (l=0; l<L; l++)
          CCE_table[(m*L)+l]=1;
        return(m*L);
      }
    }

    return(-1);

  } else { // Find first available in ue specific search space
    // according to procedure in Section 9.1.1 of 36.213 (v. 8.6)
    // compute Yk
    Yk = (unsigned int)rnti;

    for (i=0; i<=nr_tti_rx; i++)
      Yk = (Yk*39827)%65537;

    Yk = Yk % (nCCE/L);


    switch (L) {
    case 1:
    case 2:
      nb_candidates = 6;
      break;

    case 4:
    case 8:
      nb_candidates = 2;
      break;

    default:
      DevParam(L, nCCE, rnti);
      break;
    }


    LOG_D(MAC,"rnti %x, Yk = %d, nCCE %d (nCCE/L %d),nb_cand %d\n",rnti,Yk,nCCE,nCCE/L,nb_candidates);

    for (m = 0 ; m < nb_candidates ; m++) {
      search_space_free = 1;

      for (l=0; l<L; l++) {
        int cce = (((Yk+m)%(nCCE/L))*L) + l;
        if (cce >= nCCE || CCE_table[cce] == 1) {
          search_space_free = 0;
          break;
        }
      }

      if (search_space_free == 1) {
        for (l=0; l<L; l++)
          CCE_table[(((Yk+m)%(nCCE/L))*L)+l]=1;

        return(((Yk+m)%(nCCE/L))*L);
      }
    }

    return(-1);
  }
}




#ifdef NR_PDCCH_DCI_RUN
void nr_dci_decoding_procedure0(int s,                                                                        //x
                                int p,                                                                        //x
                                NR_UE_PDCCH **pdcch_vars,                                                    //x
                                int do_common,                                                                //x
                                //dci_detect_mode_t mode,                                                       //not sure if necessary
                                uint8_t nr_tti_rx,                                                            //x
                                NR_DCI_ALLOC_t *dci_alloc,                                                       //x
                                int16_t eNB_id,                                                               //x
                                uint8_t current_thread_id,                                                    //x
                                NR_DL_FRAME_PARMS *frame_parms,                                              //x
                                uint8_t mi,
                                uint16_t crc_scrambled_values[13],                                            //x
                                uint8_t L,
                                NR_UE_SEARCHSPACE_CSS_DCI_FORMAT_t format_css,
                                NR_UE_SEARCHSPACE_USS_DCI_FORMAT_t format_uss,
                                uint8_t sizeof_bits,
                                uint8_t sizeof_bytes,
                                uint8_t *dci_cnt,
                                crc_scrambled_t *crc_scrambled,
                                format_found_t *format_found,
                                uint32_t *CCEmap0,
                                uint32_t *CCEmap1,
                                uint32_t *CCEmap2) {

  uint16_t crc, CCEind, nCCE[3];
  uint32_t *CCEmap = NULL, CCEmap_mask = 0;
  int L2 = (1 << L);
  unsigned int Yk, nb_candidates = 0, i, m;
  unsigned int CCEmap_cand;

  // A[p], p is the current active CORESET
  uint16_t A[3]={39827,39829,39839};
  //Table 10.1-2: Maximum number of PDCCH candidates    per slot and per serving cell as a function of the subcarrier spacing value 2^mu*15 KHz, mu {0,1,2,3}
  uint8_t m_max_slot_pdcch_Table10_1_2 [4] = {44,36,22,20};
  //Table 10.1-3: Maximum number of non-overlapped CCEs per slot and per serving cell as a function of the subcarrier spacing value 2^mu*15 KHz, mu {0,1,2,3}
  uint8_t cce_max_slot_pdcch_Table10_1_3 [4] = {56,56,48,32};

  int coreset_nbr_cce_per_symbol=0;

  #ifdef NR_PDCCH_DCI_DEBUG
    printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure0)-> format_found is %d \n", *format_found);
  #endif

  //if (mode == NO_DCI) {
  //  #ifdef NR_PDCCH_DCI_DEBUG
  //    printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure0)-> skip DCI decoding: expect no DCIs at nr_tti_rx %d in current searchSpace\n", nr_tti_rx);
  //  #endif
  //  return;
  //}

  #ifdef NR_PDCCH_DCI_DEBUG
    printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure0)-> frequencyDomainResources=%llx, duration=%d\n",
            pdcch_vars[eNB_id]->coreset[p].frequencyDomainResources, pdcch_vars[eNB_id]->coreset[p].duration);
  #endif

  // nCCE = get_nCCE(pdcch_vars[eNB_id]->num_pdcch_symbols, frame_parms, mi);
  for (int i = 0; i < 45; i++) {
    // this loop counts each bit of the bit map coreset_freq_dom, and increments nbr_RB_coreset for each bit set to '1'
    if (((pdcch_vars[eNB_id]->coreset[p].frequencyDomainResources & 0x1FFFFFFFFFFF) >> i) & 0x1) coreset_nbr_cce_per_symbol++;
  }
  nCCE[p] = pdcch_vars[eNB_id]->coreset[p].duration*coreset_nbr_cce_per_symbol; // 1 CCE = 6 RB
  // p is the current CORESET we are currently monitoring (among the 3 possible CORESETs in a BWP)
  // the number of CCE in the current CORESET is:
  //   the number of symbols in the CORESET (pdcch_vars[eNB_id]->coreset[p].duration)
  //   multiplied by the number of bits set to '1' in the frequencyDomainResources bitmap
  //   (1 bit set to '1' corresponds to 6 RB and 1 CCE = 6 RB)
  #ifdef NR_PDCCH_DCI_DEBUG
    printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure0)-> nCCE[%d]=%d\n",p,nCCE[p]);
  #endif

/*	if (nCCE > get_nCCE(3, frame_parms, 1)) {
		LOG_D(PHY,
				"skip DCI decoding: nCCE=%d > get_nCCE(3,frame_parms,1)=%d\n",
				nCCE, get_nCCE(3, frame_parms, 1));
		return;
	}

	if (nCCE < L2) {
		LOG_D(PHY, "skip DCI decoding: nCCE=%d < L2=%d\n", nCCE, L2);
		return;
	}

	if (mode == NO_DCI) {
		LOG_D(PHY, "skip DCI decoding: expect no DCIs at nr_tti_rx %d\n",
				nr_tti_rx);
		return;
	}
*/
  if (do_common == 1) {
    Yk = 0;
    if (pdcch_vars[eNB_id]->searchSpace[s].searchSpaceType.common_dci_formats == cformat2_0) {
        // for dci_format_2_0, the nb_candidates is obtained from a different variable
        switch (L2) {
        case 1:
          nb_candidates = pdcch_vars[eNB_id]->searchSpace[s].searchSpaceType.sfi_nrofCandidates_aggrlevel1;
          break;
        case 2:
          nb_candidates = pdcch_vars[eNB_id]->searchSpace[s].searchSpaceType.sfi_nrofCandidates_aggrlevel2;
          break;
        case 4:
          nb_candidates = pdcch_vars[eNB_id]->searchSpace[s].searchSpaceType.sfi_nrofCandidates_aggrlevel4;
          break;
        case 8:
          nb_candidates = pdcch_vars[eNB_id]->searchSpace[s].searchSpaceType.sfi_nrofCandidates_aggrlevel8;
          break;
        case 16:
          nb_candidates = pdcch_vars[eNB_id]->searchSpace[s].searchSpaceType.sfi_nrofCandidates_aggrlevel16;
          break;
        default:
          break;
        }
    } else if (pdcch_vars[eNB_id]->searchSpace[s].searchSpaceType.common_dci_formats == cformat2_3) {
        // for dci_format_2_3, the nb_candidates is obtained from a different variable
        nb_candidates = pdcch_vars[eNB_id]->searchSpace[s].searchSpaceType.srs_nrofCandidates;
    } else {
      nb_candidates = (L2 == 4) ? 4 : ((L2 == 8)? 2 : 1); // according to Table 10.1-1 (38.213 section 10.1)
    }
  } else {
    switch (L2) {
    case 1:
      nb_candidates = pdcch_vars[eNB_id]->searchSpace[s].nrofCandidates_aggrlevel1;
      break;
    case 2:
      nb_candidates = pdcch_vars[eNB_id]->searchSpace[s].nrofCandidates_aggrlevel2;
      break;
    case 4:
      nb_candidates = pdcch_vars[eNB_id]->searchSpace[s].nrofCandidates_aggrlevel4;
      break;
    case 8:
      nb_candidates = pdcch_vars[eNB_id]->searchSpace[s].nrofCandidates_aggrlevel8;
      break;
    case 16:
      nb_candidates = pdcch_vars[eNB_id]->searchSpace[s].nrofCandidates_aggrlevel16;
      break;
    default:
      break;
    }
    // Find first available in ue specific search space
    // according to procedure in Section 10.1 of 38.213
    // compute Yk
    Yk = (unsigned int) pdcch_vars[eNB_id]->crnti;
    for (i = 0; i <= nr_tti_rx; i++)
      Yk = (Yk * A[p%3]) % 65537;
  }
  #ifdef NR_PDCCH_DCI_DEBUG
    printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure0)-> L2(%d) | nCCE[%d](%d) | Yk(%d) | nb_candidates(%d)\n",L2,p,nCCE[p],Yk,nb_candidates);
  #endif
  /*  for (CCEind=0;
	 CCEind<nCCE2;
	 CCEind+=(1<<L)) {*/
//	if (nb_candidates * L2 > nCCE[p])
//		nb_candidates = nCCE[p] / L2;

// In the next code line there is maybe a bug. The spec is not comparing Table 10.1-2 with nb_candidates, but with total number of candidates for all s and all p
  int m_p_s_L_max = (m_max_slot_pdcch_Table10_1_2[1]<=nb_candidates ? m_max_slot_pdcch_Table10_1_2[1] : nb_candidates);
  if (L==4) m_p_s_L_max=1; // Table 10.1-2 is not defined for L=4
  #ifdef NR_PDCCH_DCI_DEBUG
    printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure0)-> m_max_slot_pdcch_Table10_1_2(%d)=%d\n",L,m_max_slot_pdcch_Table10_1_2[L]);
  #endif
  for (m = 0; m < nb_candidates; m++) {
    int n_ci = 0;
    if (nCCE[p] < L2) return;
    int debug1 = nCCE[p] / L2;
    int debug2 = L2*m_p_s_L_max;
    #ifdef NR_PDCCH_DCI_DEBUG
      printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure0)-> debug1(%d)=nCCE[p]/L2 | nCCE[%d](%d) | L2(%d)\n",debug1,p,nCCE[p],L2);
      printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure0)-> debug2(%d)=L2*m_p_s_L_max | L2(%d) | m_p_s_L_max(%d)\n",debug2,L2,m_p_s_L_max);
    #endif
    CCEind = (((Yk + (uint16_t)(floor((m*nCCE[p])/(L2*m_p_s_L_max))) + n_ci) % (uint16_t)(floor(nCCE[p] / L2))) * L2);
    #ifdef NR_PDCCH_DCI_DEBUG
      printf ("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure0)-> CCEind(%d) = (((Yk(%d) + ((m(%d)*nCCE[p](%d))/(L2(%d)*m_p_s_L_max(%d)))) % (nCCE[p] / L2)) * L2)\n",
               CCEind,Yk,m,nCCE[p],L2,m_p_s_L_max);
      printf ("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure0)-> n_candidate(m)=%d | CCEind=%d |",m,CCEind);
    #endif
    if (CCEind < 32)
      CCEmap = CCEmap0;
    else if (CCEind < 64)
      CCEmap = CCEmap1;
    else if (CCEind < 96)
      CCEmap = CCEmap2;
    else {
      LOG_E(PHY, "Illegal CCEind %d (Yk %d, m %d, nCCE %d, L2 %d\n",CCEind, Yk, m, nCCE, L2);
      //mac_xface->macphy_exit("Illegal CCEind\n");
      return; // not reached
    }

    switch (L2) {
      case 1:
        CCEmap_mask = (1 << (CCEind & 0x1f));
        break;
      case 2:
        CCEmap_mask = (3 << (CCEind & 0x1f));
        break;
      case 4:
        CCEmap_mask = (0xf << (CCEind & 0x1f));
        break;
      case 8:
        CCEmap_mask = (0xff << (CCEind & 0x1f));
        break;
      case 16:
        CCEmap_mask = (0xfff << (CCEind & 0x1f));
        break;
      default:
        LOG_E(PHY, "Illegal L2 value %d\n", L2);
        //mac_xface->macphy_exit("Illegal L2\n");
        return; // not reached
    }
    CCEmap_cand = (*CCEmap) & CCEmap_mask;
    // CCE is not allocated yet
    #ifdef NR_PDCCH_DCI_DEBUG
      printf ("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure0)-> CCEmap_cand=%d \n",CCEmap_cand);
    #endif

    if (CCEmap_cand == 0) {
      #ifdef DEBUG_DCI_DECODING
        if (do_common == 1)
          LOG_I(PHY,"[DCI search nPdcch %d - common] Attempting candidate %d Aggregation Level %d DCI length %d at CCE %d/%d (CCEmap %x,CCEmap_cand %x)\n",
                    pdcch_vars[eNB_id]->num_pdcch_symbols,m,L2,sizeof_bits,CCEind,nCCE,*CCEmap,CCEmap_mask);
        else
          LOG_I(PHY,"[DCI search nPdcch %d - ue spec] Attempting candidate %d Aggregation Level %d DCI length %d at CCE %d/%d (CCEmap %x,CCEmap_cand %x) format %d\n",
                    pdcch_vars[eNB_id]->num_pdcch_symbols,m,L2,sizeof_bits,CCEind,nCCE,*CCEmap,CCEmap_mask,format_c);
      #endif
      #ifdef NR_PDCCH_DCI_DEBUG
        printf ("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure0)-> ... we enter function dci_decoding(sizeof_bits=%d L=%d) -----\n",sizeof_bits,L);
        printf ("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure0)-> ... we have to replace this part of the code by polar decoding\n");
      #endif
      dci_decoding(sizeof_bits, L, &pdcch_vars[eNB_id]->e_rx[CCEind * 72], &dci_decoded_output[current_thread_id][0]);
      /*
      for (i=0;i<3+(sizeof_bits>>3);i++)
      printf("dci_decoded_output[%d] => %x\n",i,dci_decoded_output[i]);
      */
      crc = (crc16(&dci_decoded_output[current_thread_id][0], sizeof_bits) >> 16) ^ extract_crc(&dci_decoded_output[current_thread_id][0], sizeof_bits);
      #ifdef NR_PDCCH_DCI_DEBUG
        printf ("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure0)-> ... we end function dci_decoding() with crc=%x\n",crc);
        printf ("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure0)-> ... we have to replace this part of the code by polar decoding\n");
      #endif
      #ifdef DEBUG_DCI_DECODING
        printf("crc =>%x\n",crc);
      #endif //uint16_t tc_rnti, uint16_t int_rnti, uint16_t sfi_rnti, uint16_t tpc_pusch_rnti, uint16_t tpc_pucch_rnti, uint16_t tpc_srs__rnti
     #ifdef NR_PDCCH_DCI_DEBUG
       printf ("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure0)-> format_found=%d\n",*format_found);
       printf ("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure0)-> crc_scrambled=%d\n",*crc_scrambled);
     #endif

      if (crc == crc_scrambled_values[_C_RNTI_])  {
        *crc_scrambled =_c_rnti;
        *format_found=1;
      }
      if (crc == crc_scrambled_values[_CS_RNTI_])  {
        *crc_scrambled =_cs_rnti;
        *format_found=1;
      }
      if (crc == crc_scrambled_values[_NEW_RNTI_])  {
        *crc_scrambled =_new_rnti;
        *format_found=1;
      }
      if (crc == crc_scrambled_values[_TC_RNTI_])  {
        *crc_scrambled =_tc_rnti;
        *format_found=_format_1_0_found;
      }
      if (crc == crc_scrambled_values[_P_RNTI_])  {
        *crc_scrambled =_p_rnti;
        *format_found=_format_1_0_found;
      }
      if (crc == crc_scrambled_values[_SI_RNTI_])  {
        *crc_scrambled =_si_rnti;
        *format_found=_format_1_0_found;
      }
      if (crc == crc_scrambled_values[_RA_RNTI_])  {
        *crc_scrambled =_ra_rnti;
        *format_found=_format_1_0_found;
      }
      if (crc == crc_scrambled_values[_SP_CSI_RNTI_])  {
        *crc_scrambled =_sp_csi_rnti;
        *format_found=_format_0_1_found;
      }
      if (crc == crc_scrambled_values[_SFI_RNTI_])  {
        *crc_scrambled =_sfi_rnti;
        *format_found=_format_2_0_found;
      }
      if (crc == crc_scrambled_values[_INT_RNTI_])  {
        *crc_scrambled =_int_rnti;
        *format_found=_format_2_1_found;
      }
      if (crc == crc_scrambled_values[_TPC_PUSCH_RNTI_]) {
        *crc_scrambled =_tpc_pusch_rnti;
        *format_found=_format_2_2_found;
      }
      if (crc == crc_scrambled_values[_TPC_PUCCH_RNTI_]) {
        *crc_scrambled =_tpc_pucch_rnti;
        *format_found=_format_2_2_found;
      }
      if (crc == crc_scrambled_values[_TPC_SRS_RNTI_]) {
        *crc_scrambled =_tpc_srs_rnti;
        *format_found=_format_2_3_found;
      }
#ifdef NR_PDCCH_DCI_DEBUG
  printf ("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure0)-> format_found=%d %d %d\n",*format_found, format_found, &format_found);
  printf ("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure0)-> crc_scrambled=%d\n",*crc_scrambled);
#endif
      if (*format_found!=255) {
        #ifdef NR_PDCCH_DCI_DEBUG
          printf ("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure0)-> rnti matches -> DCI FOUND !!! crc =>%x, sizeof_bits %d, sizeof_bytes %d \n",crc, sizeof_bits, sizeof_bytes);
        #endif
        dci_alloc[*dci_cnt].dci_length = sizeof_bits;
        dci_alloc[*dci_cnt].rnti = crc;
        dci_alloc[*dci_cnt].L = L;
        dci_alloc[*dci_cnt].firstCCE = CCEind;
        if (sizeof_bytes <= 4) {
          dci_alloc[*dci_cnt].dci_pdu[3] = dci_decoded_output[current_thread_id][0];
          dci_alloc[*dci_cnt].dci_pdu[2] = dci_decoded_output[current_thread_id][1];
          dci_alloc[*dci_cnt].dci_pdu[1] = dci_decoded_output[current_thread_id][2];
          dci_alloc[*dci_cnt].dci_pdu[0] = dci_decoded_output[current_thread_id][3];
#ifdef DEBUG_DCI_DECODING
					printf("DCI => %x,%x,%x,%x\n",dci_decoded_output[current_thread_id][0],
							dci_decoded_output[current_thread_id][1],
							dci_decoded_output[current_thread_id][2],
							dci_decoded_output[current_thread_id][3]);
#endif
        } else {
          dci_alloc[*dci_cnt].dci_pdu[7] = dci_decoded_output[current_thread_id][0];
          dci_alloc[*dci_cnt].dci_pdu[6] = dci_decoded_output[current_thread_id][1];
          dci_alloc[*dci_cnt].dci_pdu[5] = dci_decoded_output[current_thread_id][2];
          dci_alloc[*dci_cnt].dci_pdu[4] = dci_decoded_output[current_thread_id][3];
          dci_alloc[*dci_cnt].dci_pdu[3] = dci_decoded_output[current_thread_id][4];
          dci_alloc[*dci_cnt].dci_pdu[2] = dci_decoded_output[current_thread_id][5];
          dci_alloc[*dci_cnt].dci_pdu[1] = dci_decoded_output[current_thread_id][6];
          dci_alloc[*dci_cnt].dci_pdu[0] = dci_decoded_output[current_thread_id][7];
          // MAX_DCI_SIZE_BITS has to be redefined for dci_decoded_output FIXME
          // format2_0, format2_1 can be longer than 8 bytes. FIXME
#ifdef DEBUG_DCI_DECODING
					printf("DCI => %x,%x,%x,%x,%x,%x,%x,%x\n",
							dci_decoded_output[current_thread_id][0],dci_decoded_output[current_thread_id][1],dci_decoded_output[current_thread_id][2],dci_decoded_output[current_thread_id][3],
							dci_decoded_output[current_thread_id][4],dci_decoded_output[current_thread_id][5],dci_decoded_output[current_thread_id][6],dci_decoded_output[current_thread_id][7]);
#endif
        }
        if ((format_css == cformat0_0_and_1_0) || (format_uss == uformat0_0_and_1_0)){
          if ((crc_scrambled == _p_rnti) || (crc_scrambled == _si_rnti) || (crc_scrambled == _ra_rnti)){
            dci_alloc[*dci_cnt].format = format1_0;
            *dci_cnt = *dci_cnt + 1;
            format_found=_format_1_0_found;
          } else {
            if ((dci_decoded_output[current_thread_id][7]>>(sizeof_bits-1))&1 == 0){
              dci_alloc[*dci_cnt].format = format0_0;
              *dci_cnt = *dci_cnt + 1;
              format_found=_format_0_0_found;
            }
            if ((dci_decoded_output[current_thread_id][7]>>(sizeof_bits-1))&1 == 1){
              dci_alloc[*dci_cnt].format = format1_0;
              *dci_cnt = *dci_cnt + 1;
              format_found=_format_1_0_found;
            }
          }
        }
        if (format_css == cformat2_0){
          dci_alloc[*dci_cnt].format = format2_0;
          *dci_cnt = *dci_cnt + 1;
        }
        if (format_css == cformat2_1){
          dci_alloc[*dci_cnt].format = format2_1;
          *dci_cnt = *dci_cnt + 1;
        }
        if (format_css == cformat2_2){
          dci_alloc[*dci_cnt].format = format2_2;
          *dci_cnt = *dci_cnt + 1;
        }
        if (format_css == cformat2_3){
          dci_alloc[*dci_cnt].format = format2_3;
          *dci_cnt = *dci_cnt + 1;
        }
        if (format_uss == uformat0_1_and_1_1){
          // Not implemented yet FIXME
        }
        // store first nCCE of group for PUCCH transmission of ACK/NAK
        pdcch_vars[eNB_id]->nCCE[nr_tti_rx] = CCEind;
/*				if (crc == si_rnti) {
					dci_alloc[*dci_cnt].format = format_si;
					*dci_cnt = *dci_cnt + 1;
				} else if (crc == p_rnti) {
					dci_alloc[*dci_cnt].format = format_p;
					*dci_cnt = *dci_cnt + 1;
				} else if (crc == ra_rnti) {
					dci_alloc[*dci_cnt].format = format_ra;
					// store first nCCE of group for PUCCH transmission of ACK/NAK
					pdcch_vars[eNB_id]->nCCE[nr_tti_rx] = CCEind;
					*dci_cnt = *dci_cnt + 1;
				} else if (crc == pdcch_vars[eNB_id]->crnti) {

					if ((mode & UL_DCI) && (format_c == format0)
							&& ((dci_decoded_output[current_thread_id][0] & 0x80)
									== 0)) { // check if pdu is format 0 or 1A
						if (*format0_found == 0) {
							dci_alloc[*dci_cnt].format = format0;
							*format0_found = 1;
							*dci_cnt = *dci_cnt + 1;
							pdcch_vars[eNB_id]->nCCE[nr_tti_rx] = CCEind;
						}
					} else if (format_c == format0) { // this is a format 1A DCI
						dci_alloc[*dci_cnt].format = format1A;
						*dci_cnt = *dci_cnt + 1;
						pdcch_vars[eNB_id]->nCCE[nr_tti_rx] = CCEind;
					} else {
						// store first nCCE of group for PUCCH transmission of ACK/NAK
						if (*format_c_found == 0) {
							dci_alloc[*dci_cnt].format = format_c;
							*dci_cnt = *dci_cnt + 1;
							*format_c_found = 1;
							pdcch_vars[eNB_id]->nCCE[nr_tti_rx] = CCEind;
						}
					}
				}*/
				//LOG_I(PHY,"DCI decoding CRNTI  [format: %d, nCCE[nr_tti_rx: %d]: %d ], AggregationLevel %d \n",format_c, nr_tti_rx, pdcch_vars[eNB_id]->nCCE[nr_tti_rx],L2);
				//  memcpy(&dci_alloc[*dci_cnt].dci_pdu[0],dci_decoded_output,sizeof_bytes);
        switch (1 << L) {
          case 1:
            *CCEmap |= (1 << (CCEind & 0x1f));
            break;
          case 2:
            *CCEmap |= (1 << (CCEind & 0x1f));
            break;
          case 4:
            *CCEmap |= (1 << (CCEind & 0x1f));
            break;
          case 8:
            *CCEmap |= (1 << (CCEind & 0x1f));
            break;
          case 16:
            *CCEmap |= (1 << (CCEind & 0x1f));
            break;
        }

#ifdef DEBUG_DCI_DECODING
				LOG_I(PHY,"[DCI search] Found DCI %d rnti %x Aggregation %d length %d format %s in CCE %d (CCEmap %x) candidate %d / %d \n",
						*dci_cnt,crc,1<<L,sizeof_bits,dci_format_strings[dci_alloc[*dci_cnt-1].format],CCEind,*CCEmap,m,nb_candidates );
				dump_dci(frame_parms,&dci_alloc[*dci_cnt-1]);

#endif
        return;
      } // rnti match
    } else { // CCEmap_cand == 0
      printf("\n");
    }
/*
		 if ( agregationLevel != 0xFF &&
		 (format_c == format0 && m==0 && si_rnti != SI_RNTI))
		 {
		 //Only valid for OAI : Save some processing time when looking for DCI format0. From the log we see the DCI only on candidate 0.
		 return;
		 }
		 */
  } // candidate loop
  #ifdef NR_PDCCH_DCI_DEBUG
    printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure0)-> end candidate loop\n");
  #endif
}

#endif





/*void dci_decoding_procedure0(NR_UE_PDCCH **pdcch_vars,
                             int do_common,
                             dci_detect_mode_t mode,
                             uint8_t nr_tti_rx,
                             DCI_ALLOC_t *dci_alloc,
                             int16_t eNB_id,
                             uint8_t current_thread_id,
                             NR_DL_FRAME_PARMS *frame_parms,
                             uint8_t mi,
                             uint16_t si_rnti,
                             uint16_t ra_rnti,
                             uint16_t p_rnti,
                             uint8_t L,
                             uint8_t format_si,
                             uint8_t format_p,
                             uint8_t format_ra,
                             uint8_t format_c,
                             uint8_t sizeof_bits,
                             uint8_t sizeof_bytes,
                             uint8_t *dci_cnt,
                             uint8_t *format0_found,
                             uint8_t *format_c_found,
                             uint32_t *CCEmap0,
                             uint32_t *CCEmap1,
                             uint32_t *CCEmap2)
{

  uint16_t crc,CCEind,nCCE;
  uint32_t *CCEmap=NULL,CCEmap_mask=0;
  int L2=(1<<L);
  unsigned int Yk,nb_candidates = 0,i,m;
  unsigned int CCEmap_cand;
#ifdef NR_PDCCH_DCI_DEBUG
    printf("\t\t<-NR_PDCCH_DCI_DEBUG (dci_decoding_procedure0)-> \n");
#endif
  nCCE = get_nCCE(pdcch_vars[eNB_id]->num_pdcch_symbols,frame_parms,mi);

  if (nCCE > get_nCCE(3,frame_parms,1)) {
    LOG_D(PHY,"skip DCI decoding: nCCE=%d > get_nCCE(3,frame_parms,1)=%d\n", nCCE, get_nCCE(3,frame_parms,1));
    return;
  }

  if (nCCE<L2) {
    LOG_D(PHY,"skip DCI decoding: nCCE=%d < L2=%d\n", nCCE, L2);
    return;
  }

  if (mode == NO_DCI) {
    LOG_D(PHY, "skip DCI decoding: expect no DCIs at nr_tti_rx %d\n", nr_tti_rx);
    return;
  }

  if (do_common == 1) {
    nb_candidates = (L2==4) ? 4 : 2;
    Yk=0;
  } else {
    // Find first available in ue specific search space
    // according to procedure in Section 9.1.1 of 36.213 (v. 8.6)
    // compute Yk
    Yk = (unsigned int)pdcch_vars[eNB_id]->crnti;

    for (i=0; i<=nr_tti_rx; i++)
      Yk = (Yk*39827)%65537;

    Yk = Yk % (nCCE/L2);

    switch (L2) {
    case 1:
    case 2:
      nb_candidates = 6;
      break;

    case 4:
    case 8:
      nb_candidates = 2;
      break;

    default:
      DevParam(L2, do_common, eNB_id);
      break;
    }
  }

  //  for (CCEind=0;
  //     CCEind<nCCE2;
  //     CCEind+=(1<<L)) {

  if (nb_candidates*L2 > nCCE)
    nb_candidates = nCCE/L2;

  for (m=0; m<nb_candidates; m++) {

    CCEind = (((Yk+m)%(nCCE/L2))*L2);

    if (CCEind<32)
      CCEmap = CCEmap0;
    else if (CCEind<64)
      CCEmap = CCEmap1;
    else if (CCEind<96)
      CCEmap = CCEmap2;
    else {
      LOG_E(PHY,"Illegal CCEind %d (Yk %d, m %d, nCCE %d, L2 %d\n",CCEind,Yk,m,nCCE,L2);
      mac_xface->macphy_exit("Illegal CCEind\n");
      return; // not reached
    }

    switch (L2) {
    case 1:
      CCEmap_mask = (1<<(CCEind&0x1f));
      break;

    case 2:
      CCEmap_mask = (3<<(CCEind&0x1f));
      break;

    case 4:
      CCEmap_mask = (0xf<<(CCEind&0x1f));
      break;

    case 8:
      CCEmap_mask = (0xff<<(CCEind&0x1f));
      break;

    default:
      LOG_E( PHY, "Illegal L2 value %d\n", L2 );
      mac_xface->macphy_exit( "Illegal L2\n" );
      return; // not reached
    }

    CCEmap_cand = (*CCEmap)&CCEmap_mask;

    // CCE is not allocated yet

    if (CCEmap_cand == 0) {
#ifdef DEBUG_DCI_DECODING

      if (do_common == 1)
        LOG_I(PHY,"[DCI search nPdcch %d - common] Attempting candidate %d Aggregation Level %d DCI length %d at CCE %d/%d (CCEmap %x,CCEmap_cand %x)\n",
                pdcch_vars[eNB_id]->num_pdcch_symbols,m,L2,sizeof_bits,CCEind,nCCE,*CCEmap,CCEmap_mask);
      else
        LOG_I(PHY,"[DCI search nPdcch %d - ue spec] Attempting candidate %d Aggregation Level %d DCI length %d at CCE %d/%d (CCEmap %x,CCEmap_cand %x) format %d\n",
                pdcch_vars[eNB_id]->num_pdcch_symbols,m,L2,sizeof_bits,CCEind,nCCE,*CCEmap,CCEmap_mask,format_c);

#endif

      dci_decoding(sizeof_bits,
                   L,
                   &pdcch_vars[eNB_id]->e_rx[CCEind*72],
                   &dci_decoded_output[current_thread_id][0]);
      
      //  for (i=0;i<3+(sizeof_bits>>3);i++)
      //  printf("dci_decoded_output[%d] => %x\n",i,dci_decoded_output[i]);
      
      crc = (crc16(&dci_decoded_output[current_thread_id][0],sizeof_bits)>>16) ^ extract_crc(&dci_decoded_output[current_thread_id][0],sizeof_bits);
#ifdef DEBUG_DCI_DECODING
      printf("crc =>%x\n",crc);
#endif

      if (((L>1) && ((crc == si_rnti)|| (crc == p_rnti)|| (crc == ra_rnti)))||
          (crc == pdcch_vars[eNB_id]->crnti))   {
        dci_alloc[*dci_cnt].dci_length = sizeof_bits;
        dci_alloc[*dci_cnt].rnti       = crc;
        dci_alloc[*dci_cnt].L          = L;
        dci_alloc[*dci_cnt].firstCCE   = CCEind;

        //printf("DCI FOUND !!! crc =>%x,  sizeof_bits %d, sizeof_bytes %d \n",crc, sizeof_bits, sizeof_bytes);
        if (sizeof_bytes<=4) {
          dci_alloc[*dci_cnt].dci_pdu[3] = dci_decoded_output[current_thread_id][0];
          dci_alloc[*dci_cnt].dci_pdu[2] = dci_decoded_output[current_thread_id][1];
          dci_alloc[*dci_cnt].dci_pdu[1] = dci_decoded_output[current_thread_id][2];
          dci_alloc[*dci_cnt].dci_pdu[0] = dci_decoded_output[current_thread_id][3];
#ifdef DEBUG_DCI_DECODING
          printf("DCI => %x,%x,%x,%x\n",dci_decoded_output[current_thread_id][0],
                  dci_decoded_output[current_thread_id][1],
                  dci_decoded_output[current_thread_id][2],
                  dci_decoded_output[current_thread_id][3]);
#endif
        } else {
          dci_alloc[*dci_cnt].dci_pdu[7] = dci_decoded_output[current_thread_id][0];
          dci_alloc[*dci_cnt].dci_pdu[6] = dci_decoded_output[current_thread_id][1];
          dci_alloc[*dci_cnt].dci_pdu[5] = dci_decoded_output[current_thread_id][2];
          dci_alloc[*dci_cnt].dci_pdu[4] = dci_decoded_output[current_thread_id][3];
          dci_alloc[*dci_cnt].dci_pdu[3] = dci_decoded_output[current_thread_id][4];
          dci_alloc[*dci_cnt].dci_pdu[2] = dci_decoded_output[current_thread_id][5];
          dci_alloc[*dci_cnt].dci_pdu[1] = dci_decoded_output[current_thread_id][6];
          dci_alloc[*dci_cnt].dci_pdu[0] = dci_decoded_output[current_thread_id][7];
#ifdef DEBUG_DCI_DECODING
          printf("DCI => %x,%x,%x,%x,%x,%x,%x,%x\n",
              dci_decoded_output[current_thread_id][0],dci_decoded_output[current_thread_id][1],dci_decoded_output[current_thread_id][2],dci_decoded_output[current_thread_id][3],
              dci_decoded_output[current_thread_id][4],dci_decoded_output[current_thread_id][5],dci_decoded_output[current_thread_id][6],dci_decoded_output[current_thread_id][7]);
#endif
        }

        if (crc==si_rnti) {
          dci_alloc[*dci_cnt].format     = format_si;
          *dci_cnt = *dci_cnt+1;
        } else if (crc==p_rnti) {
          dci_alloc[*dci_cnt].format     = format_p;
          *dci_cnt = *dci_cnt+1;
        } else if (crc==ra_rnti) {
          dci_alloc[*dci_cnt].format     = format_ra;
          // store first nCCE of group for PUCCH transmission of ACK/NAK
          pdcch_vars[eNB_id]->nCCE[nr_tti_rx]=CCEind;
          *dci_cnt = *dci_cnt+1;
        } else if (crc==pdcch_vars[eNB_id]->crnti) {

          if ((mode&UL_DCI)&&(format_c == format0)&&((dci_decoded_output[current_thread_id][0]&0x80)==0)) {// check if pdu is format 0 or 1A
            if (*format0_found == 0) {
              dci_alloc[*dci_cnt].format     = format0;
              *format0_found = 1;
              *dci_cnt = *dci_cnt+1;
              pdcch_vars[eNB_id]->nCCE[nr_tti_rx]=CCEind;
            }
          } else if (format_c == format0) { // this is a format 1A DCI
            dci_alloc[*dci_cnt].format     = format1A;
            *dci_cnt = *dci_cnt+1;
            pdcch_vars[eNB_id]->nCCE[nr_tti_rx]=CCEind;
          } else {
            // store first nCCE of group for PUCCH transmission of ACK/NAK
            if (*format_c_found == 0) {
              dci_alloc[*dci_cnt].format     = format_c;
              *dci_cnt = *dci_cnt+1;
              *format_c_found = 1;
              pdcch_vars[eNB_id]->nCCE[nr_tti_rx]=CCEind;
            }
          }
        }

        //LOG_I(PHY,"DCI decoding CRNTI  [format: %d, nCCE[nr_tti_rx: %d]: %d ], AggregationLevel %d \n",format_c, nr_tti_rx, pdcch_vars[eNB_id]->nCCE[nr_tti_rx],L2);
        //  memcpy(&dci_alloc[*dci_cnt].dci_pdu[0],dci_decoded_output,sizeof_bytes);



        switch (1<<L) {
        case 1:
          *CCEmap|=(1<<(CCEind&0x1f));
          break;

        case 2:
          *CCEmap|=(1<<(CCEind&0x1f));
          break;

        case 4:
          *CCEmap|=(1<<(CCEind&0x1f));
          break;

        case 8:
          *CCEmap|=(1<<(CCEind&0x1f));
          break;
        }

#ifdef DEBUG_DCI_DECODING
        LOG_I(PHY,"[DCI search] Found DCI %d rnti %x Aggregation %d length %d format %s in CCE %d (CCEmap %x) candidate %d / %d \n",
              *dci_cnt,crc,1<<L,sizeof_bits,dci_format_strings[dci_alloc[*dci_cnt-1].format],CCEind,*CCEmap,m,nb_candidates );
        dump_dci(frame_parms,&dci_alloc[*dci_cnt-1]);

#endif
         return;
      } // rnti match
    }  // CCEmap_cand == 0
    
//	if ( agregationLevel != 0xFF &&
//        (format_c == format0 && m==0 && si_rnti != SI_RNTI))
//    {
//      //Only valid for OAI : Save some processing time when looking for DCI format0. From the log we see the DCI only on candidate 0.
//      return;
//    }

  } // candidate loop
}

uint16_t dci_CRNTI_decoding_procedure(PHY_VARS_NR_UE *ue,
                                DCI_ALLOC_t *dci_alloc,
                                uint8_t DCIFormat,
                                uint8_t agregationLevel,
                                int16_t eNB_id,
                                uint8_t nr_tti_rx)
{

  uint8_t  dci_cnt=0,old_dci_cnt=0;
  uint32_t CCEmap0=0,CCEmap1=0,CCEmap2=0;
  NR_UE_PDCCH **pdcch_vars = ue->pdcch_vars[ue->current_thread_id[nr_tti_rx]];
  NR_DL_FRAME_PARMS *frame_parms  = &ue->frame_parms;
  uint8_t mi = get_mi(&ue->frame_parms,nr_tti_rx);
  uint16_t ra_rnti=99;
  uint8_t format0_found=0,format_c_found=0;
  uint8_t tmode = ue->transmission_mode[eNB_id];
  uint8_t frame_type = frame_parms->frame_type;
  uint8_t format0_size_bits=0,format0_size_bytes=0;
  uint8_t format1_size_bits=0,format1_size_bytes=0;
  dci_detect_mode_t mode = dci_detect_mode_select(&ue->frame_parms,nr_tti_rx);

  switch (frame_parms->N_RB_DL) {
  case 6:
    if (frame_type == TDD) {
      format0_size_bits  = sizeof_DCI0_1_5MHz_TDD_1_6_t;
      format0_size_bytes = sizeof(DCI0_1_5MHz_TDD_1_6_t);
      format1_size_bits  = sizeof_DCI1_1_5MHz_TDD_t;
      format1_size_bytes = sizeof(DCI1_1_5MHz_TDD_t);

    } else {
      format0_size_bits  = sizeof_DCI0_1_5MHz_FDD_t;
      format0_size_bytes = sizeof(DCI0_1_5MHz_FDD_t);
      format1_size_bits  = sizeof_DCI1_1_5MHz_FDD_t;
      format1_size_bytes = sizeof(DCI1_1_5MHz_FDD_t);
    }

    break;

  case 25:
  default:
    if (frame_type == TDD) {
      format0_size_bits  = sizeof_DCI0_5MHz_TDD_1_6_t;
      format0_size_bytes = sizeof(DCI0_5MHz_TDD_1_6_t);
      format1_size_bits  = sizeof_DCI1_5MHz_TDD_t;
      format1_size_bytes = sizeof(DCI1_5MHz_TDD_t);
    } else {
      format0_size_bits  = sizeof_DCI0_5MHz_FDD_t;
      format0_size_bytes = sizeof(DCI0_5MHz_FDD_t);
      format1_size_bits  = sizeof_DCI1_5MHz_FDD_t;
      format1_size_bytes = sizeof(DCI1_5MHz_FDD_t);
    }

    break;

  case 50:
    if (frame_type == TDD) {
      format0_size_bits  = sizeof_DCI0_10MHz_TDD_1_6_t;
      format0_size_bytes = sizeof(DCI0_10MHz_TDD_1_6_t);
      format1_size_bits  = sizeof_DCI1_10MHz_TDD_t;
      format1_size_bytes = sizeof(DCI1_10MHz_TDD_t);

    } else {
      format0_size_bits  = sizeof_DCI0_10MHz_FDD_t;
      format0_size_bytes = sizeof(DCI0_10MHz_FDD_t);
      format1_size_bits  = sizeof_DCI1_10MHz_FDD_t;
      format1_size_bytes = sizeof(DCI1_10MHz_FDD_t);
    }

    break;

  case 100:
    if (frame_type == TDD) {
      format0_size_bits  = sizeof_DCI0_20MHz_TDD_1_6_t;
      format0_size_bytes = sizeof(DCI0_20MHz_TDD_1_6_t);
      format1_size_bits  = sizeof_DCI1_20MHz_TDD_t;
      format1_size_bytes = sizeof(DCI1_20MHz_TDD_t);
    } else {
      format0_size_bits  = sizeof_DCI0_20MHz_FDD_t;
      format0_size_bytes = sizeof(DCI0_20MHz_FDD_t);
      format1_size_bits  = sizeof_DCI1_20MHz_FDD_t;
      format1_size_bytes = sizeof(DCI1_20MHz_FDD_t);
    }

    break;
  }

  if (ue->prach_resources[eNB_id])
    ra_rnti = ue->prach_resources[eNB_id]->ra_RNTI;

  // Now check UE_SPEC format0/1A ue_spec search spaces at aggregation 8
  dci_decoding_procedure0(pdcch_vars,0,mode,
                          nr_tti_rx,
                          dci_alloc,
                          eNB_id,
                          ue->current_thread_id[nr_tti_rx],
                          frame_parms,
                          mi,
                          ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                          ra_rnti,
              P_RNTI,
              agregationLevel,
                          format1A,
                          format1A,
                          format1A,
                          format0,
                          format0_size_bits,
                          format0_size_bytes,
                          &dci_cnt,
                          &format0_found,
                          &format_c_found,
                          &CCEmap0,
                          &CCEmap1,
                          &CCEmap2);

  if ((CCEmap0==0xffff)||
      ((format0_found==1)&&(format_c_found==1)))
    return(dci_cnt);

  if (DCIFormat == 1)
  {
      if ((tmode < 3) || (tmode == 7)) {
          //printf("Crnti decoding frame param agregation %d DCI %d \n",agregationLevel,DCIFormat);

          // Now check UE_SPEC format 1 search spaces at aggregation 1

           //printf("[DCI search] Format 1/1A aggregation 1\n");

          old_dci_cnt=dci_cnt;
          dci_decoding_procedure0(pdcch_vars,0,mode,nr_tti_rx,
                                  dci_alloc,
                                  eNB_id,
                                  ue->current_thread_id[nr_tti_rx],
                                  frame_parms,
                                  mi,
                                  ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                                  ra_rnti,
                                  P_RNTI,
                                  0,
                                  format1A,
                                  format1A,
                                  format1A,
                                  format1,
                                  format1_size_bits,
                                  format1_size_bytes,
                                  &dci_cnt,
                                  &format0_found,
                                  &format_c_found,
                                  &CCEmap0,
                                  &CCEmap1,
                                  &CCEmap2);

          if ((CCEmap0==0xffff) ||
              (format_c_found==1))
            return(dci_cnt);

          if (dci_cnt>old_dci_cnt)
            return(dci_cnt);

          //printf("Crnti 1 decoding frame param agregation %d DCI %d \n",agregationLevel,DCIFormat);

      }
      else
      {
          AssertFatal(0,"Other Transmission mode not yet coded\n");
      }
  }
  else
  {
     AssertFatal(0,"DCI format %d not yet implemented \n",DCIFormat);
  }

  return(dci_cnt);

}
*/

#ifdef NR_PDCCH_DCI_RUN

uint16_t nr_dci_format_size (crc_scrambled_t crc_scrambled,
                             uint8_t pusch_alloc_list,
                             uint16_t n_RB_ULBWP,
                             uint16_t n_RB_DLBWP,
                             uint8_t dci_fields_sizes[NBR_NR_DCI_FIELDS][NBR_NR_FORMATS]){
#ifdef NR_PDCCH_DCI_DEBUG
    printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_format_size)-> crc_scrambled=%d, pusch_alloc_list=%d, n_RB_ULBWP=%d, n_RB_DLBWP=%d\n",crc_scrambled,pusch_alloc_list,n_RB_ULBWP,n_RB_DLBWP);
#endif

/*
 * Formats 0_1, not completely implemented. See (*)
 */
// format {0_0,0_1,1_0,1_1,2_0,2_1,2_2,2_3} according to 38.212 Section 7.3.1
/*
#define NBR_NR_FORMATS         8
#define NBR_NR_DCI_FIELDS     56

#define IDENTIFIER_DCI_FORMATS           0
#define CARRIER_IND                      1
#define SUL_IND_0_1                      2
#define SLOT_FORMAT_IND                  3
#define PRE_EMPTION_IND                  4
#define TPC_CMD_NUMBER                   5
#define BLOCK_NUMBER                     6
#define BANDWIDTH_PART_IND               7
#define SHORT_MESSAGE_IND                8
#define SHORT_MESSAGES                   9
#define FREQ_DOM_RESOURCE_ASSIGNMENT_UL 10
#define FREQ_DOM_RESOURCE_ASSIGNMENT_DL 11
#define TIME_DOM_RESOURCE_ASSIGNMENT    12
#define VRB_TO_PRB_MAPPING              13
#define PRB_BUNDLING_SIZE_IND           14
#define RATE_MATCHING_IND               15
#define ZP_CSI_RS_TRIGGER               16
#define FREQ_HOPPING_FLAG               17
#define TB1_MCS                         18
#define TB1_NDI                         19
#define TB1_RV                          20
#define TB2_MCS                         21
#define TB2_NDI                         22
#define TB2_RV                          23
#define MCS                             24
#define NDI                             25
#define RV                              26
#define HARQ_PROCESS_NUMBER             27
#define DAI_                            28
#define FIRST_DAI                       29
#define SECOND_DAI                      30
#define TB_SCALING                      31
#define TPC_PUSCH                       32
#define TPC_PUCCH                       33
#define PUCCH_RESOURCE_IND              34
#define PDSCH_TO_HARQ_FEEDBACK_TIME_IND 35
//#define SHORT_MESSAGE_IND             33
#define SRS_RESOURCE_IND                36
#define PRECOD_NBR_LAYERS               37
#define ANTENNA_PORTS                   38
#define TCI                             39
#define SRS_REQUEST                     40
#define TPC_CMD_NUMBER_FORMAT2_3        41
#define CSI_REQUEST                     42
#define CBGTI                           43
#define CBGFI                           44
#define PTRS_DMRS                       45
#define BETA_OFFSET_IND                 46
#define DMRS_SEQ_INI                    47
#define UL_SCH_IND                      48
#define PADDING_NR_DCI                  49
#define SUL_IND_0_0                     50
#define RA_PREAMBLE_INDEX               51
#define SUL_IND_1_0                     52
#define SS_PBCH_INDEX                   53
#define PRACH_MASK_INDEX                54
#define RESERVED_NR_DCI                 55
*/
  //uint8_t pusch_alloc_list=1;
  // number of ZP CSI-RS resource sets in the higher layer parameter [ZP-CSI-RS-ResourceConfigList]
  uint8_t n_zp = 1;
  uint8_t n_SRS=1;
  // for PUSCH hopping with resource allocation type 1
  //      n_UL_hopping = 1 if the higher layer parameter frequencyHoppingOffsetLists contains two  offset values
  //      n_UL_hopping = 2 if the higher layer parameter frequencyHoppingOffsetLists contains four offset values
  uint8_t n_UL_hopping=0;
  uint8_t dci_field_size_table [NBR_NR_DCI_FIELDS][NBR_NR_FORMATS] = { // This table contains the number of bits for each field (row) contained in each dci format (column).
                                                                       // The values of the variables indicate field sizes in number of bits
//Format0_0                     Format0_1                      Format1_0                      Format1_1             Formats2_0/1/2/3
{1,                             1,                             (((crc_scrambled == _p_rnti) || (crc_scrambled == _si_rnti) || (crc_scrambled == _ra_rnti)) ? 0:1),
                                                                                              1,                             0,0,0,0}, // 0  IDENTIFIER_DCI_FORMATS:
{0,                             3,                             0,                             3,                             0,0,0,0}, // 1  CARRIER_IND: 0 or 3 bits, as defined in Subclause x.x of [5, TS38.213]
{0,                             0,                             0,                             0,                             0,0,0,0}, // 2  SUL_IND_0_1:
{0,                             0,                             0,                             0,                             1,0,0,0}, // 3  SLOT_FORMAT_IND: size of DCI format 2_0 is configurable by higher layers up to 128 bits, according to Subclause 11.1.1 of [5, TS 38.213]
{0,                             0,                             0,                             0,                             0,1,0,0}, // 4  PRE_EMPTION_IND: size of DCI format 2_1 is configurable by higher layers up to 126 bits, according to Subclause 11.2 of [5, TS 38.213]. Each pre-emption indication is 14 bits
{0,                             0,                             0,                             0,                             0,0,1,0}, // 5  TPC_CMD_NUMBER: The parameter xxx provided by higher layers determines the index to the TPC command number for an UL of a cell. Each TPC command number is 2 bits
{0,                             0,                             0,                             0,                             0,0,0,1}, // 6  BLOCK_NUMBER: starting position of a block is determined by the parameter startingBitOfFormat2_3
{0,                             ceil(log2(n_RB_ULBWP)),        0,                             ceil(log2(n_RB_ULBWP)),        0,0,0,0}, // 7  BANDWIDTH_PART_IND:
{0,                             0,                             ((crc_scrambled == _p_rnti) ? 2:0),
                                                                                              0,                             0,0,0,0}, // 8  SHORT_MESSAGE_IND 2 bits if crc scrambled with P-RNTI
{0,                             0,                             ((crc_scrambled == _p_rnti) ? 8:0),
                                                                                              0,                             0,0,0,0}, // 9  SHORT_MESSAGES 8 bit8 if crc scrambled with P-RNTI
{(ceil(log2(n_RB_ULBWP*(n_RB_ULBWP+1)/2)))-n_UL_hopping,
                                (ceil(log2(n_RB_ULBWP*(n_RB_ULBWP+1)/2)))-n_UL_hopping,
                                                               0,                             0,                             0,0,0,0}, // 10 FREQ_DOM_RESOURCE_ASSIGNMENT_UL: PUSCH hopping with resource allocation type 1 not considered
                                                                                                                                       //    (NOTE 1) If DCI format 0_0 is monitored in common search space
                                                                                                                                       //    and if the number of information bits in the DCI format 0_0 prior to padding
                                                                                                                                       //    is larger than the payload size of the DCI format 1_0 monitored in common search space
                                                                                                                                       //    the bitwidth of the frequency domain resource allocation field in the DCI format 0_0
                                                                                                                                       //    is reduced such that the size of DCI format 0_0 equals to the size of the DCI format 1_0
{0,                             0,                             ceil(log2(n_RB_DLBWP*(n_RB_DLBWP+1)/2)),
                                                                                              ceil(log2(n_RB_DLBWP*(n_RB_DLBWP+1)/2)),
                                                                                                                             0,0,0,0}, // 11 FREQ_DOM_RESOURCE_ASSIGNMENT_DL:
{4,                             log2(pusch_alloc_list),        4,                             log2(pusch_alloc_list),        0,0,0,0}, // 12 TIME_DOM_RESOURCE_ASSIGNMENT: 0, 1, 2, 3, or 4 bits as defined in Subclause 6.1.2.1 of [6, TS 38.214]. The bitwidth for this field is determined as log2(I) bits,
                                                                                                                                       //    where I the number of entries in the higher layer parameter pusch-AllocationList
{0,                             1,                             1,                             1,                             0,0,0,0}, // 13 VRB_TO_PRB_MAPPING: 0 bit if only resource allocation type 0
{0,                             0,                             0,                             1,                             0,0,0,0}, // 14 PRB_BUNDLING_SIZE_IND:0 bit if the higher layer parameter PRB_bundling is not configured or is set to 'static', or 1 bit if the higher layer parameter PRB_bundling is set to 'dynamic' according to Subclause 5.1.2.3 of [6, TS 38.214]
{0,                             0,                             0,                             2,                             0,0,0,0}, // 15 RATE_MATCHING_IND: 0, 1, or 2 bits according to higher layer parameter rate-match-PDSCH-resource-set
{0,                             0,                             0,                             log2(n_zp)+1,                  0,0,0,0}, // 16 ZP_CSI_RS_TRIGGER:
{1,                             1,                             0,                             0,                             0,0,0,0}, // 17 FREQ_HOPPING_FLAG: 0 bit if only resource allocation type 0
{0,                             0,                             0,                             5,                             0,0,0,0}, // 18 TB1_MCS:
{0,                             0,                             0,                             1,                             0,0,0,0}, // 19 TB1_NDI:
{0,                             0,                             0,                             2,                             0,0,0,0}, // 20 TB1_RV:
{0,                             0,                             0,                             5,                             0,0,0,0}, // 21 TB2_MCS:
{0,                             0,                             0,                             1,                             0,0,0,0}, // 22 TB2_NDI:
{0,                             0,                             0,                             2,                             0,0,0,0}, // 23 TB2_RV:
{5,                             5,                             5,                             0,                             0,0,0,0}, // 24 MCS:
{1,                             1,                             (crc_scrambled == _c_rnti)?1:0,0,                             0,0,0,0}, // 25 NDI:
{2,                             2,                             (((crc_scrambled == _c_rnti) || (crc_scrambled == _si_rnti)) ? 2:0),
                                                                                              0,                             0,0,0,0}, // 26 RV:
{4,                             4,                             (crc_scrambled == _c_rnti)?4:0,4,                             0,0,0,0}, // 27 HARQ_PROCESS_NUMBER:
{0,                             0,                             (crc_scrambled == _c_rnti)?2:0,2,                             0,0,0,0}, // 28 DAI: For format1_1: 4 if more than one serving cell are configured in the DL and the higher layer parameter HARQ-ACK-codebook=dynamic, where the 2 MSB bits are the counter DAI and the 2 LSB bits are the total DAI
                                                                                                                                       //    2 if one serving cell is configured in the DL and the higher layer parameter HARQ-ACK-codebook=dynamic, where the 2 bits are the counter DAI
                                                                                                                                       //    0 otherwise
{0,                             2,                             0,                             0,                             0,0,0,0}, // 29 FIRST_DAI: (1 or 2 bits) 1 bit for semi-static HARQ-ACK // 2 bits for dynamic HARQ-ACK codebook with single HARQ-ACK codebook
{0,                             2,                             0,                             0,                             0,0,0,0}, // 30 SECOND_DAI: (0 or 2 bits) 2 bits for dynamic HARQ-ACK codebook with two HARQ-ACK sub-codebooks // 0 bits otherwise
{0,                             0,                             (((crc_scrambled == _p_rnti) || (crc_scrambled == _ra_rnti)) ? 2:0),
                                                                                              0,                             0,0,0,0}, // 31 TB_SCALING
{2,                             2,                             0,                             0,                             0,0,0,0}, // 32 TPC_PUSCH:
{0,                             0,                             (crc_scrambled == _c_rnti)?2:0,2,                             0,0,0,0}, // 33 TPC_PUCCH:
{0,                             0,                             (crc_scrambled == _c_rnti)?3:0,3,                             0,0,0,0}, // 34 PUCCH_RESOURCE_IND:
{0,                             0,                             (crc_scrambled == _c_rnti)?3:0,3,                             0,0,0,0}, // 35 PDSCH_TO_HARQ_FEEDBACK_TIME_IND:
{0,                             log2(n_SRS),                   0,                             0,                             0,0,0,0}, // 36 SRS_RESOURCE_IND:
{0,                             0,                             0,                             0,                             0,0,0,0}, // 37 PRECOD_NBR_LAYERS:
{0,                             0,                             0,                             0,                             0,0,0,0}, // 38 ANTENNA_PORTS:
{0,                             0,                             0,                             3,                             0,0,0,0}, // 39 TCI: 0 bit if higher layer parameter tci-PresentInDCI is not enabled; otherwise 3 bits
{0,                             3,                             0,                             0,                             0,0,0,2}, // 40 SRS_REQUEST:
{0,                             0,                             0,                             0,                             0,0,0,2}, // 41 TPC_CMD_NUMBER_FORMAT2_3:
{0,                             6,                             0,                             0,                             0,0,0,0}, // 42 CSI_REQUEST:
{0,                             8,                             0,                             8,                             0,0,0,0}, // 43 CBGTI: 0, 2, 4, 6, or 8 bits determined by higher layer parameter maxCodeBlockGroupsPerTransportBlock for the PDSCH
{0,                             0,                             0,                             1,                             0,0,0,0}, // 44 CBGFI: 0 or 1 bit determined by higher layer parameter codeBlockGroupFlushIndicator
{0,                             2,                             0,                             0,                             0,0,0,0}, // 45 PTRS_DMRS:
{0,                             2,                             0,                             0,                             0,0,0,0}, // 46 BETA_OFFSET_IND:
{0,                             1,                             0,                             1,                             0,0,0,0}, // 47 DMRS_SEQ_INI: 1 bit if the cell has two ULs and the number of bits for DCI format 1_0 before padding
                                                                                                                                       //    is larger than the number of bits for DCI format 0_0 before padding; 0 bit otherwise
{0,                             1,                             0,                             0,                             0,0,0,0}, // 48 UL_SCH_IND: value of "1" indicates UL-SCH shall be transmitted on the PUSCH and a value of "0" indicates UL-SCH shall not be transmitted on the PUSCH
{0,                             0,                             0,                             0,                             0,0,0,0}, // 49 PADDING_NR_DCI:
                                                                                                                                       //    (NOTE 2) If DCI format 0_0 is monitored in common search space
                                                                                                                                       //    and if the number of information bits in the DCI format 0_0 prior to padding
                                                                                                                                       //    is less than the payload size of the DCI format 1_0 monitored in common search space
                                                                                                                                       //    zeros shall be appended to the DCI format 0_0
                                                                                                                                       //    until the payload size equals that of the DCI format 1_0
{0,                             0,                             0,                             0,                             0,0,0,0}, // 50 SUL_IND_0_0:
{0,                             0,                             0,                             0,                             0,0,0,0}, // 51 RA_PREAMBLE_INDEX (random access procedure initiated by a PDCCH order not implemented, FIXME!!!)
{0,                             0,                             0,                             0,                             0,0,0,0}, // 52 SUL_IND_1_0 (random access procedure initiated by a PDCCH order not implemented, FIXME!!!)
{0,                             0,                             0,                             0,                             0,0,0,0}, // 53 SS_PBCH_INDEX (random access procedure initiated by a PDCCH order not implemented, FIXME!!!)
{0,                             0,                             0,                             0,                             0,0,0,0}, // 54 PRACH_MASK_INDEX (random access procedure initiated by a PDCCH order not implemented, FIXME!!!)
{0,                             0,                             ((crc_scrambled == _p_rnti)?6:(((crc_scrambled == _si_rnti) || (crc_scrambled == _ra_rnti))?16:0)),
                                                                                              0,                             0,0,0,0}  // 55 RESERVED_NR_DCI
};

// NOTE 1: adjustments in freq_dom_resource_assignment_UL to be done if necessary
// NOTE 2: adjustments in padding to be done if necessary

uint8_t dci_size [8] = {0,0,0,0,0,0,0,0}; // will contain size for each format

  for (int i=0 ; i<NBR_NR_FORMATS ; i++) {
//#ifdef NR_PDCCH_DCI_DEBUG
//  printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_format_size)-> i=%d, j=%d\n", i, j);
//#endif
    for (int j=0; j<NBR_NR_DCI_FIELDS; j++) {
      dci_size [i] = dci_size [i] + dci_field_size_table[j][i]; // dci_size[i] contains the size in bits of the dci pdu format i
      //if (i==(int)format-15) {                                  // (int)format-15 indicates the position of each format in the table (e.g. format1_0=17 -> position in table is 2)
      dci_fields_sizes[j][i] = dci_field_size_table[j][i];       // dci_fields_sizes[j] contains the sizes of each field (j) for a determined format i
      //}
    }
    #ifdef NR_PDCCH_DCI_DEBUG
      printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_format_size) dci_size[%d]=%d for n_RB_ULBWP=%d\n",
             i,dci_size[i],n_RB_ULBWP);
    #endif
  }
#ifdef NR_PDCCH_DCI_DEBUG
  printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_format_size) dci_fields_sizes[][] = { \n");
  for (int j=0; j<NBR_NR_DCI_FIELDS; j++){
    printf("\t\t");
    for (int i=0; i<NBR_NR_FORMATS ; i++) printf("%d\t",dci_fields_sizes[j][i]);
    printf("\n");
  }
  printf(" }\n");
  printf("\n\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_format_size) dci_size[0]=%d, dci_size[2]=%d\n",dci_size[0],dci_size[2]);
#endif

//  if ((format == format0_0) || (format == format1_0)) {
  // According to Section 7.3.1.1.1 in TS 38.212
  // If DCI format 0_0 is monitored in common search space and if the number of information bits in the DCI format 0_0 prior to padding
  // is less than the payload size of the DCI format 1_0 monitored in common search space for scheduling the same serving cell,
  // zeros shall be appended to the DCI format 0_0 until the payload size equals that of the DCI format 1_0.
  if (dci_size[0] < dci_size[2]) { // '0' corresponding to index for format0_0 and '2' corresponding to index of format1_0
    //if (format == format0_0) {
    dci_fields_sizes[PADDING_NR_DCI][0] = dci_size[2] - dci_size[0];
    dci_size[0] = dci_size[2];
    #ifdef NR_PDCCH_DCI_DEBUG
      printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_format_size) new dci_size[format0_0]=%d\n",dci_size[0]);
    #endif
    //}
  }
  // If DCI format 0_0 is monitored in common search space and if the number of information bits in the DCI format 0_0 prior to padding
  // is larger than the payload size of the DCI format 1_0 monitored in common search space for scheduling the same serving cell,
  // the bitwidth of the frequency domain resource allocation field in the DCI format 0_0 is reduced
  // such that the size of DCI format 0_0 equals to the size of the DCI format 1_0..
  if (dci_size[0] > dci_size[2]) {
    //if (format == format0_0) {
    dci_fields_sizes[FREQ_DOM_RESOURCE_ASSIGNMENT_UL][0] -= (dci_size[0] - dci_size[2]);
    dci_size[0] = dci_size[2];
    #ifdef NR_PDCCH_DCI_DEBUG
      printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_format_size) new dci_size[format0_0]=%d\n",dci_size[0]);
    #endif
    //}
  }
//  }
  #ifdef NR_PDCCH_DCI_DEBUG
    printf("\t\t<-NR_PDCCH_DCI_DEBUG (nr_dci_format_size) dci_fields_sizes[][] = { \n");
    for (int j=0; j<NBR_NR_DCI_FIELDS; j++){
      printf("\t\t");
      for (int i=0; i<NBR_NR_FORMATS ; i++) printf("%d\t",dci_fields_sizes[j][i]);
      printf("\n");
    }
    printf(" }\n");
  #endif

  return dci_size[0];
}

#endif

#ifdef NR_PDCCH_DCI_RUN

uint8_t nr_dci_decoding_procedure(int s,
                                  int p,
                                  PHY_VARS_NR_UE *ue,
                                  NR_DCI_ALLOC_t *dci_alloc,
                                  int do_common,
                                  int16_t eNB_id,
                                  uint8_t nr_tti_rx,
                                  uint8_t dci_fields_sizes[NBR_NR_DCI_FIELDS][NBR_NR_FORMATS],
                                  uint8_t dci_fields_sizes_cnt[MAX_NR_DCI_DECODED_SLOT][NBR_NR_DCI_FIELDS][NBR_NR_FORMATS],
                                  uint16_t n_RB_ULBWP,
                                  uint16_t n_RB_DLBWP,
                                  crc_scrambled_t *crc_scrambled,
                                  format_found_t *format_found) {

  crc_scrambled_t crc_scrambled_ = *crc_scrambled;
  format_found_t format_found_   = *format_found;
  #ifdef NR_PDCCH_DCI_DEBUG
    printf("\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure) nr_tti_rx=%d and format_found=%d %d\n",nr_tti_rx,*format_found,format_found_);
  #endif
  uint8_t dci_cnt = 0, old_dci_cnt = 0;
  uint32_t CCEmap0 = 0, CCEmap1 = 0, CCEmap2 = 0;

  NR_UE_PDCCH **pdcch_vars = ue->pdcch_vars[ue->current_thread_id[nr_tti_rx]];
  NR_DL_FRAME_PARMS *frame_parms = &ue->frame_parms;
  uint8_t mi;// = get_mi(&ue->frame_parms, nr_tti_rx);
  // we need to initialize this values as crc is going to be compared with them
  uint16_t c_rnti=pdcch_vars[eNB_id]->crnti;
  uint16_t cs_rnti,new_rnti,tc_rnti;
  uint16_t p_rnti=P_RNTI;
  uint16_t si_rnti=SI_RNTI;
  uint16_t ra_rnti=99;
  uint16_t sp_csi_rnti,sfi_rnti,int_rnti,tpc_pusch_rnti,tpc_pucch_rnti,tpc_srs_rnti; //FIXME
  uint16_t crc_scrambled_values[13] = {c_rnti,cs_rnti,new_rnti,tc_rnti,p_rnti,si_rnti,ra_rnti,sp_csi_rnti,sfi_rnti,int_rnti,tpc_pusch_rnti,tpc_pucch_rnti,tpc_srs_rnti};

  //uint8_t format0_found = 0, format_c_found = 0;
  uint8_t tmode = ue->transmission_mode[eNB_id];
  uint8_t frame_type = frame_parms->frame_type;

  uint8_t format_0_0_1_0_size_bits = 0, format_0_0_1_0_size_bytes = 0; //FIXME
  uint8_t format_0_1_1_1_size_bits = 0, format_0_1_1_1_size_bytes = 0; //FIXME
  uint8_t format_2_0_size_bits = 0, format_2_0_size_bytes = 0; //FIXME
  uint8_t format_2_1_size_bits = 0, format_2_1_size_bytes = 0; //FIXME
  uint8_t format_2_2_size_bits = 0, format_2_2_size_bytes = 0; //FIXME
  uint8_t format_2_3_size_bits = 0, format_2_3_size_bytes = 0; //FIXME
  /*
   *
   * The implementation of this function will depend on the information given by the searchSpace IE
   *
   * In LTE the UE has no knowledge about:
   * - the type of search (common or ue-specific)
   * - the DCI format it is going to be decoded when performing the PDCCH monitoring
   * So the blind decoding has to be done for common and ue-specific searchSpaces for each aggregation level and for each dci format
   *
   * In NR the UE has a knowledge about the search Space type and the DCI format it is going to be decoded,
   * so in the blind decoding we can call the function nr_dci_decoding_procedure0 with the searchSpace type and the dci format parameter
   * We will call this function as many times as aggregation levels indicated in searchSpace
   * Implementation according to 38.213 v15.1.0 Section 10.
   *
   */

  NR_UE_PDCCH *pdcch_vars2                         = ue->pdcch_vars[ue->current_thread_id[nr_tti_rx]][eNB_id];
  NR_UE_SEARCHSPACE_CSS_DCI_FORMAT_t css_dci_format = pdcch_vars2->searchSpace[s].searchSpaceType.common_dci_formats;
  NR_UE_SEARCHSPACE_USS_DCI_FORMAT_t uss_dci_format = pdcch_vars2->searchSpace[s].searchSpaceType.ue_specific_dci_formats;

  // The following initialization is only for test purposes. To be removed
  // NR_UE_SEARCHSPACE_CSS_DCI_FORMAT_t
  css_dci_format = cformat0_0_and_1_0;
  //NR_UE_SEARCHSPACE_USS_DCI_FORMAT_t
  uss_dci_format = uformat0_0_and_1_0;

  /*
   * Possible overlap between RE for SS/PBCH blocks (described in section 10, 38.213) has not been implemented yet
   * This can be implemented by setting variable 'mode = NO_DCI' when overlap occurs
   */
  //dci_detect_mode_t mode = 3; //dci_detect_mode_select(&ue->frame_parms, nr_tti_rx);

  #ifdef NR_PDCCH_DCI_DEBUG
    printf("\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure)-> searSpaceType=%d\n",do_common);
    if (do_common) {
      printf("\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure)-> css_dci_format=%d\n",css_dci_format);
    } else {
      printf("\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure)-> uss_dci_format=%d\n",uss_dci_format);
    }
  #endif
  // A set of PDCCH candidates for a UE to monitor is defined in terms of PDCCH search spaces
  if (do_common) { // COMMON SearchSpaceType assigned to current SearchSpace/CORESET
    // Type0-PDCCH  common search space for a DCI format with CRC scrambled by a SI-RNTI
               // number of consecutive resource blocks and a number of consecutive symbols for
               // the control resource set of the Type0-PDCCH common search space from
               // the four most significant bits of RMSI-PDCCH-Config as described in Tables 13-1 through 13-10
               // and determines PDCCH monitoring occasions
               // from the four least significant bits of RMSI-PDCCH-Config,
               // included in MasterInformationBlock, as described in Tables 13-11 through 13-15
    // Type0A-PDCCH common search space for a DCI format with CRC scrambled by a SI-RNTI
    // Type1-PDCCH  common search space for a DCI format with CRC scrambled by a RA-RNTI, or a TC-RNTI, or a C-RNTI
    // Type2-PDCCH  common search space for a DCI format with CRC scrambled by a P-RNTI
    if (css_dci_format == cformat0_0_and_1_0) {
      // 38.213 v15.1.0 Table 10.1-1: CCE aggregation levels and maximum number of PDCCH candidates per CCE
      // aggregation level for Type0/Type0A/Type2-PDCCH common search space
      //   CCE Aggregation Level    Number of Candidates
      //           4                       4
      //           8                       2
      //           16                      1
      // FIXME
      // We shall consider Table 10.1-1 to calculate the blind decoding only for Type0/Type0A/Type2-PDCCH
      // Shall we consider the nrofCandidates in SearSpace IE that considers Aggregation Levels 1,2,4,8,16? Our implementation considers Table 10.1-1

      // blind decoding (Type0-PDCCH,Type0A-PDCCH,Type1-PDCCH,Type2-PDCCH)
      // for format0_0 => we are NOT implementing format0_0 for common search spaces. FIXME!

      // for format0_0 and format1_0, first we calculate dci pdu size
      format_0_0_1_0_size_bits = nr_dci_format_size(_c_rnti,16,n_RB_ULBWP,n_RB_DLBWP,dci_fields_sizes);
      format_0_0_1_0_size_bytes = (format_0_0_1_0_size_bits%8 == 0) ? (uint8_t)floor(format_0_0_1_0_size_bits/8) : (uint8_t)(floor(format_0_0_1_0_size_bits/8) + 1);
      #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure)-> calculating dci format size for common searchSpaces with format css_dci_format=%d, format_0_0_1_0_size_bits=%d, format_0_0_1_0_size_bytes=%d\n",
                css_dci_format,format_0_0_1_0_size_bits,format_0_0_1_0_size_bytes);
      #endif
      // for aggregation level 4. The number of candidates (L2=4) will be calculated in function nr_dci_decoding_procedure0
      #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure)-> common searchSpaces with format css_dci_format=%d and aggregation_level=%d\n",
                css_dci_format,(1<<2));
      #endif
      old_dci_cnt = dci_cnt;
      nr_dci_decoding_procedure0(s,p,pdcch_vars, 1, nr_tti_rx, dci_alloc, eNB_id, ue->current_thread_id[nr_tti_rx], frame_parms, mi,
                crc_scrambled_values, 2,
                cformat0_0_and_1_0, uformat0_0_and_1_0,
                format_0_0_1_0_size_bits, format_0_0_1_0_size_bytes, &dci_cnt,
                &crc_scrambled_, &format_found_, &CCEmap0, &CCEmap1, &CCEmap2);
      if (dci_cnt != old_dci_cnt){
        format_0_0_1_0_size_bits = nr_dci_format_size(crc_scrambled_,16,n_RB_ULBWP,n_RB_DLBWP,dci_fields_sizes); // after decoding dci successfully we recalculate dci pdu size with correct crc scrambled to get the right field sizes
        old_dci_cnt = dci_cnt;
        for (int i=0; i<NBR_NR_DCI_FIELDS; i++)
          for (int j=0; j<NBR_NR_FORMATS; j++)
            dci_fields_sizes_cnt[dci_cnt-1][i][j]=dci_fields_sizes[i][j];
      }
      // for aggregation level 8. The number of candidates (L2=8) will be calculated in function nr_dci_decoding_procedure0
      #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure)-> common searchSpaces with format css_dci_format=%d and aggregation_level=%d\n",
                css_dci_format,(1<<3));
      #endif
      old_dci_cnt = dci_cnt;
      nr_dci_decoding_procedure0(s,p,pdcch_vars, 1, nr_tti_rx, dci_alloc, eNB_id, ue->current_thread_id[nr_tti_rx], frame_parms, mi,
                crc_scrambled_values, 3,
                cformat0_0_and_1_0, uformat0_0_and_1_0,
                format_0_0_1_0_size_bits, format_0_0_1_0_size_bytes, &dci_cnt,
                &crc_scrambled_, &format_found_, &CCEmap0, &CCEmap1, &CCEmap2);
      if (dci_cnt != old_dci_cnt){
        format_0_0_1_0_size_bits = nr_dci_format_size(crc_scrambled_,16,n_RB_ULBWP,n_RB_DLBWP,dci_fields_sizes); // after decoding dci successfully we recalculate dci pdu size with correct crc scrambled to get the right field sizes
        old_dci_cnt = dci_cnt;
        for (int i=0; i<NBR_NR_DCI_FIELDS; i++)
          for (int j=0; j<NBR_NR_FORMATS; j++)
            dci_fields_sizes_cnt[dci_cnt-1][i][j]=dci_fields_sizes[i][j];
      }
      // for aggregation level 16. The number of candidates (L2=16) will be calculated in function nr_dci_decoding_procedure0
      #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure)-> common searchSpaces with format css_dci_format=%d and aggregation_level=%d\n",
                css_dci_format,(1<<4));
      #endif
      old_dci_cnt = dci_cnt;
      nr_dci_decoding_procedure0(s,p,pdcch_vars, 1, nr_tti_rx, dci_alloc, eNB_id, ue->current_thread_id[nr_tti_rx], frame_parms, mi,
                crc_scrambled_values, 4,
                cformat0_0_and_1_0, uformat0_0_and_1_0,
                format_0_0_1_0_size_bits, format_0_0_1_0_size_bytes, &dci_cnt,
                &crc_scrambled_, &format_found_, &CCEmap0, &CCEmap1, &CCEmap2);
      if (dci_cnt != old_dci_cnt){
        format_0_0_1_0_size_bits = nr_dci_format_size(crc_scrambled_,16,n_RB_ULBWP,n_RB_DLBWP,dci_fields_sizes); // after decoding dci successfully we recalculate dci pdu size with correct crc scrambled to get the right field sizes
        old_dci_cnt = dci_cnt;
        for (int i=0; i<NBR_NR_DCI_FIELDS; i++)
          for (int j=0; j<NBR_NR_FORMATS; j++)
            dci_fields_sizes_cnt[dci_cnt-1][i][j]=dci_fields_sizes[i][j];
      }
    }

    // Type3-PDCCH  common search space for a DCI format with CRC scrambled by INT-RNTI, or SFI-RNTI,
    //    or TPC-PUSCH-RNTI, or TPC-PUCCH-RNTI, or TPC-SRS-RNTI, or C-RNTI, or CS-RNTI(s), or SP-CSI-RNTI
    if (css_dci_format == cformat2_0) {
      // for format2_0, first we calculate dci pdu size
      format_2_0_size_bits = nr_dci_format_size(_sfi_rnti,0,n_RB_ULBWP,n_RB_DLBWP,dci_fields_sizes);
      format_2_0_size_bytes = (format_2_0_size_bits%8 == 0) ? (uint8_t)floor(format_2_0_size_bits/8) : (uint8_t)(floor(format_2_0_size_bits/8) + 1);
      #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure)-> calculating dci format size for common searchSpaces with format css_dci_format=%d, format2_0_size_bits=%d, format2_0_size_bytes=%d\n",
                css_dci_format,format_2_0_size_bits,format_2_0_size_bytes);
      #endif
      // for aggregation level 1. The number of candidates (nrofCandidates-SFI) will be calculated in function nr_dci_decoding_procedure0
      old_dci_cnt = dci_cnt;
      nr_dci_decoding_procedure0(s,p,pdcch_vars, 1, nr_tti_rx, dci_alloc, eNB_id, ue->current_thread_id[nr_tti_rx], frame_parms, mi,
                crc_scrambled_values, 0,
                cformat2_0, uformat0_0_and_1_0,
                format_2_0_size_bits, format_2_0_size_bytes, &dci_cnt,
                &crc_scrambled_, &format_found_, &CCEmap0, &CCEmap1, &CCEmap2);
      if (dci_cnt != old_dci_cnt){
        old_dci_cnt = dci_cnt;
        for (int i=0; i<NBR_NR_DCI_FIELDS; i++)
          for (int j=0; j<NBR_NR_FORMATS; j++)
            dci_fields_sizes_cnt[dci_cnt-1][i][j]=dci_fields_sizes[i][j];
      }
      // for aggregation level 2. The number of candidates (nrofCandidates-SFI) will be calculated in function nr_dci_decoding_procedure0
      old_dci_cnt = dci_cnt;
      nr_dci_decoding_procedure0(s,p,pdcch_vars, 1, nr_tti_rx, dci_alloc, eNB_id, ue->current_thread_id[nr_tti_rx], frame_parms, mi,
                crc_scrambled_values, 1,
                cformat2_0, uformat0_0_and_1_0,
                format_2_0_size_bits, format_2_0_size_bytes, &dci_cnt,
                &crc_scrambled_, &format_found_, &CCEmap0, &CCEmap1, &CCEmap2);
      if (dci_cnt != old_dci_cnt){
        old_dci_cnt = dci_cnt;
        for (int i=0; i<NBR_NR_DCI_FIELDS; i++)
          for (int j=0; j<NBR_NR_FORMATS; j++)
            dci_fields_sizes_cnt[dci_cnt-1][i][j]=dci_fields_sizes[i][j];
      }
      // for aggregation level 4. The number of candidates (nrofCandidates-SFI) will be calculated in function nr_dci_decoding_procedure0
      old_dci_cnt = dci_cnt;
      nr_dci_decoding_procedure0(s,p,pdcch_vars, 1, nr_tti_rx, dci_alloc, eNB_id, ue->current_thread_id[nr_tti_rx], frame_parms, mi,
                crc_scrambled_values, 2,
                cformat2_0, uformat0_0_and_1_0,
                format_2_0_size_bits, format_2_0_size_bytes, &dci_cnt,
                &crc_scrambled_, &format_found_, &CCEmap0, &CCEmap1, &CCEmap2);
      if (dci_cnt != old_dci_cnt){
        old_dci_cnt = dci_cnt;
        for (int i=0; i<NBR_NR_DCI_FIELDS; i++)
          for (int j=0; j<NBR_NR_FORMATS; j++)
            dci_fields_sizes_cnt[dci_cnt-1][i][j]=dci_fields_sizes[i][j];
      }
      // for aggregation level 8. The number of candidates (nrofCandidates-SFI) will be calculated in function nr_dci_decoding_procedure0
      old_dci_cnt = dci_cnt;
      nr_dci_decoding_procedure0(s,p,pdcch_vars, 1, nr_tti_rx, dci_alloc, eNB_id, ue->current_thread_id[nr_tti_rx], frame_parms, mi,
                crc_scrambled_values, 3,
                cformat2_0, uformat0_0_and_1_0,
                format_2_0_size_bits, format_2_0_size_bytes, &dci_cnt,
                &crc_scrambled_, &format_found_, &CCEmap0, &CCEmap1, &CCEmap2);
      if (dci_cnt != old_dci_cnt){
        old_dci_cnt = dci_cnt;
        for (int i=0; i<NBR_NR_DCI_FIELDS; i++)
          for (int j=0; j<NBR_NR_FORMATS; j++)
            dci_fields_sizes_cnt[dci_cnt-1][i][j]=dci_fields_sizes[i][j];
      }
      // for aggregation level 16. The number of candidates (nrofCandidates-SFI) will be calculated in function nr_dci_decoding_procedure0
      old_dci_cnt = dci_cnt;
      nr_dci_decoding_procedure0(s,p,pdcch_vars, 1, nr_tti_rx, dci_alloc, eNB_id, ue->current_thread_id[nr_tti_rx], frame_parms, mi,
                crc_scrambled_values, 4,
                cformat2_0, uformat0_0_and_1_0,
                format_2_0_size_bits, format_2_0_size_bytes, &dci_cnt,
                &crc_scrambled_, &format_found_, &CCEmap0, &CCEmap1, &CCEmap2);
      if (dci_cnt != old_dci_cnt){
        old_dci_cnt = dci_cnt;
        for (int i=0; i<NBR_NR_DCI_FIELDS; i++)
          for (int j=0; j<NBR_NR_FORMATS; j++)
            dci_fields_sizes_cnt[dci_cnt-1][i][j]=dci_fields_sizes[i][j];
      }
    }
    if (css_dci_format == cformat2_1) {
      // for format2_1, first we calculate dci pdu size
      format_2_1_size_bits = nr_dci_format_size(_int_rnti,0,n_RB_ULBWP,n_RB_DLBWP,dci_fields_sizes);
      format_2_1_size_bytes = (format_2_1_size_bits%8 == 0) ? (uint8_t)floor(format_2_1_size_bits/8) : (uint8_t)(floor(format_2_1_size_bits/8) + 1);
      #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure)-> calculating dci format size for common searchSpaces with format css_dci_format=%d, format2_1_size_bits=%d, format2_1_size_bytes=%d\n",
                css_dci_format,format_2_1_size_bits,format_2_1_size_bytes);
      #endif
    }
    if (css_dci_format == cformat2_2) {
      // for format2_2, first we calculate dci pdu size
      format_2_2_size_bits = nr_dci_format_size(_tpc_pucch_rnti,0,n_RB_ULBWP,n_RB_DLBWP,dci_fields_sizes);
      format_2_2_size_bytes = (format_2_2_size_bits%8 == 0) ? (uint8_t)floor(format_2_2_size_bits/8) : (uint8_t)(floor(format_2_2_size_bits/8) + 1);
      #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure)-> calculating dci format size for common searchSpaces with format css_dci_format=%d, format2_2_size_bits=%d, format2_2_size_bytes=%d\n",
                css_dci_format,format_2_2_size_bits,format_2_2_size_bytes);
      #endif
    }
    if (css_dci_format == cformat2_3) {
      // for format2_1, first we calculate dci pdu size
      format_2_3_size_bits = nr_dci_format_size(_tpc_srs_rnti,0,n_RB_ULBWP,n_RB_DLBWP,dci_fields_sizes);
      format_2_3_size_bytes = (format_2_3_size_bits%8 == 0) ? (uint8_t)floor(format_2_3_size_bits/8) : (uint8_t)(floor(format_2_3_size_bits/8) + 1);
      #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure)-> calculating dci format size for common searchSpaces with format css_dci_format=%d, format2_3_size_bits=%d, format2_3_size_bytes=%d\n",
                css_dci_format,format_2_3_size_bits,format_2_3_size_bytes);
      #endif
    }
  } else { // UE-SPECIFIC SearchSpaceType assigned to current SearchSpace/CORESET
    // UE-specific search space for a DCI format with CRC scrambled by C-RNTI, or CS-RNTI(s), or SP-CSI-RNTI
    if (uss_dci_format == uformat0_0_and_1_0) {
      // for format0_0 and format1_0, first we calculate dci pdu size
      format_0_0_1_0_size_bits = nr_dci_format_size(_c_rnti,16,n_RB_ULBWP,n_RB_DLBWP,dci_fields_sizes);
      format_0_0_1_0_size_bytes = (format_0_0_1_0_size_bits%8 == 0) ? (uint8_t)floor(format_0_0_1_0_size_bits/8) : (uint8_t)(floor(format_0_0_1_0_size_bits/8) + 1);
      #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure)-> calculating dci format size for UE-specific searchSpaces with format uss_dci_format=%d, format_0_0_1_0_size_bits=%d, format_0_0_1_0_size_bytes=%d\n",
                css_dci_format,format_0_0_1_0_size_bits,format_0_0_1_0_size_bytes);
      #endif
      // blind decoding format0_0 for aggregation level 1. The number of candidates (nrofCandidates) will be calculated in function nr_dci_decoding_procedure0
      #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure)-> ue-Specific searchSpaces with format uss_dci_format=%d and aggregation level 1, format_0_0_1_0_size_bits=%d, format_0_0_1_0_size_bytes=%d\n",
                uss_dci_format,format_0_0_1_0_size_bits,format_0_0_1_0_size_bytes);
      #endif
      old_dci_cnt = dci_cnt;
/*
 * To be removed, just for unitary testing
 */
//#ifdef NR_PDCCH_DCI_DEBUG
//      printf("\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure)-> ### WE PROVOKE DCI DETECTION !!! ### old_dci_cnt=%d and dci_cnt=%d\n",
//              old_dci_cnt,dci_cnt);
//      dci_cnt++;
//#endif
/*
 * To be removed until here
 */
      nr_dci_decoding_procedure0(s,p,pdcch_vars, 0, nr_tti_rx, dci_alloc, eNB_id, ue->current_thread_id[nr_tti_rx], frame_parms, mi,
                crc_scrambled_values, 0,
                cformat0_0_and_1_0, uformat0_0_and_1_0,
                format_0_0_1_0_size_bits, format_0_0_1_0_size_bytes, &dci_cnt,
                &crc_scrambled_, &format_found_, &CCEmap0, &CCEmap1, &CCEmap2);
      if (dci_cnt != old_dci_cnt){
        old_dci_cnt = dci_cnt;
        for (int i=0; i<NBR_NR_DCI_FIELDS; i++)
          for (int j=0; j<NBR_NR_FORMATS; j++){
            dci_fields_sizes_cnt[dci_cnt-1][i][j]=dci_fields_sizes[i][j];
/*
 * To be removed, just for unitary testing
 */
//#ifdef NR_PDCCH_DCI_DEBUG
//            printf("dci_fields_sizes_cnt(%d,0,1][%d][%d]=(%d,%d,%d)\t\tdci_fields_sizes[%d][%d]=(%d)\n",
//              dci_cnt-1,i,j,dci_fields_sizes_cnt[dci_cnt-1][i][j],dci_fields_sizes_cnt[0][i][j],dci_fields_sizes_cnt[1][i][j],i,j,dci_fields_sizes[i][j]);
//#endif
/*
 * To be removed until here
 */
          }
      }
      // blind decoding format0_0 for aggregation level 2. The number of candidates (nrofCandidates) will be calculated in function nr_dci_decoding_procedure0
      #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure)-> ue-Specific searchSpaces with format uss_dci_format=%d and aggregation level 2, format_0_0_1_0_size_bits=%d, format_0_0_1_0_size_bytes=%d\n",
                uss_dci_format,format_0_0_1_0_size_bits,format_0_0_1_0_size_bytes);
      #endif
      old_dci_cnt = dci_cnt;
      nr_dci_decoding_procedure0(s,p,pdcch_vars, 0, nr_tti_rx, dci_alloc, eNB_id, ue->current_thread_id[nr_tti_rx], frame_parms, mi,
                crc_scrambled_values, 1,
                cformat0_0_and_1_0, uformat0_0_and_1_0,
                format_0_0_1_0_size_bits, format_0_0_1_0_size_bytes, &dci_cnt,
                &crc_scrambled_, &format_found_, &CCEmap0, &CCEmap1, &CCEmap2);
      if (dci_cnt != old_dci_cnt){
        old_dci_cnt = dci_cnt;
        for (int i=0; i<NBR_NR_DCI_FIELDS; i++)
          for (int j=0; j<NBR_NR_FORMATS; j++)
            dci_fields_sizes_cnt[dci_cnt-1][i][j]=dci_fields_sizes[i][j];
      }
      // blind decoding format0_0 for aggregation level 4. The number of candidates (nrofCandidates) will be calculated in function nr_dci_decoding_procedure0
      #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure)-> ue-Specific searchSpaces with format uss_dci_format=%d and aggregation level 4, format_0_0_1_0_size_bits=%d, format_0_0_1_0_size_bytes=%d\n",
                uss_dci_format,format_0_0_1_0_size_bits,format_0_0_1_0_size_bytes);
      #endif
      old_dci_cnt = dci_cnt;
      nr_dci_decoding_procedure0(s,p,pdcch_vars, 0, nr_tti_rx, dci_alloc, eNB_id, ue->current_thread_id[nr_tti_rx], frame_parms, mi,
                crc_scrambled_values, 2,
                cformat0_0_and_1_0, uformat0_0_and_1_0,
                format_0_0_1_0_size_bits, format_0_0_1_0_size_bytes, &dci_cnt,
                &crc_scrambled_, &format_found_, &CCEmap0, &CCEmap1, &CCEmap2);
      if (dci_cnt != old_dci_cnt){
        old_dci_cnt = dci_cnt;
        for (int i=0; i<NBR_NR_DCI_FIELDS; i++)
          for (int j=0; j<NBR_NR_FORMATS; j++)
            dci_fields_sizes_cnt[dci_cnt-1][i][j]=dci_fields_sizes[i][j];
      }
      // blind decoding format0_0 for aggregation level 8. The number of candidates (nrofCandidates) will be calculated in function nr_dci_decoding_procedure0
      #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure)-> ue-Specific searchSpaces with format uss_dci_format=%d and aggregation level 8, format_0_0_1_0_size_bits=%d, format_0_0_1_0_size_bytes=%d\n",
                uss_dci_format,format_0_0_1_0_size_bits,format_0_0_1_0_size_bytes);
      #endif
      old_dci_cnt = dci_cnt;
      nr_dci_decoding_procedure0(s,p,pdcch_vars, 0, nr_tti_rx, dci_alloc, eNB_id, ue->current_thread_id[nr_tti_rx], frame_parms, mi,
                crc_scrambled_values, 3,
                cformat0_0_and_1_0, uformat0_0_and_1_0,
                format_0_0_1_0_size_bits, format_0_0_1_0_size_bytes, &dci_cnt,
                &crc_scrambled_, &format_found_, &CCEmap0, &CCEmap1, &CCEmap2);
      if (dci_cnt != old_dci_cnt){
        old_dci_cnt = dci_cnt;
        for (int i=0; i<NBR_NR_DCI_FIELDS; i++)
          for (int j=0; j<NBR_NR_FORMATS; j++)
            dci_fields_sizes_cnt[dci_cnt-1][i][j]=dci_fields_sizes[i][j];
      }
      // blind decoding format0_0 for aggregation level 16. The number of candidates (nrofCandidates) will be calculated in function nr_dci_decoding_procedure0
      #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure)-> ue-Specific searchSpaces with format uss_dci_format=%d and aggregation level 16, format_0_0_1_0_size_bits=%d, format_0_0_1_0_size_bytes=%d\n",
                uss_dci_format,format_0_0_1_0_size_bits,format_0_0_1_0_size_bytes);
      #endif
      old_dci_cnt = dci_cnt;
      nr_dci_decoding_procedure0(s,p,pdcch_vars, 0, nr_tti_rx, dci_alloc, eNB_id, ue->current_thread_id[nr_tti_rx], frame_parms, mi,
                crc_scrambled_values, 4,
                cformat0_0_and_1_0, uformat0_0_and_1_0,
                format_0_0_1_0_size_bits, format_0_0_1_0_size_bytes, &dci_cnt,
                &crc_scrambled_, &format_found_, &CCEmap0, &CCEmap1, &CCEmap2);
      if (dci_cnt != old_dci_cnt){
        old_dci_cnt = dci_cnt;
        for (int i=0; i<NBR_NR_DCI_FIELDS; i++)
          for (int j=0; j<NBR_NR_FORMATS; j++)
            dci_fields_sizes_cnt[dci_cnt-1][i][j]=dci_fields_sizes[i][j];
      }
    }
    *crc_scrambled = crc_scrambled_;
    *format_found  = format_found_;
#ifdef NR_PDCCH_DCI_DEBUG
  printf("\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure)-> at the end crc_scrambled=%d and format_found=%d\n",*crc_scrambled,*format_found);
#endif
  /*if (uss_dci_format == uformat0_1_and_1_1) { // Not implemented yet. FIXME!!!
      // for format0_1, first we calculate dci pdu size
      format0_1_size_bits = nr_dci_format_size(format0_1,c_rnti,16,n_RB_ULBWP,n_RB_DLBWP,dci_fields_sizes);
      format0_1_size_bytes = (format0_1_size_bits%8 == 0) ? (uint8_t)floor(format0_1_size_bits/8) : (uint8_t)(floor(format0_1_size_bits/8) + 1);
      #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure)-> calculating dci format size for UE-specific searchSpaces with format uss_dci_format=%d, format0_1_size_bits=%d, format0_1_size_bytes=%d\n",
                css_dci_format,format0_1_size_bits,format0_1_size_bytes);
      #endif
      // blind decoding format0_1 for aggregation level 1.  The number of candidates (nrofCandidates) will be calculated in function nr_dci_decoding_procedure0
      // blind decoding format0_1 for aggregation level 2.  The number of candidates (nrofCandidates) will be calculated in function nr_dci_decoding_procedure0
      // blind decoding format0_1 for aggregation level 4.  The number of candidates (nrofCandidates) will be calculated in function nr_dci_decoding_procedure0
      // blind decoding format0_1 for aggregation level 8.  The number of candidates (nrofCandidates) will be calculated in function nr_dci_decoding_procedure0
      // blind decoding format0_1 for aggregation level 16. The number of candidates (nrofCandidates) will be calculated in function nr_dci_decoding_procedure0

      // for format1_1, first we calculate dci pdu size
      format1_1_size_bits = nr_dci_format_size(format1_1,c_rnti,16,n_RB_ULBWP,n_RB_DLBWP,dci_fields_sizes);
      format1_1_size_bytes = (format1_1_size_bits%8 == 0) ? (uint8_t)floor(format1_1_size_bits/8) : (uint8_t)(floor(format1_1_size_bits/8) + 1);
      #ifdef NR_PDCCH_DCI_DEBUG
        printf("\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure)-> calculating dci format size for UE-specific searchSpaces with format uss_dci_format=%d, format1_1_size_bits=%d, format1_1_size_bytes=%d\n",
                css_dci_format,format1_1_size_bits,format1_1_size_bytes);
      #endif
      // blind decoding format1_1 for aggregation level 1.  The number of candidates (nrofCandidates) will be calculated in function nr_dci_decoding_procedure0
      // blind decoding format1_1 for aggregation level 2.  The number of candidates (nrofCandidates) will be calculated in function nr_dci_decoding_procedure0
      // blind decoding format1_1 for aggregation level 4.  The number of candidates (nrofCandidates) will be calculated in function nr_dci_decoding_procedure0
      // blind decoding format1_1 for aggregation level 8.  The number of candidates (nrofCandidates) will be calculated in function nr_dci_decoding_procedure0
      // blind decoding format1_1 for aggregation level 16. The number of candidates (nrofCandidates) will be calculated in function nr_dci_decoding_procedure0
    }*/
  }
#ifdef NR_PDCCH_DCI_DEBUG
  printf("\t<-NR_PDCCH_DCI_DEBUG (nr_dci_decoding_procedure)-> at the end dci_cnt=%d \n",dci_cnt);
#endif
  return(dci_cnt);
}




#endif





/*
uint16_t dci_decoding_procedure(PHY_VARS_NR_UE *ue,
                                DCI_ALLOC_t *dci_alloc,
                                int do_common,
                                int16_t eNB_id,
                                uint8_t nr_tti_rx)
{

  uint8_t  dci_cnt=0,old_dci_cnt=0;
  uint32_t CCEmap0=0,CCEmap1=0,CCEmap2=0;
  NR_UE_PDCCH **pdcch_vars = ue->pdcch_vars[ue->current_thread_id[nr_tti_rx]];
  NR_DL_FRAME_PARMS *frame_parms  = &ue->frame_parms;
  uint8_t mi = get_mi(&ue->frame_parms,nr_tti_rx);
  uint16_t ra_rnti=99;
  uint8_t format0_found=0,format_c_found=0;
  uint8_t tmode = ue->transmission_mode[eNB_id];
  uint8_t frame_type = frame_parms->frame_type;
  uint8_t format1A_size_bits=0,format1A_size_bytes=0;
  uint8_t format1C_size_bits=0,format1C_size_bytes=0;
  uint8_t format0_size_bits=0,format0_size_bytes=0;
  uint8_t format1_size_bits=0,format1_size_bytes=0;
  uint8_t format2_size_bits=0,format2_size_bytes=0;
  uint8_t format2A_size_bits=0,format2A_size_bytes=0;
  dci_detect_mode_t mode = dci_detect_mode_select(&ue->frame_parms,nr_tti_rx);

  switch (frame_parms->N_RB_DL) {
  case 6:
    if (frame_type == TDD) {
      format1A_size_bits  = sizeof_DCI1A_1_5MHz_TDD_1_6_t;
      format1A_size_bytes = sizeof(DCI1A_1_5MHz_TDD_1_6_t);
      format1C_size_bits  = sizeof_DCI1C_1_5MHz_t;
      format1C_size_bytes = sizeof(DCI1C_1_5MHz_t);
      format0_size_bits  = sizeof_DCI0_1_5MHz_TDD_1_6_t;
      format0_size_bytes = sizeof(DCI0_1_5MHz_TDD_1_6_t);
      format1_size_bits  = sizeof_DCI1_1_5MHz_TDD_t;
      format1_size_bytes = sizeof(DCI1_1_5MHz_TDD_t);

      if (frame_parms->nb_antenna_ports_eNB == 2) {
        format2_size_bits  = sizeof_DCI2_1_5MHz_2A_TDD_t;
        format2_size_bytes = sizeof(DCI2_1_5MHz_2A_TDD_t);
        format2A_size_bits  = sizeof_DCI2A_1_5MHz_2A_TDD_t;
        format2A_size_bytes = sizeof(DCI2A_1_5MHz_2A_TDD_t);
      } else if (frame_parms->nb_antenna_ports_eNB == 4) {
        format2_size_bits  = sizeof_DCI2_1_5MHz_4A_TDD_t;
        format2_size_bytes = sizeof(DCI2_1_5MHz_4A_TDD_t);
        format2A_size_bits  = sizeof_DCI2A_1_5MHz_4A_TDD_t;
        format2A_size_bytes = sizeof(DCI2A_1_5MHz_4A_TDD_t);
      }
    } else {
      format1A_size_bits  = sizeof_DCI1A_1_5MHz_FDD_t;
      format1A_size_bytes = sizeof(DCI1A_1_5MHz_FDD_t);
      format1C_size_bits  = sizeof_DCI1C_1_5MHz_t;
      format1C_size_bytes = sizeof(DCI1C_1_5MHz_t);
      format0_size_bits  = sizeof_DCI0_1_5MHz_FDD_t;
      format0_size_bytes = sizeof(DCI0_1_5MHz_FDD_t);
      format1_size_bits  = sizeof_DCI1_1_5MHz_FDD_t;
      format1_size_bytes = sizeof(DCI1_1_5MHz_FDD_t);

      if (frame_parms->nb_antenna_ports_eNB == 2) {
        format2_size_bits  = sizeof_DCI2_1_5MHz_2A_FDD_t;
        format2_size_bytes = sizeof(DCI2_1_5MHz_2A_FDD_t);
        format2A_size_bits  = sizeof_DCI2A_1_5MHz_2A_FDD_t;
        format2A_size_bytes = sizeof(DCI2A_1_5MHz_2A_FDD_t);
      } else if (frame_parms->nb_antenna_ports_eNB == 4) {
        format2_size_bits  = sizeof_DCI2_1_5MHz_4A_FDD_t;
        format2_size_bytes = sizeof(DCI2_1_5MHz_4A_FDD_t);
        format2A_size_bits  = sizeof_DCI2A_1_5MHz_4A_FDD_t;
        format2A_size_bytes = sizeof(DCI2A_1_5MHz_4A_FDD_t);
      }
    }

    break;

  case 25:
  default:
    if (frame_type == TDD) {
      format1A_size_bits  = sizeof_DCI1A_5MHz_TDD_1_6_t;
      format1A_size_bytes = sizeof(DCI1A_5MHz_TDD_1_6_t);
      format1C_size_bits  = sizeof_DCI1C_5MHz_t;
      format1C_size_bytes = sizeof(DCI1C_5MHz_t);
      format0_size_bits  = sizeof_DCI0_5MHz_TDD_1_6_t;
      format0_size_bytes = sizeof(DCI0_5MHz_TDD_1_6_t);
      format1_size_bits  = sizeof_DCI1_5MHz_TDD_t;
      format1_size_bytes = sizeof(DCI1_5MHz_TDD_t);

      if (frame_parms->nb_antenna_ports_eNB == 2) {
        format2_size_bits  = sizeof_DCI2_5MHz_2A_TDD_t;
        format2_size_bytes = sizeof(DCI2_5MHz_2A_TDD_t);
        format2A_size_bits  = sizeof_DCI2A_5MHz_2A_TDD_t;
        format2A_size_bytes = sizeof(DCI2A_5MHz_2A_TDD_t);
      } else if (frame_parms->nb_antenna_ports_eNB == 4) {
        format2_size_bits  = sizeof_DCI2_5MHz_4A_TDD_t;
        format2_size_bytes = sizeof(DCI2_5MHz_4A_TDD_t);
        format2A_size_bits  = sizeof_DCI2A_5MHz_4A_TDD_t;
        format2A_size_bytes = sizeof(DCI2A_5MHz_4A_TDD_t);
      }
    } else {
      format1A_size_bits  = sizeof_DCI1A_5MHz_FDD_t;
      format1A_size_bytes = sizeof(DCI1A_5MHz_FDD_t);
      format1C_size_bits  = sizeof_DCI1C_5MHz_t;
      format1C_size_bytes = sizeof(DCI1C_5MHz_t);
      format0_size_bits  = sizeof_DCI0_5MHz_FDD_t;
      format0_size_bytes = sizeof(DCI0_5MHz_FDD_t);
      format1_size_bits  = sizeof_DCI1_5MHz_FDD_t;
      format1_size_bytes = sizeof(DCI1_5MHz_FDD_t);

      if (frame_parms->nb_antenna_ports_eNB == 2) {
        format2_size_bits  = sizeof_DCI2_5MHz_2A_FDD_t;
        format2_size_bytes = sizeof(DCI2_5MHz_2A_FDD_t);
        format2A_size_bits  = sizeof_DCI2A_5MHz_2A_FDD_t;
        format2A_size_bytes = sizeof(DCI2A_5MHz_2A_FDD_t);
      } else if (frame_parms->nb_antenna_ports_eNB == 4) {
        format2_size_bits  = sizeof_DCI2_5MHz_4A_FDD_t;
        format2_size_bytes = sizeof(DCI2_5MHz_4A_FDD_t);
        format2A_size_bits  = sizeof_DCI2A_5MHz_4A_FDD_t;
        format2A_size_bytes = sizeof(DCI2A_5MHz_4A_FDD_t);
      }
    }

    break;

  case 50:
    if (frame_type == TDD) {
      format1A_size_bits  = sizeof_DCI1A_10MHz_TDD_1_6_t;
      format1A_size_bytes = sizeof(DCI1A_10MHz_TDD_1_6_t);
      format1C_size_bits  = sizeof_DCI1C_10MHz_t;
      format1C_size_bytes = sizeof(DCI1C_10MHz_t);
      format0_size_bits  = sizeof_DCI0_10MHz_TDD_1_6_t;
      format0_size_bytes = sizeof(DCI0_10MHz_TDD_1_6_t);
      format1_size_bits  = sizeof_DCI1_10MHz_TDD_t;
      format1_size_bytes = sizeof(DCI1_10MHz_TDD_t);

      if (frame_parms->nb_antenna_ports_eNB == 2) {
        format2_size_bits  = sizeof_DCI2_10MHz_2A_TDD_t;
        format2_size_bytes = sizeof(DCI2_10MHz_2A_TDD_t);
        format2A_size_bits  = sizeof_DCI2A_10MHz_2A_TDD_t;
        format2A_size_bytes = sizeof(DCI2A_10MHz_2A_TDD_t);
      } else if (frame_parms->nb_antenna_ports_eNB == 4) {
        format2_size_bits  = sizeof_DCI2_10MHz_4A_TDD_t;
        format2_size_bytes = sizeof(DCI2_10MHz_4A_TDD_t);
        format2A_size_bits  = sizeof_DCI2A_10MHz_4A_TDD_t;
        format2A_size_bytes = sizeof(DCI2A_10MHz_4A_TDD_t);
      }
    } else {
      format1A_size_bits  = sizeof_DCI1A_10MHz_FDD_t;
      format1A_size_bytes = sizeof(DCI1A_10MHz_FDD_t);
      format1C_size_bits  = sizeof_DCI1C_10MHz_t;
      format1C_size_bytes = sizeof(DCI1C_10MHz_t);
      format0_size_bits  = sizeof_DCI0_10MHz_FDD_t;
      format0_size_bytes = sizeof(DCI0_10MHz_FDD_t);
      format1_size_bits  = sizeof_DCI1_10MHz_FDD_t;
      format1_size_bytes = sizeof(DCI1_10MHz_FDD_t);

      if (frame_parms->nb_antenna_ports_eNB == 2) {
        format2_size_bits  = sizeof_DCI2_10MHz_2A_FDD_t;
        format2_size_bytes = sizeof(DCI2_10MHz_2A_FDD_t);
        format2A_size_bits  = sizeof_DCI2A_10MHz_2A_FDD_t;
        format2A_size_bytes = sizeof(DCI2A_10MHz_2A_FDD_t);
      } else if (frame_parms->nb_antenna_ports_eNB == 4) {
        format2_size_bits  = sizeof_DCI2_10MHz_4A_FDD_t;
        format2_size_bytes = sizeof(DCI2_10MHz_4A_FDD_t);
        format2A_size_bits  = sizeof_DCI2A_10MHz_4A_FDD_t;
        format2A_size_bytes = sizeof(DCI2A_10MHz_4A_FDD_t);
      }
    }

    break;

  case 100:
    if (frame_type == TDD) {
      format1A_size_bits  = sizeof_DCI1A_20MHz_TDD_1_6_t;
      format1A_size_bytes = sizeof(DCI1A_20MHz_TDD_1_6_t);
      format1C_size_bits  = sizeof_DCI1C_20MHz_t;
      format1C_size_bytes = sizeof(DCI1C_20MHz_t);
      format0_size_bits  = sizeof_DCI0_20MHz_TDD_1_6_t;
      format0_size_bytes = sizeof(DCI0_20MHz_TDD_1_6_t);
      format1_size_bits  = sizeof_DCI1_20MHz_TDD_t;
      format1_size_bytes = sizeof(DCI1_20MHz_TDD_t);

      if (frame_parms->nb_antenna_ports_eNB == 2) {
        format2_size_bits  = sizeof_DCI2_20MHz_2A_TDD_t;
        format2_size_bytes = sizeof(DCI2_20MHz_2A_TDD_t);
        format2A_size_bits  = sizeof_DCI2A_20MHz_2A_TDD_t;
        format2A_size_bytes = sizeof(DCI2A_20MHz_2A_TDD_t);
      } else if (frame_parms->nb_antenna_ports_eNB == 4) {
        format2_size_bits  = sizeof_DCI2_20MHz_4A_TDD_t;
        format2_size_bytes = sizeof(DCI2_20MHz_4A_TDD_t);
        format2A_size_bits  = sizeof_DCI2A_20MHz_4A_TDD_t;
        format2A_size_bytes = sizeof(DCI2A_20MHz_4A_TDD_t);
      }
    } else {
      format1A_size_bits  = sizeof_DCI1A_20MHz_FDD_t;
      format1A_size_bytes = sizeof(DCI1A_20MHz_FDD_t);
      format1C_size_bits  = sizeof_DCI1C_20MHz_t;
      format1C_size_bytes = sizeof(DCI1C_20MHz_t);
      format0_size_bits  = sizeof_DCI0_20MHz_FDD_t;
      format0_size_bytes = sizeof(DCI0_20MHz_FDD_t);
      format1_size_bits  = sizeof_DCI1_20MHz_FDD_t;
      format1_size_bytes = sizeof(DCI1_20MHz_FDD_t);

      if (frame_parms->nb_antenna_ports_eNB == 2) {
        format2_size_bits  = sizeof_DCI2_20MHz_2A_FDD_t;
        format2_size_bytes = sizeof(DCI2_20MHz_2A_FDD_t);
        format2A_size_bits  = sizeof_DCI2A_20MHz_2A_FDD_t;
        format2A_size_bytes = sizeof(DCI2A_20MHz_2A_FDD_t);
      } else if (frame_parms->nb_antenna_ports_eNB == 4) {
        format2_size_bits  = sizeof_DCI2_20MHz_4A_FDD_t;
        format2_size_bytes = sizeof(DCI2_20MHz_4A_FDD_t);
        format2A_size_bits  = sizeof_DCI2A_20MHz_4A_FDD_t;
        format2A_size_bytes = sizeof(DCI2A_20MHz_4A_FDD_t);
      }
    }

    break;
  }

  if (do_common == 1) {
#ifdef DEBUG_DCI_DECODING
    printf("[DCI search] doing common search/format0 aggregation 4\n");
#endif

    if (ue->prach_resources[eNB_id])
      ra_rnti = ue->prach_resources[eNB_id]->ra_RNTI;

    // First check common search spaces at aggregation 4 (SI_RNTI, P_RNTI and RA_RNTI format 0/1A),
    // and UE_SPEC format0 (PUSCH) too while we're at it
    dci_decoding_procedure0(pdcch_vars,1,mode,nr_tti_rx,
                            dci_alloc,
                            eNB_id,
                            ue->current_thread_id[nr_tti_rx],
                            frame_parms,
                            mi,
                            ((ue->decode_SIB == 1) ? SI_RNTI : 0) ,
                            ra_rnti,
                            P_RNTI,
                            2,
                            format1A,
                            format1A,
                            format1A,
                            format0,
                            format1A_size_bits,
                            format1A_size_bytes,
                            &dci_cnt,
                            &format0_found,
                            &format_c_found,
                            &CCEmap0,
                            &CCEmap1,
                            &CCEmap2);

    if ((CCEmap0==0xffff) ||
        ((format0_found==1)&&(format_c_found==1)))
      return(dci_cnt);

    // Now check common search spaces at aggregation 4 (SI_RNTI,P_RNTI and RA_RNTI and C-RNTI format 1C),
    // and UE_SPEC format0 (PUSCH) too while we're at it
    dci_decoding_procedure0(pdcch_vars,1,mode,nr_tti_rx,
                            dci_alloc,
                            eNB_id,
                            ue->current_thread_id[nr_tti_rx],
                            frame_parms,
                            mi,
                            ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                            ra_rnti,
                            P_RNTI,
                            2,
                            format1C,
                            format1C,
                            format1C,
                            format1C,
                            format1C_size_bits,
                            format1C_size_bytes,
                            &dci_cnt,
                            &format0_found,
                            &format_c_found,
                            &CCEmap0,
                            &CCEmap1,
                            &CCEmap2);

    if ((CCEmap0==0xffff) ||
        ((format0_found==1)&&(format_c_found==1)))
      return(dci_cnt);

    // Now check common search spaces at aggregation 8 (SI_RNTI,P_RNTI and RA_RNTI format 1A),
    // and UE_SPEC format0 (PUSCH) too while we're at it
    //  printf("[DCI search] doing common search/format0 aggregation 3\n");
#ifdef DEBUG_DCI_DECODING
    printf("[DCI search] doing common search/format0 aggregation 8\n");
#endif
    dci_decoding_procedure0(pdcch_vars,1,mode,nr_tti_rx,
                            dci_alloc,
                            eNB_id,
                            ue->current_thread_id[nr_tti_rx],
                            frame_parms,
                            mi,
                            ((ue->decode_SIB == 1) ? SI_RNTI : 0),
			    P_RNTI,
                            ra_rnti,
                            3,
                            format1A,
                            format1A,
                            format1A,
                            format0,
                            format1A_size_bits,
                            format1A_size_bytes,
                            &dci_cnt,
                            &format0_found,
                            &format_c_found,
                            &CCEmap0,
                            &CCEmap1,
                            &CCEmap2);

    if ((CCEmap0==0xffff)||
        ((format0_found==1)&&(format_c_found==1)))
      return(dci_cnt);

    // Now check common search spaces at aggregation 8 (SI_RNTI and RA_RNTI and C-RNTI format 1C),
    // and UE_SPEC format0 (PUSCH) too while we're at it
    dci_decoding_procedure0(pdcch_vars,1,mode,nr_tti_rx,
                            dci_alloc,
                            eNB_id,
                            ue->current_thread_id[nr_tti_rx],
                            frame_parms,
                            mi,
                            ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                            ra_rnti,
			    P_RNTI,
			    3,
                            format1C,
                            format1C,
                            format1C,
                            format1C,
                            format1C_size_bits,
                            format1C_size_bytes,
                            &dci_cnt,
                            &format0_found,
                            &format_c_found,
                            &CCEmap0,
                            &CCEmap1,
                            &CCEmap2);
    //#endif

  }

  if (ue->UE_mode[eNB_id] <= PRACH)
    return(dci_cnt);

  if (ue->prach_resources[eNB_id])
    ra_rnti = ue->prach_resources[eNB_id]->ra_RNTI;

  // Now check UE_SPEC format0/1A ue_spec search spaces at aggregation 8
  //  printf("[DCI search] Format 0/1A aggregation 8\n");
  dci_decoding_procedure0(pdcch_vars,0,mode,
                          nr_tti_rx,
                          dci_alloc,
                          eNB_id,
                          ue->current_thread_id[nr_tti_rx],
                          frame_parms,
                          mi,
                          ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                          ra_rnti,
                          P_RNTI,
                          0,
                          format1A,
                          format1A,
                          format1A,
                          format0,
                          format0_size_bits,
                          format0_size_bytes,
                          &dci_cnt,
                          &format0_found,
                          &format_c_found,
                          &CCEmap0,
                          &CCEmap1,
                          &CCEmap2);

  if ((CCEmap0==0xffff)||
      ((format0_found==1)&&(format_c_found==1)))
    return(dci_cnt);

  //printf("[DCI search] Format 0 aggregation 1 dci_cnt %d\n",dci_cnt);

  if (dci_cnt == 0)
  {
  // Now check UE_SPEC format 0 search spaces at aggregation 4
  dci_decoding_procedure0(pdcch_vars,0,mode,
                          nr_tti_rx,
                          dci_alloc,
                          eNB_id,
                          ue->current_thread_id[nr_tti_rx],
                          frame_parms,
                          mi,
                          ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                          ra_rnti,
			  P_RNTI,
			  1,
                          format1A,
                          format1A,
                          format1A,
                          format0,
                          format0_size_bits,
                          format0_size_bytes,
                          &dci_cnt,
                          &format0_found,
                          &format_c_found,
                          &CCEmap0,
                          &CCEmap1,
                          &CCEmap2);

  if ((CCEmap0==0xffff)||
      ((format0_found==1)&&(format_c_found==1)))
    return(dci_cnt);


  //printf("[DCI search] Format 0 aggregation 2 dci_cnt %d\n",dci_cnt);
  }

  if (dci_cnt == 0)
  {
  // Now check UE_SPEC format 0 search spaces at aggregation 2
  dci_decoding_procedure0(pdcch_vars,0,mode,
                          nr_tti_rx,
                          dci_alloc,
                          eNB_id,
                          ue->current_thread_id[nr_tti_rx],
                          frame_parms,
                          mi,
                          ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                          ra_rnti,
			  P_RNTI,
                          2,
                          format1A,
                          format1A,
                          format1A,
                          format0,
                          format0_size_bits,
                          format0_size_bytes,
                          &dci_cnt,
                          &format0_found,
                          &format_c_found,
                          &CCEmap0,
                          &CCEmap1,
                          &CCEmap2);

  if ((CCEmap0==0xffff)||
      ((format0_found==1)&&(format_c_found==1)))
    return(dci_cnt);

  //printf("[DCI search] Format 0 aggregation 4 dci_cnt %d\n",dci_cnt);
  }

  if (dci_cnt == 0)
  {
  // Now check UE_SPEC format 0 search spaces at aggregation 1
  dci_decoding_procedure0(pdcch_vars,0,mode,
                          nr_tti_rx,
                          dci_alloc,
                          eNB_id,
                          ue->current_thread_id[nr_tti_rx],
                          frame_parms,
                          mi,
                          ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                          ra_rnti,
			  P_RNTI,
                          3,
                          format1A,
                          format1A,
                          format1A,
                          format0,
                          format0_size_bits,
                          format0_size_bytes,
                          &dci_cnt,
                          &format0_found,
                          &format_c_found,
                          &CCEmap0,
                          &CCEmap1,
                          &CCEmap2);

  if ((CCEmap0==0xffff)||
      ((format0_found==1)&&(format_c_found==1)))
    return(dci_cnt);

  //printf("[DCI search] Format 0 aggregation 8 dci_cnt %d\n",dci_cnt);

  }
  // These are for CRNTI based on transmission mode
  if ((tmode < 3) || (tmode == 7)) {
    // Now check UE_SPEC format 1 search spaces at aggregation 1
    old_dci_cnt=dci_cnt;
    dci_decoding_procedure0(pdcch_vars,0,mode,nr_tti_rx,
                            dci_alloc,
                            eNB_id,
                            ue->current_thread_id[nr_tti_rx],
                            frame_parms,
                            mi,
                            ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                            ra_rnti,
			    P_RNTI,
                            0,
                            format1A,
                            format1A,
                            format1A,
                            format1,
                            format1_size_bits,
                            format1_size_bytes,
                            &dci_cnt,
                            &format0_found,
                            &format_c_found,
                            &CCEmap0,
                            &CCEmap1,
                            &CCEmap2);
    //printf("[DCI search] Format 1 aggregation 1 dci_cnt %d\n",dci_cnt);

    if ((CCEmap0==0xffff) ||
        (format_c_found==1))
      return(dci_cnt);

    if (dci_cnt>old_dci_cnt)
      return(dci_cnt);

    // Now check UE_SPEC format 1 search spaces at aggregation 2
    old_dci_cnt=dci_cnt;
    dci_decoding_procedure0(pdcch_vars,0,mode,nr_tti_rx,
                            dci_alloc,
                            eNB_id,
                            ue->current_thread_id[nr_tti_rx],
                            frame_parms,
                            mi,
                            ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                            ra_rnti,
			    P_RNTI,
                            1,
                            format1A,
                            format1A,
                            format1A,
                            format1,
                            format1_size_bits,
                            format1_size_bytes,
                            &dci_cnt,
                            &format0_found,
                            &format_c_found,
                            &CCEmap0,
                            &CCEmap1,
                            &CCEmap2);
    //printf("[DCI search] Format 1 aggregation 2 dci_cnt %d\n",dci_cnt);

    if ((CCEmap0==0xffff)||
        (format_c_found==1))
      return(dci_cnt);

    if (dci_cnt>old_dci_cnt)
      return(dci_cnt);

    // Now check UE_SPEC format 1 search spaces at aggregation 4
    old_dci_cnt=dci_cnt;
    dci_decoding_procedure0(pdcch_vars,0,mode,nr_tti_rx,
                            dci_alloc,
                            eNB_id,
                            ue->current_thread_id[nr_tti_rx],
                            frame_parms,
                            mi,
                            ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                            ra_rnti,
			    P_RNTI,
			    2,
                            format1A,
                            format1A,
                            format1A,
                            format1,
                            format1_size_bits,
                            format1_size_bytes,
                            &dci_cnt,
                            &format0_found,
                            &format_c_found,
                            &CCEmap0,
                            &CCEmap1,
                            &CCEmap2);
    //printf("[DCI search] Format 1 aggregation 4 dci_cnt %d\n",dci_cnt);

    if ((CCEmap0==0xffff)||
        ((format0_found==1)&&(format_c_found==1)))
      return(dci_cnt);

    if (dci_cnt>old_dci_cnt)
      return(dci_cnt);

    //#ifdef ALL_AGGREGATION
    // Now check UE_SPEC format 1 search spaces at aggregation 8
    old_dci_cnt=dci_cnt;
    dci_decoding_procedure0(pdcch_vars,0,mode,nr_tti_rx,
                            dci_alloc,
                            eNB_id,
                            ue->current_thread_id[nr_tti_rx],
                            frame_parms,
                            mi,
                            ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                            ra_rnti,
			    P_RNTI,
                            3,
                            format1A,
                            format1A,
                            format1A,
                            format1,
                            format1_size_bits,
                            format1_size_bytes,
                            &dci_cnt,
                            &format0_found,
                            &format_c_found,
                            &CCEmap0,
                            &CCEmap1,
                            &CCEmap2);
    //printf("[DCI search] Format 1 aggregation 8 dci_cnt %d\n",dci_cnt);

    if ((CCEmap0==0xffff)||
        ((format0_found==1)&&(format_c_found==1)))
      return(dci_cnt);

    if (dci_cnt>old_dci_cnt)
      return(dci_cnt);

    //#endif //ALL_AGGREGATION
  } else if (tmode == 3) {


    LOG_D(PHY," Now check UE_SPEC format 2A_2A search aggregation 1 dci length: %d[bits] %d[bytes]\n",format2A_size_bits,format2A_size_bytes);
    // Now check UE_SPEC format 2A_2A search spaces at aggregation 1
    old_dci_cnt=dci_cnt;
    dci_decoding_procedure0(pdcch_vars,0,mode,
                            nr_tti_rx,
                            dci_alloc,
                            eNB_id,
                            ue->current_thread_id[nr_tti_rx],
                            frame_parms,
                            mi,
                            ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                            ra_rnti,
                            P_RNTI,
                            0,
                            format1A,
                            format1A,
                            format1A,
                            format2A,
                            format2A_size_bits,
                            format2A_size_bytes,
                            &dci_cnt,
                            &format0_found,
                            &format_c_found,
                            &CCEmap0,
                            &CCEmap1,
                            &CCEmap2);

    LOG_D(PHY," format 2A_2A search CCEmap0 %x, format0_found %d, format_c_found %d \n", CCEmap0, format0_found, format_c_found);
    if ((CCEmap0==0xffff)||
        ((format0_found==1)&&(format_c_found==1)))
      return(dci_cnt);

    LOG_D(PHY," format 2A_2A search dci_cnt %d, old_dci_cn t%d \n", dci_cnt, old_dci_cnt);
    if (dci_cnt>old_dci_cnt)
      return(dci_cnt);

    // Now check UE_SPEC format 2 search spaces at aggregation 2
    LOG_D(PHY," Now check UE_SPEC format 2A_2A search aggregation 2 dci length: %d[bits] %d[bytes]\n",format2A_size_bits,format2A_size_bytes);
    old_dci_cnt=dci_cnt;
    dci_decoding_procedure0(pdcch_vars,0,mode,
                            nr_tti_rx,
                            dci_alloc,
                            eNB_id,
                            ue->current_thread_id[nr_tti_rx],
                            frame_parms,
                            mi,
                            ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                            ra_rnti,
			    P_RNTI,
                            1,
                            format1A,
                            format1A,
                            format1A,
                            format2A,
                            format2A_size_bits,
                            format2A_size_bytes,
                            &dci_cnt,
                            &format0_found,
                            &format_c_found,
                            &CCEmap0,
                            &CCEmap1,
                            &CCEmap2);

    if ((CCEmap0==0xffff)||
        ((format0_found==1)&&(format_c_found==1)))
      return(dci_cnt);

    LOG_D(PHY," format 2A_2A search dci_cnt %d, old_dci_cn t%d \n", dci_cnt, old_dci_cnt);
    if (dci_cnt>old_dci_cnt)
      return(dci_cnt);

    // Now check UE_SPEC format 2_2A search spaces at aggregation 4
    LOG_D(PHY," Now check UE_SPEC format 2_2A search spaces at aggregation 4 \n");
    old_dci_cnt=dci_cnt;
    dci_decoding_procedure0(pdcch_vars,0,mode,
                            nr_tti_rx,
                            dci_alloc,
                            eNB_id,
                            ue->current_thread_id[nr_tti_rx],
                            frame_parms,
                            mi,
                            ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                            ra_rnti,
                            P_RNTI,
                            2,
                            format1A,
                            format1A,
                            format1A,
                            format2A,
                            format2A_size_bits,
                            format2A_size_bytes,
                            &dci_cnt,
                            &format0_found,
                            &format_c_found,
                            &CCEmap0,
                            &CCEmap1,
                            &CCEmap2);

    if ((CCEmap0==0xffff)||
        ((format0_found==1)&&(format_c_found==1)))
      return(dci_cnt);

    LOG_D(PHY," format 2A_2A search dci_cnt %d, old_dci_cn t%d \n", dci_cnt, old_dci_cnt);
    if (dci_cnt>old_dci_cnt)
      return(dci_cnt);

    //#ifdef ALL_AGGREGATION
    // Now check UE_SPEC format 2_2A search spaces at aggregation 8
    LOG_D(PHY," Now check UE_SPEC format 2_2A search spaces at aggregation 8 dci length: %d[bits] %d[bytes]\n",format2A_size_bits,format2A_size_bytes);
    old_dci_cnt=dci_cnt;
    dci_decoding_procedure0(pdcch_vars,0,mode,
                            nr_tti_rx,
                            dci_alloc,
                            eNB_id,
                            ue->current_thread_id[nr_tti_rx],
                            frame_parms,
                            mi,
                            ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                            ra_rnti,
			    P_RNTI,
                            3,
                            format1A,
                            format1A,
                            format1A,
                            format2A,
                            format2A_size_bits,
                            format2A_size_bytes,
                            &dci_cnt,
                            &format0_found,
                            &format_c_found,
                            &CCEmap0,
                            &CCEmap1,
                            &CCEmap2);
    //#endif
    if ((CCEmap0==0xffff)||
        ((format0_found==1)&&(format_c_found==1)))
      return(dci_cnt);

    LOG_D(PHY," format 2A_2A search dci_cnt %d, old_dci_cn t%d \n", dci_cnt, old_dci_cnt);
    if (dci_cnt>old_dci_cnt)
      return(dci_cnt);
  } else if (tmode == 4) {

    // Now check UE_SPEC format 2_2A search spaces at aggregation 1
    old_dci_cnt=dci_cnt;
    dci_decoding_procedure0(pdcch_vars,0,mode,
                            nr_tti_rx,
                            dci_alloc,
                            eNB_id,
                            ue->current_thread_id[nr_tti_rx],
                            frame_parms,
                            mi,
                            ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                            ra_rnti,
			    P_RNTI,
                            0,
                            format1A,
                            format1A,
                            format1A,
                            format2,
                            format2_size_bits,
                            format2_size_bytes,
                            &dci_cnt,
                            &format0_found,
                            &format_c_found,
                            &CCEmap0,
                            &CCEmap1,
                            &CCEmap2);

    if ((CCEmap0==0xffff)||
        ((format0_found==1)&&(format_c_found==1)))
      return(dci_cnt);

    if (dci_cnt>old_dci_cnt)
      return(dci_cnt);

    // Now check UE_SPEC format 2 search spaces at aggregation 2
    old_dci_cnt=dci_cnt;
    dci_decoding_procedure0(pdcch_vars,0,mode,
                            nr_tti_rx,
                            dci_alloc,
                            eNB_id,
                            ue->current_thread_id[nr_tti_rx],
                            frame_parms,
                            mi,
                            ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                            ra_rnti,
			    P_RNTI,
                            1,
                            format1A,
                            format1A,
                            format1A,
                            format2,
                            format2_size_bits,
                            format2_size_bytes,
                            &dci_cnt,
                            &format0_found,
                            &format_c_found,
                            &CCEmap0,
                            &CCEmap1,
                            &CCEmap2);

    if ((CCEmap0==0xffff)||
        ((format0_found==1)&&(format_c_found==1)))
      return(dci_cnt);

    if (dci_cnt>old_dci_cnt)
      return(dci_cnt);

    // Now check UE_SPEC format 2_2A search spaces at aggregation 4
    old_dci_cnt=dci_cnt;
    dci_decoding_procedure0(pdcch_vars,0,mode,
                            nr_tti_rx,
                            dci_alloc,
                            eNB_id,
                            ue->current_thread_id[nr_tti_rx],
                            frame_parms,
                            mi,
                            ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                            ra_rnti,
			    P_RNTI,
                            2,
                            format1A,
                            format1A,
                            format1A,
                            format2,
                            format2_size_bits,
                            format2_size_bytes,
                            &dci_cnt,
                            &format0_found,
                            &format_c_found,
                            &CCEmap0,
                            &CCEmap1,
                            &CCEmap2);

    if ((CCEmap0==0xffff)||
        ((format0_found==1)&&(format_c_found==1)))
      return(dci_cnt);

    if (dci_cnt>old_dci_cnt)
      return(dci_cnt);

    //#ifdef ALL_AGGREGATION
    // Now check UE_SPEC format 2_2A search spaces at aggregation 8
    old_dci_cnt=dci_cnt;
    dci_decoding_procedure0(pdcch_vars,0,mode,
                            nr_tti_rx,
                            dci_alloc,
                            eNB_id,
                            ue->current_thread_id[nr_tti_rx],
                            frame_parms,
                            mi,
                            ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                            ra_rnti,
			    P_RNTI,
                            3,
                            format1A,
                            format1A,
                            format1A,
                            format2,
                            format2_size_bits,
                            format2_size_bytes,
                            &dci_cnt,
                            &format0_found,
                            &format_c_found,
                            &CCEmap0,
                            &CCEmap1,
                            &CCEmap2);
    //#endif
  } else if ((tmode==5) || (tmode==6)) { // This is MU-MIMO

    // Now check UE_SPEC format 1E_2A_M10PRB search spaces aggregation 1
#ifdef DEBUG_DCI_DECODING
    LOG_I(PHY," MU-MIMO check UE_SPEC format 1E_2A_M10PRB\n");
#endif
    old_dci_cnt=dci_cnt;
    dci_decoding_procedure0(pdcch_vars,0,mode,
                            nr_tti_rx,
                            dci_alloc,
                            eNB_id,
                            ue->current_thread_id[nr_tti_rx],
                            frame_parms,
                            mi,
                            ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                            ra_rnti,
			    P_RNTI,
                            0,
                            format1A,
                            format1A,
                            format1A,
                            format1E_2A_M10PRB,
                            sizeof_DCI1E_5MHz_2A_M10PRB_TDD_t,
                            sizeof(DCI1E_5MHz_2A_M10PRB_TDD_t),
                            &dci_cnt,
                            &format0_found,
                            &format_c_found,
                            &CCEmap0,
                            &CCEmap1,
                            &CCEmap2);


    if ((CCEmap0==0xffff)||
        ((format0_found==1)&&(format_c_found==1)))
      return(dci_cnt);

    if (dci_cnt>old_dci_cnt)
      return(dci_cnt);

    // Now check UE_SPEC format 1E_2A_M10PRB search spaces aggregation 2
    old_dci_cnt=dci_cnt;
    dci_decoding_procedure0(pdcch_vars,0,mode,
                            nr_tti_rx,
                            dci_alloc,
                            eNB_id,
                            ue->current_thread_id[nr_tti_rx],
                            frame_parms,
                            mi,
                            ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                            ra_rnti,
			    P_RNTI,
                            1,
                            format1A,
                            format1A,
                            format1A,
                            format1E_2A_M10PRB,
                            sizeof_DCI1E_5MHz_2A_M10PRB_TDD_t,
                            sizeof(DCI1E_5MHz_2A_M10PRB_TDD_t),
                            &dci_cnt,
                            &format0_found,
                            &format_c_found,
                            &CCEmap0,
                            &CCEmap1,
                            &CCEmap2);

    if ((CCEmap0==0xffff)||
        ((format0_found==1)&&(format_c_found==1)))
      return(dci_cnt);

    if (dci_cnt>old_dci_cnt)
      return(dci_cnt);

    // Now check UE_SPEC format 1E_2A_M10PRB search spaces aggregation 4
    old_dci_cnt=dci_cnt;
    dci_decoding_procedure0(pdcch_vars,0,mode,
                            nr_tti_rx,
                            dci_alloc,
                            eNB_id,
                            ue->current_thread_id[nr_tti_rx],
                            frame_parms,
                            mi,
                            ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                            ra_rnti,
			    P_RNTI,
                            2,
                            format1A,
                            format1A,
                            format1A,
                            format1E_2A_M10PRB,
                            sizeof_DCI1E_5MHz_2A_M10PRB_TDD_t,
                            sizeof(DCI1E_5MHz_2A_M10PRB_TDD_t),
                            &dci_cnt,
                            &format0_found,
                            &format_c_found,
                            &CCEmap0,
                            &CCEmap1,
                            &CCEmap2);

    if ((CCEmap0==0xffff)||
        ((format0_found==1)&&(format_c_found==1)))
      return(dci_cnt);

    if (dci_cnt>old_dci_cnt)
      return(dci_cnt);

    //#ifdef ALL_AGGREGATION

    // Now check UE_SPEC format 1E_2A_M10PRB search spaces at aggregation 8
    old_dci_cnt=dci_cnt;
    dci_decoding_procedure0(pdcch_vars,0,mode,
                            nr_tti_rx,
                            dci_alloc,
                            eNB_id,
                            ue->current_thread_id[nr_tti_rx],
                            frame_parms,
                            mi,
                            ((ue->decode_SIB == 1) ? SI_RNTI : 0),
                            ra_rnti,
			    P_RNTI,
                            3,
                            format1A,
                            format1A,
                            format1A,
                            format1E_2A_M10PRB,
                            sizeof_DCI1E_5MHz_2A_M10PRB_TDD_t,
                            sizeof(DCI1E_5MHz_2A_M10PRB_TDD_t),
                            &dci_cnt,
                            &format0_found,
                            &format_c_found,
                            &CCEmap0,
                            &CCEmap1,
                            &CCEmap2);

    if ((CCEmap0==0xffff)||
        ((format0_found==1)&&(format_c_found==1)))
      return(dci_cnt);

    if (dci_cnt>old_dci_cnt)
      return(dci_cnt);

    //#endif  //ALL_AGGREGATION

  }

  return(dci_cnt);
}
*/
#ifdef PHY_ABSTRACTION
uint16_t dci_decoding_procedure_emul(NR_UE_PDCCH **pdcch_vars,
                                     uint8_t num_ue_spec_dci,
                                     uint8_t num_common_dci,
                                     DCI_ALLOC_t *dci_alloc_tx,
                                     DCI_ALLOC_t *dci_alloc_rx,
                                     int16_t eNB_id)
{

  uint8_t  dci_cnt=0,i;

  memcpy(dci_alloc_rx,dci_alloc_tx,num_common_dci*sizeof(DCI_ALLOC_t));
  dci_cnt = num_common_dci;
  LOG_D(PHY,"[DCI][EMUL] : num_common_dci %d\n",num_common_dci);

  for (i=num_common_dci; i<(num_ue_spec_dci+num_common_dci); i++) {
    LOG_D(PHY,"[DCI][EMUL] Checking dci %d => %x format %d (bit 0 %d)\n",i,pdcch_vars[eNB_id]->crnti,dci_alloc_tx[i].format,
          dci_alloc_tx[i].dci_pdu[0]&0x80);

    if (dci_alloc_tx[i].rnti == pdcch_vars[eNB_id]->crnti) {
      memcpy(dci_alloc_rx+dci_cnt,dci_alloc_tx+i,sizeof(DCI_ALLOC_t));
      dci_cnt++;
    }
  }


  return(dci_cnt);
}
#endif
